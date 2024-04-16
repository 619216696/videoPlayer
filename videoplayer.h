#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QQuickPaintedItem>
#include <QImage>
#include "videoDecoder.h"
#include "audioDecoder.h"

class VideoPlayer : public QQuickPaintedItem {
    Q_OBJECT
public:
    VideoPlayer(QQuickItem* parent = nullptr);
    ~VideoPlayer();

    Q_INVOKABLE bool loadVideo(const QString& filename, bool hw = false);

protected:
    void paint(QPainter* painter) override;

signals:
    void startDecoding();

private slots:
    void onFrameReady(QImage frame);
    void startDecoderThread();

private:
    VideoDecoder videoDecoder;
    AudioDecoder audioDecoder;
    QImage image;
};

#endif // VIDEOPLAYER_H
