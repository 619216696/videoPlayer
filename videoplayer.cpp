#include "videoplayer.h"
#include <QDebug>
#include <QDateTime>
#include <QPainter>

DecoderThread::DecoderThread(QObject *parent) : QThread(parent), fmt_ctx(nullptr), dec_ctx(nullptr), sws_ctx(nullptr), video_stream_idx(-1) {}

void DecoderThread::setup(AVFormatContext *fmt_ctx, AVCodecContext *dec_ctx, SwsContext *sws_ctx, int video_stream_idx) {
    this->fmt_ctx = fmt_ctx;
    this->dec_ctx = dec_ctx;
    this->sws_ctx = sws_ctx;
    this->video_stream_idx = video_stream_idx;
}

void DecoderThread::run() {
    AVFrame *frame = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();

    // 获取视频流的时间基准
    AVRational timeBase = fmt_ctx->streams[video_stream_idx]->time_base;

    qint64 startTime = av_gettime(); // 获取当前时间

    while (av_read_frame(fmt_ctx, packet) == 0) {
        if (packet->stream_index == video_stream_idx) {
            avcodec_send_packet(dec_ctx, packet);
            if (avcodec_receive_frame(dec_ctx, frame) == 0) {
                // 计算帧的显示时间
                qint64 pts = frame->best_effort_timestamp;
                qint64 frameTime = av_rescale_q(pts, timeBase, AV_TIME_BASE_Q);

                // 控制播放速度
                qint64 now = av_gettime() - startTime;
                qint64 delay = frameTime - now;
                if (delay > 0) {
                    av_usleep(delay);
                }

                AVFrame *frameRGBA = av_frame_alloc();
                int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, dec_ctx->width, dec_ctx->height, 1);
                uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
                av_image_fill_arrays(frameRGBA->data, frameRGBA->linesize, buffer, AV_PIX_FMT_RGBA, dec_ctx->width, dec_ctx->height, 1);
                sws_scale(sws_ctx, frame->data, frame->linesize, 0, dec_ctx->height, frameRGBA->data, frameRGBA->linesize);
                QImage img((uchar*)buffer, dec_ctx->width, dec_ctx->height, QImage::Format_RGBA8888, [](void* ptr) { av_free(ptr); }, buffer);
                emit frameReady(img.copy());
                av_frame_free(&frameRGBA);
            }
        }
        av_packet_unref(packet);
    }
    av_frame_free(&frame);
    av_packet_free(&packet);
}

VideoPlayer::VideoPlayer(QQuickItem* parent)
    : QQuickPaintedItem(parent)
    , fmt_ctx(nullptr)
    , dec_ctx(nullptr)
    , sws_ctx(nullptr)
    , video_stream_idx(-1) {
    connect(&decoderThread, &DecoderThread::frameReady, this, &VideoPlayer::onFrameReady);
    connect(this, &VideoPlayer::startDecoding, this, &VideoPlayer::startDecoderThread);
}

VideoPlayer::~VideoPlayer() {
    // 释放FFmpeg资源
    unloadVideo();
}

bool VideoPlayer::loadVideo(const QString& filePath) {
    QUrl fileUrl(filePath);
    QString localPath = fileUrl.toLocalFile();
    qDebug() << "loadVideo path: " << localPath;

    // 打开输入文件
    if (avformat_open_input(&fmt_ctx, localPath.toUtf8().constData(), nullptr, nullptr) != 0) {
        qCritical() << "Failed to open input file";
        return false;
    }

    // 寻找流信息
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        qCritical() << "Failed to retrieve stream info";
        unloadVideo();
        return false;
    }

    // 查找视频流索引
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            break;
        }
    }
    if (video_stream_idx == -1) {
        qCritical() << "No video stream found";
        unloadVideo();
        return false;
    }

    // 打开解码器
    const AVCodec* decoder = avcodec_find_decoder(fmt_ctx->streams[video_stream_idx]->codecpar->codec_id);
    if (!decoder) {
        qCritical() << "Decoder not found";
        unloadVideo();
        return false;
    }
    dec_ctx = avcodec_alloc_context3(decoder);
    if (avcodec_parameters_to_context(dec_ctx, fmt_ctx->streams[video_stream_idx]->codecpar) < 0) {
        qCritical() << "Failed to copy codec parameters to decoder context";
        unloadVideo();
        return false;
    }

    // 打开解码器上下文
    if (avcodec_open2(dec_ctx, decoder, nullptr) < 0) {
        qCritical() << "Failed to open codec context";
        unloadVideo();
        return false;
    }

    // 创建并初始化SwsContext
    sws_ctx = sws_getCachedContext(
        nullptr,                  // 可以传入已有的SwsContext指针，这里我们创建新的
        dec_ctx->width,           // 输入宽度
        dec_ctx->height,          // 输入高度
        dec_ctx->pix_fmt,         // 输入像素格式
        dec_ctx->width,           // 输出宽度（此处保持与输入相同，也可以调整）
        dec_ctx->height,          // 输出高度（同上）
        AV_PIX_FMT_RGBA,          // 输出像素格式（这里选择RGBA以便Qt快速渲染）
        SWS_BILINEAR,             // 插值方法，可以选择其他如SWS_BICUBIC等
        nullptr, nullptr, nullptr  // 其他高级选项可以留空
        );
    if (!sws_ctx) {
        qCritical() << "Failed to initialize the conversion context";
        unloadVideo();
        return false;
    }

    // 设置解码线程的上下文
    decoderThread.setup(fmt_ctx, dec_ctx, sws_ctx, video_stream_idx);
    emit startDecoding(); // 开始解码线程

    return true;
}

void VideoPlayer::unloadVideo()
{
    if (decoderThread.isRunning()) {
        decoderThread.requestInterruption();
        decoderThread.wait();
    }
    if (fmt_ctx) avformat_close_input(&fmt_ctx);
    if (dec_ctx) avcodec_free_context(&dec_ctx);
    if (sws_ctx) sws_freeContext(sws_ctx);
}

void VideoPlayer::paint(QPainter* painter) {
    if (image.isNull()) return;

    painter->drawImage(boundingRect(), image);
}

void VideoPlayer::onFrameReady(QImage frame) {
    image = frame;
    update(); // 触发重绘
}

void VideoPlayer::startDecoderThread() {
    decoderThread.start(); // 使用默认优先级启动线程
}
