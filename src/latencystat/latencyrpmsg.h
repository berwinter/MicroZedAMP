#ifndef LATENCYRPMSG_H
#define LATENCYRPMSG_H

#include "latencydemo.h"

struct rpmsg_target {
	int fd;
	int command_no;
};

#define REMOTEPROC_REQUEST_ACK_MASK			0x80000000

int rpmsg_open_device(struct rpmsg_target* target, char* dev);
int rpmsg_close_device(struct rpmsg_target* target);

int rpmsg_send_message(struct rpmsg_target* target, latency_demo_msg_type command);
int rpmsg_read_response(struct rpmsg_target* target, char* data, size_t len);

#endif /* LATENCYRPMSG_H */