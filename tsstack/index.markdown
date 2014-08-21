---
layout: default
title: TS Stack
---


<table>
<tr>
<td valign="top" width="480">

<h1>Timestamped Stack</h1>

<p align="justify"> 

The TS stack is a linearizable high-performance concurrent stack which
works by attaching timestamps to elements, rather than building a linked
data-structure in memory. The top element of the stack is then the element
with the latest timestamp, not the element at the head of a linked list.
Insights from the Stack Theorem (see below) allow the TS Stack to have
multiple top elements, implemented with unordered timestamps, which can be
removed in parallel. Thereby the contention on the head of the stack is
reduced.  Additionally timestamps allow highly efficient elimination, which
is key for high-performance concurrent stacks. The graph to the right shows
the performance of the TS Stack in comparison with a Treiber Stack and an
elimination backoff stack (EB Stack) in a high-contention producer-consumer
benchmark on a 64-core AMD Opteron cc-NUMA machine. The TS Stack
implementation is available as part of the <a href="../">Scal</a> project.

</p>

</td>
<td>
<object type="image/svg+xml" data="prodcon.svg">
  Performance of the TS Stack in a high-contention producer-consumer benchmark
</object>
</td>
</tr>
<tr/>
<tr>
<td>

<object type="image/svg+xml" data="order_correct.svg">
Illustration of order-correctness
</object>

</td>
<td width="480">

<h1>Stack Theorem</h1>

<p align="justify">

Linearizability requires that the order in which elements are pushed is
preserved. Concurrently pushed elements, however, are not ordered and
thus no order has to be preserved. Therefore a partial order on elements is
sufficient for a linearizable stack implementation, as illustrated to the
left. We show in the Stack Theorem that if pop operations always remove
one of the top elements in the partial order, then the implementation is
linearizable with respect to stack semantics. The Stack Theorem is proven
in Isabelle HOL (<a href="stackthm.tgz">sources</a>), slides describing the
Stack Theorem and the TS Stack implementation are available <a
href="http://www.cs.uni-salzburg.at/~ahaas/slides/frida14.svg">here</a>.

</p>

<p align="justify">

In the TS Stack implementation we use the timestamps to encode the partial
order on elements. By searching and removing the element with the latest
timestamp we ensure that the TS Stack satisfies all conditions of the Stack
Theorem. Therefore the TS Stack is linearizable with respect to stack
semantics.

</p>
</td>
</tr>
</table>

<h1>People</h1>
<ul>
  <li><a href="http://www-users.cs.york.ac.uk/~miked/">Mike Dodds</a></li>
  <li><a href="http://cs.uni-salzburg.at/~ahaas/">Andreas Haas</a></li>
  <li><a href="http://cs.uni-salzburg.at/~ck/">Christoph M. Kirsch</a></li>
</ul>

