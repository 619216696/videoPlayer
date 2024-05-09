#include "audiodecoder.h"
#include <QDebug>
#include <QMediaDevices>

AudioDecoder::AudioDecoder() {}

AudioDecoder::~AudioDecoder() {
    if (m_pAudioSink && !m_pAudioSink->isNull()) m_pAudioSink->stop();
    if (m_pDecCtx) avcodec_free_context(&m_pDecCtx);
    if (m_pSwrCtx) swr_free(&m_pSwrCtx);
}

bool AudioDecoder::init(AVStream* stream) {
    // 保存视频流
    m_pStream = stream;
    // 获取视频流的时间基准
    m_timeBase = m_pStream->time_base;
    // 获取解码器参数
    const AVCodecParameters* codecpar = m_pStream->codecpar;
    AVChannelLayout outChannelLayout = AV_CHANNEL_LAYOUT_STEREO;

    // 打开音频解码器
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        qCritical() << "Audio decoder not found";
        goto end;
    }

    m_pDecCtx = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(m_pDecCtx, codecpar) < 0) {
        qCritical() << "Failed to copy audio codec parameters to audio decoder context";
        goto end;
    }

    // 打开音频解码器上下文
    if (avcodec_open2(m_pDecCtx, codec, nullptr) < 0) {
        qCritical() << "Failed to open audio codec context";
        goto end;
    }

    // 初始化SwrContext
    if (swr_alloc_set_opts2(&m_pSwrCtx, &outChannelLayout, AV_SAMPLE_FMT_S16, m_pDecCtx->sample_rate, &m_pDecCtx->ch_layout, m_pDecCtx->sample_fmt, m_pDecCtx->sample_rate, 0, nullptr) != 0) {
        qCritical() << "Failed to initialize SwrContext";
        goto end;
    }
    if (swr_init(m_pSwrCtx) != 0) {
        qCritical() << "swr_init Failed";
        goto end;
    }

    return true;

end:
    if (m_pDecCtx) avcodec_free_context(&m_pDecCtx);
    return false;
}

void AudioDecoder::run() {
    m_bPlaying = true;

    // 初始化音频输出
    QAudioFormat format;
    // 设置音频格式参数
    format.setSampleRate(m_pDecCtx->sample_rate);
    format.setChannelCount(2);
    // 设置音频样本格式
    format.setSampleFormat(QAudioFormat::Int16);

    // 创建QAudioSink实例
    m_pAudioSink = new QAudioSink(QMediaDevices::defaultAudioOutput(), format, this);
    if (m_pAudioSink->isNull()) {
        qWarning() << "audio sink is null, cannot play audio.";
        return;
    }

    // 开始播放，返回可以写入音频数据的 QIODevice
    m_pAudioDevice = m_pAudioSink->start();
    if (!m_pAudioDevice) {
        qWarning() << "Failed to start audio sink.";
        return;
    }

    AVFrame* frame = av_frame_alloc();

    // 获取当前时间
    m_nStartTime = av_gettime();

    AVPacket* packet = nullptr;

    while (1) {
        // 发生了跳转
        if (m_bSeekFlag) {
            // 清除解码器上下文缓存数据
            avcodec_flush_buffers(m_pDecCtx);
            // 补偿时间
            m_nStartTime -= (m_nSeekTime - m_nFrameTime);
            // 清空跳转时间
            m_bSeekFlag = false;
        }

        // 播放暂停控制
        while (!m_bPlaying) {
            // 记录暂停的时间
            qint64 stopTime = av_gettime();
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]{ return m_bPlaying; });
            // 将暂停的时间进行补偿
            m_nStartTime += (av_gettime() - stopTime);
        }

        // 从队列中获取一个packet
        m_mutex.lock();
        // 无数据
        if (m_queue.empty()) {
            m_mutex.unlock();
            av_usleep(1000);
            continue;
        }
        packet = m_queue.front();
        m_queue.pop_front();
        m_mutex.unlock();

        // 发生了跳转
        if (m_bSeekFlag) {
            av_packet_unref(packet);
            av_packet_free(&packet);
            continue;
        }

        // 发送一个包到解码器中解码
        if (avcodec_send_packet(m_pDecCtx, packet) != 0) {
            qDebug("send AVPacket to decoder failed!\n");
            av_packet_unref(packet);
            continue;
        }
        // 接受已解码数据
        while (avcodec_receive_frame(m_pDecCtx, frame) == 0) {
            // 计算帧的播放时间
            qint64 pts = frame->best_effort_timestamp;
            m_nFrameTime = av_rescale_q(pts, m_timeBase, AV_TIME_BASE_Q);

            // 发生了跳转 则跳过关键帧到目的时间的这几帧
            if (m_nFrameTime < m_nSeekTime) {
                continue;
            } else {
                m_nSeekTime = -1;
            }

            // 控制播放速度
            while(1) {
                // 发生了跳转
                if (m_bSeekFlag) break;
                qint64 now = av_gettime() - m_nStartTime;
                qint64 delay = m_nFrameTime - now;
                qint64 sleepTime = delay > 5000 ? 5000 : delay;
                if (sleepTime > 0) {
                    av_usleep(sleepTime);
                } else {
                    break;
                }
            }
            // 发生了跳转
            if (m_bSeekFlag) break;

            // 音频帧转换
            uint8_t* out_buf = nullptr;
            av_samples_alloc(&out_buf, nullptr, 2, frame->nb_samples, AV_SAMPLE_FMT_S16, 0);
            int frame_count = swr_convert(m_pSwrCtx, &out_buf, frame->nb_samples, (const uint8_t**)frame->data, frame->nb_samples);

            if (frame_count > 0) {
                int out_buf_size = av_samples_get_buffer_size(nullptr, 2, frame_count, AV_SAMPLE_FMT_S16, 0);
                QByteArray buffer(reinterpret_cast<char*>(out_buf), out_buf_size);
                m_pAudioDevice->write(buffer);
                emit frameTimeUpdate(m_nFrameTime);
            }
            av_freep(&out_buf);
        }
        av_packet_unref(packet);
        av_packet_free(&packet);
    }
    av_frame_free(&frame);
}
