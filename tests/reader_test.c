#include <stdio.h>
#include <stdlib.h>
#include <systemd/sd-bus.h>
#include "clipboard.h"

int main(int argc, char *argv[]) {
  int r;
  uint16_t last_item_id, item_count;
  r = clip_item_count(CLIPBOARD_GENERAL, &last_item_id, &item_count);
  if (r < 0) {
    fprintf(stderr, "clip_item_count failed\n");
    return 1;
  }



  fprintf(stderr, "clip_item_count: last item id = %u, count = %u\n", last_item_id, item_count);
  
  if (last_item_id == 0) {
    fprintf(stderr, "No data on the clipboard\n");
    return 1;
  }
  
  char **typelist;
  r = clip_item_typelist(CLIPBOARD_GENERAL, last_item_id, &typelist);
  if (r < 0) {
    fprintf(stderr, "clip_item_typelist failed\n");
    return 1;
  }  
  
  char **current_type = typelist;
  fprintf(stderr, "Available types for %u:\n", last_item_id);
  while (*current_type) {
    fprintf(stderr, "\t%s\n", *current_type);
    current_type++;
  }
  clip_free_typelist(typelist);
  typelist = NULL;

  size_t datalen;
  unsigned char *data;
  r = clip_item_data_for_type(CLIPBOARD_GENERAL, last_item_id, CLIPBOARD_TYPE_TEXT, &datalen, &data);
  if (r < 0) {
    fprintf(stderr, "Failed to fetch data for %u:%s\n", last_item_id, CLIPBOARD_TYPE_TEXT);
  } else {
    char *str = clip_string_from_data(data, datalen);
    fprintf(stderr, "Fetched %lu bytes:\"%s\"\n", datalen, str);
    free(str);
  }
  return 1;
}
