#include <stdio.h>
#include <stdint.h>

typedef uint8_t ElemType;

typedef struct _circular_buffer circular_buffer;

circular_buffer *cb_new (int size);

void cb_destroy (circular_buffer *cb);
 
/* Write an element, overwriting oldest element if buffer is full. App can
   choose to avoid the overwrite by checking cb_full(). */
void cb_write (circular_buffer *cb, ElemType *elem);

int cb_write_block (circular_buffer *cb, ElemType *elem, size_t n);

/* Read oldest element. App must ensure !cb_empty() first. */
ElemType *cb_read (circular_buffer *cb, ElemType *elem);

int cb_read_block (circular_buffer *cb, ElemType *elem, size_t n);

/* Return the number of used elements */
size_t cb_used (circular_buffer *cb);

/* Return the number of available elements */
size_t cb_free (circular_buffer *cb);

#define cb_full(cb) (cb_free (cb) == 0)

#define cb_empty(cb) (cb_used (cb) == 0)

