/* Ringbuffer.c
 * Implementation for the ring buffer defined in Ringbuffer.h
 */

#include "../../Ringbuffer/Inc/fy_ringBuffer.h"
#include <string.h>

void ringBuffer_clear(ringBuffer_t *rb);

static size_t rb_used(const ringBuffer_t *rb)
{
	if (rb->head >= rb->tail) {
		return rb->head - rb->tail;
	}
	return rb->size - (rb->tail - rb->head);
}

static uint8_t *rb_head(ringBuffer_t *rb)
{
    return &rb->buffer[rb->head];
}

static uint8_t *rb_tail(ringBuffer_t *rb)
{
    return &rb->buffer[rb->tail];
}

/* Default no-op lock (file-local) */
static void RB_NoLock(void) { (void)0; }

void ringBuffer_init(ringBuffer_t *rb, uint8_t *buffer, size_t size)
{
	if (rb == NULL || buffer == NULL || size == 0) return;
	rb->buffer = buffer;
	rb->size = size;
	rb->head = 0;
	rb->tail = 0;
	rb->lock_read = RB_NoLock;
	rb->unlock_read = RB_NoLock;
	rb->lock_write = RB_NoLock;
	rb->unlock_write = RB_NoLock;
	rb->read = RB_Read;
	rb->write = RB_Write;
	rb->init = ringBuffer_init;
	rb->clear = ringBuffer_clear;
	rb->use_lock = 0;
	rb->used = rb_used;
	rb->rb_head = rb_head;
	rb->rb_tail = rb_tail;
}

void ringBuffer_clear(ringBuffer_t *rb)
{
	if (rb == NULL) return;
	/* If locking is enabled, take both write and read locks to safely reset indices. */
	if (rb->use_lock) {
		rb->lock_write();
		rb->lock_read();
	}

	rb->head = 0;
	rb->tail = 0;

	if (rb->use_lock) {
		rb->unlock_read();
		rb->unlock_write();
	}
}

void ringBuffer_registerLocks(ringBuffer_t *rb, rb_lock_fn_t l_r, rb_unlock_fn_t u_r, rb_lock_fn_t l_w, rb_unlock_fn_t u_w)
{
	if (rb == NULL) return;
	rb->lock_read = (l_r != NULL) ? l_r : RB_NoLock;
	rb->unlock_read = (u_r != NULL) ? u_r : RB_NoLock;
	rb->lock_write = (l_w != NULL) ? l_w : RB_NoLock;
	rb->unlock_write = (u_w != NULL) ? u_w : RB_NoLock;
	rb->use_lock = (l_r || u_r || l_w || u_w) ? 1 : 0;
}

size_t RB_Write(ringBuffer_t *rb, const uint8_t *src, size_t len)
{
	if (rb == NULL || rb->buffer == NULL || len == 0) return 0;
	rb->lock_write();

	size_t used = rb_used(rb);
	/* keep one slot free to distinguish full vs empty */
	size_t free_space = (rb->size > 0) ? (rb->size - used - 1) : 0;
	size_t to_write = (len <= free_space) ? len : free_space;
	if (to_write == 0) {
		rb->unlock_write();
		return 0;
	}

	/* first chunk until end of buffer */
	size_t first = rb->size - rb->head;
	if (first > to_write) first = to_write;
	if (src != NULL)
	{
		memcpy(&rb->buffer[rb->head], src, first);
	}

	/* second wrapped chunk */
	size_t second = to_write - first;
	if (second > 0) {
		if (src != NULL)
		{
			memcpy(&rb->buffer[0], src + first, second);
		}
	}

	rb->head = (rb->head + to_write) % rb->size;
	rb->unlock_write();
	return to_write;
}

size_t RB_Read(ringBuffer_t *rb, uint8_t *dst, size_t len)
{
	if (rb == NULL || rb->buffer == NULL || len == 0) return 0;
	rb->lock_read();

	size_t used = rb_used(rb);
	size_t to_read = (len <= used) ? len : used;
	if (to_read == 0) {
		rb->unlock_read();
		return 0;
	}

	size_t first = rb->size - rb->tail;
	if (first > to_read) first = to_read;
	if (dst != NULL)
	{
		memcpy(dst, &rb->buffer[rb->tail], first);
	}

	size_t second = to_read - first;
	if (second > 0) {
		if (dst != NULL)
		{
			memcpy(dst + first, &rb->buffer[0], second);
		}
	}

	rb->tail = (rb->tail + to_read) % rb->size;
	rb->unlock_read();
	return to_read;
}

