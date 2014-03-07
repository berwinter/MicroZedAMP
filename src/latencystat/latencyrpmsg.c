#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "latencyrpmsg.h"

/* Not checking return state but it can be done */
int rpmsg_send_message(struct rpmsg_target* target, latency_demo_msg_type command)
{
	ssize_t ret;
	unsigned int current_command = (unsigned int)command;
	unsigned int response_command = 0;

	if (target == NULL) {
		return -1;
	}

	/* Send commands to FreeRTOS */
	ret = write(target->fd, &current_command, sizeof(unsigned int));
	if (ret < 0) {
		perror(__FUNCTION__);
		return -1;
	}

	/* Wait for command acknowlodgement.
	 * This application expects the FreeRTOS application
	 * sends back acknowlodgement message after it receives
	 * requests.
	 * 
	 * In case extra data is passed on command, disregard until an ACK is found.
	 */
	while (1)
	{
		ret = read(target->fd, &response_command, sizeof(unsigned int));
		if (ret < 0) {
			perror(__FUNCTION__);
			return -1;
		}

		/* Check if ACK from firmware was sent */
		if ((current_command == (response_command & STATE_MASK)) &&
				(response_command & REMOTEPROC_REQUEST_ACK_MASK))
		{
			printf("%4d: Command %d ACKed\n", target->command_no++, command);
			return 0;
		}
	}
	return -1;
}

int rpmsg_read_response(struct rpmsg_target* target, char* data, size_t len)
{
	ssize_t ret;
	size_t data_read = 0;
	size_t data_left = len;
	char* data_current = data;

	if (target == NULL || data == NULL) {
		return -1;
	}

	if (len != 0) {
		do {
			ret = read(target->fd, data_current, data_left); /* Read statistic */
			if (ret < 0) {
				perror(__FUNCTION__);
				return -1;
			}
			data_read += ret;
			data_left -= ret;
			data_current += ret;
		} while (data_read < len);
	}
	return data_read;
}

int rpmsg_open_device(struct rpmsg_target* target, char* dev)
{
	int fd; /* File description */

	if (target == NULL || dev == NULL) {
		return -1;
	}

	fd = open(dev, O_RDWR);
	if (fd < 0) {
		perror(__FUNCTION__);
		return -1;
	}

	target->fd = fd;
	target->command_no = 0;

	return 0;
}

int rpmsg_close_device(struct rpmsg_target* target)
{
	int ret;
	if (target == NULL) {
		return -1;
	}

	ret = close(target->fd);
	if (ret < 0) {
		perror(__FUNCTION__);
		return -1;
	}
	return 0;
}