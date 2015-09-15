/* Code credits: Joseph J. Pfeiffer, Jr., Ph.D, Emeritus Professor, New Mexico State University */

// gcc -ggdb -Wall -o tfs tfs.c `pkg-config fuse --cflags --libs`

#include "params.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#define TFS_FD_DATA ((struct fuse_file_desc *) fuse_get_context()->private_data)

struct fuse_file_desc
{
	int bfd;
}*tfs_fd;

// Report errors to logfile and give -errno to caller
static int tfs_error(char *str)
{
    int ret = -errno;
    return ret;
}

static void tfs_fullpath(char fpath[PATH_MAX], const char *path)
{
    strcpy(fpath, TFS_DATA->rootdir);
    strncat(fpath, path, PATH_MAX); 
    //printf("tfs_fullpath:%s\n", TFS->rootdir);
}

int tfs_getattr(const char *path, struct stat *statbuf)
{
    int retstat = 0;
    char fpath[PATH_MAX];

    tfs_fullpath(fpath, path);

    retstat = lstat(fpath, statbuf);
    if (retstat != 0)
        retstat = tfs_error("tfs_getattr lstat");
    return retstat;
}

int tfs_open(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    int fd,bfd;
    char *bkp;
    char fpath[PATH_MAX];
  
   tfs_fullpath(fpath, path);
   bkp = strstr(fpath,".bkp");
   if (bkp != NULL) //If trying to open the .bkp file
   {
     bfd = open(fpath, fi->flags);
     if (bfd < 0)
        retstat = tfs_error("tfs_open open");
     else
        tfs_fd->bfd = bfd;
     *bkp = '\0';
     fd = open(fpath, fi->flags);
     if (fd < 0)
        retstat = tfs_error("tfs_open open");
     else
        fi->fh = fd;
   }
   else //opening the normal file
   {
     fd = open(fpath, fi->flags);
     if (fd < 0)
        retstat = tfs_error("tfs_open open");
     else
        fi->fh = fd;
     //opening backup file
     strcat(fpath,".bkp");
     bfd = open(fpath, fi->flags);
     if (bfd < 0)
        retstat = tfs_error("tfs_open open");
     else
        tfs_fd->bfd = bfd;
   }
   return retstat;
}

int tfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    char *bkp;
    //int fd;
    bkp = strstr(path,".bkp");//trying to read the backup file
    if (bkp != NULL) {
	retstat = pread(tfs_fd->bfd, buf, size, offset);
    }
    else  //readfing normal file
    	retstat = pread(fi->fh, buf, size, offset);
    if (retstat < 0)
        retstat = tfs_error("tfs_read read");
    return retstat;
}

int tfs_write(const char *path, const char *buf, size_t size, off_t offset,
             struct fuse_file_info *fi)
{
    int retstat = 0;
    //char *buff_local;
    //int  bkp_size = 0;
    struct stat st;
    //int fd;
    char bkp_path[PATH_MAX], *bkp;
    char fpath[PATH_MAX];
    tfs_fullpath(fpath, path);
    strcpy(bkp_path, fpath);
    bkp = strstr(bkp_path, ".bkp");
    if (bkp == NULL)
    	strcat(bkp_path, ".bkp"); 
   stat(bkp_path , &st);
   //if (st.st_size == 0)
   //{	
     // buff_local = strdup(buf);
     // if (buff_local == 0) //out of memory
      //   return -1;
      retstat = write(tfs_fd->bfd, buf, size);
      if (retstat < 0)
          retstat = tfs_error("tfs_write pwrite");
   //}
   retstat = pwrite(fi->fh, buf, size, offset);
   if (retstat < 0)
       retstat = tfs_error("tfs_write pwrite");
   return retstat; 
}

int tfs_statfs(const char *path, struct statvfs *statv)
{
    int retstat = 0;
    char fpath[PATH_MAX];

    tfs_fullpath(fpath, path);

    // get stats for underlying filesystem
    retstat = statvfs(fpath, statv);
    if (retstat < 0)
        retstat = tfs_error("tfs_statfs statvfs");
    return retstat;
}

int tfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    DIR *dp;
    struct dirent *de;

    dp = (DIR *) (uintptr_t) fi->fh;

    de = readdir(dp);
    if (de == 0) {
        retstat = tfs_error("tfs_readdir readdir");
        return retstat;
    }
    //filler(buf, de->d_name, NULL, 0);
    do {
        if (filler(buf, de->d_name, NULL, 0) != 0) {
             printf("Error in Filler\n");
             return -ENOMEM;
        }
    } while ((de = readdir(dp)) != NULL);
    
    return retstat;
}

int tfs_readlink(const char *path, char *link, size_t size)
{
    int retstat = 0;
    char fpath[PATH_MAX];

    tfs_fullpath(fpath, path);

    retstat = readlink(fpath, link, size - 1);
    if (retstat < 0)
        retstat = tfs_error("tfs_readlink readlink");
    else  {
        link[retstat] = '\0';
        retstat = 0;
    }

    return retstat;
}
/** Create a directory */
int tfs_mkdir(const char *path, mode_t mode)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    tfs_fullpath(fpath, path);
    retstat = mkdir(fpath, mode);
    if (retstat < 0)
        retstat = tfs_error("tfs_mkdir mkdir");
    return retstat;
}

//Remove a file
int tfs_unlink(const char *path)
{
    int retstat = 0;
    char fpath[PATH_MAX];

    tfs_fullpath(fpath, path);

    retstat = unlink(fpath);
    if (retstat < 0)
        retstat = tfs_error("tfs_unlink unlink");

    return retstat;
}

/** Remove a directory */
int tfs_rmdir(const char *path)
{
    int retstat = 0;
    char fpath[PATH_MAX];

    tfs_fullpath(fpath, path);

    retstat = rmdir(fpath);
    if (retstat < 0)
        retstat = tfs_error("tfs_rmdir rmdir");

    return retstat;
}

int tfs_rename(const char *path, const char *newpath)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    char fnewpath[PATH_MAX];

    tfs_fullpath(fpath, path);
    tfs_fullpath(fnewpath, newpath);

    retstat = rename(fpath, fnewpath);
    if (retstat < 0)
        retstat = tfs_error("tfs_rename rename");
    return retstat;
}

int tfs_opendir(const char *path, struct fuse_file_info *fi)
{
    DIR *dp;
    int retstat = 0;
    char fpath[PATH_MAX];

    tfs_fullpath(fpath, path);

    dp = opendir(fpath);
    if (dp == NULL)
        retstat = tfs_error("tfs_opendir opendir");

    fi->fh = (intptr_t) dp;

    return retstat;
}

int tfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    int fd,bfd;

    tfs_fullpath(fpath, path);
    fd = creat(fpath, mode);
    if (fd < 0)
        return tfs_error("tfs_create creat");
    fi->fh = fd;
    strcat(fpath, ".bkp");
    bfd = creat(fpath, mode);
    if (bfd < 0)
        return tfs_error("tfs_create creat");
    tfs_fd->bfd = bfd;
    return retstat;
}

//Similar to close()
int tfs_release(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    // We need to close the file.  Had we allocated any resources
    // (buffers etc) we'd need to free them here as well.
    retstat = close(fi->fh);
    if (tfs_fd->bfd != 0)
    	retstat = close(tfs_fd->bfd);
    //retstat = close(fi->bfh);
    return retstat;
}

//simialar to release but for directories
int tfs_releasedir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    closedir((DIR *) (uintptr_t) fi->fh);
    return retstat;
}

/* Create a symbolic link */
int tfs_symlink(const char *path, const char *link)
{
    int retstat = 0;
    char flink[PATH_MAX];
    tfs_fullpath(flink, link);

    retstat = symlink(path, flink);
    if (retstat < 0)
        retstat = tfs_error("tfs_symlink symlink");
    return retstat;
}

/* Create a hard link to a file */
int tfs_link(const char *path, const char *newpath)
{
    int retstat = 0;
    char fpath[PATH_MAX], fnewpath[PATH_MAX];
    tfs_fullpath(fpath, path);
    tfs_fullpath(fnewpath, newpath);

    retstat = link(fpath, fnewpath);
    if (retstat < 0)
        retstat = tfs_error("tfs_link link");

    return retstat;
}

int tfs_mknod(const char *path, mode_t mode, dev_t dev)
{
    int retstat = 0;
    char fpath[PATH_MAX];

    tfs_fullpath(fpath, path);

    // On Linux this could just be 'mknod(path, mode, rdev)' but this
    //  is more portable
    if (S_ISREG(mode)) {
        retstat = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (retstat < 0)
            retstat = tfs_error("tfs_mknod open");
        else {
            retstat = close(retstat);
            if (retstat < 0)
                retstat = tfs_error("tfs_mknod close");
        }
    } else
        if (S_ISFIFO(mode)) {
            retstat = mkfifo(fpath, mode);
            if (retstat < 0)
                retstat = tfs_error("tfs_mknod mkfifo");
        } else {
            retstat = mknod(fpath, mode, dev);
            if (retstat < 0)
                retstat = tfs_error("tfs_mknod mknod");
        }

    return retstat;
}

struct fuse_operations tfs_oper = {
  .getattr = tfs_getattr,
  .open = tfs_open,
  .read = tfs_read,
  .write = tfs_write,
  .create = tfs_create,
  .statfs = tfs_statfs,
  .readdir = tfs_readdir,
  .mkdir = tfs_mkdir,
  .unlink = tfs_unlink,
  .rmdir = tfs_rmdir,
  .rename = tfs_rename,
  .opendir = tfs_opendir,
  .release = tfs_release,
  .releasedir = tfs_releasedir,
  .readlink = tfs_readlink,
  .symlink = tfs_symlink,
  .link = tfs_link,
  //.mknod = tfs_mknod,
};

void tfs_usage()
{
  fprintf(stderr, "usage:  tfs [FUSE and mount options] rootDir mountPoint\n");
  abort();
}

int main(int argc, char *argv[])
{
    int fuse_stat;
    struct tfs_state *tfs_data;
    //struct fuse_file_desc *tfs_fd;
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
        tfs_usage();

    tfs_data = malloc(sizeof(struct tfs_state));
    if (tfs_data == NULL) {
        perror("main calloc: tfs_data");
        abort();
    }
    tfs_fd = malloc(sizeof(struct fuse_file_desc));
    if (tfs_fd == NULL) {
        perror("main calloc: tfs_fd");
        abort();
    }

    tfs_data->rootdir = realpath(argv[argc-2], NULL);
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
    // turn over control to fuse
    fprintf(stderr, "about to call fuse_main\n");
    fuse_stat = fuse_main(argc, argv, &tfs_oper, tfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    free(tfs_data);

    return fuse_stat;
}
