#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QQuickPaintedItem>
#include <QImage>
#include <QPainter>
#include <QThread>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/rational.h>
#include <libavutil/time.h>
}

class DecoderThread : public QThread {
    Q_OBJECT
public:
    DecoderThread(QObject *parent = nullptr);
    void run() override;
    void setup(AVFormatContext *fmt_ctx, AVCodecContext *dec_ctx, SwsContext *sws_ctx, int video_stream_idx);

signals:
    void frameReady(QImage frame);

private:
    AVFormatContext *fmt_ctx;
    AVCodecContext *dec_ctx;
    SwsContext *sws_ctx;
    int video_stream_idx;
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
    AVCodecContext* dec_ctx;
    SwsContext* sws_ctx;
    AVFrame* frame;
    AVPacket* packet;
    QImage image;
    int video_stream_idx;
    DecoderThread decoderThread;
};

#endif // VIDEOPLAYER_H
