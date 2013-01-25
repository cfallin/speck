/*
 * include/kernel/msg.h
 */

#ifndef _INCLUDE_KERNEL_MSG_H_
#define _INCLUDE_KERNEL_MSG_H_

#include <kernel/process.h>

int msg_send(pid_t proc, char *sndbuf, int sndlen, char *recbuf, int *reclen);
int msg_recv(pid_t *sender, char *buf, int *buflen);
int msg_reply(pid_t proc, char *buf, int len);

#define MSG_TYPE_IPC 1
#define MSG_TYPE_IRQ 2

#endif
