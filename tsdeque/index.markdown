---
layout: default
title: Timestamped Deque
---

# Fast Concurrent Data-Structures Through Explicit Timestamping

**Abstract**

<p style="text-align:justify">
Concurrent data-structures, such as stacks, queues and deques, often implicitly
enforce a total order over elements with their underlying memory layout.
However, linearizability only requires that elements are ordered if the
inserting methods ran sequentially.  We propose a new data-structure design
which uses explicit timestamping to avoid unwanted ordering.  Elements can be
left unordered by associating them with unordered timestamps if their insert
operations ran concurrently.  In our approach, more concurrency translates into
less ordering, and thus less-contended removal and ultimately higher performance
and scalability.
</p>

<p style="text-align:justify">
As a proof of concept, we realise our approach in a non-blocking double-ended
queue. In experiments our deque outperforms and outscales the Michael-Scott
queue by a factor of 4.2 and the Treiber stack by a factor of 2.8. It even
outscales the elimination-backoff stack, the fastest concurrent stack of which
we are aware, and the flat-combining queue, a fast queue more scalable than
Michael-Scott.
</p>
