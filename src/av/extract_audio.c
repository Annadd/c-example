#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/log.h>
#include <stdio.h>

typedef enum {
	MAIN_PROFILE = 0,
	LOW_COMPLEXITY_PROFILE = 1,
	SCALABLE_SAMPLING_RATE_PROFILE = 2,
	RESERVED_PROFILE = 3
}AudioType;

typedef enum{
	MPEG_4_VRESION = 0,
	MPEG_2_VERSION = 1
}MpegVersion;
	
typedef enum{
	CRC_HEADER_9_BYTE = 0,
	NO_CRC_HEADER_7_BYTE = 1
}ProtectionAbsent;

typedef enum{
	HZ_96000 = 0,
	HZ_88200,
	HZ_64000,
	HZ_48000,
	HZ_44100,
	HZ_32000,
	HZ_24000,
	HZ_22050,
	HZ_16000,
	HZ_12000,
	HZ_11025,
	HZ_8000,
	HZ_7350,
}SampleFrequencies;

typedef enum{
	CHANNEL_1_FRONT_CENTER = 1,
	CHANNEL_2_FRONT_LEFT_RIGHT = 2,
	CHANNEL_3_FRONT_CENTER_LEFT_RIGHT = 3,
	CHANNEL_4_FRONT_CENTER_LEFT_RIGHT_BACK_CENTER = 4,
}ChannelConfig;

void adts_header(char* szAdtsHeader, int dataLen)
{
	int adtsLen = dataLen + 7;
	int freIndex = HZ_44100;
	int chanCfg = CHANNEL_2_FRONT_LEFT_RIGHT;
	int profile = LOW_COMPLEXITY_PROFILE;
	
	szAdtsHeader[0] = 0xff;//syncword:0xfff

	szAdtsHeader[1] = 0xf0;//syncword:0xfff   syncword: 12bit
	szAdtsHeader[1] |= (MPEG_2_VERSION << 3); //id 1bit,mpeg version:0 for mpeg-4; 1 for mpeg-2
	szAdtsHeader[1] |= (0 << 1);//Layer 2bit : always: 00
	szAdtsHeader[1] |= NO_CRC_HEADER_7_BYTE;//protection absent 1bit: 1 == 7byte  0 == 9byte

	szAdtsHeader[2] = (profile - 1) << 6;   //profile  2bit:
	szAdtsHeader[2] |= (freIndex & 0x0f) << 2;//sampling frequency index 4bit:
	szAdtsHeader[2] |= (0 << 1);//private_bit 1bit:
	szAdtsHeader[2] |= (chanCfg >> 2) ;//channel config 3bit:

	szAdtsHeader[3] = (chanCfg & 0x3) << 6;//channel config 3bit:
	szAdtsHeader[3] |= (0 << 5);//original copy 1bit:
	szAdtsHeader[3] |= (0 << 4);//home 1bit:
	szAdtsHeader[3] |= (0 << 3);//copyright identification bit 1bit:
	szAdtsHeader[3] |= (0 << 2);//copyright identification start 1bit:
	
	szAdtsHeader[3] |= (adtsLen >> 11);//acc frame length 13bit:
	szAdtsHeader[4] = (uint8_t)((adtsLen & 0x7ff) >> 3);
	szAdtsHeader[5] = (uint8_t)((adtsLen & 0x7) << 5);
	
	szAdtsHeader[5] |= 0x1f;//adts buffer fullness 11bit:

	szAdtsHeader[6] = 0xfc;//number of raw data blocks in frame 2bit:
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
