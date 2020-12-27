#include <vector>
#include <string>
#include <map>
#include <deque>
#include <cstring>
#include "clip_common.h"
#include <stdio.h>

using namespace std;

class Buffer {
public:
  size_t length;
  unsigned char *data;
  bool owns_data;
  Buffer() {
    data = NULL;
    length = 0;
    owns_data = false;
  }
  Buffer(size_t len, const unsigned char *buf, bool owns) {
    length = len;
    // Playing fast and lose with const here because it is about
    // to be copied when I put it into the map.
    data = (unsigned char *)buf;
    owns_data = owns;  
  }
  Buffer(const Buffer &b) {
    length = b.length;
    data = (unsigned char *)malloc(b.length);
    memcpy(data, b.data, b.length);
    owns_data = true;
  }
  
  ~Buffer() {
    if (owns_data) {
      free(data);
    }
  }
};
  

class ClipItem {
public:
  string label;
  string sender;
  vector<string> declared_types;
  map<string, Buffer> data_cache;
};

class Clipboard {
public:
  uint16_t front_item_id;
  uint16_t ring_size;
  deque<ClipItem> ring;
};

Clipboard store[CLIPBOARD_COUNT];

// Returns -1 if no such item exists
int ring_index(uint16_t clipboard_id, uint16_t item_id){
  
  if (clipboard_id >= CLIPBOARD_COUNT) {
    fprintf(stderr, "Asked for clipboard %u\n", clipboard_id);
    return -1;
  }
  uint16_t front_index = store[clipboard_id].front_item_id;
  if (front_index == 0) {
    return -1;
  }

  // Zero means "What ever was added last"
  if (item_id == 0) {
    // Return the first index
    return 0;
  }

  int index = (int)front_index - (int)item_id;
  // 1 is equivalent to INT16_MAX + 1
  if (index < 0) {
    index = index + INT16_MAX;
  }
  if (index < 0 || index >= store[clipboard_id].ring.size()) {
    return -1;
  }
  return index;
}

int item_id_at_index(uint16_t clipboard_id, size_t idx) {
  uint16_t front_id = store[clipboard_id].front_item_id;
  int result = front_id - idx;
  if (result < 1) {
    result = result + INT16_MAX;
  }
  return result;
}

void store_set_ring_size(uint16_t clipboard_id, uint16_t max_items)
{
  if (clipboard_id >= CLIPBOARD_COUNT) {
    fprintf(stderr, "Asked set ring size for clipboard %u\n", clipboard_id);
    return;
  }

  store[clipboard_id].ring_size = max_items;
}
uint16_t store_last_item_id(uint16_t clipboard_id)
{
  if (clipboard_id >= CLIPBOARD_COUNT) {
    fprintf(stderr, "Asked for last item ID for clipboard %u\n", clipboard_id);
    return 0;
  }
  return store[clipboard_id].front_item_id;
}
uint16_t store_item_count(uint16_t clipboard_id)
{
  return store[clipboard_id].ring.size();
}

uint16_t store_create_item(uint16_t clipboard_id, const char *label, const char *sender,
			   char **typelist, uint16_t *pushed_out_ptr, char **pushed_sender_ptr)
{
  if (clipboard_id >= CLIPBOARD_COUNT) {
    fprintf(stderr, "Asked to create item on clipboard %u\n", clipboard_id);
    return 0;
  }
  uint16_t new_item_id = store[clipboard_id].front_item_id + 1;
  if (new_item_id == INT16_MAX + 1) {
    new_item_id = 1;
  }
  ClipItem new_item;
  new_item.label = label;
  new_item.sender = sender;
  for (int i = 0; typelist[i] != NULL; i++) {
    new_item.declared_types.push_back(typelist[i]);
  }
  
  store[clipboard_id].ring.push_front(new_item);
  store[clipboard_id].front_item_id = new_item_id;
  
  uint16_t ring_size = store[clipboard_id].ring_size;
  uint16_t pushed_out_item_id = 0;
  char *pushed_sender = NULL;
  if (store[clipboard_id].ring.size() > ring_size) {
    pushed_out_item_id = item_id_at_index(clipboard_id, ring_size);
    pushed_sender = strdup(store[clipboard_id].ring.back().sender.c_str());
    store[clipboard_id].ring.pop_back();
  }

  // Tell the caller what got pushed out
  if (pushed_out_ptr) {
    *pushed_out_ptr = pushed_out_item_id;
  }
  if (pushed_sender_ptr) {
    *pushed_sender_ptr = pushed_sender;
  } else {
    free(pushed_sender);
  }
  return new_item_id;
}
const char *store_sender_for_item(uint16_t clipboard_id, uint16_t item_id){
  int index = ring_index(clipboard_id, item_id);
  if (index < 0) {
    fprintf(stderr, "No such item: %u, %u\n", clipboard_id, item_id);
    return NULL;
  }
  return store[clipboard_id].ring[index].sender.c_str();
}

const char *store_label_for_item(uint16_t clipboard_id, uint16_t item_id){
    int index = ring_index(clipboard_id, item_id);
  if (index < 0) {
    fprintf(stderr, "No such item: %u, %u\n", clipboard_id, item_id);
    return NULL;
  }
  return store[clipboard_id].ring[index].label.c_str();
}

int store_store_data(uint16_t clipboard_id, uint16_t item_id, const char *type, size_t datalen, const unsigned char *data)
{
  string key = type;
  int index = ring_index(clipboard_id, item_id);
  if (index < 0) {
    return 0;
  }

  Buffer buf(datalen, data, false);
  store[clipboard_id].ring[index].data_cache.insert(std::make_pair(key, buf));
  return 1;
}

int store_fetch_data(uint16_t clipboard_id, uint16_t item_id, char *type, size_t *datalenptr, unsigned char **dataptr)
{
  string key = type;
  int index = ring_index(clipboard_id, item_id);
  if (index < 0) {
    return -1;
  }

  // Is it missing?
  if(store[clipboard_id].ring[index].data_cache.count(key) < 1) {
    return -1;
  }
  
  Buffer b = store[clipboard_id].ring[index].data_cache[key];
  if (dataptr) {
    *dataptr = (unsigned char *)malloc(b.length);
    memcpy(*dataptr, b.data, b.length);
  }

  if (datalenptr) {
    *datalenptr = b.length;
  }
  return 1;  
}
char **store_typelist(uint16_t clipboard_id, uint16_t item_id)
{
  int index = ring_index(clipboard_id, item_id);
  if (index < 0) {
    return NULL;
  }

  vector<string>types = store[clipboard_id].ring[index].declared_types;
  int type_count = types.size();
  char **result = (char **)malloc(sizeof(char *) * (type_count + 1));
  for (int i = 0; i < types.size(); i++) {
    result[i] = strdup(types[i].c_str());
  }
  result[type_count] = NULL;
  return result;
}

char **store_types_without_data(uint16_t clipboard_id, uint16_t item_id)
{
  int index = ring_index(clipboard_id, item_id);
  if (index < 0) {
    return NULL;
  }

  vector<string> types = store[clipboard_id].ring[index].declared_types;
  
  int type_count = types.size();
  vector<string> dataless_types;

  for (int i = 0; i < type_count; i++) {
    string key = types[i];
    if (store[clipboard_id].ring[index].data_cache.count(key) == 0) {
      dataless_types.push_back(key);
    }
  }

  int result_count = dataless_types.size(); 
  char **result = (char **)malloc(sizeof(char *) * (result_count + 1));
  for (int i = 0; i < result_count; i++) {
    const char *current_type = dataless_types[i].c_str(); 
    result[i] = strdup(current_type);
  }
  result[result_count] = NULL;
  return result;
}  
