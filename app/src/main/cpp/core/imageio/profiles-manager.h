//
// Created by sedv2 on 02.04.2026.
//

#ifndef STACKINGCAMERA_PROFILES_MANAGER_H
#define STACKINGCAMERA_PROFILES_MANAGER_H

#include "bitmap.h"
#include <vector>

namespace ImageIO {
    cmsHPROFILE cloneProfile(cmsHPROFILE srcProfile);
    void writeProfileToMem(cmsHPROFILE profile, uint8_t*& icc, uint32_t &iccSize);
    cmsHPROFILE buildXYZ_d65();
    cmsUInt32Number buildLcmsType(ColorModel colorModel, Depth depth);
    cmsHPROFILE createProfileFromColorSpace(ColorSpace colorSpace);
    std::vector<uint8_t> getProfileFromColorSpace(ColorSpace colorSpace);
}

#endif //STACKINGCAMERA_PROFILES_MANAGER_H
