#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "../include/comi_utils.h"
#include "../include/context.h"
#include "../include/log.h"

void *comi_init(struct fuse_conn_info *conn)
{
    fuse_get_context();

    return COMI_CONTEXT;
}

static int comi_getattr(const char *path, struct stat *stbuf)
{
	log_syscall("Comi_getattr", path);
	int res;
	char fpath[PATH_MAX];
	get_fullpath(fpath, path);
	res = lstat(fpath, stbuf);

	if (S_ISREG(stbuf->st_mode) && same_prefix(path, FILES)) {
		char proxy_path[PATH_MAX];
		get_real_path(proxy_path, path);
		mode_t tmp_mode = stbuf->st_mode;
		res = lstat(proxy_path, stbuf);
		stbuf->st_mode = tmp_mode;
	}

	if (res == -1) {
		return -errno;
	}

	return 0;
}

static int comi_access(const char *path, int mask)
{
	log_syscall("Comi_access", path);
	char fpath[PATH_MAX];
	get_fullpath(fpath, path);
	int res = access(fpath, mask);
	if (res == -1) {
		return -errno;
	}

	return 0;
}

static int comi_readlink(const char *path, char *buf, size_t size)
{
	log_syscall("Comi_readlink", path);
	char fpath[PATH_MAX];
	get_fullpath(fpath, path);
	int res = readlink(fpath, buf, size - 1);
	if (res == -1) {
		return -errno;
	}

	buf[res] = '\0';
	return 0;
}


static int comi_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	log_syscall("Comi_readdir", path);
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;
	char fpath[PATH_MAX];
	get_fullpath(fpath, path);
	dp = opendir(fpath);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int comi_mknod(const char *path, mode_t mode, dev_t rdev)
{
	log_syscall("Comi_mknod", path);
	int res;
	char fpath[PATH_MAX];
	get_fullpath(fpath, path);
	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(fpath, mode);
	else
		res = mknod(fpath, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int comi_mkdir(const char *path, mode_t mode)
{
	log_syscall("Comi_mkdir", path);
	char fpath[PATH_MAX];
	get_fullpath(fpath, path);
	int res = mkdir(fpath, mode);
	if (res == -1) {
		return -errno;
	}

	return 0;
}

static int comi_symlink(const char *from, const char *to)
{
	log_syscall("Comi_symlink", from);
	char ffrom[PATH_MAX];
	get_fullpath(ffrom, from);
	char fto[PATH_MAX];
	get_fullpath(fto, to);
	int res = symlink(ffrom, fto);
	if (res == -1) {
		return -errno;
	}

	return 0;
}

static int comi_unlink(const char *path)
{
	log_syscall("Comi_unlink", path);
	char fpath[PATH_MAX];
	get_fullpath(fpath, path);
	int res = unlink(fpath);
	if (res == -1) {
		return -errno;
	}

	return 0;
}

static int comi_rmdir(const char *path)
{
	log_syscall("Comi_rmdir", path);
	char fpath[PATH_MAX];
	get_fullpath(fpath, path);
	int res = rmdir(fpath);
	if (res == -1) {
		return -errno;
	}

	return 0;
}

static int comi_rename(const char *from, const char *to)
{
	log_syscall("Comi_rename", from);
	char ffrom[PATH_MAX];
	get_fullpath(ffrom, from);
	char fto[PATH_MAX];
	get_fullpath(fto, to);
	int res = rename(ffrom, fto);
	if (res == -1) {
		return -errno;
	}

	return 0;
}

static int comi_link(const char *from, const char *to)
{
    log_syscall("Comi_link", from);
	char ffrom[PATH_MAX];
	get_fullpath(ffrom, from);
	char fto[PATH_MAX];
	get_fullpath(fto, to);
	int res = link(ffrom, fto);
	if (res == -1) {
		return -errno;
	}

	return 0;
}

static int comi_chmod(const char *path, mode_t mode)
{
  log_syscall("Comi_chmod", path);
	char fpath[PATH_MAX];
	get_fullpath(fpath, path);
	int res = chmod(fpath, mode);
	if (res == -1) {
		return -errno;
	}

	return 0;
}

static int comi_chown(const char *path, uid_t uid, gid_t gid)
{
    log_syscall("Comi_chown", path);
	int res;
	char fpath[PATH_MAX];
	get_fullpath(fpath, path);
	res = lchown(fpath, uid, gid);
	if (res == -1) {
		return -errno;
	}

	return 0;
}

static int comi_truncate(const char *path, off_t size)
{
	log_syscall("Comi_truncate", path);
	char fpath[PATH_MAX];
	get_fullpath(fpath, path);

	int res = truncate(fpath, size);
	int fd = open(fpath, O_RDWR);
	char hash[HASH_LENGTH + 1];
	get_file_hash(fpath, hash);

	char msg[LOG_MAX];
	sprintf(msg, "Hash value after truncate %s\n", hash);
	custom_log(msg);

	pwrite(fd, hash, HASH_LENGTH, 0);

	create_folder_tree(hash);
	res = open_file_by_hash(hash);

	if (res == -1) {
		return -errno;
	}

	return 0;
}

#ifdef HAVE_UTIMENSAT
static int comi_utimens(const char *path, const struct timespec ts[2])
{
  log_syscall("Comi_utimens", path);
	char fpath[PATH_MAX];
	get_fullpath(fpath, path);
	/* don't use utime/utimes since they follow symlinks */
	int res = utimensat(0, fpath, ts, AT_SYMLINK_NOFOLLOW);
	if (res == -1) {
		return -errno;
	}

	return 0;
}
#endif


static int comi_open(const char *path, struct fuse_file_info *fi)
{
	log_syscall("Comi_open", path);
	int fd;
	
	fd = proxy_open(path, fi);
	if (fd == -1) {
		return -1;
	}
	fi->fh = fd;
	return 0;
}

static int comi_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	log_syscall("Comi_read %s size: %d\n", path);
	int fd;
	if(fi == NULL) {
		fd = proxy_open(path, NULL);
	} else {
		fd = fi->fh;
	}
	
	if (fd == -1) {
		return -errno;
	}

	int res = pread(fd, buf, size, offset);
	if (res == -1) {
		res = -errno;
	}

	if(fi == NULL) {
		close(fd);
	}
		
	return res;
}

static int comi_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	log_syscall("Comi_write", path);
	log_syscall("TEXT", buf);
	if(path == NULL || path[0] == 0) {
		printf("If you want to modify a file path cannot be empty\n");
		return -1;
	}
	
	int fd;
	(void) fi;
	char fpath[PATH_MAX] ;
	int res = get_real_path(fpath, path);
	if (res == -1) {
		return res;
	}

	char tmp_path[PATH_MAX];
	get_fullpath(tmp_path, "/comiData/tmp");
	cp_file(fpath, tmp_path);
	
	fd = open(tmp_path, O_WRONLY);
	
	res = pwrite(fd, buf, size, offset);
	if (res == -1) {
		res = -errno;
	}
	close(fd);

	char hash[HASH_LENGTH + 1];
	get_file_hash(tmp_path, hash);

	char dst_path[PATH_MAX];
	divide(dst_path, hash);

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

	mv_file(tmp_path, dst_path);
	char real_path[PATH_MAX];
	get_fullpath(real_path, path);
	fd = open(real_path, O_WRONLY);
	pwrite(fd,hash,HASH_LENGTH,0);

	close(fd);
	return res;
}

static int comi_statfs(const char *path, struct statvfs *stbuf)
{
    log_syscall("Comi_statfs", path);
	char fpath[PATH_MAX];
	get_fullpath(fpath, path);
	int res = statvfs(fpath, stbuf);
	if (res == -1) {
		return -errno;
	}

	return 0;
}

static int comi_release(const char *path, struct fuse_file_info *fi)
{
    log_syscall("Comi_release", path);
	(void) path;
	close(fi->fh);
	return 0;
}

static int comi_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
    log_syscall("Comi_fsync", path);
	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_POSIX_FALLOCATE
static int comi_fallocate(const char *path, int mode,
			off_t offset, off_t length, struct fuse_file_info *fi)
{
    log_syscall("Comi_fallocate", path);
	int fd;

	(void) fi;

	if (mode)
		return -EOPNOTSUPP;
	char fpath[PATH_MAX];
	get_fullpath(fpath, path);
	fd = open(fpath, O_WRONLY);
	if (fd == -1) {
		return -errno;
	}

	int res = -posix_fallocate(fd, offset, length);

	close(fd);
	return res;
}
#endif


#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int comi_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
    log_syscall("Comi_setxattr", path);
	char fpath[PATH_MAX];
	get_fullpath(fpath, path);
	int res = lsetxattr(fpath, name, value, size, flags);
	if (res == -1) {
		return -errno;
	}
	return 0;
}

static int comi_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
    log_syscall("Comi_getxattr", path);
	char fpath[PATH_MAX];
	get_fullpath(fpath, path);
	int res = lgetxattr(fpath, name, value, size);
	if (res == -1) {
		return -errno;
	}
	return res;
}

static int comi_listxattr(const char *path, char *list, size_t size)
{
    log_syscall("Comi_listxattr", path);
	char fpath[PATH_MAX];
	get_fullpath(fpath, path);
	int res = llistxattr(fpath, list, size);
	if (res == -1) {
		return -errno;
	}
	return res;
}

static int comi_removexattr(const char *path, const char *name)
{
    log_syscall("Comi_removexattr", path);
	char fpath[PATH_MAX];
	get_fullpath(fpath, path);
	int res = lremovexattr(fpath, name);
	if (res == -1) {
		return -errno;
	}
	return 0;
}
#endif

static int comi_create(const char *path, mode_t mode,
              struct fuse_file_info *fi)
{
	log_syscall("Comi_create", path);
	int res;
	char fpath[PATH_MAX];
	get_fullpath(fpath, path);

	res = open(fpath, fi->flags, mode);

	char hash[HASH_LENGTH + 1];
	get_file_hash(fpath, hash);
	pwrite(res, hash, HASH_LENGTH, 0);
	
	create_folder_tree(hash);
	res = open_file_by_hash(hash);

	if (res == -1)
			return -errno;

	fi->fh = res;
	return 0;
}

static struct fuse_operations comi_oper = {
	.init		= comi_init,
	.getattr	= comi_getattr,
	.access		= comi_access,
	.readlink	= comi_readlink,
	.readdir	= comi_readdir,
	.mknod		= comi_mknod,
	.mkdir		= comi_mkdir,
	.symlink	= comi_symlink,
	.unlink		= comi_unlink,
	.rmdir		= comi_rmdir,
	.rename		= comi_rename,
	.link		= comi_link,
	.chmod		= comi_chmod,
	.chown		= comi_chown,
	.truncate	= comi_truncate,
#ifdef HAVE_UTIMENSAT
	.utimens	= comi_utimens,
#endif
	.open		= comi_open,
	.read		= comi_read,
	.write		= comi_write,
	.statfs		= comi_statfs,
	.release	= comi_release,
	.fsync		= comi_fsync,
	.create 	= comi_create,
#ifdef HAVE_POSIX_FALLOCATE
	.fallocate	= comi_fallocate,
#endif
#ifdef HAVE_SETXATTR
	.setxattr	= comi_setxattr,
	.getxattr	= comi_getxattr,
	.listxattr	= comi_listxattr,
	.removexattr	= comi_removexattr,
#endif
};


int main(int argc, char *argv[]) 
{
	int fuse_stat;
	struct comi_state *comi_context;

	comi_context = malloc(sizeof(struct comi_state));

	comi_context->logfile = log_open();
	comi_context->rootdir = realpath(argv[argc-2], NULL);
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
	
  	fuse_stat = fuse_main(argc, argv, &comi_oper, comi_context);

	return fuse_stat;
}
