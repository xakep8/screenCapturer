#include "desktopCapturer.h"
#include "windowUtils.h"
#include "imageUtils.h"
#include "videoEncoder.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>
#include <fstream>
#include <libyuv.h>
#include <jpeglib.h>
#include <filesystem>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

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
        captureThumbnail(mCapturableWindows[1], "output.jpg"); // Start capturing the first capturable window as an example
        startCapture(mCapturableWindows[1], "output.mp4", 30, 10); // Start video recording for the first capturable window
        std::cout << "DesktopCapture initialized successfully." << std::endl;
    }
    DesktopCapture::~DesktopCapture()
    {
        // Clean up resources if necessary
        std::cout << "DesktopCapture destroyed." << std::endl;
    }
    void DesktopCapture::startCapture(Window windowId, const std::string &filename, int fps, int duration_seconds)
    {
        std::cout << "Starting video recording for window ID: " << windowId << std::endl;

        XWindowAttributes attrs;
        if (XGetWindowAttributes(mDisplay.get(), windowId, &attrs) == 0)
        {
            std::cerr << "Failed to get attributes for window ID: " << windowId << std::endl;
            return;
        }

        int width = attrs.width;
        int height = attrs.height;

        // Make sure dimensions are even (required for many codecs)
        width = (width + 1) & ~1;
        height = (height + 1) & ~1;

        std::cout << "Recording window: " << windowId << " with size: " << width << "x" << height << std::endl;

        // Create output directory
        std::filesystem::path outputDir = "out";
        if (!std::filesystem::exists(outputDir))
        {
            std::filesystem::create_directories(outputDir);
            std::cout << "Created output directory: " << outputDir << std::endl;
        }

        AVFormatContext *formatContext = nullptr;
        avformat_alloc_output_context2(&formatContext, nullptr, nullptr, ("out/" + filename).c_str());
        if (!formatContext)
        {
            std::cerr << "Failed to create format context" << std::endl;
            return;
        }

        // Find H.264 encoder
        const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (!codec)
        {
            std::cerr << "H.264 codec not found" << std::endl;
            avformat_free_context(formatContext);
            return;
        }

        // Create video stream
        AVStream *stream = avformat_new_stream(formatContext, codec);
        if (!stream)
        {
            std::cerr << "Failed to create video stream" << std::endl;
            avformat_free_context(formatContext);
            return;
        }

        // Configure codec context
        AVCodecContext *codecContext = avcodec_alloc_context3(codec);
        codecContext->width = width;
        codecContext->height = height;
        codecContext->time_base = {1, fps};
        codecContext->framerate = {fps, 1};
        codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
        codecContext->bit_rate = 2000000; // 2 Mbps

        // Some formats want stream headers to be separate
        if (formatContext->oformat->flags & AVFMT_GLOBALHEADER)
            codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        // Open codec
        if (avcodec_open2(codecContext, codec, nullptr) < 0)
        {
            std::cerr << "Failed to open codec" << std::endl;
            avcodec_free_context(&codecContext);
            avformat_free_context(formatContext);
            return;
        }

        // Copy codec parameters to stream
        avcodec_parameters_from_context(stream->codecpar, codecContext);

        // Open output file
        if (!(formatContext->oformat->flags & AVFMT_NOFILE))
        {
            if (avio_open(&formatContext->pb, ("out/" + filename).c_str(), AVIO_FLAG_WRITE) < 0)
            {
                std::cerr << "Failed to open output file" << std::endl;
                avcodec_free_context(&codecContext);
                avformat_free_context(formatContext);
                return;
            }
        }

        // Write header
        if (avformat_write_header(formatContext, nullptr) < 0)
        {
            std::cerr << "Failed to write header" << std::endl;
            return;
        }

        // Allocate frame
        AVFrame *frame = av_frame_alloc();
        frame->format = codecContext->pix_fmt;
        frame->width = width;
        frame->height = height;
        av_frame_get_buffer(frame, 0);

        // Create software scaler context for RGB to YUV conversion
        SwsContext *swsContext = sws_getContext(
            width, height, AV_PIX_FMT_RGB24,
            width, height, AV_PIX_FMT_YUV420P,
            SWS_BICUBIC, nullptr, nullptr, nullptr);

        int totalFrames = fps * duration_seconds;
        auto frameDelay = std::chrono::milliseconds(1000 / fps);

        std::cout << "Recording " << totalFrames << " frames at " << fps << " FPS..." << std::endl;

        for (int frameNum = 0; frameNum < totalFrames; frameNum++)
        {
            auto frameStart = std::chrono::steady_clock::now();

            // Capture current frame
            std::unique_ptr<XImage, ImageDeleter> xImage(
                XGetImage(mDisplay.get(), windowId, 0, 0, width, height, AllPlanes, ZPixmap));

            if (!xImage)
            {
                std::cerr << "Failed to capture frame " << frameNum << std::endl;
                continue;
            }

            // Convert XImage to RGB buffer
            std::vector<uint8_t> argb_buffer(width * height * 4);
            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    unsigned long pixel = XGetPixel(xImage.get(), x, y);

                    int idx = (y * width + x) * 4;
                    argb_buffer[idx + 0] = (pixel & 0x00FF0000) >> 16; // B
                    argb_buffer[idx + 1] = (pixel & 0x0000FF00) >> 8;  // G
                    argb_buffer[idx + 2] = (pixel & 0x000000FF);       // R
                    argb_buffer[idx + 3] = 255;                        // A
                }
            }

            // Convert ARGB to RGB using libyuv
            std::vector<uint8_t> rgb_buffer(width * height * 3);
            libyuv::ARGBToRGB24(
                argb_buffer.data(), width * 4,
                rgb_buffer.data(), width * 3,
                width, height);

            // Convert RGB to YUV420P using FFmpeg's swscale
            const uint8_t *rgb_data[1] = {rgb_buffer.data()};
            int rgb_linesize[1] = {width * 3};

            sws_scale(swsContext, rgb_data, rgb_linesize, 0, height,
                      frame->data, frame->linesize);

            frame->pts = frameNum;

            // Encode frame
            AVPacket *packet = av_packet_alloc();

            int ret = avcodec_send_frame(codecContext, frame);
            if (ret < 0)
            {
                std::cerr << "Error sending frame to encoder" << std::endl;
                continue;
            }

            while (ret >= 0)
            {
                ret = avcodec_receive_packet(codecContext, packet);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                else if (ret < 0)
                {
                    std::cerr << "Error during encoding" << std::endl;
                    break;
                }

                av_packet_rescale_ts(packet, codecContext->time_base, stream->time_base);
                packet->stream_index = stream->index;

                av_interleaved_write_frame(formatContext, packet);
                av_packet_unref(packet);
            }

            if (frameNum % fps == 0) // Print progress every second
            {
                std::cout << "Recorded " << frameNum << "/" << totalFrames << " frames" << std::endl;
            }

            // Maintain frame rate
            auto frameEnd = std::chrono::steady_clock::now();
            auto elapsed = frameEnd - frameStart;
            if (elapsed < frameDelay)
            {
                std::this_thread::sleep_for(frameDelay - elapsed);
            }
        }

        // Flush encoder
        avcodec_send_frame(codecContext, nullptr);
        AVPacket *packet = av_packet_alloc();
        while (avcodec_receive_packet(codecContext, packet) == 0)
        {
            av_packet_rescale_ts(packet, codecContext->time_base, stream->time_base);
            packet->stream_index = stream->index;
            av_interleaved_write_frame(formatContext, packet);
            av_packet_unref(packet);
        }

        // Write trailer and cleanup
        av_write_trailer(formatContext);

        sws_freeContext(swsContext);
        av_frame_free(&frame);
        avcodec_free_context(&codecContext);
        if (!(formatContext->oformat->flags & AVFMT_NOFILE))
            avio_closep(&formatContext->pb);
        avformat_free_context(formatContext);

        std::cout << "Video recording completed: out/" << filename << std::endl;
    }

    void DesktopCapture::captureThumbnail(Window windowId, const std::string &filename)
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
        if (!writeJPEG(("out/"+filename), rgb_buffer.data(), mScreenWidth, mScreenHeight, 90))
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