#ifndef COMI_UTILS_H
#define COMI_UTILS_H

#include "context.h"
#include "log.h"

#define FILES			"/files"
#define COMI_DATA		"/comiData"
#define HASH_LENGTH 	16

static void get_fullpath(char fpath[PATH_MAX], const char *path) 
{
	log_syscall("get_fullpath", path);
	strcpy(fpath, COMI_CONTEXT->rootdir);
	strncat(fpath, path, PATH_MAX);
}

static void divide(char fpath[PATH_MAX], const char *path) 
{
	log_syscall("divide", path);
	char realPath[HASH_LENGTH * 3 + 1];
	for(int i = 0; i < HASH_LENGTH; i++){
		realPath[i*2] = path[i];
		realPath[i*2+1] = '/';
	}
	realPath[HASH_LENGTH * 2] = 0;

	char realPathWithPrefix[PATH_MAX];
	
	strncat(realPath, path, HASH_LENGTH);
	strcpy(realPathWithPrefix, COMI_DATA);
	strncat(realPathWithPrefix, "/", PATH_MAX);
	strncat(realPathWithPrefix, realPath, PATH_MAX);
	get_fullpath(fpath, realPathWithPrefix);
	log_syscall("divide return", fpath);
}

static int get_real_path(char *fullRealPath, const char *path) 
{
	log_syscall("get_real_path", path);
	char fpath[PATH_MAX];
	get_fullpath(fpath, path);
	int fd = open(fpath, O_RDONLY);
	if(fd == -1) {
		return fd;
	}

	char buf[HASH_LENGTH * 2];
	int res = pread(fd, buf, HASH_LENGTH, 0);
	buf[HASH_LENGTH] = 0;
	close(fd);
	if(res == -1) {
		return res;
	}
	
	divide(fullRealPath, buf);
	return 0;
}

static int proxy_open(const char *path, struct fuse_file_info *fi) 
{
	log_syscall("proxy open", path);
	char fpath[PATH_MAX];
	int res = get_real_path(fpath, path);
	if(res == -1) {
		return res;
	}
	if(fi == NULL) {
		return open(fpath, O_RDONLY | O_WRONLY);
	}
	return open(fpath, fi->flags, S_IRWXU);
}

static int get_file_hash(const char *path, char *buf) 
{
	log_syscall("get_file_hash", path);
	char cmd[PATH_MAX] = "shasum -a 256 ";   
	strncat(cmd, path, PATH_MAX);

	FILE *fp;

	if ((fp = popen(cmd, "r")) == NULL) {
		printf("Error opening pipe!\n");
		return -1;
	}

	fgets(buf, HASH_LENGTH + 1, fp);
	buf[HASH_LENGTH] = 0;

	if (pclose(fp)) {
		printf("Command not found or exited with error status\n");
		return -1;
	}

	return 0;
}

static int cp_mv_file(char *cmd, char *src_path, char *dest_path)
{
	strncat(cmd, src_path, PATH_MAX);
	strncat(cmd, " ", 1);
	strncat(cmd, dest_path, PATH_MAX);

    FILE *fp;

    if ((fp = popen(cmd, "r")) == NULL) {
        printf("Error opening pipe!\n");
        return -1;
    }

    if (pclose(fp)) {
        printf("Command not found or exited with error status\n");
        return -1;
    }

    return 0;
}

static int cp_file(char *src_path, char *dest_path) 
{
	char msg[LOG_MAX];
	sprintf(msg, "cp_file src: %s dest: %s\n", src_path, dest_path);
	custom_log(msg);

	char cmd[2*PATH_MAX] = "cp ";
	return cp_mv_file(cmd, src_path, dest_path);
}

static int mv_file(char *src_path, char *dest_path) 
{
	char msg[LOG_MAX];
	sprintf(msg, "mv_file src: %s dest: %s\n", src_path, dest_path);
	custom_log(msg);

	char cmd[2*PATH_MAX] = "mv ";
	return cp_mv_file(cmd, src_path, dest_path);
}

static int same_prefix(const char* path, const char* prefix) 
{
	int path_length = strlen(path);
	int prefix_len = strlen(prefix);

	if (prefix_len > path_length) {
		return 0;
	}
	for (int i=0; i<prefix_len; i++) {
		if (path[i] != prefix[i]) {
			return 0;
		}
	}
	return 1;
}

static int open_file_by_hash(const char * hash) {
	char dst_path[PATH_MAX];
	divide(dst_path, hash);
	int res = open(dst_path, O_CREAT|O_RDWR, 0777);
	return res;
}

static void create_folder_tree(const char* hash) {
	char hashPrefix[PATH_MAX];
	get_fullpath(hashPrefix, COMI_DATA);

	struct stat st = {0};	
	for (int i = 0; i < HASH_LENGTH; i++) {
		strncat(hashPrefix, "/", 1);
		strncat(hashPrefix, hash + i, 1);
		if (stat(hashPrefix, &st) == -1) {
			mkdir(hashPrefix, 0700);
		}
	}
}


#endif
