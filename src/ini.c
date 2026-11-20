/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <limits.h>
#include <stdint.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rice-ini/rice-ini.h"

#define MAX_FILE ((size_t)1024 * 1024)

struct entry {
    char *section, *key, *value;
};
struct issue {
    int line;
    char *text;
};

struct riceini {
    struct entry *entries;
    size_t n_entries, cap_entries;
    struct issue *issues;
    size_t n_issues, cap_issues;
};

static char *dup_n(const char *s, size_t n)
{
    char *p = malloc(n + 1);
    if (!p)
        return NULL;
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

static int blank(char c)
{
    return c == ' ' || c == '\t';
}

/* Trims blanks at both ends of [*s, *s + *n). */
static void trim(const char **s, size_t *n)
{
    while (*n && blank((*s)[0])) {
        (*s)++;
        (*n)--;
    }
    while (*n && blank((*s)[*n - 1]))
        (*n)--;
}

static int add_issue(riceini_t *ini, int line, const char *text)
{
    if (ini->n_issues == ini->cap_issues) {
        size_t cap = ini->cap_issues ? ini->cap_issues * 2 : 4;
        struct issue *p = realloc(ini->issues, cap * sizeof *p);
        if (!p)
            return -1;
        ini->issues = p;
        ini->cap_issues = cap;
    }
    ini->issues[ini->n_issues].text = dup_n(text, strlen(text));
    if (!ini->issues[ini->n_issues].text)
        return -1;
    ini->issues[ini->n_issues++].line = line;
    return 0;
}

static struct entry *find(const riceini_t *ini, const char *section, const char *key)
{
    size_t i;
    for (i = 0; i < ini->n_entries; i++)
        if (!strcmp(ini->entries[i].section, section) && !strcmp(ini->entries[i].key, key))
            return &ini->entries[i];
    return NULL;
}

/* Stores a copy; replaces an existing key. Returns 0 or -1. */
static int set(riceini_t *ini, const char *section, const char *key, const char *value)
{
    struct entry *e = find(ini, section, key);
    char *v = dup_n(value, strlen(value));
    if (!v)
        return -1;
    if (e) {
        free(e->value);
        e->value = v;
        return 0;
    }
    if (ini->n_entries == ini->cap_entries) {
        size_t cap = ini->cap_entries ? ini->cap_entries * 2 : 16;
        struct entry *p = realloc(ini->entries, cap * sizeof *p);
        if (!p) {
            free(v);
            return -1;
        }
        ini->entries = p;
        ini->cap_entries = cap;
    }
    e = &ini->entries[ini->n_entries];
    e->section = dup_n(section, strlen(section));
    e->key = dup_n(key, strlen(key));
    e->value = v;
    if (!e->section || !e->key) {
        free(e->section);
        free(e->key);
        free(v);
        return -1;
    }
    ini->n_entries++;
    return 0;
}

void riceini_free(riceini_t *ini)
{
    size_t i;
    if (!ini)
        return;
    for (i = 0; i < ini->n_entries; i++) {
        free(ini->entries[i].section);
        free(ini->entries[i].key);
        free(ini->entries[i].value);
    }
    for (i = 0; i < ini->n_issues; i++)
        free(ini->issues[i].text);
    free(ini->entries);
    free(ini->issues);
    free(ini);
}

static int parse_line(riceini_t *ini, char **section, int line, const char *s, size_t n)
{
    const char *eq;
    const char *k, *v;
    size_t kn, vn;
    char *key = NULL, *val = NULL;
    int rc = 0;

    trim(&s, &n);
    if (!n || s[0] == '#' || s[0] == ';')
        return 0;

    if (s[0] == '[') {
        const char *name = s + 1;
        size_t nn = n - 1;
        free(*section);
        *section = NULL;
        if (s[n - 1] != ']' || n < 2)
            return add_issue(ini, line, "bad section header");
        nn--;
        trim(&name, &nn);
        if (!nn)
            return add_issue(ini, line, "empty section name");
        *section = dup_n(name, nn);
        return *section ? 0 : -1;
    }

    eq = memchr(s, '=', n);
    if (!eq)
        return add_issue(ini, line, "missing '='");
    k = s;
    kn = (size_t)(eq - s);
    trim(&k, &kn);
    if (!kn)
        return add_issue(ini, line, "empty key");
    if (!*section)
        return add_issue(ini, line, "key outside a section");
    v = eq + 1;
    vn = (size_t)((s + n) - v);
    trim(&v, &vn);
    key = dup_n(k, kn);
    val = dup_n(v, vn);
    if (!key || !val)
        rc = -1;
    else
        rc = set(ini, *section, key, val);
    free(key);
    free(val);
    return rc;
}

riceini_t *riceini_parse(const char *text, size_t len)
{
    riceini_t *ini = calloc(1, sizeof *ini);
    char *section = NULL;
    size_t pos = 0;
    int line = 0;

    if (!ini)
        return NULL;
    /* A UTF-8 byte order mark on the first line is skipped (editors add it). */
    if (len >= 3 && memcmp(text, "\xEF\xBB\xBF", 3) == 0)
        pos = 3;
    while (pos < len) {
        const char *nl = memchr(text + pos, '\n', len - pos);
        size_t end = nl ? (size_t)(nl - text) : len;
        size_t n = end - pos;
        line++;
        /* NOLINTNEXTLINE(clang-analyzer-security.ArrayBound): n <= len - pos by construction */
        if (n && text[pos + n - 1] == '\r')
            n--;
        if (parse_line(ini, &section, line, text + pos, n) < 0) {
            free(section);
            riceini_free(ini);
            return NULL;
        }
        pos = end + 1;
    }
    free(section);
    return ini;
}

riceini_t *riceini_load(const char *path)
{
    FILE *fp = path ? fopen(path, "rb") : NULL;
    char *buf;
    size_t n;
    riceini_t *ini;

    if (!fp)
        return riceini_parse("", 0);
    buf = malloc(MAX_FILE + 1);
    if (!buf) {
        (void)fclose(fp);
        return NULL;
    }
    n = fread(buf, 1, MAX_FILE + 1, fp);
    (void)fclose(fp);
    if (n > MAX_FILE) {
        ini = riceini_parse("", 0);
        if (ini && add_issue(ini, 0, "file larger than 1 MiB, ignored") < 0) {
            riceini_free(ini);
            ini = NULL;
        }
    } else {
        ini = riceini_parse(buf, n);
    }
    free(buf);
    return ini;
}

const char *riceini_get(const riceini_t *ini, const char *section, const char *key)
{
    const struct entry *e = ini && section && key ? find(ini, section, key) : NULL;
    return e ? e->value : NULL;
}

size_t riceini_entry_count(const riceini_t *ini)
{
    return ini ? ini->n_entries : 0;
}

const char *riceini_entry_section(const riceini_t *ini, size_t i)
{
    return ini && i < ini->n_entries ? ini->entries[i].section : NULL;
}

const char *riceini_entry_key(const riceini_t *ini, size_t i)
{
    return ini && i < ini->n_entries ? ini->entries[i].key : NULL;
}

int riceini_overlay(riceini_t *base, const riceini_t *over)
{
    size_t i;
    for (i = 0; i < over->n_entries; i++)
        if (set(base, over->entries[i].section, over->entries[i].key, over->entries[i].value) < 0)
            return -1;
    return 0;
}

size_t riceini_issue_count(const riceini_t *ini)
{
    return ini ? ini->n_issues : 0;
}

int riceini_issue_line(const riceini_t *ini, size_t i)
{
    return ini && i < ini->n_issues ? ini->issues[i].line : 0;
}

const char *riceini_issue_text(const riceini_t *ini, size_t i)
{
    return ini && i < ini->n_issues ? ini->issues[i].text : NULL;
}

/* ---- typed readers ---- */

static int hex(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

int riceini_to_color(const char *text, uint8_t rgba[4])
{
    size_t n, i;
    int d[8];
    if (!text)
        return 0;
    n = strlen(text);
    trim(&text, &n);
    if (!(n == 7 || n == 9) || text[0] != '#')
        return 0;
    for (i = 1; i < n; i++) {
        d[i - 1] = hex(text[i]);
        if (d[i - 1] < 0)
            return 0;
    }
    if (n == 7) {
        rgba[0] = (uint8_t)(d[0] * 16 + d[1]);
        rgba[1] = (uint8_t)(d[2] * 16 + d[3]);
        rgba[2] = (uint8_t)(d[4] * 16 + d[5]);
        rgba[3] = 255;
    } else {
        rgba[3] = (uint8_t)(d[0] * 16 + d[1]);
        rgba[0] = (uint8_t)(d[2] * 16 + d[3]);
        rgba[1] = (uint8_t)(d[4] * 16 + d[5]);
        rgba[2] = (uint8_t)(d[6] * 16 + d[7]);
    }
    return 1;
}

static int word_is(const char *s, size_t n, const char *w)
{
    size_t i;
    if (strlen(w) != n)
        return 0;
    for (i = 0; i < n; i++)
        if (tolower((unsigned char)s[i]) != w[i])
            return 0;
    return 1;
}

int riceini_to_bool(const char *text, int *out)
{
    size_t n;
    if (!text)
        return 0;
    n = strlen(text);
    trim(&text, &n);
    if (word_is(text, n, "true") || word_is(text, n, "yes") || word_is(text, n, "on") ||
        word_is(text, n, "1")) {
        *out = 1;
        return 1;
    }
    if (word_is(text, n, "false") || word_is(text, n, "no") || word_is(text, n, "off") ||
        word_is(text, n, "0")) {
        *out = 0;
        return 1;
    }
    return 0;
}

int riceini_to_int(const char *text, int64_t *out)
{
    char *buf, *end;
    size_t n;
    long long v;
    if (!text)
        return 0;
    n = strlen(text);
    trim(&text, &n);
    if (!n || n > 32)
        return 0;
    buf = dup_n(text, n);
    if (!buf)
        return 0;
    errno = 0;
    v = strtoll(buf, &end, 10);
    n = (*end == '\0' && errno == 0 && !isspace((unsigned char)buf[0])) ? 1 : 0;
    free(buf);
    if (n)
        *out = (int64_t)v;
    return (int)n;
}

int riceini_to_double(const char *text, double *out)
{
    size_t n, i = 0;
    double v = 0.0, scale = 1.0;
    int neg = 0, digits = 0;
    if (!text)
        return 0;
    n = strlen(text);
    trim(&text, &n);
    if (i < n && (text[i] == '-' || text[i] == '+'))
        neg = text[i++] == '-';
    for (; i < n && isdigit((unsigned char)text[i]); i++, digits++)
        v = v * 10.0 + (text[i] - '0');
    if (i < n && text[i] == '.') {
        for (i++; i < n && isdigit((unsigned char)text[i]); i++, digits++) {
            scale /= 10.0;
            v += (text[i] - '0') * scale;
        }
    }
    if (!digits || i != n)
        return 0;
    *out = neg ? -v : v;
    return 1;
}
