package calculate;

import latenessbounds.StartTimeToDequeueBounds;
import util.Operation;

public class CalculateAgeWithStartTime {
	
	public static void main(String[] args) {
		Operation[] operations = Operation.readOperationsFromFile(args[0]);
		CalculateAge.calculateAge(operations, new StartTimeToDequeueBounds());
	}
}
