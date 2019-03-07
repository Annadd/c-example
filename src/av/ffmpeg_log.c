#include <libavutil/log.h>
#include <stdio.h>

int main(int argc,char*argv[] )
{
	av_log_set_level(AV_LOG_DEBUG);
	av_log(NULL,AV_LOG_INFO,"%s \n",argv[0]);
	av_log(NULL,AV_LOG_ERROR,"%d \n",argc);
	return 0;
}
