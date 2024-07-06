//
// Created by 10484 on 2024/5/2.
//

#ifndef CLIENT_MYCODECCONTEXT_H
#define CLIENT_MYCODECCONTEXT_H

extern "C" {
#include <libavcodec/avcodec.h>
};

#include <iostream>

namespace Utils
{
    class MyCodecContext
    {
    public:
        explicit MyCodecContext(const AVCodecID codecId)
        {
            this->codec = avcodec_find_decoder(codecId);
            if (!this->codec)
            {
                std::cerr << __FILE__ << __FUNCTION__ <<
                        "avcodec_find_decoder failed" << std::endl;
                return;
            }
            this->codecCtx = avcodec_alloc_context3(codec);
            if (!this->codecCtx)
            {
                std::cerr << __FILE__ << __FUNCTION__ <<
                        "avcodec_alloc_context3 failed" << std::endl;
                return;
            }
        }

        ~MyCodecContext()
        {
            if (avcodec_is_open(codecCtx))
                avcodec_close(codecCtx);
            if (this->codecCtx)
                avcodec_free_context(&this->codecCtx);
        }

        bool Open(AVDictionary **opts = nullptr)
        {
            int ret = avcodec_open2(codecCtx, codec, opts);
            if (ret < 0)
            {
                av_strerror(ret, errMsg, sizeof(errMsg));
                fprintf(stderr, "%s %s %s %s \n",
                        __FILE__, __FUNCTION__, "avcodec_open2", errMsg);
                return false;
            }
            return true;
        }

        [[nodiscard]] AVCodecContext *GetCodecCtx() const
        {
            return this->codecCtx;
        }

        AVCodecContext **GetCodecCtxPtr()
        {
            return &this->codecCtx;
        }

        [[nodiscard]] const AVCodec *GetCodec() const
        {
            return this->codec;
        }

        AVSampleFormat GetAudioSampleFmt() const
        {
            return this->codecCtx->sample_fmt;
        }

        AVChannelLayout GetAudioCHLayout()
        {
            return this->codecCtx->ch_layout;
        }

        int GetAudioSampleRate() const
        {
            return this->codecCtx->sample_rate;
        }

        void Flush() const
        {
            avcodec_flush_buffers(codecCtx);
        }

        bool SendPacket(const std::shared_ptr<Utils::MyPacket> &pkt)
        {
            int ret = avcodec_send_packet(codecCtx, pkt->GetPkt());
            if (ret < 0)
            {
                av_strerror(ret, errMsg, 64);
                fprintf(stderr, "%s %s %s %s \n",
                        __FILE__, __FUNCTION__, "avcodec_send_packet", errMsg);
                return false;
            }
            return true;
        }

        bool SendPacket(AVPacket *&pkt)
        {
            int ret = avcodec_send_packet(codecCtx, pkt);
            if (ret < 0)
            {
                av_strerror(ret, errMsg, 64);
                fprintf(stderr, "%s %s %s %s \n",
                        __FILE__, __FUNCTION__, "avcodec_send_packet", errMsg);
                return false;
            }
            return true;
        }

        bool RecieveFrame(const std::shared_ptr<Utils::MyFrame> &frame)
        {
            int ret = avcodec_receive_frame(codecCtx, frame->GetFrame());
            if (ret < 0)
            {
                // av_strerror(ret, errMsg, 64);
                // fprintf(stderr, "%s %s %s %s \n",
                //         __FILE__, __FUNCTION__, "avcodec_receive_frame", errMsg);
                return false;
            }
            return true;
        }

        bool RecieveFrame(AVFrame *&frame)
        {
            int ret = avcodec_receive_frame(codecCtx, frame);
            if (ret < 0)
            {
                // av_strerror(ret, errMsg, 64);
                // fprintf(stderr, "%s %s %s %s \n",
                //         __FILE__, __FUNCTION__, "avcodec_receive_frame", errMsg);
                return false;
            }
            return true;
        }

        bool CopyParaFrom(AVCodecParameters * parameters)
        {
            int ret = avcodec_parameters_to_context(codecCtx, parameters);
            if (ret < 0)
            {
                av_strerror(ret, errMsg, 64);
                fprintf(stderr, "%s %s %s %s \n",
                        __FILE__, __FUNCTION__, "avcodec_parameters_to_context", errMsg);
                return false;
            }
            return true;
        }

    private:
        AVCodecContext *codecCtx = nullptr;
        const AVCodec *codec = nullptr;
        char errMsg[64] = {0};
    };
} // Utils

#endif //CLIENT_MYCODECCONTEXT_H
