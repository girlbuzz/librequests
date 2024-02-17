#ifndef __URL_H
#define __URL_H

struct authority {
	char *userinfo;
	char *host;
	unsigned short port;
};

struct uri {
	char *scheme;
	struct authority *authority;
	char *path;
	struct keyval *query;
	char *fragment;
};

void init_authority(struct authority *auth);

void cleanup_authority(struct authority *auth);

/* URL */

void init_uri(struct uri *uri);

void cleanup_uri(struct uri *uri);

int parse_uri(struct uri *uri, const char *uristr);

struct uri *parse_uri_alloc(const char *uristr);

#endif /* __URL_H */
