#ifndef STORE_H
#define STORE_H

#include <stdint.h>
// item_ids are never 0.

// Done at start up to set the number of items that can live on a clipboard
void store_set_ring_size(uint16_t clipboard_id, uint16_t max_items);

// What is the id of the last item added? 0 if there are no items
uint16_t store_last_item_id(uint16_t clipboard_id);

// How many items are on the clipboard?
uint16_t store_item_count(uint16_t clipboard_id);

// Create an item (including types and a label for observers)
// Get the id of item pushed out by reference, 0 if none
// You don't own the label or the sender -- don't free them
// You do own the typelist -- free it when you are done
uint16_t
store_create_item(uint16_t clipboard_id, const char *label, const char *sender, char **typelist,
		  uint16_t* pushed_out_id, char **pushed_sender);

// Hold this data for this clipboard/item/type
// Returns -1 if unsuccessful
int
store_store_data(uint16_t clipboard_id, uint16_t item_id, const char *type, size_t datalen,
		 const unsigned char *data);

// Get data for this clipboard/item/type
int store_fetch_data(uint16_t clipboard_id, uint16_t item_id, char *type, size_t *datalenptr, unsigned char **dataptr);

// Look up who created the item (You don't own the returned string -- don't free it)
const char *store_sender_for_item(uint16_t clipboard_id, uint16_t item_id);

// Get a label for the item (You don't own the returned string -- don't free it)
const char *store_label_for_item(uint16_t clipboard_id, uint16_t item_id);

// What is the typelist for this clipboard/item?
// Returns NULL if nonexistent
// Receiver should free result
char **store_typelist(uint16_t clipboard_id, uint16_t item_id);

// What types were promised, but not yet fulfilled?
char **store_types_without_data(uint16_t clipboard_id, uint16_t item_id);

#endif
