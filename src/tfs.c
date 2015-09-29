/* Code skeleton credits: Joseph J. Pfeiffer, Emeritus Professor, New Mexico State University */

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

// Give -errno to caller
static int tfs_error(char *str)
{
    printf("In Error function\n");
    printf("Error: %s\n", strerror(errno));
    int ret = -errno;
    return ret;
}

static void tfs_fullpath(char fpath[PATH_MAX], const char *path)
{
    printf("In fullpath function\n");
    strcpy(fpath, TFS_DATA->rootdir);
    strncat(fpath, path, PATH_MAX); 
    //printf("tfs_fullpath:%s\n", TFS->rootdir);
}

int tfs_getattr(const char *path, struct stat *statbuf)
{
    printf("In getattr function\n");
    int retstat = 0;
    char fpath[PATH_MAX];
    char tmppath[PATH_MAX];

    tfs_fullpath(fpath, path);
    tfs_fullpath(tmppath, path);
    /*check if root directory or autorun.inf file */
    if ((!strcmp(path, "/")) || (!strcmp(path, "/autorun.inf")))
    {
        printf("Fpath2: %s\n", fpath);
        retstat = lstat(fpath, statbuf);
    }
    else { /* All other files */
    	if (!strcmp(path, "/file1"))//Needs to be replaced by each file name entry from the Data structure
    	{
		strcat(tmppath, "_dir");
		//strcat(tmppath, path);		
		/*handling file write*/
		retstat = lstat(tmppath, statbuf);
		if (retstat == 0)
			statbuf->st_mode = S_IFREG; /* making dir look like a regular file */
    	}
        else {
    		 if (strstr(fpath, "_dir") != NULL) 
		 {
        		printf("Fpath3: %s\n", fpath);
        		retstat = lstat(fpath, statbuf);
        		goto ret;
    		 }
		else
	        {

			strcat(fpath, "_dir");
        		printf("Fpath4: %s\n", fpath);
        		retstat = lstat(fpath, statbuf);
		}
        }  
   }

   #if 0
    if (strstr(fpath, "_dir") != NULL) 
    {
    	printf("Fpath1: %s\n", fpath);
	retstat = lstat(fpath, statbuf);
	goto ret;
    }
    #endif
    ret:
    if (retstat != 0)
        retstat = tfs_error("tfs_getattr lstat");
    #if 0
    if (retstat < 0)
	retstat = 0;
    #endif
    return retstat;
}

/* Check file access permissions */
int tfs_access(const char *path, int mask)
{
   int retstat = 0;
   char fpath[PATH_MAX];
   tfs_fullpath(fpath, path);
   if (!(!(strcmp(path, "/")) || !(strcmp(path, "/autorun.inf"))))
   {
   strcat(fpath, "_dir");
   }
   retstat = access(fpath, mask);
   return retstat;
}

/*Change the permission bits of a file */
int tfs_chmod(const char *path, mode_t mode)
{
    printf("In chmod function\n");
    int retstat = 0;
    char fpath[PATH_MAX];
    tfs_fullpath(fpath, path);
    retstat = chmod(fpath, mode);
    if (retstat < 0)
	retstat = tfs_error("tfs_chmod chmod");
    return retstat;
}

/* Change the owner and group of a file */
int tfs_chown(const char *path, uid_t uid, gid_t gid)
{
    printf("In chown function\n");
    int retstat = 0;
    char fpath[PATH_MAX];
    tfs_fullpath(fpath, path);
    retstat = chown(fpath, uid, gid);
    if (retstat < 0)
	retstat = tfs_error("tfs_chown chown");
    return retstat;
}

/* change the size of a file */
int tfs_truncate(const char *path, off_t newsize)
{
    printf("In truncate function\n");
    int retstat = 0;
    char fpath[PATH_MAX];
    tfs_fullpath(fpath, path);
    if (strcmp(path, "/file1"))
    {
    	retstat = truncate(fpath, newsize);
    }
   if (retstat < 0 )
		retstat = tfs_error("tfs_truncate truncate");
    return retstat;
}

/*Change the access and/or modification times of a file */
int tfs_utime(const char *path, struct utimbuf *ubuf)
{
    printf("In utime function\n");
    int retstat = 0;
    char fpath[PATH_MAX];
    tfs_fullpath(fpath, path);
    strcat(fpath, "_dir");
    retstat = utime(fpath, ubuf);
    if (retstat < 0)
	retstat = tfs_error("tfs_utime utime");
    return retstat;
}

int tfs_open(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    int fd = 0;
    char fpath[PATH_MAX];
   printf("In Open function\n"); 
   tfs_fullpath(fpath, path);
   //fd = open(fpath, fi->flags);
   if (!strcmp(path, "/file1"))
   {
	retstat = 0;
   }
   else {
   	if (fd < 0)
        	retstat = tfs_error("tfs_open open");
   	else
        	fi->fh = fd;
   }
   return retstat;
}

int tfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("In Read function\n");
    int retstat = 0;
    int fd;
    char fpath[PATH_MAX];
    tfs_fullpath(fpath, path);
    if (strstr(fpath, "_dir") == NULL)
    {
	strcat(fpath, "_dir");
        strcat(fpath, path);
    }
    fd = open(fpath, fi->flags);
    retstat = pread(fd, buf, size, offset);
    if (retstat < 0)
        retstat = tfs_error("tfs_read read");
    if (fd > 0)
	close(fd);
    return retstat;
}

int tfs_write(const char *path, const char *buf, size_t size, off_t offset,
             struct fuse_file_info *fi)
{
    printf("In write function\n");
    int retstat = 0;
    char fpath[PATH_MAX];
    tfs_fullpath(fpath, path);
    strcat(fpath, "_dir");
    strcat(fpath, path);
    int fd;
    fd = creat(fpath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (fd < 0)
	return retstat = tfs_error("tfs_write open");
    else
    	fi->fh = fd;
    retstat = pwrite(fi->fh, buf, size, offset);
    if (retstat < 0)
        retstat = tfs_error("tfs_write pwrite");
    else if (fd > 0)
    {
	truncate(fpath, size); //Change the size of the file.
	close(fd);
    }
    return retstat; 
}

int tfs_statfs(const char *path, struct statvfs *statv)
{
    printf("In statfs function\n");
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
    printf("In Readdir function\n");
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
    printf("In readlink function\n");
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
    printf("In mkdir function\n");
    int retstat = 0;
    char fpath[PATH_MAX];
    tfs_fullpath(fpath, path);
    strcat(fpath, "_dir");
    retstat = mkdir(fpath, mode);
    if (retstat < 0)
        retstat = tfs_error("tfs_mkdir mkdir");
    return retstat;
}

//Remove the directory and file inside it
int tfs_unlink(const char *path)
{
    printf("In unlink function\n");
    int retstat = 0;
    char fpath[PATH_MAX];
    char dpath[PATH_MAX];
    tfs_fullpath(fpath, path); //for file
    tfs_fullpath(dpath, path); //For directory
    if (!strcmp(path, "/file1"))
    {
	strcat(fpath, "_dir");
 	strcat(dpath, "_dir");
	strcat(fpath, path);
    }

    retstat = unlink(fpath);
    if (retstat < 0)
        retstat = tfs_error("tfs_unlink unlink");
    retstat = rmdir(dpath); //remove the directory
    if (retstat < 0)
        retstat = tfs_error("tfs_unlink unlink");
    return retstat;
}

/** Remove a directory */
int tfs_rmdir(const char *path)
{
    printf("In rmdir function\n");
    int retstat = 0;
    char fpath[PATH_MAX];

    tfs_fullpath(fpath, path);
   /*code to check if _dir needs to be appeneded to fpath or just normal directory deletion */
    strcat(fpath, "_dir");
    retstat = rmdir(fpath);
    if (retstat < 0)
        retstat = tfs_error("tfs_rmdir rmdir");

    return retstat;
}

int tfs_rename(const char *path, const char *newpath)
{
    printf("In rename function\n");
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
    printf("In Opendir function\n");
    DIR *dp;
    int retstat = 0;
    char fpath[PATH_MAX];

    tfs_fullpath(fpath, path);
    if(strcmp(path, "/"))
    { 
      	if (strstr(fpath, "_dir") == NULL)
		strcat(fpath, "_dir");
    }
    dp = opendir(fpath);
    if (dp == NULL)
        retstat = tfs_error("tfs_opendir opendir");

    fi->fh = (intptr_t) dp;

    return retstat;
}

int tfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    printf("In create function\n");
    int retstat = 0;
    struct stat statbuf;
    char fpath[PATH_MAX];
    tfs_fullpath(fpath, path);
    //if (strstr(fpath, "_dir") == NULL)
    strcat(fpath, "_dir");
    retstat = mkdir(fpath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if ( lstat(fpath, &statbuf) < 0) //checking if directory is successfully created
    {
	printf("Directory creation error\n");
	retstat = -1;
    }
    if (retstat < 0)
        retstat = tfs_error("tfs_create creat");
    #if 0
    else
    {
	closedir((DIR *) (uintptr_t) fi->fh);
    }
    #endif
    return retstat;
}

//Similar to close()
int tfs_release(const char *path, struct fuse_file_info *fi)
{
    printf("In release function\n");
    int retstat = 0;
    // We need to close the file.  Had we allocated any resources
    // (buffers etc) we'd need to free them here as well.
    //retstat = close(fi->fh);
    return retstat;
}

//simialar to release but for directories
int tfs_releasedir(const char *path, struct fuse_file_info *fi)
{
    printf("In releasedir function\n");
    int retstat = 0;
    closedir((DIR *) (uintptr_t) fi->fh);
    return retstat;
}

/* Create a symbolic link */
int tfs_symlink(const char *path, const char *link)
{
    printf("In symlink function\n");
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
    printf("In link function\n");
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
    printf("In mknod function\n");
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
  .chmod = tfs_chmod,
  .chown = tfs_chown,
  .truncate = tfs_truncate,
  .utime = tfs_utime,
  .access = tfs_access,
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
