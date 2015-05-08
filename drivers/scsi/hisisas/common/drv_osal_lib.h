/*
 * Huawei Pv660/Hi1610 sas controller driver interface
 *
 * Copyright 2015 Huawei.
 *
 * This file is licensed under GPLv2.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
*/

#ifndef __DRV_OSAL_LIB_H__
#define __DRV_OSAL_LIB_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#ifndef SIZE_T
    typedef unsigned long           SIZE_T;
#endif

static inline unsigned long  osal_jiffies_to_msecs(const unsigned long j)
{
#if HZ <= MSEC_PER_SEC && !(MSEC_PER_SEC % HZ)
	return (MSEC_PER_SEC / HZ) * j;
#elif HZ > MSEC_PER_SEC && !(HZ % MSEC_PER_SEC)
	return (j + (HZ / MSEC_PER_SEC) - 1)/(HZ / MSEC_PER_SEC);
#else
# if BITS_PER_LONG == 32
	return (HZ_TO_MSEC_MUL32 * j) >> HZ_TO_MSEC_SHR32;
# else
	return (j * HZ_TO_MSEC_NUM) / HZ_TO_MSEC_DEN;
# endif
#endif
}
 
#define OSAL_JIFFIES_TO_NS(jiff)     (osal_jiffies_to_msecs(jiff) * 1000000ULL)


#define OSAL_PER_NS   (1000000000)    
typedef struct
{
    long lSec;           
    long lNanoSec;    
}STD_TIMESPEC_S;

#define OSAL_CURRENT_TIME_NSEC \
({\
    unsigned long long ulNanoTime;\
    struct timespec stCurTime = current_kernel_time();\
    ulNanoTime = (unsigned long long)stCurTime.tv_sec * OSAL_PER_NS + stCurTime.tv_nsec;\
    ulNanoTime;\
})

struct osal_timer_list {
    struct timer_list std_timer;
    unsigned long expires;
};
typedef void (*osal_timer_cb_t)(unsigned long data);

extern s32 osal_lib_init_timer(struct osal_timer_list *osal_timer, unsigned long expires,
                             unsigned long data, osal_timer_cb_t timer_cb);
extern s32 osal_lib_add_timer(struct osal_timer_list *osal_timer);
extern s32 osal_lib_mod_timer(struct osal_timer_list *osal_timer, unsigned long expires );
extern s32 osal_lib_del_timer(struct osal_timer_list *osal_timer );
extern s32 osal_lib_del_timer_sync(struct osal_timer_list *osal_timer );
extern bool osal_lib_is_timer_activated(struct osal_timer_list *osal_timer );


#define  OSAL_time_after						time_after
#define  OSAL_time_before						time_before
#define  OSAL_time_in_range						time_in_range
#define  OSAL_time_after64						time_after64
#define  OSAL_time_before64						time_before64


#define OSAL_Kmalloc(size, flag)    kmalloc(size, flag)
#define OSAL_Kfree(p)               kfree(p)
#define OSAL_GFP_KERNEL             GFP_KERNEL
#define OSAL_GFP_ATOMIC             GFP_ATOMIC
#define OSAL_get_order(size)        get_order(size)
#define OSAL_free_pages(addr, order)            free_pages(addr, order)
#define OSAL_get_free_pages(gfp_mask, order)   __get_free_pages(gfp_mask, order)

#define DRV_GFP_KERNEL              OSAL_GFP_KERNEL

#define OSAL_Malloc(size)           vmalloc(size)
#define OSAL_Free(p)                vfree(p)

#define OSAL_VMalloc(size)          vmalloc(size)
#define OSAL_VFree(p)               vfree(p)


#define OSAL_SEMAPHORE_S                    struct semaphore  
#define OSAL_SemaInit(sem, val)             sema_init(sem, val)
#define OSAL_sema_up(sem)                   up(sem)
#define OSAL_sema_down(sem)                 down(sem)
#define OSAL_sema_down_interruptible(sem)   down_interruptible(sem)
#define OSAL_sema_down_timeout(sem, timeout) down_timeout(sem, timeout)
#define OSAL_sema_destroy(sem)              {(void)(sem);}

#define OSAL_init_MUTEX_LOCKED(sem)         sema_init(sem, 0)
#define OSAL_init_MUTEX(sem)                sema_init(sem, 1)

typedef  struct mutex  OSAL_STRUCT_MUTEX_S;                  
#define OSAL_mutex_init(lock)               mutex_init(lock)
#define OSAL_mutex_destroy(lock)            mutex_destroy(lock)
#define OSAL_mutex_lock(lock)               mutex_lock(lock)
#define OSAL_mutex_unlock(lock)             mutex_unlock(lock)
#define OSAL_mutex_lock_interruptible(lock) mutex_lock_interruptible(lock)

#define OSAL_SPIN_LOCK              spinlock_t
#define OSAL_SPIN_LOCK_INIT         spin_lock_init
#define OSAL_SPIN_LOCK_Lock         spin_lock
#define OSAL_SPIN_LOCK_Unlock       spin_unlock
#define OSAL_SPIN_LOCK_IRQSAVE      spin_lock_irqsave
#define OSAL_SPIN_UNLOCK_IRQRESTORE spin_unlock_irqrestore

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,24))
#define OSAL_lock_kernel()			
#define OSAL_unlock_kernel()                      
#else
#define OSAL_lock_kernel()			lock_kernel()
#define OSAL_unlock_kernel()        unlock_kernel()  
#endif

#define OSAL_STRUCT_COMPLETION                  struct completion
#define OSAL_InitCompletion(cmp)                init_completion(cmp)
#define OSAL_WaitForCompletion(cmp)             wait_for_completion(cmp)
#define OSAL_WaitForCompletionTimeOut(cmp, timeout)      wait_for_completion_timeout(cmp, timeout)
#define OSAL_Complete(cmp)                      complete(cmp)
#define OSAL_CompleteAndExit(cmp, exitcode)     complete_and_exit(cmp, exitcode)
#define OSAL_DestroyCompletion(cmp)


 
typedef struct task_struct OSAL_TASK_STRUCT_S;

#define OSAL_kthread_run        	kthread_run
#define OSAL_kthread_stop(k)    	kthread_stop(k)
#define OSAL_kthread_should_stop	kthread_should_stop	


typedef struct module OSAL_MODULE_S;

 
#define OSAL_msleep(ms)         msleep(ms)          
#define OSAL_usleep(us)         udelay(us)         
#define OSAL_mdelay(ms)         mdelay(ms)        
#define OSAL_udelay(us)         udelay(us)        

 #define OSAL_ATOMIC                 atomic_t
#define OSAL_ATOMIC64               atomic64_t
#define OSAL_ATOMIC_READ            atomic_read
#define OSAL_ATOMIC_SET             atomic_set
#define OSAL_ATOMIC_ADD             atomic_add
#define OSAL_ATOMIC_SUB             atomic_sub
#define OSAL_ATOMIC_INC             atomic_inc
#define OSAL_ATOMIC_DEC             atomic_dec
#define OSAL_ATOMIC_INIT            ATOMIC_INIT
#define OSAL_ATOMIC64_INC_RETURN    atomic64_inc_return
#define OSAL_ATOMIC64_SET           atomic64_set
#define OSAL_atomic_dec_and_test(v)	atomic_dec_and_test(v)

typedef struct kref OSAL_KREF_S;
#define OSAL_kref_init(kref)      kref_init(kref)
#define OSAL_kref_get(kref)       kref_get(kref)
#define OSAL_kref_put             kref_put

typedef struct list_head OSAL_stListHead;
#define OSAL_InitList(ptr)                  INIT_LIST_HEAD(ptr)
#define OSAL_ListAdd(new_node, head)        list_add(new_node, head)
#define OSAL_ListDel(node)                  list_del(node)
#define OSAL_ListForEach(v_pos, v_head)     list_for_each(v_pos, v_head)
#define OSAL_ListForEachEntry(pos, head, member)        list_for_each_entry(pos, head, member)
#define OSAL_ListForEachEntrySafe(pos, n, head, member)     list_for_each_entry_safe(pos, n, head, member)
#define OSAL_ListForEachSafe(pos, n, head)    list_for_each_safe(pos, n, head)
#define OSAL_ListAddTail                    list_add_tail
#define OSAL_ListEntry                      list_entry
#define OSAL_ListEmpty                      list_empty
#define OSAL_ListSpliceInit(list, head)     list_splice_init(list, head)
#define OSAL_list_del_init(entry)			list_del_init(entry)

typedef struct hlist_head OSAL_HLIST_HEAD_S;
typedef struct hlist_node OSAL_HLIST_NODE_S;
#define OSAL_INIT_HLIST_HEAD(ptr)           INIT_HLIST_HEAD(ptr)
#define OSAL_hlist_entry(ptr, type, member) hlist_entry(ptr, type, member)
#define OSAL_hlist_for_each(pos, head)      hlist_for_each(pos, head)
#define OSAL_hash_64(val, bits)				hash_64(val, bits)
#define OSAL_hlist_empty(n)					hlist_empty(n)
#define OSAL_hlist_add_head(n, h)           hlist_add_head(n, h)
#define OSAL_hlist_add_before(n, next)      hlist_add_before(n, next)
#define OSAL_hlist_add_after(n, next)       hlist_add_after(n, next)
#define OSAL_hlist_del(n)					hlist_del(n)

 
#define OSAL_kmem_cache_t                           kmem_cache_t
 
#define OSAL_kmem_cache_create(name, size, align, flags, ctor)      kmem_cache_create(name, size, align, flags, ctor)
#define OSAL_kmem_cache_alloc(cachep, flags)        kmem_cache_alloc(cachep, flags)
#define OSAL_kmem_cache_free(cachep, objp)          kmem_cache_free(cachep, objp)
#define OSAL_kmem_cache_destroy(cachep)             kmem_cache_destroy(cachep)

 
typedef struct scsi_host_template OSAL_SCSI_HOST_TEMPLATE_S;
typedef struct Scsi_Host OSAL_SCSI_HOST_S;
typedef struct device OSAL_DEVICE_S;
typedef struct scsi_cmnd OSAL_SCSI_CMND_S;
typedef struct scsi_device OSAL_SCSI_DEVICE_S;
typedef struct scsi_sense_hdr OSAL_SCSI_SENSE_HDR_S;
typedef struct scsi_lun OSAL_SCSI_LUN_S;

#define OSAL_scsi_host_alloc(sht, privsize)    scsi_host_alloc(sht, privsize)
#define OSAL_scsi_add_host(host, dev)   scsi_add_host(host, dev)
#define OSAL_scsi_remove_host(shost)    scsi_remove_host(shost)
#define OSAL_scsi_host_lookup(hostnum)  scsi_host_lookup(hostnum)
#define OSAL_scsi_host_get(shost)       scsi_host_get(shost)      
#define OSAL_scsi_host_put(shost)       scsi_host_put(shost)
#define OSAL_scsi_device_get(sdev)      scsi_device_get(sdev)
#define OSAL_scsi_device_put(sdev)      scsi_device_put(sdev)
#define OSAL_scsi_execute(sdev, cmd, data_direction, buffer, bufflen, sshdr, timeout, retries, resid)\
    scsi_execute_req(sdev, cmd, data_direction, buffer, bufflen, sshdr, timeout,retries, resid)
#define OSAL_scsi_add_device(shost, channel, id, lun)   scsi_add_device(shost, channel, id, lun)
#define OSAL_scsi_remove_device(sdev)                   scsi_remove_device(sdev)
#define OSAL_scsi_register(sht, privsize)               scsi_register(sht, privsize)
#define OSAL_scsi_unregister(shost)                     scsi_unregister(shost)
#define OSAL_scsi_set_resid(cmd, resid)                 scsi_set_resid(cmd, resid)
#define OSAL_scsi_adjust_queue_depth(sdev, tagged, tags) scsi_adjust_queue_depth(sdev, tagged, tags)
#define OSAL_scsi_bufflen(cmd)                          scsi_bufflen(cmd)
#define OSAL_scsi_device_lookup(host, ch, target, lun)  scsi_device_lookup(host, ch, target, lun)
#define OSAL_scsi_device_online(device)                 scsi_device_online(device)
#define OSAL_int_to_scsilun(id, lun)                    int_to_scsilun(id, lun)
#define OSAL_scsi_device_set_state(sdev, state)			scsi_device_set_state(sdev, state)


#define OSAL_MemCmp(Dest, Src, Count)   memcmp(Dest, Src, Count)
#define OSAL_MemCpy(Dest, Src, Count)   memcpy(Dest, Src, Count)
#define OSAL_MemSet(ToSet,Char,Count)   memset(ToSet,Char,Count) 

#define OSAL_StrCpy(Dest, Src)          strcpy(Dest, Src)
#define OSAL_StrNCpy(des,src,len)       strncpy(des,src,len)
#define OSAL_StrCat(S1,S2)              strcat(S1,S2)
#define OSAL_StrCmp(S1, S2)             strcmp(S1, S2)
#define OSAL_StrNCmp(S1, S2, L)         strncmp(S1,S2,L)
#define OSAL_StrLen(S1)                 strlen(S1)

#define OSAL_strncat(dest, src, count)	strncat(dest, src, count)
#define OSAL_snprintf                   snprintf    
#define OSAL_sprintf                    sprintf
#define OSAL_vsnprintf                  vsnprintf

#define OSAL_cpu_to_le64    cpu_to_le64
#define OSAL_le64_to_cpu    le64_to_cpu
#define OSAL_cpu_to_le32    cpu_to_le32
#define OSAL_le32_to_cpu    le32_to_cpu
#define OSAL_cpu_to_le16    cpu_to_le16
#define OSAL_le16_to_cpu    le16_to_cpu
#define OSAL_cpu_to_be64    cpu_to_be64
#define OSAL_be64_to_cpu    be64_to_cpu
#define OSAL_cpu_to_be32    cpu_to_be32
#define OSAL_be32_to_cpu    be32_to_cpu
#define OSAL_cpu_to_be16    cpu_to_be16
#define OSAL_be16_to_cpu    be16_to_cpu

#define OSAL_virt_to_phys(address)                  virt_to_phys(address)
#define OSAL_phys_to_virt(address)                  phys_to_virt(address)

typedef dma_addr_t OSAL_DMA_ADDR_T_S;
#define OSAL_dma_pool_create(name,dev,size,align,allocation)\
    dma_pool_create(name,dev,size,align,allocation) 
#define OSAL_dma_pool_destroy(pool)     dma_pool_destroy(pool)
#define OSAL_dma_pool_alloc(pool, mem_flags, handle)\
    dma_pool_alloc(pool, mem_flags, handle)    
#define OSAL_dma_pool_free(pool, vaddr, dma)\
    dma_pool_free(pool, vaddr, dma)
#define OSAL_dma_alloc_coherent(dev, size, addr, gfp)\
    dma_alloc_coherent(dev, size, addr, gfp)
#define OSAL_dma_free_coherent(dev, size, va, addr)\
    dma_free_coherent(dev, size, va, addr)

#define OSAL_mb()           		mb()
#define OSAL_wmb()          		wmb()
#define OSAL_readl(c)       		readl(c)
#define OSAL_writel(v,c)    		writel(v,c)
#define OSAL_readw(c)       		readw(c)
#define OSAL_writew(v,c)    		writew(v,c)
#define OSAL_ioremap(cookie,size)   ioremap(cookie,size)
#define OSAL_iounmap(cookie)        iounmap(cookie)


#define OSAL_dump_stack()                       dump_stack()
#define OSAL_get_random_bytes(buf, nbytes)      get_random_bytes(buf, nbytes)
#define OSAL_container_of(ptr, type, member)    container_of(ptr, type, member)
#define OSAL_set_fs(x)                          set_fs(x)
#define OSAL_get_fs(x)                          get_fs(x)
#define OSAL_daemonize                          daemonize   

#define OSAL_schedule()	schedule()
#define OSAL_wait_for_completion_interruptible(x)       wait_for_completion_interruptible(x)
#define OSAL_wait_for_completion_timeout(x, timeout)    wait_for_completion_timeout(x, timeout)
 
typedef struct pid_t OSAL_PID_T_S;
typedef struct pid	 OSAL_PID_S;

#define OSAL_wake_up_process(p) 		wake_up_process(p)
#define OSAL_kill_pid(pid, sig, priv)   kill_pid(pid, sig, priv)
#define OSAL_find_get_pid(nr)           find_get_pid(nr)

 
typedef struct tasklet_struct OSAL_TASKLET_STRUCT_S;

#define OSAL_set_current_state(state_value)	set_current_state(state_value)
#define OSAL_tasklet_schedule(t)			tasklet_schedule(t)
#define OSAL_tasklet_kill(t)				tasklet_kill(t)
#define OSAL_tasklet_init(t, func, data)	tasklet_init(t, func, data)

 
typedef irqreturn_t OSAL_IRQRETURN_T; 
#define OSAL_IRQ_HANDLER_T irq_handler_t

#define OSAL_synchronize_irq(irq)			synchronize_irq(irq)
#define OSAL_in_interrupt()					in_interrupt()	
#define OSAL_free_irq(irq, dev_id)			free_irq(irq, dev_id)
#define OSAL_request_irq(irq, handler, flags, dev_name, dev_id) request_irq(irq, handler, flags, dev_name, dev_id)

 
typedef struct work_struct OSAL_WORK_STRUCT_S;
typedef struct workqueue_struct OSAL_WORKQUEUE_STRUCT_S;
typedef struct delayed_work OSAL_DELAYED_WORK_S;

#define OSAL_schedule_work(work)                    schedule_work(work)
#define OSAL_flush_scheduled_work()                 flush_scheduled_work()
#define OSAL_delayed_work_pending(w)                delayed_work_pending(w)
#define OSAL_cancel_delayed_work(work)              __cancel_delayed_work(work)
#define OSAL_cancel_delayed_work_sync(work)         cancel_delayed_work(work)
#define OSAL_schedule_delayed_work(dwork, delay)    schedule_delayed_work(dwork, delay)

typedef wait_queue_head_t OSAL_WAIT_QUEUE_HEAD_T_S;

#define OSAL_init_waitqueue_head(q) init_waitqueue_head(q)


void log_kproc(int module_id, int level, int msg_code, const char *funcName, int line, const char * format, ...);

#define DRV_LOG(module_id, level, msg_code, format,arg...)\
	    log_kproc(module_id, level, msg_code, __FUNCTION__, __LINE__,  format, ##arg)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif 
