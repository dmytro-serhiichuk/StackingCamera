#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <string>
#include "core.h"

static char* readProgramSource(AAssetManager *aam) {
    AAsset* asset = AAssetManager_open(aam, "program.cl", AASSET_MODE_BUFFER);
    if (!asset) {
        throw std::runtime_error("Program source file opening failed");
    }
    size_t sourceSize = AAsset_getLength(asset);
    char* buffer = new char[sourceSize + 1];
    AAsset_read(asset, buffer, sourceSize);
    AAsset_close(asset);
    buffer[sourceSize] = '\0';
    return buffer;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_sedv_stackingcamera_stacking_StackingActivity_initStacking(JNIEnv *env, jobject, jobject am) {
    try {
        JNIHelper::initialize(env);
        AAssetManager* aam = AAssetManager_fromJava(env, am);
        const char *programSrc = readProgramSource(aam);
        CL::init(programSrc);
        delete [] programSrc;
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
                                                                      jint color_space,
                                                                      jboolean save_keypoints,
                                                                      jboolean save_matches) {
    Core::applySettings(
        fast_threshold, ransac_threshold, ransac_iterations, tiles_per_side, max_keypoints,
        max_matches, brisk_pattern_scale, use16_bit, color_space, save_keypoints, save_matches
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
    jintArray jscores = env->NewIntArray((jsize)Core::sources->size);
    env->SetIntArrayRegion(jscores, 0, (jsize)Core::sources->size, scores);

    delete [] scores;
    return jscores;
}
extern "C"
JNIEXPORT jintArray JNICALL
Java_com_sedv_stackingcamera_stacking_StackingActivity_match(JNIEnv *env, jobject thiz) {
    try {
        JNIHelper::getInstance()->updateHelper(env, thiz);
        auto infos = Core::match();

        auto totalSize = (jsize)(Validation::ValidationInfo::FIELDS_NUMBER * infos.size());

        jintArray result = env->NewIntArray(totalSize);
        std::vector<jint> flat;
        flat.reserve(totalSize);

        for(const auto& info : infos) {
            flat.push_back(info.bitmapIndex);
            flat.push_back(static_cast<int>(info.homographyValidationInfo.scale));
            flat.push_back(static_cast<int>(info.homographyValidationInfo.translationX));
            flat.push_back(static_cast<int>(info.homographyValidationInfo.translationY));
            flat.push_back(static_cast<int>(info.homographyValidationInfo.perspective));
            flat.push_back(static_cast<int>(info.homographyValidationInfo.shear));
            flat.push_back(static_cast<int>(info.homographyValidationInfo.anisotropy));
            flat.push_back(info.homographyValidationInfo.isConvex ? 1 : 0);
            flat.push_back(info.homographyValidationInfo.mirrored ? 1 : 0);

            flat.push_back(static_cast<int>(info.matchesValidationInfo.inliersPercentage));
            flat.push_back(static_cast<int>(info.matchesValidationInfo.inliersNumber));
            flat.push_back(static_cast<int>(info.matchesValidationInfo.evenDistribution));
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