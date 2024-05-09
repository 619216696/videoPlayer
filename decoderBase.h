#ifndef DECODERBASE_H
#define DECODERBASE_H

#include <mutex>
#include <condition_variable>
#include <QQueue>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
}

class DecoderBase {
public:
    DecoderBase() {};
    virtual ~DecoderBase() {};

    inline void play() {
        m_bPlaying = true;
        m_cv.notify_all();
    }

    inline void stop() {
       m_bPlaying = false;
    }

    inline void addToQueue(AVPacket* packet) {
        m_mutex.lock();
        m_queue.enqueue(packet);
        m_mutex.unlock();
    }

    inline qsizetype queueSize() {
        return m_queue.size();
    }

    inline qint64 getFrameTime() {
        return m_nFrameTime;
    }

    inline void seekToPosition(qint64 seekTime) {
        m_mutex.lock();
        for (auto packet : m_queue) {
            av_packet_unref(packet);
            av_packet_free(&packet);
        }
        m_queue.clear();
        m_bSeekFlag = true;
        m_nSeekTime = seekTime;
        m_mutex.unlock();
    }

protected:
    // 帧时间 单位微秒
    qint64 m_nFrameTime = 0;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    AVStream* m_pStream = nullptr;
    AVCodecContext* m_pDecCtx = nullptr;
    AVRational m_timeBase;
    bool m_bPlaying = false;
    QQueue<AVPacket*> m_queue;
    // 跳转标志
    bool m_bSeekFlag = false;
    // 跳转时间 单位微秒
    qint64 m_nSeekTime = -1;
};

#endif // DECODERBASE_H
