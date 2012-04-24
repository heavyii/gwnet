/*
 * packet.c
 *
 * Copyright (C) 2011 crazyleen <ruishenglin@126.com>
 * 
 */
 
#include <stdio.h>
#include <string.h>
#include "packet.h"
#include "md5.h"
#include "gwnet.h" 
/*
 * description
 */

const char mac_8021x_standard[6] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x03};

/**
 * packet_get_start - make a start packet
 * @pkt:	where to store packet dataoff
 * @srcmac: local mac
 * @return: return pkt
 */
extern PACKET *packet_get_start(PACKET	*pkt, char *srcmac)
{

	if(pkt == NULL || srcmac == NULL){
		PKT_Debug("null pointer");
		return NULL;
	}

	memset(pkt, 0, sizeof(PACKET));
	memcpy(pkt->srcmac, srcmac, 6);
	memcpy(pkt->dstmac, mac_8021x_standard, 6);
	pkt->eth_type[0] = 0x88;
	pkt->eth_type[1] = 0x8e;
	pkt->version = 0x01;

	pkt->pkt_type = PKT_TYPE_START;//Type:  Start (1)
	pkt->bodylength[0] = 0x00;
	pkt->bodylength[1] = 0x00;

	return pkt;
}

/**
 * packet_get_identify - make a identify packet
 * @pkt:	where to store packet dataoff
 * @return: return pkt, or null
 */
extern PACKET *packet_get_identify(PACKET	*pkt, char *srcmac, char *dstmac, char *username)
{
	int len;

	if(pkt == NULL || srcmac == NULL || username == NULL){
		PKT_Debug("null pointer");
		return NULL;
	}

	len = strlen(username);
	if(len > USER_NAME_LEN_MAX)
		len = USER_NAME_LEN_MAX;

	memset(pkt, 0, sizeof(PACKET));
	memcpy(pkt->srcmac, srcmac, 6);
//	if(dstmac != NULL){
//		memcpy(pkt->dstmac, dstmac, 6);
//	}else{
		memcpy(pkt->dstmac, mac_8021x_standard, 6);
//	}
	pkt->eth_type[0] = 0x88;
	pkt->eth_type[1] = 0x8e;
	pkt->version = 0x01;

	pkt->pkt_type = PKT_TYPE_EAP;//Type: eap package(0)

	//body length
	pkt->bodylength[0] = (len + BODY_LENGTH_IDENTIFY_BASE) >> 8;
	pkt->bodylength[1] = len + BODY_LENGTH_IDENTIFY_BASE;

	pkt->body.identify.bodylength[0] = pkt->bodylength[0];
	pkt->body.identify.bodylength[1] = pkt->bodylength[1];

	pkt->body.identify.code = BODY_CODE_RESPONSE;
	pkt->body.identify.id = 0x01;//0x01 for stage 1
	pkt->body.identify.type = BODY_TYPE_IDENTIFY;

	memcpy(pkt->body.identify.username, username, len);
	return pkt;
}

/**
 * packet_get_challenge - make a challenge packet
 * @pkt:	where to store packet dataoff
 * @src_pkt:request Challenge packet from server
 * @return: return pkt, or null
 */
extern PACKET *packet_get_challenge(PACKET *pkt, PACKET *src_pkt, char *srcmac, char *dstmac,
		char *username, char *password)
{

	int userlen;
//	if(pkt == NULL || src_pkt || username == NULL || password == NULL){
//		PKT_Debug("null pointer");
//		return NULL;
//	}

	userlen = strlen(username);
	memset(pkt, 0, sizeof(PACKET));
//	if(dstmac != NULL){
//		memcpy(pkt->dstmac, dstmac, 6);
//	}else{
		memcpy(pkt->dstmac, mac_8021x_standard, 6);
//	}
	memcpy(pkt->srcmac, srcmac, 6);
	pkt->eth_type[0] = 0x88;
	pkt->eth_type[1] = 0x8e;
	pkt->version = 0x01;

	pkt->pkt_type = PKT_TYPE_EAP;//Type: eap package(0)

	//body length
	pkt->bodylength[0] = (userlen + BODY_LENGTH_CHALLENGE_BASE) >> 8;
	pkt->bodylength[1] = userlen + BODY_LENGTH_CHALLENGE_BASE;

	pkt->body.challenge.bodylength[0] = pkt->bodylength[0];
	pkt->body.challenge.bodylength[1] = pkt->bodylength[1];

	pkt->body.challenge.code = BODY_CODE_RESPONSE;
	pkt->body.challenge.id = src_pkt->body.request_challenge.id;//hold same id
	pkt->body.challenge.type = BODY_TYPE_CHALLENGE;
	pkt->body.challenge.value_size = PACKET_CHALLENGE_VELUE_SIZE;
	
	// compute hush value[16]--------------------------------------
	/* XXX:
	 *	Hei, challenge value is a hush value: 
	 * 	data[0]: src_pkt->body.request_challenge.id; a id from server
	 *  data[1]~data[passwordlength + 1]:	your account pass word	
	 *  data[passwordlength + 2] ~ data[passwordlength + 16]: value[16]
	 *  from server
	 *
	 * Every one loves PIC:
	 *  data:
	 * 		 --------------------------
	 * 		|id | password | value[16] |
	 * 		 --------------------------
	 * compute this data hush value, that is challenge value!
	 */
	unsigned char data[64] = "";
	unsigned char *hushvalue = NULL;
	int datalen = 0;
	int passwdlen = 0;

	data[0] = src_pkt->body.request_challenge.id;
	datalen = 1;

	passwdlen = strlen(password);
	memcpy(data + datalen, password, passwdlen);
	datalen += passwdlen;

	memcpy(data + datalen, src_pkt->body.request_challenge.value, src_pkt->body.request_challenge.value_size);
	datalen += src_pkt->body.request_challenge.value_size;

	hushvalue = ComputeHash(data, datalen);
	//compute hush value[16]done!!---------------------------------

	memcpy(pkt->body.challenge.value, hushvalue, src_pkt->body.request_challenge.value_size);

	strcpy(pkt->body.challenge.username, username);

	return pkt;
}

extern PACKET *packet_get_logoff(PACKET	*pkt, char *srcmac)
{
	if(pkt == NULL || srcmac == NULL){
		PKT_Debug("null pointer");
		return NULL;
	}

	memset(pkt, 0, sizeof(PACKET));
	memcpy(pkt->dstmac, mac_8021x_standard, 6);
	memcpy(pkt->srcmac, srcmac, 6);
	pkt->eth_type[0] = 0x88;
	pkt->eth_type[1] = 0x8e;
	pkt->version = 0x01;

	pkt->pkt_type = PKT_TYPE_LOGOFF;//Type: Logoff (2)
	pkt->bodylength[0] = 0x00;
	pkt->bodylength[1] = 0x00;

	return pkt;
}

static void printhex(char *start, void *dst, int len, char *end)
{
	unsigned char *p = NULL;
	int i;

	if(start != NULL)
		printf("%s", start);

	for(p = (unsigned char *)dst, i = 0; i < len; i++, p++){
		if(*p >= 0x10)
			printf("0x%2x ", *p);
		else
			printf("0x0%1x ", *p);
	}

	if(end != NULL)
		printf("%s", end);
}

/**
 * packet_type - tell what type of packet 
 * @pkt:	PACKET pointer
 * @return:	return packet type macro, see packet.h
 */
int packet_type(PACKET	*pkt)
{
	switch(pkt->pkt_type){
		case PKT_TYPE_EAP:
			//printf("###this is packet EAP\n");
			break;
		case PKT_TYPE_START:
			//printf("###this is packet_start\n");
			return PACKET_START;
			break;
		case PKT_TYPE_LOGOFF:
			//printf("###this is packet logoff\n");
			return PACKET_LOGOFF;
			break;
		default:
			return PACKET_UNKNOW;
	}

	//EAP packet
	switch(pkt->body.test.code){
		case BODY_CODE_REQUEST:
			if(pkt->body.test.type == BODY_TYPE_IDENTIFY){	
				//printf("####### request IDENTIFY\n");
				return PACKET_REQUEST_ID;
			}else if(pkt->body.test.type == BODY_TYPE_CHALLENGE){	
				//printf("####### request IDENTIFY\n");
				return PACKET_REQUEST_CH;
			}
			break;

		case BODY_CODE_RESPONSE:
			if(pkt->body.test.type == BODY_TYPE_IDENTIFY){	
				//printf("####### response IDENTIFY\n");
				return PACKET_RESPONSE_ID;
			}else if(pkt->body.test.type == BODY_TYPE_CHALLENGE){	
				//printf("####### response CHALLENGE\n");
				return PACKET_RESPONSE_CH;
			}
			break;

		case BODY_CODE_SUCCESS:
			//printf("####this is a SUCCESS packet\n");	
			return PACKET_SUCCESS;
			break;
		case BODY_CODE_FAIL:
			//printf("####this is a FAIL packet\n");
			return PACKET_FAIL;
			break;
		default:
			return PACKET_UNKNOW;
	}
}

int packet_length(PACKET *pkt) {

	if(pkt == NULL){
		printf("para null\n");
		return 0;
	}

	return pkt->bodylength[1] + (pkt->bodylength[0] << 8) + 18;
}

/**
 * packet_print - print packet info
 */
void packet_print(PACKET	*pkt)
{
	unsigned char *p = NULL;
	int i;
	int is_request_not_response = -1;

	printf("\n\n");
	if(pkt == NULL){
		printf("empty packet\n");
		return;
	}

	printhex("dst mac:\t", pkt->dstmac, 6, "\n");
	printhex("src mac:\t", pkt->srcmac, 6, "\n");
	printhex("eth_type:\t  ", pkt->eth_type, 2, "\n");
	printhex("version:\t  ", &pkt->version, 1, "\n");
	printhex("pkt_type:\t  ", &pkt->pkt_type, 1, "\n");
	printhex("bodylength:\t  ", pkt->bodylength, 2, " ");
	printf("(%d)\n", PACKET_LENGTH(pkt) - 18);

	
	switch(packet_type(pkt)){
		case PACKET_START:
			printf("###this is packet PACKET_START\n");
			break;

		case PACKET_LOGOFF:
			printf("###this is packet PACKET_LOGOFF\n");
			break;

		case PACKET_SUCCESS:
			printf("###this is packet PACKET_SUCCESS\n");
			break;

		case PACKET_FAIL:
			printf("###this is packet PACKET_FAIL\n");
			break;

		case PACKET_REQUEST_ID:
			printf("###this is packet PACKET_REQUEST_ID\n");
			break;

		case PACKET_REQUEST_CH:
			printf("###this is packet PACKET_REQUEST_CH\n");
			printhex("###value_size:\t  ", &pkt->body.request_challenge.value_size, 1, "\n");
			printhex("###value:\t  ", pkt->body.request_challenge.value, pkt->body.request_challenge.value_size, "\n");
			break;

		case PACKET_RESPONSE_ID:
			printf("###this is packet PACKET_RESPONSE_ID\n");
			printf("###username %24s\n", pkt->body.identify.username);
			break;

		case PACKET_RESPONSE_CH:
			printf("###this is packet PACKET_RESPONSE_CH\n");
			printf("####### response challenge\n");
			printhex("###value_size:\t  ", &pkt->body.challenge.value_size, 1, "\n");
			printhex("###value:\t  ", pkt->body.challenge.value, pkt->body.challenge.value_size, "\n");
			printhex("###username:\t  ", pkt->body.challenge.username, USER_NAME_LEN_MAX, "\n");
			printf("###username: %24s\n", pkt->body.challenge.username);
			break;

		default:
			printf("-------unknow packet!!-------\n");
			break;
	}

	printf("############################\n");
	printhex("packet hexstream: ", pkt, PACKET_LENGTH(pkt), "\n");
	printf("############################\n");

	printf("\n\n");
}
