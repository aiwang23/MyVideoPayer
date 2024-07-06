//
// Created by 10484 on 24-6-8.
//

#include "MyYUVFrame.h"

#include <cstdlib>
#include <cstring>

namespace Utils
{
    MyYUVFrame::MyYUVFrame()
    {
        mYuv420Buffer = nullptr;
    }

    MyYUVFrame::~MyYUVFrame()
    {
        if (mYuv420Buffer)
        {
            free(mYuv420Buffer);
            mYuv420Buffer = nullptr;
        }
    }

    void MyYUVFrame::initBuffer(const int &width, const int &height)
    {
        if (mYuv420Buffer)
        {
            free(mYuv420Buffer);
            mYuv420Buffer = nullptr;
        }
        mWidth = width;
        mHegiht = height;
        mYuv420Buffer = static_cast<uint8_t *>(malloc(width * height * 3 / 2));
    }

    void MyYUVFrame::setYUVbuf(const uint8_t *buf) const
    {
        const int YSize = mWidth * mHegiht;
        memcpy(mYuv420Buffer, buf, YSize * 3 / 2);
    }

    void MyYUVFrame::setYbuf(const uint8_t *buf) const
    {
        const int YSize = mWidth * mHegiht;
        memcpy(mYuv420Buffer, buf, YSize);
    }

    void MyYUVFrame::setUbuf(const uint8_t *buf) const
    {
        const int YSize = mWidth * mHegiht;
        memcpy(mYuv420Buffer, buf + YSize, YSize / 4);
    }

    void MyYUVFrame::setVbuf(const uint8_t *buf) const
    {
        const int YSize = mWidth * mHegiht;
        memcpy(mYuv420Buffer, buf + YSize + YSize / 4, YSize / 4);
    }
} // Utils
