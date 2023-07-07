/*
 * FreeRTOS V202212.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/******************************************************************************
 * This project provides two demo applications.  A simple blinky style project,
 * and a more comprehensive test and demo application.  The
 * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting in main.c is used to select
 * between the two.  See the notes on using mainCREATE_SIMPLE_BLINKY_DEMO_ONLY
 * in main.c.  This file implements the simply blinky version.
 *
 * This file only contains the source code that is specific to the basic demo.
 * Generic functions, such FreeRTOS hook functions, are defined in main.c.
 ******************************************************************************
 *
 * main_blinky() creates one queue, one software timer, and two tasks.  It then
 * starts the scheduler.
 *
 * The Queue Send Task:
 * The queue send task is implemented by the prvQueueSendTask() function in
 * this file.  It uses vTaskDelayUntil() to create a periodic task that sends
 * the value 100 to the queue every 200 (simulated) milliseconds.
 *
 * The Queue Send Software Timer:
 * The timer is an auto-reload timer with a period of two (simulated) seconds.
 * Its callback function writes the value 200 to the queue.  The callback
 * function is implemented by prvQueueSendTimerCallback() within this file.
 *
 * The Queue Receive Task:
 * The queue receive task is implemented by the prvQueueReceiveTask() function
 * in this file.  prvQueueReceiveTask() waits for data to arrive on the queue.
 * When data is received, the task checks the value of the data, then outputs a
 * message to indicate if the data came from the queue send task or the queue
 * send software timer.
 */

/* Standard includes. */
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"

/* EFS includes */
#include <efs.h>
#include <efs_posix.h>
#include <efs_ramfs.h>
#include <efs_fs.h>

/* std */
#include <fcntl.h>

/* Priorities at which the tasks are created. */
#define mainQUEUE_RECEIVE_TASK_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define	mainQUEUE_SEND_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )

/* The rate at which data is sent to the queue.  The times are converted from
milliseconds to ticks using the pdMS_TO_TICKS() macro. */
#define mainTASK_SEND_FREQUENCY_MS			pdMS_TO_TICKS( 200UL )
#define mainTIMER_SEND_FREQUENCY_MS			pdMS_TO_TICKS( 2000UL )

/* The number of items the queue can hold at once. */
#define mainQUEUE_LENGTH					( 2 )

/* The values sent to the queue receive task from the queue send task and the
queue send software timer respectively. */
#define mainVALUE_SENT_FROM_TASK			( 100UL )
#define mainVALUE_SENT_FROM_TIMER			( 200UL )

/*-----------------------------------------------------------*/

/*
 * The tasks as described in the comments at the top of this file.
 */
static void prvQueueReceiveTask( void *pvParameters );
static void prvQueueSendTask( void *pvParameters );

/*
 * The callback function executed when the software timer expires.
 */
static void prvQueueSendTimerCallback( TimerHandle_t xTimerHandle );

/*-----------------------------------------------------------*/

/* The queue used by both tasks. */
static QueueHandle_t xQueue = NULL;

/* A software timer that is started from the tick hook. */
static TimerHandle_t xTimer = NULL;

/*-----------------------------------------------------------*/


int init() {
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

	struct stat_efs *buf;
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

	struct stat_efs *buf;

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
	struct stat_efs *buf;
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


/* Define a callback function that will be used by multiple timer
instances.  The callback function does nothing but count the number
of times the associated timer expires, and stop the timer once the
timer has expired 10 times.  The count is saved as the ID of the
timer. */
void vTimerCallback( TimerHandle_t xTimer)
{
	const uint32_t ulMaxExpiryCountBeforeStopping = 10;
	uint32_t ulCount;

    /* Optionally do something if the pxTimer parameter is NULL. */
    configASSERT( xTimer );

    /* The number of times this timer has expired is saved as the timer's ID.  Obtain the count. */
    ulCount = ( uint32_t ) pvTimerGetTimerID( xTimer );

    /* Increment the count, then test to see if the timer has expired ulMaxExpiryCountBeforeStopping yet. */
    ulCount++;

    /* If the timer has expired 10 times then stop it from running. */
    if( ulCount >= ulMaxExpiryCountBeforeStopping )
    {
        /* Do not use a block time if calling a timer API function from a timer callback function, as doing so could cause a deadlock! */
        xTimerStop( xTimer, 0 );
    }
    else
    {
       	/* Store the incremented count back into the timer's ID field so it can be read back again the next time this software timer expires. */
       	vTimerSetTimerID( xTimer, ( void * ) ulCount );
    }
	return;
}

void ReadWritePerformanceTest(void)
{
	TickType_t time = 0;
	int fd = 0;
	TimerHandle_t write_timer = NULL, read_timer = NULL;
	fd = efs_open("/test_dir/performance_test.txt", O_CREAT|O_RDWR, 0);
	write_timer = xTimerCreate("write_timer", pdMS_TO_TICKS(5000), pdFALSE, (void *)0, vTimerCallback);
	xTimerStart(write_timer, 0);
	for (int i = 0; i < 1000; i++)
	{
		write(fd, "Test info in /test_dir/performance_test.txt", 45);
	}
	xTimerStop(write_timer, 0);
	time = xTimerGetPeriod(TimerHandle_t write_timer);
	printf("Written char count: %d\nTotal time spent: %d\n", 45000, time);
	printf("Bits per second: %d bps\n", 45000 / time * 8);
	read_timer = xTimerCreate("read_timer", pdMS_TO_TICKS(5000), pdFALSE, (void *)0, vTimerCallback);
	xTimerStart(read_timer, 0);
	for (int i = 0; i < 1000; i++)
	{
		char buf[45];
		read(fd, buf, 45);
	}
	xTimerStop(read_timer, 0);
	time = xTimerGetPeriod(TimerHandle_t read_timer);
	printf("Read char count: %d\nTotal time spent: %d\n", 45000, time);
	printf("Bits per second: %d bps\n", 45000 / time * 8);
	close(fd);
	return;
}
/*-----------------------------------------------------------*/

void main_blinky( void )
{
	init();
	rw_test();
	rename_test();
	stat_test();
	statfs_test();
	lseek_test();
	crypt_test();
	//ftruncate_test();
	unlink_test();
	//dir_test();
	for( ;; );
}

/*-----------------------------------------------------------*/

static void prvQueueSendTask( void *pvParameters )
{
TickType_t xNextWakeTime;
const TickType_t xBlockTime = mainTASK_SEND_FREQUENCY_MS;
const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TASK;
	/* Prevent the compiler warning about the unused parameter. */
	( void ) pvParameters;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	for( ;; )
	{
		/* Place this task in the blocked state until it is time to run again.
		The block time is specified in ticks, pdMS_TO_TICKS() was used to
		convert a time specified in milliseconds into a time specified in ticks.
		While in the Blocked state this task will not consume any CPU time. */
		vTaskDelayUntil( &xNextWakeTime, xBlockTime );

		/* Send to the queue - causing the queue receive task to unblock and
		write to the console.  0 is used as the block time so the send operation
		will not block - it shouldn't need to block as the queue should always
		have at least one space at this point in the code. */
		xQueueSend( xQueue, &ulValueToSend, 0U );
	}
}
/*-----------------------------------------------------------*/

static void prvQueueSendTimerCallback( TimerHandle_t xTimerHandle )
{
const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TIMER;

	/* This is the software timer callback function.  The software timer has a
	period of two seconds and is reset each time a key is pressed.  This
	callback function will execute if the timer expires, which will only happen
	if a key is not pressed for two seconds. */

	/* Avoid compiler warnings resulting from the unused parameter. */
	( void ) xTimerHandle;

	/* Send to the queue - causing the queue receive task to unblock and
	write out a message.  This function is called from the timer/daemon task, so
	must not block.  Hence the block time is set to 0. */
	xQueueSend( xQueue, &ulValueToSend, 0U );
}
/*-----------------------------------------------------------*/

static void prvQueueReceiveTask( void *pvParameters )
{
uint32_t ulReceivedValue;

	/* Prevent the compiler warning about the unused parameter. */
	( void ) pvParameters;

	for( ;; )
	{
		/* Wait until something arrives in the queue - this task will block
		indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
		FreeRTOSConfig.h.  It will not use any CPU time while it is in the
		Blocked state. */
		xQueueReceive( xQueue, &ulReceivedValue, portMAX_DELAY );

		/*  To get here something must have been received from the queue, but
		is it an expected value? */
		if( ulReceivedValue == mainVALUE_SENT_FROM_TASK )
		{
			/* It is normally not good to call printf() from an embedded system,
			although it is ok in this simulated case. */
			printf( "Message received from task\r\n" );
		}
		else if( ulReceivedValue == mainVALUE_SENT_FROM_TIMER )
		{
			printf( "Message received from software timer\r\n" );
		}
		else
		{
			printf( "Unexpected message\r\n" );
		}
	}
}
/*-----------------------------------------------------------*/


