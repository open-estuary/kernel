/*
 * Huawei Pv660/Hi1610 sas controller driver OS related
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


#ifndef __SALOSINTF_H__
#define __SALOSINTF_H__

#include "drv_common.h"


#define  SAS_DRV_MODULE_ID         	0
#define  SAS_DRV_MODULE_NAME       	"sas"


#define SAS_SPIN_LOCK_IRQSAVE  		OSAL_SPIN_LOCK_IRQSAVE
#define SAS_SPIN_UNLOCK_IRQRESTORE 	OSAL_SPIN_UNLOCK_IRQRESTORE

#define SAS_SPINLOCK_T              OSAL_SPIN_LOCK
#define SAS_SPIN_LOCK_INIT     		OSAL_SPIN_LOCK_INIT
#define SAS_SPIN_LOCK     			OSAL_SPIN_LOCK_Lock
#define SAS_SPIN_UNLOCK 			OSAL_SPIN_LOCK_Unlock

#define SAS_STRUCT_COMPLETION   	OSAL_STRUCT_COMPLETION
#define SAS_INIT_COMPLETION(cmp)       OSAL_InitCompletion(cmp)
#define SAS_WAIT_FOR_COMPLETION(cmp)    OSAL_WaitForCompletion(cmp)
#define SAS_WAIT_FOR_COMPLETION_TIME_OUT(cmp, timeout)   OSAL_WaitForCompletionTimeOut(cmp, timeout)
#define SAS_COMPLETE(cmp)                      OSAL_Complete(cmp)
#define SAS_COMPLETE_AND_EXIT(cmp, exitcode)  OSAL_CompleteAndExit(cmp, exitcode)

#define SAS_SCSI_HOST_LOOKUP(hostnum)  OSAL_scsi_host_lookup(hostnum)
#define SAS_SCSI_HOST_PUT(shost)       OSAL_scsi_host_put(shost)
#define SAS_SCSI_DEVICE_PUT(sdev)      OSAL_scsi_device_put(sdev)

#define SAS_MEMSET(psource, value, size) OSAL_MemSet(psource, value, size)
#define SAS_MEMCMP(psource,pdest, size) OSAL_MemCmp(psource,pdest, size)
#define SAS_MEMCPY(des, src, len) OSAL_MemCpy(des, src, len);

#define SAS_STRLEN(S1) OSAL_StrLen(S1)
#define SAS_STRCMP(S1, S2)    OSAL_StrCmp(S1, S2)
#define SAS_STRNCPY(des, src, len) OSAL_StrNCpy(des, src, len)

#define SAS_CPU_TO_LE32   OSAL_cpu_to_le32
#define SAS_BE32_TO_CPU   OSAL_be32_to_cpu

#define SAS_IOREMAP(cookie,size)   OSAL_ioremap(cookie,size)
#define SAS_IOUNMAP(cookie)        OSAL_iounmap(cookie)
#define SAS_READL(c)               OSAL_readl(c)
#define SAS_WRITEL(v,c)            OSAL_writel(v,c)

#define SAS_MSLEEP(ms)  msleep(ms)
#define SAS_MDELAY(ms)  mdelay(ms)


#define SAS_ATOMIC  OSAL_ATOMIC
#define SAS_ATOMIC_READ  OSAL_ATOMIC_READ
#define SAS_ATOMIC_SET OSAL_ATOMIC_SET
#define SAS_ATOMIC_INC OSAL_ATOMIC_INC
#define SAS_ATOMIC_DEC OSAL_ATOMIC_DEC
#define SAS_ATOMIC_DEC_AND_TEST(v)   OSAL_atomic_dec_and_test(v)


#define SAS_SET_USER_NICE(threadPriLevel)   OSAL_SetCurThrdPriority(threadPriLevel)
#define SAS_DAEMONIZE(a)       OSAL_Daemonize(a)
#define SAS_ALLOWSIGNAL(a)   OSAL_AllowSignal(a)
#define SAS_KERNELTHREAD(a,b,c)  OSAL_KernelThread(a,b,c)
#define SAS_KTHREAD_STOP(k)    OSAL_kthread_stop(k)

#define SAS_WAKE_UP_PROCESS(p)  OSAL_wake_up_process(p)
#define SAS_KILL_PID(pid, sig, priv)  OSAL_kill_pid(pid, sig, priv)
#define SAS_FIND_GET_PID(nr)    OSAL_find_get_pid(nr)


#define SAS_SET_CURRENT_STATE(state_value)   OSAL_set_current_state(state_value)


#define SAS_REQUEST_IRQ(irq, handler, flags, dev_name, dev_id)  OSAL_request_irq(irq, handler, flags, dev_name, dev_id)
#define SAS_FREE_IRQ(irq, dev_id)  OSAL_free_irq(irq, dev_id)
#define SAS_SYNCHRONIZE_IRQ(irq)   OSAL_synchronize_irq(irq)


#define SAS_KMALLOC(size, flag) kmalloc(size, flag)
#define SAS_VMALLOC(size)   vmalloc(size)
#define SAS_KFREE(ptr)  kfree(ptr)
#define SAS_VFREE(ptr)  vfree(ptr)


#define SAL_BIT(x)		((1)<<(x))


#define SAS_INIT_LIST_HEAD(ptr)  OSAL_InitList(ptr)
#define SAS_LIST_ADD(new_node, head) OSAL_ListAdd(new_node, head)
#define SAS_LIST_DEL(node)  OSAL_ListDel(node)
#define SAS_LIST_FOR_EACH(v_pos, v_head)  OSAL_ListForEach(v_pos, v_head)
#define SAS_LIST_FOR_EACH_SAFE(pos, n, head) OSAL_ListForEachSafe(pos, n, head)
#define SAS_LIST_ADD_TAIL OSAL_ListAddTail
#define SAS_LIST_ENTRY OSAL_ListEntry
#define SAS_LIST_EMPTY OSAL_ListEmpty
#define SAS_LIST_DEL_INIT(entry)  OSAL_list_del_init(entry)

#define SAS_SEMA_S				OSAL_SEMAPHORE_S
#define SAS_Down(sem)  			OSAL_sema_down(sem)
#define SAS_Up(sem)	   			OSAL_sema_up(sem)
#define SAS_SemaInit(sem,num)   OSAL_SemaInit(sem,num)
#define SAS_MutexInit(sem)      OSAL_init_MUTEX(sem)
#define SAS_Down_Timeout(sem)	OSAL_sema_down_timeout(sem)

static inline u32 SAS_GET_LIST_COUNT(struct list_head *pstListToCount)
{
	u32 cnt = 0;
	struct list_head *pstList = NULL;
	SAS_LIST_FOR_EACH(pstList, pstListToCount) {
		cnt++;
	}
	return cnt;
}

#define SAS_GET_FS(x)    OSAL_get_fs(x)
#define SAS_SET_FS(x)    OSAL_set_fs(x)
#define SAS_DUMP_STACK() OSAL_dump_stack()

#endif
