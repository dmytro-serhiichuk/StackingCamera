#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <string>
#include "imageio.h"
#include "core.h"

extern "C"
JNIEXPORT jobject JNICALL
Java_com_example_imagestacker_MainActivity_openImageNative(JNIEnv *env, jobject thiz, jint fd) {
    try {
        Core::jniHelper = new JNIHelper(env, thiz);

        BitmapPointer* bitmap = ImageIO::openImage(fd);

        if (bitmap == nullptr) {
            return nullptr;
        }

        Core::bitmaps->add(bitmap);

        jclass bitmapClass = env->FindClass("com/example/imagestacker/Bitmap");
        jmethodID constructor = env->GetMethodID(bitmapClass, "<init>", "(II)V");

        jobject kotlinBitmap = env->NewObject(
                bitmapClass,
                constructor,
                bitmap->width,
                bitmap->height
        );

        delete Core::jniHelper;
        Core::jniHelper = nullptr;

        return kotlinBitmap;
    }
    catch (std::exception &e) {
        delete Core::jniHelper;
        Core::jniHelper = nullptr;
        return nullptr;
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_imagestacker_MainActivity_saveNative(JNIEnv *env, jobject, jint fd, jint type) {
    Core::saveResult(fd, (Core::ExportTypes)type);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_imagestacker_MainActivity_removeBitmapNative(JNIEnv *env, jobject, jint index) {
    Core::removeByIndex(index);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_imagestacker_MainActivity_initNative(JNIEnv *env, jobject, jobject am) {
    try {
        AAssetManager* gAssetManager = AAssetManager_fromJava(env, am);
        OpenCL::initCL(gAssetManager);
        Core::init();
    }
    catch (const std::exception& e) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), e.what());
    }
}
extern "C"
JNIEXPORT jintArray JNICALL
Java_com_example_imagestacker_MainActivity_analyseNative(JNIEnv *env, jobject thiz, jboolean reAnalyse) {
    try {
        Core::jniHelper = new JNIHelper(env, thiz);
        Core::analyse(reAnalyse);
    }
    catch (std::exception &e) {
        delete Core::jniHelper;
        Core::jniHelper = nullptr;
        return nullptr;
    }

    jintArray jscores = env->NewIntArray(Core::keyPoints->size);
    int32_t* scores = new int32_t[Core::keyPoints->size];
    for (size_t i = 0; i < Core::keyPoints->size; i++) {
        scores[i] = Core::keyPoints->buffer[i]->size;
    }
    env->SetIntArrayRegion(jscores, 0, Core::keyPoints->size, scores);
    delete [] scores;
    delete Core::jniHelper;
    Core::jniHelper = nullptr;
    return jscores;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_imagestacker_MainActivity_stackNative(JNIEnv *env, jobject thiz, jboolean use_alignment) {
    try {
        Core::jniHelper = new JNIHelper(env, thiz);

        Core::stack(use_alignment);

        delete Core::jniHelper;
        Core::jniHelper = nullptr;
    }
    catch (std::exception &e) {
        delete Core::jniHelper;
        Core::jniHelper = nullptr;
    }
}
extern "C"
JNIEXPORT jintArray JNICALL
Java_com_example_imagestacker_MainActivity_getDetailedInfoNative(JNIEnv *env, jobject thiz,
                                                                 jint index) {
    if (index < 0 || index >= Core::bitmaps->size) {
        return nullptr;
    }

    BitmapPointer *bmp = Core::bitmaps->buffer[index];
    int32_t chunkWidth = bmp->width / Core::CHUNKS_PER_SIDE;
    int32_t chunkHeight = bmp->height / Core::CHUNKS_PER_SIDE;

    Buffer<KeyPoint> *coll = Core::keyPoints->buffer[index];

    int32_t* info = new int32_t[Core::CHUNKS_COUNT] { 0 };
    for (size_t i = 0; i < coll->size; i++) {
        uint32_t indexX = std::min((uint32_t)(*coll)[i].x / chunkWidth, Core::CHUNKS_PER_SIDE - 1);
        uint32_t indexY = std::min((uint32_t)(*coll)[i].y / chunkHeight, Core::CHUNKS_PER_SIDE - 1);

        uint32_t j = indexY * Core::CHUNKS_PER_SIDE + indexX;
        info[j]++;
    }
    jintArray jscoreInfo = env->NewIntArray(Core::CHUNKS_COUNT);
    env->SetIntArrayRegion(jscoreInfo, 0, Core::CHUNKS_COUNT, info);
    delete [] info;
    return jscoreInfo;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_imagestacker_Settings_setSettingsNative(JNIEnv *env, jobject thiz,
                                                         jint fastThreshold,
                                                         jfloat ransacThreshold,
                                                         jint ransacIterations,
                                                         jint chunkPerSide,
                                                         jint maxKeyPoints,
                                                         jint maxMatches,
                                                         jfloat briskPatternScale,
                                                         jboolean drawKeyPoints,
                                                         jboolean draw_matches
) {
    Core::setSettings(
            fastThreshold,
            ransacThreshold,
            ransacIterations,
            chunkPerSide,
            maxKeyPoints,
            maxMatches,
            briskPatternScale,
            drawKeyPoints,
            draw_matches
    );
}