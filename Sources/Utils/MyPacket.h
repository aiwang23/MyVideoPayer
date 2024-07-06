//
// Created by 10484 on 2024/5/1.
//

#ifndef CLIENT_MYPACKET_H
#define CLIENT_MYPACKET_H


extern "C" {
#include <libavutil/error.h>
#include <libavcodec/packet.h>
};

namespace Utils
{
    /**
     * 解码前的视频帧或音频帧
     */
    class MyPacket
    {
    public:
        MyPacket()
        {
            pkt = av_packet_alloc();
        }

        explicit MyPacket(AVPacket* _pkt)
        {
            pkt = av_packet_alloc();
            av_packet_move_ref(pkt, _pkt);
        }

        ~MyPacket()
        {
            if (pkt)
                av_packet_free(&pkt);
        }

        AVPacket *GetPkt() const
        {
            return pkt;
        }

        AVPacket **GetPktPtr()
        {
            return &pkt;
        }

        void Unref() const
        {
            av_packet_unref(this->pkt);
        }

        bool New(int size)
        {
            const int ret = av_new_packet(pkt, size);
            if (ret < 0)
            {
                av_strerror(ret, errMsg, 64);
                fprintf(stderr, "%s %s %s %s \n",
                        __FILE__, __FUNCTION__, "av_new_packet", errMsg);
                return false;
            }
            return true;
        }

        [[nodiscard]] uint8_t *GetData() const
        {
            return pkt && pkt->data? pkt->data: nullptr;
        }

        [[nodiscard]] int GetPts() const
        {
            return pkt->pts;
        }

        int GetDts()
        {
            return pkt->dts;
        }

        int GetStreamIdx() const
        {
            return pkt->stream_index;
        }

    protected:
        // 实际的音频帧或视频帧
        AVPacket *pkt = nullptr;
        char errMsg[64] = {0};
        uint8_t *data = nullptr;
    };
} // Utils

#endif //CLIENT_MYPACKET_H
