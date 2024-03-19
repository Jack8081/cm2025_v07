#ifndef _ROM_INTERFACE_H_
#define _ROM_INTERFACE_H_
#include <stddef.h>
#include <stdarg.h>
#include <thread_timer.h>

#define  ROM_BASE_ADDR 0x4400
#define  ROM_IPC_API_ADDR		    (ROM_BASE_ADDR + 0x10)
#define  ROM_IRQ_API_ADDR		    (ROM_BASE_ADDR + 0x6c)
#define  ROM_RINGBUF_API_ADDR		(ROM_BASE_ADDR + 0x8c)
#define  ROM_SCHED_API_ADDR		    (ROM_BASE_ADDR + 0xd8)
#define  ROM_THREAD_API_ADDR		(ROM_BASE_ADDR + 0x184)
#define  ROM_TIMEOUT_API_ADDR		(ROM_BASE_ADDR + 0x1bc)
#define  ROM_WORK_API_ADDR		(ROM_BASE_ADDR + 0x220)
#define  ROM_RBUF_API_ADD		(ROM_BASE_ADDR + 0x2c4)

#define __romfunc  

typedef struct
{
	void * p__kernel;
	struct _isr_table_entry * p_sw_isr_table;
	uint64_t curr_tick;
	int announce_remaining;
	void * p_timeout_list;
	void * p_timeout_lock;
	void * p_timer_ctx;
	void * p_async_msg_free;
	void * p_sched_spinlock;
	void * p_sem_spinlock;
    void * p_mutex_spinlock;
	void * p_work_spinlock;
	void * p_pending_cancels;
	void * p_threads_runtime_stats;
	void * p_k_sys_work_q;
	void * p_idle_thread;
	struct k_thread *p_pending_current;
	int *p_slice_time;
	int *p_slice_max_prio;
	void (*p_pm_system_resume)(void);
	enum pm_state (*p_pm_system_suspend)(int32_t ticks);
	void (*p_assert_post_action)(const char *file, unsigned int line);
	void (*p_vprintk)(const char *fmt, va_list ap);
}import_symbols_t;

extern import_symbols_t import_symbos;

typedef void (*z_arm_int_exit_t)(void);
typedef void (*_isr_wrapper_t)(void);
typedef void (*arch_cpu_atomic_idle_t)(unsigned int key);
typedef void (*arch_cpu_idle_t)(void);
typedef void (*z_arm_pendsv_t)(void);
typedef void (*idle_t)(void *unused1, void *unused2, void *unused3);
typedef void (*z_pm_save_idle_exit_t)(int32_t ticks);
typedef void (*isr_wrapper_with_profiler_t)(u32_t irqnum);

typedef struct {

    idle_t  p_idle;
    z_pm_save_idle_exit_t  p_z_pm_save_idle_exit;

    z_arm_int_exit_t  p_z_arm_int_exit;
    _isr_wrapper_t  p__isr_wrapper;
    arch_cpu_atomic_idle_t  p_arch_cpu_atomic_idle;
    arch_cpu_idle_t  p_arch_cpu_idle;
	z_arm_pendsv_t  p_z_arm_pendsv;
    isr_wrapper_with_profiler_t  p_isr_wrapper_with_profiler;

}export_irq_symbols_t;

#define  p_rom_irq_api   ((export_irq_symbols_t *)ROM_IRQ_API_ADDR)

typedef int (*mbox_message_data_check_t)(struct k_mbox_msg *rx_msg, void *buffer);
typedef void (*mbox_message_dispose_t)(struct k_mbox_msg *rx_msg);
typedef int (*mbox_message_match_t)(struct k_mbox_msg *tx_msg,
			       struct k_mbox_msg *rx_msg);
typedef int (*mbox_message_put_t)(struct k_mbox *mbox, struct k_mbox_msg *tx_msg,
			     k_timeout_t timeout);
typedef void (*k_mbox_async_put_t)(struct k_mbox *mbox, struct k_mbox_msg *tx_msg,
		      struct k_sem *sem);
typedef void (*k_mbox_clear_msg_t)(struct k_mbox *mbox);
typedef void (*k_mbox_data_get_t)(struct k_mbox_msg *rx_msg, void *buffer);
typedef int (*k_mbox_get_t)(struct k_mbox *mbox, struct k_mbox_msg *rx_msg, void *buffer,
	       k_timeout_t timeout);
typedef int (*k_mbox_get_pending_msg_cnt_t)(struct k_mbox *mbox,k_tid_t target_thread);
typedef void (*k_mbox_init_t)(struct k_mbox *mbox);
typedef int (*k_mbox_put_t)(struct k_mbox *mbox, struct k_mbox_msg *tx_msg,
	       k_timeout_t timeout);
typedef void (*z_handle_obj_poll_events_t)(sys_dlist_t *events, uint32_t state);

typedef void *(*alloc_t)(size_t size);
typedef int32_t (*queue_insert_t)(struct k_queue *queue, void *prev, void *data,
			    alloc_t alloc, bool is_append);
typedef void (*free_t)(void *ptr);
typedef void *(*z_queue_node_peek_t)(sys_sfnode_t *node, free_t free);
typedef void (*z_impl_k_sem_give_t)(struct k_sem *sem);
typedef int (*z_impl_k_sem_init_t)(struct k_sem *sem, unsigned int initial_count,
		      unsigned int limit);
typedef void (*z_impl_k_sem_reset_t)(struct k_sem *sem);
typedef int (*z_impl_k_sem_take_t)(struct k_sem *sem, k_timeout_t timeout);
typedef int (*z_impl_k_stack_pop_t)(struct k_stack *stack, stack_data_t *data,
		       k_timeout_t timeout);
typedef int (*z_impl_k_stack_push_t)(struct k_stack *stack, stack_data_t data);
typedef int (*z_impl_k_mutex_init_t)(struct k_mutex *mutex);
typedef int (*z_impl_k_mutex_lock_t)(struct k_mutex *mutex, k_timeout_t timeout);
typedef int (*z_impl_k_mutex_unlock_t)(struct k_mutex *mutex);

typedef struct {

    k_mbox_get_pending_msg_cnt_t  p_k_mbox_get_pending_msg_cnt;
    mbox_message_match_t  p_mbox_message_match;
    mbox_message_data_check_t  p_mbox_message_data_check;
    mbox_message_dispose_t  p_mbox_message_dispose;
    k_mbox_async_put_t  p_k_mbox_async_put;
    k_mbox_init_t  p_k_mbox_init;
    k_mbox_clear_msg_t  p_k_mbox_clear_msg;
    k_mbox_get_t  p_k_mbox_get;
    mbox_message_put_t  p_mbox_message_put;
    k_mbox_put_t  p_k_mbox_put;
    k_mbox_data_get_t  p_k_mbox_data_get;


    z_handle_obj_poll_events_t  p_z_handle_obj_poll_events;


    queue_insert_t  p_queue_insert;
    z_queue_node_peek_t  p_z_queue_node_peek;

    z_impl_k_sem_init_t  p_z_impl_k_sem_init;
    z_impl_k_sem_take_t  p_z_impl_k_sem_take;
    z_impl_k_sem_reset_t  p_z_impl_k_sem_reset;
    z_impl_k_sem_give_t  p_z_impl_k_sem_give;


    z_impl_k_stack_pop_t  p_z_impl_k_stack_pop;
    z_impl_k_stack_push_t  p_z_impl_k_stack_push;
	
    z_impl_k_mutex_init_t  p_z_impl_k_mutex_init;
    z_impl_k_mutex_lock_t  p_z_impl_k_mutex_lock;
    z_impl_k_mutex_unlock_t  p_z_impl_k_mutex_unlock;

}export_ipc_symbols_t;

#define  p_rom_ipc_api   ((export_ipc_symbols_t *)ROM_IPC_API_ADDR)


typedef void (*add_thread_timeout_t)(struct k_thread *thread, k_timeout_t timeout);
typedef void (*add_to_waitq_locked_t)(struct k_thread *thread, _wait_q_t *wait_q);
typedef void (*move_thread_to_end_of_prio_q_t)(struct k_thread *thread);
typedef void (*pend_t)(struct k_thread *thread, _wait_q_t *wait_q,
		 k_timeout_t timeout);
typedef void (*ready_thread_t)(struct k_thread *thread);
typedef void (*unpend_thread_no_timeout_t)(struct k_thread *thread);
typedef void (*unready_thread_t)(struct k_thread *thread);
typedef void (*update_cache_t)(int preempt_ok);
typedef int32_t (*z_tick_sleep_t)(k_ticks_t ticks);
typedef void (*k_sched_lock_t)(void);
typedef void (*k_sched_unlock_t)(void);
typedef int (*z_impl_k_is_preempt_thread_t)(void);
typedef int32_t (*z_impl_k_sleep_t)(k_timeout_t timeout);
typedef int (*z_impl_k_thread_join_t)(struct k_thread *thread, k_timeout_t timeout);
typedef void (*z_impl_k_thread_priority_set_t)(k_tid_t thread, int prio);
typedef void (*z_impl_k_thread_suspend_t)(struct k_thread *thread);
typedef void (*z_impl_k_yield_t)(void);
typedef k_tid_t (*z_impl_z_current_get_t)(void);
typedef int (*z_pend_curr_t)(struct k_spinlock *lock, k_spinlock_key_t key,
	       _wait_q_t *wait_q, k_timeout_t timeout);
typedef void (*z_pend_thread_t)(struct k_thread *thread, _wait_q_t *wait_q,
		   k_timeout_t timeout);
typedef void (*z_priq_dumb_add_t)(sys_dlist_t *pq, struct k_thread *thread);
typedef struct k_thread *(*z_priq_dumb_best_t)(sys_dlist_t *pq);
typedef void (*z_priq_dumb_remove_t)(sys_dlist_t *pq, struct k_thread *thread);
typedef void (*z_ready_thread_t)(struct k_thread *thread);
typedef void (*z_reschedule_t)(struct k_spinlock *lock, k_spinlock_key_t key);
typedef void (*z_reschedule_irqlock_t)(uint32_t key);
typedef void (*z_reset_time_slice_t)(void);
typedef int32_t (*z_sched_prio_cmp_t)(struct k_thread *thread_1,
	struct k_thread *thread_2);
typedef void (*z_sched_start_t)(struct k_thread *thread);
typedef int (*z_sched_wait_t)(struct k_spinlock *lock, k_spinlock_key_t key,
		 _wait_q_t *wait_q, k_timeout_t timeout, void **data);
typedef bool (*z_sched_wake_t)(_wait_q_t *wait_q, int swap_retval, void *swap_data);
typedef bool (*z_set_prio_t)(struct k_thread *thread, int prio);
typedef void (*z_thread_priority_set_t)(struct k_thread *thread, int prio);
typedef void (*z_thread_timeout_t)(struct _timeout *timeout);
typedef void (*z_time_slice_t)(int ticks);
typedef struct k_thread *(*z_unpend1_no_timeout_t)(_wait_q_t *wait_q);
typedef int (*z_unpend_all_t)(_wait_q_t *wait_q);
typedef struct k_thread *(*z_unpend_first_thread_t)(_wait_q_t *wait_q);
typedef void (*z_unpend_thread_t)(struct k_thread *thread);
typedef void (*z_unpend_thread_no_timeout_t)(struct k_thread *thread);
typedef int (*arch_swap_t)(unsigned int key);
typedef void (*dequeue_thread_t)(void *pq, struct k_thread *thread);
typedef int32_t (*z_impl_k_usleep_t)(int us);

typedef struct {

    update_cache_t  p_update_cache;
    z_unpend_first_thread_t  p_z_unpend_first_thread;
    move_thread_to_end_of_prio_q_t  p_move_thread_to_end_of_prio_q;
    z_priq_dumb_remove_t  p_z_priq_dumb_remove;
    z_unpend_all_t  p_z_unpend_all;
    z_impl_k_is_preempt_thread_t  p_z_impl_k_is_preempt_thread;
    k_sched_unlock_t  p_k_sched_unlock;
    z_unpend1_no_timeout_t  p_z_unpend1_no_timeout;
    z_thread_timeout_t  p_z_thread_timeout;
    z_impl_k_sleep_t  p_z_impl_k_sleep;
    z_reset_time_slice_t  p_z_reset_time_slice;
    z_sched_prio_cmp_t  p_z_sched_prio_cmp;
    z_pend_thread_t  p_z_pend_thread;
    z_thread_priority_set_t  p_z_thread_priority_set;
    add_thread_timeout_t  p_add_thread_timeout;
    z_sched_wait_t  p_z_sched_wait;
    ready_thread_t  p_ready_thread;
    z_impl_k_thread_suspend_t  p_z_impl_k_thread_suspend;
    z_reschedule_irqlock_t  p_z_reschedule_irqlock;
    z_priq_dumb_add_t  p_z_priq_dumb_add;
    unready_thread_t  p_unready_thread;
    z_unpend_thread_no_timeout_t  p_z_unpend_thread_no_timeout;
    z_priq_dumb_best_t  p_z_priq_dumb_best;
    z_unpend_thread_t  p_z_unpend_thread;
    unpend_thread_no_timeout_t  p_unpend_thread_no_timeout;
    z_impl_k_thread_priority_set_t  p_z_impl_k_thread_priority_set;
    z_tick_sleep_t  p_z_tick_sleep;
    z_impl_z_current_get_t  p_z_impl_z_current_get;
    z_impl_k_yield_t  p_z_impl_k_yield;
    k_sched_lock_t  p_k_sched_lock;
    z_pend_curr_t  p_z_pend_curr;
    z_ready_thread_t  p_z_ready_thread;
    z_sched_wake_t  p_z_sched_wake;
    z_sched_start_t  p_z_sched_start;
    z_reschedule_t  p_z_reschedule;
    z_set_prio_t  p_z_set_prio;
    add_to_waitq_locked_t  p_add_to_waitq_locked;
    z_impl_k_thread_join_t  p_z_impl_k_thread_join;
    z_time_slice_t  p_z_time_slice;
    pend_t  p_pend;
    dequeue_thread_t p_dequeue_thread;
	z_impl_k_usleep_t p_z_impl_k_usleep;

    arch_swap_t  p_arch_swap;

}export_sched_symbols_t;

#define  p_rom_sched_api   ((export_sched_symbols_t *)ROM_SCHED_API_ADDR)


typedef void (*z_spin_lock_set_owner_t)(struct k_spinlock *l);
typedef bool (*z_spin_lock_valid_t)(struct k_spinlock *l);
typedef bool (*z_spin_unlock_valid_t)(struct k_spinlock *l);
typedef void (*z_thread_mark_switched_in_t)(void);
typedef void (*z_thread_mark_switched_out_t)(void);
typedef void (*_thread_timer_remove_t)(struct k_thread *thread, struct thread_timer *ttimer);
typedef void (*thread_timer_handle_expired_t)(void);
typedef void (*thread_timer_init_t)(struct thread_timer *ttimer, thread_timer_expiry_t expiry_fn,
		       void *expiry_fn_arg);
typedef bool (*thread_timer_is_running_t)(struct thread_timer *ttimer);
typedef int (*thread_timer_next_timeout_t)(void);
typedef void (*thread_timer_start_t)(struct thread_timer *ttimer, s32_t duration, s32_t period);
typedef void (*thread_timer_stop_t)(struct thread_timer *ttimer);
typedef const char *(*k_thread_name_get_t)(struct k_thread *thread);

typedef void (*z_check_stack_sentinel_t)(void);


typedef struct {

    z_thread_mark_switched_out_t  p_z_thread_mark_switched_out;
    z_spin_unlock_valid_t  p_z_spin_unlock_valid;
    z_spin_lock_valid_t  p_z_spin_lock_valid;
    z_thread_mark_switched_in_t  p_z_thread_mark_switched_in;
    z_spin_lock_set_owner_t  p_z_spin_lock_set_owner;
    z_check_stack_sentinel_t  p_z_check_stack_sentinel;
    k_thread_name_get_t  p_k_thread_name_get;


    thread_timer_next_timeout_t  p_thread_timer_next_timeout;
    thread_timer_init_t  p_thread_timer_init;
    thread_timer_is_running_t  p_thread_timer_is_running;
    thread_timer_stop_t  p_thread_timer_stop;
    thread_timer_handle_expired_t  p_thread_timer_handle_expired;
    thread_timer_start_t  p_thread_timer_start;
    _thread_timer_remove_t  p__thread_timer_remove;

}export_thread_symbols_t;

#define  p_rom_thread_api   ((export_thread_symbols_t *)ROM_THREAD_API_ADDR)

typedef int32_t (*elapsed_t)(void);
typedef struct _timeout *(*first_t)(void);
typedef struct _timeout *(*next_t)(struct _timeout *t);
typedef int32_t (*next_timeout_t)(void);
typedef void (*remove_timeout_t)(struct _timeout *t);
typedef k_ticks_t (*timeout_rem_t)(const struct _timeout *timeout);
typedef void (*sys_clock_announce_t)(int32_t ticks);
typedef void (*sys_clock_compensate_t)(uint32_t ticks);
typedef int64_t (*sys_clock_tick_get_t)(void);
typedef uint32_t (*sys_clock_tick_get_32_t)(void);
typedef uint64_t (*sys_clock_timeout_end_calc_t)(k_timeout_t timeout);
typedef int (*z_abort_timeout_t)(struct _timeout *to);
typedef void (*z_add_timeout_t)(struct _timeout *to, _timeout_func_t fn,
		   k_timeout_t timeout);
typedef int32_t (*z_get_next_timeout_expiry_t)(void);
typedef void (*z_impl_k_busy_wait_t)(uint32_t usec_to_wait);
typedef int64_t (*z_impl_k_uptime_ticks_t)(void);
typedef void (*z_set_timeout_expiry_t)(int32_t ticks, bool is_idle);
typedef k_ticks_t (*z_timeout_expires_t)(const struct _timeout *timeout);
typedef k_ticks_t (*z_timeout_remaining_t)(const struct _timeout *timeout);

typedef uint64_t (*_get_current_jiffies_t)(void *ctx);
typedef void (*acts_timer_isr_t)(void *arg);
typedef uint32_t (*sys_clock_cycle_get_32_t)(void);
typedef uint32_t (*sys_clock_elapsed_t)(void);
typedef void (*sys_clock_idle_exit_t)(void);
typedef void (*sys_clock_set_timeout_t)(int32_t ticks, bool idle);

typedef struct {

    elapsed_t  p_elapsed;
    z_get_next_timeout_expiry_t  p_z_get_next_timeout_expiry;
    z_add_timeout_t  p_z_add_timeout;
    z_impl_k_busy_wait_t  p_z_impl_k_busy_wait;
    sys_clock_tick_get_t  p_sys_clock_tick_get;
    sys_clock_announce_t  p_sys_clock_announce;
    next_timeout_t  p_next_timeout;
    sys_clock_timeout_end_calc_t  p_sys_clock_timeout_end_calc;
    sys_clock_compensate_t  p_sys_clock_compensate;
    first_t  p_first;
    remove_timeout_t  p_remove_timeout;
    next_t  p_next;
    sys_clock_tick_get_32_t  p_sys_clock_tick_get_32;
    z_abort_timeout_t  p_z_abort_timeout;
    z_impl_k_uptime_ticks_t  p_z_impl_k_uptime_ticks;
    z_set_timeout_expiry_t  p_z_set_timeout_expiry;
	timeout_rem_t  p_timeout_rem;
	z_timeout_remaining_t  p_z_timeout_remaining;
	z_timeout_expires_t  p_z_timeout_expires;

    _get_current_jiffies_t  p__get_current_jiffies;


    sys_clock_set_timeout_t  p_sys_clock_set_timeout;
    sys_clock_cycle_get_32_t  p_sys_clock_cycle_get_32;
    sys_clock_elapsed_t  p_sys_clock_elapsed;
    acts_timer_isr_t  p_acts_timer_isr;
    sys_clock_idle_exit_t  p_sys_clock_idle_exit;
	
	
}export_timeout_symbols_t;

#define  p_rom_timeout_api   ((export_timeout_symbols_t *)ROM_TIMEOUT_API_ADDR)

typedef int (*cancel_async_locked_t)(struct k_work *work);
typedef int (*cancel_delayable_async_locked_t)(struct k_work_delayable *dwork);
typedef bool (*notify_queue_locked_t)(struct k_work_q *queue);
typedef int (*schedule_for_queue_locked_t)(struct k_work_q **queuep,
				     struct k_work_delayable *dwork,
				     k_timeout_t delay);
typedef int (*submit_to_queue_locked_t)(struct k_work *work,
				  struct k_work_q **queuep);
typedef bool (*unschedule_locked_t)(struct k_work_delayable *dwork);
typedef void (*work_queue_main_t)(void *workq_ptr, void *p2, void *p3);
typedef void (*work_timeout_t)(struct _timeout *to);
typedef int (*k_delayed_work_cancel_t)(struct k_delayed_work *work);
typedef int (*k_delayed_work_submit_t)(struct k_delayed_work *work,
					k_timeout_t delay);
typedef int (*k_delayed_work_submit_to_queue_t)(struct k_work_q *work_q,
						 struct k_delayed_work *work,
						 k_timeout_t delay);
typedef int (*k_work_reschedule_t)(struct k_work_delayable *dwork,
				     k_timeout_t delay);
typedef int (*k_work_reschedule_for_queue_t)(struct k_work_q *queue,
				 struct k_work_delayable *dwork,
				 k_timeout_t delay);
typedef int (*k_work_schedule_t)(struct k_work_delayable *dwork,
				   k_timeout_t delay);
typedef int (*k_work_schedule_for_queue_t)(struct k_work_q *queue,
			       struct k_work_delayable *dwork,
			       k_timeout_t delay);
typedef int (*k_work_submit_t)(struct k_work *work);
typedef int (*k_work_submit_to_queue_t)(struct k_work_q *queue,
			    struct k_work *work);
typedef int (*k_work_cancel_delayable_t)(struct k_work_delayable *dwork);
typedef int (*k_work_delayable_busy_get_t)(const struct k_work_delayable *dwork);
typedef int (*k_work_busy_get_t)(const struct k_work *work);


typedef struct {

    work_timeout_t  p_work_timeout;
    k_work_reschedule_for_queue_t  p_k_work_reschedule_for_queue;
    k_delayed_work_cancel_t  p_k_delayed_work_cancel;
    k_work_submit_t  p_k_work_submit;
    notify_queue_locked_t  p_notify_queue_locked;
    submit_to_queue_locked_t  p_submit_to_queue_locked;
    cancel_async_locked_t  p_cancel_async_locked;
    work_queue_main_t  p_work_queue_main;
    cancel_delayable_async_locked_t  p_cancel_delayable_async_locked;
    unschedule_locked_t  p_unschedule_locked;
    k_work_schedule_for_queue_t  p_k_work_schedule_for_queue;
    k_delayed_work_submit_t  p_k_delayed_work_submit;
    k_delayed_work_submit_to_queue_t  p_k_delayed_work_submit_to_queue;
    k_work_cancel_delayable_t  p_k_work_cancel_delayable;
    k_work_submit_to_queue_t  p_k_work_submit_to_queue;
    k_work_schedule_t  p_k_work_schedule;
    schedule_for_queue_locked_t  p_schedule_for_queue_locked;
    k_work_reschedule_t  p_k_work_reschedule;
	k_work_delayable_busy_get_t  p_k_work_delayable_busy_get;
	k_work_busy_get_t  p_k_work_busy_get;

}export_work_symbols_t;

#define  p_rom_work_api   ((export_work_symbols_t *)ROM_WORK_API_ADDR)


#include <rbuf/rbuf_core.h>
#include <rbuf/rbuf_pool.h>

struct rbuf_rom_export {
	/* rbuf_pool.c */
	rbuf_pool_t *(*rbuf_pool_init)(char *buf, unsigned int size);
	rbuf_t *(*rbuf_pool_alloc)(rbuf_pool_t *pool, unsigned int size, unsigned int mode);
	int (*rbuf_pool_free)(rbuf_t *buf);

	/* rbuf_core.c */
	rbuf_t *(*rbuf_init_buf)(char *buf, unsigned int size, unsigned int mode);
	unsigned int (*rbuf_get_space)(rbuf_t *buf);
	void* (*rbuf_put_claim)(rbuf_t *buf, unsigned int size, unsigned int *psz);
	int (*rbuf_put_finish)(rbuf_t *buf, unsigned int size);
	void* (*rbuf_get_claim)(rbuf_t *buf, unsigned int size, unsigned int *psz);
	int (*rbuf_get_finish)(rbuf_t *buf, unsigned int size);
	unsigned int (*rbuf_get_hdl)(rbuf_t *buf, unsigned int size, rbuf_hdl hdl, void *ctx);
};

#define  p_rom_rbuf_api  ((struct rbuf_rom_export *)ROM_RBUF_API_ADD)

void sys_rom_code_init(void);

#endif /*_ROM_INTERFACE_H_*/