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
Insights from the stack theorem (see below) allow the TS Stack to have
multiple top elements, implemented with unordered timestamps, which can be
removed in parallel. Thereby the contention on the head of the stack is
reduced.  Additionally timestamps allow highly efficient elimination, which
is key for high-performance concurrent stacks. The graph to the right shows
the performance of the TS Stack in comparison with a Treiber Stack and an
elimination backoff stack (EB Stack) in a high-contention producer-consumer
benchmark on a 64-core AMD Opteron cc-NUMA machine. More information about
the TS stack is available in our <a
href="http://www.cs.uni-salzburg.at/~ck/content/publications/conferences/POPL15-TSStack.pdf">POPL15
paper</a>. The TS Stack is implemented as part of the <a
href="../">Scal</a> project.

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

Proving linearizability of the TS stack is difficult because existing proof
techniques turn out to be insufficient (e.g. the TS stack does not provide
linearization points in the code). We present a generic stack theorem which
uses partial order information provided by the code to prove
linearizability. The stack theorem is based on the following insight:

</p>

<p align="justify">

Linearizability requires that the order in which elements are pushed is
preserved. Concurrently pushed elements, however, are not ordered and thus
no order has to be preserved. We show in the stack theorem that a partial
order on elements, as illustrated to the left, is sufficient for a
linearizable stack implementation . If pop operations always remove one of
the top elements in the partial order, then the implementation is
linearizable with respect to stack semantics. The stack theorem is proven
in Isabelle HOL (<a href="stackthm.tgz">sources</a>). More details about
the stack theorem is available in our <a
href="http://www.cs.uni-salzburg.at/~ck/content/publications/conferences/POPL15-TSStack.pdf">POPL15
paper</a>.

</p>

<p align="justify">

In the TS Stack implementation we use the timestamps to encode the partial
order on elements. By searching and removing the element with the latest
timestamp we ensure that the TS Stack satisfies all conditions of the stack
theorem. Therefore the TS Stack is linearizable with respect to stack
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

