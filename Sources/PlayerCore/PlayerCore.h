//
// Created by 10484 on 24-5-5.
//

#ifndef PLAYERCORE_H
#define PLAYERCORE_H
#pragma execution_character_set("utf-8")
#include <list>
#include <memory>

#include "MyQueue.h"
#include "MyFormatContext.h"
#include "MyCodecContext.h"
#include "MySwrContext.h"
#include "MySwsContext.h"
#include "MyType.h"
#include "VideoPlayerCallBack.h"
#include "MyThreadPool.h"

extern "C" {
#include <SDL.h>
}


#define FLUSH_DATA                   "FLUSH"     // 用来告诉解码器发生了跳转
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000      // 1 second of 48khz 32bit audio
#define MAX_AUDIO_SIZE               (50 * 20)   // audio Packet队列的长度
#define MAX_VIDEO_SIZE               (25 * 20)   // video Packet队列的长度


namespace PlayerCore
{
    class PlayerCore
    {
    public:
        PlayerCore();

        ~PlayerCore();

        //! 继承了 VideoPlayerCallBack 的子类，请调用此函数
        void SetVideoPlayerCallBack(VideoPlayerCallBack *pointer);

        /**
         * 开始播放文件
         * @param filePath 播放文件路径
         * @return true: 成功
         */
        bool StartPlay(const std::string &filePath);

        // 重新播放
        bool Replay();

        // 恢复播放（用于暂停后，重新开始播放）
        bool Play();

        // 暂停播放
        bool Pause();

        /**
         * 停止播放
         * @param isWait 是否等待所有的线程执行完毕再返回
         * @return
         */
        bool Stop(bool isWait = false);

        /**
         * 跳转 - 单位是微秒
         * @param pos
         */
        void Seek(int64_t pos);

        void SetMute(bool isMute);

        // 调节音量
        void SetVolume(float value);

        // 获取音量
        float GetVolume() const;

        // 获取视频时长 - 单位微秒
        int64_t GetTotalTime() const;

#undef GetCurrentTime
        // 获取音频时钟 - 单位秒
        double GetCurrentTime() const;

        std::string GetURL() const;

        PlayerState GetState() const;

    protected:
        // 读取视频文件线程
        void ReadVideoFileThread();

        // 解码视频线程
        void DecodeVideoThread();

        // SDL 音频回调函数
        static void SdlAudioCallBackFunc(void *userdata, Uint8 *stream, int len);

        void SdlAudioCallBack(Uint8 *stream, int len);

        // 解码音频
        int DecodeAudioFrame(bool isBlock = false);

        // 视频帧入队
        bool InputVideoQuene(AVPacket &pkt);

        bool InputVideoQuene(const std::shared_ptr<Utils::MyPacket> &_pkt);

        // 视频队列清空
        void ClearVideoQuene();

        // 音频入队
        bool InputAudioQuene(AVPacket &pkt);

        bool InputAudioQuene(const std::shared_ptr<Utils::MyPacket> &_pkt);

        // 音频队列清空
        void ClearAudioQuene();

    private:
        void Release();

        // 打开音频
        int OpenSDL();

        // 关闭音频
        void CloseSDL();

        // // 滤镜
        // int configure_filtergraph(AVFilterGraph* graph, const char* filtergraph, AVFilterContext* source_ctx, AVFilterContext* sink_ctx);
        // int configure_video_filters(AVFilterGraph* graph, const char* vfilters, AVFrame* frame);

        ///回调函数相关，主要用于输出信息给界面
        ///打开文件失败
        void doOpenVideoFileFailed(const int &code = 0) const;

        ///打开sdl失败的时候回调此函数
        void doOpenSdlFailed() const;

        ///获取到视频时长的时候调用此函数  更新时长
        void doTotalTimeChanged(const int64_t &uSec) const;

        ///播放器状态改变的时候回调此函数
        void doPlayerStateChanged(const PlayerState &state, const bool &hasVideo, const bool &hasAudio) const;

        ///显示视频数据，此函数不宜做耗时操作，否则会影响播放的流畅性。
        void doDisplayVideo(const uint8_t *yuv420Buffer, const int &width, const int &height) const;

    private:
        std::string mFilePath;           //视频文件路径
        PlayerState mPlayerState = STOP; //播放状态

        ///音量相关变量
        bool mIsMute = false; // 是否静音
        float mVolume = 1;    // 音量 0~1 超过1 表示放大倍数

        /// 跳转相关的变量
        int seekReq = 0;       // 跳转标志
        int64_t seekPos = 0;   // 跳转的位置 -- 微秒
        int seekFlagAudio = 0; // 跳转标志 -- 用于音频线程中
        int seekFlagVideo = 0; // 跳转标志 -- 用于视频线程中
        double seekTime = 0;   // 跳转的时间(秒)  值和seek_pos是一样的

        ///播放控制相关
        bool mIsNeedPause = false;           // 暂停后跳转先标记此变量
        bool mIsPause = false;               // 暂停标志
        bool mIsQuit = false;                // 停止
        bool mIsReadFinished = false;        // 文件读取完毕
        bool mIsReadThreadFinished = false;  // 读取线程
        bool mIsVideoThreadFinished = false; // 视频解码线程
        bool mIsAudioThreadFinished = false; // 音频播放线程

        ///音视频同步相关
        uint64_t mVideoStartTime = 0;     // 开始播放视频的时间
        uint64_t mPauseStartTime = 0;     // 暂停开始的时间
        double audioClock = 0;            // 音频时钟
        double videoClock = 0;            // 视频时钟
        AVStream *mVideoStream = nullptr; // 视频流
        AVStream *mAudioStream = nullptr; // 音频流

        ///视频相关
        std::unique_ptr<Utils::MyFormatContext> pFmtCtx = nullptr;       // 文件封装格式
        std::unique_ptr<Utils::MyCodecContext> pVideoCodecCtx = nullptr; // 视频解码器
        std::unique_ptr<Utils::MySwsContext> pVideoSwsCtx = nullptr;     // 视频转换

        ///音频相关
        std::unique_ptr<Utils::MyCodecContext> pAudioCodecCtx = nullptr; // 音频解码器
        AVFrame *audioFrame = nullptr;                                   // 解码后 重采样前的 音频帧
        // AVFrame *audioFrameReSample = nullptr;                           // 重采样后的 音频帧
        std::shared_ptr<Utils::MyFrame> audioFrameReSample = nullptr; // 重采样后的 音频帧
        std::unique_ptr<Utils::MySwrContext> pAudioSwrCtx = nullptr;  // 音频重采样

        /// 重采样
        AVSampleFormat inSampleFmt = AV_SAMPLE_FMT_NONE;  // 重采样前 格式
        int inSampleRate = 0;                             //         采样率
        AVChannelLayout inCHLayout;                       //         声道
        AVSampleFormat outSampleFmt = AV_SAMPLE_FMT_NONE; // 重采样后 格式   s16
        int outSampleRate = 0;                            //         重采样 48000/44100
        AVChannelLayout outCHLayout;                      //         声道  stereo

        /// 待播放的音频
        int audioBufferSize = 0;
        int audioBufferIdx = 0;
        DECLARE_ALIGNED(16, uint8_t, audioBuffer)[AVCODEC_MAX_AUDIO_FRAME_SIZE * 4] = {0}; // 音频

        int autorotate = 1;
        int filter_nbthreads = 0;

#if CONFIG_AVFILTER
	const char** vfilters_list = NULL;
	int nb_vfilters = 0;
	char* afilters = NULL;

	int vfilter_idx;
	AVFilterContext* in_video_filter;   // the first filter in the video chain
	AVFilterContext* out_video_filter;  // the last filter in the video chain
	//    AVFilterContext *in_audio_filter;   // the first filter in the audio chain
	//    AVFilterContext *out_audio_filter;  // the last filter in the audio chain
	//    AVFilterGraph *agraph;              // audio filter graph
#endif

        // * 视频帧队列
        Utils::MyPacketQueue mVideoPacketQueue;

        // * 音频帧队列
        Utils::MyPacketQueue mAudioPacketQueue;

        // * 对外的接口
        VideoPlayerCallBack *mVideoPlayerCallBack = nullptr;

        ///本播放器中SDL仅用于播放音频，不用做别的用途
        ///SDL播放音频相关
        SDL_AudioDeviceID mAudioID = 0;
        // 线程池
        Utils::MyThreadPool mThreadPool;
    };
} // PlayerCore

#endif //PLAYERCORE_H
