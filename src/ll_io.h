#include "listnodes.h"
#include <errno.h>
#include "getline.h"

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
