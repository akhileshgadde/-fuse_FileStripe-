#include "listnodes.h"
#include <errno.h>
#include "getline.h"
#include <errno.h>

typedef struct fuse_ll_info {
    FILE *fp;
    mode_t fmode;
    ListNode *head;
} Fuse_ll_info;

int readHashmapFile(FILE *fp, ListNode **head, mode_t *fmode)
{
    int ret = 0;
    int mode_flag = 0;
    ssize_t read;
    size_t len = 0;
    char *line = NULL;
    int part_no;
    offset_t st_off_t, end_off_t;

    while ((read =  getline(&line, &len, fp)) != -1) {
        if (!strcmp(line, "\0"))
            goto out;
        if (mode_flag == 0) {
            sscanf(line, "%07o", fmode);
            mode_flag = 1;
            continue;
        }
        sscanf(line, "%05d:%20jd;%20jd", &part_no, &st_off_t, &end_off_t);
        addtoList(head, part_no, st_off_t, end_off_t);
        printf("Adding to LL: %d:%jd;%jd\n", part_no, st_off_t,end_off_t);
    }

out:
    return ret;
}

int writeToHashmapFile(FILE *fp, ListNode **head, mode_t fmode)
{
    int ret = 0;
    ListNode *temp = *head;   

    fprintf(fp,"%07o\n", fmode);
    while (temp != NULL) {
        fprintf(fp, "%5d:%20jd;%20jd\n", temp->part_no, temp->st_off_t, temp->end_off_t);
        temp = temp->next;
    }
    return ret;
}

void writeEntryToRootHashmapFile(FILE *fp, const char *add_path,  mode_t mode)
{
    printf("adding path: %s, mode: %07o to .hashmap\n", add_path, mode);
    fprintf(fp, "%s\t%07o\n", add_path, mode);
}

int writetoRootHashmapFile(char *h_path, const char *add_path,  mode_t mode)
{
    FILE *root_fp = NULL;
    //char *tmp_str;
    int retstat = 0;
    /* adding to parent .hashmap file */
    root_fp = fopen(h_path, "a+");
    if (!root_fp) {
        printf("unable to open parent hashmap\n");
        retstat = -EINVAL;
        goto out;
    }
    writeEntryToRootHashmapFile(root_fp, add_path, mode);
    //printf("adding path: %s, mode: %07o to .hashmap\n", add_path, mode);
    //fprintf(root_fp, "%s\t%07o\n", add_path, mode); 
out:
    if (root_fp)
        fclose(root_fp);

    return retstat;
}

