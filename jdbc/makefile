all: classes library

JAVA_HOME=/usr/lib/jvm/java
JNI_INCLUDES = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux -I$(JAVA_HOME)/include/darwin -I/usr/java/default/include -I/usr/java/default/include/linux -I/usr/java/include -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux -I/usr/lib/jvm/oracle-jdk-bin-1.8/include

classes: *.java
	javac *.java

library: libsqlite-jni.so

libsqlite-jni.so: sqlite-jni.cpp 
	g++ -g -fPIC -shared $(JNI_INCLUDES) -o libsqlite-jni.so sqlite-jni.cpp -lsqlite3
