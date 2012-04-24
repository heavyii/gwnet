/*
 * packet.h
 *
 * Copyright (C) 2012 crazyleen <ruishenglin@126.com>
 * 
 */
 
#ifndef __PACKET_H__
#define	__PACKET_H__

/* 
 * 802.1x packet for Southern Medical University
 * 港湾网络 802.1x Linux客户端
 */


#define ETH_TYPE					0x888e
#define VERSION						0x01

// define pkt_type
#define PKT_TYPE_EAP				0x00
#define PKT_TYPE_START				0x01
#define PKT_TYPE_LOGOFF				0x02

//define body length
#define BODY_LENGTH_START			0x0000
#define BODY_LENGTH_LOGOFF			0x0000
/*  BODY_LENGTH_IDENTIFY_BASE + strlen(username) */
#define BODY_LENGTH_IDENTIFY_BASE	0x0005	
/*  BODY_LENGTH_IDENTIFY_BASE + strlen(username) */
#define BODY_LENGTH_CHALLENGE_BASE	0x0016

// BODY CODE
#define BODY_CODE_REQUEST 		0x01
#define BODY_CODE_RESPONSE		0x02
#define BODY_CODE_SUCCESS		0x03
#define BODY_CODE_FAIL			0x04

//BODY TYPE
#define BODY_TYPE_IDENTIFY		0x01
#define BODY_TYPE_CHALLENGE		0x04

#define PACKET_CHALLENGE_VELUE_SIZE			16
#define USER_NAME_LEN_MAX	24

//practical packet length
#define PACKET_LENGTH(pkt)	(18 + ((int)(pkt)->bodylength[0] << 8) + (pkt)->bodylength[1])

// define packet struct
typedef struct _PACKET
{
	__uint8_t	dstmac[6];	
	__uint8_t	srcmac[6];	

	__uint8_t	eth_type[2]; 	//0x88, 0x8e  	:Type: 802.1x authentication
	__uint8_t	version; 	//0x01			:Version: v1(protocal version)
	__uint8_t	pkt_type; 	//start(0x01), logoff(0x02), EAP packet(0x00)
	__uint8_t	bodylength[2]; //
	union		// packet body (EPA packet body)
	{
		// test 
		struct
		{
			__uint8_t  	code;
			__uint8_t	id;	 
			__uint8_t	bodylength[2];
			__uint8_t	type;
		}__attribute__((__packed__)) test;

		// request identify
		struct
		{
			__uint8_t  	code;//code: request (0x01)
			__uint8_t	id;	 //id:1
			__uint8_t	bodylength[2];
			__uint8_t	type;//Type: identity[rfc3748] (1)
		}__attribute__((__packed__)) request_identify;
	
		//identify
		struct
		{
			__uint8_t  	code;//code:response(0x02)
			__uint8_t	id;	 //id:1
			__uint8_t	bodylength[2];
			__uint8_t	type;//Type: identity[rfc3748] (1)
			__uint8_t	username[USER_NAME_LEN_MAX];//identity: ???（用户名）
		}__attribute__((__packed__)) identify;

		// Request, MD5-Challenge [RFC3748]
		struct
		{
			__uint8_t  	code;//code: request (0x01)
			__uint8_t	id;	 //id:2
			__uint8_t	bodylength[2];
			__uint8_t	type;//0x04,  Type:  MD5-Challenge [RFC3748] (4)
			__uint8_t	value_size;//0x10,	Value-Size: 16
			__uint8_t	value[PACKET_CHALLENGE_VELUE_SIZE];//Value: F703914F6C0D6B6AB79F00A586585CB0
		}__attribute__((__packed__)) request_challenge;

		// Response, MD5-Challenge [RFC3748]
		struct
		{
			__uint8_t  	code;//0x02, //Code: Response (0x02)
			__uint8_t	id;	 //id:2
			__uint8_t	bodylength[2];
			__uint8_t	type;//Type:  MD5-Challenge [RFC3748] (4)
			__uint8_t	value_size;//0x10,	Value-Size: 16
			__uint8_t	value[PACKET_CHALLENGE_VELUE_SIZE];//Value: F703914F6C0D6B6AB79F00A586585CB0
			__uint8_t	username[USER_NAME_LEN_MAX];//identity: ???（用户名）
		}__attribute__((__packed__)) challenge;

		// Success
		struct
		{
			__uint8_t  	code;//Code: Success (0x03)
			__uint8_t	id;	 //id:0
			__uint8_t	bodylength[2];//length 4
		}__attribute__((__packed__)) success;

		// fail
		struct
		{
			__uint8_t  	code;//Code: failed (0x04)
			__uint8_t	id;	 //id:0
			__uint8_t	bodylength[2];//length 4
		}__attribute__((__packed__)) fail;
//*/		
	}__attribute__((__packed__)) body;
}PACKET;

//packet_type tell what type of packet 
#define PACKET_UNKNOW	-1
#define PACKET_START	0
#define PACKET_LOGOFF	1
#define PACKET_SUCCESS	3
#define PACKET_FAIL		4
#define PACKET_REQUEST_ID	5
#define PACKET_REQUEST_CH	6
#define PACKET_RESPONSE_ID	7
#define PACKET_RESPONSE_CH	8
	
/**
 * packet_type - tell what type of packet 
 * @pkt:	PACKET pointer
 * @return:	return packet type macro, see packet.h
 */
extern int packet_type(PACKET	*pkt);

extern int packet_length(PACKET *pkt);

extern PACKET *packet_get_start(PACKET	*pkt, char *srcmac);
extern PACKET *packet_get_identify(PACKET	*pkt, char *srcmac, char *dstmac, char *username);

/**
 * packet_get_challenge - make a challenge packet
 * @pkt:	where to store packet dataoff
 * @src_pkt:request Challenge packet from server
 * @return: return pkt, or null
 */
extern PACKET *packet_get_challenge(PACKET *pkt, PACKET *src_pkt, char *srcmac, char *dstmac,
		char *username, char *password);

extern PACKET *packet_get_logoff(PACKET	*pkt, char *srcmac);
#endif /* __PACKET_H__ */

