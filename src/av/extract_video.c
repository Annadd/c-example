#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/log.h>
#include <stdio.h>

#ifndef AV_WB32 
#define AV_WB32(p, val)   \
do { \
	uint32_t d = (val); \
	((uint8_t*)(p))[3] = (d);	\
	((uint8_t*)(p))[2] = (d) >> 8;	\
	((uint8_t*)(p))[1] = (d) >> 16;	\
	((uint8_t*)(p))[0] = (d) >> 24;	\
}while(0)
#endif

#ifndef AV_RB16
#define AV_RB16(x)  ((((uint8_t*)(x))[0] << 8) | ((uint8_t*)(x))[1])
#endif

int alloc_and_copy(AVPacket* out, const uint8_t* sps_pps, uint32_t sps_pps_size,
	const uint8_t* in, uint32_t in_size)
{
	uint32_t offset = out->size;
	uint8_t nal_header_size = offset ? 3 : 4;

	int err = av_grow_packet(out, sps_pps_size + in_size + nal_header_size);
	if (err < 0)
		return err;

	if (sps_pps) {
		memcpy(out->data + offset, sps_pps, sps_pps_size);
	}
	memcpy(out->data + sps_pps_size + nal_header_size + offset, in, in_size);
	if (!offset) {
		AV_WB32(out->data + sps_pps_size,  1);
	}
	else {
		(out->data + offset + sps_pps_size)[0] =
			(out->data + offset + sps_pps_size)[1] = 0;
		(out->data + offset + sps_pps_size)[2] = 1;
	}

	return 0;
}

int h264_extradata_to_annexb(const uint8_t* codec_extradata, const int codec_extradata_size,
	AVPacket* out_extradata, int padding)
{
	uint16_t unit_size = 0;
	uint64_t total_size = 0;
	uint8_t* out = NULL, unit_nb = 0, sps_done = 0,
		sps_seen = 0, pps_seen = 0, sps_offset = 0, pps_offset = 0;
	const uint8_t* extradata = codec_extradata + 4;
	static const uint8_t nalu_header[] = { 0, 0, 0, 1 };
	int length_size = (*extradata & 0x3) + 1;//retrieve length coded size.

	sps_offset = pps_offset = -1;

	/* retrieve sps and pps unit(s) */
	unit_nb = *extradata++ & 0x1f;//number of sps unit(s)

	if (!unit_nb) {
		goto pps;
	}
	else {
		sps_offset = 0;
		sps_seen = 1;
	}

	while (unit_nb--) {
		int err = 0;
		unit_size = AV_RB16(extradata);
		total_size += unit_size + 4;
		if (total_size > INT_MAX - padding) {
			av_log(NULL, AV_LOG_ERROR, "too big extradata size, corrupted stream of invalid Mp4/AVCC bitstream\n");
			av_free(out);
			return AVERROR(EINVAL);
		}

		if (extradata + 2 + unit_size > codec_extradata + codec_extradata_size) {
			av_log(NULL, AV_LOG_ERROR, "packet header is not contained in global extradata, corrupted stream or invalid MP4/AVCC bitstream\n");
			av_free(out);
			return AVERROR(EINVAL);
		}

		err = av_reallocp(&out, total_size + padding);
		if (err < 0) return err;
		memcpy(out + total_size - unit_size - 4, nalu_header, 4);
		memcpy(out + total_size - unit_size, extradata + 2, unit_size);

		extradata += 2 + unit_size;
	pps:
		if (!unit_nb && !sps_done++) {
			unit_nb = *extradata++;//number of pps unit(s)
			if (unit_nb) {
				pps_offset = total_size;
				pps_seen = 1;
			}
		}
	}

	if (out)
		memset(out + total_size, 0, padding);

	if (!sps_seen)
		av_log(NULL, AV_LOG_WARNING, "SPS NALU missing or invalid. The resulting stream may not play.\n");

	if (!pps_seen)
		av_log(NULL, AV_LOG_WARNING, "PPS NALU missing or invalid. The resulting stream may not play.\n");

	out_extradata->data = out;
	out_extradata->size = total_size;

	return length_size;
}

int h264_mp4toannexb(AVFormatContext* fmt_ctx, AVPacket* in, FILE* dst_fd)
{
	AVPacket* out = NULL;
	AVPacket spspps_pkt;

	uint32_t nal_size = 0;
	uint8_t unit_type = 0;
	uint32_t cumul_size = 0;
	const uint8_t *buf = NULL, *buf_end = NULL;
	int buf_size = 0, ret = 0, i = 0, len = 0;

	out = av_packet_alloc();

	buf = in->data;
	buf_size = in->size;
	buf_end = in->data + in->size;

	do {
		ret = AVERROR(EINVAL);

		if (buf + 4 > buf_end) {
			goto _failed;
		}

		for (nal_size = 0, i = 0; i < 4; i++) {
			nal_size = (nal_size << 8) | buf[i];
		}
			

		buf += 4;/* s->length size */
		unit_type = *buf & 0x1f;

		if (nal_size > buf_end - buf || nal_size < 0) {
			goto _failed;
		}
			

		/* prepend only to the first type t nal unit of an IDR picture,
		if no sps/pps are already present */
		if (unit_type == 5) {
			h264_extradata_to_annexb(fmt_ctx->streams[in->stream_index]->codecpar->extradata,
				fmt_ctx->streams[in->stream_index]->codecpar->extradata_size, &spspps_pkt, AV_INPUT_BUFFER_PADDING_SIZE);

			ret = alloc_and_copy(out, spspps_pkt.data, spspps_pkt.size, buf, nal_size);
			if (ret < 0) {
				goto _failed;
			}
		}
		else {
			ret = alloc_and_copy(out, NULL, 0, buf, nal_size);

			if (ret < 0) {
				goto _failed;
			}
		}

		len = fwrite(out->data, 1, out->size, dst_fd);
		if (len != out->size) {
			av_log(NULL, AV_LOG_ERROR, "length of writed data isn't equal pkt.size(%d ,%d)", len, out->size);
		}

		fflush(dst_fd);

	next_nal:
		buf += nal_size;
		cumul_size += nal_size + 4;
	} while (cumul_size < buf_size);

_failed:
	av_packet_free(&out);
	return ret;
}

int main(int argc, char* argv[])
{
	AVFormatContext* fctx = NULL;
	int ret = 0;
	const char* src_file = NULL;
	const char* dst_file = NULL;
	FILE* dst_fd = NULL;

	av_log_set_level(AV_LOG_INFO);
	//av_register_all();

	if (argc < 3) {
		av_log(NULL, AV_LOG_ERROR, "please input the count of params should be more then three!\n");
		return -1;
	}

	src_file = argv[1];
	dst_file = argv[2];
	if (!src_file || !dst_file) {
		av_log(NULL, AV_LOG_ERROR, "src or dst params is null.\n");
		return -1;
	}

	/* open input media file, and allocate format context */
	ret = avformat_open_input(&fctx, src_file, NULL, NULL);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Can't open file:%s\n", src_file);
		return -1;
	}

	/* retrieve video stream */
	ret = avformat_find_stream_info(fctx, NULL);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "find stream infomation failed.\n");
		goto _close_input;
	}

	/* dump input information */
	av_dump_format(fctx, 0, src_file, 0);

	dst_fd = fopen(dst_file, "wb");
	if (!dst_fd) {
		av_log(NULL, AV_LOG_ERROR, "Can't open file:%s\n", dst_file);
		goto _close_input;
	}

	//find best video stream
	ret = av_find_best_stream(fctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Can't find the best stream..\n");
		goto _close_input;
	}

	AVPacket pkt;
	size_t len = 0;
	int video_index = ret;
	av_init_packet(&pkt);

	/* read frames from media file */
	while (av_read_frame(fctx, &pkt) >= 0) {
		if (pkt.stream_index == video_index) {
			//write video data to h264 file 
			h264_mp4toannexb(fctx, &pkt, dst_fd);
		}

		//release pkt->data 
		av_packet_unref(&pkt);
	}

_close_input:
	//close input media file 
	avformat_close_input(&fctx);

	if (dst_fd) {
		fclose(dst_fd);
	}

	return 0;
}
