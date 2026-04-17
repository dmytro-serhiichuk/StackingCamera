//
// Created by sedv2 on 17.04.2026.
//

#include "file-storage.h"
#include "jni-helper.h"

char *JniFileStorage::createTempFile() {
    auto env = JNIHelper::getInstance()->jniEnv;
    auto jc = JNIHelper::getInstance()->jniHelperClass;
    auto jo = JNIHelper::getInstance()->jniHelperObject;

    jmethodID method = env->GetMethodID(jc, "createTempFile", "()Ljava/lang/String;");
    jstring jfilePath = ((jstring)(env->CallObjectMethod(jo, method)));

    const char* tempChars = env->GetStringUTFChars(jfilePath, nullptr);
    char* result = strdup(tempChars);

    env->ReleaseStringUTFChars(jfilePath, tempChars);
    env->DeleteLocalRef(jfilePath);

    return result;
}

int JniFileStorage::createImageFile(const char *fileName) {
    auto env = JNIHelper::getInstance()->jniEnv;
    auto jc = JNIHelper::getInstance()->jniHelperClass;
    auto jo = JNIHelper::getInstance()->jniHelperObject;

    jstring jfileName = env->NewStringUTF(fileName);
    jmethodID method = env->GetMethodID(jc, "createImageFile", "(Ljava/lang/String;)I");

    jint fd = env->CallIntMethod(jo, method, jfileName);

    env->DeleteLocalRef(jfileName);

    return fd;
}
