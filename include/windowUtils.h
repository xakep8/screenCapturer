#ifndef WINDOW_UTILS_H
#define WINDOW_UTILS_H

#include <X11/Xlib.h>
#include <vector>
#include <string>

namespace window_utils
{

    bool isWindowCapturable(Display *display, Window window);

    void getAllWindows(Display *display, Window window, std::vector<Window> &windows);

    std::string getWindowClass(Display *display, Window window);

}
#endif // WINDOW_UTILS_H