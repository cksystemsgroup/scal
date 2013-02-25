package calculate;
import latenessbounds.StartTimeToDequeueBounds;
import util.Operation;

public class CalculateEarlinessWithStartTime {

	public static void main(String[] args) {
		Operation[] operations = Operation.readOperationsFromFile(args[0]);
		CalculateEarliness.calculateEarliness(operations, new StartTimeToDequeueBounds());
	}
}
