PTPd
===

PTP daemon (PTPd) is an implementation the Precision Time Protocol (PTP) version
2 as defined by 'IEEE Std 1588-2008'. PTP provides precise time coordination of
Ethernet LAN connected computers. It was designed primarily for instrumentation
and control systems.

Use
---

PTPd can coordinate the clocks of a group of LAN connected computers with each
other. It has been shown to achieve microsecond level coordination, even on
limited platforms.

The 'ptpd' program can be built from the included source code.  To use the
program, run 'ptpd' on a group of LAN connected computers. Compile with
'PTPD_DBG' defined and run with the '-C' or -V argument to watch what's going on.

If you are just looking for software to update the time on your desktop, you
probably want something that implements the Network Time Protocol. It can
coordinate computer clocks with an absolute time reference such as UTC.

Please refer to the
[INSTALL](https://raw.githubusercontent.com/ptpd/ptpd/master/INSTALL) file
for build instructions and configuration options. Please refer to the
[README.repocheckout](https://github.com/ptpd/ptpd/blob/master/README.repocheckout)
file for information on how to build from source code repositories.

Legal notice
---

PTPd was written by using only information contained within 'IEEE Std
1588-2008'. IEEE 1588 may contain patented technology, the use of which is not
under the control of the authors of PTPd. Users of IEEE 1588 may need to obtain
a license for the patented technology in the protocol. Contact the IEEE for
licensing information.

PTPd is licensed under a 2 Clause BSD Open Source License. Please refer to the
[COPYRIGHT](https://github.com/ptpd/ptpd/blob/master/COPYRIGHT) file for
additional information.

PTPd comes with absolutely no warranty.

Eclipse S-CORE modifications
---

Modifications Copyright (C) 2025 ETAS GmbH

### Build

#### Debian/Ubuntu/Raspbian

To build PTPd for Debian-based systems:

        autoreconf -vi
        ./configure --with-pcap-config
        make

#### QNX 7

To cross-compile PTPd for QNX:

        go_qnx.sh qnx_6i aarch64le
        export ac_cv_func_malloc_0_nonnull=yes
        ./configure --includedir=$QNX_TARGET/usr/include --libdir=$QNX_TARGET/usr/lib --host=${QNX_CONFIGURE_HOST} --prefix=/ --disable-snmp --enable-experimental-options --with-pcap-config=no
        make

The arguments are:


| Argument             | Description              |
| :------------------- | :----------------------- |
| --with-pcap-config=no | Used to tell where the system keeps libpcap pkg-config data. If pkg-config is not used, use the value 'no'. |
| --enable-experimental-options | On QNX systems, when experimental options are enabled, a clock_gettime approximation using CPU clock counter and attaching to IRQ0 is used, and this is also used to retrieve packet RX and TX timestamps, ignoring PCAP timestamps and socket options. This is recommended for best performance, but requires more testing before becoming the default. |

Hint: The argument '--disable-pcap' must NOT be given, because PCAP support is required for transport mode 'ethernet'.

### Usage

This chapter describes S-CORE specific compatibility options for AUTOSAR support.  
Available options to configure the PTPd behavior for AUTOSAR are:


| Argument             | Description              |
| :------------------- | :----------------------- |
| --score:autosar=<value\>                 | Enable AUTOSAR support (deactivate ANNOUNCE messages,...) |
| --score:globaltimepropagationdelay=<value\> | Static path propagation delay [sec] |

Note 1: the above parameters require the '--ptpengine:disable_bmca=y' option as well.  
Note 2: for compatibility with AUTOSAR also '--ptpengine:transport=ethernet' is required.

The easiest way to run PTPd in AUTOSAR mode is to use the pre-defined S-CORE AUTOSAR profile:

        ptpd2 -i enp0s3.1234 -V -t score-autosar

The S-CORE AUTOSAR profile uses the following settings:

        --ptpengine:domain=0
        --ptpengine:preset=slaveonly
        --ptpengine:transport=ethernet
        --ptpengine:delay_mechanism=DELAY_DISABLED
        --ptpengine:disable_bmca=y
        --score:autosar=y
        --score:globaltimepropagationdelay=0.0


In addition to the profile, you can add further configuration options. In case you specify a configuration option which is already defined by the profile, your option will overwrite the value given by the profile.  
In AUTOSAR mode the slave learns its time master from the first received SYNC message.

