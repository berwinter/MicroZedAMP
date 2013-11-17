MicroZed FreeRTOS AMP Application
======

This is a port of the *Zynq AMP Linux FreeRTOS* reference design. For more information about the demo application read the orginal document. [\[UG978\]](http://www.xilinx.com/support/documentation/sw_manuals/petalinux2013_04/ug978-petalinux-zynq-amp.pdf)

Testing Pre-Built Reference Design
-----

You can test the pre-build images as follows:

***Boot PetaLinux***

1. Conﬁgure the MicroZed to use SD boot mode by connecting 2-3 of jumper JP2 and JP2 on the board.
2. Connect the USB port on MircoZed to your host
3. Copy the "BOOT.BIN", "devicetree.dtb", "uramdisk.image.gz" and "uImage" from the pre-built directory to your SD card.
4. Insert the SD card into the SD card slot on MircoZed and then power on the board.
5. Use a serial terminal application to monitor the UART output from MircoZed. Conﬁgure the terminal application to use a baudrate of 115200-8N1.

***Starting FreeRTOS Firmware***

1. Watch the console. Log into the Linux system with username: root and password: root.
2. Load the remoteproc modules to prepare to load the 2nd processor with FreeRTOS ﬁrmware from the console as follows:

	modprobe virtio
	modprobe virtio_ring
	modprobe virtio_rpmsg_bus
	modprobe rpmsg_proto
	modprobe remoteproc

3. Load the 2nd processor with FreeRTOS ﬁrmware as follows:

	modprobe zynq_remoteproc

4. You can see the following messages on the console:

```
NET: Registered protocol family 40
CPU1: shutdown
remoteproc0: 0.remoteproc-test is available
remoteproc0: Note: remoteproc is still under development and considered
experimental.
remoteproc0: THE BINARY FORMAT IS NOT YET FINALIZED, and backward compatibility
isn’t yet guaranteed.
remoteproc0: powering up 0.remoteproc-test
remoteproc0: Booting fw image freertos, size 2310301
remoteproc0: remote processor 0.remoteproc-test is now up
virtio_rpmsg_bus virtio0: rpmsg host is online
remoteproc0: registered virtio0 (type 7)
virtio_rpmsg_bus virtio0: creating channel rpmsg-timer-statistic addr 0x50
```

***Demo Application***

1. The FreeRTOS application provided in the pre-built reference design collects interrupt latency statistics within the FreeRTOS environment, and reports the results to Linux which are displayed by the `latencystat` Linux demo application. The ´rpmsg_freertos_statistic´ module must ﬁrst be loaded so that we can send/receive messages to FreeRTOS. To load the module run the following command in the console

	modprobe rpmsg_freertos_statistic

2. Run ´mdev´ to scan and add new devices to ´/dev´:
3. Run ´latencystat´ demo application as follows:

	latencystat -b

4. The application will print output similar to the following:

```
Linux FreeRTOS AMP Demo.
0: Command 0 ACKed
1: Command 1 ACKed
Waiting for samples...
2: Command 2 ACKed
3: Command 3 ACKed
4: Command 4 ACKed
-----------------------------------------------------------
Histogram Bucket Values:
Bucket 332 ns (37 ticks) had 14814 frequency
Bucket 440 ns (49 ticks) had 1 frequency
Bucket 485 ns (54 ticks) had 1 frequency
Bucket 584 ns (65 ticks) had 1 frequency
Bucket 656 ns (73 ticks) had 1 frequency
-----------------------------------------------------------
Histogram Data:
min: 332 ns (37 ticks)
avg: 332 ns (37 ticks)
max: 656 ns (73 ticks)
out of range: 0
total samples: 14818
-----------------------------------------------------------
```

The `latencystat` demo application sends requests to FreeRTOS to ask for latency histogram data. The FreeRTOS will reply with the histogram data, and the latency demo application dumps that data.

The `latencystat` demo application can display the information in a graph format or dump the data in hex. Use the `-h` parameter to display the help information of the application.

***Accessing the Trace Buffer*** 

The Trace Buffer is a section of shared memory which is only written to by the FreeRTOS application. This Trace Buffer can be used as a logging console to transfer information to Linux. It can act similar to a one way serial console.

The Trace Buffer is a ring buffer, this means that after the Buffer is full it will wrap around and begin writing to the start of the buffer. When accessing the buffer via Linux it will not be read as a stream. The default Trace Buffer is 32 KB in size. The Trace Buffer can be accessed via debugfs as a ﬁle.

	mount -t debugfs none /sys/kernel/debug
	cat /sys/kernel/debug/remoteproc/remoteproc0/trace0

Contact
------
Bertram Winter
bertram.winter@gmail.com
