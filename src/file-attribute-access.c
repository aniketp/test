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
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/extattr.h>

#include <atf-c.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

static struct pollfd fds[1];

static fhandle_t fht;
static mode_t mode = 0777;
static char extregex[80];
static struct stat statbuff;
static struct statfs statfsbuff;
static const char *name = "authorname";
static const char *path = "fileforaudit";
static const char *errpath = "dirdoesnotexist/fileforaudit";
static const char *successreg = "fileforaudit.*return,success";
static const char *failurereg = "fileforaudit.*return,failure";


ATF_TC_WITH_CLEANUP(stat_success);
ATF_TC_HEAD(stat_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"stat(2) call");
}

ATF_TC_BODY(stat_success, tc)
{
	/* File needs to exist to call stat(2) */
	ATF_REQUIRE(open(path, O_CREAT, mode) != -1);
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(0, stat(path, &statbuff));
	check_audit(fds, successreg, pipefd);
}

ATF_TC_CLEANUP(stat_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(stat_failure);
ATF_TC_HEAD(stat_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"stat(2) call");
}

ATF_TC_BODY(stat_failure, tc)
{
	FILE *pipefd = setup(fds, "fa");
	/* Failure reason: file does not exist */
	ATF_REQUIRE_EQ(-1, stat(errpath, &statbuff));
	check_audit(fds, failurereg, pipefd);
}

ATF_TC_CLEANUP(stat_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(lstat_success);
ATF_TC_HEAD(lstat_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"lstat(2) call");
}

ATF_TC_BODY(lstat_success, tc)
{
	/* Symbolic link needs to exist to call lstat(2) */
	ATF_REQUIRE_EQ(0, symlink("symlink", path));
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(0, lstat(path, &statbuff));
	check_audit(fds, successreg, pipefd);
}

ATF_TC_CLEANUP(lstat_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(lstat_failure);
ATF_TC_HEAD(lstat_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"lstat(2) call");
}

ATF_TC_BODY(lstat_failure, tc)
{
	FILE *pipefd = setup(fds, "fa");
	/* Failure reason: symbolic link does not exist */
	ATF_REQUIRE_EQ(-1, lstat(errpath, &statbuff));
	check_audit(fds, failurereg, pipefd);
}

ATF_TC_CLEANUP(lstat_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fstat_success);
ATF_TC_HEAD(fstat_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"fstat(2) call");
}

ATF_TC_BODY(fstat_success, tc)
{
	int filedesc;
	char regex[30];

	/* File needs to exist to call fstat(2) */
	ATF_REQUIRE((filedesc = open(path, O_CREAT | O_RDWR, mode)) != -1);
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(0, fstat(filedesc, &statbuff));

	snprintf(regex, 30, "fstat.*%u.*return,success", statbuff.st_ino);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(fstat_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fstat_failure);
ATF_TC_HEAD(fstat_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"fstat(2) call");
}

ATF_TC_BODY(fstat_failure, tc)
{
	FILE *pipefd = setup(fds, "fa");
	const char *regex = "fstat.*return,failure : Bad file descriptor";
	/* Failure reason: bad file descriptor */
	ATF_REQUIRE_EQ(-1, fstat(-1, &statbuff));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(fstat_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fstatat_success);
ATF_TC_HEAD(fstatat_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"fstatat(2) call");
}

ATF_TC_BODY(fstatat_success, tc)
{
	/* File or Symbolic link needs to exist to call lstat(2) */
	ATF_REQUIRE_EQ(0, symlink("symlink", path));
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(0, fstatat(AT_FDCWD, path, &statbuff, \
		AT_SYMLINK_NOFOLLOW));
	check_audit(fds, successreg, pipefd);
}

ATF_TC_CLEANUP(fstatat_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fstatat_failure);
ATF_TC_HEAD(fstatat_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"fstatat(2) call");
}

ATF_TC_BODY(fstatat_failure, tc)
{
	FILE *pipefd = setup(fds, "fa");
	/* Failure reason: symbolic link does not exist */
	ATF_REQUIRE_EQ(-1, fstatat(AT_FDCWD, path, &statbuff, \
		AT_SYMLINK_NOFOLLOW));
	check_audit(fds, failurereg, pipefd);
}

ATF_TC_CLEANUP(fstatat_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(statfs_success);
ATF_TC_HEAD(statfs_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"statfs(2) call");
}

ATF_TC_BODY(statfs_success, tc)
{
	/* File needs to exist to call statfs(2) */
	ATF_REQUIRE(open(path, O_CREAT, mode) != -1);
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(0, statfs(path, &statfsbuff));
	check_audit(fds, successreg, pipefd);
}

ATF_TC_CLEANUP(statfs_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(statfs_failure);
ATF_TC_HEAD(statfs_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"statfs(2) call");
}

ATF_TC_BODY(statfs_failure, tc)
{
	FILE *pipefd = setup(fds, "fa");
	/* Failure reason: file does not exist */
	ATF_REQUIRE_EQ(-1, statfs(errpath, &statfsbuff));
	check_audit(fds, failurereg, pipefd);
}

ATF_TC_CLEANUP(statfs_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fstatfs_success);
ATF_TC_HEAD(fstatfs_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"fstatfs(2) call");
}

ATF_TC_BODY(fstatfs_success, tc)
{
	int filedesc;
	char regex[30];

	/* File needs to exist to call fstat(2) */
	ATF_REQUIRE((filedesc = open(path, O_CREAT | O_RDWR, mode)) != -1);
	/* Call stat(2) to store the Inode number of 'path' */
	ATF_REQUIRE_EQ(0, stat(path, &statbuff));
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(0, fstatfs(filedesc, &statfsbuff));

	snprintf(regex, 30, "fstatfs.*%u.*return,success", statbuff.st_ino);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(fstatfs_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fstatfs_failure);
ATF_TC_HEAD(fstatfs_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"fstatfs(2) call");
}

ATF_TC_BODY(fstatfs_failure, tc)
{
	FILE *pipefd = setup(fds, "fa");
	const char *regex = "fstatfs.*return,failure : Bad file descriptor";
	/* Failure reason: bad file descriptor */
	ATF_REQUIRE_EQ(-1, fstatfs(-1, &statfsbuff));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(fstatfs_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(getfsstat_success);
ATF_TC_HEAD(getfsstat_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"getfsstat(2) call");
}

ATF_TC_BODY(getfsstat_success, tc)
{
	const char *regex = "getfsstat.*return,success";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(getfsstat(NULL, 0, MNT_NOWAIT) != -1);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(getfsstat_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(getfsstat_failure);
ATF_TC_HEAD(getfsstat_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"getfsstat(2) call");
}

ATF_TC_BODY(getfsstat_failure, tc)
{
	const char *regex = "getfsstat.*return,failure : Invalid argument";
	FILE *pipefd = setup(fds, "fa");
	/* Failure reason: Invalid value for mode */
	ATF_REQUIRE_EQ(-1, getfsstat(NULL, 0, -1));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(getfsstat_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fhopen_success);
ATF_TC_HEAD(fhopen_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"fhopen(2) call");
}

ATF_TC_BODY(fhopen_success, tc)
{
	const char *regex = "fhopen.*return,success";
	/* File needs to exist to get a file-handle */
	ATF_REQUIRE(open(path, O_CREAT, mode) != -1);
	/* Get the file handle to be passed to fhopen(2) */
	ATF_REQUIRE_EQ(0, getfh(path, &fht));

	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(fhopen(&fht, O_RDWR) != -1);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(fhopen_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fhopen_failure);
ATF_TC_HEAD(fhopen_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"fhopen(2) call");
}

ATF_TC_BODY(fhopen_failure, tc)
{
	const char *regex = "fhopen.*return,failure : Stale NFS file handle";
	FILE *pipefd = setup(fds, "fa");
	/* Failure reason: fht does not represent any file */
	ATF_REQUIRE_EQ(-1, fhopen(&fht, O_RDWR));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(fhopen_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fhstat_success);
ATF_TC_HEAD(fhstat_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"fstat(2) call");
}

ATF_TC_BODY(fhstat_success, tc)
{
	const char *regex = "fhstat.*return,success";
	/* File needs to exist to get a file-handle */
	ATF_REQUIRE(open(path, O_CREAT, mode) != -1);
	/* Get the file handle to be passed to fhstat(2) */
	ATF_REQUIRE_EQ(0, getfh(path, &fht));

	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(0, fhstat(&fht, &statbuff));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(fhstat_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fhstat_failure);
ATF_TC_HEAD(fhstat_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"fhstat(2) call");
}

ATF_TC_BODY(fhstat_failure, tc)
{
	const char *regex = "fhstat.*return,failure : Stale NFS file handle";
	FILE *pipefd = setup(fds, "fa");
	/* Failure reason: fht does not represent any file */
	ATF_REQUIRE_EQ(-1, fhstat(&fht, &statbuff));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(fhstat_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fhstatfs_success);
ATF_TC_HEAD(fhstatfs_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"fstatfs(2) call");
}

ATF_TC_BODY(fhstatfs_success, tc)
{
	const char *regex = "fhstatfs.*return,success";
	/* File needs to exist to get a file-handle */
	ATF_REQUIRE(open(path, O_CREAT, mode) != -1);
	/* Get the file handle to be passed to fhstatfs(2) */
	ATF_REQUIRE_EQ(0, getfh(path, &fht));

	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(0, fhstatfs(&fht, &statfsbuff));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(fhstatfs_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fhstatfs_failure);
ATF_TC_HEAD(fhstatfs_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"fhstatfs(2) call");
}

ATF_TC_BODY(fhstatfs_failure, tc)
{
	const char *regex = "fhstatfs.*return,failure : Stale NFS file handle";
	FILE *pipefd = setup(fds, "fa");
	/* Failure reason: fht does not represent any file */
	ATF_REQUIRE_EQ(-1, fhstatfs(&fht, &statfsbuff));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(fhstatfs_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(access_success);
ATF_TC_HEAD(access_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"access(2) call");
}

ATF_TC_BODY(access_success, tc)
{
	/* File needs to exist to call access(2) */
	ATF_REQUIRE(open(path, O_CREAT, mode) != -1);
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(0, access(path, F_OK));
	check_audit(fds, successreg, pipefd);
}

ATF_TC_CLEANUP(access_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(access_failure);
ATF_TC_HEAD(access_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"access(2) call");
}

ATF_TC_BODY(access_failure, tc)
{
	FILE *pipefd = setup(fds, "fa");
	/* Failure reason: file does not exist */
	ATF_REQUIRE_EQ(-1, access(errpath, F_OK));
	check_audit(fds, failurereg, pipefd);
}

ATF_TC_CLEANUP(access_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(eaccess_success);
ATF_TC_HEAD(eaccess_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"eaccess(2) call");
}

ATF_TC_BODY(eaccess_success, tc)
{
	/* File needs to exist to call eaccess(2) */
	ATF_REQUIRE(open(path, O_CREAT, mode) != -1);
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(0, eaccess(path, F_OK));
	check_audit(fds, successreg, pipefd);
}

ATF_TC_CLEANUP(eaccess_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(eaccess_failure);
ATF_TC_HEAD(eaccess_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"eaccess(2) call");
}

ATF_TC_BODY(eaccess_failure, tc)
{
	FILE *pipefd = setup(fds, "fa");
	/* Failure reason: file does not exist */
	ATF_REQUIRE_EQ(-1, eaccess(errpath, F_OK));
	check_audit(fds, failurereg, pipefd);
}

ATF_TC_CLEANUP(eaccess_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(faccessat_success);
ATF_TC_HEAD(faccessat_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"faccessat(2) call");
}

ATF_TC_BODY(faccessat_success, tc)
{
	/* File needs to exist to call faccessat(2) */
	ATF_REQUIRE(open(path, O_CREAT, mode) != -1);
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(0, faccessat(AT_FDCWD, path, F_OK, AT_EACCESS));
	check_audit(fds, successreg, pipefd);
}

ATF_TC_CLEANUP(faccessat_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(faccessat_failure);
ATF_TC_HEAD(faccessat_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"faccessat(2) call");
}

ATF_TC_BODY(faccessat_failure, tc)
{
	FILE *pipefd = setup(fds, "fa");
	/* Failure reason: file does not exist */
	ATF_REQUIRE_EQ(-1, faccessat(AT_FDCWD, errpath, F_OK, AT_EACCESS));
	check_audit(fds, failurereg, pipefd);
}

ATF_TC_CLEANUP(faccessat_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(pathconf_success);
ATF_TC_HEAD(pathconf_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"pathconf(2) call");
}

ATF_TC_BODY(pathconf_success, tc)
{
	/* File needs to exist to call pathconf(2) */
	ATF_REQUIRE(open(path, O_CREAT, mode) != -1);
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(pathconf(path, _PC_NAME_MAX) != -1);
	check_audit(fds, successreg, pipefd);
}

ATF_TC_CLEANUP(pathconf_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(pathconf_failure);
ATF_TC_HEAD(pathconf_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"pathconf(2) call");
}

ATF_TC_BODY(pathconf_failure, tc)
{
	FILE *pipefd = setup(fds, "fa");
	/* Failure reason: file does not exist */
	ATF_REQUIRE_EQ(-1, pathconf(errpath, _PC_NAME_MAX));
	check_audit(fds, failurereg, pipefd);
}

ATF_TC_CLEANUP(pathconf_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(lpathconf_success);
ATF_TC_HEAD(lpathconf_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"lpathconf(2) call");
}

ATF_TC_BODY(lpathconf_success, tc)
{
	/* Symbolic link needs to exist to call lpathconf(2) */
	ATF_REQUIRE_EQ(0, symlink("symlink", path));
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(lpathconf(path, _PC_SYMLINK_MAX) != -1);
	check_audit(fds, successreg, pipefd);
}

ATF_TC_CLEANUP(lpathconf_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(lpathconf_failure);
ATF_TC_HEAD(lpathconf_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"lpathconf(2) call");
}

ATF_TC_BODY(lpathconf_failure, tc)
{
	FILE *pipefd = setup(fds, "fa");
	/* Failure reason: symbolic link does not exist */
	ATF_REQUIRE_EQ(-1, lpathconf(errpath, _PC_SYMLINK_MAX));
	check_audit(fds, failurereg, pipefd);
}

ATF_TC_CLEANUP(lpathconf_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fpathconf_success);
ATF_TC_HEAD(fpathconf_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"fpathconf(2) call");
}

ATF_TC_BODY(fpathconf_success, tc)
{
	int filedesc;
	const char *regex = "fpathconf.*return,success";
	/* File needs to exist to call fpathconf(2) */
	ATF_REQUIRE((filedesc = open(path, O_CREAT, mode)) != -1);
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(fpathconf(filedesc, _PC_NAME_MAX) != -1);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(fpathconf_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fpathconf_failure);
ATF_TC_HEAD(fpathconf_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"fpathconf(2) call");
}

ATF_TC_BODY(fpathconf_failure, tc)
{
	FILE *pipefd = setup(fds, "fa");
	const char *regex = "fpathconf.*return,failure : Bad file descriptor";
	/* Failure reason: Bad file descriptor */
	ATF_REQUIRE_EQ(-1, fpathconf(-1, _PC_NAME_MAX));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(fpathconf_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(extattr_get_file_success);
ATF_TC_HEAD(extattr_get_file_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"extattr_get_file(2) call");
}

ATF_TC_BODY(extattr_get_file_success, tc)
{
	const char *buff = "ezio";
	/* File needs to exist to call extattr_get_file(2) */
	ATF_REQUIRE(open(path, O_CREAT, mode) != -1);
	ATF_REQUIRE_EQ(sizeof(buff), extattr_set_file(path, \
		EXTATTR_NAMESPACE_USER, name, buff, sizeof(buff)));

	/* Prepare the regex to be checked in the audit record */
	snprintf(extregex, 80, "extattr_get_file.*%s.*%s.*return,success,%lu", \
		path, name, sizeof(buff));

	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(sizeof(buff), extattr_get_file(path, \
		EXTATTR_NAMESPACE_USER, name, NULL, 0));
	check_audit(fds, extregex, pipefd);
}

ATF_TC_CLEANUP(extattr_get_file_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(extattr_get_file_failure);
ATF_TC_HEAD(extattr_get_file_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"extattr_get_file(2) call");
}

ATF_TC_BODY(extattr_get_file_failure, tc)
{
	/* Prepare the regex to be checked in the audit record */
	snprintf(extregex, 80, "extattr_get_file.*%s.*%s.*failure", path, name);

	FILE *pipefd = setup(fds, "fa");
	/* Failure reason: file does not exist */
	ATF_REQUIRE_EQ(-1, extattr_get_file(path, \
		EXTATTR_NAMESPACE_USER, name, NULL, 0));
	check_audit(fds, extregex, pipefd);
}

ATF_TC_CLEANUP(extattr_get_file_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(extattr_get_fd_success);
ATF_TC_HEAD(extattr_get_fd_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"extattr_get_fd(2) call");
}

ATF_TC_BODY(extattr_get_fd_success, tc)
{
	int filedesc;
	const char *buff = "ezio";
	/* File needs to exist to call extattr_get_fd(2) */
	ATF_REQUIRE((filedesc = open(path, O_CREAT, mode)) != -1);
	ATF_REQUIRE_EQ(sizeof(buff), extattr_set_file(path, \
		EXTATTR_NAMESPACE_USER, name, buff, sizeof(buff)));

	/* Prepare the regex to be checked in the audit record */
	snprintf(extregex, 80, "extattr_get_fd.*%s.*return,success", name);

	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(sizeof(buff), extattr_get_fd(filedesc, \
		EXTATTR_NAMESPACE_USER, name, NULL, 0));
	check_audit(fds, extregex, pipefd);
}

ATF_TC_CLEANUP(extattr_get_fd_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(extattr_get_fd_failure);
ATF_TC_HEAD(extattr_get_fd_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"extattr_get_fd(2) call");
}

ATF_TC_BODY(extattr_get_fd_failure, tc)
{
	/* Prepare the regex to be checked in the audit record */
	snprintf(extregex, 80, "extattr_get_fd.*%s.*return,failure : "
	"Bad file descriptor", name);

	FILE *pipefd = setup(fds, "fa");
	/* Failure reason: Invalid file descriptor */
	ATF_REQUIRE_EQ(-1, extattr_get_fd(-1, \
		EXTATTR_NAMESPACE_USER, name, NULL, 0));
	check_audit(fds, extregex, pipefd);
}

ATF_TC_CLEANUP(extattr_get_fd_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(extattr_get_link_success);
ATF_TC_HEAD(extattr_get_link_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"extattr_get_link(2) call");
}

ATF_TC_BODY(extattr_get_link_success, tc)
{
	const char *buff = "ezio";
	/* Symbolic link needs to exist to call extattr_get_link(2) */
	ATF_REQUIRE_EQ(0, symlink("symlink", path));
	ATF_REQUIRE_EQ(sizeof(buff), extattr_set_link(path, \
		EXTATTR_NAMESPACE_USER, name, buff, sizeof(buff)));

	/* Prepare the regex to be checked in the audit record */
	snprintf(extregex, 80, "extattr_get_link.*%s.*%s.*return,success,%lu", \
		path, name, sizeof(buff));

	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(sizeof(buff), extattr_get_link(path, \
		EXTATTR_NAMESPACE_USER, name, NULL, 0));
	check_audit(fds, extregex, pipefd);
}

ATF_TC_CLEANUP(extattr_get_link_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(extattr_get_link_failure);
ATF_TC_HEAD(extattr_get_link_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"extattr_get_link(2) call");
}

ATF_TC_BODY(extattr_get_link_failure, tc)
{
	/* Prepare the regex to be checked in the audit record */
	snprintf(extregex, 80, "extattr_get_link.*%s.*%s.*failure", path, name);
	FILE *pipefd = setup(fds, "fa");
	/* Failure reason: symbolic link does not exist */
	ATF_REQUIRE_EQ(-1, extattr_get_link(path, \
		EXTATTR_NAMESPACE_USER, name, NULL, 0));
	check_audit(fds, extregex, pipefd);
}

ATF_TC_CLEANUP(extattr_get_link_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(extattr_list_file_success);
ATF_TC_HEAD(extattr_list_file_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"extattr_list_file(2) call");
}

ATF_TC_BODY(extattr_list_file_success, tc)
{
	int readbuff;
	const char *buff = "ezio";
	/* File needs to exist to call extattr_list_file(2) */
	ATF_REQUIRE(open(path, O_CREAT, mode) != -1);
	ATF_REQUIRE_EQ(sizeof(buff), extattr_set_file(path, \
		EXTATTR_NAMESPACE_USER, name, buff, sizeof(buff)));

	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE((readbuff = extattr_list_file(path, \
		EXTATTR_NAMESPACE_USER, NULL, 0)) != -1);
	/* Prepare the regex to be checked in the audit record */
	snprintf(extregex, 80, "extattr_list_file.*%s.*return,success,%d", \
		path, readbuff);
	check_audit(fds, extregex, pipefd);
}

ATF_TC_CLEANUP(extattr_list_file_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(extattr_list_file_failure);
ATF_TC_HEAD(extattr_list_file_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"extattr_list_file(2) call");
}

ATF_TC_BODY(extattr_list_file_failure, tc)
{
	/* Prepare the regex to be checked in the audit record */
	snprintf(extregex, 80, "extattr_list_file.*%s.*return,failure", path);

	FILE *pipefd = setup(fds, "fa");
	/* Failure reason: file does not exist */
	ATF_REQUIRE_EQ(-1, extattr_list_file(path, \
		EXTATTR_NAMESPACE_USER, NULL, 0));
	check_audit(fds, extregex, pipefd);
}

ATF_TC_CLEANUP(extattr_list_file_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(extattr_list_fd_success);
ATF_TC_HEAD(extattr_list_fd_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"extattr_list_fd(2) call");
}

ATF_TC_BODY(extattr_list_fd_success, tc)
{
	int filedesc, readbuff;
	const char *buff = "ezio";

	/* File needs to exist to call extattr_list_fd(2) */
	ATF_REQUIRE((filedesc = open(path, O_CREAT, mode)) != -1);
	ATF_REQUIRE_EQ(sizeof(buff), extattr_set_file(path, \
		EXTATTR_NAMESPACE_USER, name, buff, sizeof(buff)));

	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE((readbuff = extattr_list_fd(filedesc, \
		EXTATTR_NAMESPACE_USER, NULL, 0)) != -1);
	/* Prepare the regex to be checked in the audit record */
	snprintf(extregex, 80, "extattr_list_fd.*return,success,%d", readbuff);
	check_audit(fds, extregex, pipefd);
}

ATF_TC_CLEANUP(extattr_list_fd_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(extattr_list_fd_failure);
ATF_TC_HEAD(extattr_list_fd_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"extattr_list_fd(2) call");
}

ATF_TC_BODY(extattr_list_fd_failure, tc)
{
	/* Prepare the regex to be checked in the audit record */
	snprintf(extregex, 80, "extattr_list_fd.*return,failure : "
				"Bad file descriptor");

	FILE *pipefd = setup(fds, "fa");
	/* Failure reason: Invalid file descriptor */
	ATF_REQUIRE_EQ(-1, extattr_list_fd(-1, EXTATTR_NAMESPACE_USER, NULL, 0));
	check_audit(fds, extregex, pipefd);
}

ATF_TC_CLEANUP(extattr_list_fd_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(extattr_list_link_success);
ATF_TC_HEAD(extattr_list_link_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"extattr_list_link(2) call");
}

ATF_TC_BODY(extattr_list_link_success, tc)
{
	int readbuff;
	const char *buff = "ezio";

	/* Symbolic link needs to exist to call extattr_list_link(2) */
	ATF_REQUIRE_EQ(0, symlink("symlink", path));
	ATF_REQUIRE_EQ(sizeof(buff), extattr_set_link(path, \
		EXTATTR_NAMESPACE_USER, name, buff, sizeof(buff)));
	FILE *pipefd = setup(fds, "fa");

	ATF_REQUIRE((readbuff = extattr_list_link(path, \
		EXTATTR_NAMESPACE_USER, NULL, 0)) != -1);
	/* Prepare the regex to be checked in the audit record */
	snprintf(extregex, 80, "extattr_list_link.*%s.*return,success,%d", \
		path, readbuff);
	check_audit(fds, extregex, pipefd);
}

ATF_TC_CLEANUP(extattr_list_link_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(extattr_list_link_failure);
ATF_TC_HEAD(extattr_list_link_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"extattr_list_link(2) call");
}

ATF_TC_BODY(extattr_list_link_failure, tc)
{
	/* Prepare the regex to be checked in the audit record */
	snprintf(extregex, 80, "extattr_list_link.*%s.*failure", path);
	FILE *pipefd = setup(fds, "fa");
	/* Failure reason: symbolic link does not exist */
	ATF_REQUIRE_EQ(-1, extattr_list_link(path, \
		EXTATTR_NAMESPACE_USER, NULL, 0));
	check_audit(fds, extregex, pipefd);
}

ATF_TC_CLEANUP(extattr_list_link_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(open_read_creat_success);
ATF_TC_HEAD(open_read_creat_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful open(2)"
					" call for O_RDONLY, O_CREAT flags");
}

ATF_TC_BODY(open_read_creat_success, tc)
{
	const char *regex = "read,creat.*fileforaudit.*return,success";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(syscall(SYS_open, path, O_RDONLY | O_CREAT) != -1);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(open_read_creat_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(open_read_creat_failure);
ATF_TC_HEAD(open_read_creat_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of unsuccessful open(2)"
					" call for O_RDONLY, O_CREAT flags");
}

ATF_TC_BODY(open_read_creat_failure, tc)
{
	const char *regex = "read,creat.*fileforaudit.*return,failure";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(-1, syscall(SYS_open, errpath, O_RDONLY | O_CREAT));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(open_read_creat_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(openat_read_creat_success);
ATF_TC_HEAD(openat_read_creat_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of successful openat(2)"
					" call for O_RDONLY, O_CREAT flags");
}

ATF_TC_BODY(openat_read_creat_success, tc)
{
	const char *regex = "read,creat.*fileforaudit.*return,success";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(openat(AT_FDCWD, path, O_RDONLY | O_CREAT) != -1);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(openat_read_creat_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(openat_read_creat_failure);
ATF_TC_HEAD(openat_read_creat_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of unsuccessful openat(2)"
					" call for O_RDONLY, O_CREAT flags");
}

ATF_TC_BODY(openat_read_creat_failure, tc)
{
	const char *regex = "read,creat.*fileforaudit.*return,failure";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(-1, openat(AT_FDCWD, errpath, O_RDONLY | O_CREAT));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(openat_read_creat_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(open_read_trunc_success);
ATF_TC_HEAD(open_read_trunc_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful open(2)"
					" call for O_RDONLY, O_TRUNC flags");
}

ATF_TC_BODY(open_read_trunc_success, tc)
{
	const char *regex = "read,trunc.*fileforaudit.*return,success";
	/* File needs to exist to open(2) as O_RDONLY | O_TRUNC */
	ATF_REQUIRE(open(path, O_CREAT, mode) != -1);
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(syscall(SYS_open, path, O_RDONLY | O_TRUNC) != -1);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(open_read_trunc_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(open_read_trunc_failure);
ATF_TC_HEAD(open_read_trunc_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of unsuccessful open(2)"
					" call for O_RDONLY, O_TRUNC flags");
}

ATF_TC_BODY(open_read_trunc_failure, tc)
{
	const char *regex = "read,trunc.*fileforaudit.*return,failure";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(-1, syscall(SYS_open, errpath, O_RDONLY | O_TRUNC));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(open_read_trunc_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(openat_read_trunc_success);
ATF_TC_HEAD(openat_read_trunc_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of successful openat(2)"
					" call for O_RDONLY, O_TRUNC flags");
}

ATF_TC_BODY(openat_read_trunc_success, tc)
{
	const char *regex = "read,trunc.*fileforaudit.*return,success";
	/* File needs to exist to openat(2) as O_RDONLY | O_TRUNC */
	ATF_REQUIRE(open(path, O_CREAT, mode) != -1);
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(openat(AT_FDCWD, path, O_RDONLY | O_TRUNC) != -1);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(openat_read_trunc_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(openat_read_trunc_failure);
ATF_TC_HEAD(openat_read_trunc_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of unsuccessful openat(2)"
					" call for O_RDONLY, O_TRUNC flags");
}

ATF_TC_BODY(openat_read_trunc_failure, tc)
{
	const char *regex = "read,trunc.*fileforaudit.*return,failure";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(-1, openat(AT_FDCWD, errpath, O_RDONLY | O_TRUNC));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(openat_read_trunc_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(open_read_creat_trunc_success);
ATF_TC_HEAD(open_read_creat_trunc_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful open(2)"
				" call for O_RDONLY, O_CREAT, O_TRUNC flags");
}

ATF_TC_BODY(open_read_creat_trunc_success, tc)
{
	const char *regex = "read,creat,trunc.*fileforaudit.*return,success";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(syscall(SYS_open, path, O_RDONLY | O_CREAT | O_TRUNC) != -1);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(open_read_creat_trunc_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(open_read_creat_trunc_failure);
ATF_TC_HEAD(open_read_creat_trunc_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of unsuccessful open(2)"
				" call for O_RDONLY, O_CREAT, O_TRUNC flags");
}

ATF_TC_BODY(open_read_creat_trunc_failure, tc)
{
	const char *regex = "read,creat,trunc.*fileforaudit.*return,failure";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(-1, syscall(SYS_open, errpath, O_RDONLY | O_CREAT | O_TRUNC));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(open_read_creat_trunc_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(openat_read_creat_trunc_success);
ATF_TC_HEAD(openat_read_creat_trunc_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of successful openat(2)"
				" call for O_RDONLY, O_CREAT, O_TRUNC flags");
}

ATF_TC_BODY(openat_read_creat_trunc_success, tc)
{
	const char *regex = "read,creat,trunc.*fileforaudit.*return,success";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(openat(AT_FDCWD, path, O_RDONLY | O_CREAT | O_TRUNC) != -1);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(openat_read_creat_trunc_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(openat_read_creat_trunc_failure);
ATF_TC_HEAD(openat_read_creat_trunc_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of unsuccessful openat(2)"
				" call for O_RDONLY, O_CREAT, O_TRUNC flags");
}

ATF_TC_BODY(openat_read_creat_trunc_failure, tc)
{
	const char *regex = "read,creat,trunc.*fileforaudit.*return,failure";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(-1, openat(AT_FDCWD, errpath, O_RDONLY | O_CREAT | O_TRUNC));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(openat_read_creat_trunc_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(open_write_creat_success);
ATF_TC_HEAD(open_write_creat_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful open(2)"
					" call for O_WRONLY, O_CREAT flags");
}

ATF_TC_BODY(open_write_creat_success, tc)
{
	const char *regex = "write,creat.*fileforaudit.*return,success";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(syscall(SYS_open, path, O_WRONLY | O_CREAT) != -1);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(open_write_creat_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(open_write_creat_failure);
ATF_TC_HEAD(open_write_creat_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of unsuccessful open(2)"
					" call for O_WRONLY, O_CREAT flags");
}

ATF_TC_BODY(open_write_creat_failure, tc)
{
	const char *regex = "write,creat.*fileforaudit.*return,failure";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(-1, syscall(SYS_open, errpath, O_WRONLY | O_CREAT));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(open_write_creat_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(openat_write_creat_success);
ATF_TC_HEAD(openat_write_creat_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of successful openat(2)"
					" call for O_WRONLY, O_CREAT flags");
}

ATF_TC_BODY(openat_write_creat_success, tc)
{
	const char *regex = "write,creat.*fileforaudit.*return,success";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(openat(AT_FDCWD, path, O_WRONLY | O_CREAT) != -1);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(openat_write_creat_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(openat_write_creat_failure);
ATF_TC_HEAD(openat_write_creat_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of unsuccessful openat(2)"
					" call for O_WRONLY, O_CREAT flags");
}

ATF_TC_BODY(openat_write_creat_failure, tc)
{
	const char *regex = "write,creat.*fileforaudit.*return,failure";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(-1, openat(AT_FDCWD, errpath, O_WRONLY | O_CREAT));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(openat_write_creat_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(open_write_trunc_success);
ATF_TC_HEAD(open_write_trunc_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful open(2)"
					" call for O_WRONLY, O_TRUNC flags");
}

ATF_TC_BODY(open_write_trunc_success, tc)
{
	const char *regex = "write,trunc.*fileforaudit.*return,success";
	/* File needs to exist to open(2) as O_WRONLY | O_TRUNC */
	ATF_REQUIRE(open(path, O_CREAT, mode) != -1);
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(syscall(SYS_open, path, O_WRONLY | O_TRUNC) != -1);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(open_write_trunc_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(open_write_trunc_failure);
ATF_TC_HEAD(open_write_trunc_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of unsuccessful open(2)"
					" call for O_WRONLY, O_TRUNC flags");
}

ATF_TC_BODY(open_write_trunc_failure, tc)
{
	const char *regex = "write,trunc.*fileforaudit.*return,failure";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(-1, syscall(SYS_open, errpath, O_WRONLY | O_TRUNC));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(open_write_trunc_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(openat_write_trunc_success);
ATF_TC_HEAD(openat_write_trunc_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of successful openat(2)"
					" call for O_WRONLY, O_TRUNC flags");
}

ATF_TC_BODY(openat_write_trunc_success, tc)
{
	const char *regex = "write,trunc.*fileforaudit.*return,success";
	/* File needs to exist to openat(2) as O_WRONLY | O_TRUNC */
	ATF_REQUIRE(open(path, O_CREAT, mode) != -1);
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(openat(AT_FDCWD, path, O_WRONLY | O_TRUNC) != -1);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(openat_write_trunc_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(openat_write_trunc_failure);
ATF_TC_HEAD(openat_write_trunc_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of unsuccessful openat(2)"
					" call for O_WRONLY, O_TRUNC flags");
}

ATF_TC_BODY(openat_write_trunc_failure, tc)
{
	const char *regex = "write,trunc.*fileforaudit.*return,failure";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(-1, openat(AT_FDCWD, errpath, O_WRONLY | O_TRUNC));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(openat_write_trunc_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(open_write_creat_trunc_success);
ATF_TC_HEAD(open_write_creat_trunc_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful open(2)"
				" call for O_WRONLY, O_CREAT, O_TRUNC flags");
}

ATF_TC_BODY(open_write_creat_trunc_success, tc)
{
	const char *regex = "write,creat,trunc.*fileforaudit.*return,success";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(syscall(SYS_open, path, O_WRONLY | O_CREAT | O_TRUNC) != -1);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(open_write_creat_trunc_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(open_write_creat_trunc_failure);
ATF_TC_HEAD(open_write_creat_trunc_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of unsuccessful open(2)"
				" call for O_WRONLY, O_CREAT, O_TRUNC flags");
}

ATF_TC_BODY(open_write_creat_trunc_failure, tc)
{
	const char *regex = "write,creat,trunc.*fileforaudit.*return,failure";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(-1, syscall(SYS_open, errpath, O_WRONLY | O_CREAT | O_TRUNC));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(open_write_creat_trunc_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(openat_write_creat_trunc_success);
ATF_TC_HEAD(openat_write_creat_trunc_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of successful openat(2)"
				" call for O_WRONLY, O_CREAT, O_TRUNC flags");
}

ATF_TC_BODY(openat_write_creat_trunc_success, tc)
{
	const char *regex = "write,creat,trunc.*fileforaudit.*return,success";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(openat(AT_FDCWD, path, O_WRONLY | O_CREAT | O_TRUNC) != -1);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(openat_write_creat_trunc_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(openat_write_creat_trunc_failure);
ATF_TC_HEAD(openat_write_creat_trunc_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of unsuccessful openat(2)"
				" call for O_WRONLY, O_CREAT, O_TRUNC flags");
}

ATF_TC_BODY(openat_write_creat_trunc_failure, tc)
{
	const char *regex = "write,creat,trunc.*fileforaudit.*return,failure";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(-1, openat(AT_FDCWD, errpath, O_WRONLY | O_CREAT | O_TRUNC));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(openat_write_creat_trunc_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(open_read_write_creat_success);
ATF_TC_HEAD(open_read_write_creat_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful open(2)"
					" call for O_RDWR, O_CREAT flags");
}

ATF_TC_BODY(open_read_write_creat_success, tc)
{
	const char *regex = "read,write,creat.*fileforaudit.*return,success";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(syscall(SYS_open, path, O_RDWR | O_CREAT) != -1);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(open_read_write_creat_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(open_read_write_creat_failure);
ATF_TC_HEAD(open_read_write_creat_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of unsuccessful open(2)"
					" call for O_RDWR, O_CREAT flags");
}

ATF_TC_BODY(open_read_write_creat_failure, tc)
{
	const char *regex = "read,write,creat.*fileforaudit.*return,failure";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(-1, syscall(SYS_open, errpath, O_RDWR | O_CREAT));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(open_read_write_creat_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(openat_read_write_creat_success);
ATF_TC_HEAD(openat_read_write_creat_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of successful openat(2)"
					" call for O_RDWR, O_CREAT flags");
}

ATF_TC_BODY(openat_read_write_creat_success, tc)
{
	const char *regex = "read,write,creat.*fileforaudit.*return,success";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(openat(AT_FDCWD, path, O_RDWR | O_CREAT) != -1);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(openat_read_write_creat_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(openat_read_write_creat_failure);
ATF_TC_HEAD(openat_read_write_creat_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of unsuccessful openat(2)"
					" call for O_RDWR, O_CREAT flags");
}

ATF_TC_BODY(openat_read_write_creat_failure, tc)
{
	const char *regex = "read,write,creat.*fileforaudit.*return,failure";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(-1, openat(AT_FDCWD, errpath, O_RDWR | O_CREAT));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(openat_read_write_creat_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(open_read_write_trunc_success);
ATF_TC_HEAD(open_read_write_trunc_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful open(2)"
					" call for O_RDWR, O_TRUNC flags");
}

ATF_TC_BODY(open_read_write_trunc_success, tc)
{
	const char *regex = "read,write,trunc.*fileforaudit.*return,success";
	/* File needs to exist to open(2) as O_RDWR | O_TRUNC */
	ATF_REQUIRE(open(path, O_CREAT, mode) != -1);
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(syscall(SYS_open, path, O_RDWR | O_TRUNC) != -1);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(open_read_write_trunc_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(open_read_write_trunc_failure);
ATF_TC_HEAD(open_read_write_trunc_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of unsuccessful open(2)"
					" call for O_RDWR, O_TRUNC flags");
}

ATF_TC_BODY(open_read_write_trunc_failure, tc)
{
	const char *regex = "read,write,trunc.*fileforaudit.*return,failure";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(-1, syscall(SYS_open, errpath, O_RDWR | O_TRUNC));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(open_read_write_trunc_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(openat_read_write_trunc_success);
ATF_TC_HEAD(openat_read_write_trunc_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of successful openat(2)"
					" call for O_RDWR, O_TRUNC flags");
}

ATF_TC_BODY(openat_read_write_trunc_success, tc)
{
	const char *regex = "read,write,trunc.*fileforaudit.*return,success";
	/* File needs to exist to openat(2) as O_RDWR | O_TRUNC */
	ATF_REQUIRE(open(path, O_CREAT, mode) != -1);
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(openat(AT_FDCWD, path, O_RDWR | O_TRUNC) != -1);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(openat_read_write_trunc_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(openat_read_write_trunc_failure);
ATF_TC_HEAD(openat_read_write_trunc_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of unsuccessful openat(2)"
					" call for O_RDWR, O_TRUNC flags");
}

ATF_TC_BODY(openat_read_write_trunc_failure, tc)
{
	const char *regex = "read,write,trunc.*fileforaudit.*return,failure";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(-1, openat(AT_FDCWD, errpath, O_RDWR | O_TRUNC));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(openat_read_write_trunc_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(open_read_write_creat_trunc_success);
ATF_TC_HEAD(open_read_write_creat_trunc_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful open(2)"
				" call for O_RDWR, O_CREAT, O_TRUNC flags");
}

ATF_TC_BODY(open_read_write_creat_trunc_success, tc)
{
	const char *regex = "read,write,creat,trunc.*fileforaudit.*return,success";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(syscall(SYS_open, path, O_RDWR | O_CREAT | O_TRUNC) != -1);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(open_read_write_creat_trunc_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(open_read_write_creat_trunc_failure);
ATF_TC_HEAD(open_read_write_creat_trunc_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of unsuccessful open(2)"
				" call for O_RDWR, O_CREAT, O_TRUNC flags");
}

ATF_TC_BODY(open_read_write_creat_trunc_failure, tc)
{
	const char *regex = "read,write,creat,trunc.*fileforaudit.*return,failure";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(-1, syscall(SYS_open, errpath, O_RDWR | O_CREAT | O_TRUNC));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(open_read_write_creat_trunc_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(openat_read_write_creat_trunc_success);
ATF_TC_HEAD(openat_read_write_creat_trunc_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of successful openat(2)"
				" call for O_RDWR, O_CREAT, O_TRUNC flags");
}

ATF_TC_BODY(openat_read_write_creat_trunc_success, tc)
{
	const char *regex = "read,write,creat,trunc.*fileforaudit.*return,success";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE(openat(AT_FDCWD, path, O_RDWR | O_CREAT | O_TRUNC) != -1);
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(openat_read_write_creat_trunc_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(openat_read_write_creat_trunc_failure);
ATF_TC_HEAD(openat_read_write_creat_trunc_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of unsuccessful openat(2)"
				" call for O_RDWR, O_CREAT, O_TRUNC flags");
}

ATF_TC_BODY(openat_read_write_creat_trunc_failure, tc)
{
	const char *regex = "read,write,creat,trunc.*fileforaudit.*return,failure";
	FILE *pipefd = setup(fds, "fa");
	ATF_REQUIRE_EQ(-1, openat(AT_FDCWD, errpath, O_RDWR | O_CREAT | O_TRUNC));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(openat_read_write_creat_trunc_failure, tc)
{
	cleanup();
}


ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, stat_success);
	ATF_TP_ADD_TC(tp, stat_failure);
	ATF_TP_ADD_TC(tp, lstat_success);
	ATF_TP_ADD_TC(tp, lstat_failure);
	ATF_TP_ADD_TC(tp, fstat_success);
	ATF_TP_ADD_TC(tp, fstat_failure);
	ATF_TP_ADD_TC(tp, fstatat_success);
	ATF_TP_ADD_TC(tp, fstatat_failure);

	ATF_TP_ADD_TC(tp, statfs_success);
	ATF_TP_ADD_TC(tp, statfs_failure);
	ATF_TP_ADD_TC(tp, fstatfs_success);
	ATF_TP_ADD_TC(tp, fstatfs_failure);

	ATF_TP_ADD_TC(tp, getfsstat_success);
	ATF_TP_ADD_TC(tp, getfsstat_failure);

	ATF_TP_ADD_TC(tp, fhopen_success);
	ATF_TP_ADD_TC(tp, fhopen_failure);
	ATF_TP_ADD_TC(tp, fhstat_success);
	ATF_TP_ADD_TC(tp, fhstat_failure);
	ATF_TP_ADD_TC(tp, fhstatfs_success);
	ATF_TP_ADD_TC(tp, fhstatfs_failure);

	ATF_TP_ADD_TC(tp, access_success);
	ATF_TP_ADD_TC(tp, access_failure);
	ATF_TP_ADD_TC(tp, eaccess_success);
	ATF_TP_ADD_TC(tp, eaccess_failure);
	ATF_TP_ADD_TC(tp, faccessat_success);
	ATF_TP_ADD_TC(tp, faccessat_failure);

	ATF_TP_ADD_TC(tp, pathconf_success);
	ATF_TP_ADD_TC(tp, pathconf_failure);
	ATF_TP_ADD_TC(tp, lpathconf_success);
	ATF_TP_ADD_TC(tp, lpathconf_failure);
	ATF_TP_ADD_TC(tp, fpathconf_success);
	ATF_TP_ADD_TC(tp, fpathconf_failure);

	ATF_TP_ADD_TC(tp, extattr_get_file_success);
	ATF_TP_ADD_TC(tp, extattr_get_file_failure);
	ATF_TP_ADD_TC(tp, extattr_get_fd_success);
	ATF_TP_ADD_TC(tp, extattr_get_fd_failure);
	ATF_TP_ADD_TC(tp, extattr_get_link_success);
	ATF_TP_ADD_TC(tp, extattr_get_link_failure);

	ATF_TP_ADD_TC(tp, extattr_list_file_success);
	ATF_TP_ADD_TC(tp, extattr_list_file_failure);
	ATF_TP_ADD_TC(tp, extattr_list_fd_success);
	ATF_TP_ADD_TC(tp, extattr_list_fd_failure);
	ATF_TP_ADD_TC(tp, extattr_list_link_success);
	ATF_TP_ADD_TC(tp, extattr_list_link_failure);

	ATF_TP_ADD_TC(tp, open_read_creat_success);
	ATF_TP_ADD_TC(tp, open_read_creat_failure);
	ATF_TP_ADD_TC(tp, openat_read_creat_success);
	ATF_TP_ADD_TC(tp, openat_read_creat_failure);

	ATF_TP_ADD_TC(tp, open_read_trunc_success);
	ATF_TP_ADD_TC(tp, open_read_trunc_failure);
	ATF_TP_ADD_TC(tp, openat_read_trunc_success);
	ATF_TP_ADD_TC(tp, openat_read_trunc_failure);

	ATF_TP_ADD_TC(tp, open_read_creat_trunc_success);
	ATF_TP_ADD_TC(tp, open_read_creat_trunc_failure);
	ATF_TP_ADD_TC(tp, openat_read_creat_trunc_success);
	ATF_TP_ADD_TC(tp, openat_read_creat_trunc_failure);

	ATF_TP_ADD_TC(tp, open_write_creat_success);
	ATF_TP_ADD_TC(tp, open_write_creat_failure);
	ATF_TP_ADD_TC(tp, openat_write_creat_success);
	ATF_TP_ADD_TC(tp, openat_write_creat_failure);

	ATF_TP_ADD_TC(tp, open_write_trunc_success);
	ATF_TP_ADD_TC(tp, open_write_trunc_failure);
	ATF_TP_ADD_TC(tp, openat_write_trunc_success);
	ATF_TP_ADD_TC(tp, openat_write_trunc_failure);

	ATF_TP_ADD_TC(tp, open_write_creat_trunc_success);
	ATF_TP_ADD_TC(tp, open_write_creat_trunc_failure);
	ATF_TP_ADD_TC(tp, openat_write_creat_trunc_success);
	ATF_TP_ADD_TC(tp, openat_write_creat_trunc_failure);

	ATF_TP_ADD_TC(tp, open_read_write_creat_success);
	ATF_TP_ADD_TC(tp, open_read_write_creat_failure);
	ATF_TP_ADD_TC(tp, openat_read_write_creat_success);
	ATF_TP_ADD_TC(tp, openat_read_write_creat_failure);

	ATF_TP_ADD_TC(tp, open_read_write_trunc_success);
	ATF_TP_ADD_TC(tp, open_read_write_trunc_failure);
	ATF_TP_ADD_TC(tp, openat_read_write_trunc_success);
	ATF_TP_ADD_TC(tp, openat_read_write_trunc_failure);

	ATF_TP_ADD_TC(tp, open_read_write_creat_trunc_success);
	ATF_TP_ADD_TC(tp, open_read_write_creat_trunc_failure);
	ATF_TP_ADD_TC(tp, openat_read_write_creat_trunc_success);
	ATF_TP_ADD_TC(tp, openat_read_write_creat_trunc_failure);

	return (atf_no_error());
}
