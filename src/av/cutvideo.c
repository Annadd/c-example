#include <libavformat/avformat.h>
#include <libavutil/log.h>
#include <libavutil/timestamp.h>

static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const char *tag)
{
	AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

	printf("%s: pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
		tag,
		av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
		av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
		av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
		pkt->stream_index);
}

void cut_video(double startime, double endtime, const char* in_filename, const char* out_filename)
{
	AVOutputFormat *ofmt = NULL;
	AVFormatContext* ifmt_ctx = NULL, *ofmt_ctx = NULL;
	AVPacket pkt;
	int ret = 0, i = 0;

	if ((ret = avformat_open_input(&ifmt_ctx, in_filename, NULL, NULL))) {
		av_log(NULL, AV_LOG_ERROR, "Could not open input file '%s'", in_filename);
		return;
	}

	if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Failed to retrieve input stream information");
		goto _end;
	}

	av_dump_format(ifmt_ctx, 0, in_filename, 0);

	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
	if (!ofmt_ctx) {
		av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto _end;
	}

	ofmt = ofmt_ctx->oformat;

	AVStream *out_stream = NULL, *in_stream = NULL;
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		in_stream = ifmt_ctx->streams[i];

		out_stream = avformat_new_stream(ofmt_ctx, NULL);
		if (!out_stream) {
			av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream.\n");
			goto _end;
		}

		ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "failed to copy code context\n");
			goto _end;
		}

		out_stream->codecpar->codec_tag = 0;
	}

	av_dump_format(ofmt_ctx, 0, out_filename, 1);

	if (!(ofmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "open %s failed.\n", out_filename);
			goto _end;
		}
	}

	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "write header failed. \n");
		goto _end;
	}

	ret = av_seek_frame(ifmt_ctx, -1, startime * AV_TIME_BASE, AVSEEK_FLAG_ANY);
	if (ret < 0) { 
		av_log(NULL, AV_LOG_ERROR, "seek failed. \n");
		goto _end;
	}

	int64_t *dts_start_from = malloc(sizeof(int64_t) * ifmt_ctx->nb_streams);
	memset(dts_start_from, 0, sizeof(int64_t) * ifmt_ctx->nb_streams);

	int64_t* pts_start_from = malloc(sizeof(int64_t) * ifmt_ctx->nb_streams);
	memset(pts_start_from, 0, sizeof(int64_t) * ifmt_ctx->nb_streams);

	while (1) {
		ret = av_read_frame(ifmt_ctx, &pkt);
		if (ret < 0) { 
			av_log(NULL, AV_LOG_ERROR, "read frame failed.\n");
			break;
		}

		in_stream = ifmt_ctx->streams[pkt.stream_index];
		out_stream = ofmt_ctx->streams[pkt.stream_index];
		log_packet(ifmt_ctx, &pkt, "in");

		if (av_q2d(in_stream->time_base) * pkt.pts > endtime) {
			av_packet_unref(&pkt);
			break;
		}

		if (dts_start_from[pkt.stream_index] == 0) {
			dts_start_from[pkt.stream_index] = pkt.dts;
			av_log(NULL, AV_LOG_INFO, "dts %d %s \n", pkt.stream_index,
				av_ts2str(dts_start_from[pkt.stream_index]));
		}

		if (pts_start_from[pkt.stream_index] == 0) {
			pts_start_from[pkt.stream_index] = pkt.pts;
			av_log(NULL, AV_LOG_INFO, "pts %d %s \n", pkt.stream_index,
				av_ts2str(dts_start_from[pkt.stream_index]));
		}

		int pts = abs(pkt.pts - pts_start_from[pkt.stream_index]);
		int dts = abs(pkt.dts - dts_start_from[pkt.stream_index]);

		pkt.pts = av_rescale_q_rnd(pts, in_stream->time_base, out_stream->time_base,
			AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
		pkt.dts = av_rescale_q_rnd(dts, in_stream->time_base, out_stream->time_base,
			AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
		av_log(NULL, AV_LOG_DEBUG, " pts: %d dts: %d \n", pkt.pts, pkt.dts);

		if (pkt.pts < 0) { pkt.pts = 0; }
		if (pkt.dts < 0) { pkt.dts = 0; }
		if (pkt.pts < pkt.dts) { pkt.pts = pkt.dts; }

		pkt.duration = (int)av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;
		log_packet(ofmt_ctx, &pkt, "out");

		ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "error muxing pakcet\n");
			break;
		}

		av_packet_unref(&pkt);
	}

	free(dts_start_from);
	free(pts_start_from);
	av_write_trailer(ofmt_ctx);

_end:
	avformat_close_input(&ifmt_ctx);
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE)){
		avio_closep(&ofmt_ctx->pb);
	}
	avformat_free_context(ofmt_ctx);
}

int main(int argc, char *argv[])
{
	if (argc < 5) {
		av_log(NULL, AV_LOG_ERROR, "Usage: command startime, endtime, srcfile, outfile\n");
		return -1;
	}

	double startime = atoi(argv[1]);
	double endtime = atoi(argv[2]);

	av_log_set_level(AV_LOG_INFO);

	cut_video(startime, endtime, argv[3], argv[4]);
	return 0;
}