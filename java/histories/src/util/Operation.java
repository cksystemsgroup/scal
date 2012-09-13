package util;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.PrintStream;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.LinkedList;
import java.util.List;
import java.util.Scanner;

import scal.ConcurrentLinkedQueue;
import scal.MyQueue;

public class Operation implements Serializable {

	static enum OpType {
		enqueue, dequeue
	}

	private static final long serialVersionUID = -2760671149078229787L;
	long startTime;
	long firstAttempt;
	long successTime;
	long endTime;
	int element;
	int noRetries;
	int dequeueIndex;
	OpType type = OpType.enqueue;

	public Operation() {
	}

	public Operation(long startTime, long firstAttempt, long successTime, long endTime, int element, int noRetries) {
		super();
		this.startTime = startTime;
		this.firstAttempt = firstAttempt;
		this.successTime = successTime;
		this.endTime = endTime;
		// this.type = type;
		this.element = element;
		this.noRetries = noRetries;
	}

	public Operation(long startTime, long firstAttempt, long successTime, long endTime, int element, int noRetries, int dequeueIndex) {
		super();
		this.startTime = startTime;
		this.firstAttempt = firstAttempt;
		this.successTime = successTime;
		this.endTime = endTime;
		this.element = element;
		this.noRetries = noRetries;
		this.dequeueIndex = dequeueIndex;
	}

	public String toString() {
		return String.format("%1$#10s, %2$#10s, %3$#10s, %4$#10s, %5$#10s, %6$#10s, %7$#10s", startTime, firstAttempt, successTime, endTime, element, noRetries, dequeueIndex);

	}

	public OpType getType() {
		return type;
	}

	public void setType(OpType type) {
		this.type = type;
	}

	public int getNoRetries() {
		return noRetries;
	}

	public void setNoRetries(int noRetries) {
		this.noRetries = noRetries;
	}

	public long getStartTime() {
		return startTime;
	}

	public long getEndTime() {
		return endTime;
	}

	public long getFirstAttempt() {
		return firstAttempt;
	}

	public int getElement() {
		return element;
	}

	public int getDequeueIndex() {
		return dequeueIndex;
	}

	public void setStartTime(long startTime) {
		this.startTime = startTime;
	}

	public void setEndTime(long endTime) {
		this.endTime = endTime;
	}

	public void setFirstAttempt(long firstAttempt) {
		this.firstAttempt = firstAttempt;
	}

	public void setElement(int element) {
		this.element = element;
	}

	public long getSuccessTime() {
		return successTime;
	}

	public void setSuccessTime(long successTime) {
		this.successTime = successTime;
	}

	public void setDequeueIndex(int dequeueIndex) {
		this.dequeueIndex = dequeueIndex;
	}

	public static Operation[] readOperationsFromFile(String filename) {
		List<Operation> operationList = new LinkedList<Operation>();
		List<Operation> dequeueList = new ArrayList<Operation>();

		MyQueue<Integer> dequeueQueue = new ConcurrentLinkedQueue<Integer>();
//		int maxDequeueIndex = -1;
		try {
			Scanner scanner = new Scanner(new File(filename));

			while (scanner.hasNextLine()) {
				String line = scanner.nextLine();
//				if (line.contains(",")) {

					Operation newOp = Operation.parseOperation(line);
					if (newOp.type == OpType.enqueue) {
						operationList.add(newOp);
					} else {
						dequeueList.add(newOp);
					}
//				} else {
//					if (line.contains(";")) {
//						String[] parts = line.split(";");
//						assert (parts.length == 2);
//						int dequeueIndex = Integer.parseInt(parts[0]);
//						assert (maxDequeueIndex < dequeueIndex);
//						maxDequeueIndex = dequeueIndex;
//						dequeueQueue.enqueue(Integer.parseInt(parts[1]));
//					} else {
//						dequeueQueue.enqueue(Integer.parseInt(line));
//					}
//				}
			}
			scanner.close();
		} catch (Exception e) {
			e.printStackTrace();
		}

		if (dequeueQueue.size() == 0 && dequeueList.size() > 0) {
			Collections.sort(dequeueList, getStartTimeComparator());
			long lastStartTime = 0;
			for (Operation op : dequeueList) {
				assert (lastStartTime < op.getStartTime());
				lastStartTime = op.getStartTime();
				dequeueQueue.enqueue(op.getElement());
			}
		}

		if(operationList.size() == 0) {
			System.err.println("Invalid file: " + filename);
			System.exit(0);
		}
		
		Operation[] operations = new Operation[operationList.size()];
		operationList.toArray(operations);
		// If the dequeues are given separately in the file we add the dequeue
		// order now.
		int dequeueQueueSize = dequeueQueue.size();

		// assert(operations.length == dequeueQueueSize);
		if (operations.length != dequeueQueueSize) {
			System.err.println("Some dequeues are missing.");
		}
		operations = calculateDequeueOrder(operations, dequeueQueue);
		
		return operations;
	}

	public static Operation[] calculateDequeueOrder(Operation[] operations, MyQueue<Integer> queue) {
		Operation[] result = new Operation[queue.size()];
		// System.out.print("\nCalculate dequeue order: ");
		Arrays.sort(operations, new Comparator<Operation>() {

			public int compare(Operation o1, Operation o2) {
				return o1.getElement() - o2.getElement();
			}
		});

		int i = 0;
		for (Integer element = queue.dequeue(); element != null; element = queue.dequeue()) {

			Operation key = new Operation();
			key.setElement(element);

			int index = Arrays.binarySearch(operations, key, new Comparator<Operation>() {

				public int compare(Operation o1, Operation o2) {
					return o1.getElement() - o2.getElement();
				}
			});

			result[i] = operations[index];

			assert (operations[index].getDequeueIndex() == 0);
			operations[index].setDequeueIndex(i++);
		}

		// System.out.println("done");
		return result;
	}

	public static void writeOperationsToFile(String filename, Operation[] operations) {
		File sequential = new File(filename);
		PrintStream fileWriter;
		try {
			fileWriter = new PrintStream(sequential);
		} catch (FileNotFoundException e) {
			e.printStackTrace();
			return;
		}

		for (Operation op : operations) {
			fileWriter.println(op);
		}
		fileWriter.close();
	}

	public static Operation parseOperation(String line) {

		if (line.startsWith("profile")) {
			
			String[] parts = line.split(" ");
			assert(parts.length == 6);
			
			Operation.OpType type = OpType.enqueue;
						
			if(parts[1].equals("ds_put")) {
				type = OpType.enqueue;
			} 
			else if(parts[1].equals("ds_get")) {
				type = OpType.dequeue;
			}
			else {
				assert(false);
			}
			
			long startTime = Long.parseLong(parts[3]);
			long endTime = Long.parseLong(parts[4]);
			int element = Integer.parseInt(parts[5]);
		
			Operation result = new Operation(startTime, 0, 0, endTime, element, 0);
			result.type = type;
			
			return result;
		} else {
			String[] parts = line.split(",");
			Operation result;
			// This is the output of the java queues at the moment
			if (parts.length == 7) {
				result = new Operation(Long.parseLong(parts[0].trim()), Long.parseLong(parts[1].trim()), Long.parseLong(parts[2].trim()), Long.parseLong(parts[3].trim()),
						Integer.parseInt(parts[4].trim()), Integer.parseInt(parts[5].trim()), Integer.parseInt(parts[6].trim()));
			}
			// The output of the c queues
			else if (parts.length == 4) {

				long startTime = Long.parseLong(parts[0].trim());
				result = new Operation(startTime, startTime, // no first attempt
																// time for the
																// C
																// queues at the
																// moment
						startTime, // no success time for the C queues at the
									// moment
						Long.parseLong(parts[1].trim()), Integer.parseInt(parts[3].trim()), 0, 0);

				if (parts[2].contains("0")) {
					result.type = OpType.enqueue;
				} else {
					result.type = OpType.dequeue;
				}
			} else {
				throw new IllegalArgumentException("The string '" + line + "' does not represent a valid operation.");
			}
			return result;
		}
	}

	public static Comparator<Operation> getStartTimeComparator() {
		return new Comparator<Operation>() {

			public int compare(Operation o1, Operation o2) {
				return Long.signum(o1.getStartTime() - o2.getStartTime());
			}
		};
	}
}
