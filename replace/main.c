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
#include "efs_aes.h"

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

int init() {

    HAL_Init();                     //初始化HAL库   
    Stm32_Clock_Init(360,25,2,8);   //设置时钟,180Mhz
    //HAL_NVIC_SetPriority(SycTick_Handler, 14, 0);
    delay_init(180);                //初始化延时函数
    uart_init(115200);              //初始化USART
    LED_Init();                     //初始化LED 
    SDRAM_Init();                   //SDRAM初始化
    //创建开始任务
	return 0;
}

void rw_test() {
    int fd;
    int r;
	int i=0;
	printf("\r\n[RW Test] START\r\n");
	// ------- FIRST TEST --------
    // write to file
    fd = efs_open("/test.txt", O_CREAT|O_RDWR, 0);
	//printf("--- open1 finished ---\r\n");
	r = write(fd, "Hello World!", 13);
	printf("write: Hello World!\r\n");
	//printf("--- write1 finished ---\r\n");
    close(fd);
	// printf("fd4 = %d\r\n",fd);
	//printf("--- close1 finished ---\r\n");

    // read from file
    fd = efs_open("/test.txt", O_RDWR, 0);
	//printf("fd5 = %d\r\n",fd);
	//printf("--- open2 finished ---\r\n");
    char buf[13];
    r=read(fd, buf, 13);
	// printf("r6 = %d\r\n",r);
	//printf("--- read2 finished ---\r\n");
	close(fd);
	//printf("fd7 = %d\r\n",fd);
	//printf("--- close2 finished ---\r\n");

	//printf("\r\n--- OUTPUT ---\r\n");
    printf("read: %s\r\n", buf);

	// ------- SECOND TEST --------
	// write to file
    fd = efs_open("/test1.in", O_CREAT|O_RDWR, 0);
	//printf("fd8 = %d\r\n",fd);
    r = write(fd, "HELLO WORLD aaaa!", 18);
	// printf("r = %d\r\n",r);
	printf("write: HELLO WORLD aaaa!\r\n");
    close(fd);
	// printf("fd10 = %d\r\n",fd);
    // read from file
    fd = efs_open("/test1.in", O_RDWR, 0);
	// printf("fd11 = %d\r\n",fd);
    char buf1[20];
    r = read(fd, buf1, 18);
	// printf("r = %d\r\n",r);
	// printf("fd12 = %d\r\n",fd);
	close(fd);
    printf("read: %s\r\n", buf1);

	printf("[RW Test] END\r\n");
}

void rename_test()
{
	printf("\r\n[Rename Test] START\r\n");
	printf("/test1.in -> /test2.in\r\n");
	//printf("%d", rename("/test.txt", "/test2.in"));
	
	if (rename("/test1.in", "/test2.in") < 0)
        printf("ERROR\r\n");
    else
	{
		printf("Reading: test2.in\r\n");
		
		int fd = 0;
		printf("%d", efs_open("/test2.in", O_RDWR, 0));
		//fd = efs_open("/test2.in", O_RDWR, 0);
    	char buf[20];
    	read(fd, buf, 18);
		close(fd);
		printf("%s\r\n", buf);
		printf("OK\r\n");
	}
	printf("[Rename Test] END\r\n");
}

void stat_test()
{
	printf("\r\n[Stat Test] START\r\n");
	printf("checking /test2.in\r\n");

	struct stat_efs *buf;
	//if (stat_efs("/test2.in", buf) < 0)
    //    printf("ERROR\r\n");
    //else
	//{
	//	printf("OK\r\n");
	//	printf("st_size: %d\r\n", buf->st_size);
	//	printf("st_mode: %d\r\n", buf->st_mode);
	//}
	printf("[Stat Test] END\r\n");
}

void statfs_test()
{
	printf("\r\n[Statfs Test] START\r\n");
	printf("checking /test2.in\r\n");

	struct statfs *buf;

	//if (statfs_efs("/", buf) < 0)
    //    printf("ERROR\r\n");
    //else
	//{
	//	printf("OK\r\n");
	//	printf("f_bsize: %d\r\n", buf->f_bsize);
	//	printf("f_blocks: %d\r\n", buf->f_blocks);
	//}
	printf("[Statfs Test] END\r\n");
}

void lseek_test()
{
	printf("\r\n[Lseek Test] START\r\n");
	int fd = 0;
	fd = efs_open("/test2.in", O_CREAT|O_RDWR, 0);
	char buf[20];
	for (int i = 0; i < 20; i++)
	{
		memset(buf, 0, 20);
		lseek(fd, i, SEEK_SET);
		read(fd, buf, 1);
		printf("%d: %s\r\n", i, buf);
	}
	close(fd);

	printf("[Lseek Test] END\r\n");
}

void unlink_test()
{
	printf("\r\n[Unlink Test] START\r\n");
	if (unlink("/test2.in") < 0)
        printf("ERROR\r\n");
    else
	{
		int fd = efs_open("/test2.in", O_RDWR, 0);
		if(fd >= 0) 
			printf("ERROR\r\n");
		else
			printf("OK\r\n");
	}

	printf("[Unlink Test] END\r\n");
}

void crypt_test()
{
	printf("\r\n[Crypt Test] START\r\n");

	int fd = 0;
	fd = efs_open("/test2.in", O_RDWR, 0);
	char buf[20];
    read(fd, buf, 18);
	close(fd);
	printf("original: %s\r\n", buf);

	encryptAES("/test2.in","/test3.in",128,"qwertyuiopasdf1");

	fd = efs_open("/test3.in", O_RDWR, 0);
	memset(buf, 0, 20);
    read(fd, buf, 18);
	close(fd);
	printf("encrypted: %s\r\n", buf);

	decryptAES("/test3.in","/test4.in",128,"qwertyuiopasdf1");

	fd = efs_open("/test4.in", O_RDWR, 0);
	memset(buf, 0, 20);
    read(fd, buf, 18);
	close(fd);
	printf("decrypted: %s\r\n", buf);
	
	printf("[Crypt Test] END\r\n");
}

//not for ramfs
void ftruncate_test() 
{
	printf("\r\n[Ftruncate Test] START\r\n");

	int fd = 0;
	fd = efs_open("/test2.in", O_RDWR, 0);
	char buf[20];
    read(fd, buf, 18);
	printf("original: %s\r\n", buf);

	if(ftruncate(fd, 5) < 0)
	{
		printf("ERROR\r\n");
	}
	else 
	{
		printf("OK\r\n");
	}

	read(fd, buf, 18);
	printf("truncated: %s\r\n", buf);

	close(fd);
	printf("[Ftruncate Test] END\r\n");
}

//not for ramfs
void dir_test()
{
	printf("\r\n[Directory Test] START\r\n");

	int fd = 0;
	fd = efs_open("/test_dir/test.txt", O_CREAT|O_RDWR, 0);
	rmdir("/test_dir");
	if (mkdir("/test_dir", 0) < 0)
        printf("ERROR\r\n");
    else
	{
    	// write to file
    	fd = efs_open("/test_dir/test.txt", O_CREAT|O_RDWR, 0);
		//printf("1\r\n");
		//printf("--- open1 finished ---\r\n");
    	write(fd, "Test info in /test_dir/test.txt", 32);
		printf("write: Test info in /test_dir/test.txt\r\n");
		char buf[40];
		lseek(fd, 0, SEEK_SET);
    	read(fd, buf, 32);
		printf("read: %s\r\n", buf);
		printf("OK\r\n");
		
	}
	close(fd);
	printf("[Stat Test] END\r\n");
}

void sd_mount()
{
	int fd;
	const char sd[20] = "SDCard";


	//检测SD卡成功 		

	if(fd = efs_mkfs("fatfs", sd) != 0)
		{
			printf("FATFS mkfs failed!\r\n");
			printf("[mkfs]: fd = %d\r\n",fd);
		}

    if (fd = efs_mount(sd, "/", "fatfs", 0, 0 ) != 0) //
    {
        printf("FATFS mount failed!\r\n");
			printf("[mount]: fd = %d\r\n",fd);
		if(fd=efs_mkfs("fatfs", 0) != 0)
		{
			printf("FATFS mkfs failed!\r\n");
			printf("[mkfs]: fd = %d\r\n",fd);
		}
		else {
			printf("FATFS mkfs successed!\r\n");
			if (fd = efs_mount(sd, "/", "fatfs", 0, 0) != 0)
			{
				printf("FATFS mount failed!\r\n");
				printf("[mount]: fd = %d\r\n",fd);
			}
			else 
			{
				printf("FATFS mount successed!\r\n");
			}
		}
    }
    else
    {
        printf("FATFS mount successed!\r\n");
    } 
}

int ramfstest() 
{
    /* 2. filesystem init */
    efs_ramfs_init();

    /* 3. storage device init */


    /* 4. storage device mount */
	uint8_t *rampool;

	
	if ((rampool = (uint8_t *)pvPortMalloc(1024)) == NULL)
	{
		printf("Failed to allocate memory for ramfs!\r\n");
		return -1;
	}
	

    if (efs_mount( NULL, "/", "ramfs", 0, efs_ramfs_create(rampool, 1024)) != 0) //
    {
        printf("File System on ram initialization failed!\r\n");
        return -1;
    }
    else
    {
        printf("File System on ram initialized!\r\n");
        return 0;
    } 

	int fd;

    // write to file
    fd = efs_open("/test.txt", O_CREAT|O_RDWR, 0);
	printf("--- open1 finished ---\r\n");

    write(fd, "Hello World!", 12);
	printf("--- write1 finished ---\r\n");

    close(fd);
	printf("--- close1 finished ---\r\n");

    // read from file
    fd = efs_open("/test.txt", O_RDWR, 0);
	printf("--- open2 finished ---\r\n");
    char buf[12];
    read(fd, buf, 12);
    printf("%s\n", buf);
    close(fd);
}

int ramfs_init() {
    /* 1. efs init */
    efs_init();

    /* 2. filesystem init */
    efs_ramfs_init();

    /* 3. storage device init */


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

void fatfs_init(){
	while(SD_Init())//检测不到SD卡
	{
        printf("SD Card Error!\r\r\n");
		delay_ms(500);					
        printf("Please Check!\r\r\n");
		delay_ms(500);
		LED0=!LED0;//DS0闪烁
	}

    /* 1. efs init */
    efs_init();

    /* 2. filesystem init */
    efs_fatfs_init();

    /* 4. storage device mount */
}

int main(void)
{
    
	int i;
    init();
	for(i=1; i <= 5; i++) {
        printf("1\r\n");
        delay_ms(500);
    }
    //ramfs_init();
	fatfs_init();
	sd_mount();
	rw_test();
	rename_test();
	stat_test();
	statfs_test();
	lseek_test();
	crypt_test();
	//ftruncate_test();
	unlink_test();
	dir_test();

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
        printf("float_num 的值为: %.4f\r\r\n",float_num);
        vTaskDelay(1000);
    }
}
