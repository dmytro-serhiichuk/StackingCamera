//
// Created by sedv2 on 16.03.2025.
//

#include "brisk.h"
#include "fast.h"
#include "core.h"

Descriptors::~Descriptors() {
    if (buffer != nullptr) delete [] buffer;
    buffer = nullptr;
    count = 0;
}

size_t Descriptors::sizeOf() {
    return count * DESCRIPTOR_LENGTH * sizeof(uint64_t);
}

BRISK::BRISK(uint32_t _octaves, float _briskScaleFactor) {
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

BRISK::~BRISK() {
    delete [] scales;
    delete [] sizes;
    delete [] patternPoints;
    delete shortPairs;
    delete longPairs;
}

bool BRISK::RoiPredicate(Bitmap &bitmap, KeyPoint &kp, uint32_t size) {
    return kp.x >= size && kp.y >= size && kp.x < bitmap.width - size && kp.y < bitmap.height - size;
}

void BRISK::subpixelRefine(Bitmap &bitmap, cl_mem buffer, Buffer<KeyPoint> &keypoints) {
    cl_mem kpsBuffer = OpenCL::createBuffer(keypoints.buffer, keypoints.size * sizeof(KeyPoint));

    cl_kernel kernel = OpenCL::createKernel("subpixel_refine");

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &kpsBuffer);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer);
    clSetKernelArg(kernel, 2, sizeof(int), &bitmap.width);
    clSetKernelArg(kernel, 3, sizeof(int), &bitmap.height);

    size_t global = keypoints.size;
    OpenCL::enqueueNDRangeKernel(kernel, 1, nullptr, &global, nullptr);
    OpenCL::readBuffer(kpsBuffer, keypoints.buffer, keypoints.size * sizeof(KeyPoint));

    clReleaseKernel(kernel);
    clReleaseMemObject(kpsBuffer);
}

Buffer<KeyPoint> *BRISK::detect(Bitmap &inputBitmap) {
    cl_mem buffer = inputBitmap.createCLBuffer();
    Bitmap8* bitmap = inputBitmap.toGray8(buffer);
    bitmap->CLAHE(buffer);

    size_t kpsLen = inputBitmap.bufferLength;
    Buffer<KeyPoint>* keyPoints = new Buffer<KeyPoint>(kpsLen);
    cl_mem kpsBuffer = clCreateBuffer(
            OpenCL::context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
            kpsLen * sizeof(KeyPoint), nullptr, nullptr
    );
    cl_mem kpsCounterBuffer = clCreateBuffer(
            OpenCL::context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
            sizeof(int32_t), nullptr, nullptr
    );

    float lastScaleFactor = 1.0f;
    cl_mem clonedBuffer;

    for (size_t i = 0; i < nOctaves - 1; i++) {
        if (i != 0) {
            if (!bitmap->canResize(OCTAVE_SCALE_FACTOR, MIN_OCTAVE_SIZE)) break;
            bitmap->resize(buffer, OCTAVE_SCALE_FACTOR);
            lastScaleFactor *= OCTAVE_SCALE_FACTOR;
        }
        GaussianKernel* gk = GaussianKernel::createKernel(lastScaleFactor);
        bitmap->blur(buffer, *gk);

        if (i == 0) {
            clonedBuffer = OpenCL::cloneBuffer(buffer, bitmap->bufferSize);
        }

        FAST::detect(*bitmap, buffer, *keyPoints, kpsBuffer, kpsCounterBuffer, FAST_PADDING, i, lastScaleFactor);
        delete gk;
    }

    clReleaseMemObject(kpsCounterBuffer);
    clReleaseMemObject(kpsBuffer);
    clReleaseMemObject(buffer);
    delete bitmap;

    std::sort(keyPoints->begin(), keyPoints->end(), [](const KeyPoint &a, const KeyPoint &b) {
        return a.response > b.response;
    });

    if (keyPoints->size > MAX_KEYPOINTS) {
        keyPoints->size = MAX_KEYPOINTS;
    }

    if (keyPoints->size <= Core::KEYPOINTS_PER_CHUNK * Core::CHUNKS_COUNT) {
        subpixelRefine(inputBitmap, clonedBuffer, *keyPoints);
        clReleaseMemObject(clonedBuffer);

        size_t fi = 0;
        for (size_t i = 0; i < keyPoints->size; i++) {
            if (RoiPredicate(inputBitmap, keyPoints->buffer[i], sizes[keyPoints->buffer[i].octave + 1])) {
                (*keyPoints)[fi] = (*keyPoints)[i];
                fi++;
            }
        }
        keyPoints->size = fi;
        keyPoints->shrink();

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

    subpixelRefine(inputBitmap, clonedBuffer, *keyPoints);
    clReleaseMemObject(clonedBuffer);

    fi = 0;
    for (size_t i = 0; i < keyPoints->size; i++) {
        if (RoiPredicate(inputBitmap, keyPoints->buffer[i], sizes[keyPoints->buffer[i].octave + 1])) {
            (*keyPoints)[fi] = (*keyPoints)[i];
            fi++;
        }
    }

    keyPoints->size = fi;
    keyPoints->shrink();

    return keyPoints;
}

Descriptors *BRISK::compute(Bitmap &inputBitmap, Buffer<KeyPoint> &keyPoints) {
    cl_mem buffer = inputBitmap.createCLBuffer();
    Bitmap8* bitmap = inputBitmap.toGray8WithBufferReading(buffer);

    int32_t* integral = bitmap->integral();
    size_t integralSize = (bitmap->width + 1) * (bitmap->height + 1) * sizeof(int32_t);
    cl_mem integralBuffer = clCreateBuffer(
            OpenCL::context, CL_MEM_HOST_NO_ACCESS | CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            integralSize, integral, nullptr
    );

    cl_mem kpBuffer = clCreateBuffer(
            OpenCL::context, CL_MEM_HOST_NO_ACCESS | CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            keyPoints.size * sizeof(KeyPoint), keyPoints.buffer, nullptr
    );

    Descriptors* descriptors = new Descriptors();
    descriptors->count = keyPoints.size;
    descriptors->buffer = new uint64_t[descriptors->count * Descriptors::DESCRIPTOR_LENGTH]();
    size_t descriptorsSize = descriptors->count * Descriptors::DESCRIPTOR_LENGTH * sizeof(uint64_t);
    cl_mem descBuffer = OpenCL::createBuffer(descriptors->buffer, descriptorsSize);

    size_t patternSize = nPoints * nRotations * nOctaves * sizeof(BriskPatternPoint);
    cl_mem patternBuffer = clCreateBuffer(
            OpenCL::context, CL_MEM_HOST_NO_ACCESS | CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            patternSize, patternPoints, nullptr
    );

    cl_mem sp = clCreateBuffer(
            OpenCL::context, CL_MEM_HOST_NO_ACCESS | CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            shortPairs->size * sizeof(BriskShortPair), shortPairs->buffer, nullptr
    );
    cl_mem lp = clCreateBuffer(
            OpenCL::context, CL_MEM_HOST_NO_ACCESS | CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            longPairs->size * sizeof(BriskLongPair), longPairs->buffer, nullptr
    );

    cl_kernel kernel = OpenCL::createKernel("brisk");

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer);
    clSetKernelArg(kernel, 1, sizeof(uint32_t), &bitmap->width);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &integralBuffer);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &kpBuffer);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), &descBuffer);
    clSetKernelArg(kernel, 5, sizeof(cl_mem), &patternBuffer);
    clSetKernelArg(kernel, 6, sizeof(cl_mem), &sp);
    clSetKernelArg(kernel, 7, sizeof(cl_mem), &lp);

    size_t global = keyPoints.size;
    OpenCL::enqueueNDRangeKernel(kernel, 1, nullptr, &global, nullptr);
    OpenCL::readBuffer(descBuffer, descriptors->buffer, descriptorsSize);

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
