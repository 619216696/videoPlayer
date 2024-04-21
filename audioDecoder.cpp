#include "audiodecoder.h"
#include <QDebug>
#include <QMediaDevices>

AudioDecoder::AudioDecoder(QObject* parent): QThread(parent) {}

AudioDecoder::~AudioDecoder() {}

bool AudioDecoder::init(const QString& uri) {
    this->uri = uri;

    // 打开输入文件
    if (avformat_open_input(&fmt_ctx, uri.toUtf8().constData(), nullptr, nullptr) != 0) {
        qCritical() << "Failed to open input file";
        return false;
    }

    // 寻找流信息
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        qCritical() << "Failed to retrieve stream info";
        release();
        return false;
    }

    // 查找音频流索引
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i) {
        const auto codec_type = fmt_ctx->streams[i]->codecpar->codec_type;
        if (codec_type == AVMEDIA_TYPE_AUDIO) {
            stream_idx = i;
            break;
        }
    }
    if (stream_idx == -1) {
        qCritical() << "No audio stream";
        release();
        return false;
    }

    // 打开音频解码器
    const AVCodec* audio_decoder = avcodec_find_decoder(fmt_ctx->streams[stream_idx]->codecpar->codec_id);
    if (!audio_decoder) {
        qCritical() << "Audio decoder not found";
        release();
        return false;
    }
    dec_ctx = avcodec_alloc_context3(audio_decoder);
    if (avcodec_parameters_to_context(dec_ctx, fmt_ctx->streams[stream_idx]->codecpar) < 0) {
        qCritical() << "Failed to copy audio codec parameters to audio decoder context";
        release();
        return false;
    }
    // 打开音频解码器上下文
    if (avcodec_open2(dec_ctx, audio_decoder, nullptr) < 0) {
        qCritical() << "Failed to open audio codec context";
        release();
        return false;
    }

    // 初始化音频输出
    QAudioFormat format;
    // 设置音频格式参数
    format.setSampleRate(dec_ctx->sample_rate);
    format.setChannelCount(2);
    // 设置音频样本格式
    format.setSampleFormat(QAudioFormat::Int16);

    // 初始化SwrContext
    AVChannelLayout outChannelLayout = AV_CHANNEL_LAYOUT_STEREO;
    if (swr_alloc_set_opts2(&swr_ctx, &outChannelLayout, AV_SAMPLE_FMT_S16, dec_ctx->sample_rate, &dec_ctx->ch_layout, dec_ctx->sample_fmt, dec_ctx->sample_rate, 0, nullptr) != 0) {
        qCritical() << "Failed to initialize SwrContext";
        return false;
    }
    swr_init(swr_ctx);

    // 创建QAudioSink实例
    audioSink = new QAudioSink(QMediaDevices::defaultAudioOutput(), format, this);
    if (audioSink->isNull()) {
        qWarning() << "audio sink is null, cannot play audio.";
        return false;
    }

    // 开始播放，返回可以写入音频数据的 QIODevice
    audioDevice = audioSink->start();
    if (!audioDevice) {
        qWarning() << "Failed to start audio sink.";
        return false;
    }

    return true;
}

void AudioDecoder::release() {
    if (!audioSink->isNull()) audioSink->stop();
    if (fmt_ctx) avformat_close_input(&fmt_ctx);
    if (dec_ctx) avcodec_free_context(&dec_ctx);
    if (swr_ctx) swr_free(&swr_ctx);
}

void AudioDecoder::run() {
    this->playing = true;

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    // 获取音频流的时间基准
    AVRational audioTimeBase = fmt_ctx->streams[stream_idx]->time_base;
    // 获取当前时间
    qint64 startTime = av_gettime();

    while (av_read_frame(fmt_ctx, packet) == 0) {
        // 播放暂停控制
        while (!playing) {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [this]{ return this->playing; });
        }

        if (packet->stream_index == stream_idx) {
            avcodec_send_packet(dec_ctx, packet);
            if (avcodec_receive_frame(dec_ctx, frame) == 0) {
                // 计算帧的播放时间
                qint64 pts = frame->best_effort_timestamp;
                qint64 frameTime = av_rescale_q(pts, audioTimeBase, AV_TIME_BASE_Q);

                // 控制播放速度
                qint64 now = av_gettime() - startTime;
                qint64 delay = frameTime - now;
                if (delay > 0) {
                    av_usleep(delay);
                }

                // 音频帧转换
                uint8_t* out_buf = nullptr;
                av_samples_alloc(&out_buf, nullptr, 2, frame->nb_samples, AV_SAMPLE_FMT_S16, 0);
                int frame_count = swr_convert(swr_ctx, &out_buf, frame->nb_samples, (const uint8_t**)frame->data, frame->nb_samples);

                if (frame_count > 0) {
                    int out_buf_size = av_samples_get_buffer_size(nullptr, 2, frame_count, AV_SAMPLE_FMT_S16, 0);
                    QByteArray buffer(reinterpret_cast<char*>(out_buf), out_buf_size);
                    audioDevice->write(buffer);
                }
                av_freep(&out_buf);
            }
        }
        av_packet_unref(packet);
    }
    av_packet_free(&packet);
    av_frame_free(&frame);
}

void AudioDecoder::play() {
    playing = true;
    cv.notify_all();
}

void AudioDecoder::stop() {
    playing = false;
}
