#pragma testing

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <keyval.h>
#include <uri.h>

/* AUTHORITY */

void init_authority(struct authority *auth) {
	auth->userinfo = NULL;
	auth->host = NULL;
	auth->port = 0;
}

void cleanup_authority(struct authority *auth) {
	if (!auth)
		return;

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

/* QUERY */

static int parse_query(struct keyval *pairs, const char *query) {
	return 0;
}

/* URL */

void init_uri(struct uri *uri) {
	uri->scheme = NULL;
	uri->authority = NULL;
	uri->path = NULL;
	uri->query = NULL;
	uri->fragment = NULL;
}

void cleanup_uri(struct uri *uri) {
	free(uri->scheme);
	cleanup_authority(uri->authority);
	free(uri->authority);
	free(uri->path);
	keyvals_free(uri->query);
	free(uri->fragment);
}

void free_uri(struct uri *uri) {
	if (!uri)
		return;

	cleanup_uri(uri);

	free(uri);
}

int parse_uri(struct uri *uri, const char *uristr) {
	char *next;

	init_uri(uri);

	uri->scheme = strndup(uristr, (next = strchr(uristr, ':')) - uristr);

	if (next == NULL)
		return 1;

	if (!(*++next == '/'))
		return 1;

	if (*++next == '/') {
		next++;

		const size_t authlen = strchr(next, '/') - next;

		char *auth = strndup(next, authlen);
		uri->authority = parse_authority_alloc(auth);
		free(auth);

		next += authlen;
	} else {
		uri->authority = NULL;
	}

	const char *const query = strchr(next, '?');
	const char *const fragment = strchr(next, '#');
	const size_t rest = strlen(next);

	uri->query = keyval_alloc();

	if (query) {
		char *querystr = strndup(query, fragment - query);

		if (parse_query(uri->query, querystr))
			return 1;

		free(querystr);
	}

	if (fragment) {
		uri->fragment = strdup(fragment);
	}

	return 0;
}

struct uri *parse_uri_alloc(const char *uristr) {
	struct uri *uri = malloc(sizeof(struct uri));

	if (parse_uri(uri, uristr)) {
		free(uri);
		return NULL;
	}

	return uri;
}

#define TEST 1

#if TEST == 1
#include <assert.h>

struct test {
	const char *uri;
	const char *scheme;
	struct authority authority;
	const char *path;
	struct {
		size_t count;
		struct keyval vars[32];
	} query;
	const char *fragment;
};

struct test tests[] = {
	{
		.uri = "https://john.doe@www.example.com:123/forum/questions/?tag=networking&order=newest#top",
		.scheme = "https",
		.authority = {
			.userinfo = "john.doe",
			.host = "www.example.com",
			.port = 123,
		},
		.path = "/forum/questions/",
		.query = {
			.count = 2,
			.vars = {
				{
					.key = "tag",
					.value = "networking",
				},
				{
					.key = "order",
					.value = "newest",
				},
			},
		},
		.fragment = "top",
	},
	{
		.uri = "ldap://[2001:db8::7]/c=GB?objectClass?one",
		.scheme = "ldap",
		.authority = {
			.host = "[2001:db8::7]",
		},
		.path = "/c=GB",
		.query = {
			.count = 1,
			.vars = {
				{
					.key = "objectClass?one",
					.value = "",
				},
			},
		},
	},
	{
		.uri = "mailto:John.Doe@example.com",
		.scheme = "mailto",
		.path = "John.Doe@example.com",
	},
	{
		.uri = "news:comp.infosystems.www.servers.unix",
		.scheme = "news",
		.path = "comp.infosystems.www.servers.unix",
	},
	{
		.uri = "telnet://192.0.2.16:80/",
		.scheme = "telnet",
		.authority = {
			.host = "192.0.2.16",
			.port = 80,
		},
		.path = "/",
	},
	{
		.uri = "urn:oasis:names:specification:docbook:dtd:xml:4.1.2",
		.scheme = "urn",
		.path = "oasis:names:specification:docbook:dtd:xml:4.1.2",
	},
	{
		.uri = "file:///etc/hostname",
		.scheme = "file",
		.authority = {
			.host = "",
		},
		.path = "/etc/hostname",
	},
};

int main() {
	return 0;
}
#endif
