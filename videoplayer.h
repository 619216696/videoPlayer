#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QQuickPaintedItem>
#include <QImage>
#include "decoder.h"
#include <QAudioSink>

class VideoPlayer : public QQuickPaintedItem {
    Q_OBJECT
public:
    VideoPlayer(QQuickItem* parent = nullptr);
    ~VideoPlayer();

    // 加载视频
    Q_INVOKABLE bool loadVideo(const QString& filename, bool hw = false);
    // 设置暂停/播放
    Q_INVOKABLE inline void setPlayState(bool play) {
        m_decoder.setPlayState(play);
        setPlaying(play);
    }
    // 获取视频总时长 单位秒
    Q_INVOKABLE inline qint64 getVideoTotleTime() { return m_decoder.getTotleTime(); };
    // 获取当前播放时间 单位秒
    Q_INVOKABLE inline qint64 getPlayTime() { return m_decoder.getPlayTime(); }
    // 跳转到某位置 单位秒
    Q_INVOKABLE inline void seekToPosition(qint64 second) { m_decoder.seekToPosition(second); }

    Q_PROPERTY(bool playing MEMBER m_bPlaying WRITE setPlaying NOTIFY playingChange)

    void setPlaying(bool playing);

    Q_PROPERTY(int volumn MEMBER m_nVolumn WRITE setVolumn NOTIFY volumnChanged)

    void setVolumn(int volumn);

signals:
    void playingChange();
    void volumnChanged(int volumn);

protected:
    void paint(QPainter* painter) override;

private slots:
    void onVideoFrameReady(QImage frame);
    void onAudioFrameReady(QByteArray buffer);
    void onVolunmChange(int volumn);

private:
    Decoder m_decoder;
    QImage m_image;
    bool m_bPlaying = false;
    QAudioSink* m_pAudioSink = nullptr;
    QIODevice* m_pAudioDevice = nullptr;
    // 音量
    int m_nVolumn = 80;
};

#endif // VIDEOPLAYER_H
