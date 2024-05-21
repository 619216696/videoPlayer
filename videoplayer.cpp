#include "videoplayer.h"
#include <QDebug>
#include <QPainter>
#include <QMediaDevices>

VideoPlayer::VideoPlayer(QQuickItem* parent) : QQuickPaintedItem(parent) {}

VideoPlayer::~VideoPlayer() {
    if (m_pAudioSink && !m_pAudioSink->isNull()) m_pAudioSink->stop();
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

    connect(&m_decoder, &Decoder::videoFrameReady, this, &VideoPlayer::onVideoFrameReady);
    connect(&m_decoder, &Decoder::audioFrameReady, this, &VideoPlayer::onAudioFrameReady);

    // 初始化音频输出
    QAudioFormat format;
    // 设置音频格式参数
    format.setSampleRate(m_decoder.getAudioSampleRate());
    format.setChannelCount(2);
    // 设置音频样本格式
    format.setSampleFormat(QAudioFormat::Int16);

    // 创建QAudioSink实例
    m_pAudioSink = new QAudioSink(QMediaDevices::defaultAudioOutput(), format, this);
    if (m_pAudioSink->isNull()) {
        qWarning() << "audio sink is null, cannot play audio.";
        return false;
    }

    // 开始播放，返回可以写入音频数据的 QIODevice
    m_pAudioDevice = m_pAudioSink->start();
    if (!m_pAudioDevice) {
        qWarning() << "Failed to start audio sink.";
        return false;
    }

    // 开始线程解码
    m_decoder.start();
    setPlaying(true);
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

void VideoPlayer::onVideoFrameReady(QImage frame) {
    m_image = frame;
    // 触发重绘
    update();
}

void VideoPlayer::onAudioFrameReady(QByteArray buffer) {
    m_pAudioDevice->write(buffer);
}
