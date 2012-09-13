package calculate;

import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import java.util.Queue;
import java.util.Vector;
import java.util.concurrent.ConcurrentLinkedQueue;

import util.Operation;

public class CalculateOverlappings implements Runnable {

	public static void main(String[] args) {
		String filename = "rawdata.txt";
		Operation[] operations = Operation.readOperationsFromFile(filename);
		
		calculateOverlappings(operations);
	}


	
	/**
	 * Calculates the number of overlappings.
	 * @param operations
	 *            The array of operations, sorted by their starting time.
	 * @param noThreads
	 * 
	 * @return
	 */
	public static List<Operation>[] calculateOverlappings(Operation[] operations) {

		int noThreads = Runtime.getRuntime().availableProcessors();
		Arrays.sort(operations, getStartTimeComparator());

		Thread[] threads = new Thread[noThreads];
		@SuppressWarnings("unchecked")
		List<Operation>[] overlappings = new List[operations.length];

		for (int i = 0; i < operations.length; i++) {
			CalculateOverlappings.addTask(operations, overlappings, i);
		}

		for (int i = 0; i < noThreads; i++) {
			threads[i] = new Thread(new CalculateOverlappings());
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
		return overlappings;
	}



	private static Comparator<Operation> getStartTimeComparator() {
		return new Comparator<Operation>() {

			public int compare(Operation o1, Operation o2) {
				return Long.signum(o1.getStartTime() - o2.getStartTime());
			}
		};
	}

	static Queue<Task> tasks = new ConcurrentLinkedQueue<Task>();

	private static void addTask(Operation[] operations, List<Operation>[] overlappings, int resultIndex) {

		tasks.add(new Task(operations, overlappings, resultIndex));
	}

	public void run() {

		Task task = tasks.poll();

		if (task == null) {
			return;
		}

		int[] lowerBounds = doPrecalculation(task);

		while (task != null) {
			task.calculate(lowerBounds);
			task = tasks.poll();
		}
	}

	/**
	 * In each task we calculate the overlappings of one particular operation.
	 * The naive algorithm is to compare each operation with the given one and
	 * check if they are overlapping. For all operations this means quadratic
	 * complexity. Only the operations in a subinterval of the whole array of
	 * operations can overlap with the given operation. The operations are
	 * sorted by the starting time. The starting time defines the upper bound of
	 * the interval because we compare the starting time with the end time of
	 * the given operation. The upper bound can be found easily by a binary
	 * search. For the lower bound we precalculate an additional array. At index
	 * i it contains an index j such that operations[j] is the first operation
	 * in the array with its end time later than the operation operations[i].
	 * 
	 * @param task
	 *            The task provides the array of operations, it is the same for
	 *            all tasks.
	 * @return
	 */
	private int[] doPrecalculation(Task task) {

		int[] lowerBounds = new int[task.operations.length];
		int endTimeIndex = 0;
		long maxEndTime = task.operations[endTimeIndex].getEndTime();

		for (int i = 0; i < task.operations.length; i++) {
			Operation op = task.operations[i];

			// Assumption: For every start time there exists an end time which
			// occurs later
			while (op.getStartTime() >= maxEndTime) {
				endTimeIndex++;
				maxEndTime = task.operations[endTimeIndex].getEndTime();
			}

			lowerBounds[i] = endTimeIndex;
		}
		return lowerBounds;
	}

	static class Task {
		Operation[] operations;

		List<Operation>[] overlappings;
		int index;

		public Task(Operation[] operations, List<Operation>[] overlappings, int index) {
			super();
			this.operations = operations;
			this.overlappings = overlappings;
			this.index = index;
		}

		public void calculate(int[] lowerBounds) {
			Operation operation = operations[index];
			List<Operation> overlappingOperations = new Vector<Operation>(500);
			// Assumption: the operations Array is sorted by the start time of
			// the operation.

			// We need to find the index of the last operation whose start time
			// is earlier than the end time of the given operation.
			Operation temp = new Operation();
			temp.setStartTime(operation.getEndTime());

			int lastPossibleIndex = Arrays.binarySearch(operations, temp, new Comparator<Operation>() {
				public int compare(Operation o1, Operation o2) {
					return Long.signum(o1.getStartTime() - o2.getStartTime());
				}
			});

			lastPossibleIndex = Math.abs(lastPossibleIndex);
			lastPossibleIndex = Math.min(lastPossibleIndex, operations.length);

			int endIndex = lowerBounds[index];

			for (int i = lastPossibleIndex - 1; i >= endIndex; i--) {
				Operation op = operations[i];

				if (operation.getStartTime() < op.getEndTime() && operation.getEndTime() > op.getStartTime()) {
					if (operation != op) {
						overlappingOperations.add(op);
					}
				}
			}

			overlappings[operation.getDequeueIndex()] = overlappingOperations;
		}
	}
}
