..
   # *******************************************************************************
   # Copyright (c) 2026 Contributors to the Eclipse Foundation
   #
   # See the NOTICE file(s) distributed with this work for additional
   # information regarding copyright ownership.
   #
   # This program and the accompanying materials are made available under the
   # terms of the Apache License Version 2.0 which is available at
   # https://www.apache.org/licenses/LICENSE-2.0
   #
   # SPDX-License-Identifier: Apache-2.0
   # *******************************************************************************

Test Cases
==========

This page lists verification test cases for the ``score::time`` module, organized by functional requirement.


FR-1 Vehicle Time Synchronization
---------------------------------


TC-FR1-001 — gPTP Synchronization Establishment Verification
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Log + Packet Capture
:Priority: High

**Preconditions**

1. Test device is powered on and started
2. A gPTP Time Master is available in the network
3. Wireshark/tcpdump is deployed on test PC or mirror port
4. Device logs are accessible

**Test Steps**

1. Start Wireshark on test PC, filter: eth.type == 0x88F7
2. Start score::time service on target device
3. Wait 60s
4. Check captured gPTP packets in Wireshark
5. Execute log view command on device
6. Record AccuracyQualifier status in logs

**Expected Result**

**Packet Capture**

- ✓ Pdelay_Req packets sent by this device are visible
- ✓ Pdelay_Resp, Sync, Follow_Up packets received by this device are visible
- ✓ No Sync packets sent by this device (device is Slave)


**Log Verification**

- ✓ Log shows AccuracyQualifier changed to kSynchronized
- ✓ No persistent ERROR-level logs


TC-FR1-002 — gPTP Master Disconnection State Degradation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Log Verification
:Priority: High

**Preconditions**

1. FR-1-001 completed, device is in kSynchronized state
2. Device logs accessible
3. Network connectivity of gPTP Master can be controlled (unplug cable or shut down Master service)

**Test Steps**

1. Confirm AccuracyQualifier == kSynchronized in log, record timestamp
2. Disconnect gPTP Master network
3. Wait 60s (exceeds syncLossTimeout configuration)
4. Check latest AccuracyQualifier status in device log
5. Reconnect Master network
6. Wait 60s
7. Check AccuracyQualifier status in log again

**Expected Result**

**Disconnection Phase**

- ✓ Log shows AccuracyQualifier degraded from kSynchronized to kUnstable or kNoTime
- ✓ Sync loss warning/error present in log


**Recovery Phase**

- ✓ Log shows kSynchronized state restored
- ✓ No service crash during entire recovery process


TC-FR1-003 — Startup Behavior Without gPTP Master
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Log Verification
:Priority: High

**Preconditions**

1. No gPTP Time Master in network (or Master is shut down)
2. Device logs accessible

**Test Steps**

1. Confirm no gPTP Master in network
2. Restart score::time service on target device
3. Wait 60s
4. Check AccuracyQualifier status in log
5. Verify service process is still running

**Expected Result**

**Log Verification**

- ✓ AccuracyQualifier remains kNoTime
- ✓ Log contains clear sync failure or waiting-for-Master records


**Service Stability**

- ✓ Service process not crashed, running normally
- ✓ No segfault or core dump


TC-FR1-004 — Device Does Not Send gPTP Sync Packets (Slave Role)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Packet Capture
:Priority: Medium

**Preconditions**

1. gPTP Time Master available in network
2. Wireshark deployed

**Test Steps**

1. Start Wireshark, filter: eth.type == 0x88F7
2. Start score::time service, wait 120s
3. Filter: source MAC is this device, gPTP messageType == Sync (0x0)
4. Count number of such packets

**Expected Result**

**Packet Capture**

- ✓ 0 Sync packets actively sent by this device
- ✓ Pdelay_Req packets sent by this device visible (messageType == 0x2)
- ✓ Sync and Follow_Up packets received by this device visible

FR-2 Vehicle Time Sync Precision
--------------------------------


TC-FR2-001 — AVB Node Synchronization Accuracy Verification (≤1 μs)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: Performance
:Method: Clock Comparison
:Priority: Medium

**Preconditions**

1. Device is AVB node, gPTP sync completed
2. Reference clock source (GPS or high-precision Master) available
3. Accuracy measurement tool ready

**Test Steps**

1. Wait for AccuracyQualifier == kSynchronized then wait 5 more minutes to stabilize
2. Continuously sample for 10 minutes, record each offset
3. Calculate: max offset, average offset, standard deviation

**Expected Result**

- ✓ All sample offsets within 10 minutes have absolute value ≤ 1 μs
- ✓ No abnormal jumps (single offset change ≤ 500 ns)
- ✗ Fail if any sample offset > 1 μs


TC-FR2-002 — Non-AVB Node Synchronization Accuracy Verification (≤500 μs)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: Performance
:Method: Clock Comparison
:Priority: Medium

**Preconditions**

1. Device is non-AVB node, gPTP sync completed
2. Reference clock available

**Test Steps**

1. Wait for sync to stabilize (AccuracyQualifier == kSynchronized, then wait 2 more minutes)
2. Continuously sample for 5 minutes, record each offset
3. Calculate max offset

**Expected Result**

- ✓ All sample offsets have absolute value ≤ 500 μs
- ✓ Most samples ≤ 200 μs (target range 200–500 μs)
- ✗ Fail if offset > 500 μs

FR-3 Vehicle Time Base API
--------------------------


TC-FR3-001 — Vehicle Time Now() Can Be Read Normally
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Test Tool
:Priority: High

**Preconditions**

1. gPTP sync completed (AccuracyQualifier == kSynchronized)
2. Test client tool deployed

**Test Steps**

1. Call IVehicleTime::Now(), record return value T1
2. Wait 1s
3. Call Now() again, record T2
4. Calculate T2 - T1
5. Execute 10 consecutive times

**Expected Result**

- ✓ Each call succeeds without error code
- ✓ T2 > T1 (monotonically increasing)
- ✓ T2 - T1 in range [950ms, 1050ms] (error ≤5%)
- ✓ No crash or exception in all 10 calls


TC-FR3-002 — Vehicle Time Has Nanosecond Resolution
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Test Tool
:Priority: High

**Preconditions**

1. gPTP sync completed
2. Test client tool available

**Test Steps**

1. Rapidly call Now() 10 consecutive times
2. Record complete time value returned each time (including nanosecond portion)
3. Check difference between adjacent return values

**Expected Result**

- ✓ Return value contains non-zero nanosecond field
- ✓ Difference between adjacent calls > 0 (time is advancing)
- ✓ Difference value consistent with actual call interval, precision at microsecond level


TC-FR3-003 — Vehicle Time Aligned with gPTP Master Clock
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Test Tool + Clock Comparison
:Priority: High

**Preconditions**

1. gPTP sync completed
2. Current gPTP Master time is known
3. Test tool available

**Test Steps**

1. Read current time T_master from gPTP Master device
2. Immediately read IVehicleTime::Now() from local device to get T_local
3. Calculate ``abs(T_local - T_master)``
4. Repeat 5 times

**Expected Result**

- ✓ ``abs(T_local - T_master)`` is within acceptable range (accounting for read operation latency)
- ✓ 5 results are stable, no random jumps

FR-4 Vehicle Time Base Accuracy Qualifier
-----------------------------------------


TC-FR4-001 — AccuracyQualifier State: kNoTime
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Test Tool + Log Verification
:Priority: High

**Preconditions**

1. No gPTP Master in network
2. score::time service just started (first run, never synced)
3. Test tool available

**Test Steps**

1. Start score::time service in environment without Master
2. Wait 10s
3. Query GetAccuracyQualifier() return value
4. Simultaneously check state record in log

**Expected Result**

- ✓ GetAccuracyQualifier() returns kNoTime
- ✓ Corresponding kNoTime state record present in log
- ✓ Now() can still be called in this state (no crash)


TC-FR4-002 — AccuracyQualifier State: kSynchronized
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Test Tool + Log Verification
:Priority: High

**Preconditions**

1. Stable gPTP Master available in network
2. Test tool available

**Test Steps**

1. Start score::time service, connect to gPTP Master
2. After 60s query GetAccuracyQualifier()
3. View log to confirm state transition (kNoTime → kUnstable → kSynchronized)

**Expected Result**

- ✓ GetAccuracyQualifier() returns kSynchronized after 60s
- ✓ State transition process visible in log
- ✓ State transition order is reasonable


TC-FR4-003 — AccuracyQualifier State: kTimeJumpDetected
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Test Tool + Log Verification
:Priority: Medium

**Preconditions**

1. Device is in kSynchronized state
2. Large clock jump can be injected (> threshold)

**Test Steps**

1. Confirm current state is kSynchronized
2. Inject large time jump (local clock offset from Master > jump detection threshold, e.g., > 1ms)
3. Immediately query GetAccuracyQualifier()
4. Query again after 30s

**Expected Result**

- ✓ GetAccuracyQualifier() returns kTimeJumpDetected after jump injection
- ✓ Clock jump detection record present in log
- ✓ System recovers from kTimeJumpDetected after 30s


TC-FR4-004 — AccuracyQualifier State: kUnstable
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Test Tool + Log Verification
:Priority: Medium

**Preconditions**

1. gPTP Master signal quality can be degraded (e.g., via network emulator to introduce jitter)
2. Test tool available

**Test Steps**

1. Device is in kSynchronized state
2. Introduce persistent high jitter or intermittent packet loss to gPTP network
3. Wait 20s, query GetAccuracyQualifier()
4. Restore network quality, wait 60s, query again

**Expected Result**

- ✓ GetAccuracyQualifier() returns kUnstable during degradation period
- ✓ Log contains accuracy instability records
- ✓ State returns to kSynchronized after network recovery

FR-5 Vehicle Time Base Time Point Qualifier
-------------------------------------------


TC-FR5-001 — TimePointQualifier ASIL Safety Level Query
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Test Tool
:Priority: Medium

**Preconditions**

1. score::time service running
2. ASIL-B safety clock source configured
3. Test tool available

**Test Steps**

1. Query GetTimePointQualifier() when safety clock source is normal
2. Simulate safety clock source failure (or degrade to QM source)
3. Query GetTimePointQualifier() again

**Expected Result**

- ✓ Returns kASIL_B when safety clock source is normal
- ✓ Degrades to kQM after safety clock source failure
- ✓ State changes are logged

FR-6 Vehicle Time Control Flow
------------------------------


TC-FR6-001 — Now() Call Performance (Low Latency)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: Performance
:Method: Test Tool
:Priority: Medium

**Preconditions**

1. score::time service running
2. Test tool supports high-frequency calls and timing statistics

**Test Steps**

1. Continuously call IVehicleTime::Now() 1000 times
2. Record each call latency
3. Calculate: average latency, P99 latency, max latency

**Expected Result**

- ✓ Average call latency < 1 μs
- ✓ P99 latency < 5 μs
- ✓ No single call exceeds 100 μs (no kernel syscall blocking)


Note: If latency is significantly high (>10 μs average), shared memory mechanism may not be working

FR-7 Vehicle Time Synchronization Logging
-----------------------------------------


TC-FR7-001 — Sync State Change Log Recording
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: Manual Acceptance
:Method: Log Verification
:Priority: High

**Preconditions**

1. Device logs accessible
2. Network environment controllable (can disconnect/restore gPTP Master)

**Test Steps**

1. Clear or mark current log position
2. Disconnect gPTP Master network, wait 30s
3. Restore Master network, wait 60s
4. View log, search keywords: AccuracyQualifier, sync, kSynchronized, kNoTime, kUnstable

**Expected Result**

- ✓ Log contains AccuracyQualifier state change records (with timestamp)
- ✓ Degradation log present when Master disconnects
- ✓ Re-sync log present when Master restores
- ✓ Log timestamp format is correct


TC-FR7-002 — Sync Statistics Log (Offset/Frequency)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: Manual Acceptance
:Method: Log Verification
:Priority: Medium

**Preconditions**

1. Device in sync state, logs accessible

**Test Steps**

1. Run for 10 minutes, view periodic sync statistics in log
2. Search keywords: offset, rate, correction, pDelay

**Expected Result**

- ✓ Log contains periodically recorded clock offset values
- ✓ Rate correction (rateCorrection) or path delay (pDelay) logs present
- ✓ Values within reasonable range, no abnormal values


TC-FR7-003 — Logging Does Not Block Sync Main Process
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: Manual Acceptance
:Method: Log Verification + Clock Comparison
:Priority: Low

**Preconditions**

1. Device in sync state

**Test Steps**

1. Monitor both log output and sync accuracy (offset) simultaneously
2. Measure sync accuracy during high-frequency log output (debug level)
3. Disable verbose logging, measure accuracy again
4. Compare accuracy difference between the two modes

**Expected Result**

- ✓ Sync accuracy not significantly degraded with verbose logging enabled (offset increase < 50 μs)
- ✓ Log thread does not affect real-time performance of sync main thread

FR-8 score::time External Synchronization
-----------------------------------------


TC-FR8-001 — NTP Sync Source Configuration and Connection
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Log Verification
:Priority: Medium

**Preconditions**

1. NTP Server accessible in network
2. score::time NTP function configuration enabled
3. Logs accessible

**Test Steps**

1. Configure NTP Server address
2. Restart score::time service
3. Wait 120s
4. Check NTP sync status in log (search: NTP, AbsoluteTime, synchronized)

**Expected Result**

- ✓ Log shows NTP sync success record
- ✓ No persistent NTP connection failure errors in log
- ✓ AbsoluteTime-related log has state update records


TC-FR8-002 — Fallback Behavior When NTP Server Unreachable
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Log Verification
:Priority: Medium

**Preconditions**

1. NTP function enabled
2. NTP Server address configured as unreachable

**Test Steps**

1. Configure NTP Server to unreachable address (e.g., 192.0.2.1)
2. Restart service, wait 120s
3. Check AbsoluteTime AccuracyQualifier status in log

**Expected Result**

- ✓ Log contains NTP connection failure record
- ✓ IAbsoluteTime AccuracyQualifier does not enter synchronized state
- ✓ Service does not crash, continues providing other functions

FR-9 Absolute Time Base API
---------------------------


TC-FR9-001 — IAbsoluteTime::Now() Returns UTC Time
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Test Tool
:Priority: Medium

**Preconditions**

1. NTP sync completed
2. Test tool available
3. Accurate UTC time known (confirmed via phone or other device)

**Test Steps**

1. Call IAbsoluteTime::Now()
2. Record returned UTC time
3. Compare with UTC time displayed on reference device

**Expected Result**

- ✓ Return value is valid UTC timestamp (UNIX Epoch format, not 0)
- ✓ Offset from reference UTC time < 1s (NTP accuracy range)
- ✓ Time value monotonically increasing across multiple calls

FR-10 Absolute Time Base Accuracy Qualifier
-------------------------------------------


TC-FR10-001 — Absolute Time AccuracyQualifier State Verification
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Test Tool + Log Verification
:Priority: Medium

**Preconditions**

1. Test tool available
2. NTP sync state controllable

**Test Steps**

1. Query IAbsoluteTime GetAccuracyQualifier() when NTP not synced, record state
2. Query again after NTP sync completed
3. View log records of absolute time accuracy

**Expected Result**

- ✓ Returns appropriate low accuracy level when NTP not synced
- ✓ Accuracy level improves after NTP sync
- ✓ Accuracy level changes are logged

FR-11 Absolute Time Base Security Qualifier
-------------------------------------------


TC-FR11-001 — Absolute Time SecurityQualifier State Verification
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Test Tool + Log Verification
:Priority: Medium

**Preconditions**

1. Test tool available
2. Different security level clock sources can be simulated

**Test Steps**

1. Query IAbsoluteTime GetSecurityQualifier() when using unauthenticated NTP source
2. Query again when using authenticated/secure NTP source
3. View security level records in log

**Expected Result**

Security levels (low to high): ``NoTimeAvailable``, ``NotTrustworthy``, ``Trustworthy``, ``VeryTrustworthy``

- ✓ Returns corresponding security level for different security sources
- ✓ State changes are logged

FR-12 Absolute Time Synchronization Status Log
----------------------------------------------


TC-FR12-001 — Absolute Time Sync Log Recording
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: Manual Acceptance
:Method: Log Verification
:Priority: Low

**Preconditions**

1. NTP sync function enabled
2. Logs accessible

**Test Steps**

1. Perform NTP sync operation (restart service to re-sync)
2. View log, search: AbsoluteTime, NTP, UTC
3. Check log content completeness

**Expected Result**

- ✓ Log contains NTP sync start/completion records
- ✓ Absolute time accuracy level change records present
- ✓ Security level status records present
- ✓ Log format includes timestamp

FR-13 High Precision Clock API
------------------------------


TC-FR13-001 — HighPrecisionClock::Now() Returns High Precision Time
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Test Tool
:Priority: Medium

**Preconditions**

1. Test tool available
2. System clock (CLOCK_REALTIME) synchronized

**Test Steps**

1. Call HighPrecisionClockImpl::Now()
2. Record returned time value (precise to nanoseconds)
3. Compare with system clock_gettime(CLOCK_REALTIME) return value
4. Call 10 consecutive times, confirm monotonicity

**Expected Result**

- ✓ Return value differs from system CLOCK_REALTIME time by < 1ms
- ✓ Return value has nanosecond precision
- ✓ 10 consecutive calls are monotonically increasing
- ✓ No call failures


TC-FR13-002 — Time Relationship Between HighPrecisionClock and VehicleTime
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Test Tool
:Priority: Low

**Preconditions**

1. gPTP sync completed
2. Test tool can read both interfaces simultaneously

**Test Steps**

1. Simultaneously call HighPrecisionClockImpl::Now() and IVehicleTime::Now()
2. Calculate difference between them
3. Repeat 5 times

**Expected Result**

- ✓ Difference is stable (fixed offset, not randomly varying)
- ✓ Variance of difference < 100 μs (indicating both synced to same clock domain)

FR-14 Monotonic Clock API
-------------------------


TC-FR14-001 — MonotonicClock::Now() Monotonicity Verification
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Test Tool
:Priority: High

**Preconditions**

1. Test tool available

**Test Steps**

1. Call MonotonicClockImpl::Now() 20 times at 100ms intervals
2. Record each return value
3. Check if sequence is strictly increasing

**Expected Result**

- ✓ All 20 return values strictly increasing (T[i+1] > T[i])
- ✓ Adjacent differences approximately 100ms (error < 5%)
- ✓ No wraparound or jumps


TC-FR14-002 — MonotonicClock Maintains Monotonicity During System Clock Adjustment
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Test Tool
:Priority: High

**Preconditions**

1. Test tool available
2. Permission to adjust system clock (date -s or chronyc makestep)

**Test Steps**

1. Start continuous sampling of MonotonicClock::Now() at 50ms intervals
2. Execute system time jump during sampling (e.g., advance clock by 5s)
3. Continue sampling for 30s
4. Check monotonicity of complete sample sequence

**Expected Result**

- ✓ Entire sample sequence (including during clock jump) is strictly monotonically increasing
- ✓ MonotonicClock is not affected by system clock adjustment


TC-FR14-003 — MonotonicClock Corresponds to CLOCK_MONOTONIC
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Test Tool
:Priority: Medium

**Preconditions**

1. Test tool available
2. Can read system CLOCK_MONOTONIC at the same time

**Test Steps**

1. Simultaneously call MonotonicClockImpl::Now() and system clock_gettime(CLOCK_MONOTONIC)
2. Compare two return values
3. Repeat 5 times

**Expected Result**

- ✓ Two values approximately equal (difference < 1ms)
- ✓ Both increase at the same rate

FR-15 score::time Mocking APIs
------------------------------


TC-FR15-001 — ManualVehicleTime Can Set Time Manually
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Test Tool
:Priority: Medium

**Preconditions**

1. score::time service started in Mock mode (ManualVehicleTime implementation)
2. Test tool available

**Test Steps**

1. Set ManualVehicleTime to specified time T_set (e.g., 2024-01-01 12:00:00)
2. Call IVehicleTime::Now(), record T_read
3. Wait 1s, call Now() again, record T_read2

**Expected Result**

- ✓ T_read == T_set (allow < 1ms error)
- ✓ T_read2 == T_set (time does not advance automatically unless manually advanced)
- ✓ Does not depend on real gPTP network


TC-FR15-002 — ManualVehicleTime Time Can Be Manually Advanced
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Test Tool
:Priority: Medium

**Preconditions**

1. Mock mode started, ManualVehicleTime initial time set
2. Test tool available

**Test Steps**

1. Record current ManualVehicleTime value T0
2. Advance time by +500ms via test tool
3. Call Now(), record T1
4. Advance by +1s more
5. Call Now(), record T2

**Expected Result**

- ✓ T1 - T0 == 500ms (error < 1ms)
- ✓ T2 - T1 == 1s (error < 1ms)
- ✓ Time advancement is controllable, not affected by system clock


TC-FR15-003 — ManualVehicleTime AccuracyQualifier Can Be Set Manually
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Test Tool
:Priority: Medium

**Preconditions**

1. Mock mode started

**Test Steps**

1. Set AccuracyQualifier to kNoTime, confirm with GetAccuracyQualifier()
2. Set to kSynchronized, confirm again
3. Set to kUnstable, confirm again

**Expected Result**

- ✓ All 3 settings take effect, GetAccuracyQualifier() return value matches set value
- ✓ State switching causes no crash


TC-FR15-004 — ManualAbsoluteTime Can Set UTC Time
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: System Function
:Method: Test Tool
:Priority: Low

**Preconditions**

1. Mock mode started (ManualAbsoluteTime implementation)
2. Test tool available

**Test Steps**

1. Set ManualAbsoluteTime to specified UTC time
2. Call IAbsoluteTime::Now(), verify return value
3. Set SecurityQualifier to Trustworthy, verify GetSecurityQualifier() return value

**Expected Result**

- ✓ IAbsoluteTime::Now() returns the set UTC time
- ✓ GetSecurityQualifier() returns Trustworthy
- ✓ Function decoupled from real NTP sync behavior


TC-FR15-005 — ManualMonotonicClock Isolation in Unit Tests
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:Type: Manual Acceptance
:Method: Test Tool
:Priority: Low

**Preconditions**

1. Unit test framework configured to use Mock implementation
2. No real hardware environment required

**Test Steps**

1. Run unit tests using ManualMonotonicClock
2. Manually set and advance time in tests, verify time-related logic
3. Confirm tests do not depend on system time

**Expected Result**

- ✓ Unit tests pass without gPTP/NTP network environment
- ✓ Test execution time not affected by system clock
- ✓ Mock object behavior consistent with interface specification
