#include <stdio.h>
#include <unistd.h>
#include <uri.h>

int main(int argc, char **argv) {
	struct uri uri;

	if (argc < 2) {
		fprintf(stderr, "USAGE: %s <uri>\n", argv[0]);
		return 1;
	}

	if (parse_uri(&uri, argv[1])) {
		fprintf(stderr, "uriparse: failed to parse uri.\n");
		return 1;
	}

	if (uri.scheme)
		printf("scheme: %s\n", uri.scheme);

	if (uri.authority) {
		if (uri.authority->userinfo)
			printf("userinfo: %s\n", uri.authority->userinfo);

		printf("host: %s\n", uri.authority->host);

		if (uri.authority->port)
			printf("port: %hu\n", uri.authority->port);
	}

	printf("path: %s\n", uri.path);

	if (uri.query) {
		printf("query:\n");

		keyval_iter(kv, uri.query) {
			printf(" key: %s\n value: %s\n", kv->key, kv->value);
		}
	}

	if (uri.fragment)
		printf("fragment: %s\n", uri.fragment);

	cleanup_uri(&uri);

	return 0;
}
