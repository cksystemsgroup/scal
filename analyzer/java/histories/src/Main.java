import java.text.DecimalFormat;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.BrokenBarrierException;
import java.util.concurrent.CyclicBarrier;

import scal.BoundedSizeKFifo;
import scal.ConcurrentLinkedQueue;
import scal.MyQueue;
import util.Load;
import util.Operation;
import util.ThreadLocalOperations;

public class Main implements Runnable {

	static MyQueue<Integer> queue;
	static CyclicBarrier barrier;
	static int[] firstAttemptOrder;

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub

		ThreadLocalOperations.noOperations = 10;
		int noThreads = 4;
		
		if (args.length >= 1) {
			noThreads = Integer.parseInt(args[0]);
		}
		if (args.length >= 2) {
			ThreadLocalOperations.noOperations = Integer.parseInt(args[1]);
		}
		
		int k = 0;
		
		if (args.length >= 3) {
			k = Integer.parseInt(args[2]);
		}
		
		int workload = 0;
		
		if (args.length >= 4) {
			workload = Integer.parseInt(args[3]);
		}
		
		if (k > 1) {
			System.out.println("Use k-FIFO");
			queue = new BoundedSizeKFifo<Integer>(k, (noThreads * ThreadLocalOperations.noOperations * 2) / k);
		} else {
			System.out.println("Use Michael-Scott");
			queue = new ConcurrentLinkedQueue<Integer>();
		}
		
		String filename = "rawdata.txt";
		
		if(args.length >= 5) {
			filename = args[4];
		}

		Operation[] operations = runExperiment(noThreads, workload);
		
		Operation.calculateDequeueOrder(operations, queue);

		Operation.writeOperationsToFile(filename, operations);
		
//		calculateAge(operations);
//
//		calculateStartupTime(operations);
//		
//		System.out.println("\nData gathered\n");
//
//		// Operation[] orderedOperations =
//		// createSequentialHistories(operations);
//		// // writeOperationsToFile("sequentialHistories.txt",
//		// orderedOperations);
//		//
//		// System.out.println("\nSequential histories generated\n");
//
//		calculateFirstEqSuccess(operations);
//		// int noEarlyElements = 0;
//
//		long[] timePerTry = new long[operations.length];
//		long averageTimePerTry = calculateTries(operations, timePerTry);
//
//		// Calculate earliness and lateness
//		// for (int i = 0; i < orderedOperations.length; i++) {
//		// Operation op = orderedOperations[i];
//		//
//		// if (op.getFirstAttempt() < i + 1) {
//		// noEarlyElements++;
//		// }
//		// }
//		// long maxEarliness = calculateMaximumEarliness(orderedOperations);
//		//
//		// long maxLateness = calculateMaximumLateness(orderedOperations);
//		//
//		// int megaLatenessCap = noThreads * 10;
//		// int latenessCap = noThreads;
//		// int noMegaLate = calculateNumberOfLateOperations(orderedOperations,
//		// megaLatenessCap);
//		// int noLate = calculateNumberOfLateOperations(orderedOperations,
//		// latenessCap);
//
//		System.out.print("\nCalculate overlappings: ");
//
//		// Assumption: the operations Array is sorted by the start time of the
//		// operation.
////		List<Operation>[] overlappings = CalculateOverlappings.calculateOverlappings(noThreads, operations);
////		assert (overlappings != null);
//
//		System.out.println("Done");
//
//		CalculateLateness.calculateLateness(operations, noThreads);
//		CalculateLatenessWithStartTime.calculateLateness(operations, noThreads);
//		CalculateLatenessStartToFirstAttempt.calculateLateness(operations, noThreads);
//
////		int maxOverlappingsOfShortOperations = calcMaxOverlappingsOfShortOperations(timePerTry, averageTimePerTry, overlappings);
////		int maxOverlappings = calculateMaxOverlappings(overlappings);
//
//		// System.out.println("Number of operations: " +
//		// orderedOperations.length);
//
//		// System.out.println("Number of early elements (firstAttempt Index > dequeue Index): "
//		// + noEarlyElements);
//		// System.out.println("Maximum earliness: " + maxEarliness);
//		// System.out.println("Maximum lateness: " + maxLateness);
//		// System.out.println("Number of megalate elements (lateness > " +
//		// megaLatenessCap + "): " + noMegaLate);
//		// System.out.println("Number of late elements (lateness > " +
//		// latenessCap + "): " + noLate);
////		System.out.println("Maximum number of overlappings: " + maxOverlappings);
////		System.out.println("Maximum number of overlappings of short operations: " + maxOverlappingsOfShortOperations);
//
//		System.out.println();
//
//		// for(Operation op : operations) {
//		// System.out.println(op);
//		// }
	}

	/**
	 * Does calculations about the startup time of an operation, i.e. the
	 * time from the start of the operation to its first attempt.
	 * @param operations The set of all operations.
	 */
	private static void calculateStartupTime(Operation[] operations) {

		System.out.print("\nCalculate startup time: ");
		
		long totalStartUpTime = 0;
		long maxStartUpTime = 0;
		long minStartUpTime = 0;
		double avgStartUpTime = 0;
				
		for(Operation op : operations) {
			long startUpTime = op.getFirstAttempt() - op.getStartTime();
			
			totalStartUpTime += startUpTime;
			maxStartUpTime = Math.max(maxStartUpTime, startUpTime);
			minStartUpTime = Math.min(minStartUpTime, startUpTime);
		}
		
		avgStartUpTime = ((double)totalStartUpTime) / ((double)operations.length);
		
		int counter = 0;
		totalStartUpTime = 0;
		
		// do not count all operations which are likely to be descheduled during startup time
		for(Operation op : operations) {
			long startUpTime = op.getFirstAttempt() - op.getStartTime();
			if(startUpTime < 10 * avgStartUpTime) {
				totalStartUpTime += startUpTime;
			}
			else {
				counter++;
			}
		}
		
		double cleanAverage = ((double)totalStartUpTime) / ((double)(operations.length - counter));
		
		double stdDev = 0;
		for(Operation op : operations) {
			long startUpTime = op.getFirstAttempt() - op.getStartTime();
			if(startUpTime < 10 * avgStartUpTime) {
			double dev = avgStartUpTime - startUpTime;
			stdDev += dev * dev;
			}
		}
		
		stdDev = Math.sqrt(stdDev/(operations.length - counter));
		
		System.out.println("done");
		
		DecimalFormat format = new DecimalFormat("#.00");
		
		System.out.println("Minimum start up time: " + minStartUpTime);
		System.out.println("Maximum start up time: " + maxStartUpTime);
		System.out.println("Average start up time: " + format.format(avgStartUpTime));
		System.out.println("Clean average:         " + format.format(cleanAverage));
		System.out.println("Number of long start ups (>10*average): " + counter);
		System.out.println("Standard deviation start up time: " + format.format(stdDev));
	}



	private static void calculateFirstEqSuccess(Operation[] operations) {
		int noFirstEqSuccess = 0;
		for (int i = 0; i < operations.length; i++) {

			if (operations[i].getNoRetries() == 1) {
				noFirstEqSuccess++;
			}
		}
		System.out.println("first attempt == success: " + noFirstEqSuccess);
	}

	private static Operation[] runExperiment(int noThreads, int workload) {
		barrier = new CyclicBarrier(noThreads);

		Thread[] threads = new Thread[noThreads];
		Main[] threadData = new Main[noThreads];
		Operation[] operations = new Operation[ThreadLocalOperations.noOperations * noThreads];

		for (int i = 0; i < noThreads; i++) {
			threadData[i] = new Main(i, operations, workload);
			threads[i] = new Thread(threadData[i]);
			threads[i].start();
		}

		for (int i = 0; i < noThreads; i++) {
			try {
				threads[i].join();
			} catch (InterruptedException e) {
				e.printStackTrace();
				return null;
			}
		}

		System.out.println("\nExperiments finished\n");

		long maxExecutionTime = getExecutionTime(threadData);
		System.out.println("Execution time: " + maxExecutionTime);
		System.out.println("Number of elements: " +  (noThreads * ThreadLocalOperations.noOperations));
		return operations;
	}



	private static int calculateMaxOverlappings(List<Operation>[] overlappings) {
		int maxOverlappings = 0;
		for (int i = 0; i < overlappings.length; i++) {
			maxOverlappings = Math.max(maxOverlappings, overlappings[i].size());
		}
		return maxOverlappings;
	}

	private static long calculateTries(Operation[] operations, long[] timePerTry) {
		long totalTimeOfAllOperations = 0;
		long totalNumberOfTries = 0;
		long maxAverageTimePerTry = 0;

		// Calculate the number of tries and the time per try
		for (int i = 0; i < operations.length; i++) {

			totalTimeOfAllOperations += operations[i].getEndTime() - operations[i].getStartTime();
			totalNumberOfTries += operations[i].getNoRetries();

			long averageTime  = (operations[i].getEndTime() - operations[i].getStartTime()) / operations[i].getNoRetries();
			
			timePerTry[operations[i].getDequeueIndex()] = averageTime;
			maxAverageTimePerTry = Math.max(maxAverageTimePerTry, averageTime);
		}

		long averageTimePerTry = totalTimeOfAllOperations / totalNumberOfTries;
		long averageNumberOfTries = totalNumberOfTries / operations.length;

		int noOperationsWithLongTries = 0;

		int averageNoTriesOfLongOperations = 0;

		// Calculate the number and execution times of long tries
		for (int i = 0; i < operations.length; i++) {

			if (timePerTry[i] > 10 * averageTimePerTry) {
				averageNoTriesOfLongOperations += operations[i].getNoRetries();
				noOperationsWithLongTries++;
			}
		}

		averageNoTriesOfLongOperations /= Math.max(1, noOperationsWithLongTries);

		System.out.println("Average time per try: " + averageTimePerTry);
		System.out.println("Maximum time per try: " + maxAverageTimePerTry);
		System.out.println("Number of operations with tries longer than 4 * average try time: " + noOperationsWithLongTries);
		System.out.println("Average number of tries: " + averageNumberOfTries);
		System.out.println("Average number of tries of long operations: " + averageNoTriesOfLongOperations);
		return averageTimePerTry;
	}

	private static long getExecutionTime(Main[] threadData) {
		long result = 0;
		for (int i = 0; i < threadData.length; i++) {

			result = Math.max(result, threadData[i].executionTime);
		}
		return result;
	}

	private static int calcMaxOverlappingsOfShortOperations(long[] timePerTry, long averageTimePerTry, List<Operation>[] overlappings) {

		int result = 0;
		for (int i = 0; i < overlappings.length; i++) {
			if (timePerTry[i] < 4 * averageTimePerTry) {
				result = Math.max(result, overlappings[i].size());
			}
		}
		return result;
	}

	private static int calculateNumberOfLateOperations(Operation[] orderedOperations, int megaLatenessCap) {

		int result = 0;
		for (int i = 0; i < orderedOperations.length; i++) {
			Operation op = orderedOperations[i];

			if (i - op.getFirstAttempt() > megaLatenessCap) {
				result++;
			}
		}
		return result;
	}

	private static long calculateMaximumLateness(Operation[] orderedOperations) {
		long result = 0;
		for (int i = 0; i < orderedOperations.length; i++) {
			Operation op = orderedOperations[i];

			result = Math.max(result, (i + 1) - op.getFirstAttempt());
		}
		return result;
	}

	private static long calculateMaximumEarliness(Operation[] orderedOperations) {
		long result = 0;
		for (int i = 0; i < orderedOperations.length; i++) {
			Operation op = orderedOperations[i];

			result = Math.max(result, op.getFirstAttempt() - (i + 1));
		}
		return result;
	}

	private static Operation[] createSequentialHistories(Operation[] operations) {
		Operation[] orderedOperations = new Operation[operations.length];

		int[] dequeueOrder = new int[operations.length];
		// The first id of an element is ThreadLocalOperations.noOperations, not
		// 0. We reduce the element id by this offset to
		// get a proper array index.
		int offset = ThreadLocalOperations.noOperations;

		// Get the real order from the queue
		for (int i = 0; i < orderedOperations.length; i++) {
			int element = queue.dequeue();
			dequeueOrder[element - offset] = i + 1;
			orderedOperations[i] = new Operation();
			orderedOperations[i].setElement(element);
		}

		Arrays.sort(operations, new Comparator<Operation>() {

			public int compare(Operation o1, Operation o2) {

				return Long.signum(o1.getEndTime() - o2.getEndTime());
			}
		});

		for (int i = 0; i < operations.length; i++) {
			orderedOperations[i].setEndTime(getDequeueIndex(operations, dequeueOrder, offset, i));
		}

		Arrays.sort(operations, new Comparator<Operation>() {

			public int compare(Operation o1, Operation o2) {
				return Long.signum(o1.getFirstAttempt() - o2.getFirstAttempt());
			}
		});

		for (int i = 0; i < operations.length; i++) {
			orderedOperations[i].setFirstAttempt(getDequeueIndex(operations, dequeueOrder, offset, i));
		}

		Arrays.sort(operations, new Comparator<Operation>() {

			public int compare(Operation o1, Operation o2) {
				return Long.signum(o1.getSuccessTime() - o2.getSuccessTime());
			}
		});

		for (int i = 0; i < operations.length; i++) {
			orderedOperations[i].setSuccessTime(getDequeueIndex(operations, dequeueOrder, offset, i));
		}

		// sort the operations according to their start time
		Arrays.sort(operations, new Comparator<Operation>() {

			public int compare(Operation o1, Operation o2) {
				return Long.signum(o1.getStartTime() - o2.getStartTime());
			}
		});

		// write the element IDs to the sequential history array
		for (int i = 0; i < operations.length; i++) {
			orderedOperations[i].setStartTime(getDequeueIndex(operations, dequeueOrder, offset, i));
		}

		// later code expects that the operations in the operations array are
		// sorted by the start time.
		return orderedOperations;
	}

	private static int getDequeueIndex(Operation[] operations, int[] dequeueOrder, int offset, int i) {
		return dequeueOrder[operations[i].getElement() - offset];
	}



	int threadID;
	int counter;
	Operation[] operations;
	long executionTime;
	int workload;

	public Main(int threadID, Operation[] operations, int workload) {
		this.threadID = threadID;
		this.operations = operations;
		this.workload = workload;

		counter = (threadID + 1) * ThreadLocalOperations.noOperations;
	}

	public void run() {

		barrierWait();
		long startTime = System.nanoTime();
		for (int i = 0; i < ThreadLocalOperations.noOperations; i++) {
			Load.computePi(workload);
			queue.enqueue(counter++);
		}
		executionTime = System.nanoTime() - startTime;
		System.arraycopy(ThreadLocalOperations.current().getOperations(), 0, operations, threadID * ThreadLocalOperations.noOperations, ThreadLocalOperations.noOperations);
		// operations = ThreadLocalOperations.current().getOperations();
	}

	private void barrierWait() {
		try {
			barrier.await();
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (BrokenBarrierException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

}
