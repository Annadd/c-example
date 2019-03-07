#include <libavformat/avformat.h>
#include <libavutil/log.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	const char* src_file = "src.txt";
	const char* dst_file = "dst.txt";
	int ret = 0;

	av_log(NULL, AV_LOG_INFO, "src file name %s, dst file name %s", src_file, dst_file);
	//rename file 
	ret = avpriv_io_move(src_file, dst_file);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "%s move %s failed..\n", src_file, dst_file);
		return -1;
	}
	av_log(NULL, AV_LOG_INFO, "move ok..\n");

	//delete file
	ret = avpriv_io_delete(dst_file);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "delete %s failed..", dst_file);
		return -1;
	}

	av_log(NULL, AV_LOG_INFO, "delete ok..\n");
	return 0;
}
