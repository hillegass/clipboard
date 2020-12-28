#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <systemd/sd-bus.h>
extern "C" {
#include "clip_common.h"
}
#include "store.h"

static uint16_t last_item_id = 0;

static int method_create_item(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
  int r;
  uint16_t clipboard;
  char *label;
  r = sd_bus_message_read(m, "qs", &clipboard, &label);
  if (r < 0) {
    fprintf(stderr, "Failed to parse clipboard ID and label in CreateItem: %s\n", strerror(-r));
    return r;
  }
  
  char **typelist;
  r = sd_bus_message_read_strv(m, &typelist);
  if (r < 0) {
    fprintf(stderr, "Failed to parse typelist in CreateItem: %s\n", strerror(-r));
    return r;
  }

  const char *sender = sd_bus_message_get_sender(m);
  
  char **current_type = typelist;
  uint16_t pushed_out_id;
  char *owner;
  uint16_t last_item_id = store_create_item(clipboard, label, sender, typelist, &pushed_out_id, &owner);
  if (owner) {
    // FIXME: tell the former owner that they are released from duty
    free(owner);
  }
  r = sd_bus_reply_method_return(m, "qq", last_item_id, pushed_out_id);
  if (r < 0) {
    fprintf(stderr, "Unable to return in CreateItem\n");
  }
  
  sd_bus* bus = sd_bus_message_get_bus(m);
  sd_bus_message *signal = NULL;
  r = sd_bus_message_new_signal(bus, &signal, CLIP_PATH, CLIP_INTERFACE, "ClipboardChanged");
  if (r < 0) {
    fprintf(stderr, "Creation of signal message failed\n");
    return r;
  }
  uint16_t item_count = store_item_count(clipboard);
  r = sd_bus_message_append(signal,"qqsq", clipboard, last_item_id, label, item_count);
  return sd_bus_send(bus, signal, NULL);
}

static int method_push_data(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
  int r;
  uint16_t clipboard;
  uint16_t item_id;
  char *type;
  r = sd_bus_message_read(m, "qqs", &clipboard, &item_id, &type);
  if (r < 0) {
    fprintf(stderr, "Failed to parse clipboard ID, item_id, and type in PushData: %s\n", strerror(-r));
    return r;
  }
  
  unsigned char *data;
  size_t datalen;
  r = sd_bus_message_read_array(m, 'y', (const void **)&data, &datalen);
  if (r < 0) {
    fprintf(stderr, "Failed to parse data in PushData: %s\n", strerror(-r));
    return r;
  }

  // This will copy the type and data (which will be invalid after message
  // is freed
  r = store_store_data(clipboard, item_id, type, datalen, data);
  if (r < 0) {
    fprintf(stderr, "Saving data failed in PushData\n");
  } 
  
  return sd_bus_reply_method_return(m, "");
}

static int method_fetch_data(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
  int r;
  uint16_t clipboard;
  uint16_t item_id;
  char *type;
  r = sd_bus_message_read(m, "qqs", &clipboard, &item_id, &type);
  if (r < 0) {
    fprintf(stderr, "Failed to parse clipboard ID, item_id, type in FetchData: %s\n", strerror(-r));
    return r;
  }

  size_t datalen;
  unsigned char *data;
  r = store_fetch_data(clipboard, item_id, type, &datalen, &data);
  if (r<0) {
    fprintf(stderr, "Failed to fetch data from store: clipboard %u, item %u, type %s\n", clipboard, item_id, type);
    datalen = 0;
    data = NULL;
  }

  sd_bus_message *reply;
  r = sd_bus_message_new_method_return(m, &reply);
  if (r < 0) {
    fprintf(stderr, "Unable to make return message\n");
    free(data);
    return -1;
  }
  sd_bus_message_append_array(reply,'y', data, datalen);
  sd_bus* bus = sd_bus_message_get_bus(m);
  r = sd_bus_send(bus, reply, NULL);
  
  if (data) {
    free(data);
  }
  return r;
}

static int method_item_count(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
  int r;
  uint16_t clipboard;
  r = sd_bus_message_read(m, "q", &clipboard);
  if (r<0) {
    sd_bus_reply_method_errorf(m, "Bad Format", "Unable to parse ItemCount call");
    return r;
  }

  uint16_t last_item_id = store_last_item_id(clipboard);
  uint16_t item_count = store_item_count(clipboard);

  return sd_bus_reply_method_return(m, "qq", last_item_id, item_count);
}

static int method_typelist(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
  int r;
  uint16_t clipboard;
  uint16_t item_id;
  r = sd_bus_message_read(m, "qq", &clipboard, &item_id);
  if (r<0) {
    sd_bus_reply_method_errorf(m, "Bad Format", "Unable to parse TypeList call");
    return r;
  }

  char **typelist;
  typelist = store_typelist(clipboard, item_id);
  if (!typelist) {
    sd_bus_reply_method_errorf(m, "Bad Format", "Unable to get type list for clipboard %u, item %u",
			       clipboard, item_id);
    return -1;
  }

  sd_bus_message *reply;
  r = sd_bus_message_new_method_return(m, &reply);
  if (r < 0) {
    fprintf(stderr, "Unable to make return message\n");
    clip_free_typelist(typelist);
    return -1;
  }
  sd_bus_message_append_strv(reply, typelist);
  sd_bus* bus = sd_bus_message_get_bus(m);
  r = sd_bus_send(bus, reply, NULL);
  clip_free_typelist(typelist);
  
  return r;
}

static int method_types_without_data(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
  fprintf(stderr, "*** Unimplemented!Types Without Data called\n");
  return 1;
}

static const sd_bus_vtable clipboard_vtable[] =
  {SD_BUS_VTABLE_START(0),
   SD_BUS_METHOD("CreateItem", "qsas", "qq",
		 method_create_item, SD_BUS_VTABLE_UNPRIVILEGED),
   SD_BUS_METHOD("PushData", "qqsay", "",
		 method_push_data, SD_BUS_VTABLE_UNPRIVILEGED),
   SD_BUS_METHOD("FetchData", "qqs", "ay",
		 method_fetch_data, SD_BUS_VTABLE_UNPRIVILEGED),   
   SD_BUS_METHOD("ItemCount", "q", "qq",
		 method_item_count, SD_BUS_VTABLE_UNPRIVILEGED),
   SD_BUS_METHOD("FetchTypelist", "qq", "as",
		 method_typelist, SD_BUS_VTABLE_UNPRIVILEGED),
   SD_BUS_METHOD("TypesWithoutData", "qq", "as",
		 method_types_without_data, SD_BUS_VTABLE_UNPRIVILEGED),
   SD_BUS_VTABLE_END
};

int main(int argc, char *argv[]) {
  // Initialize store
  store_set_ring_size(CLIPBOARD_GENERAL, 5);
  store_set_ring_size(CLIPBOARD_FIND, 10);
  store_set_ring_size(CLIPBOARD_STYLE, 1);
  store_set_ring_size(CLIPBOARD_DRAG, 3);

  sd_bus_slot *slot = NULL;
  sd_bus *bus = NULL;
  int r;

  // Connect to the user bus
  r = sd_bus_open_user(&bus);
  if (r < 0) {
    fprintf(stderr, "Failed to connect to user bus: %s\n", strerror(-r));
    return EXIT_FAILURE;    
  }

  // Install the vtable
  r = sd_bus_add_object_vtable(bus, &slot, CLIP_PATH, CLIP_INTERFACE, clipboard_vtable,
			       NULL);
  if (r < 0) {
    fprintf(stderr, "Failed to issue method call: %s\n", strerror(-r));
    return EXIT_FAILURE;
  }

  // Advertise!
  r = sd_bus_request_name(bus, CLIP_DESTIN, 0);
  if (r < 0) {
    fprintf(stderr, "Failed to acquire service name: %s\n", strerror(-r));
    return EXIT_FAILURE;
  }

  for (;;) {
    /* Process requests */
    r = sd_bus_process(bus, NULL);
    if (r < 0) {
      fprintf(stderr, "Failed to process bus: %s\n", strerror(-r));
      return EXIT_FAILURE;
    }
    if (r > 0) /* we processed a request, try to process another one, right-away */
      continue;

    // Wait for another message
    r = sd_bus_wait(bus, (uint64_t) -1);
    if (r < 0) {
      fprintf(stderr, "Failed to wait on bus: %s\n", strerror(-r));
      return EXIT_FAILURE;
    }
  }
  
  // Should never get here
  return EXIT_SUCCESS;
}
