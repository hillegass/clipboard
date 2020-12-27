#include <stdio.h>
#include <stdlib.h>
#include <systemd/sd-bus.h>
#include "clipboard.h"

static sd_bus *bus = NULL;

// Just one data_provider, one change_handler, one provider_release
// function per pasteboard
static clip_data_provider data_providers[CLIPBOARD_COUNT];
static clip_change_handler change_handlers[CLIPBOARD_COUNT];
static clip_provider_release provider_release[CLIPBOARD_COUNT];

int
clip_open() {
  const char *path;
  int r;
  if (bus != NULL) {
    fprintf(stderr, "Trying to open non-null bus. Reopening?\n");
  }

  r = sd_bus_open_user(&bus);
  if (r < 0) {
    fprintf(stderr, "Failed to connect to user bus: %s\n", strerror(-r));
    return r;
  }
  return 1;
}

int
clip_close(){
  // FIXME: Provide needed data
  sd_bus_unref(bus);
  bus = NULL;
}

uint16_t
clip_create_item(uint16_t board, const char *label, char **typelist)
{
  int r;
  uint16_t item_id = 0;

  if (!bus) {
    r = clip_open();
    if (r < 0) {
      return 0;
    }
  }

  sd_bus_error error = SD_BUS_ERROR_NULL;
  sd_bus_message *send_message = NULL;
  sd_bus_message *reply_message = NULL;

  r = sd_bus_message_new_method_call(bus, &send_message, CLIP_DESTIN, CLIP_PATH,
				     CLIP_INTERFACE, "CreateItem");
  if (r < 0) {
    fprintf(stderr, "Failed to create message to send: %s\n", strerror(-r));
    goto finish;
  }

  sd_bus_message_append(send_message, "q", board);
  sd_bus_message_append(send_message, "s", label);
  sd_bus_message_append_strv(send_message, (char **)typelist);
  
  r = sd_bus_call(bus, send_message, -1, &error, &reply_message);
  if (r < 0) {
    fprintf(stderr, "Call failed in CreateItem\n");
    goto finish;
  }

  uint16_t pushed_out_id;
  r = sd_bus_message_read(reply_message, "qq", &item_id, &pushed_out_id);
  if (r < 0) {
    fprintf(stderr, "Read failed in CreateItem\n");
    goto finish;
  }

 finish:
  sd_bus_error_free(&error);
  sd_bus_message_unref(send_message);
  sd_bus_message_unref(reply_message);
  return item_id;
}

// clip_push_data_for_type moves the actual data to the clipboard for the item
// Returns -1 if an error (usually type is not available) occurs
int clip_push_data(uint16_t board, uint16_t item_id, const char *type, size_t datalen, const char *data)
{
  int r;
  sd_bus_error error = SD_BUS_ERROR_NULL;
  sd_bus_message *send_message = NULL;
  sd_bus_message *reply_message = NULL;
  r = sd_bus_message_new_method_call(bus, &send_message, CLIP_DESTIN, CLIP_PATH,
				     CLIP_INTERFACE, "PushData");
  if (r < 0) {
    fprintf(stderr, "Failed to create message to send: %s\n", strerror(-r));
    goto finish;
  }

  sd_bus_message_append(send_message, "q", board);
  sd_bus_message_append(send_message, "q", item_id);
  sd_bus_message_append(send_message, "s", type);
  sd_bus_message_append_array(send_message, 'y', (const void **)data, datalen);
  
  r = sd_bus_call(bus, send_message, -1, &error, &reply_message);
  if (r < 0) {
    fprintf(stderr, "Call failed in PushData\n");
    goto finish;
  }

  uint16_t pushed_out_id;
  r = sd_bus_message_read(reply_message, "");
  if (r < 0) {
    fprintf(stderr, "Read failed in PushData\n");
    goto finish;
  }

  // Success!
  r = 1;
  
 finish:
  sd_bus_error_free(&error);
  sd_bus_message_unref(send_message);
  sd_bus_message_unref(reply_message);
  return r;
}

// Lazy data providers register callback for data
// void size_t provide_data(uint16_t board, uint16_t item, char *datatype, char** data_ptr)
// Returns -1 on error (usually 'board' does not exist)
int clip_set_data_provider(uint16_t board, clip_data_provider provider)
{
  data_providers[board] = provider;
  return 1;
}

#pragma mark Data readers

// clip_item_count tells you how many items are on the clipboard and
// what the last item_id is.
// Return -1 error (ususually can't connect to server, no such board) occurs
int clip_item_count(uint16_t board, uint16_t *last_item_id_ptr, uint16_t *item_count_ptr)
{
  int r;
  if (!bus) {
    r = clip_open();
    if (r < 0) {
      return r;
    }
  }

  sd_bus_error error = SD_BUS_ERROR_NULL;
  sd_bus_message *m = NULL;

  /* Issue the method call and store the respons message in m */
  r = sd_bus_call_method(bus, CLIP_DESTIN, CLIP_PATH, CLIP_INTERFACE, "ItemCount",
			 &error, &m, "q", board);
  if (r < 0) {
    fprintf(stderr, "Failed to issue method call: %s\n", error.message);
    goto finish;
  }

  /* Parse the response message */
  uint16_t last_item_id;
  uint16_t item_count;
  r = sd_bus_message_read(m, "qq", &last_item_id, &item_count);
  if (r < 0) {
    fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
    goto finish;
  }

  if (last_item_id_ptr) {
    *last_item_id_ptr = last_item_id;
  }

  if (item_count_ptr) {
    *item_count_ptr = item_count;
  }
  
 finish:
  sd_bus_error_free(&error);
  sd_bus_message_unref(m);
  return r;
}

// clip_item_typelist tells you what types are available for a particular
// item. types will be NULL terminated. Caller is responsible for freeing
// Returns -1 if error occurs
int clip_item_typelist(uint16_t board, uint16_t item_id, char ***types_ptr)
{
  int r;
  sd_bus_error error = SD_BUS_ERROR_NULL;
  sd_bus_message *m = NULL;

  /* Issue the method call and store the response message in m */
  r = sd_bus_call_method(bus, CLIP_DESTIN, CLIP_PATH, CLIP_INTERFACE, "FetchTypelist",
			 &error, &m, "qq", board, item_id);
  if (r < 0) {
    fprintf(stderr, "Failed to issue method call: %s\n", error.message);
    goto finish;
  }

  /* Parse the response message */
  r = sd_bus_message_read_strv (m, types_ptr);
  if (r < 0) {
    fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
    goto finish;
  }
  
 finish:
  sd_bus_error_free(&error);
  sd_bus_message_unref(m);
  return r;
}

int clip_item_data_for_type(uint16_t board, uint16_t item_id, char *type, size_t *datalen_ptr, unsigned char **bytes_ptr)
{
  int r;
  sd_bus_error error = SD_BUS_ERROR_NULL;
  sd_bus_message *m = NULL;

  /* Issue the method call and store the response message in m */
  r = sd_bus_call_method(bus, CLIP_DESTIN, CLIP_PATH, CLIP_INTERFACE, "FetchData",
			 &error, &m, "qqs", board, item_id, type);
  if (r < 0) {
    fprintf(stderr, "Failed to issue method call: %s\n", error.message);
    goto finish;
  }

  /* Parse the response message */
  size_t datalen;
  unsigned char *bytes;
  r = sd_bus_message_read_array (m, 'y', (const void **)&bytes, &datalen);
  if (r < 0) {
    fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
    goto finish;
  }

  if (bytes_ptr) {
    *bytes_ptr = (unsigned char *)malloc(datalen);
    memcpy(*bytes_ptr, bytes, datalen);
  }

  if (datalen_ptr) {
    *datalen_ptr = datalen;
  }
  
 finish:
  sd_bus_error_free(&error);
  sd_bus_message_unref(m);
  return r;
}

// Listeners register function pointer to be called when new item
// is added to clipboard
// void handle_change(int board, int new_item_id, char *label, size_t item_count);
// Returns -1 on error (can't connect to server, no such board)
int clip_set_change_handler(uint16_t board, clip_change_handler ch)
{
  change_handlers[board] = ch;
}

