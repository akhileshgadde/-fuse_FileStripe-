/* Code skeleton credits: Joseph J. Pfeiffer, Emeritus Professor, New Mexico State University */

// gcc -ggdb -Wall -o tfs tfs.c `pkg-config fuse --cflags --libs`

#include "params.h"
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "uthash.h"
#include "ll_io.h"
//#include "strtoll.h"

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#define TFS_PRIV_DATA ((my_tfs_data *) fuse_get_context()->private_data)
#define MAX_SIZE        2*1024*1024


typedef struct file_entries_t
{
    char f_name[PATH_MAX]; /*key*/
    mode_t f_mode;
    ListNode *head;
    //int file_flag; /* 1 for directories we create and 0 for normal directories */
    UT_hash_handle hh; /* makes this structure hashable */
} file_entries;

typedef struct MY_TFS_DATA
{
    char *rootdir;
    file_entries *head;
    //int init_flag;
    char *init_file;
} my_tfs_data;


// Give -errno to caller
static int tfs_error(char *str)
{
    //printf("In Error function\n");
    printf("Error: %s\n", strerror(errno));
    int ret = -errno;
    return ret;
}

static void tfs_fullpath(char fpath[PATH_MAX], const char *path)
{
    //printf("In fullpath function\n");
    strcpy(fpath, TFS_PRIV_DATA->rootdir);
    strncat(fpath, path, PATH_MAX); 
    //printf("TFS_FULLPATH(): %s\n", fpath);
}

/* add the passed values to the HASHMAP */
file_entries *addtoHashmap(mode_t mode, char *f_name, file_entries **head)
{
    file_entries *add = (file_entries *) malloc (sizeof (file_entries));
    if (add == NULL) {
        printf("Malloc error\n");
        return NULL;
    }
    add->f_mode = mode;
    //add->file_flag = file_flag;
    strcpy(add->f_name, f_name);
    HASH_ADD_STR(*head, f_name, add);
    return add;
}   

/* Free the hash-map before exiting the program */
void delfromHashmap(file_entries *del)
{
    if (TFS_PRIV_DATA->head != NULL)
    {
        delAllFromList(&del->head);
        HASH_DEL(TFS_PRIV_DATA->head, del);
        free (del);
    }
}

/* Read line by line and store it in the respective fields */
void readFile(char *file, file_entries **head)
{
    printf("inside readFile, init_file: %s\n", file);
    file_entries *add;
    char f_name[PATH_MAX];
    mode_t mode;
    //char parts_buf[4096];
    char *token;
    //char tmp_buf[5096];
    int i;
    ssize_t read;
    int tot_parts;
    int part_no;
    offset_t st_off_t, end_off_t;
    size_t len = 0;
    char *line = NULL;
    FILE *fd = fopen(file, "r");
    if (fd == NULL) {
        printf("File open error\n");
        return;
    }
    while ((read = getline(&line, &len, fd)) != -1) {
        token = strtok(line, ":");
        printf("Token: %s\n", token);
        sscanf(token, "%07o\t%s\t%05d",&mode, f_name, &tot_parts);
        //token = strtok(NULL, ";");
        //printf("Initial parts_buf: %50s\n", parts_buf);
        //strcpy(tmp_buf, parts_buf);
        add = addtoHashmap(mode, f_name, head);
        //printf("reading from file: tot_parts: %d\n", tot_parts);
        for(i = 0; i < tot_parts; i++)
        {
            //if (i == 0)
            token = strtok(NULL, ";");
            printf("Token after 1st read: %s\n", token);
            sscanf(token, "%05d %20jd %20jd", &part_no, &st_off_t, &end_off_t);
            printf("part# %05d, st_off_t: %jd, end_off_t: %jd\n", part_no, st_off_t, end_off_t);
            addtoList(&(add->head),part_no, st_off_t, end_off_t);
        }
    }
    
    if (line)
        free(line);
    fclose(fd);
}

/* write the present added hashmap entry into a file */
void writetoFile(file_entries *add, char *file, char *mode)
{
    FILE *fd = fopen(file, mode);
    ListNode *temp = add->head;
    int tot_parts;
    if (fd == NULL) {
        printf("File open error\n");
        return;
    }
    //printf("Write: mode - %s\n", mode);
    fprintf(fd, "%07o\t%s\t",add->f_mode, add->f_name);
    tot_parts = findPartNumber(&temp);
    fprintf(fd, "%05d:", tot_parts);
    if (temp != NULL) {
        while (temp->next != NULL)
        {
            fprintf(fd, "%05d %20jd %20jd;",temp->part_no, temp->st_off_t, temp->end_off_t); 
            temp = temp->next;
        }
        fprintf(fd, "%05d %20jd %20jd;\n",temp->part_no, temp->st_off_t, temp->end_off_t);
    }
    fclose(fd);
}

int tfs_getattr(const char *path, struct stat *statbuf)
{
    printf("In getattr function\n");
    int retstat = 0;
    char fpath[PATH_MAX];
    char tmppath[PATH_MAX];
    file_entries *find;
    #if 0
    /* check the init_flag and populate the HASH_MAP from the file */
    if (TFS_PRIV_DATA->init_flag == 0) {
        readFile(TFS_PRIV_DATA->init_file);
        TFS_PRIV_DATA->init_flag = 1;
    }
    #endif
    
    tfs_fullpath(fpath, path);
    strcpy(tmppath, fpath);
    /*check if root directory or autorun.inf file */
    if ((!strcmp(path, "/")) || (!strcmp(path, "/autorun.inf")))
    {
        printf("Fpath2: %s\n", fpath);
        retstat = lstat(fpath, statbuf);
    }
    else { /* All other files */
    //printf("HASHMAP, getattr(): checking for %s\n", tmppath);
    HASH_FIND_STR(TFS_PRIV_DATA->head, tmppath, find);
    if (find != NULL)
    {
        /*handling file write*/
        printf("Found %s in hash_map\n", tmppath);
        strcat(tmppath, "_dir");
        retstat = lstat(tmppath, statbuf);
        if ((retstat == 0) && (S_ISREG(find->f_mode))) { //1 = regular file
            #if 0
            statbuf->st_mode &= ~S_IFDIR;
                        statbuf->st_mode |= S_IFREG; /* making dir look like a regular file */
            #endif  
            statbuf->st_mode = find->f_mode;
            if (find->head != NULL)
                statbuf->st_size = findOffset(&find->head);
        }
    }
        else {
        printf("HASH: find is NULL\n");
            //printf("Fpath4: %s\n", fpath);
            retstat = lstat(fpath, statbuf);
       }
    }

    if (retstat != 0)
        retstat = tfs_error("tfs_getattr lstat");
    return retstat;
}

/* Check file access permissions */
int tfs_access(const char *path, int mask)
{
   int retstat = 0;
   file_entries *find;
   char fpath[PATH_MAX];
   tfs_fullpath(fpath, path);
   #if 0
   if (!(!(strcmp(path, "/")) || !(strcmp(path, "/autorun.inf"))))
   {
   strcat(fpath, "_dir");
   }
   #endif
   HASH_FIND_STR(TFS_PRIV_DATA->head, fpath, find);
   if (find != NULL)
        strcat(fpath, "_dir");
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
    //printf("In chown function\n");
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
    #if 0
    char fpath[PATH_MAX];
    tfs_fullpath(fpath, path);
    if (strcmp(path, "/file1"))
    {
        retstat = truncate(fpath, newsize);
    }
    if (retstat < 0 )
        retstat = tfs_error("tfs_truncate truncate");
    #endif
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
    char fpath[PATH_MAX];
    FILE *fp;
    mode_t fmode;
    //printf("TFS_OPEN: Flags: 0x%08x\n", fi->flags);
    Fuse_ll_info *f_ll_info;
    f_ll_info = (Fuse_ll_info *) malloc(sizeof(Fuse_ll_info));
    if (!f_ll_info) {
        retstat = -ENOMEM;
        goto out;
    }
    tfs_fullpath(fpath, path);
    strcat(fpath, "_dir/.hashmap");
    printf("TFS_OPEN: fpath: %s\n", fpath);
    fp = fopen(fpath, "a+");/* using the flags in struct fuse_file_info since read would get read flags and write would get write flags and same logic applies for .hashmap also  - may fail in file append case?? */
    if (!fp) {
        retstat = tfs_error("TFS_OPEN open");
        goto out;
    }
    f_ll_info->fp = fp;
    printf("fp: %p, f_ll_info->fp: %p\n", fp, f_ll_info->fp);
    
    f_ll_info->head = NULL;
    
    //printf("TFS_OPEN: f_ll_info: %p, head: %p\n", f_ll_info, f_ll_info->head);
    retstat = readHashmapFile(f_ll_info->fp, &f_ll_info->head, &fmode);
    fclose (f_ll_info->fp);
    fp = fopen(fpath, "w+");
    if (!fp) {
        retstat = tfs_error("TFS_OPEN open");
        goto out;
    }
    f_ll_info->fp = fp;
    printf("open: Mode after reading: %07o\n", f_ll_info->fmode);
    f_ll_info->fmode = fmode;

    printf("TFS_OPEN: After reading from .hashmap file\n");
    printf("TFS_OPEN: f_ll_info: %p, head: %p\n", f_ll_info, f_ll_info->head);
    printList(&f_ll_info->head);
    fi->fh = (long) f_ll_info;
out:
    if (retstat < 0) {
        printf("tfs_open: retstat < 0\n");
        if (f_ll_info)
            free(f_ll_info);
    }
    return retstat;
}

int tfs_my_read(const char *fpath, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int fd;
    int retstat = 0;
    fd = open(fpath, fi->flags);
    if (fd < 0) {
        retstat = -EBADF;
        return retstat;
    }
    retstat = pread(fd, buf, size, offset);
    if (retstat < 0)
        retstat = tfs_error("tfs_read read");
    //else
    //  printf("l_buf read: %s, size: %zu\n",buf, size); 
    close(fd);
    return retstat;
}

int tfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("In Read function\n");
    int retstat = 0;
    size_t tot_read = 0;
    //char l_buf[MAX_SIZE];
    file_entries *find;
    Fuse_ll_info *f_ll_info = NULL;
    //struct stat statbuf;
    int part_no;
    ListNode *temp;
    //int fd;
    char fpath[PATH_MAX];
    char orig_path[PATH_MAX];
    tfs_fullpath(fpath, path);
    HASH_FIND_STR(TFS_PRIV_DATA->head, fpath, find);
    if (find != NULL)
    {
        #if 0
        /* checking if file inside the directory exists. If not present, just return 0 and don't print anything */
        if ((retstat = lstat(fpath, &statbuf)) != 0) {
            retstat = 0;
            goto out;
        }
        #endif
        printf("TFS_READ: offset: %jd, size: %zu\n", offset, size);
        strcat(fpath, "_dir");
        strcat(fpath, path);
        strcat(fpath, ".");
        strcpy(orig_path, fpath);
        //temp = find->head;
        f_ll_info = (Fuse_ll_info *) ((long) fi->fh);
        temp = f_ll_info->head;
        //buf[0] = '\0';
        //size = 0;
        while (temp != NULL) /* find starting offset */
        {
            if (offset == temp->st_off_t){
                printf("Found st offset in part: %d\n", temp->part_no);
                break;
            }
            temp = temp->next;
        }
        //l_size = size;
        while ((temp != NULL) && (tot_read < size))
        {
            part_no = temp->part_no;
            sprintf(fpath, "%s%d", orig_path, part_no);
            printf("TFS_READ: endoff: %jd, st_off: %jd\n", temp->end_off_t, temp->st_off_t);
            //l_size = temp->end_off_t - temp->st_off_t;
            retstat = tfs_my_read(fpath, buf+tot_read, size, 0, fi);/* To be changed to correct offset. offset = 0, bcoz reading entire file */
            if (retstat < 0) {
                retstat = tfs_error("tfs_read read");
                goto out;
            }
            tot_read += retstat;
            printf("TFS_READ: fpath after sprintf: %s, byes read: %d\n", fpath, retstat);
            //printf("l_buf tfs_read: %s, size: %zu\n",l_buf, l_size);
            //strncat(buf, l_buf, size);
            //memcpy(buf+size, l_buf, l_size);
            //printf("Buf after memcpy: %s\n", buf);
            //size += l_size;
            printf("total bytes read: %zu\n", tot_read);
            temp = temp->next; 
        }
        //strcat(buf, "\0");
    }
    #if 0
    printf("TFS_READ: Before file open, flags: 0x%08x\n", fi->flags);
    fd = open(fpath, fi->flags);
 
    printf("TFS_READ: fpath before pread: %s\n", fpath);
    //printf("TFS_READ: fd before pread: %d\n", fd);
    retstat = pread(fd, buf, size, offset);
    #endif
    //printf("TFS_READ: size after setting: %zu\n", size);
out:
    if (retstat < 0)
        return retstat;
    return tot_read;
}

int tfs_write(const char *path, const char *buf, size_t size, off_t offset,
             struct fuse_file_info *fi)
{
    printf("In write function\n");
    int retstat = 0;
    char fpath[PATH_MAX];
    char tmp_path[PATH_MAX];
    char tmp_str[10];
    int part_no;
    Fuse_ll_info *f_ll_info;
    offset_t st_off_t, end_off_t;
    tfs_fullpath(fpath, path);
    strcpy(tmp_path, fpath);
    file_entries *find; //*file_find;
    //mode_t mode;
    int fd;
    //int file_found_flag = 1;
    /* This needs to be changed. The magic hashmap is going to store the default
     * mode (of the directory). keep the original mode.
     */
    HASH_FIND_STR(TFS_PRIV_DATA->head, tmp_path, find);
    if (find != NULL) /*found entry in hash map */
    {
        //mode = find->f_mode;
        //printf("HASHMAP, write(): Found entry and setting mode\n");
        strcat(fpath, "_dir");
        strcat(fpath, path);
        
        /* New logic - retrieveing from fi->fh */
        f_ll_info = (Fuse_ll_info *) ((long) fi->fh);
        printf("TFS_OPEN: f_ll_info: %p, head: %p\n", f_ll_info, f_ll_info->head);        //printf("Fd: %d\n", f_ll_info->fd);
        
        /*finding the correct part# from the linked list */
        part_no = findPartNumber(&f_ll_info->head);
        sprintf(tmp_str, "%c%d", '.', part_no);
        strcat(fpath, tmp_str);
        /* finding offset */
        if (part_no == 0)
            st_off_t = 0;
        else
            st_off_t = findOffset(&f_ll_info->head);
        end_off_t = st_off_t + size;
    } //else
       // mode  = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | ~S_IXUSR | ~S_IXGRP | ~S_IXOTH; 
    printf("WRITE(): path before creat: %s\n", fpath);
    fd = creat(fpath, f_ll_info->fmode);
    if (fd < 0)
        return retstat = tfs_error("tfs_write open");
    printf("TFS_WRITE: writing buf: %s at offset: %jd\n", buf, offset);
    retstat = pwrite(fd, buf, size, 0);
    if (retstat < 0)
        retstat = tfs_error("tfs_write pwrite");
    else if (fd > 0)
    {
        /* add the new part to the linked list */
        addtoList(&find->head, part_no, st_off_t, end_off_t);
        /* New list with .hashmap */
        addtoList(&f_ll_info->head, part_no, st_off_t, end_off_t);
        printList(&f_ll_info->head);
//        printList(&(((Fuse_ll_info *) ((long) fi->fh))->head));
        
        printf("Added to LL & f_ll_info: %p, head: %p\n", f_ll_info, f_ll_info->head);
        truncate(fpath, size); //Change the size of the file.
        close(fd);
    }
//endReturn:
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
    //int i;
    struct dirent *de;
    file_entries *find;
    char tmp_path[PATH_MAX];
    char tmp_name[PATH_MAX];
    char fpath[PATH_MAX], *path_str;

    dp = (DIR *) (uintptr_t) fi->fh;

    de = readdir(dp);
    if (de == 0) {
        retstat = tfs_error("tfs_readdir readdir");
        return retstat;
    }
    printf("\nREADDIR: Path %s\n\n", path);
    //filler(buf, de->d_name, NULL, 0);
    do {/*if should check for each entry in hash map*/
    strcpy(tmp_path, "/");
    strcpy(tmp_name, de->d_name);
    if ((path_str = strstr(de->d_name, "_dir")) != NULL) {
            *path_str = '\0';
        }
    printf("de->d_name after ptr op: %s\n", de->d_name);
    strcat(tmp_path, de->d_name);
    tfs_fullpath(fpath, tmp_path);
    printf("Readdir: searching for %s\n", fpath);
    HASH_FIND_STR(TFS_PRIV_DATA->head,fpath, find);
    if (find == NULL) { /* entry found in hash */
        strcpy(de->d_name, tmp_name);
    }
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
    file_entries *add;
    char fpath[PATH_MAX];
    char tmp_path[PATH_MAX];
    tfs_fullpath(fpath, path);
    strcpy(tmp_path, fpath);
    strcat(fpath, "_dir");
    retstat = mkdir(fpath, mode);
    if (retstat < 0)
        retstat = tfs_error("tfs_mkdir mkdir");
    else {
        add = (file_entries *) malloc (sizeof (file_entries));
        if (add == NULL) {
            printf("Malloc error\n");
            retstat = -ENOMEM;
        }
        add->f_mode = mode;
        add->head = NULL;
        //add->file_flag = 0; /* Normal Directory */
        strcpy(add->f_name, tmp_path);
        printf("HASH, mkdir(): Adding directory %s to hash table\n",add->f_name);
        HASH_ADD_STR(TFS_PRIV_DATA->head, f_name, add);
        //writetoFile(add);
    }
    return retstat;
}

// Remove the directory and file inside it
int tfs_unlink(const char *path)
{
    printf("In unlink function\n");
    int retstat = 0;
    file_entries *find;
    ListNode *temp;
    int part_no;
    char fpath[PATH_MAX];
    char dpath[PATH_MAX];
    char orig_path[PATH_MAX];
    tfs_fullpath(fpath, path); //for file
    strcpy(dpath, fpath); //For directory
    printf("Deleting %s file\n", fpath);
    HASH_FIND_STR(TFS_PRIV_DATA->head, fpath, find);
    if (find != NULL)
    {
        printf("Found entry %s in hash_map\n", dpath);
        strcat(fpath, "_dir");
        strcat(dpath, "_dir");
        strcat(fpath, path);
        strcat(fpath, ".");
        strcpy(orig_path, fpath);
        temp = find->head;
        while (temp != NULL)
        {
            part_no = temp->part_no;
            sprintf(fpath, "%s%d", orig_path, part_no);
            printf("UNLINK: fpath: %s\n", fpath);
            retstat = unlink(fpath);
            if (retstat < 0)
            retstat = tfs_error("tfs_unlink unlink");
            temp = temp->next;
        }
        retstat = rmdir(dpath); //remove the directory
        if (retstat < 0)
            retstat = tfs_error("tfs_unlink unlink");
        else
            delfromHashmap(find);
    }
    else {
        retstat = unlink(fpath);
        if (retstat < 0)
            retstat = tfs_error("tfs_unlink unlink");
    }
    return retstat;
}

/* Remove a directory */
int tfs_rmdir(const char *path)
{
    printf("In rmdir function\n");
    int retstat = 0;
    char fpath[PATH_MAX];
    file_entries *find;
    tfs_fullpath(fpath, path);
   /*code to check if _dir needs to be appeneded to fpath or just normal directory deletion */
    HASH_FIND_STR(TFS_PRIV_DATA->head, fpath, find);
    if (find != NULL)
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
    file_entries *find;
    tfs_fullpath(fpath, path);
    HASH_FIND_STR(TFS_PRIV_DATA->head, fpath, find);
    if (find != NULL)
            strcat(fpath, "_dir");
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
    char tmp_path[PATH_MAX];
    char file[PATH_MAX];
    char *temp_str;
    FILE *fp, *root_fp;
    file_entries *add; // *find;
    mode_t d_mode;
    Fuse_ll_info *f_ll_info = NULL;

    tfs_fullpath(fpath, path);
    strcpy(tmp_path, fpath);
    d_mode = mode | S_IXUSR | S_IXGRP | S_IXOTH; /* Add execute permision b/c it's a directory*/
    strcat(fpath, "_dir");
    strcpy(file, path + 1);
    retstat = mkdir(fpath, d_mode);
    if ( lstat(fpath, &statbuf) < 0) //checking if directory is successfully created
    {
        printf("Directory creation error\n");
        retstat = tfs_error("tfs_create create");
    }
    else { /* directory is successfully created. So make an entry in hash table */
        add = (file_entries *) malloc (sizeof (file_entries));
        if (add == NULL) {
            printf("Malloc error\n");
            retstat = -1;
        }
        strcpy(add->f_name, tmp_path);
        add->f_mode = mode;
        add->head = NULL;
        HASH_ADD_STR(TFS_PRIV_DATA->head, f_name, add);
        strcat(tmp_path, "_dir");
        printf("Added fpath %s into hash table\n", add->f_name);
        /* create a .hashmap file and use that for storing linked list info persistently */
        strcat(fpath, "/.hashmap");
        f_ll_info = (Fuse_ll_info *) malloc(sizeof(Fuse_ll_info));
        if (!f_ll_info) {
            retstat = -ENOMEM;
            goto out;
        }
        fp = fopen(fpath, "w+");
        if (!fp) {
            retstat = -EINVAL;
            goto out;
        }
        f_ll_info->fp = fp;
        f_ll_info->head = NULL;
        f_ll_info->fmode = mode;
        /* adding entry to parent dir .hashmap file */
        if ((temp_str = strstr(fpath, path)) != NULL) {
            *temp_str = '\0';
            strcat(fpath, "/.hashmap");
            printf("create: opening hashmap of %s\n", fpath);
        }
        root_fp = fopen(fpath, "w+");
        if (!root_fp) {
            retstat = -EINVAL;
            goto out;
        }
        printf("Writing file: %s, mode: %07o\n", file, mode);
        fprintf(fp, "%s\t%07o\n", file, mode);
        fclose(root_fp);/* may need to be moved to tfs_release () */
        printf("TFS_CREATE: f_ll_info: %p\n", f_ll_info);
        //printf("fd: %d\n", fd);
        fi->fh = (long) f_ll_info;
    }
out:
    if (retstat < 0) {
        printf("TFS_CREATE: error, retstat < 0\n");
        fclose(f_ll_info->fp);
        if (f_ll_info)
            free(f_ll_info);
        if (lstat(tmp_path, &statbuf) == 0) /* directory was created, so remove that in error case */
            rmdir(tmp_path);
        /* removing from hash-map */
        HASH_DEL(TFS_PRIV_DATA->head, add);
        retstat = tfs_error("tfs_create creat");
    }
    return retstat;
}

//Similar to close()
int tfs_release(const char *path, struct fuse_file_info *fi)
{
    printf("In release function\n");
    int retstat = 0;
    Fuse_ll_info *f_ll_info = NULL;
    f_ll_info = (Fuse_ll_info *) ((long) fi->fh);
    printf("Release: f_ll_info: %p, f_ll_info->head: %p\n", f_ll_info, f_ll_info->head);
    /* Add code for writing info in Linked List to .hashmap file */
    writeToHashmapFile(f_ll_info->fp, &f_ll_info->head, f_ll_info->fmode);
    retstat = fclose(f_ll_info->fp);
    if (retstat != 0) {
        printf("Release: Fclose failed\n");
    }
    if (f_ll_info->head) {
        printf("TFS_RELEASE: Freeing linked list\n");
        printf("release: f_ll_info->head: %p\n", f_ll_info->head);
        //printList(&f_ll_info->head);
        delAllFromList(&(f_ll_info->head));
        printf("release: f_ll_info->head after deleting LL: %p\n", f_ll_info->head);
    }
    printf("tfs_release: about to free f_ll_info\n");
    if (f_ll_info)
        free(f_ll_info);
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
    my_tfs_data *tfs_priv_data;
    file_entries *iterator;
    mode_t hmode;
    struct stat statbuf;
    char hpath[PATH_MAX];
    int fd;
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
        tfs_usage();

    tfs_priv_data = (my_tfs_data *) malloc(sizeof(my_tfs_data));
    if (tfs_priv_data == NULL) {
        perror("main calloc: tfs_priv_data");
        abort();
    }
    printf("before realpath: %s\n", argv[argc-2]);
     
    tfs_priv_data->rootdir = realpath(argv[argc-2], NULL);
    if (!tfs_priv_data->rootdir) {
        printf("Error obtaining the mount point full path\n");
        fuse_stat = -EINVAL;
        goto out;
    }
    tfs_priv_data->head = NULL;
    tfs_priv_data->init_file = (char *) malloc (strlen("init_file.txt") + 1);
    if (tfs_priv_data->init_file == NULL) {
        printf("Malloc() error\n");
        abort();
    }
    strcpy(tfs_priv_data->init_file, "init_file.txt");
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;

    printf("main: tfs_priv_data->rootdir: %s\n", tfs_priv_data->rootdir);
    #if 1
    /* tfs_priv_data->rootdir holds the mount point 
    *  Create .hashmap file if not exists
    */
    strcpy(hpath, tfs_priv_data->rootdir);
    strcat(hpath, "/.hashmap");
    printf("hashmap file: %s\n", hpath);
    if (lstat(hpath, &statbuf) < 0) {
        hmode  = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
        fd = creat(hpath, hmode);
        if (fd < 0) {
            printf(".hashmap file creation failed for rootdir\n");
            fuse_stat = -1;
            goto out;
        }
        close(fd); //closing the .hashmap file 
    }
    #endif
    /* Read from init_file if it exists */
    readFile(tfs_priv_data->init_file, &tfs_priv_data->head);
    // turn over control to fuse
    fprintf(stderr, "about to call fuse_main\n");
    fuse_stat = fuse_main(argc, argv, &tfs_oper, tfs_priv_data);
    /* write to init_file all entries in hashmap before exiting the program */
    iterator = tfs_priv_data->head; 
    while (iterator != NULL)
    {
        //tmp = iterator;
        if (iterator == tfs_priv_data->head)
            writetoFile(iterator, tfs_priv_data->init_file, "wb");
        else
            writetoFile(iterator, tfs_priv_data->init_file, "a");
        iterator = (file_entries *) (iterator->hh.next);
        //delfromHashmap (tmp);
    }
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    free(tfs_priv_data->init_file);
    free(tfs_priv_data);
out:
    return fuse_stat;
}
