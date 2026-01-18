//
// Created by sedv2 on 10.01.2026.
//

#include "jni-helper.h"

JavaVM* JNIHelper::jvm = nullptr;
JNIHelper* JNIHelper::instance = nullptr;
char JNIHelper::LOG_BUFFER[JNIHelper::LOG_BUFFER_SIZE];

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

char *JNIHelper::createTempFile() {
    jmethodID method = jniEnv->GetMethodID(jniHelperClass, "createTempFile", "()Ljava/lang/String;");
    jstring jfilePath = ((jstring)(jniEnv->CallObjectMethod(jniHelperObject, method)));

    const char* tempChars = jniEnv->GetStringUTFChars(jfilePath, nullptr);
    char* result = strdup(tempChars);

    jniEnv->ReleaseStringUTFChars(jfilePath, tempChars);
    jniEnv->DeleteLocalRef(jfilePath);

    return result;
}

int JNIHelper::createImageFile(const char *fileName) {
    jstring jfileName = jniEnv->NewStringUTF(fileName);
    jmethodID method = jniEnv->GetMethodID(jniHelperClass, "createImageFile", "(Ljava/lang/String;)I");

    jint fd = jniEnv->CallIntMethod(jniHelperObject, method, jfileName);

    jniEnv->DeleteLocalRef(jfileName);

    return fd;
}

void JNIHelper::writeMessageToLog(bool isError, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(LOG_BUFFER, LOG_BUFFER_SIZE, format, args);
    puts(LOG_BUFFER);
    va_end(args);

    jstring jmessage = jniEnv->NewStringUTF(LOG_BUFFER);
    jmethodID method = jniEnv->GetMethodID(jniHelperClass, "addLogMessage", "(Ljava/lang/String;Z)V");

    jniEnv->CallVoidMethod(jniHelperObject, method, jmessage, isError);
    jniEnv->DeleteLocalRef(jmessage);
}
