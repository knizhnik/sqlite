#define _JNI_IMPLEMENTATION_ 1
#include <jni.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <sqlite3.h>

JNIEXPORT jobject Java_Q1_nextRow(JNIEnv *env, jobject self, jlong rdd)
{
