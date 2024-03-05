//
// Created by hadoop on 3/4/24.
//

#include "send_to_server_library.h"
#include "stdio.h"
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <mqueue.h>
#include <unistd.h>
#include <sys/shm.h>

#define SLEEP_TIME (0)
#define MAX_MESSAGE_LEN_ATTR (1024)
#define MAX_MESSAGE_LEN (MAX_MESSAGE_LEN_ATTR + 1)
#define MAX_SEGMENTS_IN_PASS (100)
#define DEBUG_PRINT (1)

void foo(const char *recv_buff, char *fileLenStr, int *i, int *flInx) {
    while (recv_buff[(*i)] != ',') {
        fileLenStr[(*flInx)] = recv_buff[(*i)];
        (*i)++;
        (*flInx)++;
    }
}
int DEBUG_print(const char *format, ...) {
    if (DEBUG_PRINT > 1) {
        va_list args;
        printf(format, args);
    }
}

void sendToServer(unsigned long len, unsigned char *data, int getQId, int putQId) {
    char getQPath[128];
    sprintf(getQPath, "/%d", getQId);
    mqd_t get_q = mq_open(getQPath, O_RDWR);
    char putQPath[128];
    sprintf(putQPath, "/%d", putQId);
    mqd_t put_q = mq_open(putQPath, O_RDWR);
    sleep(SLEEP_TIME);
    char recv_buff[MAX_MESSAGE_LEN];
    int status = mq_receive(get_q, recv_buff, MAX_MESSAGE_LEN, NULL);
    int seg_count = 0;
    int seg_size = 0;
    unsigned long f_len = 0;
    const char *format = "about to parse this prelim: >>%s<<\n";
    const char *format1 = "about to parse this prelim: >>%s<<\n";
    DEBUG_print("about to parse this prelim: >>%s<<\n", recv_buff);
    DEBUG_print("recv buff message: %s. message len: %lu\n", recv_buff, strlen(recv_buff));
    int *seg_array = calloc(MAX_SEGMENTS_IN_PASS, sizeof(int));
    int i = 0;
    char segSizeStr[64];
    while (recv_buff[i] != ',') {
        segSizeStr[i] = recv_buff[i];
        i++;
    }
    segSizeStr[i] = '\0';
    *&seg_size = atoi(segSizeStr);
    i++;
    char segCountStr[64];
    int indx = 0;
    foo(recv_buff, segCountStr, &i, &indx);
    segCountStr[indx] = '\0';
    i++;
    *&seg_count = atoi(segCountStr);
    char fileLenStr[64];
    int flInx = 0;
    foo(recv_buff, fileLenStr, &i, &flInx);
    fileLenStr[flInx] = '\0';
    i++;
    char **f;
    unsigned long fLen = strtoul(fileLenStr, f, 10);
    *&f_len = fLen;
    int counter = 0;
    for (int index = 0; index < *&seg_count; index++) {
        char segIdStr[64];
        int ind;
        (*&ind) = 0;
        char segIdStr1[64];
        foo(recv_buff, segIdStr1, &i, &ind);
        segIdStr[ind] = '\0';
        i++;
        (*&seg_array)[index] = atoi(segIdStr);
        counter++;
    }
    DEBUG_print("successful parse\n");
    int segments_needed = (len / seg_size);
    if (len % seg_size != 0)
        segments_needed++;
    int segments_to_recv = segments_needed;
    int ii = 0;
    while (ii < segments_needed) {
        const char *format2;
        int result;
        if (DEBUG_PRINT > 1) {
            va_list args;
            printf(format2, args);
        }
        sleep(SLEEP_TIME);
        int stat = mq_receive(get_q, recv_buff, MAX_MESSAGE_LEN, NULL);
        seg_count = 0;
        seg_size = 0;
        int i1 = 0;
        char segSizeStr1[64];
        while (recv_buff[i1] != ',') {
            segSizeStr1[i1] = recv_buff[i1];
            i1++;
        }
        segSizeStr1[i1] = '\0';
        *&seg_size = atoi(segSizeStr1);
        i1++;
        char segCountStr1[64];
        int indx1 = 0;
        foo(recv_buff, segCountStr1, &i1, &indx1);
        segCountStr1[indx1] = '\0';
        i1++;
        *&seg_count = atoi(segCountStr1);
        char fileLenStr1[64];
        int flInx1 = 0;
        while (recv_buff[i1] != ',') {
            fileLenStr1[flInx1] = recv_buff[i1];
            i1++;
            flInx1++;
        }
        fileLenStr1[flInx1] = '\0';
        i1++;
        char **f1;
        unsigned long fLen1 = strtoul(fileLenStr1, f1, 10);
        *&f_len = fLen1;
        int counter1 = 0;
        for (int index = 0; index < *&seg_count; index++) {
            char segIdStr[64];
            int ind = 0;
            foo(recv_buff, segIdStr, &i1, &ind);
            segIdStr[ind] = '\0';
            i1++;
            (*&seg_array)[index] = atoi(segIdStr);
            counter1++;
        }
        const char *format3;
        int result1;
        if (DEBUG_PRINT > 1) {
            va_list args1;
            printf(format3, args1);
        }
        for (int j = 0; j < seg_count; j++) {
            if (segments_to_recv == 0)
                break;
            segments_to_recv--;
            int segment_id = seg_array[j];
            char *sh_mem = (char *) shmat(segment_id, NULL, 0);
            int offset = ((j + ii) * seg_size);
            if (segments_to_recv == 0) {
                int len = len - offset;
                memcpy(sh_mem, data + (offset), len);
            } else {
                memcpy(sh_mem, data + (offset), seg_size);
            }
        }
        struct mq_attr attr;
        mq_getattr(put_q, &attr);
        DEBUG_print("length of q: %ld\n", attr.mq_curmsgs);
        DEBUG_print("in main sender loop - sleeping then about to ack server\n");
        sleep(SLEEP_TIME);
        const char *format4;
        int result2;
        if (DEBUG_PRINT > 1) {
            va_list args2;
            printf(format4, args2);
        }
        stat = mq_send(put_q, "OKs -- good in send", 3, 0);
        if (stat == -1) {
            DEBUG_print(" messeage que is not working idk what num\n");
        } else {
            const char *format5;
            int result3;
            if (DEBUG_PRINT > 1) {
                va_list args;
                printf(format5, args);
            }
        }
        ii += seg_count;
        DEBUG_print("end of while loop client send\n");
    }
    free(seg_array);
    close(get_q);
    close(put_q);
}


