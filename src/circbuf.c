#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "circbuf.h"


/* Circular buffer object */
struct _circular_buffer {
	int maxsize;   /* maximum number of elements */
	int start;  /* index of oldest element */
	int end;    /* index at which to write new element */
	int read_count;
	int write_count;

	ElemType   *elems;  /* vector of elements */
};

circular_buffer *cb_new (int maxsize) {
	circular_buffer *cb;

	cb = (circular_buffer *) malloc (sizeof (circular_buffer));
	cb -> maxsize = maxsize;
	cb -> start = 0;
	cb -> end = 0;
	cb -> read_count = 0;
	cb -> write_count = 0;
	cb -> elems = malloc (cb -> maxsize * sizeof (ElemType));

	return cb;
}
 
void cb_destroy (circular_buffer *cb) {
	if (cb) {
		free (cb -> elems);
		free (cb);
	}
}
 
/* Write an element, overwriting oldest element if buffer is full. App can
   choose to avoid the overwrite by checking cbIsFull(). */
void cb_write (circular_buffer *cb, ElemType *elem) {
	cb -> elems[cb -> end] = *elem;
	cb -> end = (cb -> end + 1) % cb -> maxsize;
//	if (cb -> end == cb -> start)
//		cb -> start = (cb -> start + 1) % cb -> maxsize; /* full, overwrite */
	++(cb -> write_count);
}

#define min(x, y) (((x) < (y)) ? (x) : (y))

/* Write a block of elements, never overwriting. */
int cb_write_block (circular_buffer *cb, ElemType *elem, size_t n) {
	if (n <= cb_free (cb)) {
		int right;

		right = min (n, cb -> maxsize - cb -> end);

		memcpy (cb -> elems + cb -> end, elem, right * sizeof (ElemType));

		if (right < n) {
			memcpy (cb -> elems, elem + right, (n - right) * sizeof (ElemType));
		}

		cb -> end = (cb -> end + n) % cb -> maxsize;
		cb -> write_count += n;

		return (1);
	} else {
		return (0);
	}
}

/* Read oldest element. App must ensure !cb_empty() first. */
ElemType *cb_read (circular_buffer *cb, ElemType *elem) {
	*elem = cb -> elems[cb -> start];
	cb -> start = (cb -> start + 1) % cb -> maxsize;
	++(cb -> read_count);

	return elem;
}

int cb_read_block (circular_buffer *cb, ElemType *elem, size_t n) {
	if (n <= cb_used (cb)) {
		int right;

		right = min (n, cb -> maxsize - cb -> start);

		memcpy (elem, cb -> elems + cb -> start, right * sizeof (ElemType));

		if (right < n) {
			memcpy (elem + right, cb -> elems, (n - right) * sizeof (ElemType));
		}

		cb -> start = (cb -> start + n) % cb -> maxsize;
		cb -> read_count += n;

		return (1);
	} else {
		return (0);
	}
}

/* Return the number of used elements */
size_t cb_used (circular_buffer *cb) {
	/* The unsigned difference WC - RC always yields the number of items placed
	 * in the buffer and not yet retrieved (even when counters overlap.
	 */
	return (cb -> write_count - cb -> read_count);
}

/* Return the number of available elements */
size_t cb_free (circular_buffer *cb) {
	return (cb -> maxsize - (cb -> write_count - cb -> read_count));
}

