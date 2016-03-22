import java.sql.*;

class Orders {
    public int o_orderkey;
    public int o_custkey;
    public byte o_orderstatus;
    public double o_totalprice;
    public String o_orderdate;
    public String o_orderpriority;
    public String o_clerk;
    public int o_shippriority;
    public String o_comment;

	public Orders(int orderkey, int custkey, byte orderstatus, double totalprice, String orderdate, String orderpriority, String clerk, int shippriority, String comment) {
		 o_orderkey = orderkey;
		 o_custkey = custkey;
		 o_orderstatus = orderstatus;
		 o_totalprice = totalprice;
		 o_orderdate = orderdate;
		 o_orderpriority = orderpriority;
		 o_clerk = clerk;
		 o_shippriority = shippriority;
		 o_comment = comment;
	}
};

public class TestJDBC
{
  public static void main( String args[] ) throws Exception
  {
    Connection c = null;
	int nIterations = args.length > 0 ? Integer.parseInt(args[0]) : 1000000;
	int nAccounts = args.length > 1 ? Integer.parseInt(args[1]) : 1500000;

	Class.forName("org.sqlite.JDBC");
	c = DriverManager.getConnection("jdbc:sqlite:test.db");
	c.setAutoCommit(false);
	System.out.println("Opened database successfully");
	
	PreparedStatement stmt = c.prepareStatement("select * from orders where o_orderkey=?");
	long start = System.currentTimeMillis();
	for (int i = 0; i < nIterations; i++) { 
		int orderkey = (int)(Math.random()*nAccounts);
		stmt.setInt(1, orderkey);
		ResultSet rs = stmt.executeQuery();
		if (rs.next()) { 
			new Orders(rs.getInt("o_orderkey"), 
					   rs.getInt("o_custkey"),
					   rs.getByte("o_orderstatus"),
					   rs.getDouble("o_totalprice"),
					   rs.getString("o_orderdate"),
					   rs.getString("o_orderpriority"),
					   rs.getString("o_clerk"),
					   rs.getInt("o_shippriority"),
					   rs.getString("o_comment"));
		}
		rs.close();
	}
	System.out.println("Elapsed time: " + (System.currentTimeMillis() - start));
	stmt.close();
	c.close();
  }
}