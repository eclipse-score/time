/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/
#include "score/time_slave/src/gptp/details/pdelay_measurer.h"
#include "score/time_slave/src/gptp/details/frame_codec.h"

#include <arpa/inet.h>
#include <array>
#include <cstring>

namespace score
{
namespace ts
{
namespace details
{

PeerDelayMeasurer::PeerDelayMeasurer(const ClockIdentity& local_identity, std::uint8_t domain) noexcept
    : local_identity_{local_identity}, domain_{domain}
{
}

int PeerDelayMeasurer::SendRequest(RawSocket& socket)
{
    PTPMessage req{};
    req.ptpHdr.tsmt = kPtpMsgtypePdelayReq | kPtpTransportSpecific;
    req.ptpHdr.version = kPtpVersion;
    req.ptpHdr.domainNumber = domain_;
    req.ptpHdr.messageLength = htons(sizeof(PdelayReqBody));
    req.ptpHdr.flagField[0] = 0;
    req.ptpHdr.flagField[1] = 0;
    req.ptpHdr.correctionField = 0;
    req.ptpHdr.reserved2 = 0;
    req.ptpHdr.sourcePortIdentity.clockIdentity = local_identity_;
    req.ptpHdr.sourcePortIdentity.portNumber = htons(0x0001U);
    req.ptpHdr.sequenceId = htons(seqnum_);
    req.ptpHdr.controlField = static_cast<std::uint8_t>(ControlField::kOther);
    req.ptpHdr.logMessageInterval = 0x7F;

    // Save a copy with host-byte-order fields for later matching.
    // portNumber and sequenceId are stored in host byte order so that
    // memcmp/equality checks in ComputeAndStoreUnlocked() agree with the
    // host-order values produced by GgtpMessageParser (LoadU16/ntohs).
    {
        std::lock_guard<std::mutex> lk(mutex_);
        req_ = req;
        req_.ptpHdr.sequenceId = seqnum_;
        req_.ptpHdr.sourcePortIdentity.portNumber = 0x0001U;  // host byte order
        req_.sendHardwareTS = TmvT{-1};  // sentinel: TX timestamp pending
        resp_count_ = 0U;
        ++seqnum_;  // uint16_t: wraps naturally at 0xFFFF
    }

    // Derive the source MAC from the EUI-64 ClockIdentity (reverse EUI-48→EUI-64
    // expansion: OUI = id[0..2], vendor = id[5..7]).
    const std::array<std::uint8_t, kMacAddrLen> src_mac = {local_identity_.id[0],
                                                            local_identity_.id[1],
                                                            local_identity_.id[2],
                                                            local_identity_.id[5],
                                                            local_identity_.id[6],
                                                            local_identity_.id[7]};

    // Use a separate stack buffer — never alias the PTPMessage object itself as a
    // raw frame buffer; AddEthernetHeader() shifts the payload in-place and would
    // write beyond sizeof(PTPMessage).
    std::uint8_t buf[2048]{};
    unsigned int len = sizeof(PdelayReqBody);
    std::memcpy(buf, &req, len);

    FrameCodec codec;
    if (!codec.AddEthernetHeader(buf, len, src_mac, sizeof(buf)))
        return -1;

    ::timespec hwts{};
    const int r = socket.Send(buf, static_cast<int>(len), hwts);
    if (r > 0)
    {
        std::lock_guard<std::mutex> lk(mutex_);
        req_.sendHardwareTS = TmvT{static_cast<std::int64_t>(hwts.tv_sec) * kNsPerSec + hwts.tv_nsec};
    }
    return r;
}

void PeerDelayMeasurer::OnResponse(const PTPMessage& msg)
{
    std::lock_guard<std::mutex> lk(mutex_);
    if (msg.ptpHdr.sequenceId != req_.ptpHdr.sequenceId)
        return;
    ++resp_count_;
    resp_ = msg;
}

void PeerDelayMeasurer::OnResponseFollowUp(const PTPMessage& msg)
{
    std::lock_guard<std::mutex> lk(mutex_);
    if (msg.ptpHdr.sequenceId != req_.ptpHdr.sequenceId)
        return;
    resp_fup_ = msg;
    ComputeAndStoreUnlocked();
}

void PeerDelayMeasurer::ComputeAndStoreUnlocked() noexcept
{
    if (resp_count_ > 1U)  // multiple responses → non-time-aware bridge detected
        return;
    if (req_.ptpHdr.sequenceId != resp_.ptpHdr.sequenceId)
        return;
    if (resp_.ptpHdr.sequenceId != resp_fup_.ptpHdr.sequenceId)
        return;

    // Reject if t1 has not been recorded yet (TX timestamp still pending after Send()).
    // Without this guard, a response arriving in the race window between the first
    // lock release and the sendHardwareTS assignment would produce a garbage delay.
    // Sentinel value -1 means "TX timestamp pending"; 0 is a valid timestamp (t=0).
    if (req_.sendHardwareTS.ns < 0)
        return;

    if (std::memcmp(&resp_.pdelay_resp.requestingPortIdentity,
                    &req_.ptpHdr.sourcePortIdentity,
                    sizeof(PortIdentity)) != 0)
        return;
    if (std::memcmp(&resp_fup_.pdelay_resp_fup.requestingPortIdentity,
                    &req_.ptpHdr.sourcePortIdentity,
                    sizeof(PortIdentity)) != 0)
        return;

    // t1 = BPF_T_BINTIME (PHC) send timestamp of our Pdelay_Req (TX loopback fd)
    const TmvT t1 = req_.sendHardwareTS;
    // t2 = remote receipt time (from Pdelay_Resp body: requestReceiptTimestamp)
    const TmvT t2 = resp_.parseMessageTs;
    // t3 = remote send time (from Pdelay_Resp_FUP body) + corrections
    const TmvT t3 = resp_fup_.parseMessageTs;
    const TmvT c1 = CorrectionToTmv(resp_.ptpHdr.correctionField);
    const TmvT c2 = CorrectionToTmv(resp_fup_.ptpHdr.correctionField);
    const TmvT t3c = TmvT{t3.ns + c1.ns + c2.ns};
    // t4 = BPF_T_BINTIME (PHC) receive timestamp of Pdelay_Resp (main BPF fd)
    const TmvT t4 = resp_.recvHardwareTS;

    const std::int64_t delay = ((t2.ns - t1.ns) + (t4.ns - t3c.ns)) / 2LL;

    if (delay < 0)
        return;

    PDelayResult r{};
    r.path_delay_ns = delay;
    r.valid = true;

    score::ts::GptpIpcPDelayData& d = r.pdelay_data;
    d.request_origin_timestamp = static_cast<std::uint64_t>(t1.ns);
    d.request_receipt_timestamp = static_cast<std::uint64_t>(t2.ns);
    d.response_origin_timestamp = static_cast<std::uint64_t>(t3.ns);
    d.response_receipt_timestamp = static_cast<std::uint64_t>(t4.ns);
    d.reference_global_timestamp = static_cast<std::uint64_t>(t3c.ns);
    d.reference_local_timestamp = static_cast<std::uint64_t>(t4.ns);
    d.sequence_id = resp_.ptpHdr.sequenceId;
    d.pdelay = static_cast<std::uint64_t>(delay);
    d.req_port_number = req_.ptpHdr.sourcePortIdentity.portNumber;
    d.req_clock_identity = ClockIdentityToU64(req_.ptpHdr.sourcePortIdentity.clockIdentity);
    d.resp_port_number = resp_.ptpHdr.sourcePortIdentity.portNumber;
    d.resp_clock_identity = ClockIdentityToU64(resp_.ptpHdr.sourcePortIdentity.clockIdentity);

    result_ = r;
    resp_count_ = 0U;
}

PDelayResult PeerDelayMeasurer::GetResult() const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return result_;
}

}  // namespace details
}  // namespace ts
}  // namespace score
