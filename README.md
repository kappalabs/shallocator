Shallocator - Shared Memory Allocator
=====================================

Introduction
------------
This project is meant to be an implementation of Buddy allocation technique. Then using the share() function or shwapoff(), the memory is distributed to hosts, which are defined in local config file.
One thread is working on allocation, another thread is maintaining the structure clean, meaning connecting freed space and setting it to zero.
One special thread is sharing the memory with hosts after share() and until unshare() is called.
