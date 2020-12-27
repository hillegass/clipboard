#include "clip_common.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

char **clip_create_typelist(size_t count, ...)
{
  char **result = (char **)malloc(sizeof(char *) * (count + 1));
  va_list args;
  va_start(args, count);
  for (int i = 0; i < count; i++) {
    result[i] = strdup(va_arg(args, char *));
  }
  va_end(args);
  // NULL terminate
  result[count] = NULL;
  return result;
}

void clip_free_typelist(char** types)
{
  for(int i = 0; types[i] != NULL; i++) {
    free(types[i]);
  }
  free(types);
}

int clip_typelist_contains(char **types, const char *target)
{
  for (int i = 0; types[i] != NULL; i++) {
    if (strcmp(types[i], target) == 0) {
      return 1;
    }
  }
  return 0;
}

size_t clip_typelist_count(char **types) {
  int i;
  for (i = 0; types[i] != NULL; i++);
  return i;
}

int clip_typelists_equal(char **a, char **b)
{
  int i;
  for (i = 0; a[i] != NULL; i++) {
    if (!clip_typelist_contains(b, a[i])) {
      return 0;
    }
  }
  int j = clip_typelist_count(b);
  return (i == j);
}

char *clip_trim_to_label(const char *text)
{
  // FIXME: should deal with UTF properly
  size_t len = strlen(text);
  if (len <= CLIP_LABEL_LEN) {
    return strdup(text);
  } else {
    char *result = malloc(CLIP_LABEL_LEN + 1);
    strncpy(result, text, CLIP_LABEL_LEN - 3);
    result[CLIP_LABEL_LEN - 2] = '.';
    result[CLIP_LABEL_LEN - 1] = '.';
    result[CLIP_LABEL_LEN] = '.';
    result[CLIP_LABEL_LEN + 1] = '\0';
    return result;
  } 
}

char *clip_string_from_data(const unsigned char *data, size_t datalen)
{
  char *result = (char *)malloc(datalen+1);
  memcpy(result, data, datalen);
  result[datalen] = '\0';
  return result;
}
