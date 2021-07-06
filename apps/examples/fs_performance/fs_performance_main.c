/****************************************************************************
 *
 * Copyright 2021 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 * examples/fs/fs_performance_main.c
 *
 *   Copyright (C) 2008, 2011-2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
/***************************************************************************
 * This code is just meant for file system performance test purpose. 
 * Stil testing in progress
***************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

/****************************************************************************
 * Pre Processor Definitions
 ****************************************************************************/
#define ROOT				"/mnt/"
#define SAMPLE_FILE 			ROOT "sample_file"
#define SINGLE_TEST_FILE_DIR		ROOT "single"
#define SINGLE_TEST_FILE		ROOT "/single/test_file"
#define MULTI_TEST_FILE_DIR 		ROOT "multiple"
#define MULTI_TEST_FILE 		ROOT "test_file_"
#define TEST_FLAG_SINGLE_WRITE_APP 	1
#define TEST_FLAG_SINGLE_WRITE_OVE 	2
#define TEST_FLAG_SINGLE_READ_SEQ 	3
#define TEST_FLAG_SINGLE_READ_OVE 	4
#define TEST_FLAG_MULTIPLE_WRITE_APP 	5
#define TEST_FLAG_MULTIPLE_WRITE_OVE 	6
#define TEST_FLAG_MULTIPLE_READ_SEQ 	7
#define TEST_FLAG_MULTIPLE_READ_OVE 	8
#define TEST_FLAG_SINGLE_WRITE_MIX 	9
#define TEST_FLAG_INCREMENTAL_OVERWRITE 10
#define TEST_FLAG_INCREMENTAL_OVERREAD  11
#define N_MULTIPLE_FILES 		12
#define TEST_FILE_NAME_LEN_MAX		27
#define MAX_ITR 			5
#define FILE_SIZE			32
#define N_FILES				2
/****************************************************************************
 * Public Variables
 ****************************************************************************/

double time_taken_append;
double time_taken_without_append;
int fd_multiple[N_MULTIPLE_FILES];
char buffer[N_MULTIPLE_FILES][TEST_FILE_NAME_LEN_MAX];
char rw_buf_1kb[1024+1];
char rw_buf_2kb[2048+1];
char rw_buf_2kb_read[2048+1];
char rw_buf_8kb[8*1024];
char rw_buf_s1800b[1800];
char rw_buf_s1[5*1024*1];
char rw_buf_s2[5*1024*2];
char rw_buf_s3[5*1024*3];
int BUFFER_SIZE_S[5];
unsigned long long time_write_inc_ove[35];
unsigned long long time_read_inc_ove[35];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

int init_sample(int buffer_length)
{
	int fd;
	int ret;
	int i;
	char sample_buf[] = "Writing data to the sample file\n";

	fd = open(SAMPLE_FILE, O_WRONLY | O_CREAT);
	if (fd < 0) {
		printf("Unable to open sample file: %s, fd = %d\n", SAMPLE_FILE, fd);
		return -ENOMEM;
	}

	for (i = 0; i < (buffer_length / 32) + 1; i++) {
		ret = write(fd, sample_buf, 32);
		if (ret != 32) {
			printf("Unable to write to sample file, fd = %d\n", ret);
			close(fd);
			return -1;
		}
	}

	close(fd);
	return OK;
}

static void output(int flag, unsigned long time, int itr)
{
	printf("====================================================================================================\n");
	switch (flag) {

	case TEST_FLAG_SINGLE_WRITE_APP:
		printf("Time taken for writing %d times to a single file consecutively is %lu.%06lu s\n", itr, (time)/1000000, (time)%1000000);
		break;

	case TEST_FLAG_SINGLE_WRITE_OVE:
		printf("Time taken for overwriting %d times to a single file at the start is %lu.%06lu s\n", itr, (time)/1000000, (time)%1000000);
		break;

	case TEST_FLAG_MULTIPLE_WRITE_APP:
		printf("Time taken for writing %d times consecutively to multiple files is %lu.%06lu s\n", itr, (time)/1000000, (time)%1000000);
		break;

	case TEST_FLAG_MULTIPLE_WRITE_OVE:
		printf("Time taken for writing %d times to multiple files at their start is %lu.%06lu s\n", itr, (time)/1000000, (time)%1000000);
                break;

	case TEST_FLAG_SINGLE_WRITE_MIX:
                printf("Time taken for open-(write*8)-close-delete %d times to a single file is %lu.%06lu s\n", itr, (time)/1000000, (time)%1000000);
                break;

	case TEST_FLAG_SINGLE_READ_SEQ:
                printf("Time taken for reading %d times from a single file consecutively is %lu.%06lu s\n", itr, (time)/1000000, (time)%1000000);
                break;

	case TEST_FLAG_SINGLE_READ_OVE:
                printf("Time taken for reading %d times from a single file from the start is %lu.%06lu s\n", itr, (time)/1000000, (time)%1000000);
                break;

	case TEST_FLAG_MULTIPLE_READ_SEQ:
                printf("Time taken for reading %d times consecutively from multiple files is %lu.%06lu s\n", itr, (time)/1000000, (time)%1000000);
                break;

	case TEST_FLAG_MULTIPLE_READ_OVE:
                printf("Time taken for reading %d times from multiple files from the start is %lu.%06lu s\n", itr, (time)/1000000, (time)%1000000);
                break;

	case TEST_FLAG_INCREMENTAL_OVERWRITE :
                printf("Time taken for over writing %d times to single file incrementally at their start is %lu.%06lu s\n", itr, (time)/1000000, (time)%1000000);
                break;

        case TEST_FLAG_INCREMENTAL_OVERREAD :
               printf("Time taken for over reading %d times to single file incrementally from their start is %lu.%06lu s\n", itr, (time)/1000000, (time)%1000000);
                break;

	default:
		break;
	}
	printf("====================================================================================================\n\n");
	return;
}

static int perform_write(int fd, int size_type) {
        int ret;
        if(size_type == 0)
		ret= write(fd, rw_buf_s1800b, BUFFER_SIZE_S[size_type]);
        else if(size_type == 1)
		ret= write(fd, rw_buf_s1, BUFFER_SIZE_S[size_type]);
        else if(size_type == 2)
		ret= write(fd, rw_buf_s2, BUFFER_SIZE_S[size_type]);
        else if(size_type == 3)
		ret= write(fd, rw_buf_s3, BUFFER_SIZE_S[size_type]);

	return ret;
}

static int perform_read(int fd, int size_type) {
        int ret ;

        if(size_type == 0)
		ret= read(fd, rw_buf_s1800b,  BUFFER_SIZE_S[size_type]);
        else if(size_type == 1)
		ret= read(fd, rw_buf_s1,  BUFFER_SIZE_S[size_type]);
        else if(size_type == 2)
		ret= read(fd, rw_buf_s2,  BUFFER_SIZE_S[size_type]);
        else if(size_type == 3)
		ret= read(fd, rw_buf_s3,  BUFFER_SIZE_S[size_type]);

        return ret;
}

static int do_test(int fd, int opn, int size_type)
{
	int totalSize = 3*1024*1024; //to be written 3 mb in total
	int ret = OK;
	int i;
	int j;
	int k;
	int res2;
	struct timeval wr_time_start;
	struct timeval wr_time_stop;
	struct timeval wr_time_mid_stop;
	unsigned long long tt= 0;
        double t;

	switch (opn) {
	case TEST_FLAG_SINGLE_WRITE_APP:
		printf("\n\n\n**********SINGLE_WRITE_APP** %d Bytes *************\n\n\n", BUFFER_SIZE_S[size_type]);
		gettimeofday(&wr_time_start, NULL);
		for (i = 0; i < (totalSize / BUFFER_SIZE_S[size_type]) - 1; i++) {
			ret = perform_write(fd, size_type);
			if (ret != BUFFER_SIZE_S[size_type]) { 
				printf("Unable to write to test file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
				return -1;
			}
		}
		gettimeofday(&wr_time_stop, NULL);
		tt = (wr_time_stop.tv_sec - wr_time_start.tv_sec) * 1000000 + (wr_time_stop.tv_usec - wr_time_start.tv_usec);
		output(TEST_FLAG_SINGLE_WRITE_APP, tt, (totalSize / BUFFER_SIZE_S[size_type]) - 1);

		ret = lseek(fd, 0, SEEK_SET);
		if (ret < 0) {
			printf("Unable to move file pointer to start\n");
			return -1;
		}

		gettimeofday(&wr_time_start, NULL);
		for (i = 0; i < (totalSize / BUFFER_SIZE_S[size_type]) - 1; i++) {
			ret = perform_read(fd, size_type);
			if (ret != BUFFER_SIZE_S[size_type]) {
				printf("Unable to read to from file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
				return -1;
			}
		}
		gettimeofday(&wr_time_stop, NULL);
		tt = (wr_time_stop.tv_sec - wr_time_start.tv_sec) * 1000000 + (wr_time_stop.tv_usec - wr_time_start.tv_usec);
		output(TEST_FLAG_SINGLE_READ_SEQ, tt, (totalSize / BUFFER_SIZE_S[size_type]) - 1);
		break;

	case TEST_FLAG_SINGLE_WRITE_OVE:
		 printf("\n\n\n**********SINGLE_WRITE_OVE** %d  Bytes*************\n\n\n",BUFFER_SIZE_S[size_type]);
		 gettimeofday(&wr_time_start, NULL);
		 for(i = 0; i < (totalSize / BUFFER_SIZE_S[size_type]) - 1; i++) {
			 fd = open(SINGLE_TEST_FILE, O_RDWR | O_CREAT | O_TRUNC);
			 if (fd < 0) {
				 printf("Unable to do test perfomance on file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
				 return -1;
			 }
			 ret = perform_write(fd, size_type);
			 if (ret != BUFFER_SIZE_S[size_type]) {
				 printf("Unable to do test performance on file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
				 return -1;
			 }
			 close(fd);
		 }
		 gettimeofday(&wr_time_stop, NULL);
		 tt = (wr_time_stop.tv_sec - wr_time_start.tv_sec) * 1000000 + (wr_time_stop.tv_usec - wr_time_start.tv_usec);
		 output(TEST_FLAG_SINGLE_WRITE_OVE, tt, (totalSize / BUFFER_SIZE_S[size_type]) - 1);

		gettimeofday(&wr_time_start, NULL);
		for(i = 0; i < (totalSize / BUFFER_SIZE_S[size_type]) - 1; i++) {
			fd = open(SINGLE_TEST_FILE, O_RDONLY);
			if(fd < 0) {
				printf("Unable to do test perfomance on file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
				return -1;
			}
			ret = perform_read(fd, size_type);
			if (ret != BUFFER_SIZE_S[size_type]) {
				printf("Unable to do read test performance on file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
				close(fd);
				return -1;
			}
			close(fd);
		}
		gettimeofday(&wr_time_stop, NULL);
		tt = (wr_time_stop.tv_sec - wr_time_start.tv_sec) * 1000000 + (wr_time_stop.tv_usec - wr_time_start.tv_usec);
		output(TEST_FLAG_SINGLE_READ_OVE, tt, (totalSize / BUFFER_SIZE_S[size_type]) - 1);

		ret = unlink(SINGLE_TEST_FILE);
		if (ret < 0) {
			printf("Failed to remove file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
			return -1;
		}
		break;

	case TEST_FLAG_MULTIPLE_WRITE_APP:
		 printf("\n\n\n**********MULTIPLE_WRITE_APP******%d Bytes*********\n\n\n", BUFFER_SIZE_S[size_type]);
		gettimeofday(&wr_time_start, NULL);
		for(i = 0; i < (totalSize / (BUFFER_SIZE_S[size_type] * 4)) - 1; i++) {
			for(j = 0; j < 4; j++) {
				ret = perform_write(fd_multiple[j], size_type);
				if (ret != BUFFER_SIZE_S[size_type]) {
					printf("Unable to write to test file: %s, ret = ", buffer[j], ret);
					return -1;
				}
			}
		}
		gettimeofday(&wr_time_stop, NULL);
		tt = (wr_time_stop.tv_sec - wr_time_start.tv_sec) * 1000000 + (wr_time_stop.tv_usec - wr_time_start.tv_usec);
		output(TEST_FLAG_MULTIPLE_WRITE_APP, tt, (totalSize / (BUFFER_SIZE_S[size_type] * 4)) - 1);

		for (i = 0; i < 4; i++) {
			ret = lseek(fd_multiple[i], 0, SEEK_SET);
			if (ret < 0) {
				printf("Unable to move file pointer for file %s to start, ret = %d, errno = %d\n", buffer[i], ret, errno);
				return -1;
			}
		}

		gettimeofday(&wr_time_start, NULL);
		for(i = 0; i < (totalSize / (BUFFER_SIZE_S[size_type] * 4)) - 1; i++) {
			for(j = 0; j < 4; j++) {
				ret = perform_read(fd_multiple[j], size_type);
				if (ret != BUFFER_SIZE_S[size_type]) {
					printf("Unable to read from test file: %s, ret = ", buffer[j], ret);
					return -1;
				}
			}
		}
		gettimeofday(&wr_time_stop, NULL);
		tt = (wr_time_stop.tv_sec - wr_time_start.tv_sec) * 1000000 + (wr_time_stop.tv_usec - wr_time_start.tv_usec);
		output(TEST_FLAG_MULTIPLE_READ_SEQ, tt, (totalSize / (BUFFER_SIZE_S[size_type] * 4)) - 1);
		break;

	case TEST_FLAG_MULTIPLE_WRITE_OVE:
		printf("\n\n\n**********MULTIPLE_WRITE_OVE***** %d Bytes**********\n\n\n",BUFFER_SIZE_S[size_type]);
		gettimeofday(&wr_time_start, NULL);
		for (i = 0; i < (totalSize / (BUFFER_SIZE_S[size_type] * 4)) - 1; i++) {
			for (j = 0; j < 4; j++) {
				fd_multiple[j] = open(buffer[j], O_RDWR | O_CREAT | O_TRUNC);
				if (fd_multiple[j] < 0) {
					printf("Unable to open file = %s for testing, fd = %d\n", buffer[j], fd_multiple[j]);
					for (k = 0; k < j; k++) {
						close(fd_multiple[k]);
					}
					return -1;
                		}
				ret = perform_write(fd_multiple[j], size_type);
				if (ret != BUFFER_SIZE_S[size_type]) {
					printf("Unable to do test performance on file: %s, ret = %d\n", buffer[j], ret);
					for (k = 0; k < 4; k++) {
						close(fd_multiple[k]);
					}
					return -1;
				}
				close(fd_multiple[j]);
			}
		}
		gettimeofday(&wr_time_stop, NULL);
                tt = (wr_time_stop.tv_sec - wr_time_start.tv_sec) * 1000000 + (wr_time_stop.tv_usec - wr_time_start.tv_usec);
                output(TEST_FLAG_MULTIPLE_WRITE_OVE, tt, (totalSize / (BUFFER_SIZE_S[size_type] * 4)) - 1);

		gettimeofday(&wr_time_start, NULL);
		for (i = 0; i < (totalSize / (BUFFER_SIZE_S[size_type] * 4)) - 1; i++) {
			for (j = 0; j < 4; j++) {
				fd_multiple[j] = open(buffer[j], O_RDONLY);
				if (fd_multiple[j] < 0) {
					printf("Unable to open file = %s for testing, fd = %d\n", buffer[j], fd_multiple[j]);
					for (k = 0; k < j; k++) {
						close(fd_multiple[k]);
					}
					return -1;
				}
				ret = perform_read(fd_multiple[j],size_type);
				if (ret != BUFFER_SIZE_S[size_type]) {
					printf("Unable to do read test performance on file: %s, ret = %d\n", buffer[j], ret);
					for (k = 0; k < 4; k++) {
						close(fd_multiple[k]);
					}
					return -1;
				}
				close(fd_multiple[j]);
			}
		}
		gettimeofday(&wr_time_stop, NULL);
		tt = (wr_time_stop.tv_sec - wr_time_start.tv_sec) * 1000000 + (wr_time_stop.tv_usec - wr_time_start.tv_usec);
		output(TEST_FLAG_MULTIPLE_READ_OVE, tt, (totalSize / (BUFFER_SIZE_S[size_type] * 4)) - 1);

		for (i = 0; i < 4; i++) {
			ret = unlink(buffer[i]);
			if (ret < 0) {
				printf("Unable to delet file: %s, ret = %d\n", buffer[i], ret);
				return -1;
			}
		}
		break;

	case TEST_FLAG_SINGLE_WRITE_MIX:
		printf("\n\n\n**********SINGLE_WRITE_MIX***************\n\n\n");
		gettimeofday(&wr_time_start, NULL);
		for(i = 0; i < 56; i++) {
			fd = open(SINGLE_TEST_FILE, O_WRONLY | O_CREAT);
			if(fd < 0) {
				printf("Unable to do test perfomance on file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
				return -1;
			}
			
			for (j = 0; j < 8; j++) {
				ret = perform_write(fd, size_type);
				if (ret != BUFFER_SIZE_S[size_type]) {
					printf("Unable to do test performance on file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
					close(fd);
					return -1;
				}
			}
			close(fd);
			unlink(SINGLE_TEST_FILE);
		}
		gettimeofday(&wr_time_stop, NULL);
		tt = (wr_time_stop.tv_sec - wr_time_start.tv_sec) * 1000000 + (wr_time_stop.tv_usec - wr_time_start.tv_usec);
		output(TEST_FLAG_SINGLE_WRITE_MIX, tt, 56);
		break;

	case TEST_FLAG_INCREMENTAL_OVERWRITE:
		//printf("\n**********SINGLE_INCREMENTAL WRITE 2kb TizenRT , %d **************\n",size_type);
		gettimeofday(&wr_time_start, NULL);
		for (j = 0; j < N_FILES; j++) {
			//fd_multiple[j] = open(buffer[j], O_RDWR | O_TRUNC);
			fd_multiple[j] = open(buffer[j], O_RDWR);
			if (!fd_multiple[j]) {
                                printf("Unable to do open file : %s, for read ret = %d\n", buffer[j], fd_multiple[j]);
                                for (k = 0; k < j; k++) {
                                        close(fd_multiple[k]);
                                }
                                return -1;
                        }
			ret = lseek(fd_multiple[j], 0, SEEK_SET);
			if (ret < 0) {
				printf("Unable to move file pointer to start\n");
				for (k = 0; k < j; k++) {
					close(fd_multiple[j]);
				}
				return -1;
			}

			for (i = 0; i < (32 * size_type); i++) {
				//ret = write(fd_multiple[j], rw_buf_1kb,1*1024);
				ret = write(fd_multiple[j], rw_buf_2kb, 2*1024);
				if (ret != (2 * 1024)) {
					printf("Unable to do test performance write on file: %s,iter= %d ,ret = %d\n", buffer[j], i, ret);
					for (k = 0; k < j; k++) {
						close(fd_multiple[k]);
					}
					return -1;
				}
			}
			close(fd_multiple[j]);
		}
		gettimeofday(&wr_time_stop, NULL);
		tt = (wr_time_stop.tv_sec - wr_time_start.tv_sec)*1000000 + (wr_time_stop.tv_usec - wr_time_start.tv_usec);
		time_write_inc_ove[size_type] += tt;
		//output(TEST_FLAG_INCREMENTAL_OVERWRITE, tt, 32*size_type);

		// calculating the size of the file
		fd = open(buffer[N_FILES - 1], O_RDONLY);
		if (fd < 0) {
			printf("Unable to do test perfomance to calucalte size on file: %s,at ret = %d\n", buffer[N_FILES - 1], ret);
			return -1;
		}
		res2 = lseek(fd, 0, SEEK_END);
		if (res2 < 0) {
			printf("Unable to move file pointer to end\n");
			close(fd);
			return -1;
		}
		//printf("2. size of file (%s) is %d bytes\n", buffer[N_FILES - 1], res2);
		close(fd);

		// incremental read

		gettimeofday(&wr_time_start, NULL);

		for(j = 0; j < N_FILES; j++) {
			fd_multiple[j] = open(buffer[j], O_RDONLY);
			if (fd_multiple[j] < 0) {
                                printf("Unable to open file: %s, for read , ret = %d\n", buffer[j], fd_multiple[j]);
                                for (k = 0; k < j; k++) {
                                        close(fd_multiple[k]);
                                }
                                return -1;
                        }
			ret = lseek(fd_multiple[j], 0, SEEK_SET);
			if (ret < 0) {
				printf("Unable to move file pointer to start\n");
				for (k = 0; k < j; k++) {
					close(fd_multiple[j]);
				}
				return -1;
			}
			for(i = 0; i < (32* size_type); i++) {
				//ret = read(fd_multiple[j], rw_buf_1kb,1*1024);
				ret = read(fd_multiple[j], rw_buf_2kb, 2*1024);
				if (ret != (2 * 1024)) {
					printf("Unable to do test performance read on file: %s, ret = %d\n", buffer[j], ret);
					for (k = 0; k < j; k++) {
                                                close(fd_multiple[k]);
                                        }
					return -1;
				}
			}
			close(fd_multiple[j]);
		}
		//printf("ret = %d byte read\n",ret);

		gettimeofday(&wr_time_stop, NULL);
		tt = (wr_time_stop.tv_sec - wr_time_start.tv_sec) * 1000000 + (wr_time_stop.tv_usec - wr_time_start.tv_usec);
		time_read_inc_ove[size_type] += tt;
		//output(TEST_FLAG_INCREMENTAL_OVERREAD, tt, 32*size_type);
		break;

	default:
		break;
	}
	return ret;
}

static int init(int size_type)
{
	int ret;
	int fd_sample = 0;
	int i;

	for (i = 0; i < 35; i++) {
		time_write_inc_ove[i] = 0;
		time_read_inc_ove[i] = 0;
	}
	//Initializing sample file
	printf("initalizing lseek %s file lfs with ftl  \n",SAMPLE_FILE);
        ret = init_sample(5120);
	if (ret < 0) {
		printf("Unable to initalize the sample file: %s, ret = %d\n", SAMPLE_FILE, ret);
		return ret;
	}

	fd_sample = open(SAMPLE_FILE, O_RDONLY);
	if (fd_sample < 0) {
		printf("Unable to open sample file for reading, ret = %d\n", ret);
		return ret;
	}

        printf("Populating in buffer started ");

	memset(rw_buf_1kb, 0, 1024 + 1);
        ret = read(fd_sample, rw_buf_1kb,1024);
        if (ret != 1024) {
        	printf("Unable to read file into 1kb buffer\n");
                close(fd_sample);
		return -1;
	}

	//printf("bytes written in 1kb buffer = %d bytes\n",ret);

	ret = lseek(fd_sample, 0, SEEK_SET);
	if (ret < 0) {
		printf("Unable to move file pointer to start\n");
		close(fd_sample);
		return -1;
	}

	//printf(" buffer content 1kb- \n %s \n\n\n", rw_buf_1kb);
	memset(rw_buf_2kb, 0, (2 * 1024) + 1);
	ret = read(fd_sample, rw_buf_2kb, 2048);
	if (ret != 2048) {
		printf("Unable to read file into 2kb\n");
		close(fd_sample);
		return -1;
	}
	ret = lseek(fd_sample, 0, SEEK_SET);
	if (ret < 0) {
		printf("Unable to move file pointer to start\n");
		close(fd_sample);
		return -1;
	}

	close(fd_sample);

	for (i = 0; i < N_FILES; i++) {
		snprintf(buffer[i], 27, "%s%d", MULTI_TEST_FILE, i);
	}

	return OK;
}

static void initalizeBuffer() {
	int i;
	BUFFER_SIZE_S[0] = 1800;
	for(i = 1; i <= 3; i++) {
		BUFFER_SIZE_S[i]= 5*1024*i;
	}
}

static int createDirectory() {
	int ret;
	ret = mkdir(SINGLE_TEST_FILE_DIR, 0777);
	if (ret < 0) {
		printf("Unable to create directory for single file testing, ret = %d\n", ret);
		return -1;
	}

	ret = mkdir(MULTI_TEST_FILE_DIR, 0777);
	if (ret < 0) {
		printf("Unable to create directory for multiple file testing, ret = %d\n", ret);
		return -1;
	}
	// calculating the buffer sizes in multiple of 5kb
	initalizeBuffer();
	printf(" Directory created & buffer of different size initialized\n");
	return 0;
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int fs_performance_main(int argc, char *argv[])
#endif
{
	/*if (argc < 2) {
		printf("Invalid input, argc = %d\n", argc);
		goto end;
	}*/

	int option = 6;// atoi(argv[1]);	//Which test case to be executed
	int ret = 0;
	int fd_single = 0;		        //File Descriptor for single file in test environment
	int fd_single2 = 0;                     //File Descriptor for incremental overwrite case
	int i;
	int j;
	int k;
	int res2;
	int size_type;
        int write_itr;
	int outer_loop;

	printf("\nChosen test option = %d\n\n", option);

	printf("11.initializing test files\n");
        ret = init(1);
	
	if (ret < 0) {
		printf("Failed to initialize test case parameters, ret = %d\n", ret);
		goto errout;
	}
	printf("Initialization complete\n\n");

       switch (option) {

	case 1: goto START;//SINGLE_APP;

	case 2: goto START;//SINGLE_OVE;

	case 3: goto START;//MULTIPLE_APP;

	case 4: goto START;//MULTIPLE_OVE;

	case 5: goto START;//SINGLE_MIX;

	case 6: goto INCREMENTAL_OVE;

	default: break;
	}


INCREMENTAL_OVE:
       // create 64kb file and overwrite and overread 1kb data 64 times...
       // append 64kb data to the file and overwrite and overread 1kb data 128 times..
       // do the above with 1kb and 2kb buffer size to write and read. 
       for(outer_loop = 1; outer_loop <= MAX_ITR ; outer_loop++) {
	       printf("incremental overwrite started outer loop = %d \n",outer_loop);
	       for( j = 0; j < N_FILES; j++) {
		       fd_multiple[j] = open(buffer[j], O_RDWR | O_CREAT );
		       if (fd_multiple[j] < 0) {
			       printf("Unable to open file %s for testing, fd = %d\n", buffer[j], fd_multiple[j]);
			       for (k = 0; k < j; k++) {
				       close(fd_multiple[k]);
			       }
			       return -1;
		       }
		       close(fd_multiple[j]);
		}
		for (size_type = 1; size_type <= FILE_SIZE; size_type++) {
			printf("\rsize: type :::: %d ",size_type);
			for (j = 0; j < N_FILES; j++ ) {
				fd_multiple[j] = open(buffer[j], O_RDWR);
				if (fd_multiple[j] < 0) {
					printf("Unable to open file for testing, fd = %d\n", fd_multiple[j]);
					for (k = 0; k < j; k++) {
						close(fd_multiple[k]);
					}
					return -1;
				}
				res2 =lseek(fd_multiple[j], 0, SEEK_END);
				if (res2 < 0) {
					printf("Unable to move file pointer to end\n");
					close(fd_multiple[j]);
					return -1;
				}
				for (write_itr = 0; write_itr < 64; write_itr++) {
					ret  = write(fd_multiple[j], rw_buf_1kb, 1024);
					if(ret != 1024) {
						printf("Unable to test performance on file: %s, ret = %d,write_itr = %d \n", buffer[j], ret,write_itr);
						for (k = 0; k < N_FILES; k++) {
							close(fd_multiple[k]);
						}
						return -1;
					}
				}
				close(fd_multiple[j]);
			}
			// calculating the size of the file
			fd_single2 = open(buffer[N_FILES - 1], O_RDONLY);
                	if (fd_single2 < 0) {
				printf("Unable to open file: %s,at ret = %d\n", buffer[N_FILES - 1], ret);
				return -1;
                	}
			res2 =lseek(fd_single2, 0, SEEK_END);
			if (res2 < 0) {
				printf("Unable to move file pointer to end\n");
				close(fd_single2);
				return -1;
			}
			close(fd_single2);
			//printf(" 1.%s size = %d bytes \t", buffer[N_FILES - 1], res2);

			ret = do_test(fd_single2, TEST_FLAG_INCREMENTAL_OVERWRITE,size_type);

			if (ret < 0) {
				printf("Unable to test performance (incremental overwrite), ret = %d\n", ret);
				return -1;
			}

			// calculating the size of the file
			fd_single2 = open(buffer[N_FILES - 1], O_RDONLY);
			if (fd_single2 < 0) {
				printf("Unable to open file: %s,at ret = %d\n", buffer[N_FILES - 1], ret);
				return -1;
			}
               		res2 = lseek(fd_single2, 0, SEEK_END);
			if (res2 < 0) {
                                printf("Unable to move file pointer to end\n");
                                close(fd_single2);
                                return -1;
                        }
			close(fd_single2);
			//printf(" 3.%s size = %d bytes\n", buffer[N_FILES - 1], res2);

            	}
		for (i = 0; i < N_FILES && outer_loop != MAX_ITR; i++) {
			ret = unlink(buffer[i]);
			if (ret < 0) {
				printf("Unable to remove file: %s, ret = %d\n", buffer[i], ret);
				goto end;
			}
		}

		printf("outer loop %d ends\n",outer_loop);
		if ((outer_loop % 10) == 0 || outer_loop == 1 || outer_loop == MAX_ITR) {
                        for( i = 1 ; i <= FILE_SIZE; i++) {
                                printf("%d - > %llu us - &  %llu us outer_loop = %d \n",i, time_write_inc_ove[i], time_read_inc_ove[i], outer_loop);
                        sleep(2);
			}
                }

		if (outer_loop == MAX_ITR) {
			goto errout;
		}
	}
	return 0;

START:
        createDirectory();
        for(size_type =0;size_type<=3;size_type++) {
	printf("\n*****************######################*******************Buffersize = %d && size_type no = %d ******************#################*****************\n",BUFFER_SIZE_S[size_type],size_type);
    	//creating a sample file and populating the buffer rw_buf_s_size_type with BUFFER_SIZE_S[size_type] kb;
    	//rw_buf_s_1800b =1800 bytes
    	// rw_buf_s_1 = 64kb data
    	// rw_buf_s_2 = 2*64 = 128 kb data and son on ...
    	ret = init(size_type);
    
   	 if (ret < 0) {
         	printf("Failed to initialize test case parameters, ret = %d\n", ret);
         	goto errout;
    	}
    
    	printf("Initialization complete\n\n");

    	for (int itr = 0; itr < 5; itr++) {
        	printf("\n#################################Iteration = %d ########################################\n",itr);

		SINGLE_APP:	
			//Do test on single file for continuous writing
			fd_single = open(SINGLE_TEST_FILE, O_RDWR | O_CREAT);
			if (fd_single < 0) {
				printf("Unable to open file for testing, fd = %d\n", fd_single);
				return -1;
			}

			ret = do_test(fd_single, TEST_FLAG_SINGLE_WRITE_APP, size_type);
			if (ret < 0) {
				printf("Unable to test performance on file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
				return -1;
			}

			close(fd_single);
			unlink(SINGLE_TEST_FILE);
			//goto end;

		SINGLE_OVE:
			//Do test on single file for overwriting
			ret = do_test(fd_single, TEST_FLAG_SINGLE_WRITE_OVE, size_type);
			if (ret < 0) {
                		printf("Unable to test performance on file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
                		return -1;
        		}
			printf("end single ove \n");
			//goto end;

		MULTIPLE_APP:
			//Initilaize set of files for multiple file testing
			printf("inside multi app \n");
			for (i = 0; i < 4; i++) {
        			fd_multiple[i] = open(buffer[i], O_RDWR | O_CREAT);
        			if (fd_multiple[i] < 0) {
                			printf("Unable to open file = !!!%s!!! for testing, fd = !!!%d!!!, errno = %d\n", buffer[i], fd_multiple[i], errno);
                			ret = fd_multiple[i];
					goto errout;
        			}
				close(fd_multiple[i]);
			}


			for (i = 0; i < 4; i++) {
                		fd_multiple[i] = open(buffer[i], O_RDWR);
				if (fd_multiple[i] < 0) {
					printf("Unable to retrieve fd for %dth file, fd = %d, errno = %d\n", (i + 1), fd_multiple[i], errno);
					goto errout;
				}
        		}

			printf("Multiple file initialization complete\n");
			sleep(3);

        		ret = do_test(0, TEST_FLAG_MULTIPLE_WRITE_APP, size_type);
        		if (ret < 0) {
                		printf("Unable to test performance on multiple files, ret = %d\n", ret);
                		return -1;
        		}
	
			for (i = 0; i < 4; i++) {
                		close(fd_multiple[i]);
				ret = unlink(buffer[i]);
				if (ret < 0) {
					printf("Unable to remove file: %s, ret = %d\n", buffer[i], ret);
					goto end;
				}
			}
			//goto end;

		MULTIPLE_OVE:
			//Do test on mutliple file for overwriting
        		ret = do_test(0, TEST_FLAG_MULTIPLE_WRITE_OVE, size_type);
        		if (ret < 0) {
                		printf("Unable to test performance on file: %s, ret = %d\n", MULTI_TEST_FILE, ret);
                		return -1;
        		}
			//goto end;

		SINGLE_MIX:
			//Do test on single file for writing 7-8 times and delete it. Repeat k iteration
			ret = do_test(0, TEST_FLAG_SINGLE_WRITE_MIX, size_type);
			if (ret < 0) {
				printf("Unable to test performance on file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
				return -1;
			}

		end:
			ret = OK;
               printf("\n###################################################################################\n");

	}
	}

	errout:
		printf("Filesystem Performance Test Example Exits\n");
	return ret;
}

/* 
 * All tests should be conducted on an erased partitions (menu option)
 * Analyze gradual increase in write times as partition gets full
 * All tests with small and large buffers AND single and multiple files
 */
