cmake_minimum_required(VERSION 3.16)
project(ScreenRecorder VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(X11 REQUIRED)

# Find common libraries for screen recording
# pkg_check_modules(X11 REQUIRED x11)
# pkg_check_modules(XEXT REQUIRED xext)
# pkg_check_modules(XFIXES REQUIRED xfixes)

# Try to find libyuv with different possible names
pkg_check_modules(LIBYUV QUIET libyuv)
if(NOT LIBYUV_FOUND)
    pkg_check_modules(LIBYUV QUIET yuv)
endif()

# If pkg-config doesn't work, try to find it manually
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
    desktopCapturer.h
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${X11_LIBRARIES}
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