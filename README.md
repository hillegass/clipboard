# clipboard
A clipboard server and client library that communicate over dbus

Over the years, copy-and-paste has been implemented in a dozen different ways. It has
always been my hope that the open source community would look at what
had been done and take the best of each:

- Apple has a good API with support for multiple clipboards and
  multiple datatypes for each clipboard item.  They also created a
  stand-alone clipboard server, separate from the window server.

- emacs has the kill buffer, which lets you keep multiple items on the
  clipboard at any time.

- Linux has D-bus, which is a very nice interprocess communication mechanism.

- It should be easy to listen for changes on the clipboard.

I have watched the desktop Linux community do half-assed fixes to the
copy-and-paste mechanism for decades.  Finally, I thought "I should
sit down and implement the clipboard server and the client library
that I think the world needs."

And here it is. (Note, there is no GUI and no applications or GUI
toolkits support this. It is just a server and client library.)

You compile the server (clipd) using make. It is written in C++ but
only relies upon the standard templates library and libsystemd (for
the sbus functions).

The server has multiple pasteboards:

- CLIPBOARD_GENERAL is for copy/paste.

- CLIPBOARD_FIND is for prepopulating the search field with whatever the user last searched for.

- CLIPBOARD_STYLE is for copying the font and other style annotations for rich text.

- CLIPBOARD_DRAG is for drag and drop.

Each clipboard has a kill ring.  For example, in this version of clipd, the
general clipboard holds 5 items. If you copy five times, all five will live on
the clipboard.  When you copy a sixth, the item that was created first will be
deleted from the clipboard.

The client library is in C and depends only on libsystemd (for the
sbus functions). It is declared in clip_common.h and clipboard.h. It
is implemented in clipboard.c.

## Data providers

When you do a copy, you can specify multiple datatypes.  For example,
in word processor, you might copy plain text and rich text on to the
clipboard. In a drawing program, you might copy a PNG and a custom
data type to the clipboard. The different datatypes are named using
strings, and I hope people use Uniform Type Identifiers like
"public.png".

Each item that goes on the pasteboard gets a label for the benefit of
tools that allow the user to browse the clipboard.

Here is what putting data on a clipboard looks like:

```
  // Create a type list
  char **typelist = clip_create_typelist(2, "public.utf8-plain-text", "public.rtf");

  // Create a clipboard item with "Frivolous Text" label
  uint16_t item_id = clip_create_item(CLIPBOARD_GENERAL, "Frivolous Text", typelist);
  clip_free_typelist(typelist);

  // Push data for plain text
  char *plain_text = "This is some text that you might want to copy";
  clip_push_data(CLIPBOARD_GENERAL, item_id, "public.utf8-plain-text", strlen(plain_text), plain_text);

  // Push data for RTF
  char *rtf_text = "{\\rtf1\\ansi{\\fonttbl\\f0\\fswiss Helvetica;}\\f0\\pard\nThis is some {\\b bold} text that you might want to copy\\par\n}";
  clip_push_data(CLIPBOARD_GENERAL, item_id, "public.rtf", strlen(rtf_text), rtf_text);
```

(I will support lazy data providers: If you have created an item and
promised a data type, when clipd is asked for the data that you have
not provided, you will get asked for it. This is not implemented yet.)

## Data consumers

You can ask clipd for the ID of the last item added to the clipboard
and the number of items on that clipboard.  (Item IDs are sequential
on a given clipboard. If the last one added is 9 and the clipboard
kill ring holds 5 items. The IDs of the items are 9, 8, 7, 6, and 5.)

Then you can ask for the datatypes for a particular item.  And then
you can ask for your favorite datatype.

```
  uint16_t last_item_id, item_count;
  clip_item_count(CLIPBOARD_GENERAL, &last_item_id, &item_count);
  
  if (item_count == 0) {
    fprintf(stderr, "No data on the clipboard\n");
    return;
  }

  // What datatypes are available to me?
  char **typelist;
  clip_item_typelist(CLIPBOARD_GENERAL, last_item_id, &typelist);

  // For fun, let's display them
  char **current_type = typelist;
  fprintf(stderr, "Available types for %u:\n", last_item_id);
  while (*current_type) {
    fprintf(stderr, "\t%s\n", *current_type);
    current_type++;
  }
  clip_free_typelist(typelist);
  typelist = NULL;

  // Fetch the plain text data
  size_t datalen;
  unsigned char *data;
  clip_item_data_for_type(CLIPBOARD_GENERAL, last_item_id, "public.utf8-plain-text", &datalen, &data);
  char *str = clip_string_from_data(data, datalen);
  free(data);
  fprintf(stderr, "Fetched %lu bytes:\"%s\"\n", datalen, str);
  free(str);
```

## Listeners

Once this clipboard is in use, users will want tools to monitor and
manipulate the data on clipboard. Notifications of changes are sent
using D-bus's signals API.

Here's how it works. You create a function that you want called
whenever a new item is added to the clipboard:

```
void on_change(uint16_t board, uint16_t new_item_id, char *label, size_t item_count)
{
  fprintf(stderr, "Clipboard %u: New item %u \"%s\" added, total items: %lu\n",
          board, new_item_id, label, item_count);
}
```

Then, you register that:
```
  clip_set_change_handler(CLIPBOARD_GENERAL, on_change);
```

Now, the checking for those signals needs to be in the event loop somehow. For now, for my command-line tools, I just have a loop:
```
  for (;;) {
    process_waiting_clipboard_events();
    wait_for_clipboard_events();
  }
```
This will involve the event loop of the application that is waiting for these notifications.