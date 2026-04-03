//
// Created by sedv2 on 17.01.2026.
//

#include "utils.h"
#include <random>

namespace {
    void drawCircle(Bitmap &bmp, int x, int y, int r, int g, int b) {
        const int offset_points[42] = {
                -4, 0, -4, -1, -3, -2, -2, -3, -1, -4,
                0, -4, 1, -4, 2, -3, 3, -2, 4, -1,
                4, 0, 4, 1, 3, 2, 2, 3, 1, 4, 0, 4,
                -1, 4, -2, 3, -3, 2, -4, 1, 0, 0
        };

        for (size_t i = 0; i < 42; i+=2) {
            size_t yy = y + offset_points[i];
            size_t xx = x + offset_points[i+1];
            size_t index = (yy * bmp.width + xx) * 3;
            bmp.buffer[index + 0] = r;
            bmp.buffer[index + 1] = g;
            bmp.buffer[index + 2] = b;
        }
    }

    int getRandomNumber(int min, int max) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(min, max);

        return distrib(gen);
    }

    void drawMatches(Bitmap &bmp1,
                     Bitmap &bmp2,
                     Buffer<Matching::Match> &matches,
                     Buffer<KeyPoint> &keyPoints1,
                     Buffer<KeyPoint> &keyPoints2,
                     uint32_t index
    ) {
        std::string name = std::string("match_") + std::to_string(index) + ".jpg";
        int fd = JNIHelper::getInstance()->createImageFile(name.c_str());

        uint32_t width = bmp1.width + bmp2.width;
        uint32_t height = bmp1.height > bmp2.height ? bmp1.height : bmp2.height;
        size_t bufferSize = width * height * 3;
        auto buffer = new uint8_t[bufferSize];

        for (size_t row = 0; row < height; row++) {
            if (row < bmp1.height) {
                memcpy(
                        buffer + row * width * 3,
                        bmp1.buffer + row * bmp1.width * 3,
                        bmp1.width * 3
                );
            }
            if (row < bmp2.height) {
                memcpy(
                        buffer + (row * width + bmp1.width) * 3,
                        bmp2.buffer + row * bmp2.width * 3,
                        bmp2.width * 3
                );
            }
        }

        Bitmap bmp { width, height, buffer, bmp1.depth, bmp1.colorModel, bmp1.colorSpace };

        for (const Matching::Match &match : matches) {
            const KeyPoint &kp1 = keyPoints1[match.index1];
            const KeyPoint &kp2 = keyPoints2[match.index2];

            int r = getRandomNumber(50, 255);
            int g = getRandomNumber(50, 255);
            int b = getRandomNumber(50, 255);

            float dx = std::abs(kp2.x - kp1.x + bmp1.width);
            float dy = std::abs(kp2.y - kp1.y);
            float x = kp1.x;
            float y = kp1.y;
            float max = std::max(dx, dy);
            float step = std::min(dx, dy) / max;
            bool d = dx > dy;
            int kx = kp2.x - kp1.x + bmp1.width > 0 ? 1 : -1;
            int ky = kp2.y - kp1.y > 0 ? 1 : -1;

            drawCircle(bmp, x, y, r, g, b);

            for (size_t j = 0; j < max; j++) {
                int bufferIndex = (((int)y) * width + ((int)x)) * 3;
                bmp.buffer[bufferIndex + 0] = r;
                bmp.buffer[bufferIndex + 1] = g;
                bmp.buffer[bufferIndex + 2] = b;

                if (d) {
                    x+=1*kx;
                    y+=step * ky;
                }
                else {
                    x+=step * kx;
                    y+=1*ky;
                }
            }

            drawCircle(bmp, x, y, r, g, b);
        }

        SaveProperties props {};
        props.outputFormat = OutputFormat::JPEG;
        save(fd, bmp, props);
    }
}

void Utils::drawKeyPoints(Bitmap &bmp, Buffer<KeyPoint> &kps) {
    const char *fileName = "keyPoints.jpg";
    int fd = JNIHelper::getInstance()->createImageFile(fileName);

    auto bitmap8 = bmp.convertDepth(Depth::U8);

    for (KeyPoint &kp : kps) {
        drawCircle(bitmap8, kp.x, kp.y, 0, 255, 0);
    }
    SaveProperties props {};
    props.outputFormat = OutputFormat::JPEG;
    save(fd, bitmap8, props);
}

void Utils::drawAllMatches(Buffer<Buffer<Matching::Match>> &matches, uint32_t bestIndex) {
    size_t index = 0;
    Bitmap bmp2_8 = Core::sources->buffer[bestIndex]->bitmapPtr->read();
    bmp2_8 = bmp2_8.convertDepth(Depth::U8);
    for (size_t i = 0; i < Core::sources->size; i++) {
        if (i == bestIndex) continue;

        Bitmap bmp1_8 = Core::sources->buffer[i]->bitmapPtr->read();
        bmp1_8 = bmp1_8.convertDepth(Depth::U8);
        drawMatches(
                bmp1_8,
                bmp2_8,
                matches[index],
                *Core::sources->buffer[i]->keyPoints,
                *Core::sources->buffer[bestIndex]->keyPoints,
                index
        );
        index++;

        JNIHelper::getInstance()->writeMessageToLog(false, "Drawing matches with image %zd completed", i);
    }
    JNIHelper::getInstance()->writeMessageToLog(false, "Drawing completed\n");
}
