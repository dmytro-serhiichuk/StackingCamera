//
// Created by sedv2 on 02.04.2026.
//

#include "profiles-manager.h"
#include "profiles/adobe_rgb_profile_data.h"
#include "profiles/pro_photo_profile_data.h"

namespace ImageIO {
    cmsHPROFILE cloneProfile(cmsHPROFILE srcProfile) {
        cmsUInt32Number size = 0;
        cmsSaveProfileToMem(srcProfile, nullptr, &size);
        auto buffer = new uint8_t[size];
        cmsSaveProfileToMem(srcProfile, buffer, &size);
        cmsHPROFILE dst = cmsOpenProfileFromMem(buffer, size);
        delete [] buffer;
        return dst;
    }

    void writeProfileToMem(cmsHPROFILE profile, uint8_t *&icc, uint32_t &iccSize) {
        cmsSaveProfileToMem(profile, nullptr, &iccSize);
        icc = new uint8_t[iccSize]();
        cmsSaveProfileToMem(profile, icc, &iccSize);
    }

    cmsUInt32Number buildLcmsType(ColorModel colorModel, Depth depth) {
        cmsUInt32Number colorSpaceFlag = 0;
        cmsUInt32Number channelsCount = 0;

        auto channels = getSamplesPerPixel(colorModel);
        auto bps = (uint32_t)depth;

        switch (colorModel) {
            case ColorModel::RGB:
                colorSpaceFlag = PT_RGB;
                channelsCount = 3;
                break;
            case ColorModel::XYZ:
                colorSpaceFlag = PT_XYZ;
                channelsCount = 3;
                break;
            default: // gray
                colorSpaceFlag = PT_GRAY;
                channelsCount = 1;
                break;
        }

        return (COLORSPACE_SH(colorSpaceFlag) |
                CHANNELS_SH(channelsCount) |
                BYTES_SH((cmsUInt32Number)bps) |
                FLOAT_SH(0) | EXTRA_SH(0));
    }

    cmsHPROFILE createProfileFromColorSpace(ColorSpace colorSpace) {
        if (colorSpace == ColorSpace::sRGB) {
            return cmsCreate_sRGBProfile();
        } else if (colorSpace == ColorSpace::Linear_sRGB) {
            cmsToneCurve* curve = cmsBuildGamma(nullptr, 1.0);
            cmsCIExyY wp = { 0.3127, 0.3290, 1.0 };
            cmsCIExyYTRIPLE primaries = {
                    { 0.6400, 0.3300, 1.0 },
                    { 0.3000, 0.6000, 1.0 },
                    { 0.1500, 0.0600, 1.0 }
            };
            cmsToneCurve* curves[3] = { curve, curve, curve };
            cmsHPROFILE h = cmsCreateRGBProfile(&wp, &primaries, curves);
            cmsFreeToneCurve(curve);
            return h;
        } else if (colorSpace == ColorSpace::AdobeRGB) {
            return cmsOpenProfileFromMem(AdobeRGB1998_icc, AdobeRGB1998_icc_len);
        } else { // ProPhoto
            return cmsOpenProfileFromMem(ISO22028_2_ROMM_RGB_icc, ISO22028_2_ROMM_RGB_icc_len);
        }
    }

    cmsHPROFILE buildXYZ_d65() {
        cmsHPROFILE hProfile = cmsCreateProfilePlaceholder(nullptr);
        cmsSetDeviceClass(hProfile, cmsSigColorSpaceClass);
        cmsSetColorSpace(hProfile, cmsSigXYZData);
        cmsSetPCS(hProfile, cmsSigXYZData);

        cmsCIExyY wpxyY;
        cmsWhitePointFromTemp(&wpxyY, 6504.0);
        cmsCIEXYZ wpXYZ;
        cmsxyY2XYZ(&wpXYZ, &wpxyY);
        cmsWriteTag(hProfile, cmsSigMediaWhitePointTag, &wpXYZ);

        {
            const cmsFloat64Number kBradford_D65_to_D50[9] = {
                    1.0478112,  0.0228866, -0.0501270,
                    0.0295424,  0.9904844, -0.0170491,
                    -0.0092345,  0.0150436,  0.7521316
            };

            cmsPipeline* lut = cmsPipelineAlloc(nullptr, 3, 3);
            cmsStage* mat = cmsStageAllocMatrix(nullptr, 3, 3, kBradford_D65_to_D50, nullptr);
            cmsPipelineInsertStage(lut, cmsAT_END, mat);
            cmsWriteTag(hProfile, cmsSigAToB0Tag, lut);
            cmsPipelineFree(lut);
        }

        {
            const cmsFloat64Number kBradford_D50_to_D65[9] = {
                    0.9554734, -0.0230531,  0.0631633,
                    -0.0282525,  1.0099416,  0.0210369,
                    0.0123043, -0.0205345,  1.3303259
            };

            cmsPipeline* lut = cmsPipelineAlloc(nullptr, 3, 3);
            cmsStage* mat = cmsStageAllocMatrix(nullptr, 3, 3, kBradford_D50_to_D65, nullptr);
            cmsPipelineInsertStage(lut, cmsAT_END, mat);
            cmsWriteTag(hProfile, cmsSigBToA0Tag, lut);
            cmsPipelineFree(lut);
        }

        return hProfile;
    }

    std::vector<uint8_t> getProfileFromColorSpace(ColorSpace colorSpace) {
        auto p = createProfileFromColorSpace(colorSpace);
        cmsUInt32Number size = 0;
        cmsSaveProfileToMem(p, nullptr, &size);
        std::vector<uint8_t> icc(size);
        cmsSaveProfileToMem(p, icc.data(), &size);
        return icc;
    }
}
