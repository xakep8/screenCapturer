# ScreenCapturer
This is simple C++ ScreenCapturer built for cross platform and is supposed to work on windows, linux and mac. This is part of my practice of C++ projects and seemed like a cool thing to build in C++, plus can be converted to a node addon later just will have to integrate with NAPI wrapper to expose the functions, or use execa to execute the functions of the binary in Javascript, lets see how it goes.

P.S. I hope this works ;)

## Building the code
You can run the project locally and test it out as follows.

`git clone https://github.com/xakep8/screenCapturer.git`

before you start building this project you'll need to install a few packages like `libyuv-dev`, `libjpeg-dev`, `libavcodec`, `libavformat`, `libswscale`, `X11/Xlib`. Install these and then follow the steps below.

followed by opening a terminal in the folder containing the project files and running the command

`cmake CMakeLists.txt`

followed by

`make`

followed by

`./out/ScreenRecorder`

## Contributing
After you've setup your project you're set to contribute to the project after every change you make to the code just repeat the cmake process above and everything after that too.
