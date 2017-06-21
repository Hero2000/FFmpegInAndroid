//
// Created by glumes on 2017/6/20.
//

#include <string>
#include <stdlib.h>
#include <jni.h>

#ifdef  __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
#include "libavutil/mem.h"

#ifdef  __cplusplus
};
#endif


#define AUDIO_INPUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096


void decode(AVCodecContext *avCodecContext, AVPacket *packet, AVFrame *frame, FILE *outfile) {
    int i, ch;
    int ret, data_size;
    // send the packet with the compressed data to the decoder
    ret = avcodec_send_packet(avCodecContext, packet);

    if (ret < 0) {
        fprintf(stderr, "Error submitting the packet to the decoder\n");
        exit(0);
    }

    /**
     * read all the output frames (in general there may be any number of them)
     */
    while (ret >= 0) {
        ret = avcodec_receive_frame(avCodecContext, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            exit(0);
        }
        data_size = av_get_bytes_per_sample(avCodecContext->sample_fmt);
        if (data_size < 0) {
            /**
             * This should not occur,checking just for paranoia
             */
            fprintf(stderr, "Failed to calculate data size\n");
            exit(1);
        }
        for (i = 0; i < frame->nb_samples; i++) {
            for (ch = 0; ch < avCodecContext->channels; ++ch) {
                fwrite(frame->data[ch] + data_size * i, (size_t) i, (size_t) data_size, outfile);
            }
        }
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_glumes_ffmpegexamples_FFmpegSampleHolder_decode(JNIEnv *env, jclass type,
                                                         jstring inputFile_, jstring outputFile_) {
    const char *filename = env->GetStringUTFChars(inputFile_, 0);
    const char *outfilename = env->GetStringUTFChars(outputFile_, 0);

    const AVCodec *avCodec;
    AVCodecContext *avCodecContext = nullptr;
    AVCodecParserContext *avCodecParserContext = nullptr;
    int len, ret;
    FILE *f, *outfile;
    uint8_t inbuf[AUDIO_INPUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data;
    size_t data_size;
    AVPacket *packet;
    AVFrame *decode_frame = nullptr;


    avcodec_register_all();
    packet = av_packet_alloc();

    /**
     * 初始化相关的变量参数
     */
    avCodec = avcodec_find_decoder(AV_CODEC_ID_MP2);
    if (!avCodec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    avCodecParserContext = av_parser_init(avCodec->id);
    if (!avCodecParserContext) {
        fprintf(stderr, "Parser not found\n");
        exit(1);
    }

    avCodecContext = avcodec_alloc_context3(avCodec);
    if (!avCodecContext) {
        fprintf(stderr, "Could not allocate audio codec context\n");
        exit(0);
    }

    if (avcodec_open2(avCodecContext, avCodec, nullptr) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    /**
     * 打开相应文件
     */
    f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    outfile = fopen(outfilename, "wb");
    if (!outfile) {
        av_free(avCodecContext);
        exit(1);
    }

    data = inbuf;
    data_size = fread(inbuf, 1, AUDIO_INPUF_SIZE, f);

    /**
     * 开始解码
     */
    while (data_size > 0) {
        if (!decode_frame) {
            if (!(decode_frame = av_frame_alloc())) {
                fprintf(stderr, "Could not allocate audio frame\n");
                exit(1);
            }
        }

        ret = av_parser_parse2(avCodecParserContext, avCodecContext, &packet->data, &packet->size,
                               data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        if (ret < 0) {
            fprintf(stderr, "Error while parsing\n");
            exit(1);
        }

        data += ret;
        data_size -= ret;

        if (packet->size) {
            decode(avCodecContext, packet, decode_frame, outfile);
        }

        if (data_size < AUDIO_REFILL_THRESH) {
            memmove(inbuf, data, data_size);
            data = inbuf;
            len = (int) fread(data + data_size, 1, AUDIO_INPUF_SIZE - data_size, f);
            if (len > 0) {
                data_size += len;
            }
        }
    }

    /**
     * 释放对应变量
     */
    packet->data = nullptr;
    packet->size = 0;
    decode(avCodecContext, packet, decode_frame, outfile);

    fclose(outfile);
    fclose(f);

    avcodec_free_context(&avCodecContext);
    av_parser_close(avCodecParserContext);
    av_frame_free(&decode_frame);
    av_packet_free(&packet);


    env->ReleaseStringUTFChars(inputFile_, filename);
    env->ReleaseStringUTFChars(outputFile_, outfilename);
}