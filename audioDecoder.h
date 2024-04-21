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

signals:
    void frameTimeUpdate(qint64 frameTime);

public slots:
    void play() override;
    void stop() override;

private:
    QString uri = "";
    SwrContext* swr_ctx = nullptr;
    QAudioSink* audioSink = nullptr;
    QIODevice* audioDevice = nullptr;
};

#endif // AUDIODECODER_H
