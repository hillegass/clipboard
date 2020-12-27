#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

extern "C" {
#include "clip_common.h"
#include "clipboard.h"
}

#include "store.h"

int main(int argc, char *argv[]) {

  store_set_ring_size(CLIPBOARD_GENERAL, 5);
  store_set_ring_size(CLIPBOARD_FIND, 10);
  store_set_ring_size(CLIPBOARD_STYLE, 1);
  store_set_ring_size(CLIPBOARD_DRAG, 3);

  assert(store_last_item_id(CLIPBOARD_GENERAL) == 0);
  assert(store_item_count(CLIPBOARD_DRAG) == 0);

  // Create a type list
  char **typelist = clip_create_typelist(2, CLIPBOARD_TYPE_TEXT, CLIPBOARD_TYPE_RTF);

  assert(clip_typelist_count(typelist) == 2);
  assert(clip_typelist_contains(typelist, CLIPBOARD_TYPE_TEXT));
  assert(clip_typelist_contains(typelist, CLIPBOARD_TYPE_RTF));

    // Create a label
  const char *label = "Test label";

  uint16_t item_id;
  uint16_t pushed_out_id;
  char *sender;
  for (int i = 0; i < 12; i++) {
    item_id = store_create_item(CLIPBOARD_GENERAL, label, ":1.132", typelist, &pushed_out_id, &sender);
    if (pushed_out_id) {
      fprintf(stderr, "Pushing item %u onto the pasteboard pushed out %u from %s\n", item_id, pushed_out_id, sender);
      free(sender);
    }
  }
  
  // Data to copy to pasteboard
  const char *plain_text = "This is some text that you might want to copy";
  const char *rtf_text = "{\\rtf1\\ansi{\\fonttbl\\f0\\fswiss Helvetica;}\\f0\\pard\nThis is some {\\b bold} text that you might want to copy\\par\n}";

  int r;
  r = store_store_data(CLIPBOARD_GENERAL, item_id, CLIPBOARD_TYPE_TEXT, strlen(plain_text), (const unsigned char *)plain_text);

  char **whole = store_typelist(CLIPBOARD_GENERAL, item_id);

  assert(clip_typelists_equal(whole, typelist));

  char **partial = store_types_without_data(CLIPBOARD_GENERAL, item_id);
  assert(clip_typelist_contains(partial, CLIPBOARD_TYPE_RTF));
  assert(clip_typelist_count(partial) == 1);

  r = store_store_data(CLIPBOARD_GENERAL, item_id, CLIPBOARD_TYPE_RTF, strlen(rtf_text), (const unsigned char *)rtf_text);

  char **empty = store_types_without_data(CLIPBOARD_GENERAL, item_id);
  assert(clip_typelist_count(empty) == 0);

  char **typelist2 = clip_create_typelist(1, CLIPBOARD_TYPE_PNG);
    
  const char *label2 = "Test label2";

  uint16_t item_id2 = store_create_item(CLIPBOARD_GENERAL, label2, ":1.232", typelist2, NULL, NULL);

  clip_free_typelist(typelist);
  clip_free_typelist(whole);
  clip_free_typelist(partial);
  clip_free_typelist(empty);
  clip_free_typelist(typelist2);
}
