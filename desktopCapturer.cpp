#include "desktopCapturer.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

namespace
{
    void get_all_windows(Display *display, Window window, std::vector<Window> &windows)
    {
        Window root_return, parent_return;
        Window *children_return;
        unsigned int nchildren_return;

        if (XQueryTree(display, window, &root_return, &parent_return, &children_return, &nchildren_return))
        {
            for (unsigned int i = 0; i < nchildren_return; i++)
            {
                windows.push_back(children_return[i]);
                get_all_windows(display, children_return[i], windows);
            }
            if (children_return)
            {
                XFree(children_return);
            }
        }
    }
    bool is_window_capturable(Display *display, Window window)
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

    std::string get_window_class(Display *display, Window window)
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
        std::vector<Window> mWindows;
        get_all_windows(mDisplay.get(), mRootWindow, mWindows);
        std::cout << "\n=== All Windows ===" << std::endl;
        std::cout << "Total windows found: " << mWindows.size() << std::endl;

        std::cout << "\n=== Capturable Windows ===" << std::endl;
        int capturable_count = 0;

        for (auto window_id : mWindows)
        {
            XWindowAttributes attrs;
            if (XGetWindowAttributes(mDisplay.get(), window_id, &attrs) == 0)
            {
                continue;
            }

            char *window_name = nullptr;
            XFetchName(mDisplay.get(), window_id, &window_name);

            bool capturable = is_window_capturable(mDisplay.get(), window_id);
            std::string window_class = get_window_class(mDisplay.get(), window_id);

            // Only print detailed info for potentially capturable windows
            if (capturable)
            {
                capturable_count++;
                std::cout << "\n--- Window #" << capturable_count << " ---" << std::endl;
                std::cout << "ID: " << window_id << std::endl;
                std::cout << "Name: " << (window_name ? window_name : "Unnamed") << std::endl;
                std::cout << "Class: " << window_class << std::endl;
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
            }

            if (window_name)
            {
                XFree(window_name);
            }
        }

        std::cout << "\n=== Summary ===" << std::endl;
        std::cout << "Total windows: " << mWindows.size() << std::endl;
        std::cout << "Capturable windows: " << capturable_count << std::endl;
    }
    DesktopCapture::~DesktopCapture()
    {
        // Clean up resources if necessary
        std::cout << "DesktopCapture destroyed." << std::endl;
    }
    void DesktopCapture::startCapture()
    {
    }

} // namespace screen_recorder