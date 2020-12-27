#ifndef CLIP_COMMON_H
#define CLIP_COMMON_H

#include <stddef.h>

#define CLIP_DESTIN "us.hilleg.clipd"
#define CLIP_PATH        "/us/hilleg/clipd"
#define CLIP_INTERFACE   "us.hilleg.clipd.Manager"

// The server has several clipboads
#define CLIPBOARD_GENERAL (0)
#define CLIPBOARD_FIND (1)
#define CLIPBOARD_STYLE (2)
#define CLIPBOARD_DRAG (3)
#define CLIPBOARD_COUNT (4)

// Recommended length for labels
#define CLIP_LABEL_LEN (20)

#pragma mark Dealing with type lists

char **clip_create_typelist(size_t count, ...);
void clip_free_typelist(char** types);
int clip_typelist_contains(char **types, const char *target);
int clip_typelists_equal(char **a, char **b);
size_t clip_typelist_count(char **types);

// Convenience method for creating labels
// Must free result after use
char *clip_trim_to_label(const char *text);
// Convenience method for converting data to c string
// Must free result after use
char *clip_string_from_data(const unsigned char *data, size_t datalen);
#endif
