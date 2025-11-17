//
// Created by sedv2 on 05.04.2025.
//

#include "utils.h"
#include "jpegio.h"
#include <random>

namespace Utils {
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

    void drawKeyPoints(Bitmap &bmp, Buffer<KeyPoint> &kps) {
        const char *fileName = "keyPoints.jpg";
        int fd = Core::jniHelper->createImageFile(fileName);

        for (KeyPoint &kp : kps) {
            drawCircle(bmp, kp.x, kp.y, 0, 65535, 0);
        }
        ImageIO::saveJPEG(fd, &bmp, 80);
    }

    int getRandomNumber(int min, int max) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(min, max);

        return distrib(gen);
    }

    void drawMatches(Bitmap &bmp1,
                     Bitmap &bmp2,
                     Buffer<Match> &matches,
                     Buffer<KeyPoint> &keyPoints1,
                     Buffer<KeyPoint> &keyPoints2,
                     uint32_t index
    ) {
        std::string name = std::string("match_") + std::to_string(index) + ".jpg";
        int fd = Core::jniHelper->createImageFile(name.c_str());

        int width = bmp1.width + bmp2.width;
        int height = bmp1.height > bmp2.height ? bmp1.height : bmp2.height;
        int bufferSize = width * height * 3;
        uint16_t *buffer = new uint16_t[bufferSize];

        for (size_t row = 0; row < height; row++) {
            if (row < bmp1.height) {
                memcpy(
                        buffer + row * width * 3,
                        bmp1.buffer + row * bmp1.width * 3,
                        bmp1.width * sizeof(uint16_t) * 3
                );
            }
            if (row < bmp2.height) {
                memcpy(
                        buffer + (row * width + bmp1.width) * 3,
                        bmp2.buffer + row * bmp2.width * 3,
                        bmp2.width * sizeof(uint16_t) * 3
                );
            }
        }

        Bitmap *bmp = new Bitmap{
                width,
                height,
                buffer
        };

        for (const Match &match : matches) {
            const KeyPoint &kp1 = keyPoints1[match.index1];
            const KeyPoint &kp2 = keyPoints2[match.index2];

            int r = getRandomNumber(10000, 65535);
            int g = getRandomNumber(10000, 65535);
            int b = getRandomNumber(10000, 65535);

            float dx = std::abs(kp2.x - kp1.x + bmp1.width);
            float dy = std::abs(kp2.y - kp1.y);
            float x = kp1.x;
            float y = kp1.y;
            float max = std::max(dx, dy);
            float step = std::min(dx, dy) / max;
            bool d = dx > dy;
            int kx = kp2.x - kp1.x + bmp1.width > 0 ? 1 : -1;
            int ky = kp2.y - kp1.y > 0 ? 1 : -1;

            drawCircle(*bmp, x, y, r, g, b);

            for (size_t j = 0; j < max; j++) {
                int bufferIndex = (((int)y) * width + ((int)x)) * 3;
                bmp->buffer[bufferIndex + 0] = r;
                bmp->buffer[bufferIndex + 1] = g;
                bmp->buffer[bufferIndex + 2] = b;

                if (d) {
                    x+=1*kx;
                    y+=step * ky;
                }
                else {
                    x+=step * kx;
                    y+=1*ky;
                }
            }

            drawCircle(*bmp, x, y, r, g, b);
        }

        // TODO:
        ImageIO::saveJPEG(fd, bmp, 80);
        delete bmp;
    }

    void drawAllMatches(Buffer<Buffer<Match>> &matches, uint32_t bestIndex) {
        Core::jniHelper->progressMessage = "Drawing Matches...";
        Core::jniHelper->setProgressMessage();

        size_t index = 0;
        Bitmap *bmp2 = Core::bitmaps->buffer[bestIndex]->read();
        for (size_t i = 0; i < Core::bitmaps->size; i++) {
            if (i == bestIndex) continue;
            Core::jniHelper->progressMessage = "Drawing Matches: " + std::to_string(index + 1) + "/" + std::to_string(matches.size);
            Core::jniHelper->setProgressMessage();

            Bitmap *bmp1 = Core::bitmaps->buffer[i]->read();
            drawMatches(*bmp1, *bmp2, matches[index], *Core::keyPoints->buffer[i], *Core::keyPoints->buffer[bestIndex], index);
            index++;
            delete bmp1;
        }
        delete bmp2;
    }
}