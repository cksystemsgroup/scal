package scal;

import util.ThreadLocalOperations;
import util.ThreadLocalRandom;

import java.util.concurrent.atomic.AtomicReferenceArray;
import java.util.concurrent.atomic.AtomicStampedReference;

/**
 * A bounded-size thread-safe k-FIFO queue based on an array. The implementation
 * is lock-free.
 *
 * @param <E> the type of items that are stored in the FIFO queue
 */
public class BoundedSizeKFifo<E> implements MyQueue<E> {

    private static final int NO_INDEX_FOUND = -1;

    private int mQueueSize;
    private int mK;
    private AtomicReferenceArray<E> mQueue;
    private AtomicStampedReference<Integer> mHead =
            new AtomicStampedReference<Integer>(0, 0);
    private AtomicStampedReference<Integer> mTail =
            new AtomicStampedReference<Integer>(0, 0);

    /**
     * Constructor initializing the queue.
     *
     * @param k the bound for out-of-order retrievals
     * @param numSegments the number of k-segments in the queue
     * @throws IllegalArgumentException if one of the arguments is out of
     *                                  range
     */
    public BoundedSizeKFifo(final int k, final int numSegments) {
        if (k < 1) {
            throw new IllegalArgumentException("k needs to be >= 1");
        }
        if (numSegments < 2) {
            throw new IllegalArgumentException(
                    "atleast two segments are needed to build the queue");
        }
        if (((long) k * (long) numSegments) > Integer.MAX_VALUE) {
            throw new IllegalArgumentException(
                    "k * numSegments out of integer range");
        }
        mQueueSize = k * numSegments;
        mK = k;
        mQueue = new AtomicReferenceArray<E>(mQueueSize);
        for (int i = 0; i < mQueueSize; i++) {
            mQueue.set(i, null);
        }
    }

    private boolean inValidRegion(final int tailOldPointer,
            final int tailCurrentPointer, final int headCurrentPointer) {
        boolean wrapAround =
                (tailCurrentPointer < headCurrentPointer) ? true : false;
        if (!wrapAround) {
            return (headCurrentPointer < tailOldPointer
                    && tailOldPointer <= tailCurrentPointer) ? true : false;
        }
        return (headCurrentPointer < tailOldPointer
                || tailOldPointer <= tailCurrentPointer) ? true : false;
    }

    private boolean notInValidRegion(final int tailOldPointer,
            final int tailCurrentPointer, final int headCurrentPointer) {
        boolean wrapAround =
                (tailCurrentPointer < headCurrentPointer) ? true : false;
        if (!wrapAround) {
            return (tailOldPointer < tailCurrentPointer
                    || headCurrentPointer < tailOldPointer) ? true : false;
        }
        return (tailOldPointer < tailCurrentPointer
                && headCurrentPointer < tailOldPointer) ? true : false;
    }

    private boolean committed(final Integer tailOldPointer, final E newItem,
            final int itemIndex) {
        if (mQueue.get(itemIndex) != newItem) {
            return true;
        }

        Integer headCurrentPointer;
        int[] headCurrentCount = new int[1];
        headCurrentPointer = mHead.get(headCurrentCount);
        Integer tailCurrentPointer = mTail.getReference();

        if (inValidRegion(tailOldPointer, tailCurrentPointer,
                headCurrentPointer)) {
            return true;
        } else if (notInValidRegion(tailOldPointer, tailCurrentPointer,
                headCurrentPointer)) {
            if (!mQueue.compareAndSet(itemIndex, newItem, null)) {
                return true;
            }
        } else {
            if (mHead.compareAndSet(headCurrentPointer, headCurrentPointer,
                    headCurrentCount[0], headCurrentCount[0] + 1)) {
                return true;
            }
            if (!mQueue.compareAndSet(itemIndex, newItem, null)) {
                return true;
            }
        }
        return false;
    }

    private E findIndex(final int startPointer, final boolean empty,
            final int[] indexHolder) {
        int randomIndex = ThreadLocalRandom.current().nextInt(0, mK);
        int itemIndex;
        E oldItem;
        indexHolder[0] = NO_INDEX_FOUND;
        for (int i = 0; i < mK; i++) {
            itemIndex = (startPointer + ((randomIndex + i) % mK)) % mQueueSize;
            oldItem = mQueue.get(itemIndex);
            if ((empty && oldItem == null)
                    || (!empty && oldItem != null)) {
                indexHolder[0] = itemIndex;
                return oldItem;
            }
        }
        return null;
    }

    private boolean queueFull(final int headOldPointer,
            final int tailOldPointer) {
        Integer headCurrentPointer;
        int[] headCurrentCount = new int[1];
        headCurrentPointer = mHead.get(headCurrentCount);
        if ((headOldPointer == headCurrentPointer.intValue())
                && (((tailOldPointer + mK) % mQueueSize) == headOldPointer)) {
            return true;
        }
        return false;
    }

    private boolean segmentNotEmpty(final int headOldPointer) {
        for (int i = 0; i < mK; i++) {
            if (mQueue.get((headOldPointer + i) % mQueueSize) != null) {
                return true;
            }
        }
        return false;
    }

    private boolean advanceHead(final Integer headOldPointer,
            final int headOldCount) {
        return mHead.compareAndSet(headOldPointer,
                (headOldPointer + mK) % mQueueSize,
                headOldCount, headOldCount + 1);
    }

    private boolean advanceTail(final Integer tailOldPointer,
            final int tailOldCount) {
        return mTail.compareAndSet(tailOldPointer,
                (tailOldPointer + mK) % mQueueSize,
                tailOldCount, tailOldCount + 1);
    }
    
    /**
     * Enqueue an item into the queue.
     *
     * @param item the item that is enqueued
     * @return false if the queue is full, true otherwise
     */
    public final boolean enqueue(final E item) {
    	
    	long start = System.nanoTime();
        long firstAttempt = 0;
        long successTime = 0;
        int noTries = 0;
    	
//        E itemOld;
        int[] itemIndex = new int[1];
        Integer tailOldPointer;
        int[] tailOldCount = new int[1];
        Integer headOldPointer;
        int[] headOldCount = new int[1];


        
        while (true) {
        	noTries++;
        	
            tailOldPointer = mTail.get(tailOldCount);
            headOldPointer = mHead.get(headOldCount);
            findIndex(tailOldPointer, true, itemIndex);
            if ((tailOldPointer.intValue() == mTail.getReference().intValue())
                    && (tailOldCount[0] == mTail.getStamp())) {
                if (itemIndex[0] != NO_INDEX_FOUND) {
                	if(firstAttempt == 0) {
                		firstAttempt = System.nanoTime();
                	}
                    if (mQueue.compareAndSet(itemIndex[0], null, item)) {

                        if (committed(tailOldPointer, item, itemIndex[0])) {

                        	successTime = System.nanoTime();
                        	ThreadLocalOperations.current().addOperation(start, firstAttempt, successTime, System.nanoTime(), ((Integer)item).intValue(), noTries);
                            return true;
                        }
                    }
                } else {
                    if (queueFull(headOldPointer, tailOldPointer)) {
                        if (segmentNotEmpty(headOldPointer)) {
                            return false;
                        }
                        advanceHead(headOldPointer, headOldCount[0]);
                    }
                    advanceTail(tailOldPointer, tailOldCount[0]);
                }
            }

        }
    }

    /**
     * Dequeue an item from the queue.
     *
     * @return null if the queue is empty, the item otherwise
     */
    public final E dequeue() {
        E item;
        int[] itemIndex = new int[1];
        Integer headOldPointer;
        Integer tailOldPointer;
        int[] headOldCount = new int[1];
        int[] tailOldCount = new int[1];

        while (true) {
            headOldPointer = mHead.get(headOldCount);
            tailOldPointer = mTail.get(tailOldCount);
            item = findIndex(headOldPointer, false, itemIndex);
            if ((headOldPointer.intValue() == mHead.getReference().intValue())
                    && (headOldCount[0] == mHead.getStamp())) {
                if (itemIndex[0] != NO_INDEX_FOUND) {
                    if (headOldPointer.intValue()
                            == tailOldPointer.intValue()) {
                        advanceTail(tailOldPointer, tailOldCount[0]);
                    }
                    if (mQueue.compareAndSet(itemIndex[0], item, null)) {
                        return item;
                    }
                } else {
                    if (headOldPointer.intValue() == tailOldPointer.intValue()
                            && (tailOldPointer.intValue()
                            == mTail.getReference().intValue())) {
                        return null;
                    }
                    advanceHead(headOldPointer, headOldCount[0]);
                }
            }
        }
    }

	@Override
	public int size() {
		throw new RuntimeException("Not implemented");
	}
}
