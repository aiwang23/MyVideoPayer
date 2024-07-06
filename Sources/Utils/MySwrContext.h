//
// Created by 10484 on 24-5-4.
//

#ifndef MYSWRCONTEXT_H
#define MYSWRCONTEXT_H
#include <iostream>

extern "C" {
#include <libswresample/swresample.h>
}

namespace Utils
{
    class MySwrContext
    {
    public:
        explicit MySwrContext()
        {
            swrCtx = swr_alloc();
        }

        ~MySwrContext()
        {
            if (swr_is_initialized(swrCtx))
                swr_close(swrCtx);
            if (swrCtx)
                swr_free(&swrCtx);
        }

        bool Init(const AVSampleFormat inAudioSampleFmt,
                  AVChannelLayout *inAudioCHLayout,
                  const int inAudioSampleRate,
                  const AVSampleFormat outAudioSampleFmt,
                  AVChannelLayout *outAudioCHLayout,
                  const int outAudioSampleRate)
        {
            int ret = swr_alloc_set_opts2(&swrCtx, outAudioCHLayout,
                                          outAudioSampleFmt,
                                          outAudioSampleRate,
                                          inAudioCHLayout,
                                          inAudioSampleFmt,
                                          inAudioSampleRate,
                                          0,
                                          nullptr
            );
            if (ret < 0)
            {
                av_strerror(ret, errBuff, sizeof(errBuff));
                std::cerr << __FILE__ << __FUNCTION__ << errBuff << std::endl;
                return false;
            }
            ret = swr_init(swrCtx);
            if (ret < 0)
            {
                av_strerror(ret, errBuff, sizeof(errBuff));
                std::cerr << __FILE__ << __FUNCTION__ << errBuff << std::endl;
                return false;
            }
            return true;
        }


        [[nodiscard]] SwrContext *GetCtx() const
        {
            return swrCtx;
        }

        [[nodiscard]] SwrContext **GetCtxPtr()
        {
            return &swrCtx;
        }

        int Convert(const uint8_t ** inData, int inNBSamples, uint8_t ** outData, int outNBSamples)
        {
            return swr_convert(swrCtx, outData, outNBSamples, inData, inNBSamples);
        }

        int GetDelay(int sampleRate)
        {
            return swr_get_delay(swrCtx, sampleRate);
        }

    private:
        SwrContext *swrCtx = nullptr;
        char errBuff[AV_ERROR_MAX_STRING_SIZE] = {0};
    };
} // Utils

#endif //MYSWRCONTEXT_H
