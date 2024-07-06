//
// Created by 10484 on 24-6-8.
//

#ifndef VIDEOPLAYERCALLBACK_H
#define VIDEOPLAYERCALLBACK_H
#include <cstdint>
#include <memory>
#include <MyQueue.h>
#include <MyYUVFrame.h>

#include "MyType.h"

namespace PlayerCore
{
    class VideoPlayerCallBack
    {
    public:
        /// 打开文件失败
        virtual void onOpenVideoFileFailed(const int &code = 0) = 0;

        /// 打开sdl失败的时候回调此函数
        virtual void onOpenSdlFailed() = 0;

        /// 获取到视频时长的时候调用此函数
        virtual void onTotalTimeChanged(const int64_t &uSec) = 0;

        /// 播放器状态改变的时候回调此函数
        virtual void onPlayerStateChanged(const PlayerState &state,
                                          const bool &hasVideo, const bool &hasAudio) =
        0;

        /// 播放视频，此函数不宜做耗时操作，否则会影响播放的流畅性
        virtual void onDisplayVideo(std::shared_ptr<Utils::MyYUVFrame> videoFrame) = 0;
    };
} // PlayerCore

#endif //VIDEOPLAYERCALLBACK_H
