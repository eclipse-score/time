# QNX-Specific Code Inventory: score_time

> **Context:** This document identifies and catalogs all code segments within this repository that contain native QNX API dependencies and cannot be compiled, executed, or tested in a standard Linux environment. This inventory is critical to justify our Linux-based code coverage strategy to safety assessors for the end-of-year QM Release.

---

## Metadata
*   **Component / Module:** `score_time`
*   **Owner Team:** `COM`
*   **Technical Contact:** `Jochen Speck / JochenMatthias.Speck@etas.com`
*   **Date of Last Assessment:** `2026-09-24`

---

## Summary Metrics
*   **Total Files Scanned:** `333 (124 .cpp; 120 .h; 89 BUILD)`
*   **Files with QNX Dependencies:** `13 (9 .cpp/.h; 4 QNX image/test configuration)`
*   **Estimated Total Lines of Code (LoC) Affected:** `1222 LoC (843 .cpp/.h; 379 QNX image/test configuration)`
*   **Affected % of Repository Codebase:** `5% (843 of 15632 .cpp/.h LoC)`

> **LoC basis:** Non-blank, non-comment lines. Only production sources are counted; unit tests, mocks and `BUILD` files are excluded, except in the QNX reference integration package where the `BUILD` file is itself the QNX image definition.

---

## QNX Dependency Inventory

| File Path & Line Range | QNX API / Header Dependency | Est. LoC | Root Cause (Why is it not testable on Linux?) | Verification Alternative (How is this tested instead?) |
|---|---|---|---|---|
|[score/time/high_res_steady_time/src/details/qtime/**](../../score/time/high_res_steady_time/src/details/qtime/)|1. `#include <sys/syspage.h>` : `SYSPAGE_ENTRY(qtime)->cycles_per_sec`.<br>2. `#include "score/os/qnx/neutrino.h"` : `score::os::qnx::Neutrino::instance().ClockCycles()`.|113|Contains QNX platform-specific implementations of functions used in the high resolution steady clock.|QNX-only unit tests (`target_compatible_with = ["@platforms//os:qnx"]`): `high_res_steady_qclock_test`, `tick_provider_test`, `factory_test`.|
|[score/time_slave/src/gptp/platform/qnx/**](../../score/time_slave/src/gptp/platform/qnx/)|1. `#include <net/bpf.h>` : `/dev/bpf`, `BIOCSETIF`, `BIOCSSEESENT`, `BIOCIMMEDIATE`, `BIOCPROMISC`, `BIOCSTSTAMP`, `BIOCSETF`, `BIOCGBLEN`.<br>2. `#include <net/if.h>` : `IFNAMSIZ`, `struct ifreq`, `IFF_PROMISC`, `IFF_ALLMULTI`, `SIOCGIFFLAGS`, `SIOCSIFFLAGS`, `SIOCADDMULTI`.<br>3. `#include <net/if_dl.h>` : `AF_LINK`, `struct sockaddr_dl`, `LLADDR`.<br>4. Qualcomm EMAC driver ioctls via `struct ifdrv` / `SIOCGDRVSPEC`: `PTP_GET_TIME` (0x102), `PTP_SET_TIME` (0x103), `EMAC_PTP_ADJ_FREQ_PPM` (52).|724|Contains QNX platform-specific implementations of the raw socket (BPF frame RX/TX with hardware timestamps), network identity (MAC address lookup) and PHC adjuster (EMAC clock step/frequency adjustment) used by the time slave.|No unit tests exercise these sources: `raw_socket_test` is Linux-only (`target_compatible_with = ["@platforms//os:linux"]`) and `bpf_device_path_test.cpp` has no Bazel target and includes a header that does not exist. The only QNX verification is the `time_slave_qnx_ref_integ` QEMU smoke test (binary/config deployment and config parsing).|
|[score/time_slave/src/gptp/details/ptp_types.h#L25-L30](../../score/time_slave/src/gptp/details/ptp_types.h#L25-L30)|None (preprocessor guard for QNX definition).|6| A simple data struct must be defined which is otherwise available via `#include <linux/if_ether.h>`.|Unnecessary due to trivial definition.|
|[score/time_slave/tests/reference_integration/qnx/**](../../score/time_slave/tests/reference_integration/qnx/)|1. `qnx_ifs` rule (`@score_rules_imagefs//rules/qnx:ifs.bzl`) : QNX SDP `mkifs` image build.<br>2. `init.build` / `tools.build` : `procnto-smp-instr`, `startup-x86`, `procmgr_symlink`, `io-sock` with `devs-vtnet_pci.so`, `devc-ser8250`, `pci-server`, `slogger2`, and QNX runtime libraries (`ldqnx-64.so.2`, `libsocket`, etc.).|379|QNX IFS boot image definition and deployed configuration for the `time_slave_qnx_ref_integ` test. All targets are `target_compatible_with = ["@platforms//os:qnx"]` and require the QNX SDP toolchain and licence to build.|Not applicable (test infrastructure, not production code). The image is built and booted under QEMU by `time_slave_qnx_ref_integ`, which verifies deployment and config parsing of the QNX binary.|

---

## Verification Strategy for Excluded Code
> **Status:** Not yet assessed. This section is intentionally deferred beyond the current revision of this inventory.
>
> Until then, the current verification of each excluded item is recorded in the *Verification Alternative* column of the [QNX Dependency Inventory](#qnx-dependency-inventory).

---

## Action Items / Refactoring Opportunities
> **Status:** Not yet assessed. This section is intentionally deferred beyond the current revision of this inventory.
