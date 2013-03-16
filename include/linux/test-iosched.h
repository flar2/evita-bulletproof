/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * The test scheduler allows to test the block device by dispatching
 * specific requests according to the test case and declare PASS/FAIL
 * according to the requests completion error code.
 * Each test is exposed via debugfs and can be triggered by writing to
 * the debugfs file.
 *
 */

#ifndef _LINUX_TEST_IOSCHED_H
#define _LINUX_TEST_IOSCHED_H

#define TEST_PATTERN_SEQUENTIAL	-1
#define TEST_PATTERN_5A		0x5A5A5A5A
#define TEST_PATTERN_FF		0xFFFFFFFF
#define TEST_NO_PATTERN		0xDEADBEEF
#define BIO_U32_SIZE 1024

struct test_data;

typedef int (prepare_test_fn) (struct test_data *);
typedef int (run_test_fn) (struct test_data *);
typedef int (check_test_result_fn) (struct test_data *);
typedef int (post_test_fn) (struct test_data *);
typedef char* (get_test_case_str_fn) (struct test_data *);
typedef void (blk_dev_test_init_fn) (void);
typedef void (blk_dev_test_exit_fn) (void);

enum test_state {
	TEST_IDLE,
	TEST_RUNNING,
	TEST_COMPLETED,
};

enum test_results {
	TEST_NO_RESULT,
	TEST_FAILED,
	TEST_PASSED,
	TEST_NOT_SUPPORTED,
};

enum req_unique_type {
	REQ_UNIQUE_NONE,
	REQ_UNIQUE_DISCARD,
	REQ_UNIQUE_FLUSH,
};

struct test_debug {
	struct dentry *debug_root;
	struct dentry *debug_utils_root;
	struct dentry *debug_tests_root;
	struct dentry *debug_test_result;
	struct dentry *start_sector;
};

struct test_request {
	struct list_head queuelist;
	unsigned int *bios_buffer;
	int buf_size;
	struct request *rq;
	bool req_completed;
	int req_result;
	int is_err_expected;
	int wr_rd_data_pattern;
	int req_id;
};

struct test_info {
	int testcase;
	unsigned timeout_msec;
	prepare_test_fn *prepare_test_fn;
	run_test_fn *run_test_fn;
	check_test_result_fn *check_test_result_fn;
	post_test_fn *post_test_fn;
	get_test_case_str_fn *get_test_case_str_fn;
	void *data;
};

struct blk_dev_test_type {
	struct list_head list;
	blk_dev_test_init_fn *init_fn;
	blk_dev_test_exit_fn *exit_fn;
};

struct test_data {
	struct list_head queue;
	struct list_head test_queue;
	struct test_request *next_req;
	wait_queue_head_t wait_q;
	enum test_state test_state;
	enum test_results test_result;
	struct test_debug debug;
	struct request_queue *req_q;
	int num_of_write_bios;
	u32 start_sector;
	struct timer_list timeout_timer;
	int wr_rd_next_req_id;
	int unique_next_req_id;
	spinlock_t lock;
	struct test_info test_info;
	bool fs_wr_reqs_during_test;
	bool ignore_round;
};

extern int test_iosched_start_test(struct test_info *t_info);
extern void test_iosched_mark_test_completion(void);
extern int test_iosched_add_unique_test_req(int is_err_expcted,
		enum req_unique_type req_unique,
		int start_sec, int nr_sects, rq_end_io_fn *end_req_io);
extern int test_iosched_add_wr_rd_test_req(int is_err_expcted,
	      int direction, int start_sec,
	      int num_bios, int pattern, rq_end_io_fn *end_req_io);

extern struct dentry *test_iosched_get_debugfs_tests_root(void);
extern struct dentry *test_iosched_get_debugfs_utils_root(void);

extern struct request_queue *test_iosched_get_req_queue(void);

extern void test_iosched_set_test_result(int);

void test_iosched_set_ignore_round(bool ignore_round);

void test_iosched_register(struct blk_dev_test_type *bdt);

void test_iosched_unregister(struct blk_dev_test_type *bdt);

#endif 
