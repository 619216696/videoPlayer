#ifndef DECODER_H
#define DECODER_H

#include <QThread>
#include <QAudioSink>
#include <QImage>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/rational.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
}

struct DecoderInitParams
{
    DecoderInitParams(const QString& uri, bool useHwDecoder = false): uri(uri), useHwDecoder(useHwDecoder) {}

    QString uri;
    bool useHwDecoder;
};

class DecoderThread : public QThread {
    Q_OBJECT
public:
    DecoderThread(QObject* parent = nullptr);
    ~DecoderThread();

    void run() override;
    bool init(const DecoderInitParams& params);
    void release();
    void play();
    void stop();

signals:
    void videoFrameReady(QImage frame);

private:
    QString uri = "";
    bool useHw = false;
    int video_stream_idx = -1;
    int audio_stream_idx = -1;
    AVFormatContext* fmt_ctx = nullptr;
    AVCodecContext* video_dec_ctx = nullptr;
    AVCodecContext* audio_dec_ctx = nullptr;
    AVBufferRef* hw_device_ctx = nullptr;
    SwsContext* sws_ctx = nullptr;
    SwrContext* swr_ctx = nullptr;
    QAudioSink* audioSink = nullptr;
    QIODevice* audioDevice = nullptr;
};

#endif // DECODER_H
