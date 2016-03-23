#define _JNI_IMPLEMENTATION_ 1
#include <jni.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <sqlite3.h>

sqlite3 *conn;
sqlite3_stmt* stmt;
jmethodID ordersCons;
jclass ordersClass;

#define CHECK(op) do { int error_ = (op); if (error_ != SQLITE_OK) { fprintf(stderr, "%s:%d: %s failed with code %d\n", __FILE__, __LINE__, #op, error_); exit(1); } } while (0)

extern "C" {

JNIEXPORT jobject Java_SQLiteJNI_select(JNIEnv *env, jobject self, jint value)
{
	jobject result = NULL;
	if (!stmt) {  
		ordersClass = (jclass)env->NewGlobalRef(env->FindClass("Orders"));
		ordersCons = env->GetMethodID(ordersClass, "<init>", "(IIBDLjava/lang/String;Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;)V");
		CHECK(sqlite3_open("test.db", &conn));
		CHECK(sqlite3_prepare_v2(conn, "select * from orders where o_orderkey=?", -1, &stmt, NULL));
	}
	CHECK(sqlite3_bind_int(stmt, 1, value));
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		result = env->NewObject(ordersClass, ordersCons, 
								(jint)sqlite3_column_int(stmt, 0),
								(jint)sqlite3_column_int(stmt, 1),
								(jbyte)sqlite3_column_int(stmt, 2),
								(jdouble)sqlite3_column_double(stmt, 3),
								env->NewStringUTF((char const*)sqlite3_column_text(stmt, 4)),
								env->NewStringUTF((char const*)sqlite3_column_text(stmt, 5)),
								env->NewStringUTF((char const*)sqlite3_column_text(stmt, 6)),
								(jint)sqlite3_column_int(stmt, 7),								
								env->NewStringUTF((char const*)sqlite3_column_text(stmt, 8)));
	}
 	CHECK(sqlite3_reset(stmt));	
	return result;
}

}
