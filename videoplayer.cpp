#include "videoplayer.h"

VideoPlayer::VideoPlayer(QQuickItem* parent)
    : QQuickPaintedItem(parent)
    , fmt_ctx(nullptr)
    , dec_ctx(nullptr)
    , sws_ctx(nullptr)
    , frame(nullptr)
    , packet(nullptr)
    , fps(0)
    , video_stream_idx(-1) {

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

    // 初始化帧和包
    frame = av_frame_alloc();
    packet = av_packet_alloc();

    // 获取帧率信息（假设默认第一帧有效）
    fps = static_cast<int>(fmt_ctx->streams[video_stream_idx]->avg_frame_rate.num /
                           fmt_ctx->streams[video_stream_idx]->avg_frame_rate.den);

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

    // 设置定时器更新画面
    startTimer(1000 / fps); // fps假设是从视频流信息获取到的帧率

    return true;
}

void VideoPlayer::unloadVideo()
{
    if (fmt_ctx) avformat_close_input(&fmt_ctx);
    if (dec_ctx) avcodec_free_context(&dec_ctx);
    if (sws_ctx) sws_freeContext(sws_ctx);
    if (frame) av_frame_free(&frame);
    if (packet) av_packet_free(&packet);
}

void VideoPlayer::timerEvent(QTimerEvent* event) {
    decodeAndRenderNextFrame();
    update();
}

void VideoPlayer::paint(QPainter* painter) {
    if (image.isNull()) return;

    painter->drawImage(boundingRect(), image);
}

void VideoPlayer::decodeAndRenderNextFrame() {
    // 从文件读取包并解码
    if (av_read_frame(fmt_ctx, packet) >= 0 && packet->stream_index == video_stream_idx) {
        avcodec_send_packet(dec_ctx, packet);
        avcodec_receive_frame(dec_ctx, frame);

        // 创建用于转换的目标帧
        AVFrame *frame_rgba = av_frame_alloc();
        frame_rgba->width = frame->width;
        frame_rgba->height = frame->height;
        frame_rgba->format = AV_PIX_FMT_RGBA;
        if (av_frame_get_buffer(frame_rgba, 0) < 0) {
            qCritical() << "Failed to allocate memory for the converted frame";
            av_frame_free(&frame_rgba);
            return;
        }

        // 进行帧格式转换
        sws_scale(
            sws_ctx,
            (const uint8_t *const*)frame->data, frame->linesize,
            0, frame->height,
            frame_rgba->data, frame_rgba->linesize
            );

        // 创建QImage并设置数据
        image = QImage((uchar*)frame_rgba->data[0], frame_rgba->width, frame_rgba->height, frame_rgba->linesize[0], QImage::Format_RGBA8888);

        // 这里假设已经在image上添加了水印，实际情况请自行实现

        av_frame_free(&frame_rgba);
    }
}
