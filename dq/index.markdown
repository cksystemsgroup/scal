---
layout: default
title: Distributed Queue
---


<table>
<tr>
<td valign="top" width="480">

<h1>Distributed Queue</h1>

<p align="justify"> 

A prominent remedy to multicore scalability issues in concurrent data
structure implementations is to relax the sequential specification of the
data structure. We present distributed queues (DQ), a new family of relaxed
concurrent queue implementations.  DQs implement relaxed queues with
linearizable emptiness check and either configurable or bounded
out-of-order behavior or pool behavior.  Our experiments show that DQs
outperform and outscale in micro- and macrobenchmarks all strict and
relaxed queue as well as pool implementations that we considered. 

</p>

<p align="justify"> 

To the
right you can see the performance and scalability of the DQs in a
high-contention producer-consumer benchmark on a 40-core (2 hyperthreads
per core) Intel Xeon server machine in comparison with a lock-based queue
(LB), a Michael-Scott queue (MS), a flat-combining queue (FC), a wait-free
queue (WF), a random-dequeue queue (RD), a segment queue (SQ), a
bounded-size k-FIFO queue (BS k-FIFO), an unbounded-size k-FIFO queue (US
k-FIFO), an elimination-diffraction pool (ED), a lock-free linearizable
pool (BAG), and a synchronous rendezvousing pool (RP). More information
about the DQs is available  in our <a
href="http://www.cs.uni-salzburg.at/~ck/content/publications/invited/CF13-DistributedQueues.pdf">CF13
paper</a>. The DQ implementations are available as part of the <a
href="../">Scal</a> project.

</p>

</td>
<td>
<object type="image/svg+xml" data="prodcon.svg">
  Performance of the DQs in a high-contention producer-consumer benchmark
</object>
</td>
</tr>
<tr/>
<tr>
<td>

<object type="image/svg+xml" data="dq.svg">
Illustration of a distributed queue
</object>

</td>
<td width="480">

<h1>DQ Design</h1>

<p align="justify">

A DQ consists of multiple FIFO queues, called partial queues, and a load
balancer. Upon an enqueue or dequeue operation one out of the p partial
queues is selected for performing the actual operation without any
further coordination with the other p−1 partial queues. Selection
is done by the load balancer.

</p>

<p align="justify">

In the experiment above we use three types of load balancers: The d-RA load
balancer randomly selects d ≥ 1 queues out of the p partial queues and then
returns (the index of) the queue with the least elements among the d queues
when called by an enqueue operation. Symmetrically, the load balancer
returns the queue with the most elements when called by a dequeue
operation. The b-RR load balancer maintains b ≥ 1 pairs of shared
round-robin counters that are associated with threads such that each thread
is permanently assigned to exactly one pair and all pairs have
approximately the same number of threads assigned. The key invariant
maintained by the LRU DQ is that the number of elements enqueued or
dequeued into a partial queue differs by at most one.


</p>
</td>
</tr>
</table>

<h1>People</h1>
<ul>
  <li><a href="//cs.uni-salzburg.at/~ahaas">Andreas Haas</a></li>
  <li><a href="http://pub.ist.ac.at/~tah/">Thomas A. Henzinger</a></li>
  <li><a href="//cs.uni-salzburg.at/~ck">Christoph Kirsch</a></li>
  <li><a href="//cs.uni-salzburg.at/~mlippautz">Michael Lippautz</a></li>
  <li><a href="//cs.uni-salzburg.at/~hpayer">Hannes Payer</a></li>
  <li><a href="http://www.cl.cam.ac.uk/~as2418/">Ali Sezgin</a></li>
  <li><a href="//cs.uni-salzburg.at/~anas">Ana Sokolova</a></li>
</ul>

