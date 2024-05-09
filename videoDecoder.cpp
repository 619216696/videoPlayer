#include "videoDecoder.h"
#include <QDebug>

VideoDecoder::VideoDecoder() {}

VideoDecoder::~VideoDecoder() {
    if (m_pDecCtx) avcodec_free_context(&m_pDecCtx);
    if (m_swsCtx) sws_freeContext(m_swsCtx);
}

bool VideoDecoder::init(AVStream* stream, bool useHardwareDecoder) {
    // 保存视频流
    m_pStream = stream;
    // 获取视频流的时间基准
    m_timeBase = m_pStream->time_base;
    // 获取解码器参数
    const AVCodecParameters* codecpar = m_pStream->codecpar;
    // 获取对应的解码器
    const AVCodec* codec = useHardwareDecoder ?
        avcodec_find_decoder_by_name("h264_cuvid")
        : avcodec_find_decoder(codecpar->codec_id);

    if (!codec) {
        qCritical() << "Video decoder not found";
        goto end;
    }

    // 分配解码器上下文
    m_pDecCtx = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(m_pDecCtx, codecpar) < 0) {
        qCritical() << "Failed to copy video codec parameters to video decoder context"; 
        goto end;
    }

    // 打开视频解码器上下文
    if (avcodec_open2(m_pDecCtx, codec, nullptr) < 0) {
        qCritical() << "Failed to open video codec context";
        goto end;
    }

    if (useHardwareDecoder) {
        // 创建硬件设备上下文
        AVBufferRef* hw_device_ctx = nullptr;
        if (av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0) < 0) {
            qCritical() << "Failed to create a CUDA hardware device context";
            goto end;
        }
        // 将硬件设备上下文分配给解码器上下文
        m_pDecCtx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
    }

    // 创建并初始化SwsContext
    m_swsCtx = sws_getCachedContext(
        nullptr,                  // 可以传入已有的SwsContext指针，这里我们创建新的
        m_pDecCtx->width,           // 输入宽度
        m_pDecCtx->height,          // 输入高度
        useHardwareDecoder ? m_pDecCtx->sw_pix_fmt : m_pDecCtx->pix_fmt,         // 输入像素格式
        m_pDecCtx->width,           // 输出宽度（此处保持与输入相同，也可以调整）
        m_pDecCtx->height,          // 输出高度（同上）
        AV_PIX_FMT_RGBA,          // 输出像素格式（这里选择RGBA以便Qt快速渲染）
        SWS_BILINEAR,             // 插值方法，可以选择其他如SWS_BICUBIC等
        nullptr, nullptr, nullptr  // 其他高级选项可以留空
        );
    if (!m_swsCtx) {
        qCritical() << "Failed to initialize the conversion context";
        goto end;
    }

    return true;

end:
    if (m_pDecCtx) avcodec_free_context(&m_pDecCtx);
    return false;
}

void VideoDecoder::run() {
    m_bPlaying = true;

    AVFrame* frame = av_frame_alloc();
    // 分配一个AVFrame用于存储转换硬件解码后的数据
    AVFrame* hw_transfer_frame = av_frame_alloc();
    AVPacket* packet = nullptr;

    while (1) {
        // 发生了跳转
        if (m_bSeekFlag) {
            // 清除解码器上下文缓存数据
            avcodec_flush_buffers(m_pDecCtx);
            // 清空跳转标志
            m_bSeekFlag = false;
        }

        // 播放暂停控制
        while (!m_bPlaying) {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]{ return m_bPlaying; });
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
            // 计算帧的显示时间
            qint64 pts = frame->best_effort_timestamp;
            m_nFrameTime = av_rescale_q(pts, m_timeBase, AV_TIME_BASE_Q);

            // 发生了跳转 则跳过关键帧到目的时间的这几帧
            if (m_nFrameTime < m_nSeekTime) {
                continue;
            } else {
                m_nSeekTime = -1;
            }

            // 根据音频进行播放同步
            while (1) {
                // 发生了跳转
                if (m_bSeekFlag) break;
                qint64 delay = m_nFrameTime - audioFrameTime;
                qint64 sleepTime = delay > 5000 ? 5000 : delay;
                if (sleepTime > 0) {
                    av_usleep(sleepTime);
                } else {
                    break;
                }
            }
            // 发生了跳转
            if (m_bSeekFlag) break;

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
            int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, m_pDecCtx->width, m_pDecCtx->height, 1);
            uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
            av_image_fill_arrays(frameRGBA->data, frameRGBA->linesize, buffer, AV_PIX_FMT_RGBA, m_pDecCtx->width, m_pDecCtx->height, 1);
            sws_scale(m_swsCtx, destFrame->data, destFrame->linesize, 0, m_pDecCtx->height, frameRGBA->data, frameRGBA->linesize);
            QImage img((uchar*)buffer, m_pDecCtx->width, m_pDecCtx->height, QImage::Format_RGBA8888, [](void* ptr) { av_free(ptr); }, buffer);
            emit frameReady(img.copy());
            av_frame_free(&frameRGBA);
        }
        av_packet_unref(packet);
        av_packet_free(&packet);
    }

    // 释放资源
    av_frame_free(&frame);
    av_frame_free(&hw_transfer_frame);
}

void VideoDecoder::audioFrameTimeUpdate(qint64 frameTime) {
    audioFrameTime = frameTime;
}
