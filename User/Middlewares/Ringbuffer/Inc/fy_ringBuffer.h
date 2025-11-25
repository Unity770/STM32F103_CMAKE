/* Ringbuffer.h
 * Simple thread-safe (lock callback) ring buffer implementation.
 * Usage:
 *  - Define a buffer and ring object with the macro:
 *      RB_DEFINE_STATIC(myrb, 256);
 *  - Optionally register lock/unlock callbacks with `register_locks` member.
 *  - Use `myrb.write(&myrb, data, len)` and `myrb.read(&myrb, out, len)`.
 */
#ifndef __FY_RINGBUFFER_H
#define __FY_RINGBUFFER_H

#include <stdint.h>
#include <stddef.h>

typedef struct ringBuffer ringBuffer_t;

typedef void (*rb_lock_fn_t)(void);
typedef void (*rb_unlock_fn_t)(void);
typedef size_t (*rb_read_fn_t)(ringBuffer_t *rb, uint8_t *dst, size_t len);
typedef size_t (*rb_write_fn_t)(ringBuffer_t *rb, const uint8_t *src, size_t len);

struct ringBuffer {
    uint8_t *buffer;        /* pointer to storage */
    size_t size;            /* total buffer size */
    size_t head;            /* write index */
    size_t tail;            /* read index */
    uint8_t use_lock;       /* 是否使用锁（0: 不使用, 1: 使用） */

    /* lock callbacks (can be NULL, in which case no locking performed) */
    rb_lock_fn_t lock_read;
    rb_unlock_fn_t unlock_read;
    rb_lock_fn_t lock_write;
    rb_unlock_fn_t unlock_write;

    /* operations (set to RB_Read / RB_Write by default) */
    rb_read_fn_t read;//支持空指针，空指针只刷新buffr位置
    rb_write_fn_t write;//支持空指针，空指针只刷新buffr位置

    /* lifecycle helpers */
    void (*init)(ringBuffer_t *rb, uint8_t *buffer, size_t size);
    void (*clear)(ringBuffer_t *rb);
    uint16_t (*used)(const ringBuffer_t *rb);
    uint8_t * (*rb_head)(ringBuffer_t *rb);
    uint8_t * (*rb_tail)(ringBuffer_t *rb);
};

/* Public API - implementations in Ringbuffer.c */
void ringBuffer_init(ringBuffer_t *rb, uint8_t *buffer, size_t size);
void ringBuffer_clear(ringBuffer_t *rb);
void ringBuffer_registerLocks(ringBuffer_t *rb, rb_lock_fn_t l_r, rb_unlock_fn_t u_r, rb_lock_fn_t l_w, rb_unlock_fn_t u_w);

/* Note: RB_NoLock is internal (not exposed here). */

/* Helper usage:
 * - 用户手动定义 backing buffer 和 ringBuffer_t 变量，然后调用 `ringBuffer_init`:
 *     static uint8_t buf[256];
 *     ringBuffer_t rb;
 *     ringBuffer_init(&rb, buf, sizeof(buf));
 * - 可选注册锁函数：`ringBuffer_registerLocks(&rb, lock_r, unlock_r, lock_w, unlock_w);`
 */

#endif /* __RINGBUFFER_H */
