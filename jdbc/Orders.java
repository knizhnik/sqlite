public class Orders {
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

