//
// Created by sedv2 on 10.01.2026.
//

#ifndef STACKINGCAMERA_BITMAP_H
#define STACKINGCAMERA_BITMAP_H

#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace ImageIO {
    enum class ColorSpace {
        Grayscale = 1,
        RGB = 3,
        RGBA = 4
    };

    enum class Depth {
        U8 = 1,
        U16 = 2
    };

    class Bitmap {
    public:
        uint8_t* buffer;
        uint32_t width;
        uint32_t height;
        size_t bufferLength;
        uint32_t stride;
        ColorSpace colorSpace;
        Depth depth;

        Bitmap(uint32_t w, uint32_t h, void* b, ColorSpace cs, Depth d) :
            width(w), height(h), buffer((uint8_t*)b), colorSpace(cs), depth(d)
        {
            bufferLength = width * height * (size_t)colorSpace;
            stride = width * (uint32_t)colorSpace;
        }
        Bitmap(const Bitmap& other) = delete;
        Bitmap& operator=(Bitmap&&) = default;

        ~Bitmap();

        Bitmap* copy() const;

        inline size_t sizeOfBuffer() const {
            return bufferLength * (size_t)depth;
        }

        inline Bitmap* convertColor(ColorSpace newColorSpace) const {
            return Bitmap::convertTo(depth, newColorSpace);
        }
        inline Bitmap* convertDepth(Depth newDepth) const {
            return Bitmap::convertTo(newDepth, colorSpace);
        }

        Bitmap* convertTo(Depth newDepth, ColorSpace newColorSpace) const;

    private:
        template <typename I, typename O>
        Bitmap* convert(Depth newDepth, ColorSpace newColorSpace) const {
            size_t newBufferLength = width * height * (size_t)newColorSpace;
            O* dst = new O[newBufferLength];
            I* src = (I*)buffer;

            if (colorSpace == newColorSpace) {
                for (size_t i = 0; i < bufferLength; i++) {
                    dst[i] = Bitmap::convertValueDepth<I, O>(src[i]);
                }
            }
            else if (newColorSpace == ColorSpace::RGB) /* to RGB */ {
                if (colorSpace == ColorSpace::Grayscale) /* from Grayscale */ {
                    for (size_t i = 0; i < bufferLength; i++) {
                        dst[i * 3]     = Bitmap::convertValueDepth<I, O>(src[i]);
                        dst[i * 3 + 1] = Bitmap::convertValueDepth<I, O>(src[i]);
                        dst[i * 3 + 2] = Bitmap::convertValueDepth<I, O>(src[i]);
                    }
                }
                else if (colorSpace == ColorSpace::RGBA) /* from RGBA */ {
                    for (size_t i = 0, newI = 0; i < bufferLength; i+=4, newI+=3) {
                        dst[newI]     = Bitmap::convertValueDepth<I, O>(src[i]);
                        dst[newI + 1] = Bitmap::convertValueDepth<I, O>(src[i + 1]);
                        dst[newI + 2] = Bitmap::convertValueDepth<I, O>(src[i + 2]);
                    }
                }
            }
            else if (newColorSpace == ColorSpace::Grayscale) /* to Grayscale */ {
                if (colorSpace == ColorSpace::RGB) /* from RGB */ {
                    for (size_t i = 0; i < newBufferLength; i++) {
                        I gray = static_cast<I>(0.299 * src[i * 3] + 0.587 * src[i * 3 + 1] + 0.114 * src[i * 3 + 2]);
                        dst[i] = Bitmap::convertValueDepth<I, O>(gray);
                    }
                }
                else if (colorSpace == ColorSpace::RGBA) /* from RGBA */ {
                    for (size_t i = 0; i < newBufferLength; i++) {
                        I gray = static_cast<I>(0.299 * src[i * 4] + 0.587 * src[i * 4 + 1] + 0.114 * src[i * 4 + 2]);
                        dst[i] = Bitmap::convertValueDepth<I, O>(gray);
                    }
                }
            }
            else if (newColorSpace == ColorSpace::RGBA) /* to RGBA */ {
                if (colorSpace == ColorSpace::RGB) /* from RGB */ {
                    for (size_t i = 0, newI = 0; i < bufferLength; i+=3, newI+=4) {
                        dst[newI]     = Bitmap::convertValueDepth<I, O>(src[i]);
                        dst[newI + 1] = Bitmap::convertValueDepth<I, O>(src[i + 1]);
                        dst[newI + 2] = Bitmap::convertValueDepth<I, O>(src[i + 2]);
                        dst[newI + 3] = Bitmap::convertValueDepth<uint8_t, O>(255);
                    }
                }
                else if (colorSpace == ColorSpace::Grayscale) /* from Grayscale */ {
                    for (size_t i = 0; i < bufferLength; i++) {
                        dst[i * 4]     = Bitmap::convertValueDepth<I, O>(src[i]);
                        dst[i * 4 + 1] = Bitmap::convertValueDepth<I, O>(src[i]);
                        dst[i * 4 + 2] = Bitmap::convertValueDepth<I, O>(src[i]);
                        dst[i * 4 + 3] = Bitmap::convertValueDepth<uint8_t, O>(255);
                    }
                }
            }

            return new Bitmap(width, height, dst, newColorSpace, newDepth);
        }

        template <typename I, typename O>
        static O convertValueDepth(I input) {
            if (std::is_same<I, O>::value) return input;

            if (std::is_same<I, uint8_t>::value && std::is_same<O, uint16_t>::value) {
                return (uint16_t)input * 257;
            }
            if (std::is_same<I, uint16_t>::value && std::is_same<O, uint8_t>::value) {
                return (uint8_t)((uint16_t)input >> 8);
            }

            throw std::invalid_argument("Unsupported types. Bitmap only supports U8 and U16 types");
        }
    };
}


#endif //STACKINGCAMERA_BITMAP_H
