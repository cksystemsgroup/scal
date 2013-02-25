package calculate;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;

import latenessbounds.LatenessBounds;

import util.Operation;

public class CalculateLateness implements Runnable {

	public static int[] calculateLateness(Operation[] operations, int latenessCap, final LatenessBounds bounds) {
		
		int noThreads = Runtime.getRuntime().availableProcessors();

		// Create the result array. The dequeue index is used to link the
		// lateness to the operation.
		int[] lateness = new int[operations.length];

		Arrays.sort(operations, new Comparator<Operation>() {

			public int compare(Operation o1, Operation o2) {

				return Long.signum(bounds.getLowerBoundValue(o1) - bounds.getLowerBoundValue(o2));
			}
		});
		// TODO Auto-generated method stub
		Thread[] threads = new Thread[noThreads];

		for (int i = 0; i < operations.length; i++) {
			CalculateLateness.addTask(operations[i], i);
		}

		CalculateLateness[] threadData = new CalculateLateness[noThreads];
		for (int i = 0; i < noThreads; i++) {
			threadData[i] = new CalculateLateness(latenessCap, operations, lateness, bounds);
			
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

		long totalLateness = 0;
		long noLateOperations = 0;
		long maxLateness = 0;
//		int noMegaLateOperations = 0;

		for (int late : lateness) {
			totalLateness += late;
			if (late > 0) {
				noLateOperations++;
			}
			maxLateness = Math.max(maxLateness, late);

//			if (late > latenessCap) {
//				noMegaLateOperations++;
//			}
		}

		// for (CalculateLateness data : threadData) {
		// totalLateness += data.totalLateness;
		// noLateOperations += data.noLateOperations;
		// maxLateness = Math.max(maxLateness, data.maxLateness);
		// noMegaLateOperations += data.noMegaLateOperations;
		// noEarly += data.noEarly;
		// }

		double averageLateness = ((double)totalLateness) / ((double)Math.max(1, noLateOperations));
		double averageLatenessAllOps = ((double)totalLateness) / ((double)operations.length);

//		System.out.println("Done");
//		System.out.println("average lateness: " + averageLateness);
//		System.out.println("number of late ops: " + noLateOperations);
//		System.out.println("number of mega late ops (>#threads): " + noMegaLateOperations);
//		System.out.println("maximum lateness: " + maxLateness);
		System.out.println(String.format("%d %.3f %.3f %d", maxLateness, averageLateness, averageLatenessAllOps, noLateOperations));
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
	LatenessBounds bounds;

	public CalculateLateness(int latenessCap, Operation[] operations, int[] lateness, LatenessBounds bounds) {
		this.latenessCap = latenessCap;
		this.operations = operations;
		this.lateness = lateness;
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

	private long[] doPrecalculation(LatenessBounds bounds) {

		long[] upperBounds = new long[operations.length];
		int lowestIndex = operations.length - 1;
		long lowestValue = bounds.getUpperBoundValue(operations[lowestIndex]);

		for (int i = operations.length - 1; i >= 0; i--) {
			Operation op = operations[i];

			if (bounds.getUpperBoundValue(op) < lowestValue) {
				lowestValue = bounds.getUpperBoundValue(op);
				lowestIndex = i;
			}

			upperBounds[i] = lowestValue;
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

		int calculate(Operation[] operations, long[] upperBounds, final LatenessBounds bounds) {

			int lateness = 0;

			int operationIndex = Arrays.binarySearch(operations, operation, new Comparator<Operation>() {
				public int compare(Operation o1, Operation o2) {
					return Long.signum(bounds.getLowerBoundValue(o1) - bounds.getLowerBoundValue(o2));
				}
			});

			// check if the operation really is in the array.
			assert (operationIndex >= 0);

			long value = bounds.getUpperBoundValue(operation);

			for (int i = operationIndex + 1; i < operations.length && value > upperBounds[i]; i++) {
				Operation op = operations[i];

				if (bounds.getUpperBoundValue(operation) > bounds.getUpperBoundValue(op)) {
					
					lateness++;
				}
			}

			return lateness;
		}
	}
}
