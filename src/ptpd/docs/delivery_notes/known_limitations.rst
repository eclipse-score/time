#################
Known Limitations
#################

.. _known_lims_instspec:

Usage of pcap library
=====================

- S-CORE Time Synchronization functionality requires ``libpcap`` for transmiting messages over raw ethernet.
- Without ``libpcap``, the exchange of messages happens via UDP, on either IPv4 or IPv6, based on the option chosen
  while running ``ptpd``.

.. note::
   **Please note** that the UDP based use-case is currently not tested and its use is solely at the discretion of users.
