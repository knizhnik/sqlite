create table orders(
    o_orderkey integer primary key,
    o_custkey integer,
    o_orderstatus char,
    o_totalprice real,
    o_orderdate date,
    o_orderpriority varchar,
    o_clerk varchar,
    o_shippriority integer,
    o_comment varchar,
    o_dummy char(1));
.separator '|'
.import /home/knizhnik/tpch-dbgen/sqlite> orders.tbl orders
