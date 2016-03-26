import sun.misc.Unsafe;
import java.lang.reflect.*;

public class SQLiteJNIUnsafe { 
	public static native long select(int i);

	static final String getFixedString(Unsafe unsafe, long addr, int size) { 
		byte[] body = new byte[size];
		unsafe.copyMemory(null, addr, body, unsafe.arrayBaseOffset(byte[].class), size);		
		return new String(body);
	}

	public static void main(String[] args) throws Exception 
	{ 
		Constructor cc = Unsafe.class.getDeclaredConstructor();
		cc.setAccessible(true);
		Unsafe unsafe = (Unsafe)cc.newInstance();	    
		
		System.loadLibrary("sqlite-jni-unsafe");
		int nIterations = args.length > 0 ? Integer.parseInt(args[0]) : 1000000;
		int nAccounts = args.length > 1 ? Integer.parseInt(args[1]) : 1500000;
		long start = System.currentTimeMillis();
		int n = 0;
		for (int i = 0; i < nIterations; i++) { 
			int orderkey = (int)(Math.random()*nAccounts);
			long o = select(orderkey);
			if (o != 0) {
				Orders orders = new Orders(
					unsafe.getInt(o+0), 
					unsafe.getInt(o+4),
 				    unsafe.getByte(o+8),
					unsafe.getDouble(o+16),
					getFixedString(unsafe, o+24, 10),
					getFixedString(unsafe, o+34, 15),
					getFixedString(unsafe, o+49, 15),
					unsafe.getInt(o+64),
					getFixedString(unsafe, o+68, 79));
				n += 1;
			}
		}
		System.out.println("Elapsed time for selecting " + n + " objects: " + (System.currentTimeMillis() - start));
	}
}
