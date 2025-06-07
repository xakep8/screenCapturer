#ifndef IMAGE_UTILS_H
#define IMAGE_UTILS_H

#include <cstdint>
#include <vector>
#include <string>

namespace image_utils
{
    bool writeJPEG(const std::string &filename, const uint8_t *rgb_buffer, int width, int height, int quality);

    bool convertARGBToRGB(const uint8_t *argb_buffer, int width, int height, std::vector<uint8_t> &rgb_buffer);
}
#endif // IMAGE_UTILS_H