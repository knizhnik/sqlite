create table lineitem(
   l_orderkey integer,
   l_partkey integer,
   l_suppkey integer,
   l_linenumber integer,
   l_quantity real,
   l_extendedprice real,
   l_discount real,
   l_tax real,
   l_returnflag char,
   l_linestatus char,
   l_shipdate date,
   l_commitdate date,
   l_receiptdate date,
   l_shipinstruct char(25),
   l_shipmode char(10),
   l_comment char(44),
   l_dummy char(1));
.separator '|'
.import /home/knizhnik/tpch-dbgen/lineitem.tbl lineitem
.timer ON
create table lineitem2(
   l_orderkey integer,
   l_partkey integer,
   l_suppkey integer,
   l_linenumber integer,
   l_quantity real,
   l_extendedprice real,
   l_discount real,
   l_tax real,
   l_returnflag integer,
   l_linestatus integer,
   l_shipdate date,
   l_commitdate integer,
   l_receiptdate integer,
   l_shipinstruct char(25),
   l_shipmode char(10),
   l_comment char(44));
insert into lineitem2 select l_orderkey,l_partkey,l_suppkey,l_linenumber,l_quantity,l_extendedprice,l_discount,l_tax,unicode(l_returnflag),unicode(l_linestatus),
cast(substr(l_shipdate,1,4) as integer)*10000+cast(substr(l_shipdate,6,2) as integer)*100+cast(substr(l_shipdate,8,2) as integer),
cast(substr(l_commitdate,1,4) as integer)*10000+cast(substr(l_commitdate,6,2) as integer)*100+cast(substr(l_commitdate,8,2) as integer),
cast(substr(l_receiptdate,1,4) as integer)*10000+cast(substr(l_receiptdate,6,2) as integer)*100+cast(substr(l_receiptdate,8,2) as integer),
l_shipinstruct,l_shipmode,l_comment from lineitem;

Select
    l_returnflag,
    l_linestatus,
    sum(l_quantity) as sum_qty,
    sum(l_extendedprice) as sum_base_price,
    sum(l_extendedprice*(1-l_discount)) as sum_disc_price,
    sum(l_extendedprice*(1-l_discount)*(1+l_tax)) as sum_charge,
    avg(l_quantity) as avg_qty,
    avg(l_extendedprice) as avg_price,
    avg(l_discount) as avg_disc,
    count(*) as count_order
from
    lineitem
where
    l_shipdate <= '1998-12-01'
group by
    l_returnflag,
    l_linestatus
order by
    l_returnflag,
    l_linestatus;

Select
    l_returnflag,
    l_linestatus,
	count(*)
from
    lineitem
where
    l_shipdate <= '1998-12-01'
group by
    l_returnflag,
    l_linestatus
order by
    l_returnflag,
    l_linestatus;



Select
    sum(l_extendedprice*l_discount) as revenue
from
    lineitem
where
    l_shipdate between '1996-01-01' and '1997-01-01'
    and l_discount between 0.08 and 0.1
    and l_quantity < 24;

select
    l_returnflag,
    l_linestatus,
    sum(l_quantity) as sum_qty,
    sum(l_extendedprice) as sum_base_price,
    sum(l_extendedprice*(1-l_discount)) as sum_disc_price,
    sum(l_extendedprice*(1-l_discount)*(1+l_tax)) as sum_charge,
    avg(l_quantity) as avg_qty,
    avg(l_extendedprice) as avg_price,
    avg(l_discount) as avg_disc,
    count(*) as count_order
from
    lineitem
where
    l_shipdate <= '1998-12-01'
group by
    l_returnflag,
    l_linestatus
order by
    l_returnflag,
    l_linestatus;
select
    sum(l_extendedprice*l_discount) as revenue
from
    lineitem
where
    l_shipdate between '1996-01-01' and '1997-01-01'
    and l_discount between 0.08 and 0.1
    and l_quantity < 24;

