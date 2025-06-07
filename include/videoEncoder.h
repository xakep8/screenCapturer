#ifndef VIDEO_ENCODER_H
#define VIDEO_ENCODER_H

#include <string>
#include <vector>
#include <cstdint>
#include <X11/Xlib.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
namespace video_encoder
{
    class VideoEncoder
    {
    public:
        VideoEncoder();
        ~VideoEncoder();

        bool initialize(const std::string &filename, int width, int height, int fps);
        bool encodeFrame(const uint8_t *rgb_buffer, int width, int height);
        void finalize();

    private:
        AVFormatContext *mFormatContext;
        AVStream *mVideoStream;
        AVCodecContext *mCodecContext;
        AVFrame *mFrame;
        SwsContext *mSwsContext;
        int mFrameIndex;
        bool mInitialized;
    };
}

#endif // VIDEO_ENCODER_H