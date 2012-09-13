package calculate;

import java.util.Arrays;
import java.util.Comparator;

import latenessbounds.LatenessBounds;

import util.Operation;

public class CalculateAge {
	
	public static int[] calculateAge(Operation[] operations, final LatenessBounds bounds) {
		
		Arrays.sort(operations, new Comparator<Operation>() {

			public int compare(Operation o1, Operation o2) {

				return Long.signum(bounds.getLowerBoundValue(o1) - bounds.getLowerBoundValue(o2));
			}
		});

		int[] lowerBoundOrder = new int[operations.length];
		
		for(int i = 0; i < lowerBoundOrder.length; i++) {
			lowerBoundOrder[operations[i].getDequeueIndex()] = i;
		}

		int[] ages = new int[lowerBoundOrder.length];
		
		int maxAge = 0;
		long totalAge = 0;
		long noAgedElements = 0;
		
		for(int i = 0; i < lowerBoundOrder.length; i++) {
			// the index i is the dequeue index of the element because the array is sorted.
			int age = i - lowerBoundOrder[i];
			ages[i] = age;
			maxAge = Math.max(maxAge, age);
			if(  age > 0) {
				totalAge+= i -lowerBoundOrder[i];
				noAgedElements++;
			}
		}
		
		long averageAge = totalAge / Math.max(1, noAgedElements);
		
		System.out.println(String.format("%d, %d, %d", maxAge, averageAge, noAgedElements));
		return ages;
	}
}
