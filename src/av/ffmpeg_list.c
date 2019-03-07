#include <libavutil/log.h>
#include <libavformat/avio.h>
#include <libavformat/avformat.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	AVIODirContext* dir_ctx = NULL;
	AVIODirEntry* entry = NULL;

	int ret = 0;
	const char* dir_name = "./";

	av_log_set_level(AV_LOG_INFO);

	ret = avio_open_dir(&dir_ctx, dir_name, NULL);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "open %s dir failed...", dir_name);
		return -1;
	}

	while (1) {
		if (avio_read_dir(dir_ctx, &entry) < 0) {
			av_log(NULL, AV_LOG_ERROR, "Can't list directory.\n");
			break;
		}

		if (!entry) {
			break;
		}
		av_log(NULL, AV_LOG_INFO, "%ld  %s\n", entry->size, entry->name);
		avio_free_directory_entry(&entry);
	}

	avio_close_dir(&dir_ctx);
	return 0;
}
