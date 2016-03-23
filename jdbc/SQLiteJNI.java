public class SQLiteJNI { 
	public static native Orders select(int i);

	public static void main(String[] args) throws Exception { 
		System.loadLibrary("sqlite-jni");
		int nIterations = args.length > 0 ? Integer.parseInt(args[0]) : 1000000;
		int nAccounts = args.length > 1 ? Integer.parseInt(args[1]) : 1500000;
		long start = System.currentTimeMillis();
		int n = 0;
		for (int i = 0; i < nIterations; i++) { 
			int orderkey = (int)(Math.random()*nAccounts);
			Orders o = select(orderkey);
			if (o != null) {
				n += 1;
			}
		}
		System.out.println("Elapsed time for selecting " + n + " objects: " + (System.currentTimeMillis() - start));
	}
}
