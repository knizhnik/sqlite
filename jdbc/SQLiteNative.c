#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sqlite3.h>

time_t getCurrentTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000000 + tv.tv_usec;
}

#define CHECK(op) do { int error_ = (op); if (error_ != SQLITE_OK) { fprintf(stderr, "%s:%d: %s failed with code %d\n", __FILE__, __LINE__, #op, error_); exit(1); } } while (0)

typedef struct Orders { 
    int o_orderkey;
    int o_custkey;
    char o_orderstatus;
    double o_totalprice;
    char o_orderdate[10];
    char o_orderpriority[15];
    char o_clerk[15];													
    int o_shippriority;													
    char o_comment[79];
} Orders;

Orders o;

int main(int argc, char* argv[]) 
{ 
	int nIterations = argc > 1 ? atoi(argv[1]) : 1000000;
	unsigned nAccounts = argc > 2 ? atoi(argv[2]) : 1500000;
	int i, n = 0;
	sqlite3 *conn;
	sqlite3_stmt* stmt;
	time_t start;
	
	CHECK(sqlite3_enable_shared_cache(0));
	CHECK(sqlite3_open("test.db", &conn));
	CHECK(sqlite3_exec(conn, "PRAGMA cache_size = 20000", NULL, NULL, NULL));
	CHECK(sqlite3_exec(conn, "PRAGMA journal_mode=WAL", NULL, NULL, NULL));
	CHECK(sqlite3_exec(conn, "begin transaction", NULL, NULL, NULL));
	CHECK(sqlite3_prepare_v2(conn, "select * from orders where o_orderkey=?", -1, &stmt, NULL));

	start = getCurrentTime();
	for (i = 0; i < nIterations; i++) { 
		int value = random() % nAccounts;
		CHECK(sqlite3_bind_int(stmt, 1, value));
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			o.o_orderkey = sqlite3_column_int(stmt, 0);
			o.o_custkey = sqlite3_column_int(stmt, 1);
			o.o_orderstatus  = (char)sqlite3_column_int(stmt, 2);		
			o.o_totalprice = sqlite3_column_double(stmt, 3);
			strncpy(o.o_orderdate, (char const*)sqlite3_column_text(stmt, 4), 10);
			strncpy(o.o_orderpriority, (char const*)sqlite3_column_text(stmt, 5), 15);
			strncpy(o.o_clerk, (char const*)sqlite3_column_text(stmt, 6), 15);
			o.o_shippriority = sqlite3_column_int(stmt, 7);
			strncpy(o.o_comment, (char const*)sqlite3_column_text(stmt, 8), 79);
			n += 1;
		}
		CHECK(sqlite3_reset(stmt));	
	}
	CHECK(sqlite3_exec(conn, "commit transaction", NULL, NULL, NULL));
	printf("Elapsed time for %d objects: %ld\n", n, getCurrentTime() - start);
	return 0;   

}
