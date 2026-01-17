//
// Created by sedv2 on 10.01.2026.
//

#include "imageio.h"
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept>
#include "raw-i.h"
#include "jpeg-io.h"
#include "png-io.h"
#include "tiff-io.h"

namespace ImageIO {
    enum class Format {
        UNDEFINED,
        JPG,
        PNG,
        RAW,
        TIFF,
    };

    uint8_t* readFile(int fd, size_t &size) {
        struct stat fileStat;
        if (fstat(fd, &fileStat) == -1) {
            return nullptr;
        }

        size = fileStat.st_size;
        if (size <= 0) {
            return nullptr;
        }

        uint8_t* buffer = new uint8_t[size];

        ssize_t bytesRead = read(fd, buffer, size);
        if (bytesRead <= 0) {
            delete[] buffer;
            return nullptr;
        }

        return buffer;
    }

    static Format getFormat(uint8_t* buffer) {
        if (buffer[0] == 0xFF && buffer[1] == 0xD8 && buffer[2] == 0xFF) {
            return Format::JPG;
        }

        if (buffer[0] == 0x89 && buffer[1] == 0x50 && buffer[2] == 0x4E && buffer[3] == 0x47 &&
            buffer[4] == 0x0D && buffer[5] == 0x0A && buffer[6] == 0x1A && buffer[7] == 0x0A) {
            return Format::PNG;
        }

        if (isRAW(buffer, 8)) {
            return Format::RAW;
        }

        // Little Endian (49 49 2A 00)
        if (buffer[0] == 0x49 && buffer[1] == 0x49 && buffer[2] == 0x2A && buffer[3] == 0x00) {
            return Format::TIFF;
        }
        // Big Endian (4D 4D 00 2A)
        if (buffer[0] == 0x4D && buffer[1] == 0x4D && buffer[2] == 0x00 && buffer[3] == 0x2A) {
            return Format::TIFF;
        }

        return Format::UNDEFINED;
    }

    BitmapPtr *open(int fd, ColorSpace colorSpace, Depth depth) {
        if (colorSpace != ColorSpace::RGB && colorSpace != ColorSpace::RGBA) {
            throw std::runtime_error("Invalid input color space");
        }

        size_t size = 0;
        uint8_t *buffer = readFile(fd, size);

        if (size < 8 || buffer == nullptr) {
            delete [] buffer;
            throw std::runtime_error("Invalid input file");
        }

        Format format = getFormat(buffer);

        BitmapPtr* bitmapPtr = nullptr;

        try {
            if (format == Format::JPG) {
                bitmapPtr = loadJPEG(buffer, size, colorSpace, depth);
            } else if (format == Format::PNG) {
                bitmapPtr = loadPNG(buffer, size, colorSpace, depth);
            } else if (format == Format::TIFF) {
                bitmapPtr = loadTIFF(fd, colorSpace, depth);
            } else if (format == Format::RAW) {
                bitmapPtr = loadRAW(buffer, size, colorSpace, depth);
            }
        } catch (std::exception &e) {
            delete [] buffer;
            throw std::runtime_error("File read failed");
        }

        delete [] buffer;

        if (bitmapPtr == nullptr) {
            throw std::runtime_error("Unsupported file format");
        }

        return bitmapPtr;
    }

    void save(int fd, Bitmap &bitmap, SaveProperties props) {
        switch (props.outputFormat) {
            case OutputFormat::JPEG:
                saveJPEG(fd, bitmap, props);
                break;
            case OutputFormat::PNG:
                savePNG(fd, bitmap, props);
                break;
            default:
                saveTIFF(fd, bitmap, props);
                break;
        }
    }
}


