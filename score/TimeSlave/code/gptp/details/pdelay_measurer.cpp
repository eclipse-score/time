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
#include "score/TimeSlave/code/gptp/details/pdelay_measurer.h"
#include "score/TimeSlave/code/gptp/details/frame_codec.h"

#include <arpa/inet.h>
#include <cstring>

namespace score
{
namespace ts
{
namespace details
{

PeerDelayMeasurer::PeerDelayMeasurer(
    const ClockIdentity& local_identity) noexcept
    : local_identity_{local_identity}
{
}

int PeerDelayMeasurer::SendRequest(IRawSocket& socket)
{
    PTPMessage req{};
    req.ptpHdr.tsmt          = kPtpMsgtypePdelayReq | kPtpTransportSpecific;
    req.ptpHdr.version       = kPtpVersion;
    req.ptpHdr.domainNumber  = 0;
    req.ptpHdr.messageLength = htons(sizeof(PdelayReqBody));
    req.ptpHdr.flagField[0]  = 0;
    req.ptpHdr.flagField[1]  = 0;
    req.ptpHdr.correctionField = 0;
    req.ptpHdr.reserved2     = 0;
    req.ptpHdr.sourcePortIdentity.clockIdentity = local_identity_;
    req.ptpHdr.sourcePortIdentity.portNumber    = htons(0x0001U);
    req.ptpHdr.sequenceId    = htons(static_cast<std::uint16_t>(seqnum_));
    req.ptpHdr.controlField  = kCtlOther;
    req.ptpHdr.logMessageInterval = 0x7F;

    // Save a copy with host-byte-order sequence ID for later matching
    {
        std::lock_guard<std::mutex> lk(mutex_);
        req_                              = req;
        req_.ptpHdr.sequenceId            = static_cast<std::uint16_t>(seqnum_);
    }
    ++seqnum_;

    auto buf = reinterpret_cast<std::uint8_t*>(&req);
    unsigned int len = sizeof(PdelayReqBody);
    FrameCodec codec;
    if (!codec.AddEthernetHeader(buf, len))
        return -1;

    ::timespec hwts{};
    const int r = socket.Send(buf, static_cast<int>(len), hwts);
    if (r > 0)
    {
        std::lock_guard<std::mutex> lk(mutex_);
        req_.sendHardwareTS = TmvT{
            static_cast<std::int64_t>(hwts.tv_sec) * kNsPerSec + hwts.tv_nsec};
    }
    return r;
}

void PeerDelayMeasurer::OnResponse(const PTPMessage& msg)
{
    std::lock_guard<std::mutex> lk(mutex_);
    resp_ = msg;
}

void PeerDelayMeasurer::OnResponseFollowUp(const PTPMessage& msg)
{
    {
        std::lock_guard<std::mutex> lk(mutex_);
        resp_fup_ = msg;
    }
    ComputeAndStore();
}

void PeerDelayMeasurer::ComputeAndStore() noexcept
{
    std::lock_guard<std::mutex> lk(mutex_);

    // All three messages must share the same sequence ID
    if (req_.ptpHdr.sequenceId != resp_.ptpHdr.sequenceId)
        return;
    if (resp_.ptpHdr.sequenceId != resp_fup_.ptpHdr.sequenceId)
        return;

    // t1 = HW send timestamp of our Pdelay_Req
    const TmvT t1 = req_.sendHardwareTS;
    // t2 = remote receipt time (from Pdelay_Resp body: requestReceiptTimestamp)
    const TmvT t2 = resp_.parseMessageTs;
    // t3 = remote send time (from Pdelay_Resp_FUP body) + corrections
    const TmvT t3  = resp_fup_.parseMessageTs;
    const TmvT c1  = CorrectionToTmv(resp_.ptpHdr.correctionField);
    const TmvT c2  = CorrectionToTmv(resp_fup_.ptpHdr.correctionField);
    const TmvT t3c = TmvT{t3.ns + c1.ns + c2.ns};
    // t4 = local HW receive timestamp of Pdelay_Resp
    const TmvT t4 = resp_.recvHardwareTS;

    const std::int64_t delay = ((t2.ns - t1.ns) + (t4.ns - t3c.ns)) / 2LL;

    PDelayResult r{};
    r.path_delay_ns = delay;
    r.valid         = true;

    score::td::PDelayData& d            = r.pdelay_data;
    d.request_origin_timestamp          = static_cast<std::uint64_t>(t1.ns);
    d.request_receipt_timestamp         = static_cast<std::uint64_t>(t2.ns);
    d.response_origin_timestamp         = static_cast<std::uint64_t>(t3.ns);
    d.response_receipt_timestamp        = static_cast<std::uint64_t>(t4.ns);
    d.reference_global_timestamp        = static_cast<std::uint64_t>(t3c.ns);
    d.reference_local_timestamp         = static_cast<std::uint64_t>(t4.ns);
    d.sequence_id                       = resp_.ptpHdr.sequenceId;
    d.pdelay                            = static_cast<std::uint64_t>(delay);
    d.req_port_number                   =
        req_.ptpHdr.sourcePortIdentity.portNumber;
    d.req_clock_identity                =
        ClockIdentityToU64(req_.ptpHdr.sourcePortIdentity.clockIdentity);
    d.resp_port_number                  =
        resp_.ptpHdr.sourcePortIdentity.portNumber;
    d.resp_clock_identity               =
        ClockIdentityToU64(resp_.ptpHdr.sourcePortIdentity.clockIdentity);

    result_ = r;
}

PDelayResult PeerDelayMeasurer::GetResult() const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return result_;
}

}  // namespace details
}  // namespace ts
}  // namespace score
