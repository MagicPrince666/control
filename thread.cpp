/**********************************************\

                   _ooOoo_
                  o8888888o
                  88" . "88
                  (| -_- |)
                  O\  =  /O
               ____/`---'\____
             .'  \\|     |//  `.
            /  \\|||  :  |||//  \
           /  _||||| -:- |||||-  \
           |   | \\\  -  /// |   |
           | \_|  ''\---/''  |   |
           \  .-\__  `-`  ___/-. /
         ___`. .'  /--.--\  `. . __
      ."" '<  `.___\_<|>_/___.'  >'"".
     | | :  `- \`.;`\ _ /`;.`/ - ` : | |
     \  \ `-.   \_ __\ /__ _/   .-` /  /
======`-.____`-.___\_____/___.-`____.-'======
                   `=---='
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

           佛祖保佑  小王子   永无BUG
				               						
\********************************************/
#include "thread.h"
#include "http.h"       //http通信
#include "md5.h"        //签名算法
#include "uart.h"   //串口
#include "uart_cdc.h"
#include "tcp_server.h" //控制接口
#include "qxwz_rtcm.h"  //差分定位
#include "qxwz.h"
//#include "server.h"     //文件服务 作为OTA升级接口
#include <signal.h>
#include <errno.h> 
#include <execinfo.h>

pthread_t pthread_id[8];//线程ID
pthread_mutex_t mut;//声明互斥变量

char mac[] = "000c292f963d";//MAC地址

/****crash handler begin******/
static void _signal_handler(int signum)  
{  
    void *array[10];  
    size_t size;  
    char **strings;  
    size_t i;  
  
    signal(signum, SIG_DFL); /* 还原默认的信号处理handler */  
  
    size = backtrace (array, 10);  
    strings = (char **)backtrace_symbols (array, size);  
  
    fprintf(stderr, "widebright received SIGSEGV! Stack trace:\n");  
    for (i = 0; i < size; i++) {  
        fprintf(stderr, "%d %s \n",i,strings[i]);  
    }  
      
    free (strings);  
    exit(1);  
}  
int tcp_control = 1;
static void sigint_handler(int sig)
{
    tcp_control = 0;
    printf("-----@@@@@ sigint_handler  is over !\n");
    
}
//#define TEST_ONE

int main (int argc, char **argv) 
{
#ifdef CROSS_COMPILE
//    get_MAC(mac,(char*)"eth0");//获取MAC地址; 
#else
    //get_MAC(mac,(char*)"ens33");//获取MAC地址
#endif

    signal(SIGPIPE, _signal_handler);    // SIGPIPE，管道破裂。
    signal(SIGSEGV, _signal_handler);    // SIGSEGV，非法内存访问
    signal(SIGFPE, _signal_handler);     // SIGFPE，数学相关的异常，如被0除，浮点溢出，等等
    signal(SIGABRT, _signal_handler);    // SIGABRT，由调用abort函数产生，进程非正常退出
    signal(SIGINT, sigint_handler);//信号处理
    pthread_mutex_init(&mut,NULL);

#ifdef TEST_ONE

    if (pthread_create(&pthread_id[0], NULL, rtcm_gps, NULL))
        printf("Create rtcm_gps error!\n");
    if(pthread_id[0] !=0) {                   
            pthread_join(pthread_id[0],NULL);
            printf("rtcm_gps %ld exit!\n",pthread_id[0]);
    }
 
#else
    
    Init_Tcp_Server();

    // pid_t pid;
    // pid = fork();  //守护进程 后台运行
    // if(pid < 0)  
    // {  
    //     perror("fork error!");  
    //     exit(1);  
    // }  
    // else if(pid > 0) //父进程退出 
    // {  
    //     exit(0);  
    // }  

    if(pthread_create(&pthread_id[0], NULL, Uart_Modules, NULL))
        printf("Create Uart_Modules error!\n");
    if(pthread_create(&pthread_id[1], NULL, Tcp_server, NULL))
        printf("Create Tcp_server error!\n");
    if(pthread_create(&pthread_id[2], NULL, Input_Modules, NULL))
        printf("Create Input_Modules error!\n");
    if(pthread_create(&pthread_id[3], NULL, Hart_Modules, NULL))
        printf("Create Hart_Modules error!\n");
    if(pthread_create(&pthread_id[4], NULL, rtcm_gps, NULL))
        printf("Create rtcm_gps error!\n");
    if(pthread_create(&pthread_id[5], NULL, Thread_acm0, NULL))
        printf("Create Thread_acm0 error!\n");
    if(pthread_create(&pthread_id[6], NULL, Thread_acm1, NULL))
        printf("Create Thread_acm1 error!\n");
    if(pthread_create(&pthread_id[7], NULL, post_to_http, NULL))
        printf("Create post_to_http error!\n");

    if(pthread_id[0] !=0) {                   
        pthread_join(pthread_id[0],NULL);
        printf("Uart_Modules exit%ld!\n",pthread_id[0]);
    }  
    if(pthread_id[1] !=0) {                   
        pthread_join(pthread_id[1],NULL);
        printf("Tcp_server exit%ld!\n",pthread_id[1]);
    }   
    if(pthread_id[2] !=0) {                   
        pthread_join(pthread_id[2],NULL);
        printf("Input_Modules exit%ld!\n",pthread_id[2]);
    } 
    if(pthread_id[3] !=0) {                   
        pthread_join(pthread_id[3],NULL);
        printf("Hart_Modules exit%ld!\n",pthread_id[3]);
    } 
    if(pthread_id[4] !=0) {                   
        pthread_join(pthread_id[4],NULL);
        printf("rtcm_gps exit%ld!\n",pthread_id[4]);
    }
    if(pthread_id[5] !=0) {                   
        pthread_join(pthread_id[5],NULL);
        printf("Thread_acm0 exit%ld!\n",pthread_id[5]);
    }    
    if(pthread_id[6] !=0) {                   
        pthread_join(pthread_id[6],NULL);
        printf("Thread_acm1 exit%ld!\n",pthread_id[6]);
    }
    if(pthread_id[7] !=0) {                   
        pthread_join(pthread_id[7],NULL);
        printf("post_to_http exit%ld!\n",pthread_id[7]);
    }     

#endif

    return 0; 
}
