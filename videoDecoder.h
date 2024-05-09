#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include "decoderBase.h"
#include <QThread>
#include <QImage>

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class VideoDecoder : public QThread, public DecoderBase {
    Q_OBJECT
public:
    VideoDecoder();
    ~VideoDecoder();

    bool init(AVStream* stream, bool useHardwareDecoder);

protected:
    void run() override;

signals:
    void frameReady(QImage frame);

public slots:
    void audioFrameTimeUpdate(qint64 frameTime);

private:
    qint64 audioFrameTime = 0;
    SwsContext* m_swsCtx = nullptr;
};

#endif // VIDEODECODER_H
