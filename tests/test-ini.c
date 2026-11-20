/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <string.h>

#include "rice-ini/rice-ini.h"

static int failures;
#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c);                                    \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)
#define PARSE(s) riceini_parse(s, strlen(s))
#define STR_IS(a, b) ((a) && !strcmp((a), (b)))

static void basic(void)
{
    riceini_t *i = PARSE("# c\n; c\n[A]\nk = v \n  x=1\n\n[B]\ny=2\n");
    CHECK(STR_IS(riceini_get(i, "A", "k"), "v"));
    CHECK(STR_IS(riceini_get(i, "A", "x"), "1"));
    CHECK(STR_IS(riceini_get(i, "B", "y"), "2"));
    CHECK(!riceini_get(i, "B", "k"));
    CHECK(!riceini_get(i, "b", "y"));
    CHECK(riceini_issue_count(i) == 0);
    riceini_free(i);
}

static void bom(void)
{
    riceini_t *i = riceini_parse("\xEF\xBB\xBF[A]\nk=1\n", 11);
    CHECK(riceini_issue_count(i) == 0);
    CHECK(STR_IS(riceini_get(i, "A", "k"), "1"));
    riceini_free(i);
}

static void value_rules(void)
{
    riceini_t *i = PARSE("[A]\nk=1\nk=2\ne=a=b\nc=1 # x\nempty=\n");
    CHECK(STR_IS(riceini_get(i, "A", "k"), "2"));
    CHECK(STR_IS(riceini_get(i, "A", "e"), "a=b"));
    CHECK(STR_IS(riceini_get(i, "A", "c"), "1 # x"));
    CHECK(STR_IS(riceini_get(i, "A", "empty"), ""));
    riceini_free(i);
    i = PARSE("[A]\r\nk=1\r\n");
    CHECK(STR_IS(riceini_get(i, "A", "k"), "1"));
    riceini_free(i);
    i = PARSE("[A]\nk=last");
    CHECK(STR_IS(riceini_get(i, "A", "k"), "last"));
    riceini_free(i);
}

static void bad_lines(void)
{
    riceini_t *i = PARSE("k=1\n[A\n[B]\nnoequals\n=novalue\nok=1\n[]\nafter=1\n");
    CHECK(riceini_issue_count(i) == 6);
    CHECK(riceini_issue_line(i, 0) == 1);
    CHECK(riceini_issue_line(i, 1) == 2);
    CHECK(riceini_issue_line(i, 2) == 4);
    CHECK(riceini_issue_line(i, 3) == 5);
    CHECK(riceini_issue_line(i, 4) == 7);
    CHECK(riceini_issue_line(i, 5) == 8); /* keys after a bad header belong to no section */
    CHECK(riceini_issue_text(i, 0) != NULL);
    CHECK(riceini_issue_text(i, 99) == NULL);
    CHECK(!riceini_get(i, "B", "noequals"));
    CHECK(STR_IS(riceini_get(i, "B", "ok"), "1"));
    riceini_free(i);
}

static void files(void)
{
    riceini_t *i = riceini_load("/nonexistent/rice.conf");
    CHECK(i && riceini_issue_count(i) == 0 && !riceini_get(i, "A", "k"));
    riceini_free(i);
    i = riceini_load(NULL);
    CHECK(i && riceini_issue_count(i) == 0);
    riceini_free(i);
    riceini_free(NULL);
    CHECK(!riceini_get(NULL, "A", "k"));
}

static void overlay(void)
{
    riceini_t *base = PARSE("[Colors]\nWindow=#111111\nText=#eeeeee\n");
    riceini_t *over = PARSE("[Colors]\nWindow=#222222\n[Shape]\nRadius=3\n");
    CHECK(riceini_overlay(base, over) == 0);
    CHECK(STR_IS(riceini_get(base, "Colors", "Window"), "#222222"));
    CHECK(STR_IS(riceini_get(base, "Colors", "Text"), "#eeeeee"));
    CHECK(STR_IS(riceini_get(base, "Shape", "Radius"), "3"));
    CHECK(STR_IS(riceini_get(over, "Colors", "Window"), "#222222"));
    riceini_free(base);
    riceini_free(over);
}

static void many_keys(void)
{
    char text[8192] = "[A]\n";
    char line[32];
    int n;
    riceini_t *i;
    for (n = 0; n < 400; n++) {
        sprintf(line, "k%d=%d\n", n, n);
        strcat(text, line);
    }
    i = PARSE(text);
    CHECK(STR_IS(riceini_get(i, "A", "k399"), "399"));
    CHECK(STR_IS(riceini_get(i, "A", "k0"), "0"));
    riceini_free(i);
}

static void typed(void)
{
    uint8_t c[4];
    int b = -1;
    int64_t l = 0;
    double d = 0;
    CHECK(riceini_to_color("#102030", c) && c[0] == 0x10 && c[1] == 0x20 && c[2] == 0x30 &&
          c[3] == 255);
    CHECK(riceini_to_color(" #80102030 ", c) && c[3] == 0x80 && c[0] == 0x10 && c[2] == 0x30);
    CHECK(riceini_to_color("#ABCDEF", c) && c[0] == 0xAB);
    CHECK(!riceini_to_color("102030", c));
    CHECK(!riceini_to_color("#1020", c));
    CHECK(!riceini_to_color("#gg2030", c));
    CHECK(!riceini_to_color(NULL, c));
    CHECK(riceini_to_bool("Yes", &b) && b == 1);
    CHECK(riceini_to_bool("off", &b) && b == 0);
    CHECK(riceini_to_bool(" TRUE ", &b) && b == 1);
    b = 7;
    CHECK(!riceini_to_bool("maybe", &b) && b == 7);
    CHECK(riceini_to_int(" 12 ", &l) && l == 12);
    CHECK(riceini_to_int("-3", &l) && l == -3);
    CHECK(!riceini_to_int("1.5", &l));
    CHECK(!riceini_to_int("", &l));
    CHECK(!riceini_to_int("99999999999999999999", &l));
    CHECK(riceini_to_double("1.5", &d) && d == 1.5);
    CHECK(riceini_to_double("-2.25", &d) && d == -2.25);
    CHECK(riceini_to_double("6", &d) && d == 6.0);
    CHECK(riceini_to_double(".5", &d) && d == 0.5);
    CHECK(!riceini_to_double("x", &d));
    CHECK(!riceini_to_double("1e3", &d));
    CHECK(!riceini_to_double("nan", &d));
    CHECK(!riceini_to_double(".", &d));
}

static void entries(void)
{
    riceini_t *i = PARSE("[A]\nk=1\nx=2\nk=3\n[B]\ny=4\n");
    CHECK(riceini_entry_count(i) == 3);
    CHECK(STR_IS(riceini_entry_section(i, 0), "A") && STR_IS(riceini_entry_key(i, 0), "k"));
    CHECK(STR_IS(riceini_entry_key(i, 1), "x"));
    CHECK(STR_IS(riceini_entry_section(i, 2), "B") && STR_IS(riceini_entry_key(i, 2), "y"));
    CHECK(!riceini_entry_key(i, 3) && !riceini_entry_section(i, 99));
    CHECK(riceini_entry_count(NULL) == 0);
    riceini_free(i);
}

int main(void)
{
    basic();
    entries();
    bom();
    value_rules();
    bad_lines();
    files();
    overlay();
    many_keys();
    typed();
    if (failures)
        printf("%d failure(s)\n", failures);
    return failures ? 1 : 0;
}
