#include "imageUtils.h"
#include <iostream>
#include <X11/Xlib.h>
#include <fstream>
#include <filesystem>
#include <jpeglib.h>
#include <libyuv.h>
#include <X11/Xutil.h>

namespace image_utils
{
    bool writeJPEG(const std::string &filename, uint8_t *image_buffer,
                   int width, int height, int quality)
    {
        FILE *outfile = fopen(filename.c_str(), "wb");
        if (!outfile)
        {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return false;
        }

        jpeg_compress_struct cinfo;
        jpeg_error_mgr jerr;

        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);
        jpeg_stdio_dest(&cinfo, outfile);

        cinfo.image_width = width;
        cinfo.image_height = height;
        cinfo.input_components = 3; // RGB
        cinfo.in_color_space = JCS_RGB;

        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, quality, TRUE);

        jpeg_start_compress(&cinfo, TRUE);

        JSAMPROW row_pointer[1];
        int row_stride = width * 3; // 3 bytes per pixel for RGB

        while (cinfo.next_scanline < cinfo.image_height)
        {
            row_pointer[0] = &image_buffer[cinfo.next_scanline * row_stride];
            jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }

        jpeg_finish_compress(&cinfo);
        fclose(outfile);
        jpeg_destroy_compress(&cinfo);

        return true;
    }
    bool convertXImageToRGB(void *ximage_ptr, int width, int height,
                            std::vector<uint8_t> &rgb_buffer)
    {
        XImage *ximage = static_cast<XImage *>(ximage_ptr);
        if (!ximage)
            return false;

        // Convert XImage to ARGB buffer first
        std::vector<uint8_t> argb_buffer(width * height * 4);
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                unsigned long pixel = XGetPixel(ximage, x, y);

                int idx = (y * width + x) * 4;
                argb_buffer[idx + 0] = (pixel & 0x00FF0000) >> 16; // B
                argb_buffer[idx + 1] = (pixel & 0x0000FF00) >> 8;  // G
                argb_buffer[idx + 2] = (pixel & 0x000000FF);       // R
                argb_buffer[idx + 3] = 255;                        // A
            }
        }

        // Convert ARGB to RGB using libyuv
        rgb_buffer.resize(width * height * 3);
        int result = libyuv::ARGBToRGB24(
            argb_buffer.data(), width * 4,
            rgb_buffer.data(), width * 3,
            width, height);

        return result == 0;
    }
}