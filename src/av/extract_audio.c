#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/log.h>
#include <stdio.h>

void adts_header(char* szAdtsHeader, int dataLen)
{
	int audio_object_type = 2;
	int sampling_frequency_index = 7;
	int channel_config = 2;

	int adtsLen = dataLen + 7;

	szAdtsHeader[0] = 0xff;//syncword:0xfff

	szAdtsHeader[1] = 0xf0;//syncword:0xfff 
	szAdtsHeader[1] |= (0 << 3); //id,mpeg version:0 for mpeg-4; 1 for mpeg-2
	szAdtsHeader[1] |= (0 << 1);//Layer:0
	szAdtsHeader[1] |= 1;//protection absent:1

	szAdtsHeader[2] = (audio_object_type - 1) << 6;
	szAdtsHeader[2] |= (sampling_frequency_index & 0x0f) << 2;
	szAdtsHeader[2] |= (0 << 1);
	szAdtsHeader[2] |= (channel_config & 0x04) >> 2;

	szAdtsHeader[3] = (channel_config & 0x03) << 6;
	szAdtsHeader[3] |= (0 << 5);
	szAdtsHeader[3] |= (0 << 4);
	szAdtsHeader[3] |= (0 << 3);
	szAdtsHeader[3] |= (0 << 2);
	szAdtsHeader[3] |= (adtsLen & 0x1800) >> 11;

	szAdtsHeader[4] = (uint8_t)((adtsLen & 0x7f8) >> 3);

	szAdtsHeader[5] = (uint8_t)((adtsLen & 0x7) << 5);
	szAdtsHeader[5] |= 0x1f;

	szAdtsHeader[6] = 0xfc;
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

	ret = avformat_open_input(&fctx, src_file, NULL, NULL);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Can't open file:%s\n", src_file);
		return -1;
	}

	av_dump_format(fctx, 0, src_file, 0);

	dst_fd = fopen(dst_file, "wb");
	if (!dst_fd) {
		av_log(NULL, AV_LOG_ERROR, "Can't open file:%s\n", dst_file);
		goto _close_input;
	}

	//get audio stream 
	ret = av_find_best_stream(fctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Can't find the best stream..\n");
		goto _close_input;
	}

	AVPacket pkt;
	size_t len = 0;
	int audio_index = ret;
	av_init_packet(&pkt);

	while (av_read_frame(fctx, &pkt) >= 0) {
		if (pkt.stream_index == audio_index) {
			//write acc adts header 7 byte 
			char adts_header_buf[7];
			adts_header(adts_header_buf, pkt.size);
			fwrite(adts_header_buf, 1, 7, dst_fd);

			//write audio data to acc file 
			len = fwrite(pkt.data, 1, pkt.size, dst_fd);
			if (len != pkt.size) {
				av_log(NULL, AV_LOG_WARNING,
					"warning: lenght of data is not equal size of pkt..\n");
			}
		}

		av_packet_unref(&pkt);
	}

_close_input:
	avformat_close_input(&fctx);

	if (dst_fd) {
		fclose(dst_fd);
	}

	return 0;
}
