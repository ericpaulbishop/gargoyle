/*
 * This program is copyright © 2013 Cezary Jackiewicz and is distributed under the terms of the GNU GPL 
 * version 2.0 with a special clarification/exception that permits adapting the program to 
 * configure proprietary "back end" software provided that all modifications to the web interface
 * itself remain covered by the GPL.
 * See http://gargoyle-router.com/faq.html#qfoss for more information
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

int main(int argc, char *argv[]){
	int fd;
	int c;
	char *device="/dev/video0";
	struct v4l2_capability cap;
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	struct v4l2_fmtdesc fmt;
	struct v4l2_frmsizeenum frmsize;
	struct v4l2_frmivalenum frmival;

	while ( (c = getopt(argc, argv, "d:")) != -1) {
		switch (c) {
			case 'd':
				device = strdup(optarg);
				break;
		}
	}

	if((fd = open(device, O_RDONLY)) == -1) {
		return -1;
	}
	if ( ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
		return -1;
	}
	printf("webcams['%s']['webcam']='%s';\n", device,cap.card);

	fmt.index = 0;
	fmt.type = type;
	if (ioctl(fd, VIDIOC_ENUM_FMT, &fmt) >= 0) {
		frmsize.pixel_format = fmt.pixelformat;
		frmsize.index = 0;
		while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0) {
			if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
				printf("webcams['%s']['res'][%d]='%dx%d';\n", device,frmsize.index,
					frmsize.discrete.width,
					frmsize.discrete.height);
			} else if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
					printf("webcams['%s']['res'][%d]='%dx%d';\n", device,frmsize.index,
					frmsize.stepwise.max_width,
					frmsize.stepwise.max_height);
			}
			frmsize.index++;
		}
	}

	close(fd);
	return 0;
}
