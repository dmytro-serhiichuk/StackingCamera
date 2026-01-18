//
// Created by sedv2 on 10.01.2026.
//

#ifndef STACKINGCAMERA_JNI_HELPER_H
#define STACKINGCAMERA_JNI_HELPER_H

#include <string>
#include <jni.h>

class JNIHelper {
private:
    static JavaVM *jvm;
    static JNIHelper *instance;

    JNIEnv *jniEnv;
    // TODO: use lightweight listener instead of the whole activity
    jobject jniHelperObject;
    jclass jniHelperClass;

    JNIHelper(): jniHelperObject(nullptr), jniHelperClass(nullptr) {}

public:
    static void initialize(JNIEnv *env);

    static JNIHelper* getInstance() {
        return instance;
    }

    ~JNIHelper();

    void updateHelper(JNIEnv *env, jobject newHelper);
    char* createTempFile();
    int createImageFile(const char *fileName);
};

#endif //STACKINGCAMERA_JNI_HELPER_H
