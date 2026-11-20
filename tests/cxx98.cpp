// SPDX-License-Identifier: GPL-3.0-or-later
// The public header compiles as C++98.
#include "rice-ini/rice-ini.h"

int main()
{
    riceini_t *i = riceini_parse("[A]\nk=1\n", 8);
    const char *v = riceini_get(i, "A", "k");
    riceini_free(i);
    return v ? 0 : 1;
}
