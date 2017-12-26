#include "uart.h"

int speed_arr[] = { 
	B921600, B460800, B230400, B115200, B57600, B38400, B19200, 
	B9600, B4800, B2400, B1200, B300, 
};
int name_arr[] = {
	921600, 460800, 230400, 115200, 57600, 38400,  19200,  
	9600,  4800,  2400,  1200,  300, 
};

void set_speed(int fd, int speed)         //设置波特率
{
	unsigned int   i;
	int   status;
	struct termios   Opt;

	tcgetattr(fd, &Opt);                 //保存原来的串口配置

	for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++) {
		if  (speed == name_arr[i]){
			tcflush(fd, TCIOFLUSH);      //刷新缓冲区
			cfsetispeed(&Opt, speed_arr[i]);	
			cfsetospeed(&Opt, speed_arr[i]);  //设置输入输出波特率
			status = tcsetattr(fd, TCSANOW, &Opt);  //更改立即发生
			if (status != 0)
				perror("tcsetattr fd");
			return;
		}
		tcflush(fd,TCIOFLUSH);
	}

        if (i == 12){
                printf("\tSorry, please set the correct baud rate!\n\n");
               //print_usage(stderr, 1);
        }
}
/*
	*@brief   设置串口数据位，停止位和校验位
	*@param  fd   类型  int 打开串口文件句柄 
	*@param  databits 类型  int 数据位 取值为7或8
	*@param  stopbits 类型  int 停止位 取值为1或2
	*@param  parity  类型 int  校验类型 取值为N,E,O,,S
*/
int set_Parity(int fd,int databits,int stopbits,int parity)   //配置串口
{
	struct termios options;

	if  ( tcgetattr( fd,&options)  !=  0) {
		perror("SetupSerial 1");
		return(FALSE);
	}
	
	options.c_cflag &= ~CSIZE ;
	options.c_oflag = 0;
	switch (databits) /*设置数据位数*/ {
	case 7:
		options.c_cflag |= CS7;
		break;
	case 8:
		options.c_cflag |= CS8;
		break;
	default:
		fprintf(stderr,"Unsupported data size\n");
		return (FALSE);
	}
	
	switch (parity) {
	case 'n':
	case 'N':
		options.c_cflag &= ~PARENB;   /* Clear parity enable */
		options.c_iflag &= ~INPCK;     /* Enable parity checking */
		break;
	case 'o':
	case 'O':
		options.c_cflag |= (PARODD | PARENB);  /* 设置奇偶校验 */
		options.c_iflag |= INPCK;             /* Disnable parity checking */
		break;
	case 'e':
	case 'E':
		options.c_cflag |= PARENB;     /* Enable parity */
		options.c_cflag &= ~PARODD;   /* 转换为偶校验 */ 
		options.c_iflag |= INPCK;       /* Disnable parity checking */
		break;
	case 'S':	
	case 's':  /*as no parity*/ 
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		break;
	default:
		fprintf(stderr,"Unsupported parity\n");
		return (FALSE);
	}
	/* 设置停止位 */  
	switch (stopbits) {
	case 1:
		options.c_cflag &= ~CSTOPB;
		break;
	case 2:
		options.c_cflag |= CSTOPB;
		break;
	default:
		fprintf(stderr,"Unsupported stop bits\n");
		return (FALSE);
	}
	/* Set input parity option */
//	if (parity != 'n')
//		options.c_iflag |= INPCK;
	options.c_iflag &= ~(IXON|IXOFF|IXANY|IGNCR|ICRNL|INLCR|BRKINT|IGNPAR|IMAXBEL|IUCLC|PARMRK|IGNBRK|INPCK|ISTRIP);
    options.c_oflag &= ~(BSDLY|CRDLY|FFDLY|NLDLY|OFDEL|OFILL|OLCUC|ONLRET|ONOCR|OPOST|OCRNL|ONLCR);
	options.c_oflag |= OPOST;
	options.c_cc[VTIME] = 0;                   // 100ms 超时
	options.c_cc[VMIN] = 0;                    // 至少读 个字节
	 
	options.c_lflag &= ~(ISIG|ICANON|XCASE|ECHO|ECHOE|ECHOK|ECHONL|ECHOPRT|TOSTOP);      //设置非规范模式

	tcflush(fd,TCIFLUSH);  // Update the options and do it NOW
	if (tcsetattr(fd,TCSANOW,&options) != 0) {
		perror("SetupSerial 3");
		return (FALSE);
	}
	return (TRUE);
}


int acm_open(char *str)
{	
	int fd=0;
	fd = open(str,O_RDWR | O_NOCTTY );//非阻塞 O_RDWR | O_NOCTTY | O_NDELAY
	int speed = 115200;
	
	if (fd > 0) {
                set_speed(fd, speed);           
        } else return -1;

	 if (set_Parity(fd,8,1,'N')== FALSE) {  //配置串口
                fprintf(stderr, "Set Parity Error\n");
                close(fd);
                return -1;
        }

        tcflush(fd, TCIOFLUSH);              //输出
	return fd;	
}


int acm_send(int fd, char *buff, int len)
{
	int count=0;
	count = write(fd, (unsigned char *)buff, len);	
	if(count == len)	
		return 0;
	else
		return -1;
}

int acm_recv(int fd, char *buff,int len)
{
	int count = 0;
	count = read(fd, buff, len);
//	printf("uart get %d buf\n",count);
	return count;
}

int acm_close(int fd)
{
	close(fd);
	return 0;
}

int monitor_routine( int  uart_fd)
{
    int i; 
    char rebuff[1024];//设置最大的数据长度为8个
    memset(rebuff,0,sizeof(char)*1024);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000;
    
    while(1)
    {
		fd_set rfds;
		int retval=0;
		
		FD_ZERO(&rfds);
		FD_SET(uart_fd, &rfds);
		
		retval = select(uart_fd + 1, &rfds, NULL, NULL, &tv);
		if(retval<0) 
			perror("select error\n");  
		else if( retval && FD_ISSET(uart_fd, &rfds))
		{  
			printf("FD_ISSET!\n");
			int rc=read(uart_fd, rebuff, 1024);
			if(rc>0)
			{
				for(i = 0; i < 8; i++)
					printf(" %x ", rebuff[i]);
				printf("\n");
			}    
		}    
    }
    return 0;
}