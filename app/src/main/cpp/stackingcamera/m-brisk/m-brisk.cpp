//
// Created by sedv2 on 11.01.2026.
//

#include "m-brisk.h"
#include <cmath>
#include "bitmap-operations.h"
#include "fast.h"
#include "core.h"
#include <algorithm>

Descriptors::~Descriptors() {
    if (buffer != nullptr) delete [] buffer;
    buffer = nullptr;
    count = 0;
}

size_t Descriptors::sizeOf() {
    return count * DESCRIPTOR_LENGTH * sizeof(uint64_t);
}

M_BRISK::M_BRISK(uint32_t _octaves, float _briskScaleFactor) {
    nOctaves = _octaves + 1;
    scaleRange = dScaleRange * nOctaves;

    size_t pairsMaxSize = nPoints * (nPoints - 1) / 2;
    shortPairs = new Buffer<BriskShortPair>(pairsMaxSize);
    longPairs = new Buffer<BriskLongPair>(pairsMaxSize);

    uint32_t nRings = 5;
    float *radiusList = new float[nRings] {
            0.0f,
            2.465f * _briskScaleFactor,
            4.165f * _briskScaleFactor,
            6.29f * _briskScaleFactor,
            9.18f * _briskScaleFactor
    };
    int32_t* numberList = new int32_t[nRings] { 1, 10, 14, 15, 20 };

    /* init sin and cos lookup tables */
    double* sinLUT = new double[nRotations];
    double* cosLUT = new double[nRotations];

    double sin = 0.0;
    double cos = 1.0;
    double dsin = std::sin(2 * M_PI / double(nRotations));
    double dcos = std::cos(2 * M_PI / double(nRotations));
    for (size_t rot = 0; rot < nRotations; rot++) {
        sinLUT[rot] = sin;
        cosLUT[rot] = cos;
        double t = sin * dcos + cos * dsin;
        cos = cos * dcos - sin * dsin;
        sin = t;
    }

    scales = new float[nOctaves];
    sizes = new uint32_t[nOctaves];
    patternPoints = new BriskPatternPoint[nPoints * nRotations * nOctaves];

    const float lb_scale = std::log2f(scaleRange);
    const float lb_scale_step = lb_scale / nOctaves;

    const float sigma_scale = 1.3f;

    /* init pattern points */
    for (uint32_t octave = 0; octave < nOctaves; octave++) {
        scales[octave] = std::pow(2.0f, octave * lb_scale_step);
        BriskPatternPoint *patternIteratorOuter = patternPoints + (octave * nRotations * nPoints);

        for (int32_t ring = 0; ring < nRings; ring++) {
            double scaleRadiusProduct = scales[octave] * radiusList[ring];
            float patternSigma = 0.0f;
            if (ring == 0) {
                patternSigma = sigma_scale * scales[octave] * 0.5f;
            }
            else {
                patternSigma = sigma_scale * scales[octave] * (double)radiusList[ring] *
                               std::sin(M_PI / numberList[ring]);
            }
            sizes[octave] = std::ceil(scales[octave] * radiusList[ring] + patternSigma) + 1;

            for (int32_t num = 0; num < numberList[ring]; num++) {
                BriskPatternPoint *patternIterator = patternIteratorOuter;
                double alpha = (double)num * 2 * M_PI / (double)numberList[ring];
                double sinAlpha = std::sin(alpha);
                double cosAlpha = std::cos(alpha);

                for (size_t rot = 0; rot < nRotations; rot++) {
                    double cosTheta = cosLUT[rot];
                    double sinTheta = sinLUT[rot];

                    patternIterator->x = scaleRadiusProduct * (cosTheta * cosAlpha - sinTheta * sinAlpha);
                    patternIterator->y = scaleRadiusProduct * (sinTheta * cosAlpha + cosTheta * sinAlpha);
                    patternIterator->sigma = patternSigma;

                    patternIterator += nPoints; // to next rotation position of this point
                }
                patternIteratorOuter++; // to next point
            }
        }
    }

    delete [] sinLUT;
    delete [] cosLUT;

    /* init pairs */
    uint32_t nLongPairs = 0;
    uint32_t nShortPairs = 0;

    const float _dMinSq = dMinSq * _briskScaleFactor * _briskScaleFactor;
    const float _dMaxSq = dMaxSq * _briskScaleFactor * _briskScaleFactor;

    for (uint32_t i = 1; i < nPoints; i++) {
        for (uint32_t j = 0; j < i; j++) {
            const float dx = patternPoints[j].x - patternPoints[i].x;
            const float dy = patternPoints[j].y - patternPoints[i].y;
            const float norm_sq = (dx * dx + dy * dy);
            if (norm_sq > _dMinSq) {
                (*longPairs)[nLongPairs].i = i;
                (*longPairs)[nLongPairs].j = j;
                (*longPairs)[nLongPairs].weighted_dx = (int)(dx / norm_sq * 2048.0 + 0.5);
                (*longPairs)[nLongPairs].weighted_dy = (int)(dy / norm_sq * 2048.0 + 0.5);
                nLongPairs++;
            }
            else if (norm_sq < _dMaxSq) {
                (*shortPairs)[nShortPairs].i = i;
                (*shortPairs)[nShortPairs].j = j;
                nShortPairs++;
            }
        }
    }

    delete [] radiusList;
    delete [] numberList;

    shortPairs->size = nShortPairs;
    shortPairs->shrink();
    longPairs->size = nLongPairs;
    longPairs->shrink();
}

M_BRISK::~M_BRISK() {
    delete [] scales;
    delete [] sizes;
    delete [] patternPoints;
    delete shortPairs;
    delete longPairs;
}

bool M_BRISK::RoiPredicate(Bitmap &bitmap, KeyPoint &kp, uint32_t size) {
    return kp.x >= size && kp.y >= size && kp.x < bitmap.width - size && kp.y < bitmap.height - size;
}

void M_BRISK::subpixelRefine(Bitmap &bitmap, cl_mem buffer, Buffer<KeyPoint> &keypoints) {
    cl_mem kpsBuffer = CL::createBuffer(
            CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
            keypoints.size * sizeof(KeyPoint), keypoints.buffer
    );

    cl_kernel kernel = CL::createKernel("subpixel_refine");

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &kpsBuffer);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer);
    clSetKernelArg(kernel, 2, sizeof(int), &bitmap.width);
    clSetKernelArg(kernel, 3, sizeof(int), &bitmap.height);

    cl_event finished;

    clEnqueueNDRangeKernel(
            CL::queue, kernel, 1, nullptr, &keypoints.size,
            nullptr, 0, nullptr, &finished
    );
    clEnqueueReadBuffer(
        CL::queue, kpsBuffer,
        CL_TRUE, 0, keypoints.size * sizeof(KeyPoint),
        keypoints.buffer, 1, &finished,nullptr
    );

    clReleaseEvent(finished);
    clReleaseKernel(kernel);
    clReleaseMemObject(kpsBuffer);
}

inline void M_BRISK::filterKeypointsAfterRefining(Bitmap &bmp, Buffer<KeyPoint> &kps) {
    size_t fi = 0;
    for (size_t i = 0; i < kps.size; i++) {
        if (RoiPredicate(bmp, kps[i], sizes[kps[i].octave + 1])) {
            kps[fi] = kps[i];
            fi++;
        }
    }
    kps.size = fi;
    kps.shrink();

    JNIHelper::getInstance()->writeMessageToLog(false, "\tTotal number of selected keypoints: %zd\n\tKeypoints detecting completed", kps.size);
}

Buffer<KeyPoint> *M_BRISK::detect(Bitmap &inputBitmap) {
    cl_mem buffer = CL::createBuffer(
            CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS,
            inputBitmap.sizeOfBuffer(), inputBitmap.buffer
    );
    BitmapInfo bitmapInfo {inputBitmap.width, inputBitmap.height, inputBitmap.colorSpace, inputBitmap.depth};
    toGray8(bitmapInfo, buffer);
    CLAHE(bitmapInfo, buffer);

    size_t kpsLen = bitmapInfo.bufferLength;
    Buffer<KeyPoint>* keyPoints = new Buffer<KeyPoint>(kpsLen);
    FAST::FAST_Buffers* fastBuffers = new FAST::FAST_Buffers(kpsLen * sizeof(KeyPoint));

    cl_mem subpixelRefineBuffer = CL::createBuffer(
            CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS,
            bitmapInfo.sizeOfBuffer(), nullptr
    );

    float lastScaleFactor = 1.0f;

    for (size_t i = 0; i < nOctaves - 1; i++) {
        if (i != 0) {
            if (!resize(bitmapInfo, buffer, OCTAVE_SCALE_FACTOR, MIN_OCTAVE_SIZE)) break;
            lastScaleFactor *= OCTAVE_SCALE_FACTOR;
        }
        GaussianKernel* gk = GaussianKernel::create(lastScaleFactor);
        blur(bitmapInfo, buffer, *gk);

        if (i == 0) {
            CL::copyBuffer(buffer, subpixelRefineBuffer, bitmapInfo.sizeOfBuffer());
        }

        FAST::detect(bitmapInfo, buffer, *keyPoints, *fastBuffers, FAST_PADDING, i, lastScaleFactor);
        delete gk;

        JNIHelper::getInstance()->writeMessageToLog(false, "\tOctave %zd keypoints detecting completed", i);
    }

    delete fastBuffers;
    clReleaseMemObject(buffer);

    std::sort(keyPoints->begin(), keyPoints->end(), [](const KeyPoint &a, const KeyPoint &b) {
        return a.response > b.response;
    });

    if (keyPoints->size > MAX_KEYPOINTS) {
        keyPoints->size = MAX_KEYPOINTS;
    }

    JNIHelper::getInstance()->writeMessageToLog(false, "\tTotal number of detected keypoints: %zd", keyPoints->size);

    if (keyPoints->size <= Core::KEYPOINTS_PER_CHUNK * Core::CHUNKS_COUNT) {
        subpixelRefine(inputBitmap, subpixelRefineBuffer, *keyPoints);
        clReleaseMemObject(subpixelRefineBuffer);
        filterKeypointsAfterRefining(inputBitmap, *keyPoints);
        return keyPoints;
    }

    int32_t chunkWidth = inputBitmap.width / Core::CHUNKS_PER_SIDE;
    int32_t chunkHeight = inputBitmap.height / Core::CHUNKS_PER_SIDE;

    uint32_t* counter = new uint32_t[Core::CHUNKS_COUNT] { 0 };
    size_t fi = 0;
    for (size_t i = 0; i < keyPoints->size; i++) {
        uint32_t x = std::min((uint32_t)keyPoints->buffer[i].x / chunkWidth, Core::CHUNKS_PER_SIDE - 1);
        uint32_t y = std::min((uint32_t)keyPoints->buffer[i].y / chunkHeight, Core::CHUNKS_PER_SIDE - 1);

        uint32_t index = y * Core::CHUNKS_PER_SIDE + x;

        if (counter[index] < Core::KEYPOINTS_PER_CHUNK) {
            keyPoints->buffer[fi] = keyPoints->buffer[i];
            counter[index]++;
            fi++;
        }
    }
    delete [] counter;

    keyPoints->size = fi;

    subpixelRefine(inputBitmap, subpixelRefineBuffer, *keyPoints);
    clReleaseMemObject(subpixelRefineBuffer);
    filterKeypointsAfterRefining(inputBitmap, *keyPoints);
    return keyPoints;
}

Descriptors *M_BRISK::compute(Bitmap &inputBitmap, Buffer<KeyPoint> &keyPoints) {
    cl_mem buffer = CL::createBuffer(
            CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS,
            inputBitmap.sizeOfBuffer(), inputBitmap.buffer
    );
    Bitmap* bitmap = toGray8WithReading(inputBitmap, buffer);

    int32_t* integral = getIntegralImage(*bitmap);
    size_t integralSize = (bitmap->width + 1) * (bitmap->height + 1) * sizeof(int32_t);

    cl_mem integralBuffer = CL::createBuffer(
        CL_MEM_HOST_NO_ACCESS | CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        integralSize, integral
    );

    cl_mem kpBuffer = CL::createBuffer(
        CL_MEM_HOST_NO_ACCESS | CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        keyPoints.size * sizeof(KeyPoint), keyPoints.buffer
    );

    Descriptors* descriptors = new Descriptors();
    descriptors->count = keyPoints.size;
    descriptors->buffer = new uint64_t[descriptors->count * Descriptors::DESCRIPTOR_LENGTH]();
    size_t descriptorsSize = descriptors->count * Descriptors::DESCRIPTOR_LENGTH * sizeof(uint64_t);
    cl_mem descBuffer = CL::createBuffer(
            CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
            descriptorsSize, descriptors->buffer
    );

    size_t patternSize = nPoints * nRotations * nOctaves * sizeof(BriskPatternPoint);
    cl_mem patternBuffer = CL::createBuffer(
            CL_MEM_HOST_NO_ACCESS | CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            patternSize, patternPoints
    );

    cl_mem sp = CL::createBuffer(
            CL_MEM_HOST_NO_ACCESS | CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            shortPairs->size * sizeof(BriskShortPair), shortPairs->buffer
    );
    cl_mem lp = CL::createBuffer(
            CL_MEM_HOST_NO_ACCESS | CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            longPairs->size * sizeof(BriskLongPair), longPairs->buffer
    );

    cl_kernel kernel = CL::createKernel("brisk");

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer);
    clSetKernelArg(kernel, 1, sizeof(uint32_t), &bitmap->width);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &integralBuffer);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &kpBuffer);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), &descBuffer);
    clSetKernelArg(kernel, 5, sizeof(cl_mem), &patternBuffer);
    clSetKernelArg(kernel, 6, sizeof(cl_mem), &sp);
    clSetKernelArg(kernel, 7, sizeof(cl_mem), &lp);

    cl_event finished;

    clEnqueueNDRangeKernel(
            CL::queue, kernel, 1, nullptr,
            &keyPoints.size, nullptr, 0, nullptr, &finished
    );
    clEnqueueReadBuffer(
        CL::queue, descBuffer,
        CL_TRUE, 0, descriptorsSize, descriptors->buffer,
        1, &finished,nullptr
    );

    clReleaseEvent(finished);
    clReleaseKernel(kernel);
    clReleaseMemObject(buffer);
    clReleaseMemObject(integralBuffer);
    clReleaseMemObject(kpBuffer);
    clReleaseMemObject(descBuffer);
    clReleaseMemObject(patternBuffer);
    clReleaseMemObject(sp);
    clReleaseMemObject(lp);

    delete[] integral;
    delete bitmap;

    return descriptors;
}
