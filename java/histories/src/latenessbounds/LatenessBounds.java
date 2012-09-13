package latenessbounds;
import util.Operation;


public interface LatenessBounds {

	long getLowerBoundValue(Operation operation);
	long getUpperBoundValue(Operation operation);
}
