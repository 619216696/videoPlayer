#include "videoDecoder.h"
#include <QDebug>
#include <QMediaDevices>
#include <QImage>

VideoDecoder::VideoDecoder(QObject* parent): QThread(parent) {}

VideoDecoder::~VideoDecoder() {}

bool VideoDecoder::init(const QString& uri, bool useHw) {
    this->uri = uri;
    this->useHw = useHw;

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

    // 查找视频流索引
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i) {
        const auto codec_type = fmt_ctx->streams[i]->codecpar->codec_type;
        if (codec_type == AVMEDIA_TYPE_VIDEO) {
            stream_idx = i;
            break;
        }
    }
    if (stream_idx == -1) {
        qCritical() << "No video stream";
        release();
        return false;
    }

    // 查找对应视频解码器
    const AVCodec* video_decoder = useHw ?
        avcodec_find_decoder_by_name("h264_cuvid")
        : avcodec_find_decoder(fmt_ctx->streams[stream_idx]->codecpar->codec_id);

    if (!video_decoder) {
        qCritical() << "Video decoder not found";
        release();
        return false;
    }
    dec_ctx = avcodec_alloc_context3(video_decoder);

    if (avcodec_parameters_to_context(dec_ctx, fmt_ctx->streams[stream_idx]->codecpar) < 0) {
        qCritical() << "Failed to copy video codec parameters to video decoder context";
        release();
        return false;
    }

    // 打开视频解码器上下文
    if (avcodec_open2(dec_ctx, video_decoder, nullptr) < 0) {
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
        dec_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
    }

    // 创建并初始化SwsContext
    sws_ctx = sws_getCachedContext(
        nullptr,                  // 可以传入已有的SwsContext指针，这里我们创建新的
        dec_ctx->width,           // 输入宽度
        dec_ctx->height,          // 输入高度
        useHw ? dec_ctx->sw_pix_fmt : dec_ctx->pix_fmt,         // 输入像素格式
        dec_ctx->width,           // 输出宽度（此处保持与输入相同，也可以调整）
        dec_ctx->height,          // 输出高度（同上）
        AV_PIX_FMT_RGBA,          // 输出像素格式（这里选择RGBA以便Qt快速渲染）
        SWS_BILINEAR,             // 插值方法，可以选择其他如SWS_BICUBIC等
        nullptr, nullptr, nullptr  // 其他高级选项可以留空
        );
    if (!sws_ctx) {
        qCritical() << "Failed to initialize the conversion context";
        release();
        return false;
    }

    return true;
}

void VideoDecoder::release() {
    if (fmt_ctx) avformat_close_input(&fmt_ctx);
    if (dec_ctx) avcodec_free_context(&dec_ctx);
    if (hw_device_ctx) av_buffer_unref(&hw_device_ctx);
    if (sws_ctx) sws_freeContext(sws_ctx);
}

void VideoDecoder::run() {
    this->playing = true;

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    // 分配一个AVFrame用于存储转换硬件解码后的数据
    AVFrame* hw_transfer_frame = av_frame_alloc();

    // 获取视频流的时间基准
    AVRational videoTimeBase = fmt_ctx->streams[stream_idx]->time_base;
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
                // 计算帧的显示时间
                qint64 pts = frame->best_effort_timestamp;
                qint64 frameTime = av_rescale_q(pts, videoTimeBase, AV_TIME_BASE_Q);

                // 控制播放速度
                qint64 now = av_gettime() - startTime;
                qint64 delay = frameTime - now;
                if (delay > 0) {
                    av_usleep(delay);
                }

                AVFrame* destFrame = frame;
                if (frame->hw_frames_ctx) { // 硬件解码则需要做转换
                    hw_transfer_frame->width = frame->width;
                    hw_transfer_frame->height = frame->height;

                    if (av_hwframe_transfer_data(hw_transfer_frame, frame, 0) == 0) {
                        destFrame = hw_transfer_frame;
                    }
                }

                // 帧格式转换为rgba格式的QImage，并发送到界面渲染
                AVFrame* frameRGBA = av_frame_alloc();
                int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, dec_ctx->width, dec_ctx->height, 1);
                uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
                av_image_fill_arrays(frameRGBA->data, frameRGBA->linesize, buffer, AV_PIX_FMT_RGBA, dec_ctx->width, dec_ctx->height, 1);
                sws_scale(sws_ctx, destFrame->data, destFrame->linesize, 0, dec_ctx->height, frameRGBA->data, frameRGBA->linesize);
                QImage img((uchar*)buffer, dec_ctx->width, dec_ctx->height, QImage::Format_RGBA8888, [](void* ptr) { av_free(ptr); }, buffer);
                emit frameReady(img.copy());
                av_frame_free(&frameRGBA);
            }
        }
        av_packet_unref(packet);
    }
    av_packet_free(&packet);
    av_frame_free(&frame);
    av_frame_free(&hw_transfer_frame);
}

void VideoDecoder::play() {
    playing = true;
    cv.notify_all();
}

void VideoDecoder::stop() {
    playing = false;
}
