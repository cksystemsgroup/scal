package calculate;
import latenessbounds.StartTimeToDequeueBounds;
import util.Operation;

public class CalculateLatenessWithStartTime {

	public static void main(String[] args) {
		Operation[] operations = Operation.readOperationsFromFile(args[0]);
		CalculateLateness.calculateLateness(operations, 24, new StartTimeToDequeueBounds());
	}
}
