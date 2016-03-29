import java.sql.*;
import java.util.Map;
import java.util.HashMap;
import java.util.ArrayList;


class Lineitem
{
	public String l_shipdate;
	public byte l_returnflag;
    public byte l_linestatus;
	public double l_quantity;
	public double l_extendedprice;
	public double l_discount;
	public double l_tax;

	public int hashCode() { 
		return (l_returnflag << 8) | l_linestatus;
	}
	
	public boolean equals(Object o) { 
		return o instanceof Lineitem && ((Lineitem)o).l_returnflag == l_returnflag && ((Lineitem)o).l_linestatus == l_linestatus;
	}

	public Lineitem(byte returnflag,
					byte linestatus,
					double quantity,
					double extendedprice,
					double discount,
					double tax,
					String shipdate)
	{
		this.l_returnflag = returnflag;
		this.l_linestatus = linestatus;
		this.l_quantity = quantity;
		this.l_extendedprice = extendedprice;
		this.l_discount = discount;
		this.l_tax = tax;
		this.l_shipdate = shipdate;
	}
}

class Agg 
{
	public double sum_qty;
	public double sum_base_price;
	public double sum_disc_price;
	public double sum_charge;
	public double avg_qty;
	public double avg_price;
	public double avg_disc;
	public int count;
};
	
	
public class JdbcQ1
{
  public static void main( String args[] ) throws Exception
  {
    Connection c = null;
	int nIterations = args.length > 0 ? Integer.parseInt(args[0]) : 1000000;
	int nAccounts = args.length > 1 ? Integer.parseInt(args[1]) : 1500000;

	Class.forName("org.sqlite.JDBC");
	c = DriverManager.getConnection("jdbc:sqlite:lineitem.db");
	c.setAutoCommit(false);
	System.out.println("Opened database successfully");
	
	long start = System.currentTimeMillis();
	int n = 0;
	HashMap<Lineitem,Agg> map = new HashMap<Lineitem,Agg>();
	Statement stmt = c.createStatement();
	ResultSet rs = stmt.executeQuery("select l_returnflag,l_linestatus,l_quantity,l_extendedprice,l_discount,l_tax,l_shipdate from lineitem");
	ArrayList<Lineitem> list = new ArrayList<Lineitem>();
	while (rs.next()) {
		Lineitem l = new Lineitem(
			(byte)rs.getString(1).charAt(0),
			(byte)rs.getString(2).charAt(0),
			rs.getDouble(3),
			rs.getDouble(4),
			rs.getDouble(5),
			rs.getDouble(6),
			rs.getString(7));
		list.add(l);
	}
	rs.close();
	System.out.println("Elapsed time for loading: " + (System.currentTimeMillis() - start));
	start = System.currentTimeMillis();
	for (Lineitem l : list) {
		if (l.l_shipdate.compareTo("1998-12-01") <= 0) { 
			Agg agg = map.get(l);
			if (agg == null) { 
				agg = new Agg();
				map.put(l, agg);
			}
			agg.sum_qty += l.l_quantity;
			agg.sum_base_price += l.l_extendedprice;
			agg.sum_disc_price += l.l_extendedprice*(1-l.l_discount);
			agg.sum_charge += l.l_extendedprice*(1-l.l_discount)*(1+l.l_tax);
			agg.avg_qty += l.l_quantity;
			agg.avg_price += l.l_extendedprice;
			agg.avg_disc += l.l_discount;
			agg.count += 1;
		}
	}
	for (Map.Entry<Lineitem,Agg> elem : map.entrySet())
	{
		Lineitem k = elem.getKey();
		Agg v = elem.getValue();
		System.out.print(k.l_returnflag);System.out.print('\t');
		System.out.print(k.l_linestatus);System.out.print('\t');
		System.out.print(v.sum_qty);System.out.print('\t');
		System.out.print(v.sum_base_price);System.out.print('\t');
		System.out.print(v.sum_charge);System.out.print('\t');
		System.out.print(v.avg_qty);System.out.print('\t');
		System.out.print(v.avg_price);System.out.print('\t');
		System.out.print(v.avg_disc);System.out.print('\t');
		System.out.println(v.count);
	}
	System.out.println("Elapsed time for selecting " + n + " objects: " + (System.currentTimeMillis() - start));
	c.close();
  }
}
