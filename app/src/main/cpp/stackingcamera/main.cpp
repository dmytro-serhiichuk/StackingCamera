#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <string>


extern "C"
JNIEXPORT void JNICALL
Java_com_sedv_stackingcamera_stacking_StackingActivity_initStacking(JNIEnv *env, jobject, jobject am) {
    try {
        AAssetManager* gAssetManager = AAssetManager_fromJava(env, am);

    }
    catch (const std::exception& e) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), e.what());
    }
}