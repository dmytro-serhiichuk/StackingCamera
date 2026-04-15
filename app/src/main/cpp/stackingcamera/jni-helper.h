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

    static const size_t LOG_BUFFER_SIZE = 512;
    static char LOG_BUFFER[LOG_BUFFER_SIZE];

    JNIEnv *jniEnv = nullptr;
    // TODO: use lightweight listener instead of the whole activity
    jobject jniHelperObject = nullptr;
    jclass jniHelperClass = nullptr;

public:
    static void initialize(JNIEnv *env);

    static JNIHelper* getInstance() {
        return instance;
    }

    ~JNIHelper();

    void updateHelper(JNIEnv *env, jobject newHelper);
    char* createTempFile();
    int createImageFile(const char *fileName);
    void writeMessageToLog(bool isError, const char *format, ...);
};

#endif //STACKINGCAMERA_JNI_HELPER_H
