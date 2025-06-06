#include "desktopCapturer.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>
#include <fstream>
#include <libyuv.h>
#include <jpeglib.h>
#include <filesystem>

namespace
{

    bool isWindowCapturable(Display *display, Window window)
    {
        XWindowAttributes attrs;
        if (XGetWindowAttributes(display, window, &attrs) == 0)
        {
            return false;
        }

        // Check if window is visible, mapped, and has reasonable size
        return (attrs.map_state == IsViewable &&
                attrs.width > 10 &&
                attrs.height > 10 &&
                attrs.c_class == InputOutput);
    }
    void getAllWindows(Display *display, Window window, std::vector<Window> &windows)
    {
        Window rootReturn, parentReturn;
        Window *childrenReturn;
        unsigned int nchildrenReturn;

        if (XQueryTree(display, window, &rootReturn, &parentReturn, &childrenReturn, &nchildrenReturn))
        {
            for (unsigned int i = 0; i < nchildrenReturn; i++)
            {
                bool isCapturable = isWindowCapturable(display, childrenReturn[i]);
                if (isCapturable)
                {
                    windows.push_back(childrenReturn[i]);
                }
                getAllWindows(display, childrenReturn[i], windows);
            }
            if (childrenReturn)
            {
                XFree(childrenReturn);
            }
        }
    }

    std::string getWindowClass(Display *display, Window window)
    {
        XClassHint class_hint;
        if (XGetClassHint(display, window, &class_hint))
        {
            std::string result = class_hint.res_class ? class_hint.res_class : "Unknown";
            if (class_hint.res_name)
                XFree(class_hint.res_name);
            if (class_hint.res_class)
                XFree(class_hint.res_class);
            return result;
        }
        return "Unknown";
    }

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
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;

        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, quality, TRUE);

        jpeg_start_compress(&cinfo, TRUE);

        JSAMPROW row_pointer[1];
        int row_stride = width * 3;

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
}

namespace screen_recorder
{
    DesktopCapture::DesktopCapture()
    {
        // Initialize the desktop capture functionality
        std::cout << "DesktopCapture initialized." << std::endl;
        mDisplay = std::unique_ptr<Display, DisplayDeleter>(XOpenDisplay(nullptr));
        if (!mDisplay)
        {
            std::cerr << "Failed to open X display." << std::endl;
            std::exit(EXIT_FAILURE);
        }
        mRootWindow = DefaultRootWindow(mDisplay.get());
        if (mRootWindow == None)
        {
            std::cerr << "Failed to get root window." << std::endl;
            std::exit(EXIT_FAILURE);
        }
        if (XGetWindowAttributes(mDisplay.get(), mRootWindow, &mWindowAttributes) == 0)
        {
            std::cerr << "Failed to get window attributes." << std::endl;
            std::exit(EXIT_FAILURE);
        }
        mScreenWidth = mWindowAttributes.width;
        mScreenHeight = mWindowAttributes.height;
        std::cout << "Screen dimensions: " << mScreenWidth << "x" << mScreenHeight << std::endl;
        getAllWindows(mDisplay.get(), mRootWindow, mCapturableWindows);
        std::cout << "\n=== All Windows ===" << std::endl;
        std::cout << "Total windows found: " << mCapturableWindows.size() << std::endl;

        std::cout << "\n=== Capturable Windows ===" << std::endl;
        int capturable_count = 0;

        for (auto windowId : mCapturableWindows)
        {
            XWindowAttributes attrs;
            if (XGetWindowAttributes(mDisplay.get(), windowId, &attrs) == 0)
            {
                continue;
            }

            char *windowName = nullptr;
            XFetchName(mDisplay.get(), windowId, &windowName);

            std::string windowClass = getWindowClass(mDisplay.get(), windowId);

            mCapturableWindows.push_back(windowId);
            capturable_count++;
            std::cout << "\n--- Window #" << capturable_count << " ---" << std::endl;
            std::cout << "ID: " << windowId << std::endl;
            std::cout << "Name: " << (windowName ? windowName : "Unnamed") << std::endl;
            std::cout << "Class: " << windowClass << std::endl;
            std::cout << "Position: " << attrs.x << "," << attrs.y << std::endl;
            std::cout << "Size: " << attrs.width << "x" << attrs.height << std::endl;
            std::cout << "Map State: " << (attrs.map_state == IsViewable ? "Viewable" : attrs.map_state == IsUnmapped ? "Unmapped"
                                                                                                                      : "Unviewable")
                      << std::endl;
            std::cout << "Border Width: " << attrs.border_width << std::endl;
            std::cout << "Depth: " << attrs.depth << " bits" << std::endl;
            std::cout << "Visual ID: " << attrs.visual->visualid << std::endl;
            std::cout << "Backing Store: " << attrs.backing_store << std::endl;
            std::cout << "Class: " << (attrs.c_class == InputOutput ? "InputOutput" : "InputOnly") << std::endl;

            if (windowName)
            {
                XFree(windowName);
            }
        }

        std::cout << "\n=== Summary ===" << std::endl;
        std::cout << "Total windows: " << mCapturableWindows.size() << std::endl;
        std::cout << "Capturable windows: " << capturable_count << std::endl;
        startCapture(mCapturableWindows[0]); // Start capturing the first capturable window as an example
    }
    DesktopCapture::~DesktopCapture()
    {
        // Clean up resources if necessary
        std::cout << "DesktopCapture destroyed." << std::endl;
    }
    void DesktopCapture::startCapture(auto windowId)
    {
        // Start capturing the specified window
        std::cout << "Starting capture for window ID: " << windowId << std::endl;
        XWindowAttributes attrs;
        if (XGetWindowAttributes(mDisplay.get(), windowId, &attrs) == 0)
        {
            std::cerr << "Failed to get attributes for window ID: " << windowId << std::endl;
            return;
        }
        mScreenWidth = attrs.width;
        mScreenHeight = attrs.height;
        std::cout << "Capturing window: " << windowId << " with size: " << mScreenWidth << "x" << mScreenHeight << std::endl;
        std::unique_ptr<XImage, ImageDeleter> xImage(
            XGetImage(mDisplay.get(), windowId, 0, 0, mScreenWidth, mScreenHeight, AllPlanes, ZPixmap));
        if (!xImage)
        {
            std::cerr << "Failed to capture image for window ID: " << windowId << std::endl;
            return;
        }
        std::vector<uint8_t> argb_buffer(mScreenWidth * mScreenHeight * 4);
        for (int y = 0; y < mScreenHeight; y++)
        {
            for (int x = 0; x < mScreenWidth; x++)
            {
                unsigned long pixel = XGetPixel(xImage.get(), x, y);

                int idx = (y * mScreenWidth + x) * 4;
                argb_buffer[idx + 0] = (pixel & 0x00FF0000) >> 16; // B
                argb_buffer[idx + 1] = (pixel & 0x0000FF00) >> 8;  // G
                argb_buffer[idx + 2] = (pixel & 0x000000FF);       // R
                argb_buffer[idx + 3] = 255;                        // A (fully opaque)
            }
        }

        // Convert ARGB to RGB using libyuv
        std::vector<uint8_t> rgb_buffer(mScreenWidth * mScreenHeight * 3);
        int result = libyuv::ARGBToRGB24(
            argb_buffer.data(), mScreenWidth * 4,
            rgb_buffer.data(), mScreenWidth * 3,
            mScreenWidth, mScreenHeight);

        if (result != 0)
        {
            std::cerr << "Failed to convert ARGB to RGB using libyuv" << std::endl;
            return;
        }

        std::filesystem::path outputDir = "out";
        if (!std::filesystem::exists(outputDir))
        {
            std::filesystem::create_directories(outputDir);
            std::cout << "Created output directory: " << outputDir << std::endl;
        }

        // Write JPEG file
        if (!writeJPEG("out/captured_image.jpg", rgb_buffer.data(), mScreenWidth, mScreenHeight, 90))
        {
            std::cerr << "Failed to write JPEG file." << std::endl;
            return;
        }
        std::cout << "Captured image for window ID: " << windowId << std::endl;
    }
    void DesktopCapture::stopCapture()
    {
        // Stop capturing the current window
        std::cout << "Stopping capture." << std::endl;
        // Implementation of stopping logic goes here
    }

} // namespace screen_recorder