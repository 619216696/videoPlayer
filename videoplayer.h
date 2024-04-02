#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QQuickPaintedItem>
#include <QImage>
#include <QPainter>
#include <QThread>
#include <QAudioOutput>
#include <QAudioSink>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/rational.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
}

class DecoderThread : public QThread {
    Q_OBJECT
public:
    DecoderThread(QObject* parent = nullptr);
    ~DecoderThread();

    void run() override;
    bool setup(AVFormatContext* fmt_ctx, AVCodecContext* video_dec_ctx, int video_stream_idx, AVCodecContext* audio_dec_ctx, int audio_stream_idx, SwsContext* sws_ctx);

signals:
    void frameReady(QImage frame);

private:
    AVFormatContext* fmt_ctx;
    AVCodecContext* video_dec_ctx;
    AVCodecContext* audio_dec_ctx;
    SwsContext* sws_ctx;
    SwrContext* swr_ctx;
    int video_stream_idx;
    int audio_stream_idx;
    QAudioOutput* audioOutput;
    QIODevice* audioDevice;
    QAudioSink* audioSink;
};

class VideoPlayer : public QQuickPaintedItem {
    Q_OBJECT
public:
    VideoPlayer(QQuickItem* parent = nullptr);

    ~VideoPlayer();

    Q_INVOKABLE bool loadVideo(const QString& filename);

protected:
    void paint(QPainter* painter) override;

private:
    void unloadVideo();

signals:
    void startDecoding();

private slots:
    void onFrameReady(QImage frame);
    void startDecoderThread();

private:
    AVFormatContext* fmt_ctx;
    AVCodecContext* video_dec_ctx;
    AVCodecContext* audio_dec_ctx;
    SwsContext* sws_ctx;
    int video_stream_idx;
    int audio_stream_idx;
    QImage image;
    DecoderThread decoderThread;
};

#endif // VIDEOPLAYER_H
