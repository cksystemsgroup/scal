package calculate;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;

import util.Operation;

public class CalculateLatenessStartToFirstAttempt implements Runnable {

	public static int[] age;

	public static int[] calculateLateness(Operation[] operations, int noThreads) {

		System.out.print("\nStart calculate lateness from start time to first attempt: ");

		// Create the result array. The dequeue index is used to link the
		// lateness to the operation.
		int[] lateness = new int[operations.length];

		Arrays.sort(operations, new Comparator<Operation>() {

			public int compare(Operation o1, Operation o2) {

				return Long.signum(o1.getStartTime() - o2.getStartTime());
			}
		});
		// TODO Auto-generated method stub
		Thread[] threads = new Thread[noThreads];

		for (int i = 0; i < operations.length; i++) {
			CalculateLatenessStartToFirstAttempt.addTask(operations[i], i);
		}

		CalculateLatenessStartToFirstAttempt[] threadData = new CalculateLatenessStartToFirstAttempt[noThreads];
		for (int i = 0; i < noThreads; i++) {
			threadData[i] = new CalculateLatenessStartToFirstAttempt(noThreads, operations, lateness);
			
			threads[i] = new Thread(threadData[i]);
			threads[i].start();
		}

		for (int i = 0; i < noThreads; i++) {
			try {
				threads[i].join();
			} catch (InterruptedException e) {
				e.printStackTrace();
				throw new RuntimeException(e);
			}
		}

		int totalLateness = 0;
		int noLateOperations = 0;
		int maxLateness = 0;
		int noMegaLateOperations = 0;

		for (int late : lateness) {
			totalLateness += late;
			if (late > 0) {
				noLateOperations++;
			}
			maxLateness = Math.max(maxLateness, late);

			if (late > noThreads) {
				noMegaLateOperations++;
			}
		}

		// for (CalculateLateness data : threadData) {
		// totalLateness += data.totalLateness;
		// noLateOperations += data.noLateOperations;
		// maxLateness = Math.max(maxLateness, data.maxLateness);
		// noMegaLateOperations += data.noMegaLateOperations;
		// noEarly += data.noEarly;
		// }

		int averageLateness = totalLateness / Math.max(1, noLateOperations);

		System.out.println("Done");
		System.out.println("average lateness: " + averageLateness);
		System.out.println("number of late ops: " + noLateOperations);
		System.out.println("number of mega late ops (>#threads): " + noMegaLateOperations);
		System.out.println("maximum lateness: " + maxLateness);

		return lateness;
	}

	static Queue<Task> tasks = new ConcurrentLinkedQueue<Task>();

	static void addTask(Operation operation, int index) {

		tasks.add(new Task(operation));
	}

	public int totalLateness = 0;
	public int noLateOperations = 0;
	public int maxLateness = 0;
	public int noMegaLateOperations = 0;
	public int noEarly = 0;

	int latenessCap;
	Operation[] operations;
	int[] lateness;

	public CalculateLatenessStartToFirstAttempt(int latenessCap, Operation[] operations, int[] lateness) {
		this.latenessCap = latenessCap;
		this.operations = operations;
		this.lateness = lateness;
	}

	@Override
	public void run() {
		// Given an operation, we look for the number of operations which start
		// later than the operation but are in the dequeue list before the given
		// operation.

		// Algorithm:
		// Sort the list of operation according to the dequeue order. Start
		// iterating through the list backwards until and count all operations
		// which started later
		// than the given operation. We can stop iterating if it is guaranteed
		// that all operations earlier in the list also start earlier than the
		// given operation.

		long[] upperBounds = doPrecalculation();

		for (Task task = tasks.poll(); task != null; task = tasks.poll()) {
			int result = task.calculate(operations, upperBounds);

			assert (task.operation.getDequeueIndex() < operations.length);
			lateness[task.operation.getDequeueIndex()] = result;

			// totalLateness += result;
			//
			// if (result > 0) {
			// noLateOperations++;
			// }
			//
			// maxLateness = Math.max(maxLateness, result);
			//
			// if (result > latenessCap) {
			// noMegaLateOperations++;
			// }
			//
			// if (result == 0) {
			// noEarly++;
			// }
		}
	}

	private long[] doPrecalculation() {

		long[] upperBounds = new long[operations.length];
		int lowestFirstAttemptIndex = operations.length - 1;
		long lowestFirstAttempt = operations[lowestFirstAttemptIndex].getFirstAttempt();

		for (int i = operations.length - 1; i >= 0; i--) {
			Operation op = operations[i];

			if (op.getFirstAttempt() < lowestFirstAttempt) {
				lowestFirstAttempt = op.getFirstAttempt();
				lowestFirstAttemptIndex = i;
			}

			upperBounds[i] = lowestFirstAttempt;
		}

		return upperBounds;

		// int[] lowerBounds = new int[operations.length];
		// int latestFirstAttemptIndex = 0;
		// long latestFirstAttempt =
		// operations[latestFirstAttemptIndex].getFirstAttempt();
		//
		// for (int i = 0; i < operations.length; i++) {
		// Operation op = operations[i];
		//
		// if(latestFirstAttempt > op.getFirstAttempt()) {
		// lowerBounds[i] = latestFirstAttemptIndex;
		// }
		// else {
		// lowerBounds[i] = i;
		// latestFirstAttemptIndex = i;
		// }
		// }
		// return lowerBounds;
	}

	static class Task {

		Operation operation;

		public Task(Operation operation) {
			super();
			this.operation = operation;
		}

		int calculate(Operation[] operations, long[] upperBounds) {

			int lateness = 0;

			int operationIndex = Arrays.binarySearch(operations, operation, new Comparator<Operation>() {
				public int compare(Operation o1, Operation o2) {
					return Long.signum(o1.getStartTime() - o2.getStartTime());
				}
			});

			// check if the operation really is in the array.
			assert (operationIndex >= 0);

			long opFirstAttempt = operation.getFirstAttempt();

			for (int i = operationIndex + 1; i < operations.length && opFirstAttempt > upperBounds[i]; i++) {
				Operation op = operations[i];

				if (operation.getFirstAttempt() > op.getFirstAttempt()) {
					
					lateness++;
				}
			}

			return lateness;
		}
	}
}
