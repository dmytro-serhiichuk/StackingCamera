//
// Created by sedv2 on 10.01.2026.
//

#ifndef STACKINGCAMERA_JNI_HELPER_H
#define STACKINGCAMERA_JNI_HELPER_H

#include <string>
#include <jni.h>

class JNIHelper {
public:
    static JavaVM *jvm;
    static JNIHelper *instance;

    JNIEnv *jniEnv = nullptr;
    // TODO: use lightweight listener instead of the whole activity
    jobject jniHelperObject = nullptr;
    jclass jniHelperClass = nullptr;

    static void initialize(JNIEnv *env);

    static JNIHelper* getInstance() {
        return instance;
    }

    ~JNIHelper();

    void updateHelper(JNIEnv *env, jobject newHelper);
};

#endif //STACKINGCAMERA_JNI_HELPER_H
