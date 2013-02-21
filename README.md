Introduction
============

This is a trial version of simplified implementation of TCP/IP protocol stack in
user space. In fact we have a group of students trying to do this, but we need a
prototype to proof that the idea works. We need a architecture which we don't
have for now.

Design?
=======

Design is before the code.

1) main idea
------------
We will borrow a lot of ideas from the kernel source code because it's been
already implementated perfectly and has too much details. And implement them all
in 2 to 3 monthes is unrealistic for us students. But every piece of code will
be written by hand.

Because programming and debugging in linux kernel is too complicated for me now,
the protocol will be implementation in user space. And it will be on top of data
link packets. The interface is already there so that we don't need to write too
much code in kernel, and we have enough space to implement our network layer and
higher layers. But how the protocol stack and the L7 applications communicates
is not decided yet. For now I just wnat to write a library to be linked for
every normal user space application. If everything could be done easily and we
still have time then, we'll try to write the interface back into the kernel.
Of course this is not necessary for reasons of performance.

Bring the protocol stack out of kernel especially together with the IP layer
usually exhibits poor performance. But who cares.

Premature optmization won't be done until we know the whole protocol stack is
working.

And security is not of the first concern for now. Of course we'll try hard to do
our best.

2) details
----------
The skbuff struct, the list, the concept of net device, the interface functions
such as socket, connect, send and so on will be included in my simple design.

(a lot will be added here.)

In fact our implementation is absolutely a simplified version. Some of the
features specified in RFCs will be enriched after we confirm the whole frame
indeed works, but some of those will never be accomplished since they are much
too complicated.

I haven't figured out the details about this prototype, so I'll just write the
data structures and functions. Perhaps a lot of them will be modified. The
reason why I'm writing this is that I realized that I'm too lazy to do some real
work, I need to start now.

The design and code may be much better than what I can image now, but not
guaranteed.

TODO
====

List for advanced technicals may be useful
replace datalink packet with vfio
use netlink to rewrite the system calls

References
==========

Indeed I have read a lot books, thesis, web pages and source code about TCP/IP,
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

    <<Pro Git>> (half)
    
2) thesis
---------
    passed

3) code
-------
    Ｌinux kernel (some, of course)
    busybox (main frame and some code)
    FINS-Framework (some)
    lwip (some)