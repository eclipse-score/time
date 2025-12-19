##############
Delivery Notes
##############

*************
1.3.7 release
*************

Minor
=====

- The ``syncPeriod`` for the Time Master is now sourced from the timesync configuration. The command-line support for this parameter has been deprecated.
- The configuration parameter ``followUpTimeoutValue`` is used to ensure the follow-up message is received within the specified time frame.
- The configuration parameter ``globalTimeSequenceCounterJumpWidth`` is used to detect the sequence-counter-jumps.
  If a detected jump in any sync message exceeds the configured threshold, the associated PTP messages are rejected.
- *Infrastructure:* Migrate to ctc-bin 3.0.0


*************
1.3.5 release
*************

Minor
=====

- The requirement to run aptpd2 as root has been removed. It can now be executed with non-root access.
- The `on` command is no longer necessary to launch aptpd2.


*************
1.3.1 release
*************

Defect Fix
==========

- Improved error handling and stability in the context of network device discovery (AZB #79872)


*************
1.3.1 release
*************

New Features
============
- Enabled the build for QNX8 SDP


*************
1.3.0 release
*************

New Features
============
- Enabled the System Clock update feature in the Consumer mode. The Clock will get updated with every successful reception
  of the sync/follow-up PTP message pair.

Defect Fix
==========
- Update the call to ``TSync_GetTimebaseConfiguration()`` API to accept the ptp-daemon role - Provider or Consumer (AZB #77926)


*************
1.2.7 release
*************

Minor
=====
- Update the detailed design and extend SW architecture documentation with sequence diagrams for PTP message
  transmission and reception


*************
1.2.5 release
*************

Minor
=====
- Update product documentation with PTP ``Sync`` and ``Followup`` message format layouts according to AUTOSAR
- Finalize SubTLV message handling after tests with different configuration permutations


*************
1.2.4 release
*************

Minor
=====
- All the SubTLVs except Status and OFS are now supported
- CRC support is introduced for the SubTLVs
- In Consumer mode, default handling is introduced for Status SubTLV


**********************
1.2.1 release (hotfix)
**********************

Minor
=====
- Fix the ptpd termination in provider mode


*************
1.2.0 release
*************

New Features
============
- Introduce message compliance for IEEE 802.1 AS & AUTOSAR extensions
- Introduce CRC package dependency into ptpd & sanity check of CRC APIs

Defect Fix
==========
- Enable conditional generation of configuration header based on underlying SDK and supported features (AZB 68716)


**************
R22.11 release
**************

Defect Fix
==========
- The population of Seconds, SecondsHi and Nanoseconds fields into PTP frame's PreciseOriginTime field, had alignment issues.
  Due to this, the time interpreted by the Consumer was incorrect. This issue is now fixed.

Minor
=====
- Cleanup with respect to API names and variable names


**************
R22.08 release
**************

New Features
============
- PTPd can now be executed in the Provider mode
- SubTLV support (Userdata) is now introduced. It is available for both Provider and Consumer modes

Minor
=====
- To distinguish modified PTPd implementation from the original, the binary is now renamed to ``aptpd``, where 'a' stands for 'adapted'
