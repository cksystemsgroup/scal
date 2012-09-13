package scal;

public interface MyQueue<E> {

	boolean enqueue(E item);
	E dequeue();
	int size();
}
