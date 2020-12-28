#include <stdio.h>
#include <stdlib.h>
#include <systemd/sd-bus.h>
#include "clipboard.h"

void on_change(uint16_t board, uint16_t new_item_id, char *label, size_t item_count)
{
  fprintf(stderr, "Clipboard %u: New item %u \"%s\" added, total items: %lu\n",
	  board, new_item_id, label, item_count);
}

int main(int argc, char *argv[]) {
  clip_set_change_handler(CLIPBOARD_GENERAL, on_change);
  for (;;) {
    process_waiting_clipboard_events();
    wait_for_clipboard_events();
  }
  return 0;
}
