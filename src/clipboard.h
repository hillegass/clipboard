/*
 * Copyright © 2020 D. Aaron Hillegass
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include "clip_common.h"
#include <stdint.h>

#define CLIPBOARD_TYPE_URL "public.url"
#define CLIPBOARD_TYPE_TEXT "public.utf8-plain-text"
#define CLIPBOARD_TYPE_RTF "public.rtf"
#define CLIPBOARD_TYPE_JPEG "public.jpeg"
#define CLIPBOARD_TYPE_PNG "public.png"
#define CLIPBOARD_TYPE_MP3 "public.mp3"


#pragma mark Data providers

// In all functions taking an item_id, if you supply 0 it will use the last
// item added to the pasteboard.

// clip_create_item creates an item on the clipboad and lists the
// data types that are coming. This is a promise to provide data.
// The item id is returned.
// 'types' should be null terminated list of UTIs in ASCII.
// 'label' is a UTF-8 string that describes the new item in 20 characters or less
// Returns item_id of new item. MAX_UINT16 on error
uint16_t
clip_create_item(uint16_t board, const char *label, char **typelist);

// clip_push_data moves the actual data to the clipboard for the item
// Returns -1 if an error (usually type is not available) occurs
int
clip_push_data(uint16_t board, uint16_t item_id, const char *type, size_t datalen, const char *data);

// Lazy data providers register callback for data
// void size_t provide_data(uint16_t board, uint16_t item, char *datatype, unsigned char** data_ptr)
typedef size_t(*clip_data_provider)(uint16_t, uint16_t, const char *, unsigned char**);

// Returns -1 on error
int clip_set_data_provider(uint16_t board, clip_data_provider provider);

// Lazy data providers need to know when they are no longer responsible for
// supplying data for a particular clipboard item.
// void provider_release(uint16_t board, uint16_t item)
typedef void(*clip_provider_release)(uint16_t, uint16_t);

// Returns negative number on error
int clip_set_provider_release(uint16_t board, clip_provider_release provider_release);

// clip_close disconnects from the clipboard server
// If the data provider is lazy, this function may block while the
// clipboard server gets promised data.
// Returns -1 on error
int clip_close();

#pragma mark Data readers

// clip_item_count tells you what the last item id is and how many items are
// on the clipboard
// Return -1 error (usually can't connect to server, no such board) occurs
int clip_item_count(uint16_t board, uint16_t *last_item_id_ptr, uint16_t *item_count_ptr);

// clip_registered_typelist tells you what types are available for a particular
// item. types will be NULL terminated. Caller is responsible for freeing
// Returns -1 if error occurs
int clip_item_typelist(uint16_t board, uint16_t item_id, char ***types_ptr);

// Actually fetch the data for a particular clipboard/item/type
int
clip_item_data_for_type(uint16_t board, uint16_t item_id, char *type, size_t *datalen,
			unsigned char **bytes);

#pragma mark Listeners

// Listeners register function pointer to be called when new item
// is added to clipboard
// void handle_change(int board, int new_item_id, char *label, size_t item_count);
typedef void (*clip_change_handler)(uint16_t, uint16_t, char *, size_t);

// Returns -1 on error (can't connect to server, no such board)
int clip_set_change_handler(uint16_t board, clip_change_handler ch);

void wait_for_clipboard_events();
void process_waiting_clipboard_events();

#endif
