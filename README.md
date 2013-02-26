Introduction
============

This is a simplified implementation of TCP/IP protocol stack in user space. In
fact there are a group of students trying to do this, but we need a sample to
show that the idea is correct. So I'm trying to write this.

Motivation
==========

To learn TCP/IP protocol and programming in Linux environment.

Design
======

Design has a higher priority than coding.

It borrows a lot of ideas from the Linux kernel because it's implementated very
well.

Because programming and debugging in linux kernel is too complicated for me now,
the protocol will be implementation in user space. And it's on top of data link
packets. The interface is already there so that we don't need to write too
much code in kernel, and we have enough space to implement our network layer and
higher layers. But how the protocol stack and the L7 applications communicates
is not decided yet. For now I just wnat to write a library to be linked for
every normal user space application. With enough time, we'll try to write a
module so that normal applications don't need to be modified.

Bring the protocol stack out of kernel especially including the IP layer
usually exhibits poor performance.

Premature optmization won't be done until we know the whole protocol stack works.

And security is not of the first concern for now. Of course we'll try hard to do
our best.

References
==========

I have read a lot of books, thesis, web pages and source code about TCP/IP,
Linux kernel, programming in Linux environment and how to contribute to open
source communities. They'll be listed here in case I forgot some. :D

1) books
--------
    APUE
    UNP
    LDD (some chapters)
    <<TCP/IP illustrated Volume 1>>
    <<The Linux Networking Architecture>>
    <<Understanding Linux Network Internals>> (a little)
    
2) thesis
---------
    <<Implementing Network Protocols at User Level>>
    <<Experiences Implementing a high-performance TCP in user-space>>

3) code
-------
    Linux kernel
    FINS-Framework (some)
    uld
