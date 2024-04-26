#ifndef DECODERBASE_H
#define DECODERBASE_H

#include <mutex>
#include <condition_variable>
#include <atomic>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
}

class DecoderBase {
public:
    DecoderBase() {};
    virtual ~DecoderBase() {};

    virtual void play() = 0;
    virtual void stop() = 0;
    virtual void seekToPosition(int64_t timestamp) = 0;
    virtual bool decodeOneFrame() = 0;

protected:
    unsigned int stream_idx = -1;
    AVFormatContext* fmt_ctx = nullptr;
    AVCodecContext* dec_ctx = nullptr;
    int64_t frameTime = 0;
    std::atomic<bool> playing = false;
    std::mutex mutex;
    std::condition_variable cv;
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;
    AVRational timeBase;
};

#endif // DECODERBASE_H
