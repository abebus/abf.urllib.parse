#include "parse.h"
#include <stdio.h>
#include <string.h>

void print_component(const char *name, url_component_t *comp) {
    printf("%s: ", name);
    if (comp->start && comp->length) {
        fwrite(comp->start, 1, comp->length, stdout);
    }
    printf("\n");
}

int main() {
    const char *url = "http://user:pass@host:80/path;params?query=1#frag";
    url_parse_result_t result;
    int err = url_parse(url, strlen(url), NULL, 0, 1, &result);
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
    printf("has_params: %d\n", result.has_params);

    // Test quote
    const char *to_quote = "abc def/!";
    quote_result_t qres;
    err = url_quote(to_quote, strlen(to_quote), "/", 1, &qres);
    if (err != URL_PARSE_OK) {
        printf("url_quote error: %d\n", err);
        return 1;
    }
    printf("quoted: %s\n", qres.data);
    quote_result_free(&qres);
    return 0;
}
