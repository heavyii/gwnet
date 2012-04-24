/*
 * gwsocket.h
 *
 * Copyright (C) 2011 crazyleen <ruishenglin@126.com>
 *
 */

#ifndef __GWSOCKET_H__
#define __GWSOCKET_H__

#include <linux/if_packet.h>

struct eapol_data {
	char ifname[12]; /* net card name, such as "eth0" */
	int sockfd;
	struct sockaddr_ll sll;
	char local_mac[6];
	char dest_mac[6]; /* standard 802.1x mac */
	char user[48];
	char passwd[48];
};

/**
 * get_hwaddr - get netdevice mac addr
 * @name: device name, e.g: eth0
 * @hwaddr: where to save mac, 6 byte hwaddr[6]
 * @return: 0 on success, -1 on failure
 */
int get_hwaddr(char *name, unsigned char *hwaddr);

/**
 * socket_eapol_init - init socket for eapol protocol
 * @ifname: net card name, such as "eth0"
 * @psll:   where to store struct sockaddr_ll
 * @return: return socket, -1 on error
 */
int socket_eapol_init(char *ifname, struct sockaddr_ll *psll);


/**
 * pselect_recvfrom - recvfrom with pselect
 * @fd:	sockfd
 * @buf: where to store rec data
 * @len: buf length
 * @tv_sec: timeout seconds
 * @return: returns the number of bytes recvfrom, -1 on timeout or error.
 */
int pselect_recvfrom(int fd, void *buf, int len, int tv_sec);

#endif /* _GWSOCKET_H_ */

