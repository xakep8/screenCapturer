cmake_minimum_required(VERSION 3.16)
project(ScreenRecorder VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(JPEG REQUIRED libjpeg)
find_package(PkgConfig REQUIRED)
find_package(X11 REQUIRED)

find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h) 
find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
find_path(SWSCALE_INCLUDE_DIR libswscale/swscale.h)

find_library(AVCODEC_LIBRARY avcodec)
find_library(AVFORMAT_LIBRARY avformat)
find_library(AVUTIL_LIBRARY avutil)
find_library(SWSCALE_LIBRARY swscale)

if(AVCODEC_INCLUDE_DIR AND AVCODEC_LIBRARY AND 
   AVFORMAT_INCLUDE_DIR AND AVFORMAT_LIBRARY AND
   AVUTIL_INCLUDE_DIR AND AVUTIL_LIBRARY AND
   SWSCALE_INCLUDE_DIR AND SWSCALE_LIBRARY)
    
    message(STATUS "Found FFMPEG components")
    set(FFMPEG_FOUND TRUE)
    set(FFMPEG_INCLUDE_DIRS ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR} ${SWSCALE_INCLUDE_DIR})
    set(FFMPEG_LIBRARIES ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY} ${SWSCALE_LIBRARY})
    
else()
    message(FATAL_ERROR "FFMPEG components not found")
endif()

# Find common libraries for screen recording
# pkg_check_modules(X11 REQUIRED x11)
# pkg_check_modules(XEXT REQUIRED xext)
# pkg_check_modules(XFIXES REQUIRED xfixes)

# Try to find libyuv with different possible names
if(NOT LIBYUV_FOUND)
    find_path(LIBYUV_INCLUDE_DIR libyuv.h 
        PATHS /usr/include /usr/local/include
        PATH_SUFFIXES libyuv)
    find_library(LIBYUV_LIBRARY yuv 
        PATHS /usr/lib /usr/local/lib)
    
    if(LIBYUV_INCLUDE_DIR AND LIBYUV_LIBRARY)
        set(LIBYUV_FOUND TRUE)
        set(LIBYUV_INCLUDE_DIRS ${LIBYUV_INCLUDE_DIR})
        set(LIBYUV_LIBRARIES ${LIBYUV_LIBRARY})
        message(STATUS "Found libyuv manually: ${LIBYUV_LIBRARY}")
    endif()
endif()

# Include directories
include_directories(${X11_INCLUDE_DIR})
# include_directories(${XEXT_INCLUDE_DIRS})
# include_directories(${XFIXES_INCLUDE_DIRS})

if(LIBYUV_FOUND)
    include_directories(${LIBYUV_INCLUDE_DIRS})
    message(STATUS "Using libyuv")
else()
    message(STATUS "Building without libyuv")
endif()

# Add executable
add_executable(${PROJECT_NAME}
    screenRecorder.cpp
    desktopCapturer.cpp
    include/desktopCapturer.h
    include/windowUtils.h
    include/videoEncoder.h
    include/imageUtils.h
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${X11_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${FFMPEG_LIBRARIES}
    # ${XEXT_LIBRARIES}
    # ${XFIXES_LIBRARIES}
)

if(LIBYUV_FOUND)
    target_link_libraries(${PROJECT_NAME} ${LIBYUV_LIBRARIES})
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_LIBYUV)
endif()

# Compiler-specific options
target_compile_options(${PROJECT_NAME} PRIVATE
    # ${X11_CFLAGS_OTHER}
    # ${XEXT_CFLAGS_OTHER}
    # ${XFIXES_CFLAGS_OTHER}
)

if(LIBYUV_FOUND AND LIBYUV_CFLAGS_OTHER)
    target_compile_options(${PROJECT_NAME} PRIVATE ${LIBYUV_CFLAGS_OTHER})
endif()

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Optional: Add install target
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)