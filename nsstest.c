/* 
   Unix SMB/CIFS implementation.

   nss tester

   Copyright (C) Andrew Tridgell 2001-2004
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <nss.h>
#include <dlfcn.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <sys/types.h>

typedef enum nss_status NSS_STATUS;

static const char *so_path = "/lib/libnss_winbind.so";
static char *nss_name;
static int nss_errno;
static NSS_STATUS last_error;
static int total_errors;

static void *find_fn(const char *name)
{
	char s[1024];
	static void *h;
	void *res;

	snprintf(s,sizeof(s), "_nss_%s_%s", nss_name, name);

	if (!h) {
		h = dlopen(so_path, RTLD_LAZY);
	}
	if (!h) {
		printf("Can't open shared library %s : %s\n", so_path, dlerror());
		exit(1);
	}
	res = dlsym(h, s);
	if (!res) {
		printf("Can't find function %s : %s\n", s, dlerror());
		return NULL;
	}
	return res;
}

static void report_nss_error(const char *who, NSS_STATUS status)
{
	last_error = status;
	total_errors++;
	printf("ERROR %s: NSS_STATUS=%d  %d (nss_errno=%d)\n", 
	       who, status, NSS_STATUS_SUCCESS, nss_errno);
}

static struct passwd *nss_getpwent(void)
{
	NSS_STATUS (*_nss_getpwent_r)(struct passwd *, char *, 
				      size_t , int *) = find_fn("getpwent_r");
	static struct passwd pwd;
	static char buf[1000];
	NSS_STATUS status;

	status = _nss_getpwent_r(&pwd, buf, sizeof(buf), &nss_errno);
	if (status == NSS_STATUS_NOTFOUND) {
		return NULL;
	}
	if (status != NSS_STATUS_SUCCESS) {
		report_nss_error("getpwent", status);
		return NULL;
	}
	return &pwd;
}

static struct passwd *nss_getpwnam(const char *name)
{
	NSS_STATUS (*_nss_getpwnam_r)(const char *, struct passwd *, char *, 
				      size_t , int *) = find_fn("getpwnam_r");
	static struct passwd pwd;
	static char buf[1000];
	NSS_STATUS status;
	
	status = _nss_getpwnam_r(name, &pwd, buf, sizeof(buf), &nss_errno);
	if (status == NSS_STATUS_NOTFOUND) {
		return NULL;
	}
	if (status != NSS_STATUS_SUCCESS) {
		report_nss_error("getpwnam", status);
		return NULL;
	}
	return &pwd;
}

static struct passwd *nss_getpwuid(uid_t uid)
{
	NSS_STATUS (*_nss_getpwuid_r)(uid_t , struct passwd *, char *, 
				      size_t , int *) = find_fn("getpwuid_r");
	static struct passwd pwd;
	static char buf[1000];
	NSS_STATUS status;
	
	status = _nss_getpwuid_r(uid, &pwd, buf, sizeof(buf), &nss_errno);
	if (status == NSS_STATUS_NOTFOUND) {
		return NULL;
	}
	if (status != NSS_STATUS_SUCCESS) {
		report_nss_error("getpwuid", status);
		return NULL;
	}
	return &pwd;
}

static void nss_setpwent(void)
{
	NSS_STATUS (*_nss_setpwent)(void) = find_fn("setpwent");
	NSS_STATUS status;
	status = _nss_setpwent();
	if (status != NSS_STATUS_SUCCESS) {
		report_nss_error("setpwent", status);
	}
}

static void nss_endpwent(void)
{
	NSS_STATUS (*_nss_endpwent)(void) = find_fn("endpwent");
	NSS_STATUS status;
	status = _nss_endpwent();
	if (status != NSS_STATUS_SUCCESS) {
		report_nss_error("endpwent", status);
	}
}


static struct group *nss_getgrent(void)
{
	NSS_STATUS (*_nss_getgrent_r)(struct group *, char *, 
				      size_t , int *) = find_fn("getgrent_r");
	static struct group grp;
	static char *buf;
	static int buflen = 1024;
	NSS_STATUS status;

	if (!buf) buf = malloc(buflen);

again:	
	status = _nss_getgrent_r(&grp, buf, buflen, &nss_errno);
	if (status == NSS_STATUS_TRYAGAIN) {
		buflen *= 2;
		buf = realloc(buf, buflen);
		goto again;
	}
	if (status == NSS_STATUS_NOTFOUND) {
		return NULL;
	}
	if (status != NSS_STATUS_SUCCESS) {
		report_nss_error("getgrent", status);
		return NULL;
	}
	return &grp;
}

static struct group *nss_getgrnam(const char *name)
{
	NSS_STATUS (*_nss_getgrnam_r)(const char *, struct group *, char *, 
				      size_t , int *) = find_fn("getgrnam_r");
	static struct group grp;
	static char *buf;
	static int buflen = 1000;
	NSS_STATUS status;

	if (!buf) buf = malloc(buflen);
again:	
	status = _nss_getgrnam_r(name, &grp, buf, buflen, &nss_errno);
	if (status == NSS_STATUS_TRYAGAIN) {
		buflen *= 2;
		buf = realloc(buf, buflen);
		goto again;
	}
	if (status == NSS_STATUS_NOTFOUND) {
		return NULL;
	}
	if (status != NSS_STATUS_SUCCESS) {
		report_nss_error("getgrnam", status);
		return NULL;
	}
	return &grp;
}

static struct group *nss_getgrgid(gid_t gid)
{
	NSS_STATUS (*_nss_getgrgid_r)(gid_t , struct group *, char *, 
				      size_t , int *) = find_fn("getgrgid_r");
	static struct group grp;
	static char *buf;
	static int buflen = 1000;
	NSS_STATUS status;
	
	if (!buf) buf = malloc(buflen);
again:	
	status = _nss_getgrgid_r(gid, &grp, buf, buflen, &nss_errno);
	if (status == NSS_STATUS_TRYAGAIN) {
		buflen *= 2;
		buf = realloc(buf, buflen);
		goto again;
	}
	if (status == NSS_STATUS_NOTFOUND) {
		return NULL;
	}
	if (status != NSS_STATUS_SUCCESS) {
		report_nss_error("getgrgid", status);
		return NULL;
	}
	return &grp;
}

static void nss_setgrent(void)
{
	NSS_STATUS (*_nss_setgrent)(void) = find_fn("setgrent");
	NSS_STATUS status;
	status = _nss_setgrent();
	if (status != NSS_STATUS_SUCCESS) {
		report_nss_error("setgrent", status);
	}
}

static void nss_endgrent(void)
{
	NSS_STATUS (*_nss_endgrent)(void) = find_fn("endgrent");
	NSS_STATUS status;
	status = _nss_endgrent();
	if (status != NSS_STATUS_SUCCESS) {
		report_nss_error("endgrent", status);
	}
}

static int nss_initgroups(char *user, gid_t group, gid_t **groups, long int *start, long int *size)
{
	NSS_STATUS (*_nss_initgroups)(char *, gid_t , long int *,
				      long int *, gid_t **, long int , int *) = 
		find_fn("initgroups_dyn");
	NSS_STATUS status;

	if (!_nss_initgroups) return NSS_STATUS_UNAVAIL;

	status = _nss_initgroups(user, group, start, size, groups, 0, &nss_errno);
	if (status != NSS_STATUS_SUCCESS) {
		report_nss_error("initgroups", status);
	}
	return status;
}

static void print_passwd(struct passwd *pwd)
{
	printf("%s:%s:%d:%d:%s:%s:%s\n", 
	       pwd->pw_name,
	       pwd->pw_passwd,
	       pwd->pw_uid,
	       pwd->pw_gid,
	       pwd->pw_gecos,
	       pwd->pw_dir,
	       pwd->pw_shell);
}

static void print_group(struct group *grp)
{
	int i;
	printf("%s:%s:%d: ", 
	       grp->gr_name,
	       grp->gr_passwd,
	       grp->gr_gid);
	
	if (!grp->gr_mem[0]) {
		printf("\n");
		return;
	}
	
	for (i=0; grp->gr_mem[i+1]; i++) {
		printf("%s, ", grp->gr_mem[i]);
	}
	printf("%s\n", grp->gr_mem[i]);
}

static void nss_test_initgroups(char *name, gid_t gid)
{
	long int size = 16;
	long int start = 1;
	gid_t *groups = NULL;
	int i;
	NSS_STATUS status;

	groups = (gid_t *)malloc(size * sizeof(gid_t));
	groups[0] = gid;

	status = nss_initgroups(name, gid, &groups, &start, &size);
	if (status == NSS_STATUS_UNAVAIL) {
		printf("No initgroups fn\n");
		return;
	}

	for (i=0; i<start-1; i++) {
		printf("%d, ", groups[i]);
	}
	printf("%d\n", groups[i]);
}


static void nss_test_users(void)
{
	struct passwd *pwd;

	nss_setpwent();
	/* loop over all users */
	while ((pwd = nss_getpwent())) {
		printf("Testing user %s\n", pwd->pw_name);
		printf("getpwent:   "); print_passwd(pwd);
		pwd = nss_getpwuid(pwd->pw_uid);
		if (!pwd) {
			total_errors++;
			printf("ERROR: can't getpwuid\n");
			continue;
		}
		printf("getpwuid:   "); print_passwd(pwd);
		pwd = nss_getpwnam(pwd->pw_name);
		if (!pwd) {
			total_errors++;
			printf("ERROR: can't getpwnam\n");
			continue;
		}
		printf("getpwnam:   "); print_passwd(pwd);
		printf("initgroups: "); nss_test_initgroups(pwd->pw_name, pwd->pw_gid);
		printf("\n");
	}
	nss_endpwent();
}

static void nss_test_groups(void)
{
	struct group *grp;

	nss_setgrent();
	/* loop over all groups */
	while ((grp = nss_getgrent())) {
		printf("Testing group %s\n", grp->gr_name);
		printf("getgrent: "); print_group(grp);
		grp = nss_getgrnam(grp->gr_name);
		if (!grp) {
			total_errors++;
			printf("ERROR: can't getgrnam\n");
			continue;
		}
		printf("getgrnam: "); print_group(grp);
		grp = nss_getgrgid(grp->gr_gid);
		if (!grp) {
			total_errors++;
			printf("ERROR: can't getgrgid\n");
			continue;
		}
		printf("getgrgid: "); print_group(grp);
		printf("\n");
	}
	nss_endgrent();
}

static void nss_test_errors(void)
{
	struct passwd *pwd;
	struct group *grp;

	pwd = getpwnam("nosuchname");
	if (pwd || last_error != NSS_STATUS_NOTFOUND) {
		total_errors++;
		printf("ERROR Non existant user gave error %d\n", last_error);
	}

	pwd = getpwuid(0xFFF0);
	if (pwd || last_error != NSS_STATUS_NOTFOUND) {
		total_errors++;
		printf("ERROR Non existant uid gave error %d\n", last_error);
	}

	grp = getgrnam("nosuchgroup");
	if (grp || last_error != NSS_STATUS_NOTFOUND) {
		total_errors++;
		printf("ERROR Non existant group gave error %d\n", last_error);
	}

	grp = getgrgid(0xFFF0);
	if (grp || last_error != NSS_STATUS_NOTFOUND) {
		total_errors++;
		printf("ERROR Non existant gid gave error %d\n", last_error);
	}
}

 int main(int argc, char *argv[])
{	
	char *p;

	if (argc > 1) so_path = argv[1];

	p = strrchr(so_path, '_');
	if (!p) {
		printf("Badly formed name for .so - must be libnss_FOO.so\n");
		exit(1);
	}
	nss_name = strdup(p+1);
	p = strchr(nss_name, '.');
	if (p) *p = 0;

	printf("so_path=%s nss_name=%s\n\n", so_path, nss_name);

	nss_test_users();
	nss_test_groups();
	nss_test_errors();

	printf("total_errors=%d\n", total_errors);

	return total_errors;
}
