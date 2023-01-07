/*
	Set SO_PRIORITY right after each socket() call.
	Compile with: gcc -shared -ldl -fPIC so_priority.c -o so_priority.so
	Usage: SO_PRIORITY_DEBUG=1 SO_PRIORITY_VALUE=6 LD_PRELOAD=/path/to/so_priority.so program arg1 arg2

	(c) 2020-2023 - Xavier G.
	This program is free software. It comes without any warranty, to
	the extent permitted by applicable law. You can redistribute it
	and/or modify it under the terms of the Do What The Fuck You Want
	To Public License, Version 2, as published by Sam Hocevar. See
	http://www.wtfpl.net/ for more details.
*/

#define _GNU_SOURCE /* RTLD_NEXT */
#include <dlfcn.h> /* dlsym, dlerror */
#include <errno.h> /* errno */
#include <stdio.h> /* dprintf */
#include <stdlib.h> /* atoi, getenv */
#include <string.h> /* strerror */
#include <sys/types.h> /* setsockopt */
#include <sys/socket.h> /* setsockopt */

#ifndef SOPRIORITY_SO_NAME
#define SOPRIORITY_SO_NAME "so_priority.so"
#endif
#ifndef SOPRIORITY_VALUE_EV
#define SOPRIORITY_VALUE_EV "SO_PRIORITY_VALUE"
#endif
#ifndef SOPRIORITY_DEBUG_EV
#define SOPRIORITY_DEBUG_EV "SO_PRIORITY_DEBUG"
#endif

typedef int (*socket_function_type)(int, int, int);
 
/* Pointer to the actual socket symbol. */
socket_function_type so_priority_socket = NULL;

/* SO_PRIORITY value to set; can be configured using the SO_PRIORITY_VALUE environment variable. */
int so_priority_value = 0;
/* Debug level; can be configured using the SO_PRIORITY_DEBUG environment variable. */
int so_priority_debug = 0;
int so_priority_known = 0;

/* Ask the linker to provide the actual "socket" symbol: */
static int so_priority_get_socket() {
	char *error_string;
	so_priority_socket = (socket_function_type)dlsym(RTLD_NEXT, "socket");
	if (!so_priority_socket) {
		error_string = dlerror();
		if (error_string) {
			dprintf(2, "%s: unable to find the actual socket symbol: %s\n", SOPRIORITY_SO_NAME, error_string);
		}
		else {
			dprintf(2, "%s: unable to find the actual socket symbol\n", SOPRIORITY_SO_NAME);
		}
		return 0;
	}
	return 1;
}

/* Read the priority value from the environment. */
static void so_priority_set_options() {
	char *str;
	str = getenv(SOPRIORITY_VALUE_EV);
	if (str) so_priority_value = atoi(str);
	str = getenv(SOPRIORITY_DEBUG_EV);
	if (str) so_priority_debug = atoi(str);
	so_priority_known = 1;
}

/* Wrapper around the actual socket() function that sets SO_PRIORITY by calling setsockopt(). */
int socket(int domain, int type, int protocol) {
	int ret, setsockopt_errno, socket_errno, sockfd;

	if (!so_priority_socket && !so_priority_get_socket()) {
		errno = ENOENT;
		return -1;
	}
	/* Actually call socket(): */
	sockfd = so_priority_socket(domain, type, protocol);
	/* Do not call setsockopt() if socket() failed: */
	if (sockfd < 0) return sockfd;

	/* Time for the actual work: */
	socket_errno = errno;

	if (!so_priority_known) so_priority_set_options();
	ret = setsockopt(sockfd, SOL_SOCKET, SO_PRIORITY, &so_priority_value, sizeof(so_priority_value));
	/* Ignore setsockopt() errors, except for debugging purposes. */
	if (so_priority_debug) {
		setsockopt_errno = errno;
		dprintf(2, "%s: setsockopt(%d, SOL_SOCKET, SO_PRIORITY, %d, %zu) returned %d",
		        SOPRIORITY_SO_NAME, sockfd, so_priority_value, sizeof(so_priority_value), ret);
		if (ret < 0) dprintf(2, "; errno is %d: %s", setsockopt_errno, strerror(setsockopt_errno));
		dprintf(2, "\n");
	}
	
	errno = socket_errno;
	return sockfd;
}
