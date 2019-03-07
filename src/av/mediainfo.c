#include <libavformat/avformat.h>
#include <libavutil/log.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	AVFormatContext* fctx = NULL;
	int ret = 0;

	const char* file_name = NULL;
	av_log_set_level(AV_LOG_INFO);
	av_register_all();

	if (argc < 2) {
		av_log(NULL, AV_LOG_ERROR, "please input file name..\n");
		return -1;
	}

	file_name = argv[1];
	ret = avformat_open_input(&fctx, file_name, NULL, NULL);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Can't open file:%s\n", file_name);
		return -1;
	}

	av_dump_format(fctx, 0, file_name, 0);
	avformat_close_input(&fctx);
	return 0;
}
