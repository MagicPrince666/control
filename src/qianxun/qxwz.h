#ifndef __QXWZ_H
#define __QXWZ_H

#include "qxwz_rtcm.h"

#undef QXLOGI
#define QXLOGI printf

//extern qxwz_rtcm get;
//extern char rtcm_flag;
extern char appkey[10];
extern char appSecret[80];
extern char deviceId[32];
extern char deviceType[32];
extern int rtcm_contrl;
extern unsigned char rtcm_status[3];

extern "C"
{
//void get_MAC(char *mac,char *net);
void qxwz_rtcm_response_callback(qxwz_rtcm data);
void qxwz_status_response_callback(qxwz_rtcm_status code);
void get_qxwz_sdk_account_info(void);
void test_qxwz_rtcm_start_stop(void);
void getAccountExpireDate(void);
}
void *rtcm_gps (void *arg);
#endif