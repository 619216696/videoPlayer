#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QThread>
#include <QImage>
#include "./decoderBase.h"

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class VideoDecoder : public QThread, DecoderBase {
    Q_OBJECT
public:
    VideoDecoder(QObject* parent = nullptr);
    ~VideoDecoder();

    void run() override;

    bool init(const QString& uri, bool useHw);
    void release();

signals:
    void frameReady(QImage frame);

public slots:
    void play() override;
    void stop() override;
    void seekToPosition(qint64 timestamp) override;
    void audioFrameTimeUpdate(qint64 frameTime);

private:
    QString uri = "";
    bool useHw = false;
    AVBufferRef* hw_device_ctx = nullptr;
    SwsContext* sws_ctx = nullptr;
    qint64 audioFrameTime = 0;
};

#endif // VIDEODECODER_H
