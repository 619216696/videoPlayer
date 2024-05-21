#include "decoder.h"
#include <qDebug>

#define MAX_AUDIO_SIZE (50 * 20)
#define MAX_VIDEO_SIZE (25 * 20)

Decoder::Decoder() {}

Decoder::~Decoder() {
    if (m_audioDecoder.isRunning()) {
        m_audioDecoder.requestInterruption();
        m_audioDecoder.wait();
    }
    if (m_videoDecoder.isRunning()) {
        m_videoDecoder.requestInterruption();
        m_videoDecoder.wait();
    }
    if (m_pFmtCtx) avformat_close_input(&m_pFmtCtx);
}

bool Decoder::init(const QString& uri, bool useHardwareDecoder /* = false */) {
  m_strUri = uri;

  // 打开输入文件
  if (avformat_open_input(&m_pFmtCtx, m_strUri.toUtf8().constData(), nullptr, nullptr) != 0) {
    qCritical() << "Failed to open input file";
    goto end;
  }

  // 寻找流信息
  if (avformat_find_stream_info(m_pFmtCtx, nullptr) < 0) {
    qCritical() << "Failed to retrieve stream info";
    goto end;
  }

  // 查找视频流和音频流
  for (unsigned int i = 0; i < m_pFmtCtx->nb_streams; ++i) {
    const auto stream = m_pFmtCtx->streams[i];
    const auto codec_type = stream->codecpar->codec_type;
    if (m_nVideoStreamIdx == -1 && codec_type == AVMEDIA_TYPE_VIDEO) {
        m_nVideoStreamIdx = i;
    }
    if (m_nAudioStreamIdx == -1 && codec_type == AVMEDIA_TYPE_AUDIO) {
        m_nAudioStreamIdx = i;
    }
  }

  // 存在音频流
  if (m_nAudioStreamIdx != -1) {
      // 初始化音频解码线程
      if (!m_audioDecoder.init(m_pFmtCtx->streams[m_nAudioStreamIdx])) {
        qCritical() << "audioDecoder init failed";
        goto end;
      }
      m_nAudioSampleRate = m_audioDecoder.getSampleRate();
      // 启动音频解码线程
      m_audioDecoder.start();
      // 连接信号
      connect(&m_audioDecoder, &AudioDecoder::frameTimeUpdate, &m_videoDecoder, &VideoDecoder::audioFrameTimeUpdate);
      connect(&m_audioDecoder, &AudioDecoder::frameReady, this, &Decoder::audioFrameReady);
  }

  // 存在视频流
  if (m_nVideoStreamIdx != -1) {
    // 初始化视频解码线程
    if (!m_videoDecoder.init(m_pFmtCtx->streams[m_nVideoStreamIdx], useHardwareDecoder)) {
        qCritical() << "videoDecoder init failed";
        goto end;
    }
    // 启动视频解码线程
    m_videoDecoder.start();
    // 连接信号
    connect(&m_videoDecoder, &VideoDecoder::frameReady, this, &Decoder::videoFrameReady);
  }

  m_nDuration = m_pFmtCtx->duration / AV_TIME_BASE;
  return true;

end:
  if (m_pFmtCtx) avformat_close_input(&m_pFmtCtx);
  return false;
}

void Decoder::run() {
    m_bPlaying = true;

    AVPacket* packet = av_packet_alloc();
    while(1) {
        // 发生了跳转
        if (m_nSeekTime != -1) {
            qint64 frameTime = av_rescale_q(m_nSeekTime, AV_TIME_BASE_Q, m_pFmtCtx->streams[m_nVideoStreamIdx]->time_base);
            if (av_seek_frame(m_pFmtCtx, m_nVideoStreamIdx, frameTime, AVSEEK_FLAG_BACKWARD) < 0) {
                qCritical() << "seek frame failed, seekTime: " << m_nSeekTime;
            } else {
                m_audioDecoder.seekToPosition(m_nSeekTime);
                m_videoDecoder.seekToPosition(m_nSeekTime);
            }
            m_nSeekTime = -1;
        }

        // 超出缓存限制，则停止读packet
        if (m_videoDecoder.queueSize() > MAX_VIDEO_SIZE || m_audioDecoder.queueSize() > MAX_AUDIO_SIZE) {
            av_usleep(10000);
            continue;
        }
        // 暂停
        if (!m_bPlaying) {
            av_usleep(10000);
            continue;
        }

        // 读packet，加入对应的队列中
        if (av_read_frame(m_pFmtCtx, packet) < 0)
        {
            av_usleep(10000);
            continue;
        }

        if (packet->stream_index == m_nVideoStreamIdx) {
            m_videoDecoder.addToQueue(av_packet_clone(packet));
        } else if (packet->stream_index == m_nAudioStreamIdx) {
            m_audioDecoder.addToQueue(av_packet_clone(packet));
        }

        av_packet_unref(packet);
    }
    av_packet_free(&packet);
}

void Decoder::setPlayState(bool play) {
    m_bPlaying = play;
    if (play) {
        m_audioDecoder.play();
        m_videoDecoder.play();
    } else {
        m_audioDecoder.stop();
        m_videoDecoder.stop();
    }
}

void Decoder::seekToPosition(qint64 second) {
    m_nSeekTime = second * AV_TIME_BASE;
}
