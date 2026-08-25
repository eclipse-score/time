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

.. _time_daemon_detailed_design:

Time Daemon Detailed Design
===========================

.. document:: Time Daemon Detailed Design
   :id: doc__time_daemon_detailed_design
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: YES
   :realizes: wp__sw_implementation
   :tags: time_daemon

Description
-----------

Use Cases
~~~~~~~~~

TimeDaemon is the non Autosar adaptive process who is intended to get the Vehicle Time from the ptp slave daemon (ptpd or any other), verify and validate the timepoints and distribute time information across the clients.

More precisely we can specify the following use cases for the time daemon:

1. Providing current Vehicle time to different applications
2. Setting the synchronization qualifier (aka Synchronized, Timeout, so on)
3. Providing needed information for diagnostics
4. Providing needed information for addition verification, ex SafeCarTime

The raw architectural diagram is represented below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/sad_deployment.puml
   :alt: Raw architectural diagram

.. raw:: html

   </div>

Rationale Behind Decomposition into Units
------------------------------------------

TimeDaemon is decomposed into six implementation units following SOLID principles
(Single Responsibility, Open/Closed) and design patterns (Publish-Subscribe, State Machines):

1. **Application** — Orchestrates initialization and lifecycle of all daemon components
2. **Message Broker** — Central publish-subscribe hub for decoupled inter-unit communication
3. **ControlFlowDivider** — Separates execution threads to prevent blocking and maintain data flow consistency
4. **PTP Machine** — Retrieves raw time data from PTP stack at consistent rates
5. **Verification Machine** — Validates and qualifies time data (sync status, jump detection, timeout)
6. **IPC Machine** — Exports qualified time snapshots to client applications via shared-memory interface

This separation enables independent testing, reusable implementations (e.g., different PTP stacks),
and clear responsibility boundaries critical for ASIL_B safety qualification.

Static Diagrams for Unit Interactions
--------------------------------------

Class View
~~~~~~~~~~

Main classes and unit relationships are presented on this diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/dd_class.puml
   :alt: Class View
   :width: 100%
   :align: center

.. raw:: html

   </div>

Deployment View
~~~~~~~~~~~~~~~

The design deployment and process architecture is represented on this diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/dd_deployment.puml
   :alt: Deployment View

.. raw:: html

   </div>

Dynamic Diagrams for Unit Interactions
---------------------------------------

Data and Control Flow
~~~~~~~~~~~~~~~~~~~~~

The data and control flow between units is presented in the following diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/dd_data_control_flow.puml
   :alt: Data and Control flow View

.. raw:: html

   </div>

On this view you could see several "workers" scopes:

1. PTP retrieving scope
2. PTPTimeInfo handling scope
3. PTPTimeInfo receiving on Application side scope

Each control flow is implemented with the dedicated thread or process and is independent form another ones.

Control flows
^^^^^^^^^^^^^

PTP retrieving scope
''''''''''''''''''''

This control flow is responsible for the:

1. retrieve the latest information from the ptp stack and
2. provide it to the ``PTPTimeInfo handling`` control flow

PTPTimeInfo handling scope
'''''''''''''''''''''''''''

This control flow is responsible for the:

1. Validate the time information, provided by the ``PTP retrieving`` workflow and
2. publish it to the ``Applications`` via some IPC

PTPTimeInfo receiving on Application side scope
''''''''''''''''''''''''''''''''''''''''''''''''

This control flow is responsible for the:

1. Propagate the time information from the ``PTPTimeInfo handling`` to the business logic of the applications.

Data types or events
^^^^^^^^^^^^^^^^^^^^

There are also several data types, which components are communicating to each other:

.. _raw_ptp_data:

Raw ptp data
''''''''''''

``raw_ptp_data`` is the data, which is provided by ``PTPMachine`` component and is just the raw data from ptp stack. is handled in the "PTP retrieving scope"

.. _input_ptp_data:

Input ptp data
''''''''''''''

``input_ptp_data`` is the same data as :ref:`raw_ptp_data` but which is handled already in "PTPTimeInfo handling scope"

.. _verified_ptp_data:

Verified ptp data
'''''''''''''''''

``verified_ptp_data`` is the :ref:`input_ptp_data` which was verified according to the business logic and updated accordingly. This data should be published to the Applications.

Units within Time Daemon
-------------------------

The following units comprise TimeDaemon's internal implementation:

Application Unit
~~~~~~~~~~~~~~~~

The ``Application`` component is the main entry point for the ``TimeDaemon``. It is responsible for orchestrating the overall lifecycle and initialization of all daemon components.

The ``TimebaseHandler`` component is an timebase-specific logic implementation. There might be several handlers available in the ``Application`` per amount of timebases supported. This separation allows for different timebase implementations while maintaining a consistent application structure.

Implementation Requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``Application`` has the following requirements:

- The ``Application`` shall implement the ``Initialize()`` method to create and initialize all daemon components
- The ``Application`` shall implement the ``Run()`` method to start all components and wait for termination
- The ``Application`` shall connect components to the ``MessageBroker`` by setting up all required subscriptions during initialization stage
- The ``Application`` shall support extension for different timebases.

Class view
^^^^^^^^^^

The Class Diagram is presented below:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/app/app_class.puml
   :alt: Class Diagram

.. raw:: html

   </div>

Initialization flow
^^^^^^^^^^^^^^^^^^^

During initialization, the ``Application`` uses the ``MachineFactory`` to create, configure and subscribe all components in a specific order:

- Create the ``MessageBroker`` first, as other components depend on it
- Create ProactiveMachines (``PtpMachine``, ``ControlFlowDivider``) that drive system behavior

  - Initialize each component
  - Set up MessageBroker subscriptions to component notifications
  - Set up component subscriptions to MessageBroker topics

- Create ReactiveMachines (``VerificationMachine``, ``IPCMachine``) that respond to events

  - Initialize each component
  - Set up MessageBroker subscriptions to component notifications
  - Set up component subscriptions to MessageBroker topics

The initialization workflow is represented in the following sequence diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/app/app_init_seq.puml
   :alt: Initialization workflow

.. raw:: html

   </div>

Execution and shutdown flow
^^^^^^^^^^^^^^^^^^^^^^^^^^^

During execution, the ``Application``:

- Starts all ``ProactiveMachines`` in the correct order
- Monitors the stop token for termination requests
- When termination is requested, stops all ``ProactiveMachines`` in reverse order

The execution and shutdown workflow is represented in the following sequence diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/app/app_workflow_seq.puml
   :alt: Execution workflow

.. raw:: html

   </div>



Message Broker Unit
~~~~~~~~~~~~~~~~~~~

The ``Message Broker`` component is the central communication hub that implements the Publish-Subscribe pattern within the ``TimeDaemon``. It enables decoupled communication between components by managing topics and distributing messages to interested subscribers.

The component maintains a registry of topics and their subscribers, delivering messages to all registered subscribers when a component publishes to a topic. This decoupling allows components to evolve independently without direct dependencies on each other.

Implementation Requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``Message Broker`` has the following requirements:

- The ``Message Broker`` shall maintain a registry of topics and their subscribers
- The ``Message Broker`` shall allow components to subscribe to topics of interest
- The ``Message Broker`` shall distribute messages to all subscribers when a topic is published to

Class view
^^^^^^^^^^

The Class Diagram is presented below:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/msg_broker/msg_broker_class.puml
   :alt: Class Diagram

.. raw:: html

   </div>

Initialization flow
^^^^^^^^^^^^^^^^^^^

During initialization, all machine objects, see ``BaseMachine``, the ``Application`` component needs to subscribe machines to ``Message Broker`` to the topics of interest.

The initialization workflow is represented in the following sequence diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/msg_broker/msg_broker_init_seq.puml
   :alt: Initialization workflow

.. raw:: html

   </div>

Message flow
^^^^^^^^^^^^

The message flow through the ``Message Broker`` is represented in the following sequence diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/msg_broker/msg_broker_workflow_seq.puml
   :alt: Message DiagramFlow

.. raw:: html

   </div>

Concurrency aspects
^^^^^^^^^^^^^^^^^^^

The ``Message Broker`` doesn't provide any synchronization between the publish-callback invoking processes.
Moreover, the callback invoke will happened in the scope of the thread, where the ``publish`` method is called.
To separate the control flows, the ``ControlFlowDivider`` shall be used.

Scalability
^^^^^^^^^^^

The ``Message Broker`` can be extended to support configuration-driven subscriptions, where topic relationships are defined in configuration files rather than hardcoded.



ControlFlowDivider Unit
~~~~~~~~~~~~~~~~~~~~~~~

The ``ControlFlowDivider`` component is responsible for separating control (execution) flows within the ``TimeDaemon`` and providing the execution control flow for the data processing. It contains dedicated threads where data is published to the ``Message Broker``, ensuring that blocking operations in one component do not affect the execution of other components and data missing is not affecting the data analysis in processing pipeline.

This component acts as a crucial intermediary that maintains the responsiveness of the system by decoupling the execution contexts of different operations, particularly between the PTP data retrieval and the time data processing pipelines.

Implementation Requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``ControlFlowDivider`` has the following requirements:

- The ``ControlFlowDivider`` shall provide separate execution threads for different control flows
- The ``ControlFlowDivider`` shall isolate components from execution time variations in other components
- The ``ControlFlowDivider`` shall maintain consistent data publishing rates to the subscribers
- The ``ControlFlowDivider`` shall push the last received data to the subscribers if there is no new data for some time with the predefined rate, to avoid data missing in the processing pipeline
- The ``ControlFlowDivider`` shall enable periodic processing of the pipeline through consistent event generation
- The ``ControlFlowDivider`` shall buffer incoming data from fast producers

Class view
^^^^^^^^^^

The Class Diagram is presented below:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ctrlflow/ctrlflow_class.puml
   :alt: Class Diagram

.. raw:: html

   </div>

Initialization flow
^^^^^^^^^^^^^^^^^^^

During initialization, the ``ControlFlowDivider`` performs the following steps:

- Initialize internal data structures (queue, mutex, condition variable)
- Create a worker thread to process data independently
- Start the worker thread which enters a waiting state

The initialization workflow is represented in the following sequence diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ctrlflow/ctrlflow_init_seq.puml
   :alt: Initialization workflow

.. raw:: html

   </div>

Message flow
^^^^^^^^^^^^

When the ``ControlFlowDivider`` receives new data from the ``PTP Machine`` via the ``Message Broker``, it processes it through the following workflow:

1. The ``Message Broker`` executes the onNewData callback and provides the new data
2. The data is placed in a thread-safe queue and exists from the callback
3. The worker thread wakes up, retrieves the data from the queue and
4. The worker thread publishes the retrieved data to the :ref:`input_ptp_data` topic
5. if there was no data for some timeout, the worker shall published the empty data to the :ref:`input_ptp_data` topic.

This separation of control flows ensures that slow or blocking operations in the PTP stack communication do not affect the responsiveness of time data processing in the ``TimeDaemon``.

The execution workflow is represented in the following sequence diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ctrlflow/ctrlflow_workflow_seq.puml
   :alt: Execution workflow

.. raw:: html

   </div>



PTP Machine Unit
~~~~~~~~~~~~~~~~

The ``PTP Machine`` component shall retrieve all needed information from the ptp stack (ex ``ptpd``) and provide it to the ``Message Broker`` for routing.
All communication with the ptp stack ight use ``devctl`` calls, which take some time, thus these calls shall be done in the dedicated thread.

Implementation Requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``PTP Machine`` has the following requirements:

- The ``PTP Machine`` shall retrieve the latest time information from the PTP stack (e.g., ``ptpd``)
- The ``PTP Machine`` shall publish retrieved time information to the ``Message Broker`` using the defined topic
- The ``PTP Machine`` shall format data according to the ``PtpTimeInfo`` structure required by downstream components
- The ``PTP Machine`` shall retrieve time information at a consistent rate to maintain time synchronization
- The ``PTP Machine`` shall maintain consistent publishing rates for time data even when experiencing delays in PTP stack communication.
- The ``PTP Machine`` shall support exchangeability with different PTP stack implementations

Class view
^^^^^^^^^^

The Class Diagram is presented below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ptp_machine/ptp_machine_class.puml
   :alt: Class Diagram

.. raw:: html

   </div>

As long as it wraps the particular communication with the ptp stack, the implementations should be easily exchangeable with another one in case of stack change.

Component initialization
^^^^^^^^^^^^^^^^^^^^^^^^

During initialization the ``PTP Machine`` shall initialize the ptp stack to be able to communicate with it.

The initialization workflow is described below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ptp_machine/ptp_machine_init_seq.puml
   :alt: Initialization workflow

.. raw:: html

   </div>

Publish new data
^^^^^^^^^^^^^^^^

After ``PTP Machine`` collects new data from the ptp stack, the component shall publish it to the ``Message Broker`` as :ref:`raw_ptp_data`.

The publish workflow is described below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ptp_machine/ptp_machine_get_new_data_seq.puml
   :alt: Publish workflow

.. raw:: html

   </div>

ShmPTPEngine Unit
~~~~~~~~~~~~~~~~~

The ``ShmPTPEngine`` component (in ``score::td::details``) is a ``PTP Machine`` implementation that reads ``GptpIpcData`` from the shared memory channel written by TimeSlave and converts it into the ``PtpTimeInfo`` structure expected by the TimeDaemon pipeline.

It is instantiated as ``GPTPShmMachine`` — a type alias for ``PTPMachine<details::ShmPTPEngine>`` — which connects ``ShmPTPEngine`` to the TimeDaemon's internal ``MessageBroker``.

Implementation Requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``ShmPTPEngine`` has the following requirements:

- The ``ShmPTPEngine`` shall call ``GptpIpcReceiver::Init(ipc_name)`` during ``Initialize()`` to open the shared memory channel
- The ``ShmPTPEngine`` shall call ``GptpIpcReceiver::Receive()`` in ``ReadPTPSnapshot()`` to fetch the latest ``GptpIpcData``
- The ``ShmPTPEngine`` shall map all fields of ``GptpIpcData`` to the corresponding fields of ``PtpTimeInfo`` (status flags, Sync/FollowUp data, peer-delay data, time references)
- The ``ShmPTPEngine`` shall call ``GptpIpcReceiver::Close()`` during ``Deinitialize()``
- The ``ShmPTPEngine`` shall be instantiatable with a configurable IPC channel name (default: ``/gptp_ptp_info``)

Class View
^^^^^^^^^^

The Class Diagram is presented below:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/shm_ptp_engine/shm_ptp_engine_class.puml
   :alt: Class Diagram

.. raw:: html

   </div>

Initialization
^^^^^^^^^^^^^^

During initialization the ``ShmPTPEngine`` shall open the shared memory channel to be able to read from it.

The initialization workflow is represented in the following sequence diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/shm_ptp_engine/shm_ptp_engine_init_seq.puml
   :alt: Initialization workflow

.. raw:: html

   </div>

Read PTP Snapshot
^^^^^^^^^^^^^^^^^

After ``ShmPTPEngine`` reads the latest ``GptpIpcData`` from shared memory, it maps it to ``PtpTimeInfo`` and publishes via the ``MessageBroker``.

The periodic read and publish workflow is described below:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/shm_ptp_engine/shm_ptp_engine_read_seq.puml
   :alt: Periodic read and publish workflow

.. raw:: html

   </div>

Data Mapping
^^^^^^^^^^^^

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
^^^^^^^

``CreateGPTPShmMachine(name, ipc_name)`` is a convenience factory function in ``score::td`` that creates a configured ``GPTPShmMachine`` (``shared_ptr``) backed by ``ShmPTPEngine``:

.. code-block:: cpp

   auto machine = CreateGPTPShmMachine("shm", "/gptp_ptp_info");



Verification Machine Unit
~~~~~~~~~~~~~~~~~~~~~~~~~

The ``Verification Machine`` component is responsible for validating and qualifying the time information received from the ``PTP Machine``. It applies various validation rules to ensure the time data meets quality requirements before distribution to applications.

The component implements a pipeline pattern where each stage performs a specific validation and adds appropriate qualifiers to the time data. This modular design allows for easy extension with additional validation steps.

Implementation Requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``Verification Machine`` has the following requirements:

- The ``Verification Machine`` shall validate and qualify time information received from the PTP Machine
- The ``Verification Machine`` shall validate if the time base is synchronized state
- The ``Verification Machine`` shall validate if the time base is in timeout state
- The ``Verification Machine`` shall validate timestamp for time jumps based on local clock
- The ``Verification Machine`` shall subscribe to the :ref:`input_ptp_data` topic via the ``Message Broker``
- The ``Verification Machine`` shall publish verified time data to the ``Message Broker`` using the :ref:`verified_ptp_data` topic
- The ``Verification Machine`` shall support extensibility to add new validation stages in the pipeline

Class view
^^^^^^^^^^

The Class Diagram is presented below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ver_machine/ver_class.puml
   :alt: Class Diagram

.. raw:: html

   </div>

Component initialization
^^^^^^^^^^^^^^^^^^^^^^^^

During initialization, the ``Verification Machine`` performs the following steps:

1. Set up the validation pipeline by creating and connecting validation stages

The component shall be subscribed by the ``Application`` to the :ref:`input_ptp_data` topic of the ``MessageBroker``

The initialization workflow is represented in the following sequence diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ver_machine/ver_init_seq.puml
   :alt: Initialization workflow

.. raw:: html

   </div>

Data verification workflow
^^^^^^^^^^^^^^^^^^^^^^^^^^

When the ``Verification Machine`` receives new PTP data, it processes it through the validation pipeline:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ver_machine/ver_verification_seq.puml
   :alt: Validation pipeline

.. raw:: html

   </div>



IPC Machine Unit
~~~~~~~~~~~~~~~~

The ``IPC Machine`` component shall get the :ref:`verified_ptp_data` from the ``Verification Machine`` and provide it to the ``VehicleClock`` backend (see :doc:`score::time — Unified Clock Interface <../../time/index>`) through a custom shared memory channel.

The component provides two sub components: publisher and receiver to be deployed on the TimeDaemon and Application sides accordingly.

Implementation Requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``IPC Machine`` has the following requirements:

- The ``IPC Machine`` shall provide verified time data to the ``VehicleClock`` backend component through a custom shared memory channel
- The ``IPC Machine`` shall create and initialize the IPC
- The ``IPC Machine`` shall support multiple client applications accessing the same time data
- The ``IPC Machine`` shall subscribe to the :ref:`verified_ptp_data` topic via the ``Message Broker``

Class view
^^^^^^^^^^

The Class Diagram is presented below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ipc/ipc_class.puml
   :alt: Class Diagram

.. raw:: html

   </div>

Component initialization
^^^^^^^^^^^^^^^^^^^^^^^^

Initialization is divided to two parts:

1. Initialization on the TimeDaemon side
2. Initialization on the Application side

Important thing, the shared memory IPC publisher shall be created and offered by the ``TimeDaemon`` before the Application side subscriber can connect. The Application shall retry until the service is found.

The main workflow is described below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ipc/ipc_init_seq.puml
   :alt: Main workflow

.. raw:: html

   </div>

The component shall be subscribed during initialization by the ``Application`` on the :ref:`verified_ptp_data` updates from the ``Message Broker``

Publish new data
^^^^^^^^^^^^^^^^

When ``IPC Machine`` receives the new :ref:`verified_ptp_data` from Message Broker, it shall serialize data and write it to shared memory.

As long as there are different use cases by using it, like:

1. Get current Vehicle time
2. Get data for diagnostics

All ``PtpTimeInfo`` data (or almost all) shall be published to the subscribed applications.

The publish workflow is described below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ipc/ipc_publish_seq.puml
   :alt: Publish workflow

.. raw:: html

   </div>

Receive data
^^^^^^^^^^^^

From Application side the receiver shall read from shared memory via the IPC receiver component and provide the data to the caller.

The receive workflow is described below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ipc/ipc_receive_seq.puml
   :alt: Receive workflow

.. raw:: html

   </div>



Logging configuration
~~~~~~~~~~~~~~~~~~~~~

The daemon should have the following logging contexts:

.. list-table:: Logging Contexts
   :header-rows: 1
   :widths: 30 20 50

   * - component
     - App/Context ID
     - Comments
   * - TimeDaemon
     - TDON
     - **T**\ ime\ **D**\ aem\ **ON**
   * - Application
     - TDAP
     - **T**\ ime\ **D**\ aemon **AP**\ plication
   * - MessageBroker
     - TDMB
     - **T**\ ime\ **D**\ aemon **M**\ essage\ **B**\ roker
   * - ControlFlowDivider
     - TDCD
     - **T**\ ime\ **D**\ aemon **C**\ ontrolFlow\ **D**\ ivider
   * - PTPMachine
     - TDPM
     - **T**\ ime\ **D**\ aemon **P**\ TP\ **M**\ achine
   * - ShmPTPEngine
     - GPTP
     - **GPTP** Shm adapter (Initialize / ReadPTPSnapshot)
   * - VerificationMachine
     - TDVM
     - **T**\ ime\ **D**\ aemon **V**\ erification\ **M**\ achine
   * - IPCMachine::receiver
     - TDIR
     - **T**\ ime\ **D**\ aemon **I**\ PCMachine::\ **R**\ eceiver
   * - IPCMachine::publisher
     - TDIP
     - **T**\ ime\ **D**\ aemon **I**\ PCMachine::\ **P**\ ublisher

Variability
~~~~~~~~~~~

Configuration files
^^^^^^^^^^^^^^^^^^^

The ``TimeDaemon`` uses structured configuration files to enable customization of its runtime behavior. These data could be configured:

1. Component-specific Configuration:

   a. Each component can have dedicated configuration sections
   b. Parameters such as update rates, timeouts, and thresholds can be specified

2. Topic Configuration:

   a. Topics for the ``Message Broker`` can be defined in configuration
   b. Publisher and subscriber relationships can be specified externally
   c. Component roles (publisher/subscriber) can be assigned through configuration

3. File Format and Structure: The configuration files use JSON format for readability and easy parsing:

.. code-block:: json

   {
     "message_broker": {
       "topics": [
         {
           "name": "raw_ptp_data",
           "publishers": ["PtpMachine"],
           "subscribers": ["ControlFlowDivider"]
         },
         {
           "name": "input_ptp_data",
           "publishers": ["ControlFlowDivider"],
           "subscribers": ["VerificationMachine"]
         },
         {
           "name": "verified_ptp_data",
           "publishers": ["VerificationMachine"],
           "subscribers": ["IPCMachine"]
         }
       ]
     },
     "ptp_machine": {
       "update_interval_ms": 50,
       "ptp_stack_type": "ptp",
       "ptp_stack_parameters": {
         "device": "/dev/ptp0"
       }
     },
     "control_flow_divider": {
       "timeout_ms": 500,
       "publishing_rate_ms": 100
     },
     "verification_machine": {
       "validation_stages": ["synchronization", "timejumps", "timeout"],
       "timejumps_parameters": {
         "max_backward_jump_ns": 100000
       },
       "timeout_parameters": {
         "threshold_ns": 100000
       }
     },
     "ipc_machine": {
       "shared_memory_name": "vehicle_time",
       "shared_memory_size": 4096
     }
   }

Scalability
^^^^^^^^^^^

The ``TimeDaemon``'s architecture supports scalability in the following ways:

Component Extensibility:
''''''''''''''''''''''''

1. New machine components can be added by implementing the ``BaseMachine`` interface
2. Additional validation stages can be plugged into the ``VerificationMachine`` pipeline
3. Alternative IPC mechanisms or communication with ptp stack can be implemented by alternative the ``IPCMachine`` or ``PTPMachine`` implementation

Example based on Qualified Vehicle Time integration
'''''''''''''''''''''''''''''''''''''''''''''''''''

The ``Qualified Vehicle Time`` integration extends the standard ``TimeDaemon`` architecture with:

1. A ``Qualified Vehicle Time`` component that performs additional time qualification and provide new topics: ``qualified_ptp_data`` and ``diagnostic_sct_data``
2. A dedicated IPC channel for SCT diagnostic data
3. A ``score::time::qvt`` library for diagnostic applications

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/examples/qvt/qvt_deployment.puml
   :alt: Deployment view

.. raw:: html

   </div>

The ``Qualified Vehicle Time`` component is integrated into the existing processing pipeline:

1. It subscribes to the :ref:`verified_ptp_data` topic from the ``VerificationMachine``
2. It processes and qualifies the time data with additional QVT-specific checks
3. It publishes two types of data:

   a. Qualified time data to the standard IPC Machine towards clients interested in the qualified Vehicle Time
   b. Diagnostic data to a dedicated QVT IPC channel towards Diagnostic and Central Validator notifications

The extended data flow with Qualified Vehicle Time integration is shown below:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/examples/qvt/qvt_data_control_flow.puml
   :alt: Data flow

.. raw:: html

   </div>

Example based on Absolute Time integration
''''''''''''''''''''''''''''''''''''''''''

Another example of the ``TimeDaemon`` extension is the integration of an ``Absolute Time`` source, such as GNSS, to provide absolute time information alongside the relative Vehicle Time from PTP.

The ``Absolute Time`` integration extends the standard ``TimeDaemon`` architecture with:

1. An ``SDatMachine`` component that retrieves absolute time from GNSS via SOMEIP or other sources and provide new topics: ``absolute_time_data``
2. A dedicated verification stage in the ``VerificationMachine`` for Absolute Time qualification
3. A dedicated IPC channel for Absolute Time data
4. A ``score::time::abs`` library for applications requiring absolute time on Clients side.

The way how it is integrated is presented below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/examples/abs_time/abs_time_deployment.puml
   :alt: Data flow

.. raw:: html

   </div>


The control and data flow with Absolute Time integration is shown below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/examples/abs_time/abs_time_data_control_flow.puml
   :alt: Data flow

.. raw:: html

   </div>

Using in test environment
~~~~~~~~~~~~~~~~~~~~~~~~~~

Using in ITF
^^^^^^^^^^^^

Normal behavior is expected.

Using in Component Tests on the host
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Overview
''''''''

The ``TimeDaemon`` can be utilized in the ``Component Tests`` environment to enable comprehensive testing of time-dependent components without relying on physical PTP hardware.
This approach allows test cases to manipulate time values and synchronization states to validate application behavior under various timing conditions.

For the Component tests the ``PtpMachine::PtpEngine`` library is the only one platform-dependent.
Thus the ``TimeDaemon`` components remain largely unchanged except for the ``PTPMachine`` component, which is replaced with an test-specific implementation that can be controlled via test cases
This component shall:

1. simulate "normal" ``PTPMachine`` behavior
2. have the communication channel to the test case and react on the manipulations

Next steps: plugin system
~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``TimeDaemon`` could be extended with a flexible plugin system that enables dynamic component loading, configuration, subscription and extension without requiring code changes or recompilation.

Plugin Architecture
^^^^^^^^^^^^^^^^^^^

The plugin system is structured around the following key elements:

1. ``Component Registry``: A central registry that maintains information about available component implementations
2. ``Component Factory``: Creates component instances based on configuration
3. ``Plugin Manager``: Loads and initializes plugins at runtime
4. ``Configuration-Driven Assembly``: Components and their relationships defined in configuration files

Component Creation Process
^^^^^^^^^^^^^^^^^^^^^^^^^^^

During ``TimeDaemon`` initialization:

1. The ``Plugin Manager`` loads all specified plugins from configured directories or bazel targets
2. Each plugin registers its component factories with the registry
3. The ``Application`` reads the component configuration
4. For each component in the configuration:

   a. The appropriate factory is retrieved from the registry
   b. The component is created with its specified parameters
   c. Components are connected based on the ``MessageBroker`` topic configuration

ASIL-B qualification
~~~~~~~~~~~~~~~~~~~~~

Clean separation of concerns allows the ``VehicleClock`` ``td_impl`` backend as well as ``TimeDaemon`` to be qualified according to ASIL-B requirements following ISO 26262 standard.

Inspection Checklist
---------------------

The checklist for verification of the detailed design and code can be found here:

.. toctree::

   chklst_impl_inspection
