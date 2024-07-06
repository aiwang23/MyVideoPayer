//
// Created by 10484 on 24-5-5.
//

#ifndef MYSWSCONTEXT_H
#define MYSWSCONTEXT_H
#include <iostream>

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

namespace Utils
{
    class MySwsContext
    {
    public:
        explicit MySwsContext(const int inWidth,
                     const int inHeight,
                     const AVPixelFormat inPixFmt,
                     const int outWidth,
                     const int outHeight,
                     const AVPixelFormat outPixFmt)
        {
            this->swsCtx = sws_getContext(
                inWidth,
                inHeight,
                inPixFmt,
                outWidth,
                outHeight,
                outPixFmt,
                SWS_BICUBIC,
                nullptr, nullptr, nullptr
            );
            if (!swsCtx)
            {
                std::cerr << __FILE__ << __FUNCTION__ << "failed " << std::endl;
                return;
            }
        }

        ~MySwsContext()
        {
            if (swsCtx)
                sws_freeContext(swsCtx);
        }

        [[nodiscard]] SwsContext *GetCtx() const
        {
            return swsCtx;
        }

        int Scale(uint8_t **inData, const int *inLineSize, int inSliceY, int inSliceH,
                   uint8_t **outData, const int *outLineSize)
        {
            int len = sws_scale(swsCtx, inData, inLineSize, inSliceY, inSliceH,
                      outData, outLineSize);
            return len;
        }

    private:
        SwsContext *swsCtx = nullptr;
    };
} // Utils

#endif //MYSWSCONTEXT_H
