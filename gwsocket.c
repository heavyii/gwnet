/*
 * gwsocket.c
 *
 * Copyright (C) 2011 crazyleen <ruishenglin@126.com>
 *
 */

#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <features.h>    /* for the glibc version number */
#if __GLIBC__ >= 2 && __GLIBC_MINOR >= 1
#include <netpacket/packet.h>
#include <net/ethernet.h>     /* the L2 protocols */
#else
#include <asm/types.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>   /* The L2 protocols */
#endif
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdlib.h>
#include "gwsocket.h"

#define _PATH_PROCNET_DEV               "/proc/net/dev"

static char *get_name(char *name, char *p) {
	while (isspace(*p))
		p++;

	while (*p) {
		if (isspace(*p))
			break;
		if (*p == ':') { /* could be an alias */
			char *dot = p, *dotname = name;
			*name++ = *p++;
			while (isdigit(*p))
				*name++ = *p++;
			if (*p != ':') { /* it wasn't, backup */
				p = dot;
				name = dotname;
			}
			if (*p == '\0')
				return NULL;
			p++;
			break;
		}
		*name++ = *p++;
	}
	*name++ = '\0';
	return p;
}

/**
 * read_netdev_proc - read net dev names form proc/net/dev
 * @devname: where to store dev names, devname[num][len]
 */
static int read_netdev_proc(void *devname, const int num, const int len) {
	FILE *fh;
	char buf[512];
	int cnt = 0;
	char *dev = (char *) devname;

	if (devname == NULL || num < 1 || len < 4) {
		printf("read_netdev_proc: para error\n");
		return -1;
	}

	memset(devname, 0, len * num);

	fh = fopen(_PATH_PROCNET_DEV, "r");
	if (!fh) {
		fprintf(stderr, "Warning: cannot open %s (%s). Limited output.\n",
				_PATH_PROCNET_DEV, strerror(errno));
		return -1;
	}

	fgets(buf, sizeof buf, fh); /* eat two line */
	fgets(buf, sizeof buf, fh);

	cnt = 0;
	while (fgets(buf, sizeof buf, fh) && cnt < num) {
		char *s, name[IFNAMSIZ];
		s = get_name(name, buf);

		strncpy(devname, name, len);
		devname += len;
		printf("get_name: %s\n", name);
	}

	if (ferror(fh)) {
		perror(_PATH_PROCNET_DEV);
	}

	fclose(fh);
	return 0;
}

/**
 * get_hwaddr - get netdevice mac addr
 * @name: device name, e.g: eth0
 * @hwaddr: where to save mac, 6 byte hwaddr[6]
 * @return: 0 on success, -1 on failure
 */
int get_hwaddr(char *name, unsigned char *hwaddr) {
	struct ifreq ifr;
	unsigned char memzero[6];
	int sock;

	if (name == NULL || hwaddr == NULL) {
		printf("get_hwaddr: NULL para\n");
		return -1;
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		printf("get_hwaddr: socket error\n");
		//return -1;
	}

	//get eth1 mac addr
	memset(hwaddr, 0, 6);
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, name, 6);
	if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
		perror("get_hwaddr ioctl:");
		close(sock);
		return -1;
	} else {
		memcpy(hwaddr, ifr.ifr_hwaddr.sa_data, 6);
		//printf("hwaddr: %2x : %2x : %2x : %2x : %2x : %2x\n", hwaddr[0], hwaddr[1],hwaddr[2], hwaddr[3],hwaddr[4], hwaddr[5]);
	}

	memset(memzero, 0, 6);
	if (memcmp(memzero, hwaddr, 6) == 0) {
		printf("no mac\n");
		return -1;
	}

	close(sock);
	return 0;
}

/**
 * socket_eapol_init - init socket for eapol protocol
 * @ifname: net card name, such as "eth0"
 * @psll:   where to store struct sockaddr_ll
 * @return: return socket, -1 on error
 */
int socket_eapol_init(char *ifname, struct sockaddr_ll *psll) {
	struct ifreq ifr;
	int sockfd;
	int sockopts, sockerr, retval;

	//check para
	if (ifname == NULL || psll == NULL) {
		printf("para NULL\n");
		return -1;
	}

	bzero(psll, sizeof(struct sockaddr_ll));
	bzero(&ifr, sizeof(ifr));

	sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_PAE));
	if (sockfd < 0) {
		perror("socket");
		return -1;
	}

	//indicate net card
	strcpy((char *) &ifr.ifr_name, ifname);
	retval = ioctl(sockfd, SIOCGIFINDEX, &ifr);
	if (retval < 0) {
		perror("ioctl");
		close(sockfd);
		return -1;
	}

	psll->sll_family = PF_PACKET;
	psll->sll_ifindex = ifr.ifr_ifindex;
	psll->sll_protocol = htons(ETH_P_PAE);

	retval = bind(sockfd, (const struct sockaddr *) psll,
			sizeof(struct sockaddr_ll));
	if (retval < 0) {
		perror("bind");
		close(sockfd);
		return -1;
	}

	//set nonblock socket
	sockopts = fcntl(sockfd, F_GETFL, 0);
	if (sockopts < 0) {
		perror("sockopts: fcntl");
		close(sockfd);
		return -1;
	}
	sockerr = fcntl(sockfd, F_SETFL, sockopts | O_NONBLOCK);
	if (sockerr < 0) {
		perror("sockerr: fcntl");
		//close(sockfd);
		//return -1;
	}

	return sockfd;
}

static void sig_intr_handler(int signo) {
	printf("SIGINT, exit...\n");
	exit(0);
}

/**
 * pselect_recvfrom - recvfrom with pselect
 * @fd:	sockfd
 * @buf: where to store rec data
 * @len: buf length
 * @tv_sec: timeout seconds
 * @return: returns the number of bytes recvfrom, -1 on timeout or error.
 */
int pselect_recvfrom(int fd, void *buf, int len, int tv_sec) {
	fd_set read_set;
	int ret;
	struct timespec timeout;
	sigset_t sigset_full;

	if(buf == NULL){
		printf("para NULL\n");
		return -1;
	}
	bzero(buf, len);

	signal(SIGINT, sig_intr_handler);
	signal(SIGTERM, sig_intr_handler);
	sigfillset(&sigset_full);
	sigprocmask(SIG_BLOCK, &sigset_full, NULL);

	sigfillset(&sigset_full);
	sigdelset(&sigset_full, SIGINT);
	sigdelset(&sigset_full, SIGTERM);
	FD_ZERO(&read_set);
	FD_SET(fd, &read_set);
	timeout.tv_sec = tv_sec;
	timeout.tv_nsec = 0;

	//wait with all signals(except SIGINT) blocked.
	ret = pselect(fd + 1, &read_set, NULL, NULL, &timeout, &sigset_full);
	if (ret <= 0) {
		//perror("pselect");
		return -1;
	}

	//rec data
	if (FD_ISSET(fd, &read_set)) {
		return recvfrom(fd, buf, len, 0, NULL, NULL);
	}

	return -1;
}

