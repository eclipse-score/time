#####################
Software Architecture
#####################

*******************
Required Interfaces
*******************

- The ``score-ptp-daemon`` is a background daemon process that receives/sends the PTP packets over Ethernet - facilitating
  Time Synchronization over Ethernet.
- The PTP messages are forwarded to the AUTOSAR Adaptive TimeSynchronization stack via the ``tsync-ptp-lib`` module.
- ``tsync-ptp-lib`` provides the following APIs that help facilitate the forwarding of time messages -

   +-------------------------------------------+-----------------------------------------------------+
   | **API Name**                              | **Description**                                     |
   +-------------------------------------------+-----------------------------------------------------+
   | TSync_Open                                | API to perform basic initialization of the          |
   |                                           | tsync-ptp-lib.                                      |
   +-------------------------------------------+-----------------------------------------------------+
   | TSync_Close                               | API to shutdown the tsync-ptp-lib.                  |
   +-------------------------------------------+-----------------------------------------------------+
   | TSync_OpenTimebase                        | API to open a timebase for reading and writing.     |
   |                                           | (For every configured time base)                    |
   +-------------------------------------------+-----------------------------------------------------+
   | TSync_CloseTimebase                       | API to close and invalidate the given timebase      |
   |                                           | handle.                                             |
   +-------------------------------------------+-----------------------------------------------------+
   | TSync_BusSetGlobalTime                    | API to allow the Time Base Provider Modules to      |
   |                                           | forward a new Global Time tuple (i.e., the Received |
   |                                           | Time Tuple) to the tsync-ptp-lib                    |
   +-------------------------------------------+-----------------------------------------------------+
   | TSync_BusGetGlobalTime                    | API to allow the Time Base Provider Modules to      |
   |                                           | read new Global Time tuple from tsync-ptp-lib       |
   |                                           | and send over Ethenet.                              |
   +-------------------------------------------+-----------------------------------------------------+
   | TSync_GetCurrentVirtualLocalTime          | API to read Virtual Local Time of the referenced    |
   |                                           | Time Base.                                          |
   +-------------------------------------------+-----------------------------------------------------+
   | TSync_RegisterTransmitGlobalTimeCallback  | API to register trigger transmit callback.          |
   |                                           | tsync-ptp-lib will call this callback to trigger    |
   |                                           | immediate time transmission.                        |
   +-------------------------------------------+-----------------------------------------------------+
   | TSync_GetTimebaseConfiguration            | API to read configuration parameters                |
   |                                           | from score::time.                                    |
   +-------------------------------------------+-----------------------------------------------------+


************************
Autosar TLVs and SubTLVs
************************

Overview
========

- To enable the Autosar TLV and SubTLVs, please refer the product documentation of ``tsync-daemon`` from the AUTOSAR
  Adaptive layer.
- When they are enabled, the ``FollowUp`` packet will be appended with both Autosar TLV and configured SubTLVs.
- They are briefly explained below:

Autosar TLV
-----------

- Except for the ``lengthField``, all the other field values are static.
- The values are provided in the Protocol Specification, and explained in detail as part of the :ref:`Product Documentation<tsync_tlv_format>`

Autosar SubTLV
--------------

- Each SubTLV payload is individually configurable. However, the order of the SubTLV is always the same.
- Few SubTLVs can be "Secured" if CRC is globally enabled via the configuration parameter ``CRC-SECURED`` (in case of
  Providers) and ``CRC-VALIDATED`` (in case of Consumers) in arxml.

SubTLV Types
============

There are 5 types of SubTLVs:

- Time Secured
- UserData Secured/Non-Secured
- Status Secured/Non-Secured (out-of-scope, see below)
- OFS Secured/Non-Secured (out-of-scope, see below)

Time Secured
------------

- Always Secured, no "Unsecured" variant.
- Used to detect the integrity of the "time values" in FollowUp messages.
- The SubTLV is available only when the CRC is globally enabled in the configuration.
- Since there is only CRC enabled version of this SubTLV, if the CRC is not enabled globally, this SubTLV will not be included.
- Here are the list of "time values" that are considered for the CRC computation.

  - messageLength
  - domainNumber
  - correctionField
  - sourcePortIdentity
  - sequenceIdentity
  - preciseOriginTimestamp

- There are 2 CRCs associated with this SubTLV - ``CRC_Time_0`` and ``CRC_Time_1``

  - ``CRC_Time_0`` is computed using ``domainNumber``, ``sourcePortIdentity`` and ``preciseOriginTimestamp``
  - ``CRC_Time_1`` is computed using ``messageLength``, ``correctionField`` and ``sequenceId``

.. note::
   Not all the "time values" are necessarily used. Only the ones that are configured will be considered for the CRC computation.

Userdata Secured/Not-Secured
----------------------------

- Can be Secured or Not-Secured
- Used to transmit 3 bytes of user defined values.
- The SubTLV can be CRC protected if CRC is enabled globally.

Status Secured/Not-Secured
--------------------------

- Can be Secured or Not-Secured, used to transmit the status of Sync-To-Gateway bit.
- The SubTLV can be CRC protected if CRC is enabled globally.

.. important::
   Since the Gateway use-case is not supported by AUTOSAR Adaptive as of today, this SubTLV is out-of-scope for now.

OFS Secured/Not-Secured
-----------------------

 - Out of scope, since Offset timebases are slated to be deprecated in future AUTOSAR Adaptive standard releases.


****************
Dynamic Behavior
****************

Consumer Functionality
======================

- When the PTP frame arrives over Ethernet, its packet length is first checked to ensure it matches the expected length.
- If the preliminary checks are successful, the time information will be processed.
- Since the Adaptive Applications need this time information, they must be forwarded to the Adaptive TSYNC stack.
- ``tsync-ptp-lib`` acts as the interface between ``score-ptp-daemon`` and the Adaptive Application.

.. uml:: diagrams/consumer_reception.puml

Provider Functionality
======================

- When acting as the Provider, the ``score-ptp-daemon`` has to transmit the messages over Ethernet periodically.
- In this case, the time information comes from the Adaptive Applications.
- Hence, the daemon uses the interfaces provided by ``tsync-ptp-lib`` to fetch the time from the Adaptive Application.

.. uml:: diagrams/provider_periodic_transmission.puml

.. uml:: diagrams/provider_immediate_transmission.puml
