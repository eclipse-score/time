Concept for TimeDaemon
======================

.. contents:: Table of Contents
   :depth: 3
   :local:

TimeDaemon concept
------------------

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

Components decomposition
~~~~~~~~~~~~~~~~~~~~~~~~~

The design consists of several sw components:

1. `Application <#application-sw-component>`_
2. `Message Broker <#message-broker-sw-component>`_
3. `ControlFlowDivider <#controlflowdivider-sw-component>`_
4. `PTP Machine <#ptp-machine-sw-component>`_
5. `Verification Machine <#verification-machine-sw-component>`_
6. `IPC Machine <#ipc-machine-sw-component>`_
7. `score::time::svt <#score-time-synchronizedvehicletime-sw-component>`_

Deployment view
~~~~~~~~~~~~~~~

The design deployment is represented on the following diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/dd_deployment.puml
   :alt: Deployment View

.. raw:: html

   </div>

Class view
~~~~~~~~~~

Main classes and components are presented on this diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/dd_class.puml
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

Raw ptp data
''''''''''''

``raw_ptp_data`` is the data, which is provided by ``PTPMachine`` component and is just the raw data from ptp stack. is handled in the "PTP retrieving scope"

Input ptp data
''''''''''''''

``input_ptp_data`` is the same data as `raw_ptp_data <#raw-ptp-data>`_ but which is handled already in "PTPTimeInfo handling scope"

Verified ptp data
'''''''''''''''''

``verified_ptp_data`` is the `input_ptp_data <#input-ptp-data>`_ which was verified according to the business logic and updated accordingly. This data should be published to the Applications.

SW Components decomposition
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Application SW component
^^^^^^^^^^^^^^^^^^^^^^^^^

The ``Application`` component is the main entry point for the ``TimeDaemon``. It is responsible for orchestrating the overall lifecycle and initialization of all daemon components.

The ``TimebaseHandler`` component is an timebase-specific logic implementation. There might be several handlers available in the ``Application`` per amount of timebases supported. This separation allows for different timebase implementations while maintaining a consistent application structure.

Component requirements
''''''''''''''''''''''

The ``Application`` has the following requirements:

- The ``Application`` shall implement the ``Initialize()`` method to create and initialize all daemon components
- The ``Application`` shall implement the ``Run()`` method to start all components and wait for termination
- The ``Application`` shall connect components to the ``MessageBroker`` by setting up all required subscriptions during initialization stage
- The ``Application`` shall support extension for different timebases.

Class view
''''''''''

The Class Diagram is presented below:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/app/app_class.puml
   :alt: Class Diagram

.. raw:: html

   </div>

Initialization flow
'''''''''''''''''''

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
''''''''''''''''''''''''''''

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

Message Broker SW component
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``Message Broker`` component is the central communication hub that implements the Publish-Subscribe pattern within the ``TimeDaemon``. It enables decoupled communication between components by managing topics and distributing messages to interested subscribers.

The component maintains a registry of topics and their subscribers, delivering messages to all registered subscribers when a component publishes to a topic. This decoupling allows components to evolve independently without direct dependencies on each other.

Component requirements
''''''''''''''''''''''

The ``Message Broker`` has the following requirements:

- The ``Message Broker`` shall maintain a registry of topics and their subscribers
- The ``Message Broker`` shall allow components to subscribe to topics of interest
- The ``Message Broker`` shall distribute messages to all subscribers when a topic is published to

Class view
''''''''''

The Class Diagram is presented below:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/msg_broker/msg_broker_class.puml
   :alt: Class Diagram

.. raw:: html

   </div>

Initialization flow
'''''''''''''''''''

During initialization, all machine objects, see ``BaseMachine``, the ``Application`` component needs to subscribe machines to ``Message Broker`` to the topics of interest.

The initialization workflow is represented in the following sequence diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/msg_broker/msg_broker_init_seq.puml
   :alt: Initialization workflow

.. raw:: html

   </div>

Message flow
''''''''''''

The message flow through the ``Message Broker`` is represented in the following sequence diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/msg_broker/msg_broker_workflow_seq.puml
   :alt: Message DiagramFlow

.. raw:: html

   </div>

Concurrency aspects
'''''''''''''''''''

The ``Message Broker`` doesn't provide any synchronization between the publish-callback invoking processes.
Moreover, the callback invoke will happened in the scope of the thread, where the ``publish`` method is called.
To separate the control flows, the `ControlFlowDivider <#controlflowdivider-sw-component>`_ shall be used

Scalability
'''''''''''

The ``Message Broker`` can be extended to support configuration-driven subscriptions, where topic relationships are defined in configuration files rather than hardcoded.

ControlFlowDivider SW component
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``ControlFlowDivider`` component is responsible for separating control (execution) flows within the ``TimeDaemon`` and providing the execution control flow for the data processing. It contains dedicated threads where data is published to the ``Message Broker``, ensuring that blocking operations in one component do not affect the execution of other components and data missing is not affecting the data analysis in processing pipeline.

This component acts as a crucial intermediary that maintains the responsiveness of the system by decoupling the execution contexts of different operations, particularly between the PTP data retrieval and the time data processing pipelines.

Component requirements
''''''''''''''''''''''

The ``ControlFlowDivider`` has the following requirements:

- The ``ControlFlowDivider`` shall provide separate execution threads for different control flows
- The ``ControlFlowDivider`` shall isolate components from execution time variations in other components
- The ``ControlFlowDivider`` shall maintain consistent data publishing rates to the subscribers
- - The ``ControlFlowDivider`` shall push the last received data to the subscribers if there is no new data for some time with the predefined rate, to avoid data missing in the processing pipeline
- The ``ControlFlowDivider`` shall enable periodic processing of the pipeline through consistent event generation
- The ``ControlFlowDivider`` shall buffer incoming data from fast producers

Class view
''''''''''

The Class Diagram is presented below:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ctrlflow/ctrlflow_class.puml
   :alt: Class Diagram

.. raw:: html

   </div>

Initialization flow
'''''''''''''''''''

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
''''''''''''

When the ``ControlFlowDivider`` receives new data from the ``PTP Machine`` via the ``Message Broker``, it processes it through the following workflow:

1. The ``Message Broker`` executes the onNewData callback and provides the new data
2. The data is placed in a thread-safe queue and exists from the callback
3. The worker thread wakes up, retrieves the data from the queue and
4. The worker thread publishes the retrieved data to the `input_ptp_data <#input-ptp-data>`_ topic
5. if there was no data for some timeout, the worker shall published the empty data to the `input_ptp_data <#input-ptp-data>`_ topic.

This separation of control flows ensures that slow or blocking operations in the PTP stack communication do not affect the responsiveness of time data processing in the ``TimeDaemon``.

The execution workflow is represented in the following sequence diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ctrlflow/ctrlflow_workflow_seq.puml
   :alt: Execution workflow

.. raw:: html

   </div>

PTP Machine SW component
^^^^^^^^^^^^^^^^^^^^^^^^^

The ``PTP Machine`` component shall retrieve all needed information from the ptp stack (ex ``ptpd``) and provide it to the ``Message Broker`` for routing.
All communication with the ptp stack ight use ``devctl`` calls, which take some time, thus these calls shall be done in the dedicated thread.

Component requirements
''''''''''''''''''''''

The ``PTP Machine`` has the following requirements:

- The ``PTP Machine`` shall retrieve the latest time information from the PTP stack (e.g., ``ptpd``)
- The ``PTP Machine`` shall publish retrieved time information to the ``Message Broker`` using the defined topic
- The ``PTP Machine`` shall format data according to the ``PTPTimeInfo`` structure required by downstream components
- The ``PTP Machine`` shall retrieve time information at a consistent rate to maintain time synchronization
- The ``PTP Machine`` shall maintain consistent publishing rates for time data even when experiencing delays in PTP stack communication.
- The ``PTP Machine`` shall support exchangeability with different PTP stack implementations

Class view
''''''''''

The Class Diagram is presented below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ptp_machine/ptp_machine_class.puml
   :alt: Class Diagram

.. raw:: html

   </div>

As long as it wraps the particular communication with the ptp stack, the implementations should be easily exchangeable with another one in case of stack change.

Component initialization
'''''''''''''''''''''''''

During initialization the ``PTP Machine`` shall initialize the ptp stack to be able to communicate with it.

The initialization workflow is described below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ptp_machine/ptp_machine_init_seq.puml
   :alt: Initialization workflow

.. raw:: html

   </div>

Publish new data
''''''''''''''''

After ``PTP Machine`` collects new data from the ptp stack, the component shall publish it to the ``Message Broker`` as `raw-ptp-data <#raw-ptp-data>`_.

The publish workflow is described below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ptp_machine/ptp_machine_get_new_data_seq.puml
   :alt: Publish workflow

.. raw:: html

   </div>

Verification Machine SW component
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``Verification Machine`` component is responsible for validating and qualifying the time information received from the ``PTP Machine``. It applies various validation rules to ensure the time data meets quality requirements before distribution to applications.

The component implements a pipeline pattern where each stage performs a specific validation and adds appropriate qualifiers to the time data. This modular design allows for easy extension with additional validation steps.

Component requirements
''''''''''''''''''''''

The ``Verification Machine`` has the following requirements:

- The ``Verification Machine`` shall validate and qualify time information received from the PTP Machine
- The ``Verification Machine`` shall validate if the time base is synchronized state
- The ``Verification Machine`` shall validate if the time base is in timeout state
- The ``Verification Machine`` shall validate timestamp for time jumps based on local clock
- The ``Verification Machine`` shall subscribe to the `input_ptp_data <#input-ptp-data>`_ topic via the ``Message Broker``
- The ``Verification Machine`` shall publish verified time data to the ``Message Broker`` using the `verified-ptp-data <#verified-ptp-data>`_ topic
- The ``Verification Machine`` shall support extensibility to add new validation stages in the pipeline

Class view
''''''''''

The Class Diagram is presented below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ver_machine/ver_class.puml
   :alt: Class Diagram

.. raw:: html

   </div>

Component initialization
'''''''''''''''''''''''''

During initialization, the ``Verification Machine`` performs the following steps:

1. Set up the validation pipeline by creating and connecting validation stages

The component shall be subscribed by the ``Application`` to the `input_ptp_data <#input-ptp-data>`_ topic of the ``MessageBroker``

The initialization workflow is represented in the following sequence diagram:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ver_machine/ver_init_seq.puml
   :alt: Initialization workflow

.. raw:: html

   </div>

Data verification workflow
'''''''''''''''''''''''''''

When the ``Verification Machine`` receives new PTP data, it processes it through the validation pipeline:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ver_machine/ver_verification_seq.puml
   :alt: Validation pipeline

.. raw:: html

   </div>

IPC Machine SW component
^^^^^^^^^^^^^^^^^^^^^^^^^

The ``IPC Machine`` component shall get the `verified-ptp-data <#verified-ptp-data>`_ from the ``Verification Machine`` and provide it to the ``score::time::svt`` through the ``score::communication`` module. As the fast initial implementation, a custom shared memory backend is used.

The component provides two sub components: publisher and receiver to be deployed on the TimeDaemon and Application sides accordingly.

Component requirements
''''''''''''''''''''''

The ``IPC Machine`` has the following requirements:

- The ``IPC Machine`` shall provide verified time data to the ``score::time::svt`` component through the ``score::communication`` module
- The ``IPC Machine`` shall create and initialize the IPC
- The ``IPC Machine`` shall support multiple client applications accessing the same time data
- The ``IPC Machine`` shall subscribe to the `verified_ptp_data <#verified-ptp-data>`_ topic via the ``Message Broker``

Class view
''''''''''

The Class Diagram is presented below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ipc/ipc_class.puml
   :alt: Class Diagram

.. raw:: html

   </div>

Component initialization
'''''''''''''''''''''''''

Initialization is divided to two parts:

1. Initialization on the TimeDaemon side
2. Initialization on the Application side

Important thing, the ``score::communication`` publisher shall be created and offered by the ``TimeDaemon`` before the Application side subscriber can connect. The Application shall retry until the service is found.

The main workflow is described below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ipc/ipc_init_seq.puml
   :alt: Main workflow

.. raw:: html

   </div>

The component shall be subscribed during initialization by the ``Application`` on the `verified-ptp-data <#verified-ptp-data>`_ updates from the ``Message Broker``

Publish new data
''''''''''''''''

When ``IPC Machine`` receives the new `verified-ptp-data <#verified-ptp-data>`_ from Message Broker, it shall serialize data and publish it via ``score::communication``.

As long as there are different use cases by using it, like:

1. Get current Vehicle time
2. Get data for diagnostics

All ``PTPTimeInfo`` data (or almost all) shall be published to the subscribed applications.

The publish workflow is described below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ipc/ipc_publish_seq.puml
   :alt: Publish workflow

.. raw:: html

   </div>

Receive data
''''''''''''

From Application side the receiver shall subscribe via ``score::communication`` and provide the data to the caller.

The receive workflow is described below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ipc/ipc_receive_seq.puml
   :alt: Receive workflow

.. raw:: html

   </div>

score::time::SynchronizedVehicleTime SW Component
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``score::time::svt`` is the interface towards Applications, how they could get the access to the Vehicle Time.

Component requirements
''''''''''''''''''''''

The ``score::time::svt`` has the following requirements:

- The ``score::time::svt`` shall expose vehicle time amd it synchronization status to applications
- The ``score::time::svt`` shall retrieve time data from ``IPC Machine`` receiver component
- The ``score::time::svt`` shall adjust vehicle time with local clock to provide accurate timestamps
- The ``score::time::svt`` shall support fast and low-latency time access via the ``Now()`` method

Class view
''''''''''

The Class Diagram is presented below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/mw/mw_class.puml
   :alt: Class Diagram

.. raw:: html

   </div>

Receive data
''''''''''''

In case of receiving data, the ``Application`` shall just call ``score::time::svt::Now()`` and it shall return the latest published Vehicle Time, which is already adjusted with local clock.

To do so, in the ``score::time::svt`` there is a thread, who polls for new data the ``IPCMachine::receiver`` and put the data to the process-internal shared buffer (memory), from where it is being read on ``score::time::svt::Now()`` call.

The main workflow is described below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/mw/mw_time_receive_seq.puml
   :alt: Receive data workflow

.. raw:: html

   </div>

This design guarantees very low latency of the executing the ``score::time::svt::Now()`` function but brings additional efforts for the thread, memory buffer, synchronizing and so on.

Receive data (simplified)
''''''''''''''''''''''''''

As an alternative design, the receiving concept could be simplified and ``score::time::svt::Now()`` could directly invoke the ``IPCMachine::receiver`` call, adjust the ``Vehicle time`` and return it to the ``Application``.

The design is represented below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/mw/mw_time_receive_simple_seq.puml
   :alt: Receive data (simplified) workflow

.. raw:: html

   </div>

In this case, there will be no need for additional thread, shared buffer and synchronization, but the ``score::time::svt::Now()`` call will take longer. To decide which approach to use, additional tests shall be

Deployment
''''''''''

The implementation of ``score::time::svt::details::timed`` could be placed in parallel to other implementations, like ``score::time::svt::details::mocked`` one and could be selected by Bazel select. Also it will ease the integration process.

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

1. It subscribes to the `verified_ptp_data <#verified-ptp-data>`_ topic from the ``VerificationMachine``
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

Clean separation of concerns allows ``score::time::svt`` as well as ``TimeDaemon`` to be qualified according to ASIL-B requirements following ISO 26262 standard.
