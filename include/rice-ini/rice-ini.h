/* rice-ini: a small INI reader. systemd style:
 * [Section], Key=Value, '#' and ';' comment lines, no inline comments, last key wins.
 * Bad lines are skipped and reported; a missing file is an empty ini, not an error.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef RICEINI_H
#define RICEINI_H

#include <stddef.h>
#include <stdint.h>

#define RICEINI_VERSION_MAJOR 0
#define RICEINI_VERSION_MINOR 1
#define RICEINI_VERSION_MICRO 0

/* RICEINI_STATIC: vendored copy, exports nothing. RICEINI_BUILDING: the shared
 * library, exports the API. Neither: a program linking the shared library. */
#if defined(RICEINI_STATIC)
#define RICEINI_API __attribute__((visibility("hidden")))
#elif defined(RICEINI_BUILDING)
#define RICEINI_API __attribute__((visibility("default")))
#else
#define RICEINI_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct riceini riceini_t;

/* Both return NULL only when out of memory. riceini_load gives an empty ini for a
 * missing or unreadable file, and an issue (line 0) for a file over 1 MiB. */
RICEINI_API riceini_t *riceini_parse(const char *text, size_t len);
RICEINI_API riceini_t *riceini_load(const char *path);
RICEINI_API void riceini_free(riceini_t *ini); /* NULL is fine */

/* The value, or NULL when absent. Section and key are case sensitive. The pointer
 * lives until the ini is freed or overlaid. */
RICEINI_API const char *riceini_get(const riceini_t *ini, const char *section, const char *key);

/* Every key in the order first seen (a repeated key keeps its first place). Lets a caller
 * report keys it does not know. NULL for an index out of range. */
RICEINI_API size_t riceini_entry_count(const riceini_t *ini);
RICEINI_API const char *riceini_entry_section(const riceini_t *ini, size_t i);
RICEINI_API const char *riceini_entry_key(const riceini_t *ini, size_t i);

/* Copies every key of over into base; over wins. Returns 0, or -1 out of memory. */
RICEINI_API int riceini_overlay(riceini_t *base, const riceini_t *over);

RICEINI_API size_t riceini_issue_count(const riceini_t *ini);
RICEINI_API int riceini_issue_line(const riceini_t *ini, size_t i); /* 1-based; 0: not a line */
RICEINI_API const char *riceini_issue_text(const riceini_t *ini, size_t i);

/* Typed readers for one raw value. Return 1 and fill the output, or 0 when the
 * text is not valid (the output is untouched). Surrounding blanks are allowed. */
RICEINI_API int riceini_to_color(const char *text, uint8_t rgba[4]); /* #rrggbb or #aarrggbb */
RICEINI_API int riceini_to_bool(const char *text, int *out); /* true/false/yes/no/on/off/1/0 */
RICEINI_API int riceini_to_int(const char *text, int64_t *out);
RICEINI_API int riceini_to_double(const char *text,
                                  double *out); /* -12.5; no exponent, locale free */

#ifdef __cplusplus
}
#endif

#endif
