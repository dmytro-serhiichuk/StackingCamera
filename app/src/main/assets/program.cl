#define PI 3.14159265358979323846f

__kernel void from_rgb16_to_gray8(
    __global const ushort* input_image,
    __global uchar* output_image,
    const int width,
    const int channels
) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    int out_index = y * width + x;
    int in_index = out_index * channels;

    float r = (float)input_image[in_index];
    float g = (float)input_image[in_index + 1];
    float b = (float)input_image[in_index + 2];

    ushort usVal = (ushort)(0.299f * r + 0.587f * g + 0.114f * b);
    uchar val = (uchar)(usVal >> 8);
    output_image[out_index] = val;
}

__kernel void from_rgb8_to_gray8(
    __global const uchar* input_image,
    __global uchar* output_image, 
    const int width,
    const int channels
) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    int out_index = y * width + x;
    int in_index = out_index * channels;

    float r = (float)input_image[in_index];
    float g = (float)input_image[in_index + 1];
    float b = (float)input_image[in_index + 2];

    output_image[out_index] = (uchar)(0.299f * r + 0.587f * g + 0.114f * b);
}

__kernel void clahe_make_lut(
    __global uchar* image,
    uint width,
    uint height,
    __global uint* map,    // tileCount x tileCount x 256
    uint tileCount,         // def. 32
    ulong clipLimit,        // def. 4.0f => знайдено наперед: clipLimit * (tileWidth * tileHeight) / binsCount -- лише для меншого регіона
    uint tileWidth,         // width / tileCount (лише менший регіон, більший знаходиться вже в ядрі)
    uint tileHeight         // height / tileCount
)
{
    const int BINS_COUNT = 256;

    int tid = get_local_id(0);
    int groupSize = get_local_size(0);

    int totalTileIndex = get_group_id(0);
    int tileX = totalTileIndex % tileCount;
    int tileY = totalTileIndex / tileCount;

    __local uint local_hist[BINS_COUNT];

    for (int i = tid; i < BINS_COUNT; i+=groupSize) {
        local_hist[i] = 0;
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    int startX = tileX * tileWidth;
    int startY = tileY * tileHeight;
    int actualTileW = min(startX + tileWidth,  width) - startX;
    int actualTileH = min(startY + tileHeight, height) - startY;

    // Filling the local map
    int totalPixels = actualTileW * actualTileH;
    int imgBaseOffset = startY * width + startX;

    for (int i = tid; i < totalPixels; i += groupSize) {
        int py = i / actualTileW;
        int px = i % actualTileW;

        int globalIdx = imgBaseOffset + py * width + px;
        uchar val = image[globalIdx];
        atomic_inc(&local_hist[val]);
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    if (tid == 0) {
        // clip histogram
        // calculating total excess
        ulong excessCount = 0;
        for (int i = 0; i < BINS_COUNT; i++) {
            long excess = local_hist[i] - clipLimit;
            if (excess > 0) excessCount += excess;
        }

        // clip and redistribute excess pixels
        ulong increment = excessCount / BINS_COUNT;
        ulong upper = clipLimit - increment;
        for (int i = 0; i < BINS_COUNT; i++) {
            if (local_hist[i] > clipLimit) local_hist[i] = clipLimit;
            else {
                if (local_hist[i] > upper) {
                    excessCount -= local_hist[i] - upper;
                    local_hist[i] = clipLimit;
                }
                else {
                    excessCount -= increment;
                    local_hist[i] += increment;
                }
            }
        }
        // Redistribute remaining excess
        while (excessCount > 0) {
            for (int i = 0; excessCount > 0 && i < BINS_COUNT; i++) {
                ulong stepSize = BINS_COUNT / excessCount;
                if (stepSize < 1) stepSize = 1;
                    for (int j = i; excessCount > 0 && j < BINS_COUNT; j+=stepSize) {
                    if (local_hist[j] < clipLimit) {
                        local_hist[j]++;
                        excessCount--;
                    }
                }
            }
        }

        // map hist
        const float scale = (float)(BINS_COUNT - 1) / (actualTileW * actualTileH);
        ulong sum = 0;
        for (int i = 0; i < BINS_COUNT; i++) {
            sum += local_hist[i];
            ulong val = (ulong)(sum * scale);
            if (val > 255) val = 255;
            local_hist[i] = (uint)val;
        }
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    // copy from local hist to global
    __global uint *global_hist_ptr = map + totalTileIndex * BINS_COUNT;
    for (int i = tid; i < BINS_COUNT; i+=groupSize) {
        global_hist_ptr[i] = local_hist[i];
    }
}

__kernel void clahe_interpolate(
    __global uchar *image,
    uint width,
    uint height,
    __global uint *map,
    int tileCount,
    uint tileWidth,
    uint tileHeight)
{
    const int BINS_COUNT = 256;

    int x = get_global_id(0);
    int y = get_global_id(1);

    int tileX = min(x / tileWidth, (uint)tileCount - 1);
    int tileY = min(y / tileHeight, (uint)tileCount - 1);

    int startX = tileX * tileCount;
    int startY = tileY * tileCount;
    int actualTileW = min(startX + tileWidth,  width) - startX;
    int actualTileH = min(startY + tileHeight, height) - startY;

    float posX = (float)x / actualTileW - 0.5f;
    float posY = (float)y / actualTileH - 0.5f;

    int tileXL = clamp((int)floor(posX), 0, tileCount - 1);
    int tileYU = clamp((int)floor(posY), 0, tileCount - 1);
    int tileXR = clamp(tileXL + 1, 0, tileCount - 1);
    int tileYD = clamp(tileYU + 1, 0, tileCount - 1);

    float dx = posX - tileXL;
    float dy = posY - tileYU;

    int iUL = tileYU * tileCount + tileXL;
    int iUR = tileYU * tileCount + tileXR;
    int iDL = tileYD * tileCount + tileXL;
    int iDR = tileYD * tileCount + tileXR;

    __global const uint *UL = map + iUL * BINS_COUNT;
    __global const uint *UR = map + iUR * BINS_COUNT;
    __global const uint *DL = map + iDL * BINS_COUNT;
    __global const uint *DR = map + iDR * BINS_COUNT;

    uchar pixel = image[y * width + x];

    float valUL = (float)UL[pixel];
    float valUR = (float)UR[pixel];
    float valDL = (float)DL[pixel];
    float valDR = (float)DR[pixel];

    float newVal = (1.0f - dx) * (1.0f - dy) * valUL +
                   dx * (1.0f - dy) * valUR +
                   (1.0f - dx) * dy * valDL +
                   dx * dy * valDR;

    image[y * width + x] = (uchar)newVal;
}

__kernel void gaussian_blur_horizontal(
    __global const uchar* input_image,
    __global uchar* temp_image,
    int width, int height,
    __constant float* weights,
    const int radius
) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= width || y >= height) return;

    float pixel_sum = 0.0f;

    for (int k = -radius; k <= radius; k++) {
        int sample_x = clamp(x + k, 0, width - 1);

        float weight = weights[k + radius];

        uchar pixel_val = input_image[y * width + sample_x];

        pixel_sum += pixel_val * weight;
    }

    temp_image[y * width + x] = (uchar)(pixel_sum + 0.5f);
}

__kernel void gaussian_blur_vertical(
    __global const uchar* temp_image,
    __global uchar* output_image,
    int width, int height,
    __constant float* weights,
    const int radius
) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= width || y >= height) return;

    float pixel_sum = 0.0f;

    for (int k = -radius; k <= radius; k++) {
        int sample_y = clamp(y + k, 0, height - 1);

        float weight = weights[k + radius];

        uchar pixel_val = temp_image[sample_y * width + x];

        pixel_sum += pixel_val * weight;
    }

    output_image[y * width + x] = (uchar)(pixel_sum + 0.5f);
}

__kernel void resize(
    __global const uchar* input_image, 
    __global uchar* output_image, 
    int input_width, int input_height, 
    int output_width, float scaleFactor
) {
    int x_out = get_global_id(0);
    int y_out = get_global_id(1);

    float x_in = ((float)x_out + 0.5f) / scaleFactor - 0.5f;
    float y_in = ((float)y_out + 0.5f) / scaleFactor - 0.5f;

    int x0 = clamp((int)floor(x_in), 0, input_width - 1);
    int y0 = clamp((int)floor(y_in), 0, input_height - 1);
    int x1 = clamp(x0 + 1, 0, input_width - 1);
    int y1 = clamp(y0 + 1, 0, input_height - 1);

    float dx = x_in - x0;
    float dy = y_in - y0;

    float top_left = input_image[y0 * input_width + x0];
    float top_right = input_image[y0 * input_width + x1];
    float bottom_left = input_image[y1 * input_width + x0];
    float bottom_right = input_image[y1 * input_width + x1];

    float top = top_left + dx * (top_right - top_left);
    float bottom = bottom_left + dx * (bottom_right - bottom_left);
    float value = top + dy * (bottom - top);

    output_image[y_out * output_width + x_out] = (uchar)clamp(value, 0.0f, 255.0f);
}

typedef struct {
    float x;
    float y;
    int response;
    uint octave;
} KeyPoint;

inline bool checkExtremum(
    __local const uchar* input_image,
    int intensity_central, 
    int x, int y, int width
) {
    const int ALLOWED_DIFF = 2;
    bool central_brighter = true;

    int point = input_image[(y - 1) * width + (x - 1)];
    central_brighter = intensity_central > point;

    int intensity_central_extended = central_brighter ? intensity_central + ALLOWED_DIFF : intensity_central - ALLOWED_DIFF;

    point = input_image[(y - 1) * width + (x)];
    if ((central_brighter && intensity_central_extended < point) || (!central_brighter && intensity_central_extended > point)) return false;
    point = input_image[(y - 1) * width + (x + 1)];
    if ((central_brighter && intensity_central_extended < point) || (!central_brighter && intensity_central_extended > point)) return false;
    point = input_image[(y) * width + (x - 1)];
    if ((central_brighter && intensity_central_extended < point) || (!central_brighter && intensity_central_extended > point)) return false;
    point = input_image[(y) * width + (x + 1)];
    if ((central_brighter && intensity_central_extended < point) || (!central_brighter && intensity_central_extended > point)) return false;
    point = input_image[(y + 1) * width + (x - 1)];
    if ((central_brighter && intensity_central_extended < point) || (!central_brighter && intensity_central_extended > point)) return false;
    point = input_image[(y + 1) * width + (x)];
    if ((central_brighter && intensity_central_extended < point) || (!central_brighter && intensity_central_extended > point)) return false;
    point = input_image[(y + 1) * width + (x + 1)];
    if ((central_brighter && intensity_central_extended < point) || (!central_brighter && intensity_central_extended > point)) return false;

    return true;
}

typedef struct {
    int count;
    int response;
    bool isDarker;
} FastInfo;

inline void fast_handle_point(
    __local const uchar* input_image,
    int intensity_central,
    __private FastInfo* fast_info,
    int threshold,
    int width,
    int x, int y,
    int N)
{
    int point = input_image[y * width + x];
    int diff = point - intensity_central;
    int abs_diff = abs(diff);
    if (fast_info->count < N) {
        if (abs_diff >= threshold) {
            if ((diff >= 0 && fast_info->isDarker) || (diff <= 0 && !fast_info->isDarker)) {
                fast_info->count++;
            }
            else {
                fast_info->isDarker = !fast_info->isDarker;
                fast_info->count = 1;
            }
        }
        else {
            fast_info->count = 0;
        }
    }
    fast_info->response += abs_diff;
}

typedef struct {
    int left;
    int right;
    int top;
    int bottom;
} Bounds;

__kernel void fast_9(
    __global const uchar* input_image,
    int width,
    int height,
    int threshold,
    __global KeyPoint* keypoints,
    __global int* points_count,
    uint octave,
    float scaleFactor,
    uint padding,
    __local uchar* map
) {
    __local int local_count;
    __local int group_start_index;

    int local_x = get_local_id(0);
    int local_y = get_local_id(1);

    const int local_w = get_local_size(0);
    const int local_h = get_local_size(1);

    const int local_size = local_w * local_h;

    int lid = local_x + local_y * local_w;
    if (lid == 0) {
        local_count = 0;
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    int g_x = get_group_id(0);
    int g_y = get_group_id(1);

    int x = get_global_id(0);
    int y = get_global_id(1);

    const int MAP_PADDING = 3;
    const int map_width = local_w + MAP_PADDING * 2;

    Bounds local_group_bounds = {
            local_w * g_x,
            local_w * g_x + local_w,
            local_h * g_y,
            local_h * g_y + local_h
    };
    Bounds raw_map_bounds = {
            local_group_bounds.left - MAP_PADDING,
            local_group_bounds.right + MAP_PADDING,
            local_group_bounds.top - MAP_PADDING,
            local_group_bounds.bottom + MAP_PADDING
    };
    Bounds valid_bounds = {
            padding,
            width - padding,
            padding,
            height - padding
    };
    Bounds map_bounds = {
            max(raw_map_bounds.left, valid_bounds.left),
            min(raw_map_bounds.right, valid_bounds.right),
            max(raw_map_bounds.top, valid_bounds.top),
            min(raw_map_bounds.bottom, valid_bounds.bottom)
    };

    int copy_w = 0, copy_h = 0;
    bool has_valid_data = (map_bounds.left < map_bounds.right && map_bounds.top < map_bounds.bottom);
    if (has_valid_data) {
        copy_w = map_bounds.right - map_bounds.left;
        copy_h = map_bounds.bottom - map_bounds.top;

        const int copy_n = copy_w * copy_h;

        const int local_offset_x = map_bounds.left - raw_map_bounds.left;
        const int local_offset_y = map_bounds.top - raw_map_bounds.top;

        for (int idx = lid; idx < copy_n; idx += local_size) {
            const int row = idx / copy_w;
            const int col = idx % copy_w;

            const int g_i = (map_bounds.left + col) + (map_bounds.top + row) * width;
            const int l_i = (local_offset_x + col) + (local_offset_y + row) * map_width;
            map[l_i] = input_image[g_i];
        }
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    const int N = 9;
    const int MAX_KEYPOINTS = width * height;
    bool found = false;
    int my_local_idx = -1;
    FastInfo fastInfo = { 0, 0, true };


    bool should_process = (x >= valid_bounds.left && x < valid_bounds.right &&
                           y >= valid_bounds.top && y < valid_bounds.bottom);

    if (should_process && has_valid_data) {
        const int map_x = local_x + MAP_PADDING;
        const int map_y = local_y + MAP_PADDING;

        int intensity_central = map[map_y * map_width + map_x];

        // Quick check
        if (!((abs(map[(map_y - 3) * map_width + map_x] - intensity_central) < threshold &&
              abs(map[(map_y + 3) * map_width + map_x] - intensity_central) < threshold) ||
             (abs(map[map_y * map_width + map_x - 3] - intensity_central) < threshold &&
              abs(map[map_y * map_width + map_x + 3] - intensity_central) < threshold)) &&
            checkExtremum(map, intensity_central, map_x, map_y, map_width)) {

            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x - 3, map_y - 0, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x - 3, map_y - 1, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x - 2, map_y - 2, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x - 1, map_y - 3, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x - 0, map_y - 3, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x + 1, map_y - 3, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x + 2, map_y - 2, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x + 3, map_y - 1, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x + 3, map_y - 0, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x + 3, map_y + 1, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x + 2, map_y + 2, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x + 1, map_y + 3, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x - 0, map_y + 3, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x - 1, map_y + 3, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x - 2, map_y + 2, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x - 3, map_y + 1, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x - 3, map_y + 0, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x - 3, map_y - 1, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x - 2, map_y - 2, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x - 1, map_y - 3, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x - 0, map_y - 3, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x + 1, map_y - 3, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x + 2, map_y - 2, N);
            fast_handle_point(map, intensity_central, &fastInfo, threshold, map_width, map_x + 3, map_y - 1, N);

            found = (fastInfo.count >= N);

            if (found) {
                my_local_idx = atomic_inc(&local_count);
            }
        }
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    if (lid == 0 && local_count > 0) {
        group_start_index = atomic_add(points_count, local_count);
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    if (found && my_local_idx >= 0) {
        int final_index = group_start_index + my_local_idx;
        if (final_index < MAX_KEYPOINTS) {
            keypoints[final_index].x        = ((float)x + 0.5f) / scaleFactor - 0.5f;
            keypoints[final_index].y        = ((float)y + 0.5f) / scaleFactor - 0.5f;
            keypoints[final_index].response = fastInfo.response;
            keypoints[final_index].octave   = octave;
        }
    }
}

__kernel void subpixel_refine(
    __global KeyPoint* keypoints,
    __global const uchar* image,
    const int width,
    const int height
) {
    const int stride = width;
    const int max_iterations = 100;
    const int gid = get_global_id(0);

    KeyPoint kp = keypoints[gid];
    float x = kp.x, y = kp.y;
    int ix = (int)floor(x), iy = (int)floor(y);

    int iteration = 0;
    bool moved = false;

    do {
        const int base = iy * stride + ix;

        const float I00  = image[base];
        const float I10  = image[base + 1];
        const float I_10 = image[base - 1];
        const float I01  = image[base + stride];
        const float I0_1 = image[base - stride];
        const float I11  = image[base + stride + 1];
        const float I_1_1 = image[base - stride - 1];
        const float I1_1 = image[base - stride + 1];
        const float I_11 = image[base + stride - 1];

        const float dx = 0.5f * (I10 - I_10);
        const float dy = 0.5f * (I01 - I0_1);

        const float dxx = I10 - 2.0f*I00 + I_10;
        const float dyy = I01 - 2.0f*I00 + I0_1;
        const float dxy = 0.25f * (I11 + I_1_1 - I_11 - I1_1);

        const float det = dxx * dyy - dxy * dxy;

        float offset_x = 0.0f, offset_y = 0.0f;
        moved = false;

        if (fabs(det) > 1e-6f) {
            const float inv_det = 1.0f / det;
            offset_x = -(dyy * dx - dxy * dy) * inv_det;
            offset_y = -(-dxy * dx + dxx * dy) * inv_det;

            if (fabs(offset_x) > 0.5f) {
                ix += (offset_x > 0) ? 1 : -1;
                moved = true;
            }
            if (fabs(offset_y) > 0.5f) {
                iy += (offset_y > 0) ? 1 : -1;
                moved = true;
            }

            if (moved && (ix < 1 || ix >= width-2 || iy < 1 || iy >= height-2)) {
                break;
            }
        } else {
            break;
        }
        
        iteration++;
    } while (moved && iteration < max_iterations);

    if (!moved && iteration < max_iterations) {
        const int base = iy * stride + ix;
        const float I00  = image[base];
        const float I10  = image[base + 1];
        const float I_10 = image[base - 1];
        const float I01  = image[base + stride];
        const float I0_1 = image[base - stride];

        const float dx = 0.5f * (I10 - I_10);
        const float dy = 0.5f * (I01 - I0_1);
        const float dxx = I10 - 2.0f*I00 + I_10;
        const float dyy = I01 - 2.0f*I00 + I0_1;
        const float dxy = 0.25f * (
            image[base + stride + 1] + image[base - stride - 1]
          - image[base + stride - 1] - image[base - stride + 1]
        );
        
        const float det = dxx * dyy - dxy * dxy;
        float offset_x = 0.0f, offset_y = 0.0f;
        
        if (fabs(det) > 1e-6f) {
            const float inv_det = 1.0f / det;
            offset_x = -(dyy * dx - dxy * dy) * inv_det;
            offset_y = -(-dxy * dx + dxx * dy) * inv_det;

            offset_x = clamp(offset_x, -0.5f, 0.5f);
            offset_y = clamp(offset_y, -0.5f, 0.5f);
        }
        
        kp.x = (float)ix + offset_x;
        kp.y = (float)iy + offset_y;
    } else {
        kp.x = (float)ix;
        kp.y = (float)iy;
    }
    
    keypoints[gid] = kp;
}

__kernel void find_closest_descriptors(
    __global const ulong* descriptors1,
    __global const ulong* descriptors2,
    __global int* closest_indices,
    __global int* min_distances,
    int num_descriptors1,
    int num_descriptors2)
{
    const int descriptor_size = 8;
    int idx1 = get_global_id(0);

    if (idx1 >= num_descriptors1) return;

    int min_distance = INT_MAX;
    int best_match = -1;

    ulong d1 = descriptors1[idx1 * descriptor_size + 0];
    ulong d2 = descriptors1[idx1 * descriptor_size + 1];
    ulong d3 = descriptors1[idx1 * descriptor_size + 2];
    ulong d4 = descriptors1[idx1 * descriptor_size + 3];
    ulong d5 = descriptors1[idx1 * descriptor_size + 4];
    ulong d6 = descriptors1[idx1 * descriptor_size + 5];
    ulong d7 = descriptors1[idx1 * descriptor_size + 6];
    ulong d8 = descriptors1[idx1 * descriptor_size + 7];

    for (int idx2 = 0; idx2 < num_descriptors2; idx2++) {
        int hamming_distance = 0;

        hamming_distance += popcount(d1 ^ descriptors2[idx2 * descriptor_size + 0]);
        hamming_distance += popcount(d2 ^ descriptors2[idx2 * descriptor_size + 1]);
        hamming_distance += popcount(d3 ^ descriptors2[idx2 * descriptor_size + 2]);
        hamming_distance += popcount(d4 ^ descriptors2[idx2 * descriptor_size + 3]);

        hamming_distance += popcount(d5 ^ descriptors2[idx2 * descriptor_size + 4]);
        hamming_distance += popcount(d6 ^ descriptors2[idx2 * descriptor_size + 5]);
        hamming_distance += popcount(d7 ^ descriptors2[idx2 * descriptor_size + 6]);
        hamming_distance += popcount(d8 ^ descriptors2[idx2 * descriptor_size + 7]);

        if (hamming_distance < min_distance) {
            min_distance = hamming_distance;
            best_match = idx2;
        }
    }

    closest_indices[idx1] = best_match;
    min_distances[idx1]   = min_distance;
}

// -------- BRISK --------

typedef struct {
    float x;
    float y;
    float sigma;
} BriskPatternPoint;

typedef struct {
    uint i;
    uint j;
} BriskShortPair;

typedef struct {
    uint i;
    uint j;
    int weighted_dx;
    int weighted_dy;
} BriskLongPair;

inline int smoothedIntensity(
    __global const uchar* image, 
    __global const int* integral,
    KeyPoint kp,
    BriskPatternPoint briskPoint,
    const uint imageWidth
) {
    // get the float position
    const float xf = briskPoint.x + kp.x;
    const float yf = briskPoint.y + kp.y;
    const int x = (int)(xf);
    const int y = (int)(yf);

    // get the sigma:
    const float sigma_half = briskPoint.sigma;
    const float area = 4.0f * sigma_half * sigma_half;

    int ret_val;

    // standart case (sigma_half cannot be less than 0.5 in this realisation)

    const int scaling = (int)(4194304.0f / area);
    const int scaling2 = (int)((float)(scaling) * area / 1024.0f);

    // calculate borders
    const float x_1 = xf - sigma_half;
    const float x1 = xf + sigma_half;
    const float y_1 = yf - sigma_half;
    const float y1 = yf + sigma_half;

    const int x_left = (int)(x_1 + 0.5f);
    const int y_top = (int)(y_1 + 0.5f);
    const int x_right = (int)(x1 + 0.5f);
    const int y_bottom = (int)(y1 + 0.5f);

    // overlap area - multiplication factors:
    const float r_x_1 = (float)(x_left) - x_1 + 0.5f;
    const float r_y_1 = (float)(y_top) - y_1 + 0.5f;
    const float r_x1 = x1 - (float)(x_right) + 0.5f;
    const float r_y1 = y1 - (float)(y_bottom) + 0.5f;
    const int dx = x_right - x_left - 1;
    const int dy = y_bottom - y_top - 1;
    const int A = (int)((r_x_1 * r_y_1) * scaling);
    const int B = (int)((r_x1 * r_y_1) * scaling);
    const int C = (int)((r_x1 * r_y1) * scaling);
    const int D = (int)((r_x_1 * r_y1) * scaling);
    const int r_x_1_i = (int)(r_x_1 * scaling);
    const int r_y_1_i = (int)(r_y_1 * scaling);
    const int r_x1_i = (int)(r_x1 * scaling);
    const int r_y1_i = (int)(r_y1 * scaling);

    const uint integralWidth = imageWidth + 1;

    if (dx + dy > 2)
    {
        // now the calculation:
        __global const uchar* ptr = image + x_left + imageWidth * y_top;
        // first the corners:
        ret_val = A * (int)(*ptr);
        ptr += dx + 1;
        ret_val += B * (int)(*ptr);
        ptr += dy * imageWidth + 1;
        ret_val += C * (int)(*ptr);
        ptr -= dx + 1;
        ret_val += D * (int)(*ptr);

        // next the edges:
        __global const int* ptr_integral = integral + x_left + integralWidth * y_top + 1;
        // find a simple path through the different surface corners
        const int tmp1 = (*ptr_integral);
        ptr_integral += dx;
        const int tmp2 = (*ptr_integral);
        ptr_integral += integralWidth;
        const int tmp3 = (*ptr_integral);
        ptr_integral++;
        const int tmp4 = (*ptr_integral);
        ptr_integral += dy * integralWidth;
        const int tmp5 = (*ptr_integral);
        ptr_integral--;
        const int tmp6 = (*ptr_integral);
        ptr_integral += integralWidth;
        const int tmp7 = (*ptr_integral);
        ptr_integral -= dx;
        const int tmp8 = (*ptr_integral);
        ptr_integral -= integralWidth;
        const int tmp9 = (*ptr_integral);
        ptr_integral--;
        const int tmp10 = (*ptr_integral);
        ptr_integral -= dy * integralWidth;
        const int tmp11 = (*ptr_integral);
        ptr_integral++;
        const int tmp12 = (*ptr_integral);

        // assign the weighted surface integrals:
        const int upper = (tmp3 - tmp2 + tmp1 - tmp12) * r_y_1_i;
        const int middle = (tmp6 - tmp3 + tmp12 - tmp9) * scaling;
        const int left = (tmp9 - tmp12 + tmp11 - tmp10) * r_x_1_i;
        const int right = (tmp5 - tmp4 + tmp3 - tmp6) * r_x1_i;
        const int bottom = (tmp7 - tmp6 + tmp9 - tmp8) * r_y1_i;

        return (ret_val + upper + middle + left + right + bottom + scaling2 / 2) / scaling2;
    }

    // now the calculation:
    __global const uchar* ptr = image + x_left + imageWidth * y_top;
    // first row:
    ret_val = A * (int)(*ptr);
    ptr++;
    __global const uchar* end1 = ptr + dx;
    for (; ptr < end1; ptr++)
    {
        ret_val += r_y_1_i * (int)(*ptr);
    }
    ret_val += B * (int)(*ptr);
    // middle ones:
    ptr += imageWidth - dx - 1;
    __global const uchar* end_j = ptr + dy * imageWidth;
    for (; ptr < end_j; ptr += imageWidth - dx - 1)
    {
        ret_val += r_x_1_i * (int)(*ptr);
        ptr++;
        __global const uchar* end2 = ptr + dx;
        for (; ptr < end2; ptr++)
        {
            ret_val += (int)(*ptr) * scaling;
        }
        ret_val += r_x1_i * (int)(*ptr);
    }
    // last row:
    ret_val += D * (int)(*ptr);
    ptr++;
    __global const uchar* end3 = ptr + dx;
    for (; ptr < end3; ptr++)
    {
        ret_val += r_y1_i * (int)(*ptr);
    }
    ret_val += C * (int)(*ptr);

    return (ret_val + scaling2 / 2) / scaling2;
}

__kernel void brisk(
    __global const uchar* image,
    const uint imageWidth,
    __global const int* integral,
    __global const KeyPoint* keypoints,
    __global ulong* descriptors,
    __global const BriskPatternPoint* pattern,
    __global const BriskShortPair* shortPairs,
    __global const BriskLongPair* longPairs
) {
    const uint nRotations = 1024;
    const uint nPoints = 60;
    const uint nShortPairs = 512;
    const uint nLongPairs = 870;

    int k = get_global_id(0);

    KeyPoint kp = keypoints[k];

    int _values[nPoints];

    for (uint i = 0; i < nPoints; i++) {
        _values[i] = smoothedIntensity(
            image, integral, kp,
            pattern[(kp.octave + 1) * nRotations * nPoints + i], 
            imageWidth);
    }

    int dir0 = 0;
    int dir1 = 0;

    int t1 = 0;
    int t2 = 0;

    for (uint iter = 0; iter < nLongPairs; iter++) {
        t1 = *(_values + longPairs[iter].i);
        t2 = *(_values + longPairs[iter].j);
        const int delta = t1 - t2;
        dir0 += delta * longPairs[iter].weighted_dx / 1024;
        dir1 += delta * longPairs[iter].weighted_dy / 1024;    
    }
    float deg = atan2((float)dir1, (float)dir0);

    int theta = (int)(nRotations * deg / (2 * PI) + 0.5f);
    if (theta < 0) theta += nRotations;
    else if (theta >= nRotations) theta -= nRotations;

    for (uint i = 0; i < nPoints; i++) {
        _values[i] = smoothedIntensity(
            image, integral, kp,
            pattern[(kp.octave + 1) * nRotations * nPoints + theta * nPoints + i], 
            imageWidth);
    }

    uint shifter = 0;
    int di = k * 8;
    for (uint iter = 0; iter < nShortPairs; iter++) {
        t1 = *(_values + shortPairs[iter].i);
        t2 = *(_values + shortPairs[iter].j);
        if (t1 > t2) {
            descriptors[di] |= 1 << shifter;
        }
        shifter++;
        if (shifter == 64) {
            shifter = 0;
            di++;
        }
    }
}

// ------------------------

__kernel void warp_perspective_16(
    __global const ushort* input_image,
    __global ushort* output_image,
    __constant float* H,
    const int input_width,
    const int input_height,
    const int output_width,
    const int output_height,
    const int channels
)
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= output_width || y >= output_height) return;

    float x_src = H[0] * x + H[1] * y + H[2];
    float y_src = H[3] * x + H[4] * y + H[5];
    float w = H[6] * x + H[7] * y + H[8];

    float inv_w = 1.0f / w;
    x_src *= inv_w;
    y_src *= inv_w;

    if (x_src >= 0 && x_src < input_width && y_src >= 0 && y_src < input_height) {
        int x0 = (int)x_src;
        int y0 = (int)y_src;

        int x1 = min(x0 + 1, input_width - 1);
        int y1 = min(y0 + 1, input_height - 1);

        float dx = x_src - x0;
        float dy = y_src - y0;

        for (int c = 0; c < 3; c++) {
            float p0 = input_image[(y0 * input_width + x0) * channels + c];
            float p1 = input_image[(y0 * input_width + x1) * channels + c];
            float p2 = input_image[(y1 * input_width + x0) * channels + c];
            float p3 = input_image[(y1 * input_width + x1) * channels + c];

            float value = (1 - dx) * (1 - dy) * p0 +
                          dx * (1 - dy) * p1 +
                          (1 - dx) * dy * p2 +
                          dx * dy * p3;
            output_image[(y * output_width + x) * channels + c] = (ushort)clamp(value, 0.0f, 65535.0f);
        }
    } else {
        output_image[(y * output_width + x) * channels]     = 65000;
        output_image[(y * output_width + x) * channels + 1] = 0;
        output_image[(y * output_width + x) * channels + 2] = 0;
    }
}

__kernel void warp_perspective_8(
    __global const uchar* input_image,
    __global uchar* output_image,
    __constant float* H,
    const int input_width,
    const int input_height,
    const int output_width,
    const int output_height,
    const int channels
)
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= output_width || y >= output_height) return;

    float x_src = H[0] * x + H[1] * y + H[2];
    float y_src = H[3] * x + H[4] * y + H[5];
    float w = H[6] * x + H[7] * y + H[8];

    float inv_w = 1.0f / w;
    x_src *= inv_w;
    y_src *= inv_w;

    if (x_src >= 0 && x_src < input_width && y_src >= 0 && y_src < input_height) {
        int x0 = (int)x_src;
        int y0 = (int)y_src;

        int x1 = min(x0 + 1, input_width - 1);
        int y1 = min(y0 + 1, input_height - 1);

        float dx = x_src - x0;
        float dy = y_src - y0;

        for (int c = 0; c < 3; c++) {
            float p0 = input_image[(y0 * input_width + x0) * channels + c];
            float p1 = input_image[(y0 * input_width + x1) * channels + c];
            float p2 = input_image[(y1 * input_width + x0) * channels + c];
            float p3 = input_image[(y1 * input_width + x1) * channels + c];

            float value = (1 - dx) * (1 - dy) * p0 +
                          dx * (1 - dy) * p1 +
                          (1 - dx) * dy * p2 +
                          dx * dy * p3;
            output_image[(y * output_width + x) * channels + c] = (uchar)clamp(value, 0.0f, 255.0f);
        }
    } else {
        output_image[(y * output_width + x) * channels]     = 255;
        output_image[(y * output_width + x) * channels + 1] = 0;
        output_image[(y * output_width + x) * channels + 2] = 0;
    }
}