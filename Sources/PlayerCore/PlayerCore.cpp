//
// Created by 10484 on 24-5-5.
//
#pragma execution_character_set("utf-8")
#include "PlayerCore.h"
#include <cstdio>

#include "MyType.h"

extern "C" {
#include <libavutil/time.h>
}

/**
 * 休眠 - ms级
 * @param mSecond
 */
static void mSleep(const int mSecond)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(mSecond));
}

namespace PlayerCore
{
    PlayerCore::PlayerCore()
        : inCHLayout(), outCHLayout(), mVideoPacketQueue(MAX_VIDEO_SIZE), // 初始化视频队列
          mAudioPacketQueue(MAX_AUDIO_SIZE),                              // 初始化视频队列
          mThreadPool(3)                                                  // 线程池
    {
        // 支持打开网络文件
        avformat_network_init();
        // 默认停止
        mPlayerState = STOP;
        // 音频驱动ID
        mAudioID = 0;
        // 默认不静音
        mIsMute = false;
        mIsNeedPause = false;
        // 音量最大
        mVolume = 1;
    }

    PlayerCore::~PlayerCore()
    {
        avformat_network_deinit();
    }

    void PlayerCore::SetVideoPlayerCallBack(VideoPlayerCallBack *pointer)
    {
        mVideoPlayerCallBack = pointer;
    }

    bool PlayerCore::StartPlay(const std::string &filePath)
    {
        // 只有停止了 才能开始播放
        if (mPlayerState != STOP)
        {
            return false;
        }
        // 默认不关闭各个线程
        mIsQuit = false;
        // 默认不暂停
        mIsPause = false;

        // 路径需要有效才更改
        if (!filePath.empty())
            mFilePath = filePath;
        else
            return false;

        ///SDL初始化需要放入子线程中，否则有些电脑会有问题。
        if (SDL_Init(SDL_INIT_AUDIO))
        {
            doOpenSdlFailed();
            fprintf(stderr, "Could not initialize SDL - %s. \n", SDL_GetError());
            return false;
        }
        // 默认读取线程未结束
        mIsReadThreadFinished = false;
        // 默认还没有读完文件
        mIsReadFinished = false;

        const char *file_path = mFilePath.c_str();

        audioFrame = nullptr;

        mAudioStream = nullptr;
        mVideoStream = nullptr;

        audioClock = 0;
        videoClock = 0;

        // * 初始化文件封装格式
        pFmtCtx = std::make_unique<Utils::MyFormatContext>();

        AVDictionary *opts = nullptr;
        // 设置tcp or udp，默认一般优先tcp再尝试udp
        av_dict_set(&opts, "rtsp_transport", "tcp", 0);
        // 设置超时3秒
        av_dict_set(&opts, "stimeout", "60000000", 0);
        // * 打开文件
        // if (avformat_open_input(&pFormatCtx, file_path, nullptr, &opts) != 0)
        if (pFmtCtx->Open(file_path, nullptr, &opts) == false)
        {
            fprintf(stderr, "can't open the file. \n");
            doOpenVideoFileFailed();
            Release();
        }
        av_dict_free(&opts);

        // * 获取视频流和音频流的索引
        const int videoStreamIdx = pFmtCtx->GetStreamIdx(AVMEDIA_TYPE_VIDEO);
        const int audioStreamIdx = pFmtCtx->GetStreamIdx(AVMEDIA_TYPE_AUDIO);
        // 更新视频时长
        doTotalTimeChanged(GetTotalTime());

        ///打开视频解码器，并启动视频线程
        if (videoStreamIdx >= 0)
        {
            // * 初始化，视频解码器
            pVideoCodecCtx = std::make_unique<Utils::MyCodecContext>(
                pFmtCtx->GetCodecID(AVMEDIA_TYPE_VIDEO));
            // * 拷贝视频编解码参数到视频解码器中
            pVideoCodecCtx->CopyParaFrom(
                pFmtCtx->GetCodecPara(AVMEDIA_TYPE_VIDEO));

            // * 打开视频解码器
            if (pVideoCodecCtx->Open() == false)
            {
                fprintf(stderr, "Could not open video codec.\n");
                doOpenVideoFileFailed();
                Release();
            }

            // * 获取视频流
            mVideoStream = pFmtCtx->GetStream(AVMEDIA_TYPE_VIDEO);

            // * 视频解码线程入池
            mThreadPool.enqueue([this](PlayerCore *ptr)
            {
                ptr->DecodeVideoThread();
            }, this);
        }

        if (audioStreamIdx >= 0)
        {
            // * 创建音频解码器
            pAudioCodecCtx = std::make_unique<Utils::MyCodecContext>(
                pFmtCtx->GetCodecID(AVMEDIA_TYPE_AUDIO));

            // * 拷贝音频编解码参数到音频解码器中
            pAudioCodecCtx->CopyParaFrom(
                pFmtCtx->GetCodecPara(AVMEDIA_TYPE_AUDIO));

            // * 打开音频解码器
            if (pAudioCodecCtx->Open() == false)
            {
                fprintf(stderr, "Could not open audio codec.\n");
                doOpenVideoFileFailed();
                Release();
            }

            // 解码后的音频帧分配内存
            audioFrame = av_frame_alloc();

            audioFrameReSample = nullptr;
            // * 初始化，音频重采样
            pAudioSwrCtx = std::make_unique<Utils::MySwrContext>();
            // 重采样之前
            inSampleFmt = pAudioCodecCtx->GetAudioSampleFmt();
            inCHLayout = pAudioCodecCtx->GetAudioCHLayout();
            inSampleRate = pAudioCodecCtx->GetAudioSampleRate();
            // 重采样之后
            outSampleFmt = AV_SAMPLE_FMT_S16;
            av_channel_layout_default(&outCHLayout, 2);
            outSampleRate = pAudioCodecCtx->GetAudioSampleRate();
            // 初始化
            int ret = pAudioSwrCtx->Init(
                inSampleFmt, &inCHLayout,
                inSampleRate,
                outSampleFmt, &outCHLayout,
                outSampleRate
            );
            // 设置重采样参数失败了
            if (ret == false)
            {
                char buff[128] = {0};
                av_strerror(ret, buff, 128);
                fprintf(stderr, "Could not open resample context %s\n", buff);

                pAudioSwrCtx.reset();
                pAudioSwrCtx = nullptr;
                doOpenVideoFileFailed();
                Release();
            }

            // * 音频流
            mAudioStream = pFmtCtx->GetStream(AVMEDIA_TYPE_AUDIO);

            // * 打开SDL播放声音
            int code = OpenSDL();
            if (code == 0)
            {
                SDL_LockAudioDevice(mAudioID);
                SDL_PauseAudioDevice(mAudioID, 0);
                SDL_UnlockAudioDevice(mAudioID);

                mIsAudioThreadFinished = false;
            }
            else
            {
                doOpenSdlFailed();
            }
            // }
        }

        // * 输出视频信息
        pFmtCtx->DumpFormat();
        // 开始播放
        mPlayerState = PLAYING;
        doPlayerStateChanged(PLAYING, mVideoStream != nullptr, mAudioStream != nullptr);
        // 获取开始播放的时间
        mVideoStartTime = av_gettime();
        fprintf(stderr, "%s mIsQuit=%d mIsPause=%d \n",
                __FUNCTION__, mIsQuit, mIsPause);


        // * 视频读取线程入池
        mThreadPool.enqueue([this](PlayerCore *ptr)
        {
            ptr->ReadVideoFileThread();
        }, this);

        return true;
    }

    bool PlayerCore::Replay()
    {
        // 先关闭，再启动
        bool ret = Stop();
        if (ret == false)
            return false;
        ret = StartPlay(mFilePath);
        return ret;
    }

    bool PlayerCore::Play()
    {
        // 告诉其他线程要恢复播放
        mIsNeedPause = false;
        mIsPause = false;
        // 不是暂停 不能重新播放
        if (mPlayerState != PAUSE)
        {
            return false;
        }

        // 获取暂停时间
        uint64_t pauseTime = av_gettime() - mVideoStartTime; //暂停了多长时间
        mVideoStartTime += pauseTime;                        //将暂停的时间加到开始播放的时间上，保证同步不受暂停的影响

        mPlayerState = PLAYING;
        doPlayerStateChanged(PLAYING, mVideoStream != nullptr, mAudioStream != nullptr);

        return true;
    }

    bool PlayerCore::Pause()
    {
        fprintf(stderr, "%s mIsPause=%d \n", __FUNCTION__, mIsPause);
        // 告诉其他线程要暂停了
        mIsPause = true;
        // 只有在播放的状态下 才能被暂停
        if (mPlayerState != PLAYING)
        {
            return false;
        }

        //mPauseStartTime = av_gettime();

        mPlayerState = PAUSE;

        //emit doPlayerStateChanged(VideoPlayer_Pause, mVideoStream != nullptr, mAudioStream != nullptr);
        doPlayerStateChanged(PAUSE, mVideoStream != nullptr, mAudioStream != nullptr);

        return true;
    }

    bool PlayerCore::Stop(bool isWait)
    {
        if (mPlayerState == STOP)
        {
            return false;
        }
        // 告诉其他线程要停止了
        mPlayerState = STOP;
        mIsQuit = true;
        // 等待读取线程停止后再退出
        if (isWait)
        {
            while (!mIsReadThreadFinished)
            {
                mSleep(3);
            }
        }

        return true;
    }

    void PlayerCore::Seek(int64_t pos)
    {
        if (!seekReq)
        {
            seekPos = pos;
            seekReq = 1;
        }
    }

    void PlayerCore::SetMute(bool isMute)
    {
        mIsMute = isMute;
    }

    void PlayerCore::SetVolume(float value)
    {
        mVolume = value;
    }

    float PlayerCore::GetVolume() const
    {
        return mVolume;
    }

    int64_t PlayerCore::GetTotalTime() const
    {
        return pFmtCtx->GetDuration();
    }

#undef GetCurrentTime
    double PlayerCore::GetCurrentTime() const
    {
        return audioClock;
    }

    std::string PlayerCore::GetURL() const
    {
        return std::string(pFmtCtx->GetURL());
    }

    PlayerState PlayerCore::GetState() const
    {
        return mPlayerState;
    }

    void PlayerCore::ReadVideoFileThread()
    {
        int videoStreamIdx = pFmtCtx->GetStreamIdx(AVMEDIA_TYPE_VIDEO);
        int audioStreamIdx = pFmtCtx->GetStreamIdx(AVMEDIA_TYPE_AUDIO);

        while (true)
        {
            // 停止播放
            if (mIsQuit)
            {
                break;
            }
            // 跳转
            if (seekReq)
            {
                int stream_index = -1;
                int64_t seek_target = seekPos;

                // 有可能不是视频 是音频
                if (videoStreamIdx >= 0)
                    stream_index = videoStreamIdx;
                else if (audioStreamIdx >= 0)
                    stream_index = audioStreamIdx;

                AVRational aVRational = {1, AV_TIME_BASE};
                if (stream_index >= 0)
                {
                    seek_target = av_rescale_q(seek_target, aVRational,
                                               pFmtCtx->GetFmtCtx()->streams[stream_index]->time_base);
                }

                // * 跳转
                if (pFmtCtx->Seek(stream_index, seek_target, AVSEEK_FLAG_BACKWARD) == false)
                {
                    // fprintf(stderr, "%s: error while seeking\n", pFormatCtx->url);
                    fprintf(stderr, "%s: error while seeking\n", pFmtCtx->GetURL());
                }
                else
                {
                    // 音频流
                    if (audioStreamIdx >= 0)
                    {
                        AVPacket packet;
                        av_new_packet(&packet, 10);
                        // strcpy((char *) packet.data, FLUSH_DATA);
                        strcpy_s(reinterpret_cast<char *>(packet.data),
                                 10, FLUSH_DATA);
                        ClearAudioQuene();       //清除队列
                        InputAudioQuene(packet); //往队列中存入用来清除的包
                    }
                    // 视频流
                    if (videoStreamIdx >= 0)
                    {
                        AVPacket packet;
                        av_new_packet(&packet, 10);
                        // strcpy((char *) packet.data, FLUSH_DATA);
                        strcpy_s(reinterpret_cast<char *>(packet.data),
                                 10, FLUSH_DATA);
                        ClearVideoQuene();       //清除队列
                        InputVideoQuene(packet); //往队列中存入用来清除的包
                        videoClock = 0;
                    }
                    // 计算出跳转后真正的开始时间
                    //? 假如跳转到30秒
                    //? mVideoStartTime 不可能从30秒的时候开始
                    //? 所以要 av_gettime() - seekPos
                    mVideoStartTime = av_gettime() - seekPos;
                    //mPauseStartTime = av_gettime();
                }
                seekReq = 0;
                seekTime = static_cast<double>(seekPos) / 1000000.0;
                // 告诉其他线程跳转成功了
                seekFlagAudio = 1;
                seekFlagVideo = 1;
                // 假如刚好暂停中跳转
                if (mIsPause)
                {
                    // 告诉其他线程 暂停中发生了跳转
                    mIsNeedPause = true;
                    mIsPause = false;
                }
            }

            // 这里做了个限制  当队列里面的数据超过某个大小的时候 就暂停读取  防止一下子就把视频读完了，导致的空间分配不足
            // 这个值可以稍微写大一些
            if (mAudioPacketQueue.GetSize() > MAX_AUDIO_SIZE ||
                mVideoPacketQueue.GetSize() > MAX_VIDEO_SIZE)
            {
                mSleep(10);
                continue;
            }
            // 假如还在暂停中就什么都不做
            if (mIsPause == true)
            {
                mSleep(10);
                continue;
            }

            auto pkt = std::make_shared<Utils::MyPacket>();
            // * 从文件中读取一帧
            if (pFmtCtx->ReadPacket(pkt) == false)
            {
                // 读完了
                mIsReadFinished = true;
                // 要退出了
                if (mIsQuit)
                {
                    break; //解码线程也执行完了 可以退出了
                }

                mSleep(10);
                continue;
            }

            // * 这一帧是视频帧
            if (pkt->GetStreamIdx() == videoStreamIdx)
            {
                // inputVideoQuene(packet);
                InputVideoQuene(pkt);
                //这里我们将数据存入队列 因此不调用 av_free_packet 释放
            }
            // * 这一帧是音频帧
            else if (pkt->GetStreamIdx() == audioStreamIdx)
            {
                // 音频解码线程已经结束 不需要放队列了
                if (mIsAudioThreadFinished)
                {
                    ///SDL没有打开，则音频数据直接释放
                    pkt->Unref();
                }
                else
                {
                    // 入队
                    InputAudioQuene(pkt);
                }
            }
            // * 其他流不需要 直接释放
            else
            {
                pkt->Unref();
            }
        }

        ///文件读取结束 跳出循环的情况
        ///等待播放完毕
        while (!mIsQuit)
        {
            mSleep(100);
        }

        Release();

        fprintf(stderr, "%s finished \n", __FUNCTION__);
    }

    void PlayerCore::DecodeVideoThread()
    {
        fprintf(stderr, "%s start \n", __FUNCTION__);
        // 默认视频解码线程未结束
        mIsVideoThreadFinished = false;
        // 视频的分辨率
        int videoWidth = 0;
        int videoHeight = 0;

        double video_pts = 0; //当前视频的pts
        double audio_pts = 0; //音频pts

        AVFrame *pFrame = nullptr;
        // AVFrame *pFrameYUV = nullptr;
        std::shared_ptr<Utils::MyFrame> pFrameYUV = nullptr;
        uint8_t *yuv420pBuffer = nullptr; //解码后的yuv数据
        // struct SwsContext *imgConvertCtx = nullptr; //用于解码后的视频格式转换

        // AVCodecContext *pCodecCtx = this->pCodecCtx; //视频解码器
        // AVCodecContext *pCodecCtx = this->pVideoCodecCtx->GetCodecCtx(); //视频解码器

        pFrame = av_frame_alloc();

#if CONFIG_AVFILTER
	AVFilterGraph* graph = NULL;
	AVFilterContext* filt_out = NULL, * filt_in = NULL;
	int last_w = 0;
	int last_h = 0;
	//    AVPixelFormat last_format = -2;
	int last_format = -2;
	//    int last_serial = -1;
	int last_vfilter_idx = 0;
#endif

        while (true)
        {
            if (mIsQuit)
            {
                ClearVideoQuene(); //清空队列
                break;
            }

            if (mIsPause == true) //判断暂停
            {
                mSleep(10);
                continue;
            }

            // packet queue没东西 等待
            if (mVideoPacketQueue.GetSize() <= 0)
            {
                if (mIsReadFinished)
                {
                    //队列里面没有数据了且读取完毕了
                    break;
                }
                else
                {
                    mSleep(1); //队列只是暂时没有数据而已
                    continue;
                }
            }


            auto pkt = std::make_shared<Utils::MyPacket>();
            if (mVideoPacketQueue.GetPacket(pkt) == false)
            {
                continue;
            }

            //收到这个数据 说明刚刚执行过跳转 现在需要把解码器的数据 清除一下
            if (strcmp(reinterpret_cast<char *>(pkt->GetData()), FLUSH_DATA) == 0)
            {
                // avcodec_flush_buffers(pCodecCtx);
                pVideoCodecCtx->Flush();
                pkt->Unref();
                continue;
            }
            // * 解码
            if (pVideoCodecCtx->SendPacket(pkt) == false)
            {
                fprintf(stderr, "input AVPacket to decoder failed!\n");
                // av_packet_unref(packet);
                pkt->Unref();
                continue;
            }

            while (pVideoCodecCtx->RecieveFrame(pFrame) == true)
            {
                // 无效
                if (pkt->GetDts() == AV_NOPTS_VALUE && pFrame->opaque &&
                    *static_cast<uint64_t *>(pFrame->opaque) != AV_NOPTS_VALUE)
                {
                    video_pts = static_cast<double>(*static_cast<uint64_t *>(pFrame->opaque));
                }
                else if (pkt->GetDts() != AV_NOPTS_VALUE)
                {
                    video_pts = pkt->GetDts();
                }
                else
                {
                    video_pts = 0;
                }

                // 实际的pts
                video_pts *= av_q2d(mVideoStream->time_base);
                videoClock = video_pts;

                // * 跳转了
                if (seekFlagVideo)
                {
                    //发生了跳转 则跳过关键帧到目的时间的这几帧
                    if (video_pts < seekTime)
                    {
                        pkt->Unref();
                        continue;
                    }
                    else
                    {
                        seekFlagVideo = 0;
                    }
                }

                // * 音视频同步
                // 实现的原理就是，判断是否到显示此帧图像的时间了，没到则休眠5ms，然后继续判断
                while (true)
                {
                    if (mIsQuit)
                    {
                        break;
                    }
                    // 有音频流 更新音频流时钟
                    if (mAudioStream != nullptr && !mIsAudioThreadFinished)
                    {
                        if (mIsReadFinished && mAudioPacketQueue.GetSize() <= 0)
                        {
                            //读取完了 且音频数据也播放完了 就剩下视频数据了  直接显示出来了 不用同步了
                            break;
                        }

                        ///有音频的情况下，将视频同步到音频
                        ///跟音频的pts做对比，比视频快则做延时
                        audio_pts = audioClock;
                    }
                    else
                    {
                        ///没有音频的情况下，直接同步到外部时钟
                        audio_pts = static_cast<double>(av_gettime() - mVideoStartTime) /
                                    1000000.0;
                        audioClock = audio_pts;
                    }

                    //主要是 跳转的时候 我们把video_clock设置成0了
                    //因此这里需要更新video_pts
                    //否则当从后面跳转到前面的时候 会卡在这里
                    video_pts = videoClock;
                    // 音频先被放出来
                    if (video_pts <= audio_pts)
                        break;
                    // 视频比音频快多少
                    int delayTime = static_cast<int>((video_pts - audio_pts) * 1000);

                    delayTime = delayTime > 5 ? 5 : delayTime;

                    if (!mIsNeedPause)
                    {
                        mSleep(delayTime);
                    }
                }
                //#define CONFIG_AVFILTER 0
#if CONFIG_AVFILTER
			if (last_w != pFrame->width
				|| last_h != pFrame->height
				|| last_format != pFrame->format
				//            || last_serial != is->viddec.pkt_serial
				|| last_vfilter_idx != vfilter_idx)
			{
				//            av_log(NULL, AV_LOG_DEBUG,
				//                   "Video frame changed from size:%dx%d format:%s serial:%d to size:%dx%d format:%s serial:%d\n",
				//                   last_w, last_h,
				//                   (const char *)av_x_if_null(av_get_pix_fmt_name(last_format), "none"), last_serial, pFrame->width, pFrame->height,
				//                   (const char *)av_x_if_null(av_get_pix_fmt_name(pFrame->format), "none"), is->viddec.pkt_serial);

				avfilter_graph_free(&graph);
				graph = avfilter_graph_alloc();
				if (!graph)
				{
					//                ret = AVERROR(ENOMEM);
					//                goto the_end;
					continue;
				}
				graph->nb_threads = filter_nbthreads;

				int ret = configure_video_filters(graph, vfilters_list ? vfilters_list[vfilter_idx] : NULL, pFrame);
				if (ret < 0)
				{
					//                SDL_Event event;
					//                event.type = FF_QUIT_EVENT;
					//                event.user.data1 = is;
					//                SDL_PushEvent(&event);
					//                goto the_end;
					continue;
				}
				filt_in = in_video_filter;
				filt_out = out_video_filter;
				last_w = pFrame->width;
				last_h = pFrame->height;
				last_format = pFrame->format;
				//            last_serial = is->viddec.pkt_serial;
				last_vfilter_idx = vfilter_idx;
				//            frame_rate = av_buffersink_get_frame_rate(filt_out);
			}

			int ret = av_buffersrc_add_frame(filt_in, pFrame);
			if (ret < 0)
			{
				//            goto the_end;
				continue;
			}

			while (ret >= 0)
			{
				//            is->frame_last_returned_time = av_gettime_relative() / 1000000.0;

				ret = av_buffersink_get_frame_flags(filt_out, pFrame, 0);
				if (ret < 0)
				{
					//                if (ret == AVERROR_EOF)
					//                    is->viddec.finished = is->viddec.pkt_serial;
					ret = 0;
					break;
				}

				//            frame_last_filter_delay = av_gettime_relative() / 1000000.0 - is->frame_last_returned_time;
				//            if (fabs(is->frame_last_filter_delay) > AV_NOSYNC_THRESHOLD / 10.0)
				//                is->frame_last_filter_delay = 0;
				//            tb = av_buffersink_get_time_base(filt_out);
#endif

                // 分辨率改变了 要更新图像转换
                if (pFrame->width != videoWidth || pFrame->height != videoHeight)
                {
                    videoWidth = pFrame->width;
                    videoHeight = pFrame->height;

                    if (pFrameYUV != nullptr)
                    {
                        pFrameYUV.reset();
                    }

                    if (yuv420pBuffer != nullptr)
                    {
                        av_free(yuv420pBuffer);
                    }

                    if (pVideoSwsCtx)
                    {
                        pVideoSwsCtx.reset();
                    }

                    // pFrameYUV = av_frame_alloc();
                    pFrameYUV = std::make_shared<Utils::MyFrame>();

                    const int yuvSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, videoWidth, videoHeight, 1);
                    //按1字节进行内存对齐,得到的内存大小最接近实际大小
                    //    int yuvSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 0);  //按0字节进行内存对齐，得到的内存大小是0
                    //    int yuvSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 4);   //按4字节进行内存对齐，得到的内存大小稍微大一些

                    const auto numBytes = static_cast<unsigned int>(yuvSize);
                    yuv420pBuffer = static_cast<uint8_t *>(av_malloc(
                        numBytes * sizeof(uint8_t)));

                    pFrameYUV->FillArray(yuv420pBuffer, AV_PIX_FMT_YUV420P,
                                         videoWidth, videoHeight, 1);

                    ///由于解码后的数据不一定都是yuv420p，因此需要将解码后的数据统一转换成YUV420P
                    // imgConvertCtx = sws_getContext(videoWidth, videoHeight,
                    //                                (AVPixelFormat) pFrame->format, videoWidth, videoHeight,
                    //                                AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
                    pVideoSwsCtx = std::make_unique<Utils::MySwsContext>(
                        videoWidth, videoHeight,
                        static_cast<AVPixelFormat>(pFrame->format),
                        videoWidth, videoHeight,
                        AV_PIX_FMT_YUV420P
                    );
                }

                pVideoSwsCtx->Scale(pFrame->data,
                                    pFrame->linesize, 0,
                                    videoHeight, pFrameYUV->GetData(),
                                    pFrameYUV->GetLineSize());

                doDisplayVideo(yuv420pBuffer, videoWidth, videoHeight);

#if CONFIG_AVFILTER

			}
#endif
                if (mIsNeedPause)
                {
                    mIsPause = true;
                    mIsNeedPause = false;
                }
            }
            pkt->Unref();
        }

#if CONFIG_AVFILTER
	avfilter_graph_free(&graph);
#endif

        av_free(pFrame);

        if (pFrameYUV != nullptr)
        {
            pFrameYUV.reset();
        }

        if (yuv420pBuffer != nullptr)
        {
            av_free(yuv420pBuffer);
        }

        if (pVideoSwsCtx)
        {
            pVideoSwsCtx.reset();
        }

        if (!mIsQuit)
        {
            mIsQuit = true;
        }

        mIsVideoThreadFinished = true;

        fprintf(stderr, "%s finished \n", __FUNCTION__);
    }

    void PlayerCore::SdlAudioCallBackFunc(void *userdata, Uint8 *stream, int len)
    {
        const auto player = static_cast<PlayerCore *>(userdata);
        player->SdlAudioCallBack(stream, len);
    }

    void PlayerCore::SdlAudioCallBack(Uint8 *stream, int len)
    {
        /*   len是由SDL传入的SDL缓冲区的大小，如果这个缓冲未满，我们就一直往里填充数据 */
        while (len > 0)
        {
            /*  audio_buf_index 和 audio_buf_size 标示我们自己用来放置解码出来的数据的缓冲区，*/
            /*   这些数据待copy到SDL缓冲区， 当audio_buf_index >= audio_buf_size的时候意味着我*/
            /*   们的缓冲为空，没有数据可供copy，这时候需要调用audio_decode_frame来解码出更
            /*   多的桢数据 */
            if (audioBufferIdx >= audioBufferSize)
            {
                int audio_data_size = DecodeAudioFrame();

                /* audio_data_size < 0 标示没能解码出数据，我们默认播放静音 */
                if (audio_data_size <= 0)
                {
                    /* silence */
                    audioBufferSize = 1024;
                    /* 清零，静音 */
                    memset(audioBuffer, 0, audioBufferSize);
                }
                else
                {
                    audioBufferSize = audio_data_size;
                }
                audioBufferIdx = 0;
            }
            /*  查看stream可用空间，决定一次copy多少数据，剩下的下次继续copy */
            int len1 = audioBufferSize - audioBufferIdx;

            if (len1 > len)
            {
                len1 = len;
            }

            if (audioBuffer == nullptr)
                return;

            if (mIsMute || mIsNeedPause) //静音 或者 是在暂停的时候跳转了
            {
                memset(audioBuffer + audioBufferIdx, 0, len1);
            }
            else
            {
                // 调整PCM音频数据的音量
                RaiseVolume(reinterpret_cast<char *>(audioBuffer) + audioBufferIdx,
                            len1, 1, mVolume);
            }

            memcpy(stream, static_cast<uint8_t *>(audioBuffer) + audioBufferIdx,
                   len1);

            //        SDL_memset(stream, 0x0, len);// make sure this is silence.
            //        SDL_MixAudio(stream, (uint8_t *) audio_buf + audio_buf_index, len1, SDL_MIX_MAXVOLUME);

            //        SDL_MixAudio(stream, (uint8_t * )is->audio_buf + is->audio_buf_index, len1, 50);
            //        SDL_MixAudioFormat(stream, (uint8_t * )is->audio_buf + is->audio_buf_index, AUDIO_S16SYS, len1, 50);

            len -= len1;
            stream += len1;
            audioBufferIdx += len1;
        }
    }

    int PlayerCore::DecodeAudioFrame(bool isBlock)
    {
        int audioBufferSize = 0;

        while (true)
        {
            if (mIsQuit)
            {
                mIsAudioThreadFinished = true;
                ClearAudioQuene(); //清空队列
                break;
            }

            if (mIsPause == true) //判断暂停
            {
                break;
            }

            std::unique_lock<std::mutex> audioLock(mAudioPacketQueue.mutex);
            if (mAudioPacketQueue.GetSize() <= 0)
            {
                if (isBlock)
                {
                    mAudioPacketQueue.cond.wait_for(audioLock,
                                                    std::chrono::seconds(1000));
                }
                else
                {
                    audioLock.unlock();
                    break;
                }
            }


            std::shared_ptr<Utils::MyPacket> pkt;
            mAudioPacketQueue.GetPacket(pkt);
            audioLock.unlock();

            if (pkt->GetPts() != AV_NOPTS_VALUE)
            {
                audioClock = av_q2d(mAudioStream->time_base) * pkt->GetPts();
            }

            // 收到这个数据 说明刚刚执行过跳转 现在需要把解码器的数据 清除一下
            // if (strcmp((char *) pkt->data,FLUSH_DATA) == 0)
            if (strcmp(reinterpret_cast<char *>(pkt->GetData()),FLUSH_DATA) == 0)
            {
                // avcodec_flush_buffers(this->aCodecCtx);
                // av_packet_unref(pkt);
                pAudioCodecCtx->Flush();
                pkt->Unref();
                continue;
            }

            if (seekFlagAudio)
            {
                //发生了跳转 则跳过关键帧到目的时间的这几帧
                if (audioClock < seekTime)
                {
                    continue;
                }
                else
                {
                    seekFlagAudio = 0;
                }
            }


            pAudioCodecCtx->SendPacket(pkt);

            pkt->Unref();

            while (pAudioCodecCtx->RecieveFrame(audioFrame) == true)
            {
                /// ffmpeg解码之后得到的音频数据不是SDL想要的，
                /// 因此这里需要重采样成44100 双声道 AV_SAMPLE_FMT_S16

                /// 需要保证重采样后音频的时间是相同的，不同采样率下同样时间采集的数据采样点个数肯定不一样。
                /// 因此就需要重新计算采样点个数（使用下面的函数）
                /// 将in_sample_rate的采样次数换算成out_sample_rate对应的采样次数
                const int nb_samples = static_cast<int>(av_rescale_rnd(
                    pAudioSwrCtx->GetDelay(outSampleRate) + audioFrame->nb_samples,
                    outSampleRate, inSampleRate, AV_ROUND_UP
                ));

                if (audioFrameReSample != nullptr)
                {
                    // if (audioFrameReSample->nb_samples != nb_samples)
                    if (audioFrameReSample->GetAudioNBSamples() != nb_samples)
                    {
                        // av_frame_free(&audioFrameReSample);
                        audioFrameReSample.reset();
                        audioFrameReSample = nullptr;
                    }
                }

                ///解码一帧后才能获取到采样率等信息，因此将初始化放到这里
                if (audioFrameReSample == nullptr)
                {
                    // audioFrameReSample = av_frame_alloc();
                    audioFrameReSample = std::make_shared<Utils::MyFrame>();

                    // audioFrameReSample->format = outSampleFmt;
                    audioFrameReSample->SetAudioSampleFmt(outSampleFmt);
                    // av_channel_layout_copy(&audioFrameReSample->ch_layout,
                    //                        &outCHLayout);
                    audioFrameReSample->SetCHLayout(outCHLayout);
                    // audioFrameReSample->sample_rate = outSampleRate;
                    audioFrameReSample->SetAudioSampleRate(outSampleRate);
                    // audioFrameReSample->nb_samples = nb_samples;
                    audioFrameReSample->SetAudioNBSamples(nb_samples);


                    // int ret = av_samples_fill_arrays(
                    //     audioFrameReSample->data,
                    //     audioFrameReSample->linesize,
                    //     audioBuffer,
                    //     outCHLayout.nb_channels,
                    //     audioFrameReSample->nb_samples,
                    //     outSampleFmt,
                    //     0);
                    bool ret = audioFrameReSample->FillArray(
                        audioBuffer,
                        outCHLayout.nb_channels,
                        audioFrameReSample->GetAudioNBSamples(),
                        outSampleFmt,
                        0
                    );
                    if (ret == false)
                    {
                        fprintf(stderr, "Error allocating an audio buffer\n");
                        //                        exit(1);
                    }
                }

                // * 重采样
                // pAudioSwrCtx->Convert(
                //     const_cast<const uint8_t **>(audioFrame->data),
                //     audioFrame->nb_samples,
                //     audioFrameReSample->data,
                //     audioFrameReSample->nb_samples
                // );
                pAudioSwrCtx->Convert(
                    const_cast<const uint8_t **>(audioFrame->data),
                    audioFrame->nb_samples,
                    audioFrameReSample->GetData(),
                    audioFrameReSample->GetAudioNBSamples()
                );

                ///下面这两种方法计算的大小是一样的
#if 0
            int resampled_data_size = len2 * audio_tgt_channels * av_get_bytes_per_sample(out_sample_fmt);
#else
                int resampled_data_size = av_samples_get_buffer_size(
                    nullptr, outCHLayout.nb_channels,
                    // audioFrameReSample->nb_samples,
                    audioFrameReSample->GetAudioNBSamples(),
                    outSampleFmt, 1);
#endif
                //qDebug()<<__FUNCTION__<<resampled_data_size<<aFrame_ReSample->nb_samples<<aFrame->nb_samples;
                audioBufferSize += resampled_data_size;
            }
            break;
        }

        return audioBufferSize;
    }

    bool PlayerCore::InputVideoQuene(AVPacket &_pkt)
    {
        if (!_pkt.data)
        {
            return false;
        }

        // mConditon_Video->Lock();
        // mVideoPacktList.push_back(pkt);
        const auto pkt = std::make_shared<Utils::MyPacket>(&_pkt);
        mVideoPacketQueue.PushPacket(pkt);
        mVideoPacketQueue.cond.notify_one();
        // mConditon_Video->Signal();
        // mConditon_Video->Unlock();

        return true;
    }

    bool PlayerCore::InputVideoQuene(const std::shared_ptr<Utils::MyPacket> &_pkt)
    {
        if (!_pkt || !_pkt->GetData())
        {
            return false;
        }

        // mConditon_Video->Lock();
        // mVideoPacktList.push_back(pkt);
        bool ret = mVideoPacketQueue.PushPacket(_pkt);
        mVideoPacketQueue.cond.notify_one();
        // mConditon_Video->Signal();
        // mConditon_Video->Unlock();

        return ret;
    }

    void PlayerCore::ClearVideoQuene()
    {
        // mConditon_Video->Lock();
        // for (AVPacket pkt: mVideoPacktList)
        // {
        // 	//        av_free_packet(&pkt);
        // 	av_packet_unref(&pkt);
        // }
        // mVideoPacktList.clear();
        mVideoPacketQueue.Clear();
        // mConditon_Video->Unlock();
    }

    bool PlayerCore::InputAudioQuene(AVPacket &_pkt)
    {
        if (!_pkt.data)
        {
            return false;
        }

        // mConditon_Audio->Lock();
        // mAudioPacktList.push_back(pkt);
        auto pkt = std::make_shared<Utils::MyPacket>(&_pkt);
        mAudioPacketQueue.PushPacket(pkt);
        mAudioPacketQueue.cond.notify_one();
        // mConditon_Audio->Signal();
        // mConditon_Audio->Unlock();

        return true;
    }

    bool PlayerCore::InputAudioQuene(const std::shared_ptr<Utils::MyPacket> &_pkt)
    {
        if (!_pkt || !_pkt->GetData())
        {
            return false;
        }

        // mConditon_Audio->Lock();
        // mAudioPacktList.push_back(pkt);
        // auto pkt = std::make_shared<Utils::MyPacket>(&_pkt);
        mAudioPacketQueue.PushPacket(_pkt);
        mAudioPacketQueue.cond.notify_one();
        // mConditon_Audio->Signal();
        // mConditon_Audio->Unlock();

        return true;
    }

    void PlayerCore::ClearAudioQuene()
    {
        // mConditon_Audio->Lock();
        // for (AVPacket pkt: mAudioPacktList)
        // {
        // 	//        av_free_packet(&pkt);
        // 	av_packet_unref(&pkt);
        // }
        // mAudioPacktList.clear();
        mAudioPacketQueue.Clear();
        // mConditon_Audio->Unlock();
    }

    void PlayerCore::Release()
    {
        ClearAudioQuene();
        ClearVideoQuene();
        // 不是外部调用的stop 是正常播放结束
        if (mPlayerState != STOP)
        {
            Stop();
        }
        // 确保其他线程结束后 再销毁队列
        while ((mVideoStream != nullptr && !mIsVideoThreadFinished) ||
               (mAudioStream != nullptr && !mIsAudioThreadFinished))
        {
            mSleep(10);
        }

        CloseSDL();

        if (pAudioSwrCtx)
        {
            pAudioSwrCtx.reset();
            pAudioSwrCtx = nullptr;
        }

        if (audioFrame != nullptr)
        {
            av_frame_free(&audioFrame);
            audioFrame = nullptr;
        }

        if (audioFrameReSample != nullptr)
        {
            // av_frame_free(&audioFrameReSample);
            audioFrameReSample.reset();
            audioFrameReSample = nullptr;
        }

        if (pAudioCodecCtx)
        {
            pAudioCodecCtx.reset();
            pAudioCodecCtx = nullptr;
        }

        if (pVideoCodecCtx)
        {
            pVideoCodecCtx.reset();
            pVideoCodecCtx = nullptr;
        };
        pFmtCtx.reset();

        SDL_Quit();

        doPlayerStateChanged(STOP, mVideoStream != nullptr, mAudioStream != nullptr);

        mIsReadThreadFinished = true;
    }

    int PlayerCore::OpenSDL()
    {
        ///打开SDL，并设置播放的格式为:AUDIO_S16LSB 双声道，44100hz
        ///后期使用ffmpeg解码完音频后，需要重采样成和这个一样的格式，否则播放会有杂音
        SDL_AudioSpec wanted_spec, spec;
        int wanted_nb_channels = 2;
        //    int samplerate = 44100;
        // int samplerate = out_sample_rate;
        int samplerate = outSampleRate;

        wanted_spec.channels = wanted_nb_channels;
        wanted_spec.freq = samplerate;
        // 512 最小样本数
        wanted_spec.samples = FFMAX(512, 2 << av_log2(wanted_spec.freq / 30));
        wanted_spec.format = AUDIO_S16SYS; // 具体含义请查看“SDL宏定义”部分
        wanted_spec.silence = 0;           // 0指示静音
        //    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;  // 自定义SDL缓冲区大小
        wanted_spec.callback = SdlAudioCallBackFunc; // 回调函数
        wanted_spec.userdata = this;                 // 传给上面回调函数的外带数据

        // 获取当前驱动程序暴露的可用设备数量
        const int num = SDL_GetNumAudioDevices(0);
        for (int i = 0; i < num; i++)
        {
            // SDL_OpenAudioDevice 打开音频设备
            // SDL_GetAudioDeviceName  获取特定音频设备的人可读名称
            mAudioID = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(i, 0), false, &wanted_spec, &spec, 0);
            if (mAudioID > 0)
            {
                break;
            }
        }

        /* 检查实际使用的配置（保存在spec,由SDL_OpenAudio()填充） */
        //    if (spec.format != AUDIO_S16SYS)
        if (mAudioID <= 0)
        {
            // 打不开音频
            mIsAudioThreadFinished = true;
            return -1;
        }
        fprintf(stderr, "mAudioID=%d\n\n\n\n\n\n", mAudioID);
        return 0;
    }

    void PlayerCore::CloseSDL()
    {
        if (mAudioID > 0)
        {
            SDL_LockAudioDevice(mAudioID);
            // 暂停和取消音频回调处理
            SDL_PauseAudioDevice(mAudioID, 1);
            SDL_UnlockAudioDevice(mAudioID);

            SDL_CloseAudioDevice(mAudioID);
        }

        mAudioID = 0;
    }

    void PlayerCore::doOpenVideoFileFailed(const int &code) const
    {
        fprintf(stderr, "%s \n", __FUNCTION__);

        if (mVideoPlayerCallBack != nullptr)
        {
            mVideoPlayerCallBack->onOpenVideoFileFailed(code);
        }
    }

    void PlayerCore::doOpenSdlFailed() const
    {
        fprintf(stderr, "%s \n", __FUNCTION__);

        if (mVideoPlayerCallBack != nullptr)
        {
            mVideoPlayerCallBack->onOpenSdlFailed();
        }
    }

    void PlayerCore::doTotalTimeChanged(const int64_t &uSec) const
    {
        fprintf(stderr, "%s \n", __FUNCTION__);

        if (mVideoPlayerCallBack != nullptr)
        {
            mVideoPlayerCallBack->onTotalTimeChanged(uSec);
        }
    }

    void PlayerCore::doPlayerStateChanged(const PlayerState &state,
                                          const bool &hasVideo, const bool &hasAudio) const
    {
        fprintf(stderr, "%s \n", __FUNCTION__);

        if (mVideoPlayerCallBack != nullptr)
        {
            mVideoPlayerCallBack->onPlayerStateChanged(state, hasVideo, hasAudio);
        }
    }

    void PlayerCore::doDisplayVideo(const uint8_t *yuv420Buffer,
                                    const int &width, const int &height) const
    {
        //    fprintf(stderr, "%s \n", __FUNCTION__);
        if (mVideoPlayerCallBack != nullptr)
        {
            const auto videoFrame = std::make_shared<Utils::MyYUVFrame>();
            videoFrame->initBuffer(width, height);
            videoFrame->setYUVbuf(yuv420Buffer);

            mVideoPlayerCallBack->onDisplayVideo(videoFrame);
        }
    }
} // PlayerCore
