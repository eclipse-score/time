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
4. Publishing the resulting ``PtpTimeInfo`` to shared memory for consumption by the TimeDaemon

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

1. periodically call ``GptpEngine::ReadPTPSnapshot()`` to get the latest time measurement
2. enrich the snapshot with the local clock timestamp from ``HighPrecisionLocalSteadyClock``
3. publish to shared memory via ``GptpIpcPublisher::Publish()``

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

``PtpTimeInfo`` is the aggregated snapshot that combines PTP status flags, Sync/FollowUp data, peer delay data, and a local clock reference. This is the data published to shared memory for the TimeDaemon.

SW Components decomposition
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TimeSlave Application SW component
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``TimeSlave Application`` component is the main entry point for the TimeSlave process. It extends ``score::mw::lifecycle::Application`` and is responsible for orchestrating the overall lifecycle of the GptpEngine and the IPC publisher.

Component requirements
''''''''''''''''''''''

The ``TimeSlave Application`` has the following requirements:

- The ``TimeSlave Application`` shall implement the ``Initialize()`` method to create the ``GptpEngine`` with configured options, initialize the ``GptpIpcPublisher`` (creates the shared memory segment), and prepare the ``HighPrecisionLocalSteadyClock`` for local time reference
- The ``TimeSlave Application`` shall implement the ``Run()`` method to start the GptpEngine, enter a periodic publish loop, and monitor the ``stop_token`` for graceful shutdown
- The ``TimeSlave Application`` shall implement the ``Deinitialize()`` method to stop the GptpEngine threads and destroy the shared memory segment
- The ``TimeSlave Application`` shall periodically read the latest ``PtpTimeInfo`` snapshot, enrich it with the local clock timestamp, and publish it via ``GptpIpcPublisher``

GptpEngine SW component
^^^^^^^^^^^^^^^^^^^^^^^^

The ``GptpEngine`` component is the core gPTP protocol engine. It manages two background threads (RxThread and PdelayThread) for network I/O and peer delay measurement, and exposes a thread-safe ``ReadPTPSnapshot()`` method for the main thread to read the latest time measurement.

Component requirements
''''''''''''''''''''''

The ``GptpEngine`` has the following requirements:

- The ``GptpEngine`` shall manage an RxThread for receiving and parsing gPTP frames from raw Ethernet sockets
- The ``GptpEngine`` shall manage a PdelayThread for periodic peer delay measurement
- The ``GptpEngine`` shall provide a thread-safe ``ReadPTPSnapshot()`` method that returns the latest ``PtpTimeInfo``
- The ``GptpEngine`` shall support configurable parameters via ``GptpEngineOptions`` (interface name, PDelay interval, sync timeout, time jump thresholds, PHC configuration)
- The ``GptpEngine`` shall support exchangeability of the raw socket implementation for different platforms (Linux, QNX)

Class view
''''''''''

The Class Diagram is presented below:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/gptp_engine_class.puml
   :alt: Class Diagram

.. raw:: html

   </div>

Threading model
'''''''''''''''

The GptpEngine operates with two background threads. The threading model is represented below:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/gptp_threading.puml
   :alt: Threading Model

.. raw:: html

   </div>

Concurrency aspects
'''''''''''''''''''

The ``GptpEngine`` uses the following synchronization mechanisms:

- A ``std::mutex`` protects the ``latest_snapshot_`` field, shared between the RxThread (writer) and the main thread (reader via ``ReadPTPSnapshot()``)
- The ``PeerDelayMeasurer`` uses its own ``std::mutex`` to synchronize between the PdelayThread (``SendRequest()``) and the RxThread (``OnResponse()``, ``OnResponseFollowUp()``)
- The ``SyncStateMachine`` uses ``std::atomic<bool>`` for the timeout flag, which is read from the main thread and written from the RxThread

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

Component requirements
''''''''''''''''''''''

The ``PeerDelayMeasurer`` has the following requirements:

- The ``PeerDelayMeasurer`` shall transmit PDelayReq frames and capture the hardware transmit timestamp (``t1``)
- The ``PeerDelayMeasurer`` shall receive PDelayResp (providing ``t2``, ``t4``) and PDelayRespFollowUp (providing ``t3c``) messages
- The ``PeerDelayMeasurer`` shall compute the peer delay using the IEEE 802.1AS formula: ``path_delay = ((t2 - t1) + (t4 - t3c)) / 2``
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

libTSClient SW component
^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``libTSClient`` component is the shared memory IPC library that connects the TimeSlave process to the TimeDaemon process. It provides a lock-free, single-writer/multi-reader communication channel using a seqlock protocol over POSIX shared memory.

The component provides two sub components: publisher and receiver to be deployed on the TimeSlave and TimeDaemon sides accordingly.

Component requirements
''''''''''''''''''''''

The ``libTSClient`` has the following requirements:

- The ``libTSClient`` shall define a shared memory layout (``GptpIpcRegion``) with a magic number for validation, an atomic seqlock counter, and a ``PtpTimeInfo`` data payload
- The ``libTSClient`` shall align the shared memory region to 64 bytes (cache line size) to prevent false sharing
- The ``libTSClient`` shall provide a ``GptpIpcPublisher`` component that creates and manages the POSIX shared memory segment and writes ``PtpTimeInfo`` using the seqlock protocol
- The ``libTSClient`` shall provide a ``GptpIpcReceiver`` component that opens the shared memory segment read-only and reads ``PtpTimeInfo`` with up to 20 seqlock retries
- The ``libTSClient`` shall use the POSIX shared memory name ``/gptp_ptp_info`` by default

Class view
''''''''''

The Class Diagram is presented below:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ipc_channel.puml
   :alt: Class Diagram

.. raw:: html

   </div>

Publish new data
''''''''''''''''

When ``TimeSlave Application`` has a new ``PtpTimeInfo`` snapshot, it publishes to the shared memory via the seqlock protocol:

1. Increment ``seq`` (becomes odd — signals write in progress)
2. ``memcpy`` the data
3. Increment ``seq`` (becomes even — signals write complete)

Receive data
''''''''''''

From TimeDaemon side, the receiver reads from the shared memory using the seqlock protocol with bounded retry:

1. Read ``seq`` (must be even, otherwise retry)
2. ``memcpy`` the data
3. Read ``seq`` again (must match step 1, otherwise retry — torn read detected)
4. Return ``std::optional<PtpTimeInfo>`` (empty if all 20 retries exhausted)

The seqlock protocol workflow is presented in the following sequence diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ipc_sequence.puml
   :alt: Seqlock Protocol

.. raw:: html

   </div>

Platform support
~~~~~~~~~~~~~~~~~

TimeSlave supports two target platforms with platform-specific implementations selected at compile time via Bazel ``select()``:

.. list-table:: Platform Implementations
   :header-rows: 1
   :widths: 25 35 40

   * - Component
     - Linux
     - QNX
   * - Raw Socket
     - ``AF_PACKET`` with ``SO_TIMESTAMPING``
     - QNX raw-socket shim
   * - Network Identity
     - ``ioctl(SIOCGIFHWADDR)``
     - QNX-specific MAC resolution
   * - PHC Adjuster
     - ``clock_adjtime()``
     - EMAC PTP ioctls
   * - HighPrecisionLocalSteadyClock
     - ``std::chrono`` system clock
     - QTIME clock API

The ``IRawSocket`` and ``INetworkIdentity`` interfaces provide the abstraction boundary. Platform-specific source files are organized under ``score/TimeSlave/code/gptp/platform/linux/`` and ``score/TimeSlave/code/gptp/platform/qnx/``.

Instrumentation
~~~~~~~~~~~~~~~~

ProbeManager
^^^^^^^^^^^^

The ``ProbeManager`` is a singleton that records probe events at key processing points in the gPTP engine. Probe points include:

- ``RxPacketReceived`` — Raw frame received from socket
- ``SyncFrameParsed`` — Sync message successfully parsed
- ``FollowUpProcessed`` — Offset computed from Sync/FollowUp pair
- ``OffsetComputed`` — Final offset value available
- ``PdelayReqSent`` — PDelayReq frame transmitted
- ``PdelayCompleted`` — Peer delay measurement completed
- ``PhcAdjusted`` — PHC adjustment applied

The ``GPTP_PROBE()`` macro provides zero-overhead when probing is disabled.

Recorder
^^^^^^^^^

Thread-safe CSV file writer that persists probe events and other diagnostic data. Each ``RecordEntry`` contains a timestamp, event type, offset, peer delay, sequence ID, and status flags.

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
   * - ``interface_name``
     - string
     - Network interface for gPTP frames (e.g., ``eth0``)
   * - ``pdelay_interval_ms``
     - uint32_t
     - Interval between PDelayReq transmissions
   * - ``sync_timeout_ms``
     - uint32_t
     - Timeout for Sync message reception before declaring timeout state
   * - ``time_jump_forward_ns``
     - int64_t
     - Threshold for forward time jump detection
   * - ``time_jump_backward_ns``
     - int64_t
     - Threshold for backward time jump detection
   * - ``phc_config``
     - PhcConfig
     - PHC device path, step threshold, and enable flag

The ``PhcConfig`` struct additionally contains:

.. list-table:: PhcAdjuster Configuration
   :header-rows: 1
   :widths: 30 15 55

   * - Parameter
     - Type
     - Description
   * - ``enabled``
     - bool
     - Enable or disable PHC adjustment
   * - ``device_path``
     - string
     - Path to the PHC device (e.g., ``/dev/ptp0``)
   * - ``step_threshold_ns``
     - int64_t
     - Offset threshold above which a step correction is applied instead of frequency slew
