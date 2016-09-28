import java.sql.*;

public class TestJDBC
{
  public static void main( String args[] ) throws Exception
  {
    Connection c = null;
	int nIterations = args.length > 0 ? Integer.parseInt(args[0]) : 1000000;
	int nAccounts = args.length > 1 ? Integer.parseInt(args[1]) : 1500000;

	Class.forName("org.sqlite.JDBC");
	c = DriverManager.getConnection("jdbc:sqlite:test.db");
	System.out.println("Opened database successfully");
	Statement s = c.createStatement();
	s.execute("PRAGMA cache_size = 20000");	
	s.execute("PRAGMA journal_mode=WAL");	
	s.close();
	c.setAutoCommit(false);
	PreparedStatement stmt = c.prepareStatement("select * from orders where o_orderkey=?");
	long start = System.currentTimeMillis();
	int n = 0;
	for (int i = 0; i < nIterations; i++) { 
		stmt.setInt(1, (int)(Math.random()*nAccounts));
		ResultSet rs = stmt.executeQuery();
		while (rs.next()) { 
			new Orders(rs.getInt("o_orderkey"), 
					   rs.getInt("o_custkey"),
					   rs.getByte("o_orderstatus"),
					   rs.getDouble("o_totalprice"),
					   rs.getString("o_orderdate"),
					   rs.getString("o_orderpriority"),
					   rs.getString("o_clerk"),
					   rs.getInt("o_shippriority"),
					   rs.getString("o_comment"));
			n += 1;
		}
		rs.close();
	}
	System.out.println("Elapsed time for selecting " + n + " objects: " + (System.currentTimeMillis() - start));
	stmt.close();
	c.commit();
	c.close();
  }
}
