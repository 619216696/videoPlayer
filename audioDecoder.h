#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include <QThread>
#include "./decoderBase.h"

extern "C" {
#include <libswresample/swresample.h>
}

class AudioDecoder: public QThread, public DecoderBase {
    Q_OBJECT
public:
    AudioDecoder();
    ~AudioDecoder();

    bool init(AVStream* stream);
    inline int getSampleRate() { return m_pDecCtx->sample_rate; }

protected:
    void run() override;

signals:
    void frameTimeUpdate(qint64 frameTime);
    void frameReady(QByteArray buffer);

private:
    // 开始播放的时间 单位微秒
    qint64 m_nStartTime = 0;
    SwrContext* m_pSwrCtx = nullptr;
};

#endif // AUDIODECODER_H
