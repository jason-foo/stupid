Introduction
============

STUPID means Simple TCP/IP User sPace Implementation Daemon :D

We are a group of students trying to accomplish it, but we need a prototype to
see whether our idea is correct. So I'm trying.

Motivation
==========

To learn TCP/IP protocol and programming in Linux environment.

Design
======

TCP/IP is complicated and hard to be implemented within 3 monthes, so I borrowed
a lot of ideas from Linux kernel and my version will discard some features.

Because programming and debugging in linux kernel is too complicated for me now,
the protocol will be implementation in user space.

It's on top of data link packets. The interface is easy to use so that we don't
need to write much in kernel.

Bring the protocol stack out of kernel especially including the IP layer
usually exhibits poor performance. And security is not of the first concern for
now. Of course we'll try hard to do it.

Architecture
============

Every packet is represented with a sk_buff structure. And we simulated a network
adapter.

We have to deal with raw packets from/to NIC, data from/to applications
and the main protocol processing part, so the whole daemon must be separated at least
3 threads. In fact, there are lot's of asynchronous events(e.g. arp, dynamical
routing, TCP connections handling), 3 threads are not enough. I'll try to
implement a notification chain to handle the events(simulating the softirq may
be too hard).

               |----------------------|   
dev <------->  | main protocol entiry |  <-------->  API  <-------> application
               |----------------------|
               | ethernet |  IP  | L4 |
                         arp  ^
			      |
                              |
                        static route

Setup and Run
=============

Setup your ip, mac, gateway and mask in dev.conf(mac and ip can be different
from your host, but don't changed it too frequently for other systems have arp
cache)

run src/stupid as root

run test/echo_udp/server as root on another computer. and these don't need to be
in the same LAN.

then run demo/udp_echo_client, server IP is hard coded in udp_echo_client.c, you
may want to change it. And when compiling udp_echo_client you need
api/libstupid.a compiled first.

References
==========

I have read a lot of books, thesis, web pages and source code about TCP/IP,
Linux kernel, programming in Linux environment and how to contribute to open
source communities.

1) books
--------
    <<Advanced Programming in UNIX Environment>>
    <<UNIX Network Programming>>
    <<Linux Device Drivers>>  (some chapters)
    <<TCP/IP illustrated Volume 1>>
    <<The Linux Networking Architecture>>
    <<Understanding Linux Network Internals>> (some chapters)
    
2) thesis
---------
    <<Implementing Network Protocols at User Level>>
    <<Experiences Implementing a high-performance TCP in user-space>>

3) code
-------
    Linux kernel
    FINS-Framework (some)
    uld (from Cisco System)
