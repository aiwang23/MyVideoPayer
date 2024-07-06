//
// Created by 10484 on 24-6-8.
//

#ifndef MYYUVFRAME_H
#define MYYUVFRAME_H
#include <cstdint>

namespace Utils
{
    /// YUV数据的封装类
    class MyYUVFrame
    {
    public:
        MyYUVFrame();

        ~MyYUVFrame();

        void initBuffer(const int &width, const int &height);

        void setYUVbuf(const uint8_t *buf) const;

        void setYbuf(const uint8_t *buf) const;

        void setUbuf(const uint8_t *buf) const;

        void setVbuf(const uint8_t *buf) const;

        uint8_t *buffer() const { return mYuv420Buffer; }
        int width() const { return mWidth; }
        int height() const { return mHegiht; }

    protected:
        uint8_t *mYuv420Buffer = nullptr;

        int mWidth = 0;
        int mHegiht = 0;
    };
} // Utils

#endif //MYYUVFRAME_H
