#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "led.h"
#include "ff.h"
#include "ffconf.h"
#include "sdram.h"
#include "sdio_sdcard.h"

/* EFS includes */
#include <efs.h>
#include <efs_posix.h>
#include <efs_ramfs.h>
#include <efs_fs.h>

#define START_TASK_PRIO 1 //任务优先级
#define START_STK_SIZE 128 //任务堆栈大小
TaskHandle_t StartTask_Handler; //任务句柄
void start_task(void *pvParameters); //任务函数
#define LED0_TASK_PRIO 2 //任务优先级
#define LED0_STK_SIZE 50 //任务堆栈大小
TaskHandle_t LED0Task_Handler; //任务句柄
void led0_task(void *p_arg); //任务函数
#define LED1_TASK_PRIO 3 //任务优先级
#define LED1_STK_SIZE 50 //任务堆栈大小
TaskHandle_t LED1Task_Handler; //任务句柄
void led1_task(void *p_arg); //任务函数
#define FLOAT_TASK_PRIO 4 //任务优先级
#define FLOAT_STK_SIZE 128 //任务堆栈大小
TaskHandle_t FLOATTask_Handler; //任务句柄
void float_task(void *p_arg); //任务函数

FATFS *fs[1];

int init() {

    HAL_Init();                     //初始化HAL库   
    Stm32_Clock_Init(360,25,2,8);   //设置时钟,180Mhz
    //HAL_NVIC_SetPriority(SycTick_Handler, 14, 0);
    delay_init(180);                //初始化延时函数
    uart_init(115200);              //初始化USART
    LED_Init();                     //初始化LED 
    SDRAM_Init();                   //SDRAM初始化
    //创建开始任务

 	while(SD_Init())//检测不到SD卡
	{
        printf("SD Card Error!\r\n");
		delay_ms(500);					
        printf("Please Check!\r\n");
		delay_ms(500);
		LED0=!LED0;//DS0闪烁
	}

    /* 1. efs init */
    efs_init();

    /* 2. filesystem init */
    efs_fatfs_init();



    /* 4. storage device mount */
	uint8_t *rampool;

	
	if ((rampool = (uint8_t *)pvPortMalloc(1024)) == NULL)
	{
		printf("Failed to allocate memory for ramfs!\n");
		return -1;
	}
	

    if (efs_mount( NULL, "/", "ramfs", 0, efs_ramfs_create(rampool, 512)) != 0) //
    {
        printf("File System on ram initialization failed!\n");
        return -1;
    }
    else
    {
        printf("File System on ram initialized!\n");
        return 0;
    } 
}

void rw_test() {
    int fd;
	printf("\n[RW Test] START\n");
	// ------- FIRST TEST --------
    // write to file
    fd = efs_open("/test.txt", O_CREAT|O_RDWR, 0);
	//printf("1\n");
	//printf("--- open1 finished ---\n");
    write(fd, "Hello World!", 13);
	printf("write: Hello World!\n");
	//printf("--- write1 finished ---\n");
    close(fd);
	//printf("--- close1 finished ---\n");

    // read from file
    fd = efs_open("/test.txt", O_RDWR, 0);
	//printf("--- open2 finished ---\n");
    char buf[13];
    read(fd, buf, 13);
	//printf("--- read2 finished ---\n");
	close(fd);
	//printf("--- close2 finished ---\n");

	//printf("\n--- OUTPUT ---\n");
    printf("read: %s\n", buf);

	// ------- SECOND TEST --------
	// write to file
    fd = efs_open("/test1.in", O_CREAT|O_RDWR, 0);
    write(fd, "HELLO WORLD aaaa!", 18);
	printf("write: HELLO WORLD aaaa!\n");
    close(fd);

    // read from file
    fd = efs_open("/test1.in", O_RDWR, 0);
    char buf1[20];
    read(fd, buf1, 18);
	close(fd);
    printf("read: %s\n", buf1);

	printf("[RW Test] END\n");
}

void rename_test()
{
	printf("\n[Rename Test] START\n");
	printf("/test1.in -> /test2.in\n");
	//printf("%d", rename("/test.txt", "/test2.in"));
	
	if (rename("/test1.in", "/test2.in") < 0)
        printf("ERROR\n");
    else
	{
		printf("Reading: test2.in\n");
		
		int fd = 0;
		printf("%d", efs_open("/test2.in", O_RDWR, 0));
		//fd = efs_open("/test2.in", O_RDWR, 0);
    	char buf[20];
    	read(fd, buf, 18);
		close(fd);
		printf("%s\n", buf);
		printf("OK\n");
	}
	printf("[Rename Test] END\n");
}

void stat_test()
{
	printf("\n[Stat Test] START\n");
	printf("checking /test2.in\n");

	struct stat *buf;
	if (stat("/test2.in", buf) < 0)
        printf("ERROR\n");
    else
	{
		printf("OK\n");
		printf("st_size: %d\n", buf->st_size);
		printf("st_mode: %d\n", buf->st_mode);
	}
	printf("[Stat Test] END\n");
}

void statfs_test()
{
	printf("\n[Statfs Test] START\n");
	printf("checking /test2.in\n");

	struct stat *buf;

	if (statfs("/", buf) < 0)
        printf("ERROR\n");
    else
	{
		printf("OK\n");
		printf("st_size: %d\n", buf->st_size);
		printf("st_blocks: %d\n", buf->st_blocks);
	}
	printf("[Statfs Test] END\n");
}

void lseek_test()
{
	printf("\n[Lseek Test] START\n");
	int fd = 0;
	fd = efs_open("/test2.in", O_CREAT|O_RDWR, 0);
	char buf[20];
	for (int i = 0; i < 20; i++)
	{
		memset(buf, 0, 20);
		lseek(fd, i, SEEK_SET);
		read(fd, buf, 1);
		printf("%d: %s\n", i, buf);
	}
	close(fd);

	printf("[Lseek Test] END\n");
}

void unlink_test()
{
	printf("\n[Unlink Test] START\n");
	if (unlink("/test2.in") < 0)
        printf("ERROR\n");
    else
	{
		int fd = open("/test2.in", O_RDWR, 0);
		if(fd > 0) 
			printf("ERROR\n");
		else
			printf("OK\n");
	}

	printf("[Unlink Test] END\n");
}

void crypt_test()
{
	printf("\n[Crypt Test] START\n");

	int fd = 0;
	fd = efs_open("/test2.in", O_RDWR, 0);
	char buf[20];
    read(fd, buf, 18);
	close(fd);
	printf("original: %s\n", buf);

	encrypt("/test2.in", 7);

	fd = efs_open("/test2.in", O_RDWR, 0);
	memset(buf, 0, 20);
    read(fd, buf, 18);
	close(fd);
	printf("encrypted: %s\n", buf);

	decrypt("/test2.in", 7);

	fd = efs_open("/test2.in", O_RDWR, 0);
	memset(buf, 0, 20);
    read(fd, buf, 18);
	close(fd);
	printf("decrypted: %s\n", buf);
	
	printf("[Crypt Test] END\n");
}

//not for ramfs
void ftruncate_test() 
{
	printf("\n[Ftruncate Test] START\n");

	int fd = 0;
	fd = efs_open("/test2.in", O_RDWR, 0);
	char buf[20];
    read(fd, buf, 18);
	printf("original: %s\n", buf);

	if(ftruncate(fd, 5) < 0)
	{
		printf("ERROR\n");
	}
	else 
	{
		printf("OK\n");
	}

	read(fd, buf, 18);
	printf("truncated: %s\n", buf);

	close(fd);
	printf("[Ftruncate Test] END\n");
}

//not for ramfs
void dir_test()
{
	printf("\n[Directory Test] START\n");

	int fd = 0;
	fd = efs_open("/test_dir/test.txt", O_CREAT|O_RDWR, 0);
	struct stat *buf;
	if (mkdir("/test_dir", 0) < 0)
        printf("ERROR\n");
    else
	{
    	// write to file
    	fd = efs_open("/test_dir/test.txt", O_CREAT|O_RDWR, 0);
		//printf("1\n");
		//printf("--- open1 finished ---\n");
    	write(fd, "Test info in /test_dir/test.txt", 32);
		printf("write: Test info in /test_dir/test.txt\n");
		char buf[40];
    	read(fd, buf, 32);
		printf("%s\n", buf);
		printf("OK\n");
		
	}
	close(fd);
	printf("[Stat Test] END\n");
}


int main(void)
{
    FIL *fd = NULL;
    char buf[13];
    UINT temp;
    int i;
    FRESULT result;
    BYTE work[2048];

    init();

	//检测SD卡成功 		
    for(i=1; i <= 5; i++) {
        printf("1\r\n");
        delay_ms(500);
    }
    //while(SD_Init())//检测不到SD卡
	//{
    //    printf("SD Card Error!\r\n");
	//	delay_ms(500);					
    //    printf("Please Check!\r\n");
	//	delay_ms(500);
	//	LED0=!LED0;//DS0闪烁
	//}
	rw_test();
	rename_test();
	stat_test();
	statfs_test();
	lseek_test();
	crypt_test();
	//ftruncate_test();
	unlink_test();
	//dir_test();

    xTaskCreate((TaskFunction_t )start_task, //任务函数
                (const char* )"start_task", //任务名称
                (uint16_t )START_STK_SIZE, //任务堆栈大小
                (void* )NULL, //传递给任务函数的参数
                (UBaseType_t )START_TASK_PRIO, //任务优先级
                (TaskHandle_t* )&StartTask_Handler); //任务句柄
    vTaskStartScheduler(); //开启任务调度
}
//开始任务任务函数
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL(); //进入临界区
    //创建 LED0 任务
    xTaskCreate((TaskFunction_t )led0_task,
                (const char* )"led0_task",
                (uint16_t )LED0_STK_SIZE,
                (void* )NULL,
                (UBaseType_t )LED0_TASK_PRIO,
                (TaskHandle_t* )&LED0Task_Handler);
    //创建 LED1 任务
    xTaskCreate((TaskFunction_t )led1_task,
                (const char* )"led1_task",
                (uint16_t )LED1_STK_SIZE,
                (void* )NULL,
                (UBaseType_t )LED1_TASK_PRIO,
                (TaskHandle_t* )&LED1Task_Handler);
    //浮点测试任务
    xTaskCreate((TaskFunction_t )float_task,
                (const char* )"float_task",
                (uint16_t )FLOAT_STK_SIZE,
                (void* )NULL,
                (UBaseType_t )FLOAT_TASK_PRIO,
                (TaskHandle_t* )&FLOATTask_Handler);
    vTaskDelete(StartTask_Handler); //删除开始任务
    taskEXIT_CRITICAL(); //退出临界区
}

//LED0 任务函数
void led0_task(void *pvParameters)
{
    while(1)
    {
        LED0=~LED0;
        vTaskDelay(500);
    }
} 

//LED1 任务函数
void led1_task(void *pvParameters)
{
    while(1)
    {
        LED1=0;
        vTaskDelay(200);
        LED1=1;
        vTaskDelay(800);
    }
}
//浮点测试任务
void float_task(void *p_arg)
{
    static float float_num=0.00;
    while(1)
    {
        float_num+=0.01f;
        printf("float_num 的值为: %.4f\r\n",float_num);
        vTaskDelay(1000);
    }
}
