//
// Created by 10484 on 2024/5/2.
//

#ifndef CLIENT_MYQUEUE_H
#define CLIENT_MYQUEUE_H

#include "concurrentqueue.h"
#include "MyPacket.h"
#include "MyFrame.h"
#include <iostream>

namespace Utils
{
    class MyPacketQueue
    {
    public:
        explicit MyPacketQueue(const size_t len)
        {
            this->length = len;
        }

        bool GetPacket(std::shared_ptr<Utils::MyPacket> &pkt)
        {
            bool ret = this->pktQueue.try_dequeue(pkt);
            if (!ret)
            {
                std::cerr << __FILE__ << __FUNCTION__ << "try_dequeue failed" << std::endl;
                return false;
            }
            return true;
        }

        bool PushPacket(const std::shared_ptr<Utils::MyPacket> &pkt)
        {
            if (!pkt || !pkt->GetPkt())
            {
                std::cerr << __FILE__ << __FUNCTION__ << "pkt is null" << std::endl;
                return false;
            }
            bool ret = this->pktQueue.enqueue(pkt);
            if (!ret)
            {
                std::cerr << __FILE__ << __FUNCTION__ << "try_dequeue failed" << std::endl;
                return false;
            }
            return true;
        }

        size_t GetSize() const
        {
            return this->pktQueue.size_approx();
        }

        size_t GetLength() const
        {
            return length;
        }
        void Clear()
        {
            std::unique_lock<std::mutex> lock;
            std::shared_ptr<Utils::MyPacket> pkt = std::make_shared<Utils::MyPacket>();
            int ret;
            while (pktQueue.size_approx())
            {
                pktQueue.try_dequeue(pkt);
                pkt.reset();
                ret = pktQueue.size_approx();
            }
        }

    private:
        // 实际的队列
        moodycamel::ConcurrentQueue<std::shared_ptr<Utils::MyPacket> > pktQueue;
        // 能够容纳的帧数
        size_t length = 0;

    public:
        /// 用来保证出队入队的线程安全
        /// @note 出队或入队时，需要上锁
        std::mutex mutex;
        /// 用来让队列出队入队时更加的平滑
        /// @note 假如需要出队 但是队列中没有数据 就等待
        std::condition_variable cond;
    };

    class MyFrameQueue
    {
    public:
        explicit MyFrameQueue(const size_t len)
        {
            this->length = len;
        }

        bool GetFrame(std::shared_ptr<Utils::MyFrame> &frame)
        {
            if (!frame || !frame->GetFrame())
            {
                std::cerr << __FILE__ << __FUNCTION__ << "frame is null" << std::endl;
                return false;
            }
            auto size = this->frameQueue.size_approx();
            bool ret = this->frameQueue.try_dequeue(frame);
            size = this->frameQueue.size_approx();
            if (!ret)
            {
                std::cerr << __FILE__ << __FUNCTION__ << "try_dequeue failed" << std::endl;
                return ret;
            }
            return ret;
        }

        bool PushFrame(const std::shared_ptr<Utils::MyFrame> &frame)
        {
            if (!frame || !frame->GetFrame() || !frame->GetFrame()->data)
            {
                std::cerr << __FILE__ << __FUNCTION__ << "frame is null" << std::endl;
                return false;
            }
            if (this->length <= this->GetSize())
                return false;
            bool ret = this->frameQueue.enqueue(frame);
            if (!ret)
            {
                std::cerr << __FILE__ << __FUNCTION__ << "try_dequeue failed" << std::endl;
                return false;
            }
            return true;
        }

        size_t GetSize() const
        {
            return frameQueue.size_approx();
        }

        size_t GetLength() const
        {
            return length;
        }

    private:
        moodycamel::ConcurrentQueue<std::shared_ptr<Utils::MyFrame> > frameQueue;
        size_t length = 0;

    public:
        std::mutex mutex;
        std::condition_variable cond;
    };
} // Utils

#endif //CLIENT_MYQUEUE_H
