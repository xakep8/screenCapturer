#include "videoEncoder.h"
#include <iostream>
#include <filesystem>

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

namespace video_encoder
{
    VideoEncoder::VideoEncoder()
        : mFormatContext(nullptr), mCodecContext(nullptr), mVideoStream(nullptr),
          mFrame(nullptr), mSwsContext(nullptr), mFrameIndex(0), mInitialized(false)
    {
    }

    VideoEncoder::~VideoEncoder()
    {
        finalize();
    }

    bool VideoEncoder::initialize(const std::string& filename, int width, int height, int fps, int bitrate)
    {
        // Create output directory
        std::filesystem::path outputDir = "out";
        if (!std::filesystem::exists(outputDir))
        {
            std::filesystem::create_directories(outputDir);
        }

        // Initialize format context
        avformat_alloc_output_context2(&mFormatContext, nullptr, nullptr, ("out/" + filename).c_str());
        if (!mFormatContext)
        {
            std::cerr << "Failed to create format context" << std::endl;
            return false;
        }

        // Find H.264 encoder
        const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (!codec)
        {
            std::cerr << "H.264 codec not found" << std::endl;
            return false;
        }

        // Create video stream
        mVideoStream = avformat_new_stream(mFormatContext, codec);
        if (!mVideoStream)
        {
            std::cerr << "Failed to create video stream" << std::endl;
            return false;
        }

        // Configure codec context
        mCodecContext = avcodec_alloc_context3(codec);
        mCodecContext->width = width;
        mCodecContext->height = height;
        mCodecContext->time_base = {1, fps};
        mCodecContext->framerate = {fps, 1};
        mCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
        mCodecContext->bit_rate = bitrate;

        if (mFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
            mCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        // Open codec
        if (avcodec_open2(mCodecContext, codec, nullptr) < 0)
        {
            std::cerr << "Failed to open codec" << std::endl;
            return false;
        }

        // Copy codec parameters to stream
        avcodec_parameters_from_context(mVideoStream->codecpar, mCodecContext);

        // Open output file
        if (!(mFormatContext->oformat->flags & AVFMT_NOFILE))
        {
            if (avio_open(&mFormatContext->pb, ("out/" + filename).c_str(), AVIO_FLAG_WRITE) < 0)
            {
                std::cerr << "Failed to open output file" << std::endl;
                return false;
            }
        }

        // Write header
        if (avformat_write_header(mFormatContext, nullptr) < 0)
        {
            std::cerr << "Failed to write header" << std::endl;
            return false;
        }

        // Allocate frame
        mFrame = av_frame_alloc();
        mFrame->format = mCodecContext->pix_fmt;
        mFrame->width = width;
        mFrame->height = height;
        av_frame_get_buffer(mFrame, 0);

        // Create software scaler context
        mSwsContext = sws_getContext(
            width, height, AV_PIX_FMT_RGB24,
            width, height, AV_PIX_FMT_YUV420P,
            SWS_BICUBIC, nullptr, nullptr, nullptr);

        mInitialized = true;
        return true;
    }

    bool VideoEncoder::encodeFrame(const uint8_t* rgb_buffer, int width, int height)
    {
        if (!mInitialized) return false;

        // Convert RGB to YUV420P
        const uint8_t* rgb_src[1] = {rgb_buffer};
        int rgb_linesize[1] = {width * 3};

        sws_scale(mSwsContext, rgb_src, rgb_linesize, 0, height,
                  mFrame->data, mFrame->linesize);

        mFrame->pts = mFrameIndex++;

        // Encode frame
        AVPacket* packet = av_packet_alloc();
        if (!packet) return false;

        int ret = avcodec_send_frame(mCodecContext, mFrame);
        if (ret < 0)
        {
            av_packet_free(&packet);
            return false;
        }

        while (ret >= 0)
        {
            ret = avcodec_receive_packet(mCodecContext, packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            else if (ret < 0)
                break;

            av_packet_rescale_ts(packet, mCodecContext->time_base, mVideoStream->time_base);
            packet->stream_index = mVideoStream->index;

            av_interleaved_write_frame(mFormatContext, packet);
            av_packet_unref(packet);
        }

        av_packet_free(&packet);
        return true;
    }

    void VideoEncoder::finalize()
    {
        if (!mInitialized) return;

        // Flush encoder
        avcodec_send_frame(mCodecContext, nullptr);
        AVPacket* packet = av_packet_alloc();
        if (packet)
        {
            while (avcodec_receive_packet(mCodecContext, packet) == 0)
            {
                av_packet_rescale_ts(packet, mCodecContext->time_base, mVideoStream->time_base);
                packet->stream_index = mVideoStream->index;
                av_interleaved_write_frame(mFormatContext, packet);
                av_packet_unref(packet);
            }
            av_packet_free(&packet);
        }

        // Write trailer and cleanup
        av_write_trailer(mFormatContext);

        if (mSwsContext) sws_freeContext(mSwsContext);
        if (mFrame) av_frame_free(&mFrame);
        if (mCodecContext) avcodec_free_context(&mCodecContext);
        if (!(mFormatContext->oformat->flags & AVFMT_NOFILE))
            avio_closep(&mFormatContext->pb);
        if (mFormatContext) avformat_free_context(mFormatContext);

        mInitialized = false;
    }
}