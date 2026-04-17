//
// Created by sedv2 on 17.04.2026.
//

#include "logger.h"
#include "jni-helper.h"

char JniLogger::LOG_BUFFER[JniLogger::LOG_BUFFER_SIZE];

void JniLogger::log(bool isError, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(LOG_BUFFER, LOG_BUFFER_SIZE, format, args);
    puts(LOG_BUFFER);
    va_end(args);

    auto env = JNIHelper::getInstance()->jniEnv;
    auto jc = JNIHelper::getInstance()->jniHelperClass;
    auto jo = JNIHelper::getInstance()->jniHelperObject;

    jstring jmessage = env->NewStringUTF(LOG_BUFFER);
    jmethodID method = env->GetMethodID(jc, "addLogMessage", "(Ljava/lang/String;Z)V");

    env->CallVoidMethod(jo, method, jmessage, isError);
    env->DeleteLocalRef(jmessage);
}
