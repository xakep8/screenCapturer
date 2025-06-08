#include "windowUtils.h"
#include <iostream>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

namespace window_utils
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

    std::string getWindowName(Display* display, Window window)
    {
        char* window_name = nullptr;
        XFetchName(display, window, &window_name);
        if (window_name)
        {
            std::string result = window_name;
            XFree(window_name);
            return result;
        }
        return "Unnamed";
    }

} // namespace window_utils
