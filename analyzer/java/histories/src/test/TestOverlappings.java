package test;

import java.util.List;

import calculate.CalculateOverlappings;

import util.Operation;


public class TestOverlappings {

	/**
	 * @param args
	 */
	public static void main(String[] args) {

		Operation[] oneOverAll = new Operation[] {
			
			new Operation(2,  6,  7,  3, 2, 1, 1),
			new Operation(4, 12, 15, 5, 3, 1, 2),
			new Operation(6, 13, 17, 7, 4, 1, 3),
			new Operation(1,  5, 10, 8, 1, 1, 4),
		};
		
		Operation[] noOverlappings = new Operation[] {
				new Operation(1,  5, 10, 2, 1, 1, 4),
				new Operation(3,  6,  7,  4, 2, 1, 1),
				new Operation(5, 12, 15, 6, 3, 1, 2),
				new Operation(7, 13, 17, 8, 4, 1, 3),
			};
		
		Operation[] allOverlap = new Operation[] {
				new Operation(1,  5, 10, 5, 1, 1, 4),
				new Operation(2,  6,  7, 6, 2, 1, 1),
				new Operation(3, 12, 15, 7, 3, 1, 2),
				new Operation(4, 13, 17, 8, 4, 1, 3),
			};
		
		List<Operation>[] result = CalculateOverlappings.calculateOverlappings(noOverlappings);
		printOverlappings(noOverlappings, result, "No Overlappings");
		for(List<Operation> overlappings : result) {
			assert(overlappings.size() == 0);
		}
		
		result = CalculateOverlappings.calculateOverlappings(allOverlap);
		printOverlappings(allOverlap, result, "All Overlap");
		for(List<Operation> overlappings : result) {
			assert(overlappings.size() == allOverlap.length - 1);
		}
		
		result = CalculateOverlappings.calculateOverlappings(oneOverAll);
		printOverlappings(oneOverAll, result, "One Over All");
		assert(result[0].size() == 1);
		assert(result[1].size() == 1);
		assert(result[2].size() == 1);
		assert(result[3].size() == 3);
	}

	private static void printOverlappings(Operation[] operations, List<Operation>[] result, String label) {
		
		System.out.println("\n+++++++++++++++++++++++" + label + "+++++++++++++++++++++++++++++++++++++");
		
		for(int i = 0; i < operations.length; i++) {
			System.out.println(operations[i]);
			for(Operation op : result[i]) {
				System.out.println("\t" + op);
			}
		}
	}

}
