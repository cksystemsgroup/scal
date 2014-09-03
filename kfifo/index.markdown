---
layout: default
title: k-FIFO Queue
---


<table>
<tr>
<td valign="top" width="480">

<h1>k-FIFO Queue</h1>

<p align="justify"> 

We introduce fast and scalable algorithms that implement bounded- and
unbounded-size lock-free k-FIFO queues on parallel, shared memory hardware.
Logically, a k-FIFO queue can be understood as queue where elements may be
dequeued out-of-order up to k-1, or as pool where the oldest element is
dequeued within at most k dequeue operations.  The presented algorithms
enable up to k enqueue and k dequeue operations to be performed in
parallel. Moreover, we demonstrate a prototypical controller which
identifies optimal k automatically at runtime achieving better performance
than with any statically configured k. Unlike previous designs, however,
the algorithms also implement linearizable emptiness (and full) checks
without impairing scalability.  In experiments our algorithms outperform
and outscale all state-of-the-art concurrent pool and queue algorithms
other than the distributed queues in all micro- and most macrobenchmarks.
More information about the k-FIFO queues is available  in our <a
href="http://www.cs.uni-salzburg.at/~ck/content/publications/conferences/PaCT13-FastScalableQueues.pdf">PaCT13
paper</a>. The k-FIFO queue implementations are available as part of the <a
href="../">Scal</a> project.

</p>


</td>
<td>
<object type="image/svg+xml" data="kfifo.svg">
  Illustration of a k-FIFO queue
</object>
</td>
</tr>
<tr/>
<tr>
<td>

<object type="image/svg+xml" data="prodcon.svg">
  Performance of the k-FIFO queue in a low-contention producer-consumer benchmark
</object>

</td>
<td width="480" valign="top">

<h1>Performance</h1>

<p align="justify"> 

To the left you can see the performance and scalability of the k-FIFO
queue algorithms in a low-contention producer-consumer benchmark on a
40-core (2 hyperthreads per core) Intel Xeon server machine in comparison
with a lock-based queue (LB), a Michael-Scott queue (MS), a flat-combining
queue (FC), a wait-free queue (WF), a random-dequeue queue (RD), a segment
queue (SQ), an elimination-diffraction pool (ED), a lock-free linearizable
pool (BAG), a synchronous rendezvousing pool (RP), and various distributed
queue configurations (DQ). 

</p>

</td>
</tr>
</table>

<h1>People</h1>
<ul>
  <li><a href="http://cs.uni-salzburg.at/~ck/">Christoph M. Kirsch</a></li>
  <li><a href="//cs.uni-salzburg.at/~mlippautz">Michael Lippautz</a></li>
  <li><a href="//cs.uni-salzburg.at/~hpayer">Hannes Payer</a></li>
</ul>

