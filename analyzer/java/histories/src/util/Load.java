package util;

public class Load {
	public static double computePi(final int n) {
		
		if(n==0) {
			return 3;
		}
		
		int f = 1 - n;
		int ddFX = 1;
		int ddFY = -2 * n;
		int x = 0;
		int y = n;
		int in = 0;
		while (x < y) {
			if (f >= 0) {
				--y;
				ddFY += 2;
				f += ddFY;
			}
			x++;
			ddFX += 2;
			f += ddFX;
			in += (y - x);
		}
		return 8.0 * in / ((double) n * n);
	}
}
