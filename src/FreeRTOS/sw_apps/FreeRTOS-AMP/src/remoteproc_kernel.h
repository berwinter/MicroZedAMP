/* Copyright (C) 2012 Xilinx Inc. */

/*
 * This header defines a number of structs, macros and variables that are used
 * for communication with the Linux Kernel.
 *
 * - RPMSG primitives, and macros
 * - ELF file section definitions
 * - Resource Table types and value macros
 */

#ifndef REMOTEPROC_KERNEL_H
#define REMOTEPROC_KERNEL_H

/* Just load all symbols from Linker script */
extern char *__ring_rx_addr;
#define RING_RX					(unsigned int)&__ring_rx_addr
extern char *__ring_tx_addr;
#define RING_TX					(unsigned int)&__ring_tx_addr

extern char *__ring_tx_addr_used;
#define RING_TX_USED			(unsigned int)&__ring_tx_addr_used
extern char *__ring_rx_addr_used;
#define RING_RX_USED			(unsigned int)&__ring_rx_addr_used

extern char *_start; /* ELF_START should be zero all the time */
#define ELF_START				(unsigned int)&_start
extern char *__elf_end;
#define ELF_END					(unsigned int)&__elf_end

extern char *__trace_buffer_start;
#define TRACE_BUFFER_START		(unsigned int)&__trace_buffer_start
extern char *__trace_buffer_end;
#define TRACE_BUFFER_END		(unsigned int)&__trace_buffer_end

/* This value should be shared with Linker script */
#define TRACE_BUFFER_SIZE		0x8000

/* section helpers */
#define __section(S)			__attribute__((__section__(#S)))
#define __resource				__section(.resource_table)

/* flip up bits whose indices represent features we support */
#define RPMSG_IPU_C0_FEATURES	1

/* virtio ids: keep in sync with the linux "include/linux/virtio_ids.h" */
#define VIRTIO_ID_CONSOLE		3 /* virtio console */
#define VIRTIO_ID_RPMSG			7 /* virtio remote processor messaging */

/* Indices of rpmsg virtio features we support */
#define VIRTIO_RPMSG_F_NS		0 /* RP supports name service notifications */

/* Resource info: Must match include/linux/remoteproc.h: */
#define TYPE_CARVEOUT			0
#define TYPE_DEVMEM				1
#define TYPE_TRACE				2
#define TYPE_VDEV				3
#define TYPE_MMU				4

#define NO_RESOURCE_ENTRIES		13

struct fw_rsc_mmu {
	unsigned int type;
	unsigned int id;
	unsigned int da;
	unsigned int len; /* unused now */
	unsigned int flags;
	char name[32];
};

struct fw_rsc_carveout {
	unsigned int type;
	unsigned int da;
	unsigned int pa;
	unsigned int len;
	unsigned int flags;
	unsigned int reserved;
	char name[32];
};

struct fw_rsc_devmem {
	unsigned int type;
	unsigned int da;
	unsigned int pa;
	unsigned int len;
	unsigned int flags;
	unsigned int reserved;
	char name[32];
};

struct fw_rsc_trace {
	unsigned int type;
	unsigned int da;
	unsigned int len;
	unsigned int reserved;
	char name[32];
};

struct fw_rsc_vdev_vring {
	unsigned int da; /* device address */
	unsigned int align;
	unsigned int num;
	unsigned int notifyid;
	unsigned int reserved;
};

struct fw_rsc_vdev {
	unsigned int type;
	unsigned int id;
	unsigned int notifyid;
	unsigned int dfeatures;
	unsigned int gfeatures;
	unsigned int config_len;
	char status;
	char num_of_vrings;
	char reserved[2];
};

struct rpmsg_channel_info {
#define RPMSG_NAME_SIZE			32
	char name[RPMSG_NAME_SIZE];
	unsigned int src;
	unsigned int dst;
};

struct rpmsg_hdr {
	unsigned int src;
	unsigned int dst;
	unsigned int reserved;
	unsigned short len;
	unsigned short flags;
	unsigned char data[0];
} __packed;


/* Virtio ring descriptors: 16 bytes.  These can chain together via "next" */
struct vring_desc {
	unsigned int addr; /* Address (guest-physical). */
	unsigned int addr_hi;
	unsigned int len; /* Length. */
	unsigned short flags; /* The flags as indicated above. */
	unsigned short next; /* We chain unused descriptors via this, too */
};

/* unsigned int is used here for ids for padding reasons. */
struct vring_used_elem {
	unsigned int id; /* Index of start of used descriptor chain. */
	unsigned int len; /* Total length of the descriptor chain which was used (written to) */
};

struct vring_used {
	unsigned short flags;
	unsigned short idx;
	struct vring_used_elem ring[];
};

#define VRING_ADDR_MASK				0xffffff
#define VRING_SIZE					256

/* Tx Vring IRQ from Linux */
#define TXVRING_IRQ					2
/* Rx Vring IRQ from Linux */
#define RXVRING_IRQ					3
/* IRQ to notify Linux */
#define NOTIFY_LINUX_IRQ			6

/* vring data buffer max length including the header */
#define PACKET_LEN_MAX				512
#define DATA_LEN_MAX				(PACKET_LEN_MAX - sizeof(struct rpmsg_hdr))

#endif /* REMOTEPROC_KERNEL_H */
