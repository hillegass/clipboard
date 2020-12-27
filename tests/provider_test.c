#include <stdio.h>
#include <stdlib.h>
#include <systemd/sd-bus.h>
#include "clipboard.h"

int main(int argc, char *argv[]) {
  int r;
    
  // Create a type list
  char **typelist = clip_create_typelist(2, CLIPBOARD_TYPE_TEXT, CLIPBOARD_TYPE_RTF);

  // Data to copy to pasteboard
  char *plain_text = "This is some text that you might want to copy";

  // Create a label
  char *label = clip_trim_to_label(plain_text);

  // Create a clipboard item
  uint16_t item_id = clip_create_item(CLIPBOARD_GENERAL, label, typelist);
  free(label);
  clip_free_typelist(typelist);
  
  r = clip_push_data(CLIPBOARD_GENERAL, item_id, CLIPBOARD_TYPE_TEXT, strlen(rtf_text), rtf_text);
  if (r < 0) {
    fprintf(stderr, "Error pushing data:%s\n", CLIPBOARD_TYPE_TEXT);
  }
  char *rtf_text = "{\\rtf1\\ansi{\\fonttbl\\f0\\fswiss Helvetica;}\\f0\\pard\nThis is some {\\b bold} text that you might want to copy\\par\n}";

  r = clip_push_data(CLIPBOARD_GENERAL, item_id, CLIPBOARD_TYPE_RTF, strlen(plain_text), plain_text);
  if (r < 0) {
    fprintf(stderr, "Error pushing data:%s\n", CLIPBOARD_TYPE_RTF);
  }
}
