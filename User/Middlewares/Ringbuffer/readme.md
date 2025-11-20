# 1. 特性
环形缓冲buffer在只有一个reader和一个writer的情况下可以无锁；
使用锁时请确保开启了继承优先级；
实际可用大小为size-1
# 2. 举例
## 2.1 基于stm32Hal库的Uart驱动设计
hal库在接收和发送函数时，代入的data指针变量不可立即变化，这就导致实际外设使用还是会阻塞。  
加入环形缓冲后，可实现真正的非阻塞；  
## 2.2 使用示例

下面给出两个常见使用场景的示例：无锁（Single-Producer Single-Consumer，SPSC）和有锁（在多线程或中断/线程混合访问场景）。

2.2.1 无锁（SPSC，单读者单写者）

在只有一个生产者和一个消费者、且运行在同一核的场景中，可以不使用锁（性能最好）。示例：

```c
/* 定义 backing buffer 与 ring 实例 */
static uint8_t rb_buf[256];
ringBuffer_t rb;
/* 初始化 */
ringBuffer_init(&rb, rb_buf, sizeof(rb_buf));
rb.write(&rb, data, len);
rb.read(&rb, out, sizeof(out));
```

注意：
- 如果读/写分别在主任务与中断中进行（ISR 与线程混合），建议使用有锁模式或用临界区保护（见下）。
- 即使是 SPSC，在某些编译器或体系结构下也可能需要 `volatile` 或内存屏障以保证可见性；在 Cortex-M 上常见 SPSC 做法在无跨核的情况下一般可行。

2.2.2 有锁（线程/中断混合或多读写者）

在多任务或中断与线程混合访问时，应注册锁函数以确保线程安全。下面给出两种常见实现：使用全局临界区（禁中断）或使用 RTOS 互斥量。

（A）使用临界区（适用于短临界段、ISR/线程混合访问）

```c
static inline void rb_lock_enter(void)  { __disable_irq(); }
static inline void rb_lock_exit(void)   { __enable_irq(); }

/* 注册读写锁（此示例使用相同的临界区函数） */
ringBuffer_registerLocks(&rb, rb_lock_enter, rb_lock_exit, rb_lock_enter, rb_lock_exit);

/* 写/读接口与无锁示例相同 */
rb.write(&rb, data, len);
rb.read(&rb, out, n);
```

注意：对中断禁用是粗粒度且会影响延迟，适用于短操作且对实时性要求不高的场景。

（B）使用 RTOS 互斥量（适用于线程间同步）

```c
/* 假设有平台 mutex 接口：my_mutex_lock()/my_mutex_unlock() */
void my_rb_lock(void)   { my_mutex_lock(&rb_mutex); }
void my_rb_unlock(void) { my_mutex_unlock(&rb_mutex); }

ringBuffer_registerLocks(&rb, my_rb_lock, my_rb_unlock, my_rb_lock, my_rb_unlock);

/* 在多线程中安全地读写 */
rb.write(&rb, data, len);
rb.read(&rb, out, n);
```

注意：
- 如果使用 RTOS 互斥量，确保在 `ringBuffer_deinit` 前正确释放/删除互斥量；`ringBuffer_deinit` 会尝试在清理前获取已注册的锁以保证安全反初始化（如果锁函数会阻塞，请注意可能的等待）。
- `ringBuffer_registerLocks` 会在任意非 NULL 回调时把 `use_lock` 置为 1。

# 3. 实现方案
1.定义环形缓冲区结构体，包含成员：buffer大小，数据头，数据尾，取锁函数（读和写），释放锁函数（读和写），取数据函数，写数据函数
2.锁是永久等待
3.是否使用锁可选
# 4. 缺点
缓冲区大小固定（没有使用内存池）