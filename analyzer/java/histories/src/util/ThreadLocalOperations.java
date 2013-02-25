package util;

public class ThreadLocalOperations {

	public static int noOperations = 100000;

	/**
	 * The actual ThreadLocal
	 */
	private static final ThreadLocal<ThreadLocalOperations> localOperations = new ThreadLocal<ThreadLocalOperations>() {
		protected ThreadLocalOperations initialValue() {
			return new ThreadLocalOperations();
		}
	};

	/**
	 * Returns the current thread's {@code ThreadLocalOperations}.
	 * 
	 * @return the current thread's {@code ThreadLocalOperations}
	 */
	public static ThreadLocalOperations current() {
		return localOperations.get();
	}

	Operation[] operations = new Operation[noOperations];
	int next = 0;

	public void addOperation(long start, long firstAttempt, long successTime, long end, int element, int noRetries) {
		if (next < noOperations) {
			operations[next++] = new Operation(start, firstAttempt, successTime, end, element, noRetries);
		}
	}

	public Operation[] getOperations() {
		return operations;
	}

}
