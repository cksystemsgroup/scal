package calculate;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;

import latenessbounds.LatenessBounds;

import util.Operation;

public class CalculateEarliness implements Runnable {

	public static int[] calculateEarliness(Operation[] operations, final LatenessBounds bounds) {
		
		int noThreads = Runtime.getRuntime().availableProcessors();

		// Create the result array. The dequeue index is used to link the
		// earliness to the operation.
		int[] earliness = new int[operations.length];

		Arrays.sort(operations, new Comparator<Operation>() {

			public int compare(Operation o1, Operation o2) {

				return Long.signum(bounds.getUpperBoundValue(o1) - bounds.getUpperBoundValue(o2));
			}
		});
		// TODO Auto-generated method stub
		Thread[] threads = new Thread[noThreads];

		for (int i = 0; i < operations.length; i++) {
			CalculateEarliness.addTask(operations[i], i);
		}

		CalculateEarliness[] threadData = new CalculateEarliness[noThreads];
		for (int i = 0; i < noThreads; i++) {
			threadData[i] = new CalculateEarliness(operations, earliness, bounds);
			
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

		long totalEarliness = 0;
		long noEarlyOperations = 0;
		long maxEarliness = 0;
//		int noMegaLateOperations = 0;

		for (int early : earliness) {
			totalEarliness += early;
			if (early > 0) {
				noEarlyOperations++;
			}
			maxEarliness = Math.max(maxEarliness, early);

		}

		double averageEarliness = ((double)totalEarliness) / ((double)Math.max(1, noEarlyOperations));
		double averageEarlinessAllOps = ((double)totalEarliness) / ((double)operations.length);

//		System.out.println("Done");
//		System.out.println("average lateness: " + averageLateness);
//		System.out.println("number of late ops: " + noLateOperations);
//		System.out.println("number of mega late ops (>#threads): " + noMegaLateOperations);
//		System.out.println("maximum lateness: " + maxLateness);
		System.out.println(String.format("%d %.3f %.3f %d", maxEarliness, averageEarliness, averageEarlinessAllOps, noEarlyOperations));
		return earliness;
	}

	static Queue<Task> tasks = new ConcurrentLinkedQueue<Task>();

	static void addTask(Operation operation, int index) {

		tasks.add(new Task(operation));
	}

	Operation[] operations;
	int[] earliness;
	LatenessBounds bounds;

	public CalculateEarliness(Operation[] operations, int[] lateness, LatenessBounds bounds) {
		this.operations = operations;
		this.earliness = lateness;
		this.bounds = bounds;
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

		long[] upperBounds = doPrecalculation(bounds);

		for (Task task = tasks.poll(); task != null; task = tasks.poll()) {

			int result = task.calculate(operations, upperBounds, bounds);

			assert (task.operation.getDequeueIndex() < operations.length);
			earliness[task.operation.getDequeueIndex()] = result;

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

	private long[] doPrecalculation(LatenessBounds bounds) {

		long[] upperBounds = new long[operations.length];
		long lowestValue = bounds.getLowerBoundValue(operations[operations.length -1]);

		for (int i = operations.length - 1; i >= 0; i--) {
			Operation op = operations[i];

			if (bounds.getLowerBoundValue(op) < lowestValue) {
				lowestValue = bounds.getLowerBoundValue(op);
			}

			upperBounds[i] = lowestValue;
		}

		return upperBounds;

	}

	static class Task {

		Operation operation;

		public Task(Operation operation) {
			super();
			this.operation = operation;
		}

		int calculate(Operation[] operations, long[] upperBounds, final LatenessBounds bounds) {

			int earliness = 0;

			int operationIndex = Arrays.binarySearch(operations, operation, new Comparator<Operation>() {
				public int compare(Operation o1, Operation o2) {
					return Long.signum(bounds.getUpperBoundValue(o1) - bounds.getUpperBoundValue(o2));
				}
			});

			// check if the operation really is in the array.
			assert (operationIndex >= 0);

			long value = bounds.getLowerBoundValue(operation);

			for (int i = operationIndex + 1; i < operations.length && value > upperBounds[i]; i++) {
				Operation op = operations[i];

				if (bounds.getLowerBoundValue(operation) > bounds.getLowerBoundValue(op)) {
					
					earliness++;
				}
			}

			return earliness;
		}
	}
}
