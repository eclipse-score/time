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

.. _ts_client_detailed_design:

Time Sync Client Detailed Design
=================================

.. document:: Time Sync Client Detailed Design
   :id: doc__ts_client_detailed_design
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__sw_implementation
   :tags: ts_client

Description
-----------

The ``ts_client`` component provides shared memory-based IPC for gPTP time synchronization data exchange between TimeSlave and TimeDaemon processes. It implements a lock-free, single-writer/multi-reader communication channel using the seqlock protocol over POSIX shared memory.

Use Cases
~~~~~~~~~

1. Publishing time synchronization snapshots from TimeSlave to shared memory
2. Reading time synchronization snapshots from TimeDaemon
3. Lock-free concurrent access with bounded retry on torn reads

Rationale Behind Decomposition into Units
------------------------------------------

The ``ts_client`` component is decomposed into two implementation units:

1. **GptpIpcPublisher** — Creates and writes to the shared memory segment (TimeSlave side)
2. **GptpIpcReceiver** — Opens and reads from the shared memory segment (TimeDaemon side)

This separation enables independent deployment in different processes while maintaining a consistent IPC protocol.

Static Diagrams for Unit Interactions
--------------------------------------

Class View
~~~~~~~~~~

Main classes and their relationships:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ipc_channel.puml
   :alt: Class View

.. raw:: html

   </div>

Units within Time Sync Client
------------------------------

GptpIpcPublisher Unit
~~~~~~~~~~~~~~~~~~~~~

The ``GptpIpcPublisher`` component creates and manages the POSIX shared memory segment and writes ``GptpIpcData`` using the seqlock protocol.

Implementation Requirements
'''''''''''''''''''''''''''

The ``GptpIpcPublisher`` has the following requirements:

- The ``GptpIpcPublisher`` shall create a POSIX shared memory segment via ``shm_open()`` with ``O_CREAT`` flag
- The ``GptpIpcPublisher`` shall map the shared memory region as ``GptpIpcRegion`` aligned to 64 bytes
- The ``GptpIpcPublisher`` shall initialize the magic number field to ``0x47505450`` ('GPTP')
- The ``GptpIpcPublisher`` shall write ``GptpIpcData`` using the seqlock protocol:

  1. Increment ``seq`` (becomes odd — signals write in progress)
  2. Apply a release memory fence
  3. ``memcpy`` the ``GptpIpcData`` payload
  4. Store ``seq_confirm = seq + 1``
  5. Increment ``seq`` (both ``seq`` and ``seq_confirm`` become even — signals write complete)

- The ``GptpIpcPublisher`` shall use the default shared memory name ``/gptp_ptp_info`` unless overridden
- The ``GptpIpcPublisher`` shall support ``Destroy()`` to unmap and unlink the shared memory segment

GptpIpcReceiver Unit
~~~~~~~~~~~~~~~~~~~~

The ``GptpIpcReceiver`` component opens the shared memory segment read-only and reads ``GptpIpcData`` with bounded retry on torn reads.

Implementation Requirements
'''''''''''''''''''''''''''

The ``GptpIpcReceiver`` has the following requirements:

- The ``GptpIpcReceiver`` shall open the POSIX shared memory segment via ``shm_open()`` with ``O_RDONLY`` flag
- The ``GptpIpcReceiver`` shall map the shared memory region as read-only (``PROT_READ``)
- The ``GptpIpcReceiver`` shall validate the magic number (``0x47505450``) on ``Init()``
- The ``GptpIpcReceiver`` shall read ``GptpIpcData`` using the seqlock protocol with up to 20 retries:

  1. Read ``seq1`` with acquire ordering (must be even, otherwise retry)
  2. ``memcpy`` the ``GptpIpcData`` payload
  3. Apply an acquire-release fence
  4. Read ``seq_confirm`` as ``seq2`` and re-read ``seq`` as ``seq3``
  5. If ``seq1 == seq2 == seq3``, the read is consistent; otherwise retry

- The ``GptpIpcReceiver`` shall return ``std::optional<GptpIpcData>`` (empty if all retries exhausted)
- The ``GptpIpcReceiver`` shall support ``Close()`` to unmap the shared memory region

Seqlock Protocol Workflow
~~~~~~~~~~~~~~~~~~~~~~~~~~

The seqlock protocol ensures lock-free communication between publisher and receiver:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ipc_sequence.puml
   :alt: Seqlock Protocol

.. raw:: html

   </div>

Shared Memory Layout
~~~~~~~~~~~~~~~~~~~~

The ``GptpIpcRegion`` structure defines the shared memory layout:

.. code-block:: cpp

   struct alignas(64) GptpIpcRegion
   {
       std::atomic<std::uint32_t> magic{0x47505450};  // 'GPTP'
       std::atomic<std::uint32_t> seq{0};
       score::ts::GptpIpcData data{};
       std::atomic<std::uint32_t> seq_confirm{1};
   };

- Aligned to 64 bytes (cache line size) to prevent false sharing
- ``magic`` field validates the segment on reader initialization
- ``seq`` and ``seq_confirm`` implement the seqlock protocol
- ``data`` contains the ``GptpIpcData`` payload

Using in Test Environment
--------------------------

The ``GptpIpcPublisher`` and ``GptpIpcReceiver`` rely on POSIX shared memory (``shm_open``), which works on any Linux host. Component tests can run end-to-end using real IPC without platform-specific mocks.

Inspection Checklist
--------------------

The checklist for verification of the detailed design and code can be found here:

.. toctree::

   chklst_impl_inspection
