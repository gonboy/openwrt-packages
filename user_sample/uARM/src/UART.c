/*
	文件：UART.c
	说明：串口控制函数源文件
	作者：SchumyHao
	版本：V02
	日期：2013.03.18
*/

/* 串口类型定义 ：标准串口GNR_COM；USB串口USB_COM；Arduino串口*/
/*#define GNR_COM*/
/*#define USB_COM*/
#define ACM_COM
//TODO：没有定义时的提示

/* 头文件 */
#include "UART.h"

/* 函数 */
/* 串口配置函数 */
int ConfigUart(t_uart* pUart, struct termios* pOldCfg){
	assert(IS_FD(pUart->Fd));
	assert(IS_BAUD_RATE(pUart->BaudRate));
	assert(IS_DATA_BITS(pUart->DataBits));
	assert(IS_STOP_BITS(pUart->StopBits));
	assert(IS_PARITY(pUart->Parity));
	
	struct termios NewCfg;
	int Speed;
	
	//读当前TTY属性
	if (tcgetattr(pUart->Fd, pOldCfg) != 0){
		perror("Can not got current com_config.\n");
		return -1;
	}
	
	NewCfg = *pOldCfg;
	//将TTY设置为RAW模式
	cfmakeraw(&NewCfg);
	NewCfg.c_cflag &= ~CSIZE;
	//设置模波特率
	switch (pUart->BaudRate){
		case BAUD_RATE_2400:
			Speed = B2400;break;
		case BAUD_RATE_4800:
			Speed = B4800;break;
		case BAUD_RATE_9600:
			Speed = B9600;break;
		case BAUD_RATE_19200:
			Speed = B19200;break;
		case BAUD_RATE_38400:
			Speed = B38400;break;
		case BAUD_RATE_57600:
			Speed = B57600;break;
		default:
			Speed = B9600;
	}
	cfsetispeed(&NewCfg, Speed);
	cfsetospeed(&NewCfg, Speed);
	//设置数据位
	switch (pUart->DataBits){
		case DATA_BITS_7BITS:
			NewCfg.c_cflag |= CS7;break;
		case DATA_BITS_8BITS:
		default:
			NewCfg.c_cflag |= CS8;
			
	}
	//设置校验位
	switch (pUart->Parity){
		case PARITY_O:
			NewCfg.c_cflag |= (PARODD | PARENB);
			NewCfg.c_iflag |= INPCK;
			break;
		case PARITY_E:
			NewCfg.c_cflag |= PARENB;
			NewCfg.c_cflag &= ~PARODD;
			NewCfg.c_iflag |= INPCK;
			break;
		case PARITY_NONE:
		default:
			NewCfg.c_cflag &= ~PARENB;
			NewCfg.c_iflag &= ~INPCK;
	}
	//设置停止位
	switch (pUart->StopBits){
		case STOP_BITS_2BITS:
			NewCfg.c_cflag |= CSTOPB;break;
		case STOP_BITS_1BIT:
		default:
			NewCfg.c_cflag &= ~CSTOPB;
	}
	//设置接收等待时间
	NewCfg.c_cc[VTIME] = 0;		//Wait forever
	NewCfg.c_cc[VMIN] = 1;		//Return when recieve 1byte
	//清除旧数据
	tcflush(pUart->Fd, TCIOFLUSH);	//Clean input&output data
	//激活新配置属性
	if((tcsetattr(pUart->Fd, TCSANOW, &NewCfg)) != 0){
		perror("Active configuration failed!\n");
		return -1;
	}
	return 0;
}

/* 串口结构体初始化函数 */
int InitUartStruct(t_uart* pUart){
	pUart->Fd = INIT_FD;
	pUart->pFp = NULL;
	pUart->BaudRate = BAUD_RATE_9600;
	pUart->DataBits = DATA_BITS_8BITS;
	pUart->StopBits = STOP_BITS_1BIT;
	pUart->Parity = PARITY_NONE;
	return 0;
}

/* 打开串口函数 */
int OpenPort(int const ComPort){
	assert(IS_COM_PORT(ComPort));
	int Fd;
	#ifdef GNR_COM
	char *pDev[] = {"/dev/ttyS0","/dev/ttyS1","/dev/ttyS2"};
	#endif
	#ifdef USB_COM
	char *pDev[] = {"/dev/ttyUSB0","/dev/ttyUSB1","/dev/ttyUSB2"};
	#endif
	#ifdef ACM_COM
	char *pDev[] = {"/dev/ttyACM0","/dev/ttyACM1","/dev/ttyACM2"};
	#endif

#ifdef DEBUG
printf("UART device is %s.\n",pDev[ComPort]);
#endif
	//打开设备文件：O_NOCTTY:不将终端设置为此进程的控制终端（仅通信）
	//O_NONBLOCK：I/O操作设置为非阻塞模式：操作不能完成返回错误，并不阻塞进程
	if((Fd = open(pDev[ComPort], O_RDWR|O_NOCTTY|O_NONBLOCK)) < 0){
		perror("Can not open serial port.\n");
		return -1;
	}
	//将串口设定位阻塞模式，等待串口输入
	//TODO：测试先非阻塞，后阻塞的用意
	if(fcntl(Fd, F_SETFL, 0) < 0){
		perror("Can not block input.\n");
		return -1;
	}
	//测试串口是否为Terminal
	//TODO：这里又是做了什么？
	if(isatty(STDIN_FILENO) == 0){
		perror("Serial is not a terminal device.\n");
		return -1;
	}
	return Fd;
}

