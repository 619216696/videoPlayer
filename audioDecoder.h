#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include <QThread>
#include <QAudioSink>
#include "./decoderBase.h"

extern "C" {
#include <libswresample/swresample.h>
}

class AudioDecoder: public QThread, DecoderBase {
    Q_OBJECT
public:
    AudioDecoder(QObject* parent = nullptr);
    ~AudioDecoder();

    void run() override;

    bool init(const QString& uri);
    void release();
    qint64 getDuration();

signals:
    void frameTimeUpdate(qint64 frameTime);

public slots:
    void play() override;
    void stop() override;
    void seekToPosition(qint64 timestamp) override;

private:
    QString uri = "";
    SwrContext* swr_ctx = nullptr;
    QAudioSink* audioSink = nullptr;
    QIODevice* audioDevice = nullptr;
    qint64 duration = 0;
    qint64 startTime = 0;
};

#endif // AUDIODECODER_H
