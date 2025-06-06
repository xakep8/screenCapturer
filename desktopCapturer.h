#ifndef SCREEN_RECORDER_H
#define SCREEN_RECORDER_H
#include <iostream>
#include <thread>
#include "libyuv/video_common.h"
#include "libyuv/convert.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <memory>
#include <vector>

struct DisplayDeleter {
    void operator()(Display* display) const {
        if (display) {
            XCloseDisplay(display);
        }
    }
};

struct ImageDeleter {
    void operator()(XImage* image) const {
        if (image) {
            XDestroyImage(image);
        }
    }
};

namespace screen_recorder {
class DesktopCapture {
private:
    std::thread mCaptureThread;
    std::unique_ptr<Display, DisplayDeleter> mDisplay;
    Window mRootWindow;
    int mScreenWidth;
    int mScreenHeight;
    XWindowAttributes mWindowAttributes;
    std::vector<Window> mCapturableWindows;
    // std::unique_ptr<XImage, decltype(&XDestroyImage)> mXImage;

public:
    DesktopCapture();
    ~DesktopCapture();
    void startCapture(auto window_id);
    void stopCapture();
};
}
#endif