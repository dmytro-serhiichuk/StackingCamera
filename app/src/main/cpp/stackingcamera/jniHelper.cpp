//
// Created by sedv2 on 27.01.2025.
//

#include "jniHelper.h"

JNIHelper::~JNIHelper() {
    env->DeleteGlobalRef(jniHelperObject);
    env->DeleteGlobalRef(jniHelperClass);
}

jcharArray JNIHelper::toJavaCharArray(const char *input) {
    if (input == nullptr) {
        return nullptr;
    }

    size_t len = std::strlen(input);

    jcharArray jCharArr = env->NewCharArray(len);
    if (jCharArr == nullptr) {
        return nullptr;
    }

    jchar* unicodeChars = new jchar[len];
    for (size_t i = 0; i < len; ++i) {
        unicodeChars[i] = static_cast<jchar>(input[i]);
    }

    env->SetCharArrayRegion(jCharArr, 0, len, unicodeChars);

    delete[] unicodeChars;

    return jCharArr;
}

void JNIHelper::setProgressMessage() {
    jmethodID method = env->GetMethodID(jniHelperClass, "setLoadingMessage", "([C)V");

    jcharArray message = toJavaCharArray(progressMessage.c_str());

    env->CallVoidMethod(jniHelperObject, method, message);
    env->DeleteLocalRef(message);
}

int JNIHelper::createImageFile(const char *fileName) {
    jmethodID method = env->GetMethodID(jniHelperClass, "createImageFile", "([C)I");

    jcharArray jFileName = toJavaCharArray(fileName);

    jint fd = env->CallIntMethod(jniHelperObject, method, jFileName);
    env->DeleteLocalRef(jFileName);

    return fd;
}

char* JNIHelper::createTempFile() {
    jmethodID method = env->GetMethodID(jniHelperClass, "createTempFile", "()Ljava/lang/String;");
    jstring str = static_cast<jstring>(env->CallObjectMethod(jniHelperObject, method));
    const char *cstr = env->GetStringUTFChars(str, nullptr);

    char* filePath = new char[strlen(cstr) + 1];
    strcpy(filePath, cstr);

    env->ReleaseStringUTFChars(str, cstr);
    env->DeleteLocalRef(str);
    return filePath;
}