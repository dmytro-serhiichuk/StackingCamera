//
// Created by sedv2 on 10.01.2026.
//

#include "jni-helper.h"

JavaVM* JNIHelper::jvm = nullptr;
JNIHelper* JNIHelper::instance = nullptr;

void JNIHelper::initialize(JNIEnv *env) {
    if (!jvm) env->GetJavaVM(&jvm);
    if (!instance) instance = new JNIHelper();
}

JNIHelper::~JNIHelper() {
    if (jvm) {
        JNIEnv *env;
        jvm->GetEnv((void**)&env, JNI_VERSION_1_6);

        if (jniHelperObject) env->DeleteGlobalRef(jniHelperObject);
        if (jniHelperClass) env->DeleteGlobalRef(jniHelperClass);
    }
}

void JNIHelper::updateHelper(JNIEnv *env, jobject newHelper) {
    jniEnv = env;
    if (jniHelperObject) jniEnv->DeleteGlobalRef(jniHelperObject);
    if (jniHelperClass) jniEnv->DeleteGlobalRef(jniHelperClass);

    jniHelperObject = jniEnv->NewGlobalRef(newHelper);
    jniHelperClass = (jclass)jniEnv->NewGlobalRef(env->GetObjectClass(newHelper));
}
