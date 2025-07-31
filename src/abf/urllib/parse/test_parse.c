#include "parse.h"
#include <stdio.h>

void print_component(const char *name, url_component_t *comp) {
    printf("%s: ", name);
    if (comp->start && comp->length) {
        fwrite(comp->start, 1, comp->length, stdout);
    }
    printf("\n");
}

int main() {
    char *url = "http://user:pass@host:80/path;params?query=1#frag";
    url_parse_result_t result;
    int err = url_parse(url, strlen(url), NULL, true, &result);
    if (err != URL_PARSE_OK) {
        printf("url_parse error: %d\n", err);
        return 1;
    }
    print_component("scheme", &result.scheme);
    print_component("netloc", &result.netloc);
    print_component("path", &result.path);
    print_component("params", &result.params);
    print_component("query", &result.query);
    print_component("fragment", &result.fragment);
    printf("has_params: %d\n", (int)result.has_params);

    // Test quote (use mutable buffer, sized for worst-case expansion)
    const char *orig = "abc def/!";
    size_t orig_len = strlen(orig);
    size_t bufsize = orig_len * 3 + 1; // worst-case: every char encoded
    char buf[bufsize];
    memcpy(buf, orig, orig_len);
    buf[orig_len] = '\0';
    err = url_quote(buf, orig_len, "/", 1);
    if (err != URL_PARSE_OK) {
        printf("url_quote error: %d\n", err);
        return 1;
    }
    printf("quoted: %s\n", buf);
    return 0;
}
