Concept for TimeSlave
======================

.. contents:: Table of Contents
   :depth: 3
   :local:

TimeSlave concept
------------------

Use Cases
~~~~~~~~~

TimeSlave is a standalone gPTP (IEEE 802.1AS) slave endpoint process that implements the low-level time synchronization protocol for the Eclipse SCORE time system. It is deployed as a separate process from the TimeDaemon to isolate real-time network I/O from the higher-level time validation and distribution logic.

More precisely we can specify the following use cases for the TimeSlave:

1. Receiving gPTP Sync/FollowUp messages from a Time Master on the Ethernet network
2. Measuring peer delay via the IEEE 802.1AS PDelayReq/PDelayResp exchange
3. Optionally adjusting the PTP Hardware Clock (PHC) on the NIC
4. Publishing the resulting ``GptpIpcData`` to shared memory for consumption by the TimeDaemon

The raw architectural diagram is represented below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/timeslave_deployment.puml
   :alt: Raw architectural diagram

.. raw:: html

   </div>

Components decomposition
~~~~~~~~~~~~~~~~~~~~~~~~~

The design consists of several sw components:

1. `TimeSlave Application <#timeslave-application-sw-component>`_
2. `GptpEngine <#gptpengine-sw-component>`_
3. `FrameCodec <#framecodec-sw-component>`_
4. `MessageParser <#messageparser-sw-component>`_
5. `SyncStateMachine <#syncstatemachine-sw-component>`_
6. `PeerDelayMeasurer <#peerdelaymeasurer-sw-component>`_
7. `PhcAdjuster <#phcadjuster-sw-component>`_
8. `libTSClient <#libtsclient-sw-component>`_
9. `ShmPTPEngine <#shmptpengine-sw-component>`_

Class view
~~~~~~~~~~

Main classes and components are presented on this diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/timeslave_class.puml
   :alt: Class View
   :width: 100%
   :align: center

.. raw:: html

   </div>

Data and control flow
~~~~~~~~~~~~~~~~~~~~~

The Data and Control flow are presented in the following diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/timeslave_data_flow.puml
   :alt: Data and Control flow View

.. raw:: html

   </div>

On this view you could see several "workers" scopes:

1. RxThread scope
2. PdelayThread scope
3. Main thread (periodic publish) scope

Each control flow is implemented with the dedicated thread and is independent from another ones.

Control flows
^^^^^^^^^^^^^

RxThread scope
''''''''''''''

This control flow is responsible for the:

1. receive raw gPTP Ethernet frames with hardware timestamps from the NIC via raw sockets
2. decode and parse the PTP messages (Sync, FollowUp, PdelayResp, PdelayRespFollowUp)
3. correlate Sync/FollowUp pairs and compute clock offset and neighborRateRatio
4. update the shared ``PtpTimeInfo`` snapshot under mutex protection

PdelayThread scope
'''''''''''''''''''

This control flow is responsible for the:

1. periodically transmit PDelayReq frames and capture hardware transmit timestamps
2. coordinate with the RxThread to receive PDelayResp and PDelayRespFollowUp messages
3. compute the peer delay using the IEEE 802.1AS formula: ``path_delay = ((t2 - t1) + (t4 - t3c)) / 2``

Main thread (periodic publish) scope
''''''''''''''''''''''''''''''''''''''

This control flow is responsible for the:

1. periodically call ``GptpEngine::FinalizeSnapshot()`` to check timeout and commit the pending snapshot
2. call ``GptpEngine::ReadPTPSnapshot(data)`` to copy the latest ``GptpIpcData`` into a local variable
3. publish to shared memory via ``GptpIpcPublisher::Publish(data)``

Data types or events
^^^^^^^^^^^^^^^^^^^^

There are several data types, which components are communicating to each other:

PTPMessage
''''''''''

``PTPMessage`` is a union-based container for decoded gPTP messages including the hardware receive timestamp. It is produced by ``MessageParser`` and consumed by ``SyncStateMachine`` and ``PeerDelayMeasurer``.

SyncResult
''''''''''

``SyncResult`` is produced by ``SyncStateMachine::OnFollowUp()`` and contains the computed master timestamp, clock offset, Sync/FollowUp data, and time jump flags (forward/backward).

PDelayResult
''''''''''''

``PDelayResult`` is produced by ``PeerDelayMeasurer`` and contains the computed path delay in nanoseconds and a validity flag.

PtpTimeInfo
''''''''''''

``PtpTimeInfo`` is the TimeDaemon-internal aggregated snapshot. It is **not** the shared memory type; it is produced by ``ShmPTPEngine::ReadPTPSnapshot()`` by field-mapping from ``GptpIpcData`` into the format expected by the TimeDaemon pipeline.

SW Components decomposition
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TimeSlave Application SW component
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``TimeSlave Application`` component is the main entry point for the TimeSlave process. It extends ``score::mw::lifecycle::Application`` and is responsible for orchestrating the overall lifecycle of the GptpEngine and the IPC publisher.

Component requirements
''''''''''''''''''''''

The ``TimeSlave Application`` has the following requirements:

- The ``TimeSlave Application`` shall implement the ``Initialize()`` method to create the ``GptpEngine`` with configured options, initialize the ``GptpIpcPublisher`` (creates the shared memory segment), and create the ``HighPrecisionLocalSteadyClock`` for the engine
- The ``TimeSlave Application`` shall implement the ``Run()`` method to enter a periodic publish loop (50 ms interval) and monitor the ``stop_token`` for graceful shutdown
- On each loop iteration, ``TimeSlave Application`` shall call ``GptpEngine::FinalizeSnapshot()``, then ``GptpEngine::ReadPTPSnapshot(data)``, and publish the resulting ``GptpIpcData`` via ``GptpIpcPublisher::Publish(data)``
- The ``TimeSlave Application`` shall call ``GptpEngine::Deinitialize()`` and ``GptpIpcPublisher::Destroy()`` after the ``stop_token`` is set

GptpEngine SW component
^^^^^^^^^^^^^^^^^^^^^^^^

The ``GptpEngine`` component is the core gPTP protocol engine. It manages two background threads (RxThread and PdelayThread) for network I/O and peer delay measurement, and exposes a thread-safe ``ReadPTPSnapshot()`` method for the main thread to read the latest time measurement.

Component requirements
''''''''''''''''''''''

The ``GptpEngine`` has the following requirements:

- The ``GptpEngine`` shall manage an RxThread for receiving and parsing gPTP frames from raw Ethernet sockets
- The ``GptpEngine`` shall manage a PdelayThread for periodic peer delay measurement
- The ``GptpEngine`` shall provide a ``FinalizeSnapshot()`` method that checks for sync timeout, applies status flags, and commits the pending snapshot to the current snapshot; this must be called before ``ReadPTPSnapshot()``
- The ``GptpEngine`` shall provide a ``ReadPTPSnapshot(GptpIpcData&)`` method that copies the latest committed snapshot into the caller's buffer and returns false only if the engine is not initialized
- The ``GptpEngine`` shall support configurable parameters via ``GptpEngineOptions`` (interface name, PDelay interval, PDelay warmup, sync timeout, time-jump threshold, PHC configuration)
- The ``GptpEngine`` shall support exchangeability of the raw socket implementation for different platforms (Linux, QNX)

Class view
''''''''''

The Class Diagram is presented below:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/gptp_engine/gptp_engine_class.puml
   :alt: Class Diagram

.. raw:: html

   </div>

Threading model
'''''''''''''''

The GptpEngine operates with two background threads. The threading model is represented below:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/gptp_engine/gptp_threading.puml
   :alt: Threading Model

.. raw:: html

   </div>

Concurrency aspects
'''''''''''''''''''

The ``GptpEngine`` uses the following synchronization mechanisms:

- A ``std::mutex`` protects the ``pending_snapshot_`` and ``current_snapshot_`` fields (both ``GptpIpcData``): the RxThread writes ``pending_snapshot_``; the main thread calls ``FinalizeSnapshot()`` (commits pending to current) and ``ReadPTPSnapshot()`` (reads current)
- The ``PeerDelayMeasurer`` uses its own ``std::mutex`` to synchronize between the PdelayThread (``SendRequest()``) and the RxThread (``OnResponse()``, ``OnResponseFollowUp()``)
- The ``SyncStateMachine`` uses ``std::atomic<bool>`` for the timeout flag, which is read from the main thread and written from the RxThread

Hardware timestamping fallback
'''''''''''''''''''''''''''''''

During ``Initialize()``, ``GptpEngine`` calls ``IRawSocket::EnableHwTimestamping()`` to request NIC-level receive timestamps (``SO_TIMESTAMPING`` on Linux). If the NIC does not support hardware timestamping, the call returns ``false`` and a warning is logged:

.. code-block:: none

   GptpEngine: HW timestamping not available on <iface>, falling back to SW timestamps

The engine continues to run normally. The difference between the two modes:

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Field
     - HW timestamping available
     - SW timestamping fallback
   * - ``recvHardwareTS`` (Sync receive time)
     - NIC hardware timestamp (nanosecond precision, captured at wire level)
     - Software timestamp (captured at socket receive, higher jitter)
   * - ``sync_fup_data.reference_local_timestamp``
     - Derived from NIC hardware timestamp
     - Derived from software timestamp
   * - ``GptpIpcData.local_time``
     - Always ``CLOCK_MONOTONIC`` (unaffected)
     - Always ``CLOCK_MONOTONIC`` (unaffected)
   * - Clock offset accuracy
     - High (sub-microsecond typical)
     - Reduced (jitter depends on OS scheduling latency)

The fallback does not affect protocol correctness — Sync/FollowUp correlation and peer delay measurement continue to work — but the computed clock offset will be less accurate due to higher receive timestamp jitter.

FrameCodec SW component
^^^^^^^^^^^^^^^^^^^^^^^^^

The ``FrameCodec`` component handles raw Ethernet frame encoding and decoding for gPTP communication.

Component requirements
''''''''''''''''''''''

The ``FrameCodec`` has the following requirements:

- The ``FrameCodec`` shall parse incoming Ethernet frames, extracting source/destination MAC addresses, handling 802.1Q VLAN tags, and validating the EtherType (``0x88F7``)
- The ``FrameCodec`` shall construct outgoing Ethernet headers for PDelayReq frames using the standard PTP multicast destination MAC (``01:80:C2:00:00:0E``)

MessageParser SW component
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``MessageParser`` component parses the PTP wire format (IEEE 1588-v2) from raw payload bytes.

Component requirements
''''''''''''''''''''''

The ``MessageParser`` has the following requirements:

- The ``MessageParser`` shall validate the PTP header (version, domain, message length)
- The ``MessageParser`` shall decode all relevant message types: Sync, FollowUp, PdelayReq, PdelayResp, PdelayRespFollowUp
- The ``MessageParser`` shall use packed wire structures (``__attribute__((packed))``) for direct memory mapping of PTP messages

SyncStateMachine SW component
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``SyncStateMachine`` component implements the two-step Sync/FollowUp correlation logic. It correlates incoming Sync and FollowUp messages by sequence ID, computes the clock offset and neighbor rate ratio, and detects time jumps.

Component requirements
''''''''''''''''''''''

The ``SyncStateMachine`` has the following requirements:

- The ``SyncStateMachine`` shall store Sync messages and correlate them with subsequent FollowUp messages by sequence ID
- The ``SyncStateMachine`` shall compute the clock offset: ``offset_ns = master_time - slave_receive_time - path_delay``
- The ``SyncStateMachine`` shall compute the ``neighborRateRatio`` from successive Sync intervals (master vs. slave clock progression)
- The ``SyncStateMachine`` shall detect forward and backward time jumps against configurable thresholds
- The ``SyncStateMachine`` shall provide thread-safe timeout detection via ``std::atomic<bool>``, set when no Sync is received within the configured timeout

PeerDelayMeasurer SW component
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``PeerDelayMeasurer`` component implements the IEEE 802.1AS two-step peer delay measurement protocol. It manages the four timestamps (``t1``, ``t2``, ``t3c``, ``t4``) across two threads.

Timestamp definitions
'''''''''''''''''''''

.. list-table:: Peer Delay Timestamps (IEEE 802.1AS)
   :header-rows: 1
   :widths: 10 20 30 40

   * - Symbol
     - Message
     - Captured by
     - Meaning
   * - ``t1``
     - PDelayReq (TX)
     - Slave (PdelayThread)
     - HW transmit timestamp of the PDelayReq frame leaving the slave NIC
   * - ``t2``
     - PDelayResp (RX)
     - Master → carried in PDelayResp body
     - HW receive timestamp of the PDelayReq frame arriving at the master NIC
   * - ``t3c``
     - PDelayRespFollowUp
     - Master → carried in PDelayRespFollowUp body
     - HW transmit timestamp of the PDelayResp frame leaving the master NIC ("corrected" because it includes the master's turnaround correction)
   * - ``t4``
     - PDelayResp (RX)
     - Slave (RxThread)
     - HW receive timestamp of the PDelayResp frame arriving at the slave NIC

The peer delay formula is: ``path_delay = ((t2 - t1) + (t4 - t3c)) / 2``

- ``(t2 - t1)`` = propagation time from slave → master
- ``(t4 - t3c)`` = propagation time from master → slave
- The average of the two gives the one-way link delay

Component requirements
''''''''''''''''''''''

The ``PeerDelayMeasurer`` has the following requirements:

- The ``PeerDelayMeasurer`` shall transmit PDelayReq frames and capture the hardware transmit timestamp (``t1``)
- The ``PeerDelayMeasurer`` shall receive PDelayResp (providing ``t2``, ``t4``) and PDelayRespFollowUp (providing ``t3c``) messages
- The ``PeerDelayMeasurer`` shall compute the peer delay using the IEEE 802.1AS formula: ``path_delay = ((t2 - t1) + (t4 - t3c)) / 2``
- The ``PeerDelayMeasurer`` shall discard PDelayResp and PDelayRespFollowUp messages whose sequence ID does not match the most recently transmitted PDelayReq
- The ``PeerDelayMeasurer`` shall suppress the path-delay result when more than one PDelayResp is received for a single PDelayReq (detection of non-time-aware bridges per IEEE 802.1AS)
- The ``PeerDelayMeasurer`` shall provide thread-safe access to the ``PDelayResult`` via a mutex, as ``SendRequest()`` runs on the PdelayThread while response handlers are called from the RxThread

PhcAdjuster SW component
^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``PhcAdjuster`` component synchronizes the PTP Hardware Clock (PHC) on the NIC. It applies step corrections for large offsets and frequency slew for smooth convergence of small offsets.

Component requirements
''''''''''''''''''''''

The ``PhcAdjuster`` has the following requirements:

- The ``PhcAdjuster`` shall apply an immediate time step correction for offsets exceeding ``step_threshold_ns``
- The ``PhcAdjuster`` shall apply frequency slew (in ppb) for offsets below the step threshold
- The ``PhcAdjuster`` shall support platform-specific implementations: ``clock_adjtime()`` on Linux, EMAC PTP ioctls on QNX
- The ``PhcAdjuster`` shall be configurable via ``PhcConfig`` (device path, step threshold, enable/disable flag)

Fallback behavior when PHC is unavailable
''''''''''''''''''''''''''''''''''''''''''

The ``PhcAdjuster`` degrades gracefully in two scenarios:

1. **PHC disabled** (``PhcConfig.enabled = false``, the default): ``AdjustOffset()`` and ``AdjustFrequency()`` are no-ops. The gPTP protocol pipeline (Sync/FollowUp reception, peer-delay measurement, ``GptpIpcData`` publishing) is completely unaffected. The hardware clock is not touched.

2. **PHC enabled but device inaccessible** (e.g., ``/dev/ptp0`` does not exist on Linux, or the EMAC interface name is wrong on QNX):

   - **Linux**: the constructor calls ``open(device, O_RDWR)``; on failure ``phc_fd_`` stays at ``-1``. Both ``AdjustOffset()`` and ``AdjustFrequency()`` guard against ``phc_fd_ < 0`` and return immediately — a true silent skip with no system call.

   - **QNX**: ``qnx_phc_open()`` always returns ``0`` and never fails — it only stores the device name in a thread-local context. There is no ``phc_fd_ < 0`` guard. The adjustment methods always call ``qnx_phc_adjtime_step()`` / ``qnx_phc_adjfreq_ppb()``, which internally create a UDP socket and issue ``SIOCGDRVSPEC`` / ``SIOCSDRVSPEC`` ioctls. If the socket or ioctl fails (e.g., wrong interface name, unsupported hardware), the function returns ``-1``, but the caller discards it with a ``(void)`` cast. There is no explicit skip — the call is always attempted and errors are silently absorbed.

In both scenarios TimeSlave continues to track the master clock and publish accurate ``GptpIpcData`` snapshots (including offset and status flags) to shared memory. The downstream TimeDaemon and any applications consuming time are unaffected — only the NIC hardware clock itself will drift relative to PTP time.

libTSClient SW component
^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``libTSClient`` component is the shared memory IPC library that connects the TimeSlave process to the TimeDaemon process. It provides a lock-free, single-writer/multi-reader communication channel using a seqlock protocol over POSIX shared memory.

The component provides two sub components: publisher and receiver to be deployed on the TimeSlave and TimeDaemon sides accordingly.

Component requirements
''''''''''''''''''''''

The ``libTSClient`` has the following requirements:

- The ``libTSClient`` shall define a shared memory layout (``GptpIpcRegion``) with a magic number (``0x47505450`` = 'GPTP') for validation, an atomic seqlock counter (``seq``), a confirmation counter (``seq_confirm``), and a ``GptpIpcData`` data payload
- The ``libTSClient`` shall align the shared memory region to 64 bytes (cache line size) to prevent false sharing
- The ``libTSClient`` shall provide a ``GptpIpcPublisher`` component (in ``score::ts::details``) that creates and manages the POSIX shared memory segment and writes ``GptpIpcData`` using the seqlock protocol
- The ``libTSClient`` shall provide a ``GptpIpcReceiver`` component (in ``score::ts::details``) that opens the shared memory segment read-only and reads ``GptpIpcData`` with up to 20 seqlock retries
- The ``libTSClient`` shall use the POSIX shared memory name ``/gptp_ptp_info`` by default

Class view
''''''''''

The Class Diagram is presented below:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/libtsclient/ipc_channel.puml
   :alt: Class Diagram

.. raw:: html

   </div>

Publish new data
''''''''''''''''

When ``TimeSlave Application`` has a new ``GptpIpcData`` snapshot, it publishes to the shared memory via the seqlock protocol:

1. Increment ``seq`` (becomes odd — signals write in progress); a release fence is applied
2. ``memcpy`` the ``GptpIpcData``
3. Store ``seq_confirm = seq + 1`` and increment ``seq`` (both become even — signals write complete)

Receive data
''''''''''''

From TimeDaemon side, the receiver reads from the shared memory using the seqlock protocol with bounded retry:

1. Read ``seq1`` with acquire ordering (must be even, otherwise retry — write in progress)
2. ``memcpy`` the ``GptpIpcData``
3. Apply an acquire-release fence; read ``seq_confirm`` as ``seq2`` and re-read ``seq`` as ``seq3``
4. If ``seq1 == seq2 == seq3``, the read is consistent; otherwise retry — torn read detected
5. Return ``std::optional<GptpIpcData>`` (empty if all 20 retries exhausted)

The seqlock protocol workflow is presented in the following sequence diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/libtsclient/ipc_sequence.puml
   :alt: Seqlock Protocol

.. raw:: html

   </div>

Platform support
~~~~~~~~~~~~~~~~~

TimeSlave supports two target platforms with platform-specific implementations selected at compile time via Bazel ``select()``:

.. list-table:: Platform Implementations
   :header-rows: 1
   :widths: 25 37 38

   * - Component
     - Linux
     - QNX
   * - Raw Socket
     - ``AF_PACKET`` + ``SO_TIMESTAMPING``; HW RX timestamp via ``recvmsg`` ``SCM_TIMESTAMPING``
     - BPF (``/dev/bpf``); HW RX timestamp via ``bpf_xhdr.bh_tstamp`` (``BIOCSTSTAMP BPF_T_BINTIME|BPF_T_PTP``); TX PHC timestamp via dedicated TX loopback fd (``BIOCSSEESENT``), filtered to Pdelay_Req frames only (BPF message-type 0x02); single static context (not thread-local)
   * - Network Identity
     - ``ioctl(SIOCGIFHWADDR)`` → EUI-48 → EUI-64
     - ``getifaddrs()`` + ``AF_LINK`` / ``sockaddr_dl`` (``LLADDR``) → EUI-48/64
   * - PHC Adjuster
     - ``clock_adjtime()`` (``SYS_clock_adjtime`` syscall); step via ``ADJ_SETOFFSET|ADJ_NANO``; slew via ``ADJ_FREQUENCY`` (scaled-ppm)
     - ``SIOCGDRVSPEC`` / ``SIOCSDRVSPEC`` on UDP socket; step via ``PTP_GET_TIME`` (0x102) + ``PTP_SET_TIME`` (0x103); slew via ``EMAC_PTP_ADJ_FREQ_PPM`` (0x200) in ppm
   * - HighPrecisionLocalSteadyClock
     - ``CLOCK_MONOTONIC`` via ``clock_gettime()``
     - QNX ``ClockCycles()`` CPU instruction (reads hardware performance counter directly, equivalent to ``RDTSC`` on x86 / ``CNTVCT`` on ARM64), converted to nanoseconds via cycles-per-second calibration. Used instead of ``clock_gettime()`` because QNX ``CLOCK_MONOTONIC`` resolution is limited to microsecond level, whereas ``ClockCycles()`` provides nanosecond-level precision with no syscall overhead.

The ``IRawSocket`` and ``INetworkIdentity`` interfaces provide the abstraction boundary. Platform-specific source files are organized under ``score/TimeSlave/code/gptp/platform/linux/`` and ``score/TimeSlave/code/gptp/platform/qnx/``.

Instrumentation
~~~~~~~~~~~~~~~~

ProbeManager
^^^^^^^^^^^^

The ``ProbeManager`` is a singleton that traces probe events at key processing points in the gPTP engine. It emits a ``LogDebug`` entry on every ``Trace()`` call and forwards the event to a linked ``Recorder`` (if set and enabled). Probing is controlled at runtime via ``SetEnabled()``; the ``GPTP_PROBE()`` macro provides zero overhead when disabled.

Supported probe points (``ProbePoint`` enum):

.. list-table:: ProbePoint Events
   :header-rows: 1
   :widths: 10 30 60

   * - Value
     - Enumerator
     - Trigger
   * - 0
     - ``kRxPacketReceived``
     - Raw Ethernet frame received from socket (RxThread)
   * - 1
     - ``kSyncFrameParsed``
     - Sync message successfully decoded by ``GptpMessageParser``
   * - 2
     - ``kFollowUpProcessed``
     - FollowUp received; ``SyncStateMachine::OnFollowUp()`` returned a ``SyncResult``
   * - 3
     - ``kOffsetComputed``
     - Final clock offset value available after Sync/FollowUp correlation
   * - 4
     - ``kPdelayReqSent``
     - PDelayReq frame transmitted by ``PeerDelayMeasurer``
   * - 5
     - ``kPdelayCompleted``
     - Peer delay computation finished (all four timestamps collected)
   * - 6
     - ``kPhcAdjusted``
     - ``PhcAdjuster`` applied a step or frequency correction

When a probe event is forwarded to the ``Recorder``, it is written with ``RecordEvent::kProbe`` and the ``ProbePoint`` value stored in the ``status_flags`` field of the CSV row.

Recorder
^^^^^^^^^

Thread-safe CSV file writer. When enabled, appends one row per event to the configured file. The file is opened in append mode (``ios::app``); a CSV header is written only if the file is newly created (size == 0).

**Status model:** the ``Recorder`` starts in the state determined by ``Config.enabled``. If a write error occurs (``file_.good()`` fails after a flush), ``enabled_`` is atomically set to ``false`` and all subsequent ``Record()`` calls become no-ops. The file is never re-opened after an error.

Configuration (``Recorder::Config``):

.. list-table:: Recorder Configuration
   :header-rows: 1
   :widths: 30 15 55

   * - Parameter
     - Type
     - Description
   * - ``enabled``
     - bool
     - Enable or disable recording; default: ``false``
   * - ``file_path``
     - string
     - Output CSV file path; default: ``/var/log/gptp_record.csv``
   * - ``offset_threshold_ns``
     - int64_t
     - Reserved for ``kOffsetThreshold`` events (threshold above which offsets are logged); default: ``1 000 000`` (1 ms)
   * - ``flush_interval``
     - uint32_t
     - Number of rows between explicit ``file_.flush()`` calls; default: ``8``

CSV output format::

   mono_ns,event,offset_ns,pdelay_ns,seq_id,status_flags

Supported ``RecordEvent`` values written to the ``event`` column:

.. list-table:: RecordEvent Values
   :header-rows: 1
   :widths: 10 30 60

   * - Value
     - Enumerator
     - Description
   * - 0
     - ``kSyncReceived``
     - A Sync message was received and processed
   * - 1
     - ``kPdelayCompleted``
     - A full peer delay measurement cycle completed
   * - 2
     - ``kClockJump``
     - A forward or backward time jump was detected
   * - 3
     - ``kOffsetThreshold``
     - Clock offset exceeded ``offset_threshold_ns``
   * - 4
     - ``kProbe``
     - Forwarded from ``ProbeManager::Trace()``; ``status_flags`` column carries the ``ProbePoint`` value

Logging configuration
~~~~~~~~~~~~~~~~~~~~~

The TimeSlave and its TimeDaemon-side adapter use the following logging contexts:

.. list-table:: Logging Contexts
   :header-rows: 1
   :widths: 35 20 45

   * - Component
     - Context ID
     - Comments
   * - TimeSlave Application
     - TSAP
     - **T**\ ime\ **S**\ lave **App**\ lication lifecycle (Initialize / Run)
   * - gPTP Engine (RxThread / PdelayThread)
     - GTPS
     - **GPTP** **SLAVE** engine — low-level protocol processing
   * - ShmPTPEngine (TimeDaemon side)
     - GPTP
     - TimeDaemon **GPTP** machine adapter (Initialize / ReadPTPSnapshot)

Variability
~~~~~~~~~~~

Configuration
^^^^^^^^^^^^^

The ``GptpEngineOptions`` struct provides all configurable parameters for the gPTP engine:

.. list-table:: GptpEngine Configuration
   :header-rows: 1
   :widths: 30 15 55

   * - Parameter
     - Type
     - Description
   * - ``iface_name``
     - string
     - Network interface for gPTP frames (e.g., ``emac0``); default: ``"emac0"``
   * - ``pdelay_interval_ms``
     - int
     - Interval between PDelayReq transmissions (ms); default: ``1000``
   * - ``pdelay_warmup_ms``
     - int
     - Delay before the first PDelayReq is sent (ms); default: ``2000``
   * - ``sync_timeout_ms``
     - int
     - Timeout for Sync message reception before declaring timeout state (ms); default: ``3300``
   * - ``jump_future_threshold_ns``
     - int64_t
     - Threshold above which a positive clock offset is flagged as a forward time jump (ns); default: ``500 000 000``
   * - ``phc_config``
     - PhcConfig
     - PHC hardware clock adjustment settings (see ``PhcConfig`` table below); disabled by default

The ``PhcConfig`` struct (embedded in ``GptpEngineOptions``) contains:

.. list-table:: PhcAdjuster Configuration
   :header-rows: 1
   :widths: 30 15 55

   * - Parameter
     - Type
     - Description
   * - ``enabled``
     - bool
     - Enable or disable PHC adjustment; default: ``false``
   * - ``device``
     - string
     - PHC device identifier: ``/dev/ptp0`` on Linux, ``emac0`` on QNX
   * - ``step_threshold_ns``
     - int64_t
     - Offset threshold above which a step correction is applied instead of frequency slew (ns); default: ``100 000 000``

Scalability
^^^^^^^^^^^

The TimeSlave architecture supports the following extensibility points:

Platform extensibility
''''''''''''''''''''''

1. New target platforms can be supported by implementing the ``IRawSocket`` and ``INetworkIdentity`` interfaces under a new ``platform/<os>/`` directory and selecting the implementation via ``Bazel select()``
2. The ``PhcAdjuster`` platform implementations (``clock_adjtime`` on Linux, EMAC ioctls on QNX) can be extended for additional hardware without changing protocol logic

Protocol extensibility
''''''''''''''''''''''

1. The ``GptpEngine`` accepts injected ``IRawSocket`` and ``INetworkIdentity`` dependencies, making it straightforward to test or replace individual platform abstractions
2. The shared memory IPC channel name is configurable (``GptpIpcPublisher::Init(name)`` / ``GptpIpcReceiver::Init(name)``), allowing multiple gPTP instances per ECU if needed

TimeDaemon integration extensibility
''''''''''''''''''''''''''''''''''''''

1. The ``ShmPTPEngine`` implements the same ``PTPEngine`` concept as other ``PTPMachine`` backends, making it transparently exchangeable with any other engine implementation
2. Alternative IPC mechanisms (e.g., socket-based) can be introduced by implementing a new engine class without modifying the ``PTPMachine`` template or downstream components

ShmPTPEngine SW component
^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``ShmPTPEngine`` component (in ``score::td::details``) is the TimeDaemon-side adapter that reads ``GptpIpcData`` from the shared memory channel written by TimeSlave and converts it into the ``PtpTimeInfo`` structure expected by the TimeDaemon pipeline.

It is instantiated as ``GPTPShmMachine`` — a type alias for ``PTPMachine<details::ShmPTPEngine>`` — which connects ``ShmPTPEngine`` to the TimeDaemon's internal ``MessageBroker``.

Component requirements
''''''''''''''''''''''

The ``ShmPTPEngine`` has the following requirements:

- The ``ShmPTPEngine`` shall call ``GptpIpcReceiver::Init(ipc_name)`` during ``Initialize()`` to open the shared memory channel
- The ``ShmPTPEngine`` shall call ``GptpIpcReceiver::Receive()`` in ``ReadPTPSnapshot()`` to fetch the latest ``GptpIpcData``
- The ``ShmPTPEngine`` shall map all fields of ``GptpIpcData`` to the corresponding fields of ``PtpTimeInfo`` (status flags, Sync/FollowUp data, peer-delay data, time references)
- The ``ShmPTPEngine`` shall call ``GptpIpcReceiver::Close()`` during ``Deinitialize()``
- The ``ShmPTPEngine`` shall be instantiatable with a configurable IPC channel name (default: ``/gptp_ptp_info``)

Class view
''''''''''

The Class Diagram is presented below:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/shm_ptp_engine/shm_ptp_engine_class.puml
   :alt: Class Diagram

.. raw:: html

   </div>

Component initialization
''''''''''''''''''''''''

During initialization the ``ShmPTPEngine`` shall open the shared memory channel to be able to read from it.

The initialization workflow is represented in the following sequence diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/shm_ptp_engine/shm_ptp_engine_init_seq.puml
   :alt: Initialization workflow

.. raw:: html

   </div>

Read PTP snapshot
'''''''''''''''''

After ``ShmPTPEngine`` reads the latest ``GptpIpcData`` from shared memory, it maps it to ``PtpTimeInfo`` and publishes via the ``MessageBroker``.

The periodic read and publish workflow is described below:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/shm_ptp_engine/shm_ptp_engine_read_seq.puml
   :alt: Periodic read and publish workflow

.. raw:: html

   </div>

Data mapping
''''''''''''

``ShmPTPEngine::ReadPTPSnapshot()`` performs a field-by-field mapping from ``GptpIpcData`` to ``PtpTimeInfo``:

.. list-table:: GptpIpcData → PtpTimeInfo Mapping
   :header-rows: 1
   :widths: 50 50

   * - ``GptpIpcData`` field
     - ``PtpTimeInfo`` field
   * - ``ptp_assumed_time``
     - ``ptp_assumed_time``
   * - ``local_time``
     - ``local_time`` (wrapped in ``ReferenceClock::time_point``)
   * - ``rate_deviation``
     - ``rate_deviation``
   * - ``status.is_synchronized``
     - ``status.is_synchronized``
   * - ``status.is_timeout``
     - ``status.is_timeout``
   * - ``status.is_time_jump_future``
     - ``status.is_time_jump_future``
   * - ``status.is_time_jump_past``
     - ``status.is_time_jump_past``
   * - ``status.is_correct``
     - ``status.is_correct``
   * - ``sync_fup_data.*`` (9 fields)
     - ``sync_fup_data.*`` (direct copy)
   * - ``pdelay_data.*`` (12 fields)
     - ``pdelay_data.*`` (direct copy)

Factory
'''''''

``CreateGPTPShmMachine(name, ipc_name)`` is a convenience factory function in ``score::td`` that creates a configured ``GPTPShmMachine`` (``shared_ptr``) backed by ``ShmPTPEngine``:

.. code-block:: cpp

   auto machine = CreateGPTPShmMachine("shm", "/gptp_ptp_info");

Using in test environment
~~~~~~~~~~~~~~~~~~~~~~~~~~

Using in ITF
^^^^^^^^^^^^

Normal behavior is expected. TimeSlave runs as a standalone process, communicates over real Ethernet, and writes to ``/gptp_ptp_info`` shared memory as in production.

Using in Component Tests on the host
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Overview
''''''''

The ``TimeSlave`` and its constituent components can be tested on an x86 Linux host without PTP hardware or a real network. The key platform-dependent abstractions all have test-injectable counterparts:

.. list-table:: Testable Abstractions
   :header-rows: 1
   :widths: 30 35 35

   * - Abstraction
     - Production implementation
     - Test replacement
   * - ``IRawSocket``
     - ``RawSocket`` (AF_PACKET)
     - ``FakeSocket`` (push-based frame queue)
   * - ``INetworkIdentity``
     - ``NetworkIdentity`` (ioctl)
     - ``FakeIdentity`` (fixed clock identity)
   * - ``HighPrecisionLocalSteadyClock``
     - Platform clock (Linux / QNX)
     - ``FakeClock`` (returns fixed timestamp)

The ``GptpEngine`` provides a dedicated test constructor that accepts injected implementations:

.. code-block:: cpp

   GptpEngine engine(opts,
                     std::make_unique<FakeClock>(),
                     std::make_unique<FakeSocket>(),
                     std::make_unique<FakeIdentity>());

This allows complete white-box testing of the Sync/FollowUp correlation, peer-delay measurement, timeout detection, and time-jump flagging logic by pushing crafted PTP frames directly into the ``FakeSocket`` queue.

The ``GptpIpcPublisher`` and ``GptpIpcReceiver`` rely on POSIX shared memory (``shm_open``), which works on any Linux host, so ``ShmPTPEngine`` component tests can run end-to-end using real IPC without modification.
