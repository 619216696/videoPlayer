#include "videoplayer.h"
#include <QDebug>
#include <QPainter>

VideoPlayer::VideoPlayer(QQuickItem* parent) : QQuickPaintedItem(parent) {
    connect(&videoDecoder, &VideoDecoder::frameReady, this, &VideoPlayer::onFrameReady);
    connect(this, &VideoPlayer::startDecoding, this, &VideoPlayer::startDecoderThread);
}

VideoPlayer::~VideoPlayer() {
    if (audioDecoder.isRunning()) {
        audioDecoder.requestInterruption();
        audioDecoder.wait();
    }
    if (videoDecoder.isRunning()) {
        videoDecoder.requestInterruption();
        videoDecoder.wait();
    }
}

bool VideoPlayer::loadVideo(const QString& filePath, bool useHw) {
    QUrl fileUrl(filePath);
    QString localPath = fileUrl.toLocalFile();
    qDebug() << "loadVideo path: " << localPath;

    // 设置解码线程的上下文
    if (!videoDecoder.init(localPath, useHw)) {
        qCritical() << "video decoder thread init failed";
        return false;
    }

    if (!audioDecoder.init(localPath)) {
        qCritical() << "audio decoder thread init failed";
        return false;
    }

    // 开始线程解码
    emit startDecoding();
    return true;
}

void VideoPlayer::paint(QPainter* painter) {
    if (image.isNull()) return;

    painter->drawImage(boundingRect(), image);
}

void VideoPlayer::onFrameReady(QImage frame) {
    image = frame;
    // 触发重绘
    update();
}

void VideoPlayer::startDecoderThread() {
    // 使用默认优先级启动线程
    videoDecoder.start();
    audioDecoder.start();
}
