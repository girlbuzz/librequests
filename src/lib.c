#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TODO(...) __builtin_unreachable()

#include "keyval.h"

/* AUTHORITY */

struct authority {
	char *userinfo;
	char *host;
	unsigned short port;
};

void cleanup_authority(struct authority *auth) {
	free(auth->userinfo);
	free(auth->host);
}

static int parse_authority(struct authority *auth, const char *authstr) {
	char *n;
	char *userinfo = NULL;
	char *host = NULL;
	unsigned short port = 0;

	if ((n = strchr(authstr, '@'))) {
		userinfo = strndup(authstr, n - authstr);
		authstr = n + 1;
	}

	if ((n = strchr(authstr, ':'))) {
		host = strndup(authstr, n - authstr);

		if (*++n == '\0') {
			free(userinfo);
			free(host);
			return 1;
		}

		port = atoi(n);
	} else {
		if (*authstr == '\0') {
			free(userinfo);
			return 1;
		}

		host = strdup(authstr);
	}

	auth->host = host;
	auth->userinfo = userinfo;
	auth->port = port;

	return 0;
}

static struct authority *parse_authority_alloc(const char *authstr) {
	struct authority *auth = malloc(sizeof(struct authority));

	if (parse_authority(auth, authstr)) {
		free(auth);
		return NULL;
	}

	return auth;
}

/* URL */

struct url {
	char *scheme;
	struct authority *authority;
	char *path;
	struct keyval *query;
	char *fragment;
};

void init_url(struct url *url) {
	url->scheme = NULL;
	url->authority = NULL;
	url->path = NULL;
	url->query = NULL;
	url->fragment = NULL;
}

void cleanup_url(struct url *url) {
	free(url->scheme);
	cleanup_authority(url->authority);
	free(url->authority);
	free(url->path);
	free(url->query);
	free(url->fragment);
}

int parse_url(struct url *url, const char *urlstr) {
	char *next;
	url->scheme = strndup(urlstr, (next = strchr(urlstr, ':')) - urlstr);

	if (next == NULL) {
		free(url->scheme);
		return 1;
	}

	if (!(*++next == '/'))
		return 1;

	if (*++next == '/') {
		next++;

		size_t authlen = strchr(next, '/') - next;

		char *auth = strndup(next, authlen);
		url->authority = parse_authority_alloc(auth);
		free(auth);

		next += authlen;
	} else {
		url->authority = NULL;
	}

	return 0;
}


#if TEST == 1
#include <assert.h>

int main() {
	struct url url;

	parse_url(&url, "http://admin@www.example.com:200/index.html?password=abc123;token=QWERTYUIOP#panel");

	assert(!strcmp(url.scheme, "http") && "scheme failed to parse");

	assert(url.authority && "no authority");
	assert(!strcmp(url.authority->host, "www.example.com") && "host failed to parse");
	assert(!strcmp(url.authority->userinfo, "april") && "userinfo failed to parse");
	assert((url.authority->port == 200) && "userinfo failed to parse");

	assert(!strcmp(kv_get_value(url.query, "password"), "abc123") && "query.password failed to parse");
	assert(!strcmp(kv_get_value(url.query, "token"), "QWERTYUIOP") && "query.token failed to parse");

	assert(!strcmp(url.fragment, "panel") && "fragment failed to parse");

	return 0;
}
#endif
