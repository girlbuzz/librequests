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

	init_authority(auth);

	/* parse a potential userinfo */
	if ((n = strchr(authstr, '@'))) {
		auth->userinfo = strndup(authstr, n - authstr);
		authstr = n + 1;
	}

	/* ipv6 can contain : and that will confuse us. handle that case differently */
	if (*authstr == '[') {
		const char *const end = strchr(authstr, ']');

		if (!end)
			return 1;

		auth->host = strndup(authstr, end - authstr + 1);
		authstr = end + 1;
	} else {
		if ((n = strchr(authstr, ':'))) {
			auth->host = strndup(authstr, n - authstr);

			if (*++n == '\0')
				1;

			auth->port = atoi(n);
		} else {
			if (*authstr == '\0')
				auth->host = strdup("");
			else
				auth->host = strdup(authstr);
		}
	}

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

static struct keyval *parse_query(struct keyval *pairs, const char *query) {
	char *mut_query = strdup(query);
	char *saveptr;
	char *s;

	s = strtok_r(mut_query, "&", &saveptr);

	if (s == NULL)
		return NULL;

	do {
		char *value = strchr(s, '=');

		if (value) {
			*value = '\0';
			value++;
		} else {
			value = "";
		}

		pairs = kv_set_value(pairs, s, value);
	} while ((s = strtok_r(NULL, "&", &saveptr)) != NULL);

	free(mut_query);

	return pairs;
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

/* this function does not deallocate any parts of the URI if an error occurs.
 * this is to give the user of the library to potentially recover from an error.
 * it is safe to call cleanup_uri on uri if an error occurs.
 * 
 * this function will not fix incorrectly formed URIs, it is strict. */
int parse_uri(struct uri *uri, const char *uristr) {
	const char *next;
	size_t rest;

	init_uri(uri);

	/* get uri scheme. this part is required. */
	const char *const colon = strchr(uristr, ':');

	if (!colon)
		return 1;

	uri->scheme = strndup(uristr, colon - uristr);

	next = colon + 1;

	/* parse an authority if one exists. this can be an empty string. */
	if (!strncmp(next, "//", 2) || *next == '[') {
		next += *next == '[' ? 1 : 2;

		/* if we have an authority, it will always end with a / */
		const char *const slash = strchr(next, '/');

		if (!slash)
			return 1;

		const size_t authlen = slash ? slash - next : strlen(next);

		char *auth = strndup(next, authlen);
		uri->authority = parse_authority_alloc(auth);
		free(auth);

		next += authlen;
	} else {
		uri->authority = NULL;
	}

	/* from here, theres just the path, and potentially a ?query or #fragment */
	const char *const inq = strchr(next, '?');
	const char *const query = inq + 1;
	const char *const pound = strchr(next, '#');
	const char *const fragment = pound + 1;

	rest = strlen(next);

	/* because of our rest variable, we check for a fragment first. We wouldn't want to
	 * set rest to be up to query only to then replace it with the start of fragment. */
	if (pound) {
		uri->fragment = strdup(fragment);
		rest = rest - strlen(pound);
	}

	/* finally, check for a query */
	if (inq) {
		const size_t fragment_len = pound ? strlen(pound) : 0;
		char *querystr = strndup(query, strlen(query) - fragment_len);
		rest = inq - next;

		uri->query = keyval_alloc();

		if ((uri->query = parse_query(uri->query, querystr)) == NULL)
			return 1;

		free(querystr);
	}

	uri->path = strndup(next, rest);

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
	{
		.uri = "steam://run/440",
		.scheme = "steam",
		.authority = {
			.host = "run",
		},
		.path = "/440",
	},
};

void run_test(const struct test *test) {
	struct uri uri;

	printf("Testing '%s'\n", test->uri);

	parse_uri(&uri, test->uri);

	if (test->scheme && strcmp(uri.scheme, test->scheme)) {
		fprintf(stderr, "scheme does not match. Should be '%s', was '%s'\n", test->scheme, uri.scheme);
		exit(1);
	}

	if (test->authority.host) {
		if (test->authority.userinfo && strcmp(uri.authority->userinfo, test->authority.userinfo)) {
			fprintf(stderr, "User info does not match. Should be '%s', was '%s'\n", test->authority.userinfo, uri.authority->userinfo);
			exit(1);
		}

		if (test->authority.host && strcmp(uri.authority->host, test->authority.host)) {
			fprintf(stderr, "User info does not match. Should be '%s', was '%s'\n", test->authority.host, uri.authority->host);
			exit(1);
		}

		if (test->authority.port && uri.authority->port != test->authority.port) {
			fprintf(stderr, "User info does not match. Should be '%hu', was '%hu'\n", test->authority.port, uri.authority->port);
			exit(1);
		}
	}

	if (test->path && strcmp(uri.path, test->path)) {
		fprintf(stderr, "Path failed to parse. Should be '%s', was '%s'\n", test->path, uri.path);
		exit(1);
	}

	if (test->query.count > 0) {
		size_t i;
		
		for (i = 0; i < test->query.count; i++) {
			const char *var = kv_get_value(uri.query, test->query.vars[i].key);

			if (!var) {
				fprintf(stderr, "Value not found.\n");
				exit(1);
			}

			if (strcmp(var, test->query.vars[i].value)) {
				fprintf(stderr, "Value failed to parse. Should be '%s=%s', was '%s'\n",
					test->query.vars[i].key,
					test->query.vars[i].value,
					var);

				exit(1);
			}
		}
	}

	if (test->fragment && strcmp(uri.fragment, test->fragment)) {
		fprintf(stderr, "Fragment failed to parse. Should be '%s', was '%s'\n", test->fragment, uri.fragment);
		exit(1);
	}

	printf("Passed.\n");

	cleanup_uri(&uri);
}

int main() {
	size_t i;

	for (i = 0; i < sizeof(tests) / sizeof(struct test); i++) {
		run_test(&tests[i]);
	}

	return 0;
}
#endif
