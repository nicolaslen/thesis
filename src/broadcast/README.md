# tob
Total Order Broadcast implementation over Open MPI

This is a project assignement from the 2nd year Master Course ["Concurrent and Parallel Programming"](http://www.dis.uniroma1.it/~hpdcs/index.php?option=com_content&view=article&id=19) at DIAG / Sapienza University of Rome.

In this course principles for designing/developing parallel/distributed programs and evaluating their performance are provided. To this end, the course will first introduce high performance architectures, and will afterwards focus on methodologies for the development of parallel and distributed programs. MPI and PDES frameworks will be used to show how to develop parallel applications. At the end of the course the transactional programming will be introduced and it will be used to manage the concurrency in high performance platforms (centralized and distributed).

The examination also requires a project which isuniquely assigned and must be developed by single students (no grouping).

This projects is the implementation of the Total Order Broadcast abstraction over Open-MPI.
This also implies that it contains the implementation of the full stack to reach TOB from a a Perfect Point-to-Point Link,i.e., Perfect Failure Detector, Best Effort Broadcast, Eager Reliable Broadcast, Flooding Consensus and finally Consensus Based TOB.

Main.c contains several examples of usage
