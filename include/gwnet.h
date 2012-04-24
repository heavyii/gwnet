/*
 * gwnet.h
 *
 * Copyright (C) 2011 crazyleen <ruishenglin@126.com>
 *
 */
#ifndef __GWNET_H__
#define __GWNET_H__



#define PKT_Trace(...)   do { printf("%s:%u:TRACE: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf(" [%s]\n", __FUNCTION__); } while (0);
#define PKT_Debug(...);  do { printf("%s:%u:DEBUG: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf(" [%s]\n", __FUNCTION__); } while (0);
#define PKT_Info(...);  do{ printf("%s:%u:INFO: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf(" [%s]\n", __FUNCTION__); } while (0);
#define PKT_Error(...); do { printf("%s:%u:ERROR: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf(" [%s]\n", __FUNCTION__); } while (0);


#endif /* _GWNET_H_ */

