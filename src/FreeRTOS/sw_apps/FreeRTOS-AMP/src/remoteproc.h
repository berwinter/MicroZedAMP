/* Copyright (C) 2012 Xilinx Inc. */

#ifndef REMOTEPROC_H
#define REMOTEPROC_H

/* TTC1 base address, mapped by the remoteproc resource initialization */
#ifndef TTC_BASEADDR
#define TTC_BASEADDR 0XF8002000
#endif

/* Resource table setup */
void mmu_resource_table_setup(void);

/* Structure of a message passed between the application and the remoteproc
 * handler */
struct remoteproc_request {
	struct rpmsg_hdr* __hdr;
	unsigned int state;
};

/* Mask for the state field which is stored as the first word in each message */
#define REMOTEPROC_REQUEST_ACK_MASK			0x80000000

typedef void (remoteproc_rx_callback)(struct remoteproc_request* req,
		unsigned char* data, unsigned int len);

/* trace buffer init function */
void trace_init(void);

/* Remoteproc init functions */
void remoteproc_init(remoteproc_rx_callback* handler);
void remoteproc_init_irqs(void);

/* Message response functions */
void remoteproc_request_ack(struct remoteproc_request* req);
void remoteproc_request_response(struct remoteproc_request* req,
		unsigned char* data, unsigned int len);

#endif /* REMOTEPROC_H */
