#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

#define  INBUF_SIZE 4096

#pragma pack(1)
typedef struct {
	unsigned short bfType;
	unsigned long bfSize; 
	unsigned short bfReserved1;
	unsigned short bfReserved2; 
	unsigned long bfOffBits;  
} BITMAPFILEHEADER;

typedef struct {
	unsigned long biSize; 
	long biWidth; 
	long biHeight;
	unsigned short biPlanes; 
	unsigned short biBitCount;//rgb-24 
	unsigned long biCompression;
	unsigned long biSizeImage; 
	long biXPelsPerMeter;  
	long biYPelsPerMeter; 
	unsigned long biClrUsed;
	unsigned long biClrImportant;
} BITMAPINFOHEADER;
#pragma pack()

void writeBmp(const char *filename, uint8_t *pRGBBuffer, int width, int height, int bpp)
{
	BITMAPFILEHEADER bmpheader;
	BITMAPINFOHEADER bmpinfo;
	FILE *fp = NULL;

	fp = fopen(filename, "wb");
	if (fp == NULL){
		return;
	}

	bmpheader.bfType = ('M' << 8) | 'B';
	bmpheader.bfReserved1 = 0;
	bmpheader.bfReserved2 = 0;
	bmpheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmpheader.bfSize = bmpheader.bfOffBits + width * height * bpp / 8;

	bmpinfo.biSize = sizeof(BITMAPINFOHEADER);
	bmpinfo.biWidth = width;
	bmpinfo.biHeight = 0 - height;
	bmpinfo.biPlanes = 1;
	bmpinfo.biBitCount = bpp;
	bmpinfo.biCompression = 0x0;
	bmpinfo.biSizeImage = 0;
	bmpinfo.biXPelsPerMeter = 100;
	bmpinfo.biYPelsPerMeter = 100;
	bmpinfo.biClrUsed = 0;
	bmpinfo.biClrImportant = 0;

	fwrite(&bmpheader, sizeof(BITMAPFILEHEADER), 1, fp);
	fwrite(&bmpinfo, sizeof(BITMAPINFOHEADER), 1, fp);
	fwrite(pRGBBuffer, width*height*bpp / 8, 1, fp);
	fclose(fp);
	fp = NULL;
}

static void yuvToRgb(struct SwsContext* img_convert_ctx, AVFrame* src_picture, const char* filename)
{
	uint8_t* dst_buffer = { NULL };
	AVFrame	*frame_rgb = NULL;

	frame_rgb = av_frame_alloc();

	dst_buffer = malloc(src_picture->width * src_picture->height * 3);
	if (!dst_buffer)return;

	av_image_fill_arrays(frame_rgb->data, frame_rgb->linesize, dst_buffer,
		AV_PIX_FMT_RGB24, src_picture->width, src_picture->height, 1);

	int ret = sws_scale(img_convert_ctx, src_picture->data, src_picture->linesize, 0, src_picture->height,
		frame_rgb->data, frame_rgb->linesize);
	
	if (ret > 0) {
		writeBmp(filename, dst_buffer, src_picture->width, src_picture->height, 24);
	}

	free(dst_buffer);
	av_frame_free(&frame_rgb);
}

static int decode_write_frame(const char* outfilename, AVCodecContext *avctx, struct SwsContext* img_convert_ctx, AVFrame* frame, AVPacket* pkt)
{
	int ret = 0;
	char buf[1024] = { 0 };

	ret = avcodec_send_packet(avctx, pkt);
	if (ret < 0) {
		fprintf(stderr, "Error sending a packet for decoding\n");
		return -1;
	}

	while (ret >= 0) {
		ret = avcodec_receive_frame(avctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return 0;
		else if (ret < 0) {
			fprintf(stderr, "Error during decoding\n");
			return -1;
		}

		fprintf(stdout, "saving frame %3d\n", avctx->frame_number);
		fflush(stdout);

		/* the picture is allocated by the decoder. no need to free it */
		snprintf(buf, sizeof(buf), "%s-%d.bmp", outfilename, avctx->frame_number);
		yuvToRgb(img_convert_ctx, frame, buf);
	}

	return 0;
}

int main(int argc, char** argv)
{
	int ret = 0;
	FILE *f = NULL;
	const char *filename = NULL, *outfilename = NULL;
	AVFormatContext *fmt_ctx = NULL;
	AVCodecContext *c = NULL;
	const AVCodec *codec = NULL;
	AVStream *st = NULL;
	int stream_index = -1;
	AVFrame *frame = NULL;

	struct SwsContext *img_convert_ctx = NULL;

	AVPacket *avpkt = NULL;

	if (argc <= 2) {
		fprintf(stderr, "Usage: %s <input file> <output file>\n", argv[0]);
		exit(0);
	}

	filename = argv[1];
	outfilename = argv[2];

	avpkt = av_packet_alloc();
	frame = av_frame_alloc();
	if (!avpkt || !frame) {
		fprintf(stderr, "allocate apcket or frame failed.\n");
		exit(1);
	}

	if (avformat_open_input(&fmt_ctx, filename, NULL, NULL) < 0) {
		fprintf(stderr, "Could not open source file %s\n", filename);
		exit(1);
	}

	if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
		fprintf(stderr, "Could not find stream information\n");
		exit(1);
	}

	av_dump_format(fmt_ctx, 0, filename, 0);

	ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (ret < 0) {
		fprintf(stderr, "Could not find %s stream in input file %s\n", av_get_media_type_string(AVMEDIA_TYPE_VIDEO), filename);
		goto _close;
	}

	stream_index = ret;
	st = fmt_ctx->streams[stream_index];

	codec = avcodec_find_decoder(st->codecpar->codec_id);
	if (!codec) {
		fprintf(stderr, "Failed to find %s codec\n",
			av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
		goto _close;
	}

	c = avcodec_alloc_context3(NULL);
	if (!c) {
		fprintf(stderr, "Could not allocate video codec context\n");
		goto _close;
	}

	ret = avcodec_parameters_to_context(c, st->codecpar);
	if (ret < 0) {
		fprintf(stderr, "Failed to copy %s codec parameters to decode context\n",
			av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
		goto _close;
	}

	if (avcodec_open2(c, codec, NULL) < 0) {
		fprintf(stderr, "Could not open codec\n");
		goto _close;
	}

	img_convert_ctx = sws_getContext(c->width, c->height,
		c->pix_fmt, c->width, c->height, AV_PIX_FMT_RGB24,
		SWS_BICUBIC, NULL, NULL, NULL);
	if (!img_convert_ctx) {
		fprintf(stderr, "sws context allocate failed.\n");
		goto _close;
	}

	while (av_read_frame(fmt_ctx, avpkt) >= 0) {
		if (avpkt->stream_index == stream_index) {

			ret = decode_write_frame(outfilename, c, img_convert_ctx, frame, avpkt);
			if (ret < 0) { goto _close; }
		}

		av_packet_unref(avpkt);
	}

_close:
	if (c) {
		avcodec_free_context(&c);
	}
	if (avpkt) {
		av_packet_free(&avpkt);
	}
	if (frame) {
		av_frame_free(&frame);
	}

	if (img_convert_ctx) {
		sws_freeContext(img_convert_ctx);
	}

	avformat_close_input(&fmt_ctx);
	return 0;
}