#include <stdlib.h>

#include "lexer-private.h"
#include "utils.h"

int named_elements_cmp(const void *a, const void *b)
{
    const named_element_t *na, *nb;

    na = (const named_element_t *) a; /* key */
    nb = (const named_element_t *) b;

    return strcmp_l(na->name, na->name_len, nb->name, nb->name_len);
}

int named_elements_casecmp(const void *a, const void *b)
{
    const named_element_t *na, *nb;

    na = (const named_element_t *) a; /* key */
    nb = (const named_element_t *) b;

    return ascii_strcasecmp_l(na->name, na->name_len, nb->name, nb->name_len);
}