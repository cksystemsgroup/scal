package latenessbounds;

import calculate.CalculateLateness;
import util.Operation;

public class SuccessTimeToDequeueBounds implements LatenessBounds {

	public static void main(String[] args) {
		Operation[] operations = Operation.readOperationsFromFile(args[0]);
		CalculateLateness.calculateLateness(operations, 24, new SuccessTimeToDequeueBounds());
	}
	
	@Override
	public long getLowerBoundValue(Operation operation) {
		return operation.getSuccessTime();
	}

	@Override
	public long getUpperBoundValue(Operation operation) {
		return operation.getDequeueIndex();
	}

}
