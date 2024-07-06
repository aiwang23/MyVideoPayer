//
// Created by 10484 on 2024/5/2.
//

#ifndef CLIENT_MYFRAME_H
#define CLIENT_MYFRAME_H


extern "C" {
#include "libavutil/frame.h"
#include <libavutil/imgutils.h>
};

namespace Utils
{
    class MyFrame
    {
    public:
        MyFrame()
        {
            frame = av_frame_alloc();
        }

        ~MyFrame()
        {
            if (frame)
                av_frame_free(&frame);
        }

        [[nodiscard]] AVFrame *GetFrame() const
        {
            return this->frame;
        }

        AVFrame **GetFramePtr()
        {
            return &this->frame;
        }

        [[nodiscard]] uint8_t **GetData() const
        {
            return frame->data;
        }

        [[nodiscard]] int *GetLineSize() const
        {
            return frame->linesize;
        }

        int FillArray(uint8_t *&buffer, const AVPixelFormat &format,
                      const int w, const int h, const int align) const
        {
            return av_image_fill_arrays(frame->data, frame->linesize, buffer, format,
                                        w, h, align);
        }

        int FillArray(const uint8_t *buffer, const int nbCHannels, const int nbSamples,
                      const AVSampleFormat sampleFmt, const int align) const
        {
            return av_samples_fill_arrays(frame->data,
                                          frame->linesize,
                                          buffer,
                                          nbCHannels,
                                          nbSamples,
                                          sampleFmt,
                                          align);
        }

        [[nodiscard]] int GetAudioNBSamples() const
        {
            return frame->nb_samples;
        }

        int GetPts()
        {
            return frame->pts;
        }

        uint8_t **GetData()
        {
            return frame->data;
        }

        int *GetLinsSize() const
        {
            return frame->linesize;
        }

        void SetAudioSampleFmt(const AVSampleFormat format) const
        {
            frame->format = format;
        }

        bool SetCHLayout(const AVChannelLayout &channel_layout) const
        {
            int ret = av_channel_layout_copy(&frame->ch_layout, &channel_layout);
            if (ret < 0)
            {
                char err[64] = {0};
                av_strerror(ret, err, 64);
                fprintf(stderr, "%s %s %s %s \n", __FILE__,
                        __FUNCTION__, "av_channel_layout_copy", err);
                return false;
            }
            return true;
        }

        void SetAudioSampleRate(const int sampleRate) const
        {
            frame->sample_rate = sampleRate;
        }

        void SetAudioNBSamples(const int NBSamples) const
        {
            frame->nb_samples = NBSamples;
        }

    private:
        AVFrame *frame = nullptr;
    };
} // Utils

#endif //CLIENT_MYFRAME_H
