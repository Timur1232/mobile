#include <jni.h>

JNIEXPORT jint JNICALL
Java_com_example_myapp_MainActivity_add(JNIEnv* env, jobject thiz, jint a, jint b) {
    (void) env;
    (void) thiz;
    return a + b;
}
