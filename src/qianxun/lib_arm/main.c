#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#include "qxwz_rtcm.h"
//#include "qxlog.h"

#undef QXLOGI
#define QXLOGI printf

//#define _QXWZ_TEST_START_STOP


qxwz_account_info *p_account_info = NULL;
void  get_qxwz_sdk_account_info(void);


void qxwz_rtcm_response_callback(qxwz_rtcm data){
    //printf("QXWZ_RTCM_DATA:%s\n",data.buffer);
    QXLOGI("QXWZ_RTCM_DATA:%s\n",data.buffer);
    QXLOGI("QXWZ_RTCM_DATA:%ld\n",data.length);
}


void qxwz_status_response_callback(qxwz_rtcm_status code){
    //printf("QXWZ_RTCM_STATUS:%d\n",code);
    QXLOGI("QXWZ_RTCM_STATUS:%d\n",code);
	struct tm *ptr = NULL;
	//test account expire
	if(code == QXWZ_STATUS_OPENAPI_ACCOUNT_TOEXPIRE){
		get_qxwz_sdk_account_info();
	}
}

void  get_qxwz_sdk_account_info(void)
{
	p_account_info = getqxwzAccount();
	if(p_account_info->appkey != NULL) {
		printf("appkey=%s\n",p_account_info->appkey);
	}
	if(p_account_info->deviceID != NULL) {
		printf("deviceID=%s\n",p_account_info->deviceID);
	}
	if(p_account_info->deviceType != NULL) {
		printf("deviceType=%s\n",p_account_info->deviceType);
	}

	if(p_account_info->NtripUserName != NULL) {
		printf("NtripUserName=%s\n",p_account_info->NtripUserName);
	}
	if(p_account_info->NtripPassword != NULL) {
		printf("NtripPassword=%s\n",p_account_info->NtripPassword);
	}
	printf("expire_time=%d\n",p_account_info->expire_time);
}



//void getAccountExpireDate(void);


#ifdef _QXWZ_TEST_START_STOP
pthread_t qxwz_rtcm_test;
void test_qxwz_rtcm_start_stop(void);
#endif


int main(int argc, const char * argv[]) {
    //设置appKey和appSecret
    //apapKey申请详细见说明文档
    qxwz_config config;
    //RTD	
    config.appkey="20554";
    config.appSecret="3f0e7f9430a6cbb60e48f6b52a01e51f899dfbb37e61c2f6b040a2554a6d4973";
    config.deviceId="00:0c:29:2f:96:3d";
    config.deviceType="记录仪设备";

    qxwz_setting(&config);
    //启动rtcm sdk
    qxwz_rtcm_start(qxwz_rtcm_response_callback,qxwz_status_response_callback);

	#ifdef _QXWZ_TEST_START_STOP
    pthread_create(&qxwz_rtcm_test,NULL,test_qxwz_rtcm_start_stop,NULL);
	#endif
    //demo测试10秒发送gga
    //每秒发送gga以获取最新的rtcm数据流
    int i;
    for (i = 0; i < 120; i++) {
        qxwz_rtcm_sendGGAWithGGAString("$GPGGA,000001,3112.518576,N,12127.901251,E,1,8,1,0,M,-32,M,3,0*4B\r\n");
        //printf("Send GGA done\r\n");
        QXLOGI("Send GGA done\r\n");
		//getAccountExpireDate();
		get_qxwz_sdk_account_info();
        sleep(1);
    }
    QXLOGI("qxwz_rtcm_stop here\r\n");
//    //关闭rtcm sdk
    qxwz_rtcm_stop();
    QXLOGI("qxwz_rtcm_stop done\r\n");
    return 0;
}
#if 0
void getAccountExpireDate(void)
{
	struct tm *ptr = NULL;
	expire_time = qxwz_get_account_expire_time();
	//printf("expire_time=%d,date=",expire_time.expire_time);
	QXLOGI("expire_time=%d,date=",expire_time.expire_time);
	ptr = &expire_time.expire_date;
		
	QXLOGI("year:%d,month:%d,mday:%d,hour:%d,minute:%d,second:%d\n", \
		ptr->tm_year+1900,ptr->tm_mon+1,ptr->tm_mday,ptr->tm_hour,ptr->tm_min,ptr->tm_sec);
}
#endif

#ifdef _QXWZ_TEST_START_STOP
void test_qxwz_rtcm_start_stop(void)
{
	//sleep(2);
	qxwz_rtcm_stop();
    while(1)
    {
    	qxwz_rtcm_start(qxwz_rtcm_response_callback,qxwz_status_response_callback); 
		sleep(50);
	    time_t time_stop_begin = time(NULL);
	    qxwz_rtcm_stop();
	    time_t time_stop_end = time(NULL);
	    QXLOGI("time_stop_begin:%d,time_stop_end:%d\n",time_stop_begin,time_stop_end);
		sleep(1);
    }
}
#endif


