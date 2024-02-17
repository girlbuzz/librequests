# libcrequest

This is a library for handling network requests. It uses standard URI format for resources, it also includes a parser for URIs.

# How to Build

On any GNU/Linux system, or any system where you have access to GNUMake, use `gnumake release` to build release binaries.
They will be located in `./target/release/`.

On any POSIX system, there is a POSIX makefile available (`Makefile.posix`). This will not output a dynamic library
since there is no POSIX standard for dynamic libraries. Binaries will be located in `./`.

# Example

```c
#include <stdio.h>
#include <uri.h>

int main(void) {
    struct uri uri;

    uri_parse(&uri, "https://www.example.com/startpage.html");

    assert(!strcmp(uri.scheme, "https"));
    assert(!strcmp(uri.authority.host, "www.example.com"));
    assert(!strcmp(uri.path, "/startpage.html"));

    /* remember to call `cleanup_*` on most library structs.
     * call `free_*` on ones that are heap allocated.
     * cleanup functions are not safe to call on NULL, free functions are. */
    cleanup_uri(&uri);

    return 0;
}
```
