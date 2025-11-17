//
// Created by sedv2 on 27.01.2025.
//

#ifndef IMAGESTACKER_JNIHELPER_H
#define IMAGESTACKER_JNIHELPER_H

#include <string>
#include <jni.h>

class JNIHelper {
private:
    JNIEnv *env;
    jobject jniHelperObject;
    jclass jniHelperClass;

    jcharArray toJavaCharArray(const char *input);
public:
    std::string progressMessage = "";

    JNIHelper() {}

    JNIHelper(JNIEnv *env, jobject helper) :
        env(env),
        jniHelperObject(env->NewGlobalRef(helper)),
        jniHelperClass((jclass)env->NewGlobalRef(env->GetObjectClass(helper))) {}

    ~JNIHelper();

    void setProgressMessage();

    char* createTempFile();

    int createImageFile(const char *fileName);
};

#endif //IMAGESTACKER_JNIHELPER_H
