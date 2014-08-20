---
layout: default
title: TS Stack
---

# Timestamped Stack

<table>
<tr>
<td valign="top" width="410">

The TS Stack is a linearizable concurrent stack implementation which uses
timestamps to order elements. A pop operations removes the element with the
youngest timestamp. Insights from the Stack Theorem (see below) allow the
TS Stack to timestamp some elements unordered timestamps such that these
elements can be removed in parallel. Thereby the contention on the stack is
reduced. Additionally timestamps allow highly efficient elimination, which
is key for high-performance concurrent stacks. The graph to the right shows
the performance of the TS Stack in a high-contention producer-consumer
benchmark on 64-core AMD Opteron cc-NUMA machine. The TS Stack
implementation is available as part of the <a href="../">Scal</a> project.

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
<td>
<br/>
<h1>Stack Theorem</h1>
<p>
The Stack Theorem says that an implementation is linarizable wrt. stack
semantics if the implementation is linearizable set (no elements leak,
correct emptiness check, ...) and it is order-correct (see the example to
the left). The Stack Theorem is proven in Isabelle HOL (<a
href="stackthm.tgz">sources</a>), Slides describing the Stack Theorem and
the TS Stack implementation are available <a
href="http://www.cs.uni-salzburg.at/~ahaas/slides/frida14.svg">here</a>.
</p>

<p>
In the example to the left element c is pushed on the stack after b, d is pushed
concurrently to both.  A stack implementation is order-correct if it orders
elements at least as strong as in the example (b and c are ordered, d is
incomparable wrt. c) and always returns one of the top elements, e.g.
either c or d in the example.
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

