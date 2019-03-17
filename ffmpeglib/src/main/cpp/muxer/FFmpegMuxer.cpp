//
// Created by glumes on 2019/3/17.
//

#include "FFmpegMuxer.h"

#define USE_H264BSF 1

void FFmpegMuxer::muxer_simple(const char *input_path, const char *output_path_video,
                               const char *output_path_audio) {

    AVFormatContext *formatContext;

    AVPacket packet;

    int ret, i;
    int videoindex = -1, audioindex = -1;

    av_register_all();

    if ((ret = avformat_open_input(&formatContext, input_path, 0, 0)) < 0) {
        LogClient::LogD("could not open input file");
        return;
    }

    if ((ret = avformat_find_stream_info(formatContext, 0)) < 0) {
        LogClient::LogD("failed to retrieve input stream information");
        return;
    }

    videoindex = -1;

    for (i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
        } else if (formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioindex = i;
        }
    }

    FILE *fp_audio = fopen(output_path_audio, "wb+");
    FILE *fp_video = fopen(output_path_video, "wb+");

#if USE_H264BSF
    AVBitStreamFilterContext *h264bsfc = av_bitstream_filter_init("h264_mp4oannexb");
#endif

    while (av_read_frame(formatContext, &packet) >= 0) {
        if (packet.stream_index == videoindex) {
#if USE_H264BSF
            av_bitstream_filter_filter(h264bsfc, formatContext->streams[videoindex]->codec, nullptr,
                                       &packet.data, &packet.size, packet.data, packet.size, 0);
#endif
            LOGD("write video packet.size:%d\tpts:%lld\n", packet.size, packet.pts);
            fwrite(packet.data, 1, packet.size, fp_video);
        } else if (packet.stream_index == audioindex) {
            LOGD("write video packet.size:%d\tpts:%lld\n", packet.size, packet.pts);
        }
        av_free_packet(&packet);
    }
#if USE_H264BSF
    av_bitstream_filter_close(h264bsfc);
#endif
    fclose(fp_video);
    fclose(fp_audio);

    avformat_close_input(&formatContext);

    if (ret < 0 && ret != AVERROR_EOF) {
        LogClient::LogD("Errro occurred.\n");
        return;
    }
}

void FFmpegMuxer::muxer_standard(const char *input_path, const char *output_path_video,
                                 const char *output_path_audio) {

    AVOutputFormat *outputFormat_a = nullptr, *outputFormat_v = nullptr;

    AVFormatContext *iformatContext = nullptr, *iformatContext_a = nullptr, *iformatContext_v = nullptr;

    AVPacket packet;

    int ret, i;

    int videoindex = -1, audioindex = -1;

    int frame_index = 0;

    av_register_all();

    if ((ret = avformat_open_input(&iformatContext, input_path, 0, 0)) < 0) {
        LogClient::LogD("could not open input file");
        return;
    }

    if ((ret = avformat_find_stream_info(iformatContext, 0)) < 0) {
        LogClient::LogD("failed to retrieve input stream information");
        return;
    }

    avformat_alloc_output_context2(&iformatContext_v, nullptr, nullptr, output_path_video);

    if (!iformatContext_v) {
        LogClient::LogD("could not create output context\n");
        return;
    }

    outputFormat_v = iformatContext_v->oformat;

    avformat_alloc_output_context2(&iformatContext_a, nullptr, nullptr, output_path_audio);

    if (!iformatContext_a) {
        LogClient::LogD("could not create output context\n");
        return;
    }

    outputFormat_a = iformatContext_a->oformat;

    for (i = 0; i < iformatContext->nb_streams; i++) {
        AVFormatContext *
    }
}
