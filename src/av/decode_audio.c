#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

static void decode(AVCodecContext *dec_ctx, AVFrame* frame, AVPacket* pkt, FILE* outfile)
{
	int i = 0, ch = 0;
	int ret = 0, data_size = 0;

	/* send the packet with the compressed data to the decoder */
	ret = avcodec_send_packet(dec_ctx, pkt);
	if (ret < 0) {
		fprintf(stderr, "Error submitting the packet to the decoder\n");
		exit(1);
	}

	/* read all the output frames (in general there may be any number of them)*/
	while (ret >= 0) {
		ret = avcodec_receive_frame(dec_ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			return;
		}
		else if (ret < 0){
			fprintf(stderr, "Error during decoding\n");
			exit(1);
		}

		data_size = av_get_bytes_per_sample(dec_ctx->sample_fmt);
		if (data_size < 0) {
			fprintf(stderr, "Failed to calculate data size\n");
			exit(1);
		}

		for (i = 0; i < frame->nb_samples; i++) {
			for (ch = 0; ch < dec_ctx->channels; ch++) {
				fwrite(frame->data[ch] + data_size * i, 1, data_size, outfile);
			}
		}
	}
}

int main(int argc, char** argv)
{
	const char *outfilename = NULL, *filename = NULL;
	const AVCodec *codec = NULL;
	AVCodecContext *c = NULL;
	AVCodecParserContext *parser = NULL;
	int ret = 0;
	FILE* f = NULL, *outfile = NULL;
	uint8_t inbuf[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE] = { 0 };
	uint8_t *data = NULL;
	size_t len = 0, data_size = 0;
	AVPacket *pkt = NULL;
	AVFrame *decoded_frame = NULL;

	if (argc <= 2) {
		fprintf(stderr, "Usage: %s <input file> <output file>\n", argv[0]);
		exit(0);
	}
	filename = argv[1];
	outfilename = argv[2];

	pkt = av_packet_alloc();
	if (!pkt) return -1;

	codec = avcodec_find_decoder(AV_CODEC_ID_MP2);
	if (!codec) {
		fprintf(stderr, "Codec not found\n");
		exit(1);
	}

	parser = av_parser_init(codec->id);
	if (!parser) {
		fprintf(stderr, "Parser not found\n");
		exit(1);
	}

	c = avcodec_alloc_context3(codec);
	if (!c) {
		fprintf(stderr, "Could not allocate audio codec context\n");
		exit(1);
	}

	/* open it */
	if (avcodec_open2(c, codec, NULL) < 0) {
		fprintf(stderr, "Could not open codec\n");
		exit(1);
	}

	f = fopen(filename, "rb");
	if (!f) {
		fprintf(stderr, "Could not open %s\n", filename);
		exit(1);
	}
	outfile = fopen(outfilename, "wb");
	if (!outfile) {
		avcodec_free_context(&c);
		exit(1);
	}

	/* decode until eof */
	data = inbuf;
	data_size = fread(inbuf, 1, AUDIO_INBUF_SIZE, f);

	while (data > 0) {
		if (!decoded_frame) {
			if (!(decoded_frame = av_frame_alloc())) {
				fprintf(stderr, "Could not allocate audio frame\n");
				exit(1);
			}
		}

		ret = av_parser_parse2(parser, c, &pkt->data,
			&pkt->size, data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
		if (ret < 0) {
			fprintf(stderr, "Error while parsing\n");
			exit(1);
		}

		data += ret;
		data_size -= ret;

		if (pkt->size) {
			decode(c, decoded_frame, pkt, outfile);
		}

		if (data_size < AUDIO_REFILL_THRESH) {
			memmove(inbuf, data, data_size);
			data = inbuf;
			len = fread(data + data_size, 1, AUDIO_INBUF_SIZE - data_size, f);
			if (len > 0) {
				data_size += len;
			}
		}
	}

	/* flush the decoder */
	pkt->data = NULL;
	pkt->size = 0;

	decode(c, decoded_frame, pkt, outfile);
	fclose(outfile);
	fclose(f);

	avcodec_free_context(&c);
	av_parser_close(parser);
	av_frame_free(&decoded_frame);
	av_packet_free(&pkt);
	return 0;
}