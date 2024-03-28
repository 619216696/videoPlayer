#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <QQuickPaintedItem>
#include <QPainter>
#include <QImage>
#include <QDebug>

class VideoPlayer : public QQuickPaintedItem
{
    Q_OBJECT
public:
    VideoPlayer(QQuickItem* parent = nullptr);

    ~VideoPlayer();

    Q_INVOKABLE bool loadVideo(const QString& filename);

protected:
    void timerEvent(QTimerEvent* event) override;

    void paint(QPainter* painter) override;

private:
    AVFormatContext* fmt_ctx;
    AVCodecContext* dec_ctx;
    SwsContext* sws_ctx;
    AVFrame* frame;
    AVPacket* packet;
    QImage image;
    int fps;
    int video_stream_idx;

    void decodeAndRenderNextFrame();
    void unloadVideo();
};

#endif // VIDEOPLAYER_H
