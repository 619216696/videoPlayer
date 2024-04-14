#include "decoder.h"
#include <QDebug>
#include <QMediaDevices>
#include <QImage>

DecoderThread::DecoderThread(QObject* parent): QThread(parent) {}

DecoderThread::~DecoderThread() {}

bool DecoderThread::init(const DecoderInitParams& params) {
    this->uri = params.uri;
    this->useHw = params.useHwDecoder;

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

    // 查找视频流及音频流索引
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i) {
        const auto codec_type = fmt_ctx->streams[i]->codecpar->codec_type;
        if (codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
        } else if (codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
        }
    }
    if (video_stream_idx == -1 || audio_stream_idx == -1) {
        qCritical() << "No video stream, audio strem found, video_stream_idx: " << video_stream_idx << ", audio_stream_idx: " << audio_stream_idx;
        release();
        return false;
    }

    // 查找对应视频解码器
    const AVCodec* video_decoder = useHw ?
        avcodec_find_decoder_by_name("h264_cuvid")
        : avcodec_find_decoder(fmt_ctx->streams[video_stream_idx]->codecpar->codec_id);

    if (!video_decoder) {
        qCritical() << "Video decoder not found";
        release();
        return false;
    }
    video_dec_ctx = avcodec_alloc_context3(video_decoder);

    if (avcodec_parameters_to_context(video_dec_ctx, fmt_ctx->streams[video_stream_idx]->codecpar) < 0) {
        qCritical() << "Failed to copy video codec parameters to video decoder context";
        release();
        return false;
    }

    // 打开视频解码器上下文
    if (avcodec_open2(video_dec_ctx, video_decoder, nullptr) < 0) {
        qCritical() << "Failed to open video codec context";
        release();
        return false;
    }

    if (useHw) {
        // 创建硬件设备上下文
        AVBufferRef* hw_device_ctx = nullptr;
        if (av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0) < 0) {
            qCritical() << "Failed to create a CUDA hardware device context";
            release();
            return false;
        }
        // 将硬件设备上下文分配给解码器上下文
        video_dec_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
    }

    // 打开音频解码器
    const AVCodec* audio_decoder = avcodec_find_decoder(fmt_ctx->streams[audio_stream_idx]->codecpar->codec_id);
    if (!audio_decoder) {
        qCritical() << "Audio decoder not found";
        release();
        return false;
    }
    audio_dec_ctx = avcodec_alloc_context3(audio_decoder);
    if (avcodec_parameters_to_context(audio_dec_ctx, fmt_ctx->streams[audio_stream_idx]->codecpar) < 0) {
        qCritical() << "Failed to copy audio codec parameters to audio decoder context";
        release();
        return false;
    }
    // 打开音频解码器上下文
    if (avcodec_open2(audio_dec_ctx, audio_decoder, nullptr) < 0) {
        qCritical() << "Failed to open audio codec context";
        release();
        return false;
    }

    // 创建并初始化SwsContext
    sws_ctx = sws_getCachedContext(
        nullptr,                  // 可以传入已有的SwsContext指针，这里我们创建新的
        video_dec_ctx->width,           // 输入宽度
        video_dec_ctx->height,          // 输入高度
        useHw ? video_dec_ctx->sw_pix_fmt : video_dec_ctx->pix_fmt,         // 输入像素格式
        video_dec_ctx->width,           // 输出宽度（此处保持与输入相同，也可以调整）
        video_dec_ctx->height,          // 输出高度（同上）
        AV_PIX_FMT_RGBA,          // 输出像素格式（这里选择RGBA以便Qt快速渲染）
        SWS_BILINEAR,             // 插值方法，可以选择其他如SWS_BICUBIC等
        nullptr, nullptr, nullptr  // 其他高级选项可以留空
        );
    if (!sws_ctx) {
        qCritical() << "Failed to initialize the conversion context";
        release();
        return false;
    }

    // 初始化音频输出
    QAudioFormat format;
    // 设置音频格式参数
    format.setSampleRate(audio_dec_ctx->sample_rate);
    format.setChannelCount(2);
    // 设置音频样本格式
    format.setSampleFormat(QAudioFormat::Float);

    // 初始化SwrContext
    AVChannelLayout outChannelLayout = AV_CHANNEL_LAYOUT_STEREO;
    if (swr_alloc_set_opts2(&swr_ctx, &outChannelLayout, AV_SAMPLE_FMT_FLT, audio_dec_ctx->sample_rate, &audio_dec_ctx->ch_layout, audio_dec_ctx->sample_fmt, audio_dec_ctx->sample_rate, 0, nullptr) != 0) {
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

void DecoderThread::release() {
    if (!audioSink->isNull()) audioSink->stop();
    if (fmt_ctx) avformat_close_input(&fmt_ctx);
    if (video_dec_ctx) avcodec_free_context(&video_dec_ctx);
    if (audio_dec_ctx) avcodec_free_context(&audio_dec_ctx);
    if (hw_device_ctx) av_buffer_unref(&hw_device_ctx);
    if (sws_ctx) sws_freeContext(sws_ctx);
    if (swr_ctx) swr_free(&swr_ctx);
}

void DecoderThread::run() {
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    // 分配一个AVFrame用于存储转换硬件解码后的数据
    AVFrame* hw_transfer_frame = av_frame_alloc();

    // 获取视频流的时间基准
    AVRational videoTimeBase = fmt_ctx->streams[video_stream_idx]->time_base;
    AVRational audioTimeBase = fmt_ctx->streams[audio_stream_idx]->time_base;
    // 获取当前时间
    qint64 startTime = av_gettime();

    while (av_read_frame(fmt_ctx, packet) == 0) {
        if (packet->stream_index == video_stream_idx) {
            avcodec_send_packet(video_dec_ctx, packet);
            if (avcodec_receive_frame(video_dec_ctx, frame) == 0) {
                AVFrame* destFrame = frame;

                if (frame->hw_frames_ctx) { // 硬件解码则需要做转换
                    hw_transfer_frame->width = frame->width;
                    hw_transfer_frame->height = frame->height;

                    if (av_hwframe_transfer_data(hw_transfer_frame, frame, 0) == 0) {
                        destFrame = hw_transfer_frame;
                    }
                } else { // 软件解码需要延时，控制播放速度
                    // 计算帧的显示时间
                    qint64 pts = frame->best_effort_timestamp;
                    qint64 frameTime = av_rescale_q(pts, videoTimeBase, AV_TIME_BASE_Q);

                    // 控制播放速度
                    qint64 now = av_gettime() - startTime;
                    qint64 delay = frameTime - now;
                    if (delay > 0) {
                        av_usleep(delay);
                    }
                }

                // 帧格式转换为rgba格式的QImage，并发送到界面渲染
                AVFrame* frameRGBA = av_frame_alloc();
                int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, video_dec_ctx->width, video_dec_ctx->height, 1);
                uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
                av_image_fill_arrays(frameRGBA->data, frameRGBA->linesize, buffer, AV_PIX_FMT_RGBA, video_dec_ctx->width, video_dec_ctx->height, 1);
                sws_scale(sws_ctx, destFrame->data, destFrame->linesize, 0, video_dec_ctx->height, frameRGBA->data, frameRGBA->linesize);
                QImage img((uchar*)buffer, video_dec_ctx->width, video_dec_ctx->height, QImage::Format_RGBA8888, [](void* ptr) { av_free(ptr); }, buffer);
                emit videoFrameReady(img.copy());
                av_frame_free(&frameRGBA);
            }
        } else if (packet->stream_index == audio_stream_idx) {
            avcodec_send_packet(audio_dec_ctx, packet);
            if (avcodec_receive_frame(audio_dec_ctx, frame) == 0) {
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
                const int out_samples = av_rescale_rnd(swr_get_delay(swr_ctx, audio_dec_ctx->sample_rate) + frame->nb_samples, audio_dec_ctx->sample_rate, audio_dec_ctx->sample_rate, AV_ROUND_UP);
                uint8_t* out_buf = nullptr;
                av_samples_alloc(&out_buf, nullptr, 2, out_samples, AV_SAMPLE_FMT_FLT, 0);
                int frame_count = swr_convert(swr_ctx, &out_buf, out_samples, (const uint8_t**)frame->data, frame->nb_samples);

                if (frame_count > 0) {
                    int out_buf_size = av_samples_get_buffer_size(nullptr, 2, frame_count, AV_SAMPLE_FMT_FLT, 0);
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
    av_frame_free(&hw_transfer_frame);
}
