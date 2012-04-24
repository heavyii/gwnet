/*
 * gwnet.c
 *
 * Copyright (C) 2011 crazyleen <ruishenglin@126.com>
 * 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "packet.h"
#include "gwsocket.h"
#include "para.h"
#include "gwnet.h"
/*
 * description
 */

#define GWNET_VERSION "V0.8"

static struct eapol_data eapdata;

/**
 * eapdata_init - init eapdata(build socket, indicate net device, save account)
 * @return: 0 on success, -1 on error
 */
int eapdata_init(struct eapol_data *eapdatap, char *netdev, char *user,
		char *passwd) {
	if(eapdatap == NULL || netdev == NULL || user == NULL || passwd == NULL){
		printf("%s: null para\n", __FUNCTION__);
		return -1;
	}

	bzero(eapdatap, sizeof(struct eapol_data));
	strncpy(eapdatap->ifname, netdev, 12);
	strncpy(eapdatap->user, user, 48);
	strncpy(eapdatap->passwd, passwd, 48);

	eapdatap->sockfd = socket_eapol_init(eapdatap->ifname, &eapdatap->sll);
	if (eapdatap->sockfd < 0) {
		printf("socket_eapol_init error\n");
		return -1;
	}

	//get mac
	if (get_hwaddr(eapdatap->ifname, eapdatap->local_mac) < 0) {
		printf("get %s mac error\n", eapdatap->ifname);
		return -1;
	}

	return 0;
}

void gwnet_logoff(void) {
	PACKET pkt_logoff;
	packet_get_logoff(&pkt_logoff, eapdata.local_mac);
	printf("logoff...\n");
	if (eapdata.sockfd < 0 || &pkt_logoff == NULL) {
		return;
	}

	if (sendto(eapdata.sockfd, &pkt_logoff, packet_length(&pkt_logoff), 0,
			(struct sockaddr *) &eapdata.sll, sizeof eapdata.sll) < 0) {
		perror("sendto");
	}
}

int main(int argc, char **argv) {
	PACKET pkt_recv;
	int reclen;
	int i;
	int cnt = 0;
	struct para_data conf;
	bzero(&conf, sizeof conf);
	conf.file = "/etc/gwnet.conf";
	para_init(&conf);

	//get config
	char *user = NULL;
	char *passwd = NULL;
	char *netdev = NULL;

	printf("gwnet %s\n", GWNET_VERSION);
	printf("Copyright (C) 2012 crazyleen <ruishenglin@126.com>\n");

	if(argc < 4){
		user = para_read(&conf, "username");
		passwd = para_read(&conf, "password");
		netdev = para_read(&conf, "ifname");
	}else if(argc == 4){
		netdev = argv[1];
		user = argv[2];
		passwd = argv[3];

		if(netdev != NULL && user != NULL && passwd != NULL){
			para_update(&conf, "ifname", netdev, "gwnet config file");
			para_update(&conf, "username", user, NULL);
			para_update(&conf, "password", passwd, NULL);
		}
		para_print(&conf);
		para_write(&conf);
	}

	if(netdev == NULL || user == NULL || passwd == NULL){
		printf("USAGE:\n");
		printf("\tgwnet\n");
		printf("\tgwnet netCardName User Password\n");
		printf("config file:%s, cmd \"gwnet netCardName User Password\" will update config file\n", conf.file);
		printf("\texample: gwnet eth0 crazyleen 123456\n");
		exit(0);
	}

	printf("ifname: %s\nuser: %s\npasswd: %s\n", netdev, user, "******");

	//init socket
	if (eapdata_init(&eapdata, netdev, user, passwd) < 0) {
		printf("socket init failed\n");
		return -1;
	}

	//packet init
	PACKET pkt_start;
	packet_get_start(&pkt_start, eapdata.local_mac);

start_auth:
	//find server
	for (i = 0; i < 10; i++) {
		printf("find server...\n");
		if (sendto(eapdata.sockfd, &pkt_start, packet_length(&pkt_start), 0,
				(struct sockaddr *) &eapdata.sll, sizeof eapdata.sll) < 0) {
			perror("sendto");
			return -1;
		}

		bzero(&pkt_recv, sizeof pkt_recv);
		reclen = pselect_recvfrom(eapdata.sockfd, &pkt_recv, sizeof pkt_recv,
				2);
		if (reclen <= 0) {
			printf("timeout\n");
			continue;
		}

		//server founded
		if (packet_type(&pkt_recv) == PACKET_REQUEST_ID) {
			break;
		}
	}
	if (i == 10) {
		//no server
		printf("can't find server!!\n");
		return 1;
	}

	//init identify packet
	PACKET pkt_id;
	packet_get_identify(&pkt_id, eapdata.local_mac, eapdata.dest_mac,
			eapdata.user);

	printf("send start packet...\n");
	if (sendto(eapdata.sockfd, &pkt_start, packet_length(&pkt_start), 0,
			(struct sockaddr *) &eapdata.sll, sizeof eapdata.sll) < 0) {
		perror("sendto");
		return -1;
	}

	atexit(gwnet_logoff);
	while (1) {

		bzero(&pkt_recv, sizeof pkt_recv);
		reclen = pselect_recvfrom(eapdata.sockfd, &pkt_recv, sizeof pkt_recv,
				30);
		if (reclen <= 0) {
			printf("timeout\n");
			goto start_auth;
		}

		//packet_print(&pkt_recv);

		switch (packet_type(&pkt_recv)) {
		case PACKET_REQUEST_ID:
			printf("send identity packet... (%d)\r\b", cnt++);
			fflush(stdout);
			if (sendto(eapdata.sockfd, &pkt_id, packet_length(&pkt_id), 0,
					(struct sockaddr *) &eapdata.sll, sizeof eapdata.sll) < 0) {
				perror("sendto");
				return -1;
			}
			break;
		case PACKET_REQUEST_CH: {
			PACKET pkt_passwd;
			printf("send challenge packet...\n");
			packet_get_challenge(&pkt_passwd, &pkt_recv, eapdata.local_mac,
					eapdata.dest_mac, eapdata.user, eapdata.passwd);
			if (sendto(eapdata.sockfd, &pkt_passwd, packet_length(&pkt_passwd),
					0, (struct sockaddr *) &eapdata.sll, sizeof eapdata.sll)
					< 0) {
				perror("sendto");
				return -1;
			}
		}
			break;
		case PACKET_SUCCESS:
			printf("login success!!\n");
			break;
		case PACKET_FAIL:
			printf("login failed, may be used by other or wrong password\n");
			exit(3);
			break;
		default:
			printf("unknow packet\n");
			break;
		}
	}

	return 0;
}
