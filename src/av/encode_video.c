#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>

static void encode(AVCodecContext* enc_ctx, AVFrame * frame, AVPacket *pkt, FILE * outfile) 
{
	int ret = 0;

	/* send the frame to the encoder */
	if (frame) {
		av_log(NULL, AV_LOG_INFO, "send frame %3"PRId64"\n", frame->pts);
	}
	fflush(stdout);

	ret = avcodec_send_frame(enc_ctx, frame);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Error sending a frame for encoding\n");
		exit(1);
	}

	while (ret >= 0){
		ret = avcodec_receive_packet(enc_ctx, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			return;
		}
		else if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Error during encoding\n");
			exit(1);
		}

		av_log(NULL, AV_LOG_INFO, "Write packet %3"PRId64" (size=%5d)\n", pkt->pts, pkt->size);
		fwrite(pkt->data, 1, pkt->size, outfile);
		av_packet_unref(pkt);
	}
}

int main(int argc, char** argv)
{
	const char* filename = NULL, *codec_name = NULL;
	const AVCodec *codec = NULL;
	AVCodecContext *c = NULL;
	int i = 0, ret = 0, x = 0, y = 0, got_output = 0;
	FILE *f = NULL;
	AVFrame *frame = NULL;
	AVPacket *pkt = NULL;
	uint8_t endcode[] = { 0, 0, 1, 0xb7 };
	av_log_set_level(AV_LOG_DEBUG);

	if (argc < 3) {
		av_log(NULL, AV_LOG_ERROR, "Usage: %s <output file> <codec name>\n", argv[0]);
		exit(1);
	}

	filename = argv[1];
	codec_name = argv[2];

	/* find the mpeg1 video encoder */
	codec = avcodec_find_encoder_by_name(codec_name);
	if (!codec) {
		av_log(NULL, AV_LOG_ERROR, "Codec not found\n");
		exit(1);
	}

	c = avcodec_alloc_context3(codec);
	if (!c) {
		av_log(NULL, AV_LOG_ERROR, "Could not allocate video codec context\n");
		exit(1);
	}

	/* put sample parameters  */
	c->bit_rate = 400000;
	/* resolution must be a multiple of two */
	c->width = 352;
	c->height = 288;
	/* frames per second */
	c->time_base = (AVRational) { 1, 25 };
	c->framerate = (AVRational) { 25, 1 };
	c->gop_size = 10;
	c->max_b_frames = 1;
	c->pix_fmt = AV_PIX_FMT_YUV420P;

	if (codec->id == AV_CODEC_ID_H264) av_opt_set(c->priv_data, "preset", "slow", 0);

	/* open it */
	if (avcodec_open2(c, codec, NULL) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Open video codec failed\n");
		exit(1);
	}

	frame = av_frame_alloc();
	if (!frame) {
		av_log(NULL, AV_LOG_ERROR, "Could not allocate video frame\n");
		exit(1);
	}
	frame->format = c->pix_fmt;
	frame->width = c->width;
	frame->height = c->height;

	ret = av_frame_get_buffer(frame, 0);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Could not allocate the video frame data\n");
		exit(1);
	}

	pkt = av_packet_alloc();
	if (!pkt) {
		exit(1);
	}

	f = fopen(filename, "wb");
	if (!f) {
		av_log(NULL, AV_LOG_ERROR, "Open %s failed\n", filename);
		exit(1);
	}

	/* encode 1 second of video */
	for (i = 0; i < 25; i++) {
		/* make sure the frame data is writeable  */
		ret = av_frame_make_writable(frame);
		if (ret < 0) {
			exit(1);
		}

		/* prepare a dummy image */
		for (y = 0; y < c->height; y++) {
			for (x = 0; x < c->width; x++) {
				frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
			}
		}

		/* Cb and Cr */
		for (y = 0; y < c->height; y++) {
			for (x = 0; x < c->width; x++) {
				frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
				frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
			}
		}
		
		frame->pts = i;
		av_log(NULL, AV_LOG_DEBUG, "pts :%d \n", frame->pts);
		fflush(stdout);
		/* encode the image */
		encode(c, frame, pkt, f);
	}

	/* fluse the encoder */
	encode(c, NULL, pkt, f);

	/* add sequence end code to have a real mpeg file */
	fwrite(endcode, 1, sizeof(endcode), f);
	fclose(f);

	avcodec_free_context(&c);
	av_frame_free(&frame);
	av_packet_free(&pkt);

	return 0;
}