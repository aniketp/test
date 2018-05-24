/*-
 * Copyright 2018 Aniket Pandey
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <sys/stat.h>
#include <sys/mman.h>

#include <atf-c.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils.h"

static struct pollfd fds[1];
static mode_t mode = 0777;
static char extregex[80];
static struct stat statbuff;
static const char *path = "fileforaudit";
static const char *errpath = "dirdoesnotexist/fileforaudit";
static const char *failurereg = "fileforaudit.*return,failure";


ATF_TC_WITH_CLEANUP(munmap_success);
ATF_TC_HEAD(munmap_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"munmap(2) call");
}

ATF_TC_BODY(munmap_success, tc)
{
	int pagesize = sysconf(_SC_PAGESIZE);
	const char *regex = "munmap.*return,success";
	char *addr = mmap(NULL, 4 * pagesize, PROT_READ , MAP_ANONYMOUS, -1, 0);
	FILE *pipefd = setup(fds, "cl");
	ATF_REQUIRE_EQ(0, munmap(addr, pagesize));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(munmap_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(munmap_failure);
ATF_TC_HEAD(munmap_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"munmap(2) call");
}

ATF_TC_BODY(munmap_failure, tc)
{
	const char *regex = "munmap.*return,failure : Invalid argument";
	FILE *pipefd = setup(fds, "cl");
	ATF_REQUIRE_EQ(-1, munmap((void *)SIZE_MAX, -1));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(munmap_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(close_success);
ATF_TC_HEAD(close_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"close(2) call");
}

ATF_TC_BODY(close_success, tc)
{
	int filedesc;
	/* File needs to exist to call close(2) */
	ATF_REQUIRE((filedesc = open(path, O_CREAT | O_RDWR, mode)) != -1);
	/* Call stat(2) to store the Inode number of 'path' */
	ATF_REQUIRE_EQ(0, stat(path, &statbuff));
	FILE *pipefd = setup(fds, "cl");
	ATF_REQUIRE_EQ(0, close(filedesc));

	snprintf(extregex, 80, "close.*%lu.*return,success", statbuff.st_ino);
	check_audit(fds, extregex, pipefd);
}

ATF_TC_CLEANUP(close_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(close_failure);
ATF_TC_HEAD(close_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"close(2) call");
}

ATF_TC_BODY(close_failure, tc)
{
	const char *regex = "close.*return,failure";
	FILE *pipefd = setup(fds, "cl");
	/* Failure reason: file does not exist */
	ATF_REQUIRE_EQ(-1, close(-1));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(close_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(revoke_failure);
ATF_TC_HEAD(revoke_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"revoke(2) call");
}

ATF_TC_BODY(revoke_failure, tc)
{
	FILE *pipefd = setup(fds, "cl");
	/* Failure reason: file does not exist */
	ATF_REQUIRE_EQ(-1, revoke(errpath));
	check_audit(fds, failurereg, pipefd);
}

ATF_TC_CLEANUP(revoke_failure, tc)
{
	cleanup();
}


ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, munmap_success);
	ATF_TP_ADD_TC(tp, munmap_failure);
	ATF_TP_ADD_TC(tp, close_success);
	ATF_TP_ADD_TC(tp, close_failure);
	ATF_TP_ADD_TC(tp, revoke_failure);

	return (atf_no_error());
}