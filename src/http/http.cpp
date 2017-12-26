/***************************************************************

 *File : http.c
 *Auth : leo 
 *Date : 20170520
 *Mail : 846863428@qq.com 
 
***************************************************************/    
#include <stdio.h>  
#include <stdlib.h>  
#include <arpa/inet.h>  
#include <netdb.h>  
#include <string.h>  
#include <sys/socket.h>
#include <stdint.h>
#include <unistd.h>
      
#include "http.h"  
#include "md5.h"
#include "uart_cdc.h"
#include "../../thread.h"

#define BUFFER_SIZE 1024  


#define HTTP_GET  "GET /%s HTTP/1.1\r\nHOST: %s:%d\r\nAccept: */*\r\n\r\n"
#define F06_POST_GPS "POST /%s HTTP/1.1\r\nHOST: %s:%d\r\nAccept: */*\r\n\r\n"

      
static int http_tcpclient_create(const char *host, int port){  
    struct hostent *he;  
    struct sockaddr_in server_addr;   
    int socket_fd;  
      
    if((he = gethostbyname(host))==NULL){  
        return -1;     
     }  
      
    server_addr.sin_family = AF_INET;  
    server_addr.sin_port = htons(port);  
    server_addr.sin_addr = *((struct in_addr *)he->h_addr);  
   
    if((socket_fd = socket(AF_INET,SOCK_STREAM,0))==-1){  
       return -1;      
    }  
      
    if(connect(socket_fd, (struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1){  
         return -1;      
    }  
      
        return socket_fd;  
}  
      
static void http_tcpclient_close(int socket){  
        close(socket);  
}  
      
static int http_parse_url(const char *url,char *host,char *file,int *port)  
{  
    char *ptr1,*ptr2;  
    int len = 0;  
    if(!url || !host || !file || !port){  
        return -1;  
    }  
      
    ptr1 = (char *)url;  
  
    if(!strncmp(ptr1,"http://",strlen("http://"))){  
        ptr1 += strlen("http://");  
    }else{  
        return -1;  
    }  
      
    ptr2 = strchr(ptr1,'/');  
    if(ptr2){  
        len = strlen(ptr1) - strlen(ptr2);  
        memcpy(host,ptr1,len);  
        host[len] = '\0';  
        if(*(ptr2 + 1)){  
            memcpy(file,ptr2 + 1,strlen(ptr2) - 1 );  
            file[strlen(ptr2) - 1] = '\0';  
        }  
    }else{  
        memcpy(host,ptr1,strlen(ptr1));  
        host[strlen(ptr1)] = '\0';  
    }  
    //get host and ip  
    ptr1 = strchr(host,':');  
    if(ptr1){  
        *ptr1++ = '\0';  
        *port = atoi(ptr1);  
    }else{  
        *port = MY_HTTP_DEFAULT_PORT;  
    }  
      
    return 0;  
}  
      
      
static int http_tcpclient_recv(int socket,char *lpbuff){  
    int recvnum = 0;  
      
    recvnum = recv(socket, lpbuff,BUFFER_SIZE*4,0);  
      
    return recvnum;  
}  
      
static int http_tcpclient_send(int socket,char *buff,int size){  
    int sent=0,tmpres=0;  
      
    while(sent < size){  
        tmpres = send(socket,buff+sent,size-sent,0);  
        if(tmpres == -1){  
            return -1;  
        }  
        sent += tmpres;  
    }  
    return sent;  
}  
      
static char *http_parse_result(const char*lpbuf)  
{  

    char *ptmp = NULL;   
    char *response = NULL;

    ptmp = (char*)strstr(lpbuf,"HTTP/1.1");  
    if(!ptmp){  
        printf("http/1.1 not faind\n");  
        return NULL;  
    }  

    if(atoi(ptmp + 9)!=200){  //在这个地方出错
        printf("result:%s\n",lpbuf);  
        return NULL;  
    }  
      
    ptmp = (char*)strstr(lpbuf,"\r\n\r\n");  
    if(!ptmp){  
        printf("ptmp is NULL\n");  
        return NULL;  
    } 

    response = (char *)malloc(strlen(ptmp)+1);  
    if(!response){  
        printf("malloc failed \n");  
        return NULL;  
    }  
    strcpy(response,ptmp+4);  

    return response;  
}  

#define LI

char * f06_post(const char *url,const unsigned char *post_buf ,long leng ,char * filename)
{       
    int socket_fd = -1;  
    char lpbuf[BUFFER_SIZE] = {'\0'};   
    char host_addr[BUFFER_SIZE] = {'\0'};  
    char file[BUFFER_SIZE] = {'\0'};  
    int port = 0;  
    int len=0;  
    unsigned char *response = NULL; 
      
    if(!url || !post_buf){  
        printf("      failed!\n");  
        return NULL;  
    }  
      
    if(http_parse_url(url,host_addr,file,&port)){  
        printf("http_parse_url failed!\n");  
         return NULL;  
    }  
//    printf("host_addr:%s\nfile:%s\n,port:%d\n",host_addr,file,port);  
      
    socket_fd = http_tcpclient_create(host_addr,port);  
    if(socket_fd < 0){  
        printf("http_tcpclient_create failed\n");  
        return NULL;  
    } 
#ifdef LI
//curl -F 'file=@./test.jpg' -s -v 'http://119.147.36.42:7084/F06/UploadOne?mac=000c292f963d&time=1494405717&ver=v0.4&mchid=junchentech&platform=F06&cipher=uvBo6JBR&sign=9CE9E36A1F235EE98F1CA360AA4C6BD5&channel=10&dev_id=400&member_id=63&msg_id=IG06R4Pp&md5=368deee43b25b1e0078ebbec9c07e2fe&type=jpg&shoot_time=1490065398&size=46553' 
    char *head_buf = (char *)malloc(1024);
    char *body_buf = (char *)malloc(900*1024);
    //http body头部：
    int body_len = 0;
    body_len += sprintf(body_buf, "--75fc6c\r\n");
    body_len += sprintf(body_buf + body_len, "Content-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n", filename);
    body_len += sprintf(body_buf + body_len, "Content-Type: image/jpeg\r\n\r\n");
    printf("%s",body_buf);
    memcpy(body_buf + body_len,post_buf,leng);
    body_len += leng;
    body_len += sprintf(body_buf + body_len, "\r\n--75fc6c--\r\n"); 
    printf("\r\n--75fc6c--\r\n");  

    //http头部： 
    int head_len = 0;
    head_len = sprintf(head_buf, "POST /%s HTTP/1.1\r\n", file);
    head_len += sprintf(head_buf + head_len, "HOST: %s:%d\r\n", host_addr, port);
    head_len += sprintf(head_buf + head_len, "Accept: */*\r\n");
    head_len += sprintf(head_buf + head_len, "Content-Length: %d\r\n", body_len);
    head_len += sprintf(head_buf + head_len, "Content-Type: multipart/form-data; boundary=75fc6c\r\n\r\n");
    printf("%s",head_buf);

    response = (unsigned char *)malloc(head_len + body_len);
    memcpy(response,head_buf,head_len);//拷贝协议头
    memcpy(response+head_len,body_buf,body_len);
    free(head_buf);
    free(body_buf);
    len = http_tcpclient_send(socket_fd,(char *)response,head_len + body_len);//开始post

#else 
    char *body_buf = malloc(900*1024);
    //http body头部：
    int body_len = 0;
    body_len += sprintf(body_buf, "--75fc6c\r\n");
    body_len += sprintf(body_buf + body_len, "Content-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n", filename);
    body_len += sprintf(body_buf + body_len, "Content-Type: image/jpeg\r\n");
    body_len += sprintf(body_buf + body_len, "\r\n");
    memcpy(body_buf + body_len,post_buf,leng);
    body_len += leng;
    body_len += sprintf(body_buf + body_len, "\r\n--75fc6c--\r\n"); 

    sprintf(lpbuf,HTTP_POST,file,host_addr,port, body_len, filename); 
    printf("%s",lpbuf);
    response = malloc(strlen(lpbuf) + body_len);  
    memcpy(response,lpbuf,strlen(lpbuf));//拷贝协议头
    memcpy(response+strlen(lpbuf),body_buf,body_len);   //lpbuf指向内存和post_str拼接到一起
    len = http_tcpclient_send(socket_fd,response,strlen(lpbuf) + body_len);//开始post
    free(body_buf);
#endif
    
    if(len < 0){  
        printf("http_tcpclient_send failed..\n");  
        return NULL;  
    }
    else printf("post len:%d\n",len);
    free(response);   
      
    /*it's time to recv from server*/ 
    memset(lpbuf,'\0',BUFFER_SIZE); 
    if(http_tcpclient_recv(socket_fd,lpbuf) <= 0){  //接收返回信息
        printf("http_tcpclient_recv failed\n");  
        return NULL;  
    }  
      
    http_tcpclient_close(socket_fd);  

    return http_parse_result(lpbuf);  
}

char * http_get(const char *url)  
{        
    int socket_fd = -1;  
    char lpbuf[BUFFER_SIZE] = {'\0'};    
    char host_addr[BUFFER_SIZE] = {'\0'};  
    char file[BUFFER_SIZE] = {'\0'};  
    int port = 0;  
    int len=0;  
      
    if(!url){  
        printf("      failed!\n");  
        return NULL;  
    }  
      
    if(http_parse_url(url,host_addr,file,&port)){  
        printf("http_parse_url failed!\n");  
        return NULL;  
    }  
//    printf("host_addr:%s\tfile:%s\t,%d\n",host_addr,file,port);  
      
    socket_fd =  http_tcpclient_create(host_addr,port);  
    if(socket_fd < 0){  
        printf("http_tcpclient_create failed\n");  
        return NULL;  
    }  
      
    sprintf(lpbuf,HTTP_GET,file,host_addr,port);  
    //printf("http url:%s",lpbuf);
      
    if(http_tcpclient_send(socket_fd,lpbuf,strlen(lpbuf)) < 0){  
        printf("http_tcpclient_send failed..\n");  
        return NULL;  
    }  
    printf("send:\n%s\n",lpbuf); 

    memset(lpbuf,'\0',BUFFER_SIZE); 
    len = http_tcpclient_recv(socket_fd,lpbuf) ; 
    if(len <= 0){  
        printf("http_tcpclient_recv failed\n");  
       return NULL;  
    }  
    http_tcpclient_close(socket_fd); 

    return http_parse_result(lpbuf); 
}  

char * post_gps(const char *url)
{      
    int socket_fd = -1;  
    char lpbuf[BUFFER_SIZE] = {'\0'};  
    char host_addr[BUFFER_SIZE] = {'\0'};  
    char file[BUFFER_SIZE] = {'\0'};  
    int port = 0;   
   
    if(http_parse_url(url,host_addr,file,&port)){  
        printf("http_parse_url failed!\n");  
         return NULL;  
    }  
    //printf("host_addr:%s\nfile:%s\nport:%d\n",host_addr,file,port);  
      
    socket_fd = http_tcpclient_create(host_addr,port);  //创建http客户端出错
    if(socket_fd < 0){  
        printf("http_tcpclient_create failed\n");  
        return NULL;  
    }  
    //printf("http_tcpclient_create sucess\n"); 
           
    sprintf(lpbuf,F06_POST_GPS,file,host_addr,port);  
    printf("%s",lpbuf); 
    if(http_tcpclient_send(socket_fd,lpbuf,strlen(lpbuf)) < 0){  
        printf("http_tcpclient_send failed..\n");  
        return NULL;  
    }  
    //printf("发送请求:\n%s\n",lpbuf);  
      
    /*it's time to recv from server*/  
    memset(lpbuf,'\0',BUFFER_SIZE);
    if(http_tcpclient_recv(socket_fd,lpbuf) <= 0){  
        printf("http_tcpclient_recv failed\n");  
        return NULL;  
    }       
    http_tcpclient_close(socket_fd); 

    return http_parse_result(lpbuf);  
}  


unsigned int shoot_time;         //拍摄时间戳 
time_t now;   //当前时间 
unsigned int dev_id = 0;         //设备 id 从f06/Init获得
const char *ver = "0.5";        //版本号
const char *mchid = "junchentech";//厂家ID
const char *platform = "F06";    //请求平台
const char *key = "12345678";    //厂商 key：根据不同厂商烧入硬件的密钥值， 数据库中有保存
unsigned int channel = 10;//渠道号（F06 项目 10）
unsigned char md[16] = {0}; 


int fd_gps = 0; 

void get_MAC(char *mac,char *net)
{
    struct   ifreq   ifreq; 
    int   sock;   
    
    if((sock=socket(AF_INET,SOCK_STREAM,0)) <0)
    { 
        perror( "socket"); 
    } 
    strcpy(ifreq.ifr_name,net); 
    if(ioctl(sock,SIOCGIFHWADDR,&ifreq) <0) 
    { 
        perror( "ioctl"); 
    } 
    sprintf( mac,"%02x%02x%02x%02x%02x%02x", 
            (unsigned  char)ifreq.ifr_hwaddr.sa_data[0], 
            (unsigned  char)ifreq.ifr_hwaddr.sa_data[1], 
            (unsigned  char)ifreq.ifr_hwaddr.sa_data[2], 
            (unsigned  char)ifreq.ifr_hwaddr.sa_data[3], 
            (unsigned  char)ifreq.ifr_hwaddr.sa_data[4], 
            (unsigned  char)ifreq.ifr_hwaddr.sa_data[5]);
}

void generate(int len,char* buffer)
{
    /*产生密码用的字符串*/
    static const char string[]= "0123456789abcdefghiljklnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int i = 0;
    for(; i < len; i++)
    {
        buffer[i] = string[rand()%strlen(string)]; /*产生随机数*/
    }
}

int get_from_http (void) 
{ 
    char *str = NULL;
    char *dev_str = NULL;
    char *cipher = NULL;//前端随机生成密码
    unsigned char *sign = NULL;//请求签名
    char * url = (char*)malloc(1024);    
    int  i,get_id = 0;
    char buf[33]={'\0'};
    char tmp[3]={'\0'};

    cipher = (char*)malloc(16);//前端随机生成密码
    sign = (unsigned char*)malloc(16);//请求签名
    
    time(&now);//获取当前时间 

    srand((int)time(0));  
    generate(8,cipher); //生成8位随机密码

    sprintf(url,"cipher=%s&key=%s&mac=%s&mchid=%s&platform=%s&time=%ld&ver=%s"\
    ,cipher,key,mac,mchid,platform,now,ver);

    MDString(url,md);   //加密字符串

    memcpy(sign, md, 16);
    
    for( i=0; i<16; i++ ){
        sprintf(tmp,"%02x", sign[i] );
        strcat(buf,tmp);
    }

    sprintf(url,"http://alpha.f06dev.bqlnv.com.cn/F06/Init?mac=%s&time=%ld&ver=%s&mchid=%s&platform=%s&cipher=%s&sign=%s&channel=%d"\
    ,mac,now,ver,mchid,platform,cipher,buf,channel);

    str = http_get(url);
    if(str == NULL)
    {
        printf("get error!\n");
        free(cipher);
        free(sign);
        free(url);
        return -1;
    }
    else{
        
        printf("%s",str);
       
        dev_str = (char*)strstr(str,"dev_id"); 
        if(dev_str != NULL)
        {
            get_id = atoi(dev_str + 8);//获取设备号  
            printf("dev_id = %d\n",get_id);
            free(cipher);
            free(sign);
            free(url);
            printf("qiut http get!\n");

            return get_id;
        }
        else
        {
            printf("get dev_id error!\n");   
            sleep (1);/*休息一秒，延长任务的执行时间*/ 
        }
        free(str);
        free(url);
        free(cipher);
        free(sign); 
    }
    
    return 0;    
} 

//MD5(cipher=?&key=?&mac=?&mchid=?& platform=?&time=?&ver=?)
void * post_to_http (void *arg) 
{ 
    MY_MD5_CTX ctx;   
    char *str = NULL;
    char *dev_str = NULL;
    char *cipher = NULL;//前端随机生成密码
    unsigned char *sign = NULL;//请求签名
    //int fd_pic;                      //图片文件句柄
    unsigned int member_id = 0;          //接收图片用户 id
    char * msg_id = NULL;                   //拍照消息指令 id
    unsigned char * md5 = (unsigned char*)malloc(16);                      //文件 MD5 值
    char * type = (char*)"jpg";             //文件类型支持：bmp、 jpg、 jpeg、 png、 3gp、 mp4
    char *size = (char*)malloc(16);//图片大小
    unsigned char *pic;                       //post body 文件内容，二进制，文件长度最大 900K
    int pic_leng = 0;
    char *url = (char*)malloc(1024);
    char *server_sn = (char*)malloc(32);//FA163E0EB151494471538295295400

    cipher = (char*)malloc(16);       //前端随机生成密码
    sign = (unsigned char*)malloc(16);//请求签名 MD5(cipher=?&dev_id=?&key=?&time=?&ver=?)

    char buf[33]={'\0'};
    char buf_file[33]={'\0'};
    char tmp[3]={'\0'};
    int i,j;

    while(tcp_control)
    {
        if(dat.status == 1)//从http获取信息后可以上传图片
        {
            for(i=0;i<3;i++)// 重试3回
            {
                //if(dev_id <= 0) dev_id =  get_from_http();
                if(dev_id <= 0) dev_id = 399;
                else break;
            }
            if(dev_id <= 0)
            {
                free(md5);
                free(size);
                free(server_sn);
                free(url);
                free(cipher);
                free(sign);
                return NULL;
            }
                     
            pic = (unsigned char *)malloc(dat.lenght);
            memcpy(pic,dat.date,dat.lenght);//拷贝图片到另一快内存
            pic_leng = dat.lenght;
            free(dat.date);//释放掉CDC存储图片的内存
            dat.status = 0;//清除图片上传标志位 可以重新接收       
            
            if(pic_leng < 900*1024)//小于900K 单次上传
            {
                time(&now);//获取当前时间 

                srand((int)time(0));  
                generate(8,cipher); //生成8位随机密码
                msg_id = filename;//文件名作为拍照指令ID
                member_id = rand() % 101;  //产生0-100的随机数
                memset(url, '\0', 1024);
                sprintf(url,"cipher=%s&dev_id=%d&key=%s&time=%ld&ver=%s"\
                        ,cipher,dev_id,key,now,ver);

                MDString(url,md);
                memcpy(sign, md, 16);

                MD5Init(&ctx);   //初始化一个MD5_CTX这样的结构体
                MD5Update(&ctx,pic,pic_leng);   //更新这块区域，防止引用非法数据
                MD5Final(md,&ctx);  //最后把ctx中的数据按照MD5算法生成16位的MD5码，存放到md中
                memcpy(md5, md, 16);          
            
                for( i=0; i<16; i++ ){
                    sprintf(tmp,"%02X", sign[i] );
                    strcat(buf,tmp);
                    sprintf(tmp,"%02x", md5[i] );
                    strcat(buf_file,tmp);
                }
                sprintf(size, "%d" ,pic_leng);
                sprintf(url,"http://alpha.f06dev.bqlnv.com.cn/F06/UploadOne?mac=%s&time=%ld&ver=%s&mchid=%s&platform=%s&cipher=%s&sign=%s&channel=%d&dev_id=%d&member_id=%d&msg_id=%s&md5=%s&type=%s&shoot_time=%d&size=%s"\
                ,mac,now,ver,mchid,platform,cipher,buf,channel,dev_id,member_id,msg_id,buf_file,type,shoot_time,size);
                memset(buf, '\0', 33);     //必须清楚数据 否则会导致数组越界
                memset(buf_file, '\0', 33);//必须清楚数据 否则会导致数组越界

                str = f06_post(url,pic,pic_leng,filename);             
                printf("get massage:\n%s",str);
                free(str);

                //close(fd_pic);
                free(pic);
                sleep (5);/*休息一秒，延长任务的执行时间*/
            }
            else //大于900K 需要分片上传
            {
                int partnum = 0;//片数
                int partlen = 800*1024;//一片800*1024字节
                unsigned char *piece = NULL;
                int remainder = 0;

                remainder = (pic_leng)%partlen;
                if(remainder != 0) partnum = (pic_leng)/partlen + 1;//一片1024byle 总共size/1024 片 最后一片不到1024byle
                else partnum = (pic_leng)/partlen;//刚好是个整数

                time(&now);//获取当前时间 

                srand((int)time(0));  
                generate(8,cipher); //生成8位随机密码
                msg_id = filename;
                member_id = rand() % 101;  //产生0-100的随机数
                memset(url, '\0', 1024);
                sprintf(url,"cipher=%s&dev_id=%d&key=%s&time=%ld&ver=%s"\
                        ,cipher,dev_id,key,now,ver);

                MDString(url,md);
                memcpy(sign, md, 16);

                MD5Init(&ctx);   //初始化一个MD5_CTX这样的结构体
                MD5Update(&ctx,pic,pic_leng);   //更新这块区域，防止引用非法数据
                MD5Final(md,&ctx);  //最后把ctx中的数据按照MD5算法生成16位的MD5码，存放到md中
                memcpy(md5, md, 16);        
            
                for( i=0; i<16; i++ ){
                    sprintf(tmp,"%02X", sign[i] );
                    strcat(buf,tmp);
                    sprintf(tmp,"%02x", md5[i] );
                    strcat(buf_file,tmp);
                }
                sprintf(size, "%d" ,pic_leng);
                sprintf(url,"http://alpha.f06dev.bqlnv.com.cn/F06/UploadInit?mac=%s&time=%ld&ver=%s&mchid=%s&platform=%s&cipher=%s&sign=%s&channel=%d&dev_id=%d&member_id=%d&msg_id=%s&md5=%s&partnum=%d&partlen=%d&size=%s&type=%s&shoot_time=%d"\
                ,mac,now,ver,mchid,platform,cipher,buf,channel,dev_id,member_id,msg_id,buf_file,partnum,partlen,size,type,shoot_time);
                memset(buf, '\0', 33);     //必须清楚数据 否则会导致数组越界
                memset(buf_file, '\0', 33);//必须清楚数据 否则会导致数组越界

                str = http_get(url);
                printf("%s",str);
                free(str);        

                dev_str = (char*)strstr(str,"server_sn"); 
                if(dev_str != NULL)
                { 
                    for(i=0; i< 32; i++)
                    {
                        if(*(dev_str+12+i) != '\"')
                            server_sn[i] = *(dev_str+12+i); 
                        else 
                        {
                            server_sn[i] = '\0';
                            break;
                        }
                    }
                    piece = (unsigned char *)malloc(partlen);//必须支持单片大小

                    for(j = 0; j < partnum ; j++) 
                    {
                        time(&now);//获取当前时间 

                        srand((int)time(0));  
                        generate(8,cipher); //生成8位随机密码
                        msg_id = filename;
                        member_id = rand() % 101;  //产生0-100的随机数
                        memset(url, '\0', 1024);
                        if(j < partnum - 1)
                        {
                            memcpy(piece , pic + j*partlen, partlen);          
                        }                           
                        else//最后一个包
                        {
                            memset(piece + remainder ,0, partlen - remainder);
                            memcpy(piece , pic + j*partlen, remainder);//把剩下不满partlen的包post上去               
                        } 
                        sprintf(url,"cipher=%s&dev_id=%d&key=%s&time=%ld&ver=%s"\
                                ,cipher,dev_id,key,now,ver);                

                        MDString(url,md);
                        memcpy(sign, md, 16);

                        MD5Init(&ctx);   //初始化一个MD5_CTX这样的结构体
                        if(j < partnum)
                            MD5Update(&ctx,piece,partlen);   //更新这块区域，防止引用非法数据
                        else 
                            MD5Update(&ctx,piece,remainder);   //更新这块区域，防止引用非法数据
                        MD5Final(md,&ctx);  //最后把ctx中的数据按照MD5算法生成16位的MD5码，存放到md中
                        memcpy(md5, md, 16);          
                    
                        for( i=0; i<16; i++ ){
                            sprintf(tmp,"%02X", sign[i] );
                            strcat(buf,tmp);
                            sprintf(tmp,"%02x", md5[i] );
                            strcat(buf_file,tmp);
                        }
                        sprintf(url,"http://alpha.f06dev.bqlnv.com.cn/F06/UploadPart?mac=%s&time=%ld&ver=%s&mchid=%s&platform=%s&cipher=%s&sign=%s&channel=%d&dev_id=%d&md5=%s&server_sn=%s&part=%d"\
                        ,mac,now,ver,mchid,platform,cipher,buf,channel,dev_id,buf_file,server_sn,j+1);
                        memset(buf, '\0', 33);     //必须清楚数据 否则会导致数组越界
                        memset(buf_file, '\0', 33);//必须清楚数据 否则会导致数组越界
                        if(j < partnum)
                            str = f06_post(url,piece,partlen,filename);
                        else
                            str = f06_post(url,piece,remainder,filename);
                        printf("get massage:\n%s",str);
                        free(str);
                        
                    }
                    free(piece);           
                    break; 
                }                           
                sleep (1);/*休息一秒，延长任务的执行时间*/
            }        
        } 
        else
        {
            sleep(1);
        }   
    }
    free(cipher);
    free(sign);
    free(url);
    free(size);
    free(server_sn);
    printf("post finish!\n");
    pthread_exit(NULL); 
}

int post_gps_to_http (char * gps) 
{ 

    char *cipher = NULL;//前端随机生成密码
    unsigned char *sign = NULL;//请求签名
    char * str = NULL;
    char * url = (char*)malloc(1024);
    unsigned int channel = 21;//渠道号（F14 项目 21）
    int  i;
    char buf[33] = {'\0'};
    char tmp[3] = {'\0'};

    cipher = (char*)malloc(16);//前端随机生成密码
    sign = (unsigned char*)malloc(16);//请求签名

    for(i=0;i<3;i++)
    {
        if(dev_id <= 0)dev_id = 399;
        //dev_id =  get_from_http();
        else break;
    }
    if(dev_id <= 0)
    {
        free(url);
        free(cipher);
        free(sign);
        return -1;
    }
    memset(cipher,'\0',16);
    memset(url,'\0',1024);
    time(&now);//获取当前时间 

    srand((int)time(0));  
    generate(8,cipher); //生成8位随机密码

    sprintf(url,"cipher=%s&dev_id=%d&key=%s&time=%ld&ver=%s"\
    ,cipher,dev_id,key,now,ver);

    MDString(url,md);   //加密字符串

    memcpy(sign, md, 16); 
    
    for( i=0; i<16; i++ ){
        sprintf(tmp,"%02x", sign[i] );
        strcat(buf,tmp);
    }

    sprintf(url,"http://alpha.f06dev.bqlnv.com.cn/F06/UploadGPS?mac=%s&time=%ld&ver=%s&mchid=%s&platform=%s&cipher=%s&sign=%s&channel=%d&dev_id=%d&coordinates_value=%s"\
    ,mac,now,ver,mchid,platform,cipher,buf,channel,dev_id,gps);

    str = post_gps(url);//上传GPS信息
    printf("post gps:\n%s\n",str);
    free(str);

    free(url);
    free(cipher);
    free(sign);
    return 0;    
}