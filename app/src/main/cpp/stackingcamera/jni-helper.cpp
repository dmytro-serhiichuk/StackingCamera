//
// Created by sedv2 on 10.01.2026.
//

#include "jni-helper.h"

JNIHelper::~JNIHelper() {
    env->DeleteGlobalRef(jniHelperObject);
    env->DeleteGlobalRef(jniHelperClass);
}

char *JNIHelper::createTempFile() {
    jmethodID method = env->GetMethodID(jniHelperClass, "createTempFile", "()Ljava/lang/String;");
    jstring jfilePath = ((jstring)(env->CallObjectMethod(jniHelperObject, method)));

    const char* tempChars = env->GetStringUTFChars(jfilePath, nullptr);
    char* result = strdup(tempChars);

    env->ReleaseStringUTFChars(jfilePath, tempChars);
    env->DeleteLocalRef(jfilePath);

    return result;
}

int JNIHelper::createImageFile(const char *fileName) {
    jstring jfileName = env->NewStringUTF(fileName);
    jmethodID method = env->GetMethodID(jniHelperClass, "createImageFile", "(Ljava/lang/String;)I");

    jint fd = env->CallIntMethod(jniHelperObject, method, jfileName);

    env->DeleteLocalRef(jfileName);

    return fd;
}
