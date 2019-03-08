#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/log.h>

static void log_packet(const AVFormatContext* fmt_ctx, const AVPacket* pkt, const char* tag)
{
	AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;
	av_log(NULL, AV_LOG_INFO, "%s: pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
	   tag,
	   av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
	   av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
	   av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
	   pkt->stream_index);
}

int main(int argc,char** argv)
{
	AVOutputFormat* ofmt = NULL;
	AVFormatContext* ifmt_ctx = NULL, *ofmt_ctx = NULL;
	AVPacket pkt;
	const char* in_filename = NULL,*out_filename = NULL;
	int ret = 0, i = 0;
	int stream_index = 0;
	int* stream_mapping = NULL;
	int stream_mapping_size = 0;
	
	if(argc < 3){
		av_log(NULL, AV_LOG_ERROR, "usage: %s input output\nAPI example program to remux a media file with libavformat and libavcodec.\nThe output format is guessed according to the file extension.\n\n",argv[0]);
		return -1;
	}
	
	in_filename = argv[1];
	out_filename = argv[2];
	
	if ((ret = avformat_open_input(&ifmt_ctx, in_filename, NULL, NULL)) < 0){
		av_log(NULL, AV_LOG_ERROR, "Could not open input file '%s'", in_filename);
		return -1;
	}
	
	if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0){
		av_log(NULL, AV_LOG_ERROR, "Failed to retrieve input stream information");
		goto _end;
	}
	
	av_dump_format(ifmt_ctx, 0, in_filename, 0);
	
	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
	if(!ofmt_ctx){
		av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto _end;
	}
	
	stream_mapping_size = ifmt_ctx->nb_streams;
	stream_mapping = av_mallocz_array(stream_mapping_size, sizeof(*stream_mapping));
	if(!stream_mapping){
		ret = AVERROR(ENOMEM);
		goto _end;
	}
	
	ofmt = ofmt_ctx->oformat;
	
	AVStream *out_stream = NULL, *in_stream = NULL;
	for(i = 0; i < ifmt_ctx->nb_streams; i++){
		out_stream = NULL;
		in_stream = ifmt_ctx->streams[i];
		AVCodecParameters *in_codecpar = in_stream->codecpar;
		
		if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO && 
		    in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO && 
			in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE){
			stream_mapping[i] = -1;
			continue;
		}
		
		stream_mapping[i] = stream_index++;
		
		out_stream = avformat_new_stream(ofmt_ctx, NULL);
		if(!out_stream){
			av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
			ret = AVERROR_UNKNOWN;
			goto _end;
		}
		
		ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
		if (ret < 0){
			av_log(NULL, AV_LOG_ERROR, "Failed to copy parameters\n");
			goto _end;
		}
		out_stream->codecpar->codec_tag = 0;
	}
	av_dump_format(ofmt_ctx, 0, out_filename, 1);
	
	if (!(ofmt->flags & AVFMT_NOFILE)){
		ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
		if (ret < 0){
			av_log(NULL, AV_LOG_ERROR, "Could not open output file: %s\n",out_filename);
			goto _end;
		}
	}
	
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0){
		av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
		goto _end;
	}
	
	while (1) {
		ret = av_read_freme(ifmt_ctx, &pkt);
		if (ret < 0)
			break;
		
		in_stream = ifmt_ctx->streams[pkt.stream_index];
		if (pkt.stream_index >= stream_mapping_size || 
		   stream_mapping[pkt.stream_index] < 0){
			av_packet_unref(&pkt);
			continue; 
		}
		
		pkt.stream_index = stream_mapping[pkt.stream_index];
		out_stream = ofmt_ctx->streams[pkt.stream_index];
		log_packet(ifmt_ctx, &pkt, "in");
		
		/*copy packet*/
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;
		log_packet(ofmt_ctx, &pkt, "out");
		
		ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
		if (ret < 0){
			av_log(NULL, AV_LOG_ERROR, "Error muxing packet\n");
			break;
		}
		av_packet_unref(&pkt);
	}
	av_write_trailer(ofmt_ctx);
	
_end:
	avformat_close_input(&ifmt_ctx);
	
	/* close output */
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE)){
		avio_closep(&ofmt_ctx->pb);
	}
	avformat_free_context(ofmt_ctx);
	av_freep(&stream_mapping);
	
	if (ret < 0 && ret != AVERROR_EOF){
		av_log(NULL, AV_LOG_ERROR, "Error occurred: %s \n",av_err2str(ret));
		return -1;
	}
	return 0;
}