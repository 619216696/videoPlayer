#include "videoplayer.h"

VideoPlayer::VideoPlayer(QQuickItem* parent)
    : QQuickPaintedItem(parent)
    , fmt_ctx(nullptr)
    , dec_ctx(nullptr)
    , frame(nullptr)
    , packet(nullptr)
    , fps(30) {

}

VideoPlayer::~VideoPlayer() {
    // 释放FFmpeg资源
    avformat_close_input(&fmt_ctx);
    avcodec_free_context(&dec_ctx);
    sws_freeContext(sws_ctx);
    av_frame_free(&frame);
    av_packet_free(&packet);
}

int VideoPlayer::loadVideo(const QString& filePath) {
    QUrl fileUrl(filePath);
    QString localPath = fileUrl.toLocalFile();
    qDebug() << "loadVideo path: " << localPath;
    // 初始化并打开文件
    // ...

    // 查找视频流并初始化解码器
    // ...

    // 创建SWScale上下文用于转换像素格式
    // sws_ctx = sws_getContext(
    //     dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
    //     dec_ctx->width, dec_ctx->height, AV_PIX_FMT_RGBA,
    //     SWS_BILINEAR, nullptr, nullptr, nullptr);

    // // 设置定时器更新画面
    // startTimer(1000 / fps); // fps假设是从视频流信息获取到的帧率

    return 0; // 成功
}

void VideoPlayer::timerEvent(QTimerEvent* event) {
    // decodeAndRenderNextFrame();
    // update();
}

void VideoPlayer::paint(QPainter* painter) {
    // if (image.isNull()) return;

    // painter->drawImage(boundingRect(), image);
}

void VideoPlayer::decodeAndRenderNextFrame() {
    // 从文件读取包并解码
    // if (av_read_frame(fmt_ctx, packet) >= 0 && packet->stream_index == video_stream_idx) {
    //     avcodec_send_packet(dec_ctx, packet);
    //     avcodec_receive_frame(dec_ctx, frame);

    //     // 将解码帧转换为RGBA格式
    //     uint8_t *buffer_rgba[(frame->height * frame->width * 4)];
    //     sws_scale(sws_ctx, (uint8_t const *const *)frame->data, frame->linesize, 0, frame->height, buffer_rgba, frame->width * 4);

    //     // 创建QImage并设置数据
    //     image = QImage(frame->width, frame->height, QImage::Format_RGBA8888);
    //     memcpy(image.bits(), buffer_rgba, image.byteCount());

    //     // 这里假设已经在image上添加了水印，实际情况请自行实现
    // }
}
