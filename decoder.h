#ifndef DECODER_H
#define DECODER_H

#include "videoDecoder.h"
#include "audioDecoder.h"
#include <QObject>
#include <QThread>
#include <QString>

extern "C" {
#include <libavformat/avformat.h>
}

class Decoder : public QThread
{
    Q_OBJECT
public:
    Decoder();
    ~Decoder();

    bool init(const QString& uri, bool useHardwareDecoder = false);
    void setPlayState(bool play);
    void seekToPosition(qint64 second);

    // 获取视频总时长 单位秒
    inline qint64 getTotleTime() { return m_nDuration; }
    // 获取当前播放时间 单位秒
    inline qint64 getPlayTime() { return m_audioDecoder.getFrameTime() / AV_TIME_BASE; }

signals:
    void frameReady(QImage frame);

protected:
    void run() override;

private:
    QString m_strUri = "";
    AVFormatContext* m_pFmtCtx = nullptr;
    VideoDecoder m_videoDecoder;
    AudioDecoder m_audioDecoder;
    qint64 m_nVideoStreamIdx = -1;
    qint64 m_nAudioStreamIdx = -1;
    // 总播放时长 单位秒
    qint64 m_nDuration = 0;
    bool m_bPlaying = false;
    // 跳转的时间 单位微秒
    qint64 m_nSeekTime = -1;
};

#endif // DECODER_H
