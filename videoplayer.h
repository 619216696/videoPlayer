#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QQuickPaintedItem>
#include <QImage>
#include "decoder.h"

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
    DecoderThread decoderThread;
    QImage image;
};

#endif // VIDEOPLAYER_H
