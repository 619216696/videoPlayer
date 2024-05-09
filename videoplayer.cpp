#include "videoplayer.h"
#include <QDebug>
#include <QPainter>

VideoPlayer::VideoPlayer(QQuickItem* parent) : QQuickPaintedItem(parent) {}

VideoPlayer::~VideoPlayer() {
    if (m_decoder.isRunning()) {
        m_decoder.requestInterruption();
        m_decoder.wait();
    }
}

bool VideoPlayer::loadVideo(const QString& filePath, bool useHw) {
    QUrl fileUrl(filePath);
    QString localPath = fileUrl.toLocalFile();
    qDebug() << "loadVideo path: " << localPath;

    // 设置解码线程的上下文
    if (!m_decoder.init(localPath, useHw)) {
        qCritical() << "decoder thread init failed";
        return false;
    }

    connect(&m_decoder, &Decoder::frameReady, this, &VideoPlayer::onFrameReady);

    // 开始线程解码
    m_decoder.start();
    return true;
}

void VideoPlayer::paint(QPainter* painter) {
    if (m_image.isNull()) return;

    QImage img = m_image.scaled(this->width(), this->height(), Qt::KeepAspectRatio);

    int x = this->width() - img.width();
    int y = this->height() - img.height();

    x /= 2;
    y /= 2;

    painter->drawImage(QPoint(x,y), img);
}

void VideoPlayer::onFrameReady(QImage frame) {
    m_image = frame;
    // 触发重绘
    update();
}
