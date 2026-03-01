#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <string>
#include "core.h"

extern "C"
JNIEXPORT void JNICALL
Java_com_sedv_stackingcamera_stacking_StackingActivity_initStacking(JNIEnv *env, jobject, jobject am) {
    try {
        JNIHelper::initialize(env);
        AAssetManager* aam = AAssetManager_fromJava(env, am);
        Core::init(aam);
    }
    catch (const std::exception& e) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), e.what());
    }
}
extern "C"
JNIEXPORT jlong JNICALL
Java_com_sedv_stackingcamera_stacking_StackingActivity_loadBitmap(JNIEnv *env, jobject thiz,
                                                                  jint fd) {
    try {
        JNIHelper::getInstance()->updateHelper(env, thiz);
        Core::loadBitmap(fd);
        auto loadedBitmapPtr = Core::sources->buffer[Core::sources->size - 1]->bitmapPtr;

        jlong packed = ((jlong)loadedBitmapPtr->width << 32) | loadedBitmapPtr->height;
        return packed;
    } catch (std::exception &e) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), e.what());
        return -1;
    }
}
inline int calculateImageChannelSupport(bool uint8, bool uint16) {
    if (uint8 && uint16) return 3;
    else if (uint8) return 1;
    else if (uint16) return 2;
    else return 0;
}
extern "C"
JNIEXPORT jobject JNICALL
Java_com_sedv_stackingcamera_stacking_settings_Settings_getAvailableImageSettings(JNIEnv *env,
                                                                                  jobject thiz) {
    jclass cls = env->FindClass("com/sedv/stackingcamera/stacking/settings/AvailableImageSettings");
    jmethodID constructor = env->GetMethodID(cls, "<init>", "(II)V");

    int rgbSupport = calculateImageChannelSupport(
        CL::rgbInfo.UNORM_INT8_SUPPORT.SUPPORT_READ_WRITE,
        CL::rgbInfo.UNORM_INT16_SUPPORT.SUPPORT_READ_WRITE
    );
    int rgbaSupport = calculateImageChannelSupport(
            CL::rgbaInfo.UNORM_INT8_SUPPORT.SUPPORT_READ_WRITE,
            CL::rgbaInfo.UNORM_INT16_SUPPORT.SUPPORT_READ_WRITE
    );

    jobject javailableImageSettings = env->NewObject(cls,constructor, rgbSupport, rgbaSupport);
    env->DeleteLocalRef(cls);

    return javailableImageSettings;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_sedv_stackingcamera_stacking_settings_Settings_applySettings(JNIEnv *env, jobject thiz,
                                                                      jint fast_threshold,
                                                                      jfloat ransac_threshold,
                                                                      jint ransac_iterations,
                                                                      jint tiles_per_side,
                                                                      jint max_keypoints,
                                                                      jint max_matches,
                                                                      jfloat brisk_pattern_scale,
                                                                      jboolean use16_bit,
                                                                      jboolean use_images,
                                                                      jint image_format,
                                                                      jboolean save_keypoints,
                                                                      jboolean save_matches) {
    Core::applySettings(
        fast_threshold, ransac_threshold, ransac_iterations, tiles_per_side, max_keypoints,
        max_matches, brisk_pattern_scale, use16_bit, use_images, image_format,
        save_keypoints, save_matches
    );
}
extern "C"
JNIEXPORT void JNICALL
Java_com_sedv_stackingcamera_stacking_StackingActivity_removeBitmap(JNIEnv *env, jobject thiz,
                                                                    jint index) {
    Core::removeBitmapAt(index);
}
extern "C"
JNIEXPORT jintArray JNICALL
Java_com_sedv_stackingcamera_stacking_StackingActivity_analyse(JNIEnv *env, jobject thiz,
                                                               jboolean reanalyse) {
    JNIHelper::getInstance()->updateHelper(env, thiz);
    Core::analyse(reanalyse);

    auto scores = new int32_t[Core::sources->size]();
    for (size_t i = 0; i < Core::sources->size; i++) {
        if (Core::sources->buffer[i]->keyPoints != nullptr) {
            scores[i] = (int32_t)Core::sources->buffer[i]->keyPoints->size;
        }
    }
    jintArray jscores = env->NewIntArray(Core::sources->size);
    env->SetIntArrayRegion(jscores, 0, Core::sources->size, scores);

    delete [] scores;
    return jscores;
}
extern "C"
JNIEXPORT jintArray JNICALL
Java_com_sedv_stackingcamera_stacking_StackingActivity_match(JNIEnv *env, jobject thiz) {
    try {
        JNIHelper::getInstance()->updateHelper(env, thiz);
        auto infos = Core::match();

        int totalSize = HomographyValidation::ValidationInfo::FIELDS_NUMBER * infos.size();

        jintArray result = env->NewIntArray(totalSize);
        std::vector<jint> flat;
        flat.reserve(totalSize);

        for(const auto& info : infos) {
            flat.push_back(info.bitmapIndex);
            flat.push_back(static_cast<int>(info.scale));
            flat.push_back(static_cast<int>(info.translationX));
            flat.push_back(static_cast<int>(info.translationY));
            flat.push_back(static_cast<int>(info.perspective));
            flat.push_back(static_cast<int>(info.shear));
            flat.push_back(static_cast<int>(info.anisotropy));
            flat.push_back(info.isConvex ? 1 : 0);
            flat.push_back(info.mirrored ? 1 : 0);
        }

        env->SetIntArrayRegion(result, 0, totalSize, flat.data());
        return result;
    } catch (std::exception &e) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), e.what());
        return nullptr;
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_sedv_stackingcamera_stacking_StackingActivity_stack(JNIEnv *env, jobject thiz,
                                                             jboolean disable_alignment) {
    try {
        JNIHelper::getInstance()->updateHelper(env, thiz);
        Core::stack(disable_alignment);
    } catch (std::exception &e) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), e.what());
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_sedv_stackingcamera_stacking_StackingActivity_save(JNIEnv *env, jobject thiz, jint fd,
                                                            jint format) {
    try {
        Core::save(fd, format);
    } catch (std::exception &e) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), e.what());
    }
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_sedv_stackingcamera_stacking_StackingActivity_getIndexOfReferenceFrame(JNIEnv *env,
                                                                                jobject thiz) {
    return Core::updateReferenceFrameIndex();
}