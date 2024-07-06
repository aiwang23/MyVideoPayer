//
// Created by 10484 on 2024/5/1.
//

#ifndef CLIENT_MYFORMATCONTEXT_H
#define CLIENT_MYFORMATCONTEXT_H

extern "C" {
#include <libavformat/avformat.h>

#include <utility>
};

namespace Utils
{
    /**
     * 自己包装的封装格式 方便内存管理
     */
    class MyFormatContext
    {
    public:
        MyFormatContext()
        {
            fmtCtx = avformat_alloc_context();
        }

        ~MyFormatContext()
        {
            if (fmtCtx)
                avformat_close_input(&fmtCtx);
            if (fmtCtx)
                avformat_free_context(fmtCtx);
        }


        bool Open(std::string _filePath,
                  const AVInputFormat *input_format = nullptr,
                  AVDictionary **opts = nullptr)
        {
            if (_filePath.empty())
            {
                fprintf(stderr, "%s %s filepath is empty \n",
                        __FILE__, __FUNCTION__);
                return false;
            }
            this->filePath = std::move(_filePath);
            // 打开，输入文件
            int ret = avformat_open_input(&this->fmtCtx, this->filePath.c_str(),
                                          input_format, opts);
            if (ret != 0)
            {
                av_strerror(ret, errMsg, 64);
                fprintf(stderr, "%s %s avformat_open_input %s ",
                        __FILE__, __FUNCTION__, errMsg);
                return false;
            }

            ret = avformat_find_stream_info(this->fmtCtx, nullptr);
            if (ret < 0)
            {
                av_strerror(ret, errMsg, sizeof(errMsg));
                fprintf(stderr, "%s %s %s %s \n",
                        __FILE__, __FUNCTION__, "avformat_find_stream_info", errMsg);
                return false;
            }

            // 寻找音频流和视频流索引
            this->videoStreamIdx = this->audioStreamIdx = -1;
            this->videoStreamIdx = av_find_best_stream(
                this->fmtCtx, AVMEDIA_TYPE_VIDEO,
                -1, -1, nullptr, 0
            );
            this->audioStreamIdx = av_find_best_stream(
                this->fmtCtx, AVMEDIA_TYPE_AUDIO,
                -1, -1, nullptr, 0
            );
            return true;
        }

        /**
         * 获取AVFormatContext的一级指针
         * @return AVFormatContext *
         */
        [[nodiscard]] AVFormatContext *GetFmtCtx() const
        {
            return this->fmtCtx;
        }

        /**
         * 获取AVFormatContext的二级指针
         * @return AVFormatContext **
         */
        AVFormatContext **GetFmtCtxPtr()
        {
            return &this->fmtCtx;
        }

        int GetStreamIdx(const AVMediaType media) const
        {
            if (media == AVMEDIA_TYPE_VIDEO)
                return videoStreamIdx;
            else if (media == AVMEDIA_TYPE_AUDIO)
                return audioStreamIdx;
            else
                return -1;
        }

        AVStream *GetStream(const AVMediaType media) const
        {
            if (media == AVMEDIA_TYPE_VIDEO)
                return videoStreamIdx >= 0 ? fmtCtx->streams[videoStreamIdx] : nullptr;
            else if (media == AVMEDIA_TYPE_AUDIO)
                return audioStreamIdx >= 0 ? fmtCtx->streams[audioStreamIdx] : nullptr;
            else
                return nullptr;
        }

        AVCodecID GetCodecID(const AVMediaType media) const
        {
            if (media == AVMEDIA_TYPE_VIDEO)
                return videoStreamIdx >= 0
                           ? fmtCtx->streams[videoStreamIdx]->codecpar->codec_id
                           : AV_CODEC_ID_NONE;
            else if (media == AVMEDIA_TYPE_AUDIO)
                return audioStreamIdx >= 0
                           ? fmtCtx->streams[audioStreamIdx]->codecpar->codec_id
                           : AV_CODEC_ID_NONE;
            else
                return AV_CODEC_ID_NONE;
        }

        int64_t GetDuration() const
        {
            return fmtCtx->duration;
        }

        bool Seek(const int streamIdx, const int64_t target,
                  const int flag = AVSEEK_FLAG_BACKWARD)
        {
            int ret = av_seek_frame(fmtCtx, streamIdx, target, flag);
            if (ret < 0)
            {
                av_strerror(ret, errMsg, 64);
                fprintf(stderr, "%s %s %s %s \n",
                        __FILE__, __FUNCTION__, "av_seek_frame", errMsg);
                return false;
            }
            return true;
        }

        bool ReadPacket(AVPacket *&pkt)
        {
            int ret = av_read_frame(fmtCtx, pkt);
            if (ret < 0)
            {
                av_strerror(ret, errMsg, 64);
                fprintf(stderr, "%s %s %s %s \n",
                        __FILE__, __FUNCTION__, "av_read_frame", errMsg);
                return false;
            }
            return true;
        }

        bool ReadPacket(const std::shared_ptr<Utils::MyPacket> &pkt)
        {
            int ret = av_read_frame(fmtCtx, pkt->GetPkt());
            if (ret < 0)
            {
                av_strerror(ret, errMsg, 64);
                fprintf(stderr, "%s %s %s %s \n",
                        __FILE__, __FUNCTION__, "av_read_frame", errMsg);
                return false;
            }
            return true;
        }

        const char * GetURL()
        {
            return fmtCtx->url;
        }

        AVCodecParameters *GetCodecPara(AVMediaType media)
        {
            if (media == AVMEDIA_TYPE_VIDEO)
                return videoStreamIdx >= 0
                           ? fmtCtx->streams[videoStreamIdx]->codecpar
                           : nullptr;
            else if (media == AVMEDIA_TYPE_AUDIO)
                return audioStreamIdx >= 0
                           ? fmtCtx->streams[audioStreamIdx]->codecpar
                           : nullptr;
            else
                return nullptr;
        }

        void DumpFormat(const int idx = 0, const int isOutPut = 0) const
        {
            av_dump_format(fmtCtx, idx, fmtCtx->url, isOutPut);
        }

    private:
        AVFormatContext *fmtCtx = nullptr;
        std::string filePath;
        char errMsg[64] = {0};
        int videoStreamIdx = -1;
        int audioStreamIdx = -1;
    };
} // Utils

#endif //CLIENT_MYFORMATCONTEXT_H
