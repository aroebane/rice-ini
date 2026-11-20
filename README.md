# rice-ini

A small INI reader. Pure C99, no dependencies. Meson build, pkg-config file, shared
library by default; it can also be built as a static library that exports nothing.

Format (systemd style): `[Section]`, `Key=Value`, lines starting with `#` or `;` are
comments, no inline comments, no quoting, the last duplicate key wins, `=` may appear
in a value. Section and key names are case sensitive.

    riceini_t *ini = riceini_load("/path/data.conf");   /* missing file: empty, no error */
    const char *v = riceini_get(ini, "Shape", "Radius");
    long r;
    if (v && riceini_to_int(v, &r)) { ... }
    riceini_overlay(base, ini);                        /* ini wins, key by key */
    riceini_free(ini);

Bad lines are skipped and listed by `riceini_issue_count/line/text`.
`riceini_entry_count/section/key` walk every key in file order, so a caller can report
keys it does not know. `riceini_parse(text, len)` reads from memory instead of a file; a
UTF-8 byte order mark at the start is skipped. Typed readers (`riceini_to_color`, `_bool`,
`_int`, `_double`) return 0 for invalid text. Doubles are parsed without the C locale, so
`1.5` works in any application.

Schemas (which sections and keys exist) belong to the caller, not to this library.

## Licence

GPL-3.0-or-later (`LICENSE`). No third-party code; see `NOTICE` and `CREDITS.md`.

## Build

    make build
    make check          # format-check, test, lint (clang-tidy), shellcheck
    make install        # PREFIX defaults to ~/.local
    make compdb         # compile_commands.json for clangd, via bear
