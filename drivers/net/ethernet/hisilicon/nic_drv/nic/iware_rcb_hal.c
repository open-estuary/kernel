/*--------------------------------------------------------------------------------------------------------------------------*/
/*!!Warning: This is a key information asset of Huawei Tech Co.,Ltd                                                         */
/*CODEMARK:64z4jYnYa5t1KtRL8a/vnMxg4uGttU/wzF06xcyNtiEfsIe4UpyXkUSy93j7U7XZDdqx2rNx
p+25Dla32ZW7osA9Q1ovzSUNJmwD2Lwb8CS3jj1e4NXnh+7DT2iIAuYHJTrgjUqp838S0X3Y
kLe48+m6h9vMt03DFcyLbRZLl4l78X9jvgaHHi4vgkp6ssb1n/9nRprZtSc7fuUjjhJHtgsP
oVY8h5mvKCtTr6+2AyqYgSCzD9mG5QbOBRJ3agOasw+ePekEOFVj8VOmOLo6lA==*/
/*--------------------------------------------------------------------------------------------------------------------------*/
/************************************************************************

  Hisilicon NIC driver
  Copyright(c) 2014 - 2019 Huawei Corporation.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:TBD

************************************************************************/

#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/smp.h>
#include <linux/ip.h>

#include <net/busy_poll.h>
/*
#include "iware_module.h"
#include "iware_typedef.h" */
#include "iware_error.h"
#include "iware_log.h"

#include "iware_nic_main.h"
#include "iware_rcb_hal.h"

/**
 *rcb_int_ctrl_hw - rcb irq enable control
 *@rcb_dev: rcb device
 *@flag:ring flag fro tx or rx
 *@enable:enable
 */
static void rcb_int_ctrl_hw(struct rcb_device *rcb_dev,
			    u32 flag, u32 enable)
{
	u32 int_mask_en = !enable;

	if ((u32)flag & RCB_INT_FLAG_TX) {
		rcb_write_reg(rcb_dev, RCB_RING_INTMSK_TXWL_REG,
			      int_mask_en);
		rcb_write_reg(rcb_dev, RCB_RING_INTMSK_TX_OVERTIME_REG,
			      int_mask_en);
	}

	if ((u32)flag & RCB_INT_FLAG_RX) {
		rcb_write_reg(rcb_dev, RCB_RING_INTMSK_RXWL_REG,
			      int_mask_en);
		rcb_write_reg(rcb_dev, RCB_RING_INTMSK_RX_OVERTIME_REG,
			      int_mask_en);
	}

	if ((u32)flag & RCB_INT_FLAG_ERR) {
		rcb_write_reg(rcb_dev, RCB_RING_INTMSK_CFG_ERR_REG,
			      int_mask_en);
	}
}

/**
 *rcb_get_int_stat - get rcb irq status
 *@rcb_dev: rcb device
 *return status
 */
u32 rcb_get_int_stat(struct rcb_device *rcb_dev)
{
	/*»ñÈ¡½ÓÊÕÖÐ¶Ï×´Ì¬£¬ÓÐÖÐ¶Ï£¬º¯Êý·µ»Ø0·ñÔò·µ»Ø-1 */

	if (rcb_read_reg(rcb_dev, RCB_RING_INTSTS_RX_RING_REG))
		return 1;
	else if (rcb_read_reg(rcb_dev, RCB_RING_INTSTS_RX_OVERTIME_REG))
		return 2;
	else
		return 0;
}

/**
 *rcb_show_tx_cnt - show rcb tx statistics
 *@rcb_dev: rcb device
 */
void rcb_show_tx_cnt(struct nic_ring_pair *ring)
{
	u64 cnt_tmp[4];
	struct rcb_device *rcb_dev = &ring->rcb_dev;
	u32 ring_id = rcb_dev->index;

	cnt_tmp[0] = ring->hw_stats.tx_pkts;;

	cnt_tmp[1] = rcb_read_reg(rcb_dev, RCB_RING_TX_RING_FBDNUM_REG);

	cnt_tmp[2] = rcb_read_reg(rcb_dev, RCB_RING_TX_RING_TAIL_REG);

	cnt_tmp[3] = rcb_read_reg(rcb_dev, RCB_RING_TX_RING_HEAD_REG);

	osal_printf("Ring %d tx count   :%lld \r\n", ring_id, cnt_tmp[0]);
	osal_printf("Ring %d untx count :%lld \r\n", ring_id, cnt_tmp[1]);
	osal_printf("Ring %d tx tail	:%lld \r\n", ring_id, cnt_tmp[2]);
	osal_printf("Ring %d tx head	:%lld \r\n", ring_id, cnt_tmp[3]);

}

/**
 *rcb_show_tx_cnt - show rcb rx statistics
 *@rcb_dev: rcb device
 */
void rcb_show_rx_cnt(struct nic_ring_pair *ring)
{
	u64 cnt_tmp[4];
	struct rcb_device *rcb_dev = &ring->rcb_dev;
	u32 ring_id = rcb_dev->index;

	cnt_tmp[0] = ring->hw_stats.rx_pkts;

	cnt_tmp[1] = rcb_read_reg(rcb_dev, RCB_RING_RX_RING_FBDNUM_REG);

	cnt_tmp[2] = rcb_read_reg(rcb_dev, RCB_RING_RX_RING_TAIL_REG);

	cnt_tmp[3] = rcb_read_reg(rcb_dev, RCB_RING_RX_RING_HEAD_REG);

	osal_printf("Ring %d rx count   :%lld \r\n", ring_id, cnt_tmp[0]);
	osal_printf("Ring %d unrx count :%lld \r\n", ring_id, cnt_tmp[1]);
	osal_printf("Ring %d rx tail	:%lld \r\n", ring_id, cnt_tmp[2]);
	osal_printf("Ring %d rx head	:%lld \r\n", ring_id, cnt_tmp[3]);

}

/**
 *rcb_get_tx_ring_head_hw - get ring head
 *@rcb_dev: rcb device
 *return ring head
 */

static u32 rcb_get_tx_ring_head_hw(struct rcb_device *rcb_dev)
{
    u32 head;

    head = rcb_read_reg(rcb_dev, RCB_RING_TX_RING_HEAD_REG);

    return head;
}

#if 0  //TBD
static inline void rcb_get_rx_ring_head(struct rcb_device *rcb_dev,
		u32 *head)
{
	*head = rcb_read_reg(rcb_dev, RCB_RING_RX_RING_HEAD_REG);
}

/**
 *rcb_get_tx_ring_tail - get ring tail
 *@rcb_dev: rcb device
 *@pulTail:tail
 */
static inline void rcb_get_rx_ring_tail(struct rcb_device *rcb_dev,
		u32 *tail)
{
	*tail = rcb_read_reg(rcb_dev, RCB_RING_RX_RING_TAIL_REG);

}
#endif

/**
 *rcb_write_tx_ring_tail - update ring tail
 *@rcb_dev: rcb device
 *@pulTail:tail
 */
static inline void rcb_write_tx_ring_tail(struct rcb_device *rcb_dev,
		u32 tail)
{
	rcb_write_reg(rcb_dev, RCB_RING_TX_RING_TAIL_REG, tail);

}

static u32 rcb_get_rx_ring_vld_bd_num_hw(
	struct rcb_device *rcb_dev)
{
	return rcb_read_reg(rcb_dev, RCB_RING_RX_RING_FBDNUM_REG);
}

/**
 *rcb_write_rx_ring_head - update ring head
 *@rcb_dev: rcb device
 *@pulTail:tail
 */
static inline void rcb_write_rx_ring_head(struct rcb_device *rcb_dev,
	u32 head)
{
	rcb_write_reg(rcb_dev, RCB_RING_RX_RING_HEAD_REG, head);

}

/**
 *rcb_print_pkt - print packet
 *@rcb_dev: rcb device
 *@pulTail:tail
 */
void rcb_print_pkt(u8 *packet, u32 length)
{
	u32 i = 0;

	if (NULL == packet)
		return;

	for (i = 0; i < length; i++) {
		osal_printf("%02x", *(packet + i));
		if (15 == (i % 16))
			osal_printf("\n\r");
	}
	osal_printf("\n\r");
}

/**
 *rcb_rx_ring_init - init rcb rx ring
 *@ring: rcb ring
 *return status
 */
static int rcb_rx_ring_init(struct nic_ring_pair *ring)
{
	u32 bd_idx;
	int ret;
	u32 ring_id = 0;
	u64 bd_paddr = 0;
	u32 bd_size_type = 0;
	struct rcb_rx_des *rx_desc = NULL;
	struct nic_rx_ring *rx_ring = &ring->rx_ring;
	struct rcb_device *rcb_dev = &ring->rcb_dev;
	struct nic_rx_buffer *rx_bi = NULL;
	struct sk_buff *skb = NULL;
	struct net_device *netdev = ring->netdev;

	ring_id = ring->rcb_dev.index;
	log_dbg(ring->dev, "ring_id = %d\n", ring_id);

	rx_ring->desc
	    =
	    osal_kzalloc(rx_ring->count * sizeof(struct rcb_rx_des),
			 GFP_ATOMIC);
	if (NULL == rx_ring->desc) {
		log_err(ring->dev, "osal_kzalloc fail, ring_id = %d\n", ring_id);
		return -EINVAL;
	}
	rx_ring->rx_buffer_info =
	    osal_kzalloc(rx_ring->count * sizeof(struct nic_rx_buffer),
			 GFP_ATOMIC);
	if (NULL == rx_ring->rx_buffer_info) {
		log_err(ring->dev, "osal_kzalloc fail, ring_id = %d\n", ring_id);
		ret = -EINVAL;
		goto malloc_bd_fail;
	}
	log_dbg(ring->dev,
		"desc(%#llx) rx_buffer_info(%#llx) count(%d) buf_size(%d)\n",
		(u64) rx_ring->desc, (u64) rx_ring->rx_buffer_info,
		rx_ring->count, rcb_dev->buf_size);

	rx_ring->next_to_clean = 0;
	rx_ring->next_to_use = 0;
	for (bd_idx = 0; bd_idx < rx_ring->count; bd_idx++) {
		skb = netdev_alloc_skb(netdev, rcb_dev->buf_size);
		if (NULL == skb) {
			log_err(ring->dev,
				"netdev_alloc_skb fail! bd_idx = %d\n", bd_idx);
			ret = -EINVAL;
			goto alloc_skb_fail;
		}
		rx_bi = &rx_ring->rx_buffer_info[bd_idx];
		rx_bi->skb = skb;
		rx_desc = RCB_RX_DESC(rx_ring, bd_idx);
		rx_bi->dma = (dma_addr_t) virt_to_phys(skb->data);
		rx_desc->addr = rx_bi->dma;

		rx_desc->word2.bits.VLD = 0;
	}

	/*complete recieve all packets*/
	wmb();

	bd_paddr = virt_to_phys(rx_ring->desc);
	log_dbg(ring->dev, "desc=%#llx desc_p=%#llx\n",
		(u64) rx_ring->desc, (u64) bd_paddr);

	rcb_write_reg(rcb_dev, RCB_RING_RX_RING_BASEADDR_L_REG,
		      (u32) bd_paddr);
	rcb_write_reg(rcb_dev, RCB_RING_RX_RING_BASEADDR_H_REG,
		      (u32) (bd_paddr >> 32));

	switch (rcb_dev->buf_size) {
	case 512:
		bd_size_type = BD_SIZE_512_TYPE;
		break;
	case 1024:
		bd_size_type = BD_SIZE_1024_TYPE;
		break;
	case 2048:
		bd_size_type = BD_SIZE_2048_TYPE;
		break;
	case 4096:
		bd_size_type = BD_SIZE_4096_TYPE;
		break;
	default:
		log_err(ring->dev, "buf_size=%#x error! ring_id = %d\n",
			rcb_dev->buf_size, ring_id);
		ret = HRD_COMMON_ERR_INPUT_INVALID;
		goto alloc_skb_fail;
	}
	rcb_write_reg(rcb_dev, RCB_RING_RX_RING_BD_LEN_REG, bd_size_type);

	rcb_write_reg(rcb_dev, RCB_RING_RX_RING_BD_NUM_REG,
		      rcb_dev->port_id_in_dsa);
	rcb_write_reg(rcb_dev, RCB_RING_RX_RING_PKTLINE_REG,
		      rcb_dev->port_id_in_dsa);

	return 0;

alloc_skb_fail:
	for (bd_idx = 0; bd_idx < rx_ring->count; bd_idx++) {
		rx_bi = &rx_ring->rx_buffer_info[bd_idx];
		if (NULL != rx_bi->skb)
			dev_kfree_skb(rx_bi->skb);
	}

malloc_bd_fail:

	if (rx_ring->desc != NULL) {
		osal_kfree(rx_ring->desc);
		rx_ring->desc = NULL;
	}
	if (rx_ring->rx_buffer_info != NULL) {
		osal_kfree(rx_ring->rx_buffer_info);
		rx_ring->rx_buffer_info = NULL;
	}
	return ret;
}

/**
 *rcb_tx_ring_init - init rcb tx ring
 *@ring: rcb ring
 *return status
 */
static int rcb_tx_ring_init(struct nic_ring_pair *ring)
{
	int ret;
	u32 ring_id;
	u64 bd_paddr = 0;
	struct nic_tx_ring *tx_ring = &ring->tx_ring;
	struct rcb_device *rcb_dev = &ring->rcb_dev;
	u32 bd_size_type = 0;

	ring_id = rcb_dev->index;

	log_dbg(ring->dev, "ring_id = %d\n", ring_id);
	tx_ring->desc
	    =
	    osal_kzalloc(tx_ring->count * sizeof(struct rcb_tx_des),
			 GFP_ATOMIC);
	if (NULL == tx_ring->desc) {
		log_err(ring->dev,
			"osal_kzalloc failed! ring_id = %d\n", ring_id);
		return HRD_COMMON_ERR_NULL_POINTER;
	}
	tx_ring->tx_buffer_info
	    =
	    osal_kzalloc(tx_ring->count * sizeof(struct nic_tx_buffer),
			 GFP_ATOMIC);
	if (NULL == tx_ring->tx_buffer_info) {
		log_err(ring->dev,
			"osal_kzalloc failed! ring_id = %d\n", ring_id);
		ret = HRD_COMMON_ERR_NULL_POINTER;
		goto malloc_bd_fail;
	}

	tx_ring->next_to_clean = 0;
	tx_ring->next_to_use = 0;

	bd_paddr = virt_to_phys(tx_ring->desc);
	rcb_write_reg(rcb_dev, RCB_RING_TX_RING_BASEADDR_L_REG,
		      (u32) bd_paddr);
	rcb_write_reg(rcb_dev, RCB_RING_TX_RING_BASEADDR_H_REG,
		      (u32) (bd_paddr >> 32));

	switch (rcb_dev->buf_size) {
	case 512:
		bd_size_type = 0;
		break;
	case 1024:
		bd_size_type = 1;
		break;
	case 2048:
		bd_size_type = 2;
		break;
	case 4096:
		bd_size_type = 3;
		break;
	default:
		log_err(ring->dev, "buf_size=%#x error! ring_id = %d\n",
			rcb_dev->buf_size, ring_id);
		ret = HRD_COMMON_ERR_INPUT_INVALID;
		goto malloc_bd_fail;
	}
	rcb_write_reg(rcb_dev, RCB_RING_TX_RING_BD_LEN_REG, bd_size_type);


	/** Modified by CHJ,for hulk3.19  ge5  get big file calltrace */
	log_dbg(ring->dev,"CHJ RCB_RING_TX_RING_BD_NUM_REG:%d\r\n",
		rcb_dev->port_id_in_dsa);
	rcb_write_reg(rcb_dev, RCB_RING_TX_RING_BD_NUM_REG,
		      rcb_dev->port_id_in_dsa);

	return 0;

malloc_bd_fail:
	if (tx_ring->desc != NULL) {
		osal_kfree(tx_ring->desc);
		tx_ring->desc = NULL;
	}
	if (tx_ring->tx_buffer_info != NULL) {
		osal_kfree(tx_ring->tx_buffer_info);
		tx_ring->tx_buffer_info = NULL;
	}
	return ret;
}

/**
 *rcb_rx_ring_uninit - uninit rcb rx ring
 *@ring: rcb ring
 */
static void rcb_rx_ring_uninit(struct nic_ring_pair *ring)
{
	u32 bd_idx;
	u32 ring_id = 0;
	struct nic_rx_ring *rx_ring = &ring->rx_ring;
	struct nic_rx_buffer *rx_bi = NULL;

	ring_id = ring->rcb_dev.index;
	log_dbg(ring->dev, "ring_id = %d\n", ring_id);

	for (bd_idx = 0; bd_idx < rx_ring->count; bd_idx++) {
		rx_bi = &rx_ring->rx_buffer_info[bd_idx];
		if (NULL != rx_bi->skb)
			dev_kfree_skb(rx_bi->skb);
	}

	if (rx_ring->desc != NULL) {
		osal_kfree(rx_ring->desc);
		rx_ring->desc = NULL;
	}
	if (rx_ring->rx_buffer_info != NULL) {
		osal_kfree(rx_ring->rx_buffer_info);
		rx_ring->rx_buffer_info = NULL;
	}

}

/**
 *rcb_tx_ring_uninit - uninit rcb tx ring
 *@ring: rcb ring
 *rerurn status
 */
static int rcb_tx_ring_uninit(struct nic_ring_pair *ring)
{
	u32 ring_id = 0;
	struct nic_tx_ring *tx_ring = &ring->tx_ring;

	ring_id = ring->rcb_dev.index;
	log_dbg(ring->dev, "ring_id = %d\n", ring_id);

	if (tx_ring->desc != NULL) {
		osal_kfree(tx_ring->desc);
		tx_ring->desc = NULL;
	}
	if (tx_ring->tx_buffer_info != NULL) {
		osal_kfree(tx_ring->tx_buffer_info);
		tx_ring->tx_buffer_info = NULL;
	}

	return 0;
}

static u32 rcb_unmap_and_free_tx_res(struct nic_ring_pair *ring, u16 *ntc)
{
	struct nic_tx_ring *tx_ring = &ring->tx_ring;
	struct nic_tx_buffer *tx_buffer = NULL;
	u32 buf_idx;
	u32 buf_num;
	u32 bytes_compl = 0;

	tx_buffer = &tx_ring->tx_buffer_info[*ntc];
	buf_num = tx_buffer->buf_num;

	bytes_compl += tx_buffer->skb->len;
	dev_kfree_skb_any(tx_buffer->skb);
	tx_buffer->buf_num = 0;
	/* unmap skb header data */
	dma_unmap_single(ring->dev, dma_unmap_addr(tx_buffer, dma),
		dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);

	/* clear tx_buffer data */
	tx_buffer->skb = NULL;
	dma_unmap_len_set(tx_buffer, len, 0);

	/*unmap remaining buffers */
	for (buf_idx = 1; buf_idx < buf_num; buf_idx++) {
		(*ntc)++;
		*ntc = (*ntc >= tx_ring->count) ? 0 : *ntc;
		tx_buffer = &tx_ring->tx_buffer_info[*ntc];

		/*unmap any remaining paged data */
		if (dma_unmap_len(tx_buffer, len)) {
			dma_unmap_page(ring->dev,
				dma_unmap_addr(tx_buffer, dma),
				dma_unmap_len(tx_buffer, len),
				DMA_TO_DEVICE);
			dma_unmap_len_set(tx_buffer, len, 0);
		}
	}
	return bytes_compl;
}


static void rcb_clean_tx_ring(struct nic_ring_pair *ring)
{
	struct nic_tx_ring *tx_ring = &ring->tx_ring;
	u32 head = 0;
	u16 ntc = tx_ring->next_to_clean;
	struct net_device *dev = ring->netdev;
	struct netdev_queue *dev_queue = NULL;

	head = tx_ring->next_to_use;

	while (likely(head != ntc)) {
		(void)rcb_unmap_and_free_tx_res(ring, &ntc);
		ntc++;
		ntc = (ntc >= tx_ring->count) ? 0 : ntc;
	}
	tx_ring->next_to_clean = ntc;

	dev_queue = netdev_get_tx_queue(dev, ring->rcb_dev.queue_index);
	netdev_tx_reset_queue(dev_queue);
}

/**
 *rcb_pkt_send_hw - send pkt
 *@skb:skb buffer
 *@ring: rcb ring
 *rerurn status
 */
netdev_tx_t rcb_pkt_send_hw(struct sk_buff *skb, struct nic_ring_pair *ring)
{
	u32 next_to_use;
	u32 buf_num = 0;
	struct rcb_tx_des *tx_desc;
	struct nic_device *nic_dev = netdev_priv(ring->netdev);
	struct nic_tx_ring *tx_ring = NULL;
	struct nic_tx_buffer *tx_buffer = NULL;
	struct netdev_queue *net_queue = NULL;

	/* u64 tx_buf_phy_addr = 0; */
	u32 ring_idx = 0;
	u32 pkt_len = skb->len;
	struct skb_frag_struct *frag;
	dma_addr_t dma;
	u32 size;
	/* struct sk_buff *tmp_skb; */
	u32 idx;


    	struct sk_buff *new_skb = NULL;

	tx_ring = &ring->tx_ring;
	ring_idx = ring->rcb_dev.index;

	log_dbg(ring->dev, "ring_idx=%d\n", ring_idx);

	if (unlikely((0 == pkt_len) || (pkt_len > nic_dev->mtu))) {
		tx_ring->tx_err_cnt++;
		log_err(ring->dev,
			"input skb->len(%d) error, nic_dev->mtu=%d!\n",
			skb->len, nic_dev->mtu);
		return NETDEV_TX_OK;
	}

	buf_num = skb_shinfo(skb)->nr_frags + 1;

	tx_desc = (struct rcb_tx_des *)tx_ring->desc;
	next_to_use = tx_ring->next_to_use;

	if (next_to_use < tx_ring->next_to_clean) {
		if (unlikely
		    (((tx_ring->next_to_clean - next_to_use) - 1) < buf_num)) {
			tx_ring->tx_err_cnt++;
			return NETDEV_TX_BUSY;
		}
	} else {
		if (unlikely
		    (((tx_ring->next_to_clean +
		       (tx_ring->count - next_to_use)) - 1) < buf_num)) {
			tx_ring->tx_err_cnt++;
			return NETDEV_TX_BUSY;
		}
	}

#if 1/*RCB_MUTIL_BD_SUPPORT */
    if (unlikely(buf_num > ring->max_buf_num)) {

 		new_skb = skb_copy(skb, GFP_ATOMIC);
        //new_skb = netdev_alloc_skb(ring->netdev, skb->len);
        if (NULL == new_skb) {
            tx_ring->tx_err_cnt++;
            log_err(ring->dev,
                "total buf_num(%d), malloc error!\n", buf_num);
            dev_kfree_skb_any(skb);
            return NETDEV_TX_OK;
        }
 
        /**change MUTIL_BD to SINGLE_BD*/
        dev_kfree_skb_any(skb);
        skb = new_skb;
        buf_num = 1;
    }
#endif

	log_dbg(ring->dev, "next_to_use=%d tx_desc=%#llx buf_num=%d\n",
		next_to_use, (u64) tx_desc, buf_num);

	tx_buffer = &tx_ring->tx_buffer_info[next_to_use];
	tx_buffer->skb = skb;
	tx_buffer->buf_num = buf_num;
	size = skb_headlen(skb);
	dma = dma_map_single(ring->dev, skb->data, size, DMA_TO_DEVICE);
	/*dma = virt_to_phys((const volatile void*)skb->data);*/
	for (idx = 0; idx < buf_num; idx++) {
		if (dma_mapping_error(ring->dev, dma))
			goto dma_error;

		dma_unmap_len_set(tx_buffer, len, size);
		dma_unmap_addr_set(tx_buffer, dma, dma);
		tx_desc[next_to_use].addr = dma;
		tx_desc[next_to_use].word2.bits.Send_size = size;
		tx_desc[next_to_use].word2.bits.Buf_num = buf_num;

		/*confug bd buffer end */
		tx_desc[next_to_use].word3.bits.VLD = 1;
		tx_desc[next_to_use].word3.bits.FE = 0;

        if (skb->ip_summed == CHECKSUM_PARTIAL) {
    		if (skb->protocol == ntohs(ETH_P_IP)) {
    			tx_desc[next_to_use].word3.bits.IP_offset = ETH_HLEN;
    			tx_desc[next_to_use].word3.bits.L3CS = 1;

	    			/* check for tcp/udp header */
	    			tx_desc[next_to_use].word3.bits.L4CS = 1;

    		} else if (skb->protocol == ntohs(ETH_P_IPV6)) {
    			tx_desc[next_to_use].word3.bits.IP_offset = ETH_HLEN;
    			tx_desc[next_to_use].word3.bits.L3CS = 1;

	    			/* check for tcp/udp header */
	    			tx_desc[next_to_use].word3.bits.L4CS = 1;
	    		}
		}

		log_dbg(ring->dev, "addr(%#llx) word2(%#x) word3(%#x)\r\n",
			tx_desc[next_to_use].addr, tx_desc[next_to_use].word2.u_32,
			tx_desc[next_to_use].word3.u_32);

		if (idx != (buf_num - 1)) {
			frag = &skb_shinfo(skb)->frags[idx];
			size = skb_frag_size(frag);
			dma = skb_frag_dma_map(ring->dev,
				frag, 0, size, DMA_TO_DEVICE);
			/*dma = virt_to_phys((const __iomem void *)
					   skb_frag_page(frag));*/
		} else
			tx_desc[next_to_use].word3.bits.FE = 1;

		next_to_use = (next_to_use + 1) % tx_ring->count;
		tx_buffer = &tx_ring->tx_buffer_info[next_to_use];
	}

	net_queue = netdev_get_tx_queue(ring->netdev, skb->queue_mapping);
	netdev_tx_sent_queue(net_queue, skb->len);
	txq_trans_update(net_queue);

	/*complete translate all packets*/
	wmb();
	tx_ring->next_to_use = next_to_use;

	rcb_write_tx_ring_tail(&ring->rcb_dev, buf_num);

	tx_ring->tx_pkts++;
	tx_ring->tx_bytes += pkt_len;

	log_dbg(ring->dev, "ring(%d)tx length = %d \r\n", ring_idx, pkt_len);

	return NETDEV_TX_OK;
#if 1
dma_error:
	log_err(ring->dev, "TX DMA map failed\n");

	/* clear dma mappings for failed tx_buffer_info map */
	next_to_use = tx_ring->next_to_use;
	do {
		tx_buffer = &tx_ring->tx_buffer_info[next_to_use];
		if (tx_buffer->skb) {
			dev_kfree_skb_any(tx_buffer->skb);
			if (dma_unmap_len(tx_buffer, len))
				dma_unmap_single(ring->dev,
					dma_unmap_addr(tx_buffer, dma),
					dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);
		} else if (dma_unmap_len(tx_buffer, len)) {
			dma_unmap_page(ring->dev, dma_unmap_addr(tx_buffer, dma),
				dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);
		}
		tx_buffer->buf_num = 0;
		tx_buffer->skb = NULL;
		dma_unmap_len_set(tx_buffer, len, 0);
		next_to_use = (next_to_use + 1) % tx_ring->count;
		tx_buffer = &tx_ring->tx_buffer_info[next_to_use];

	} while (--idx > 0);

	return NETDEV_TX_OK;
#endif
}

/**
 *rcb_clean_tx_irq_hw - clean rcb tx irq
 *@ring: rcb ring
 *rerurn status
 */
static bool rcb_clean_tx_irq_hw(struct nic_ring_pair *ring)
{
	struct nic_tx_ring *tx_ring = &ring->tx_ring;
	u32 head = 0;
	u16 ntc = tx_ring->next_to_clean;
	int budget = NIC_TX_CLEAN_MAX_NUM;
	struct net_device *dev = ring->netdev;
	u32 bytes_compl = 0;
	u32 pkts_compl = 0;
	struct netdev_queue *dev_queue = NULL;

	head = rcb_get_tx_ring_head_hw(&ring->rcb_dev);

	/*clock tx dev before clean tx irq*/
	rmb();

	while (likely((head != ntc) && (budget > 0))) {
		pkts_compl++;
		bytes_compl += rcb_unmap_and_free_tx_res(ring, &ntc);
		ntc++;
		ntc = (ntc >= tx_ring->count) ? 0 : ntc;
		budget--;
	}
	tx_ring->next_to_clean = ntc;

	dev_queue = netdev_get_tx_queue(dev, ring->rcb_dev.queue_index);
	if (likely(pkts_compl || bytes_compl))
		netdev_tx_completed_queue(dev_queue, pkts_compl, bytes_compl);

	if (unlikely(netif_tx_queue_stopped(dev_queue) && pkts_compl))
		netif_tx_wake_queue(dev_queue);

	return !!budget;
}

/**
 *rcb_alloc_rx_buffers - alloc rx buffers
 *@ring: rcb ring
 *@cleand_count:count
 *rerurn status
 */
static void rcb_alloc_rx_buffers(struct nic_ring_pair *ring, u32 cleand_count)
{
	struct sk_buff *skb = NULL;
	struct rcb_device *rcb_dev = &ring->rcb_dev;
	struct nic_rx_ring *rx_ring = &ring->rx_ring;
	u32 ring_id = 0;
	u32 alloc_count = 0;
	u16 idx = rx_ring->next_to_use;
	void *tmp_buffer = NULL;
	struct rcb_rx_des *rx_desc = NULL;
	struct nic_rx_buffer *rx_buffer = NULL;

	ring_id = ring->rcb_dev.index;
	log_dbg(ring->dev, "ring_id = %d \r\n", ring_id);

	rx_buffer = &rx_ring->rx_buffer_info[idx];
	rx_desc = RCB_RX_DESC(rx_ring, idx);

	idx -= rx_ring->count;

	do {
		skb = netdev_alloc_skb(ring->netdev, rcb_dev->buf_size);
		if (unlikely(NULL == skb)) {
			if ((rx_ring->rx_stats.alloc_rx_buff_failed %
			     RCB_ERROR_PRINT_CYCLE) == 0)
				log_err(ring->dev,
					"netdev_alloc_skb_ip_align fail for skb!\n");

			rx_ring->rx_stats.alloc_rx_buff_failed++;
			break;
		}

		rx_buffer->skb = skb;
		tmp_buffer = skb->data;

		tmp_buffer = (void *)virt_to_phys(tmp_buffer);
#if 1
		log_dbg(ring->dev,
			"BD %d  new skb: vir addr=0x%llx phy addr=0x%llx, buf_size=0x%x\n",
			(u16)(idx + rx_ring->count), (u64)skb->data,
			(u64)tmp_buffer, rcb_dev->buf_size);
#endif

		rx_desc->addr = (u64)tmp_buffer;
		rx_desc->word2.bits.VLD = 0;

		rx_desc++;
		rx_buffer++;
		idx++;
		if (unlikely(!idx)) {
			rx_desc = RCB_RX_DESC(rx_ring, 0);
			rx_buffer = rx_ring->rx_buffer_info;
			idx -= rx_ring->count;
		}

		alloc_count++;

	} while (cleand_count != alloc_count);

	/*alloc all skb*/
	wmb();

	idx += rx_ring->count;
	if (rx_ring->next_to_use != idx) {
		rx_ring->next_to_use = idx;
		rcb_write_rx_ring_head(rcb_dev, alloc_count);
	}

}

/**
 *rcb_recv_hw - rcb recieve pkt
 *@ring: rcb ring
 *@budget:budget for recieve ring
 *rerurn status
 */
int rcb_recv_hw(struct nic_ring_pair *ring, int budget)
{
	struct net_device *netdev = ring->netdev;
	struct sk_buff *skb = NULL;
	struct nic_device *nic_dev = netdev_priv(ring->netdev);
	struct rcb_device *rcb_dev = &ring->rcb_dev;
	struct nic_rx_ring *rx_ring = &ring->rx_ring;
	u32 ring_id = 0;
	int recv_cnt = 0;
	int real_cnt = 0;
	u32 clean_count = 0;
	u32 tmp_cnt = 0;
	u16 ntc = rx_ring->next_to_clean;
	u32 len = 0;

	struct rcb_rx_des *rx_desc = NULL;
	struct nic_rx_buffer *rx_buffer = NULL;

	ring_id = ring->rcb_dev.index;
	log_dbg(ring->dev, "ring_id = %d \r\n", ring_id);

	real_cnt = rcb_get_rx_ring_vld_bd_num_hw(rcb_dev);

	/*complete read rx ring bd number*/
	rmb();

recv:
	while ((recv_cnt < budget) && (recv_cnt < real_cnt)) {
#if 1
		if (clean_count >= RCB_NOF_ALLOC_RX_BUFF_ONCE) {
			rcb_alloc_rx_buffers(ring, clean_count);
			clean_count = 0;
		}
#endif
		rx_desc = RCB_RX_DESC(rx_ring, ntc);

		rx_buffer = &rx_ring->rx_buffer_info[ntc];

		skb = rx_buffer->skb;
		if (unlikely(NULL == skb)) {
			(void)log_crit(ring->dev,
				 "skb is NULL! next_to_clean = %d\r\n", ntc);
			rx_ring->rx_stats.rx_buff_err++;

            ntc++;
            ntc = (ntc >= rx_ring->count) ? 0 : ntc;
			recv_cnt++;
			continue;
		}
		clean_count++;

		if (unlikely(0 == rx_desc->word2.bits.VLD)) {
			(void)log_crit(ring->dev,
				 "rx BD No.%d: invalid! desc addr=0x%llx word2=%#x word3=%#x word4=%#x\n",
				 ntc, rx_desc->addr, rx_desc->word2.u_32,
				 rx_desc->word3.u_32, rx_desc->word4.u_32);

			rx_ring->rx_stats.non_vld_descs++;
            /* free the invld bd*/
            dev_kfree_skb_any(skb);
			/*updata other counter**/
            ntc++;
            ntc = (ntc >= rx_ring->count) ? 0 : ntc;
			recv_cnt++;
			continue;
		}

		len = rx_desc->word3.bits.pkt_len;
		if (unlikely((0 == len) || (len > nic_dev->mtu)
			     || (0 != rx_desc->word2.bits.DROP))) {
			if ((rx_ring->rx_stats.err_pkt_len %
			     RCB_ERROR_PRINT_CYCLE) == 0) {
				(void)log_crit(ring->dev,
					 "BD No.%d: pkt len error! desc addr=0x%llx word2=%#x word3=%#x word4=%#x\n",
					 ntc, rx_desc->addr,rx_desc->word2.u_32,
					 rx_desc->word3.u_32,
					 rx_desc->word4.u_32);
				rcb_print_pkt(skb->data, 64);
			}
			rx_ring->rx_stats.err_pkt_len++;

            /* free the invld bd*/
            dev_kfree_skb_any(skb);
			/*updata other counter**/
            ntc++;
            ntc = (ntc >= rx_ring->count) ? 0 : ntc;
			recv_cnt++;
			continue;
		}
		prefetchw(skb->data);

		rx_ring->rx_pkts++;
		rx_ring->rx_bytes += len;

		log_dbg(ring->dev, "len = %d \r\n", len);

		(void)skb_put(skb, len);
		skb->ip_summed = CHECKSUM_UNNECESSARY;

		skb_record_rx_queue(skb, rcb_dev->queue_index);
		skb->protocol = eth_type_trans(skb, netdev);
		(void)napi_gro_receive(&ring->rx_ring.napi, skb);

		/* netdev->last_rx = jiffies; TBD*/

		ntc++;
		ntc = (ntc >= rx_ring->count) ? 0 : ntc;
		recv_cnt++;
	}

	if (clean_count > 0) {
		rcb_alloc_rx_buffers(ring, clean_count);
		clean_count = 0;
	}

	if (recv_cnt < budget) {
		tmp_cnt = rcb_get_rx_ring_vld_bd_num_hw(rcb_dev);

		/*complete read rx ring bd number*/
		rmb();

		if (tmp_cnt > 0) {
			real_cnt += tmp_cnt;
			goto recv;
		}
	}

	rx_ring->next_to_clean = ntc;
	return recv_cnt;
}

/**
 *rcb_recv_hw - rcb recieve pkt
 *@ring: rcb ring
 *@budget:budget for recieve ring
 *@rx_process:receive_process fun
 *rerurn status
 */
int rcb_recv_hw_ex(struct nic_ring_pair *ring, int budget,
    int (* rx_process)(struct napi_struct *, struct sk_buff *))
{
	struct sk_buff *skb = NULL;
	struct nic_device *nic_dev = netdev_priv(ring->netdev);
	struct rcb_device *rcb_dev = &ring->rcb_dev;
	struct nic_rx_ring *rx_ring = &ring->rx_ring;
	u32 ring_id = 0;
	int recv_cnt = 0;
	int real_cnt = 0;
	u32 clean_count = 0;
	u32 tmp_cnt = 0;
	u16 ntc = rx_ring->next_to_clean;
	u32 len = 0;
	u32 rx_process_ok_cnt = 0;
	struct rcb_rx_des *rx_desc = NULL;
	struct nic_rx_buffer *rx_buffer = NULL;

	ring_id = ring->rcb_dev.index;
	log_dbg(ring->dev, "ring_id = %d \r\n", ring_id);

	real_cnt = rcb_get_rx_ring_vld_bd_num_hw(rcb_dev);

	/*complete read rx ring bd number*/
	rmb();

recv:
	while ((recv_cnt < budget) && (recv_cnt < real_cnt)) {
#if 1
		if (clean_count >= RCB_NOF_ALLOC_RX_BUFF_ONCE) {
			rcb_alloc_rx_buffers(ring, clean_count);
			clean_count = 0;
		}
#endif
		rx_desc = RCB_RX_DESC(rx_ring, ntc);

		rx_buffer = &rx_ring->rx_buffer_info[ntc];

		skb = rx_buffer->skb;
		if (unlikely(NULL == skb)) {
			(void)log_crit(ring->dev,
				 "skb is NULL! next_to_clean = %d\r\n", ntc);
			rx_ring->rx_stats.rx_buff_err++;

			ntc++;
			ntc = (ntc >= rx_ring->count) ? 0 : ntc;
			recv_cnt++;
			continue;
		}
		clean_count++;

		if (unlikely(0 == rx_desc->word2.bits.VLD)) {
			(void)log_crit(ring->dev,
				 "rx BD No.%d: invalid! desc addr=0x%llx word2=%#x word3=%#x word4=%#x\n",
				 ntc, rx_desc->addr, rx_desc->word2.u_32,
				 rx_desc->word3.u_32, rx_desc->word4.u_32);

			rx_ring->rx_stats.non_vld_descs++;
			/* free the invld bd*/
			dev_kfree_skb_any(skb);
			/*updata other counter**/
			ntc++;
			ntc = (ntc >= rx_ring->count) ? 0 : ntc;
			recv_cnt++;
			continue;
		}

		len = rx_desc->word3.bits.pkt_len;
		if (unlikely((0 == len) || (len > nic_dev->mtu)
			     || (0 != rx_desc->word2.bits.DROP))) {
			if ((rx_ring->rx_stats.err_pkt_len %
			     RCB_ERROR_PRINT_CYCLE) == 0) {
				(void)log_crit(ring->dev,
					 "BD No.%d: pkt len error! desc addr=0x%llx word2=%#x word3=%#x word4=%#x\n",
					 ntc, rx_desc->addr,rx_desc->word2.u_32,
					 rx_desc->word3.u_32,
					 rx_desc->word4.u_32);
				rcb_print_pkt(skb->data, 64);
			}
			rx_ring->rx_stats.err_pkt_len++;

			/* free the invld bd*/
			dev_kfree_skb_any(skb);
			/*updata other counter**/
			ntc++;
			ntc = (ntc >= rx_ring->count) ? 0 : ntc;
			recv_cnt++;
			continue;
	        }
		prefetchw(skb->data);

		rx_ring->rx_pkts++;
		rx_ring->rx_bytes += len;

		log_dbg(ring->dev, "len = %d \r\n", len);

		(void)skb_put(skb, len);
	        if(!rx_process(&ring->rx_ring.napi, skb))
			rx_process_ok_cnt++;

	        ntc++;
	        ntc = (ntc >= rx_ring->count) ? 0 : ntc;
		recv_cnt++;
	}

	if (clean_count > 0) {
		rcb_alloc_rx_buffers(ring, clean_count);
		clean_count = 0;
	}

	if (recv_cnt < budget) {
		tmp_cnt = rcb_get_rx_ring_vld_bd_num_hw(rcb_dev);

		/*complete read rx ring bd number*/
		rmb();

		if (tmp_cnt > 0) {
			real_cnt += tmp_cnt;
			goto recv;
		}
	}

	rx_ring->next_to_clean = ntc;

	return rx_process_ok_cnt;
}

/**
 *rcb_ring_enable_hw - enable ring
 *@ring: rcb ring
 *rerurn status
 */
static int rcb_ring_enable_hw(struct nic_ring_pair *ring)
{

	rcb_write_reg(&ring->rcb_dev, RCB_RING_PREFETCH_EN_REG, 0x1);

	return 0;
}

/**
 *rcb_ring_disable_hw - disable ring
 *@ring: rcb ring
 *rerurn status
 */
static void rcb_ring_disable_hw(struct nic_ring_pair *ring)
{
	rcb_write_reg(&ring->rcb_dev, RCB_RING_PREFETCH_EN_REG, 0x0);
}

/**
 *rcb_init_hw - init rcb
 *@ring: rcb ring
 *rerurn status
 */
static int rcb_init_hw(struct nic_ring_pair *ring)
{
	int ret = 0;
	u32 ring_id = ring->rcb_dev.index;

	log_dbg(ring->dev, "func begin ring_id = %d\n", ring_id);

	ret = rcb_rx_ring_init(ring);
	if (ret != 0) {
		log_err(ring->dev,
			"rcb_rx_ring_init failed! ring_id=%d, ret=%d\n",
			ring_id, ret);
		return ret;
	}

	ret = rcb_tx_ring_init(ring);
	if (ret != 0) {
		log_err(ring->dev,
			"rcb_tx_ring_init failed! ringid=%d, ret=%d\n",
			ring_id, ret);
		goto rcb_tx_ring_init_fail;
	}

	return ret;

rcb_tx_ring_init_fail:
	rcb_rx_ring_uninit(ring);

	return ret;
}

static void rcb_reset_ring_hw(struct nic_ring_pair *ring)
{
	u32 ring_id = ring->rcb_dev.index;
	u32 wait_cnt;
	u32 try_cnt = 0;
	u32 could_ret = 0;

	log_dbg(ring->dev, "rcb_reset_ring_hw begin ring_id = %d\n", ring_id);

	do {
		rcb_write_reg(&ring->rcb_dev, RCB_RING_T0_BE_RST, 1);

		wait_cnt = 0;
		could_ret = rcb_read_reg(&ring->rcb_dev, RCB_RING_COULD_BE_RST);
		while (!could_ret && (wait_cnt < RCB_RESET_WAIT_TIMES)){
			msleep(1);
			could_ret = rcb_read_reg(&ring->rcb_dev,
				RCB_RING_COULD_BE_RST);
			wait_cnt++;
		}

		rcb_write_reg(&ring->rcb_dev, RCB_RING_T0_BE_RST, 0);
		if (wait_cnt < RCB_RESET_WAIT_TIMES) {
			return;
		}

	} while (try_cnt++ < RCB_RESET_TRY_TIMES);

	if (try_cnt >= RCB_RESET_TRY_TIMES)
		log_err(ring->dev, "rcb_reset_ring_hw ring%d fail\n", ring_id);
}

/**
 *rcb_uninit_hw - uninit rcb
 *@ring: rcb ring
 *rerurn status
 */
static void rcb_uninit_hw(struct nic_ring_pair *ring)
{
	u32 ring_id = ring->rcb_dev.index;

	log_dbg(ring->dev, "ring_id = %d\n", ring_id);

	

	(void)rcb_tx_ring_uninit(ring);
	rcb_rx_ring_uninit(ring);
}

void rcb_update_stats(struct nic_ring_pair *ring)
{
	struct nic_ring_hw_stats *hw_stats = &ring->hw_stats;

	hw_stats->rx_pkts += rcb_read_reg(&(ring->rcb_dev),
			 RCB_RING_RX_RING_PKTNUM_RECORD_REG);

	rcb_write_reg(&(ring->rcb_dev),
			 RCB_RING_RX_RING_PKTNUM_RECORD_REG, 0x1);

	hw_stats->tx_pkts += rcb_read_reg(&(ring->rcb_dev),
			 RCB_RING_TX_RING_PKTNUM_RECORD_REG);

	rcb_write_reg(&(ring->rcb_dev),
			 RCB_RING_TX_RING_PKTNUM_RECORD_REG, 0x1);
}

/**
 *rcb_get_ethtool_stats - get rcb statistic
 *@ring: rcb ring
 *@cmd:ethtool cmd
 *@data:statistic value
 */
void rcb_get_ethtool_stats(struct nic_ring_pair *ring,
			   struct ethtool_stats *cmd, u64 *data)
{
	u64 *regs_buff = data;

	rcb_update_stats(ring);

	regs_buff[0] = ring->hw_stats.tx_pkts;

	regs_buff[1] =
	    rcb_read_reg(&(ring->rcb_dev), RCB_RING_TX_RING_FBDNUM_REG);

	regs_buff[2] =
	    rcb_read_reg(&(ring->rcb_dev), RCB_RING_TX_RING_TAIL_REG);

	regs_buff[3] =
	    rcb_read_reg(&(ring->rcb_dev), RCB_RING_TX_RING_HEAD_REG);

	regs_buff[4] = ring->tx_ring.next_to_clean;

	regs_buff[5] = ring->tx_ring.next_to_use;

	regs_buff[6] = ring->hw_stats.rx_pkts;

	regs_buff[7] =
	    rcb_read_reg(&(ring->rcb_dev), RCB_RING_RX_RING_FBDNUM_REG);

	regs_buff[8] =
	    rcb_read_reg(&(ring->rcb_dev), RCB_RING_RX_RING_TAIL_REG);

	regs_buff[9] =
	    rcb_read_reg(&(ring->rcb_dev), RCB_RING_RX_RING_HEAD_REG);

	regs_buff[10] = ring->rx_ring.next_to_clean;

	regs_buff[11] = ring->rx_ring.next_to_use;

}

/**
 *rcb_get_regs - get rcb key regs
 *@rcb_common: rcb common
 *@data:key regs value
 */
void rcb_get_regs(struct rcb_common_dev *rcb_common, void *data)
{

	u32 *regs_buff = data;
	u32 i = 0;

	/*RCB_COMMON 0 - 0x4A0  296ö */
	for (i = 0; i < 296; i++)
		regs_buff[i] = rcb_com_read_reg(rcb_common, i * 4);

	for (i = 0; i < 4; i++)
		regs_buff[296 + i] = 0x5a5a5a5a;

	/*RCB_COMMON_ENTRY 0x9000-0x 9308 194 */
	for (i = 300; i < 494; i++)
		regs_buff[i] =
		    rcb_com_read_reg(rcb_common, 0x9000 + (i - 300) * 4);

	for (i = 0; i < 6; i++)
		regs_buff[494 + i] = 0x5a5a5a5a;


}

/**
 *rcb_get_sset_count - ethtool regs count
 *@ring: rcb ring
 *@stringset:ethtool cmd
 *return regs count
 */
int rcb_get_sset_count(struct nic_ring_pair *ring, u32 stringset)
{
	if (ETH_DUMP_REG == stringset)
		return ETH_DUMP_REG_NUM;
	else if (ETH_STATIC_REG == stringset)
		return ETH_STATIC_REG_NUM;

	return 0;
}

/**
 *rcb_get_strings - get rcb key regs etring s name
 *@ring: rcb ring
 *@stringset:ethtool cmd
 *@data:strings name value
 */
void rcb_get_strings(struct nic_ring_pair *ring, u32 stringset, u8 *data)
{
	char *buff = (char*)data;
	int index = 0;

	index = ring->rcb_dev.index % 16;

	snprintf(buff, ETH_GSTRING_LEN, "TX%d_RING_PKTNUM", index);
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "TX%d_RING_FBDNUM", index);
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "TX%d_RING_TAIL", index);
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "TX%d_RING_HEAD", index);
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "TX%d_next_to_clean", index);
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "TX%d_next_to_use", index);
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "RX%d_RING_PKTNUM", index);
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "RX%d_RING_FBDNUM", index);
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "RX%d_RING_TAIL", index);
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "RX%d_RING_HEAD", index);
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "RX%d_next_to_clean", index);
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "RX%d_next_to_use", index);

}

/**
 *rcb_get_rx_coalesce_usecs - rcb get rx coalesce_usecs
 *@rcb_common: rcb rcb_common
 *@usecs:coalesce_usecs
 *return status
 */

int rcb_get_rx_coalesce_usecs(struct rcb_common_dev *rcb_common,
	u32 port_id, u32 *usecs)
{
	u32 value = 0;

	value = rcb_com_read_reg(rcb_common, RCB_CFG_OVERTIME_REG);
	*usecs = value;
	return 0;
}

/**
 *rcb_get_rx_max_coalesced_frames - rcb get rx coalesce frames
 *@rcb_common: rcb rcb_common
 *@frame_num:coalesce_frames
 *return status
 */
int rcb_get_rx_max_coalesced_frames(struct rcb_common_dev *rcb_common,
		u32 port_id, u32 *frame_num)
{
	u32 value = 0;

	if(port_id <= 5)
		value = rcb_com_read_reg(rcb_common, RCB_CFG_PKTLINE_REG + port_id * 4);
	else
		value = rcb_com_read_reg(rcb_common, RCB_CFG_PKTLINE_REG);
	*frame_num = value;
	return 0;
}

/**
 *rcb_dump_regs - dump rcb common regs
 *@rcb_common: rcb rcb_common
 *return NULL
 */
void rcb_dump_regs(struct rcb_common_dev *rcb_common)
{

	u32 i = 0;
	u32 data[4] = { 0 };

	osal_printf
	    ("******************** rcb   *********************************\n");
	for (i = 0; i < 268; i += 4) {
		data[0] = rcb_com_read_reg(rcb_common, i * 4);
		data[1] = rcb_com_read_reg(rcb_common, i * 4 + 4);
		data[2] = rcb_com_read_reg(rcb_common, i * 4 + 8);
		data[3] = rcb_com_read_reg(rcb_common, i * 4 + 12);
		osal_printf("%#18x :	 %#08x	  %#08x	 %#08x	  %#08x\n ",
			    0xc5080000 + 0x10000 * rcb_common->comm_index +
			    i * 4, data[0], data[1], data[2], data[3]);

	}
	osal_printf("\n");

	for (i = 268; i < 464; i += 4) {
		data[0] = rcb_com_read_reg(rcb_common, 0x9000 + (i - 268) * 4);
		data[1] =
		    rcb_com_read_reg(rcb_common, 0x9000 + (i + 1 - 268) * 4);
		data[2] =
		    rcb_com_read_reg(rcb_common, 0x9000 + (i + 2 - 268) * 4);
		data[3] =
		    rcb_com_read_reg(rcb_common, 0x9000 + (i + 3 - 268) * 4);
		osal_printf("%#18x :	 %#08x	  %#08x	 %#08x	  %#08x\n ",
			    0xc5080000 + 0x10000 * rcb_common->comm_index +
			    0x9000 + i * 4, data[0], data[1], data[2], data[3]);
	}
	osal_printf("\n");

}

/**
 *rcb_get_dump_regs - dump rcb ring regs
 *@ring: rcb ring
 *return NULL
 */
void rcb_get_dump_regs(struct nic_ring_pair *ring)
{
	u32 i = 0;
	u32 data[4] = { 0 };
	u32 index = 0;

	index = ring->rcb_dev.index % 16;
	osal_printf("******************** ring%d   ***************", index);
	osal_printf("******************\n");
	for (i = 0; i < 136; i += 4) {
		data[0] = rcb_read_reg(&(ring->rcb_dev), i * 4);
		data[1] = rcb_read_reg(&(ring->rcb_dev), i * 4 + 4);
		data[2] = rcb_read_reg(&(ring->rcb_dev), i * 4 + 8);
		data[3] = rcb_read_reg(&(ring->rcb_dev), i * 4 + 12);
		osal_printf("%#18x :	 %#08x	  %#08x	 %#08x	  %#08x\n ",
			    0xc5080000 + 0x10000 + 0x10000 * index + i * 4,
			    data[0], data[1], data[2], data[3]);

	}
	osal_printf("\n");

}

/**
 *rcb_set_ops - set rcb operations
 *@ops: rcb_operation
 *return NULL
 */
void rcb_set_ops(struct rcb_operation *ops)
{
	ops->init = rcb_init_hw;
	ops->uninit = rcb_uninit_hw;
	ops->reset_ring = rcb_reset_ring_hw;
	ops->pkt_send = rcb_pkt_send_hw;
	ops->enable = rcb_ring_enable_hw;
	ops->disable = rcb_ring_disable_hw;
	ops->multi_pkt_recv = rcb_recv_hw;
	ops->int_ctrl = rcb_int_ctrl_hw;
	ops->clean_tx_irq = rcb_clean_tx_irq_hw;
	ops->clean_tx_ring = rcb_clean_tx_ring;
	ops->get_tx_ring_head = rcb_get_tx_ring_head_hw;
	ops->get_rx_ring_vld_bd_num = rcb_get_rx_ring_vld_bd_num_hw;

	ops->get_sset_count = rcb_get_sset_count;
	ops->get_strings = rcb_get_strings;
	ops->get_ethtool_stats = rcb_get_ethtool_stats;
	ops->dump_reg = rcb_get_dump_regs;

}

/*****************************************************************************
****************************RCB COMMON****************************************
*****************************************************************************/

/**
 *rcb_common_init_hw - init rcb common
 *@rcb_common: rcb_common device
 *return 0-sucess,else fail
 */
static int rcb_common_init_hw(struct rcb_common_dev *rcb_common)
{
	u32 reg_val = 0;

	pr_debug("func begin\n");
	reg_val = rcb_com_read_reg(rcb_common, RCB_COM_CFG_INIT_FLAG_REG);
	if (0x1 != (reg_val & 0x1)) {
		pr_err("RCB_COM_CFG_INIT_FLAG_REG reg = 0x%x\r\n",
			reg_val);
		return HRD_COMMON_ERR_UNKNOW_DEVICE;
	}

	rcb_com_write_reg(rcb_common, RCB_COM_CFG_ENDIAN_REG,
			  RCB_COMMON_ENDIAN);

	return 0;
}

/**
 *rcb_set_port_desc_cnt - set rcb port description num
 *@rcb_common: rcb_common device
 *@port_idx:port index
 *@desc_cnt:BD num
 *return NULL
 */
static void rcb_set_port_desc_cnt(struct rcb_common_dev *rcb_common,
				  u32 port_idx, u32 desc_cnt)
{
	pr_debug("func begin, prot%d desc_cnt%d\n",	port_idx, desc_cnt);

	if (port_idx >= 6)
		port_idx = 0;

	rcb_com_write_reg(rcb_common, RCB_CFG_BD_NUM_REG + port_idx * 4,
			  desc_cnt);
}

/**
 *rcb_set_port_coalesced_frames - set rcb port coalesced frames
 *@rcb_common: rcb_common device
 *@port_idx:port index
 *@coalesced_frames:BD num for coalesced frames
 *return NULL
 */
static void rcb_set_port_coalesced_frames(struct rcb_common_dev *rcb_common,
					  u32 port_idx, u32 coalesced_frames)
{
	pr_debug("func begin port_idx = %d\n", port_idx);
	if (port_idx >= 6)
		port_idx = 0;
	rcb_com_write_reg(rcb_common, RCB_CFG_PKTLINE_REG + port_idx * 4,
			  coalesced_frames);
}

/**
 *rcb_set_port_time_out - set rcb port coalesced time_out
 *@rcb_common: rcb_common device
 *@port_idx:port index
 *@time_out:time for coalesced time_out
 *return NULL
 */
static void rcb_set_port_time_out(struct rcb_common_dev *rcb_common,
				  u32 port_idx, u32 time_out)
{
	pr_debug("func begin\n");

	rcb_com_write_reg(rcb_common, RCB_CFG_OVERTIME_REG, time_out);

}

/**
 *rcb_common_init_commit_hw - check rcb common init completed
 *@rcb_common: rcb_common device
 *return NULL
 */
static void rcb_common_init_commit_hw(struct rcb_common_dev *rcb_common)
{
	pr_debug("func begin\n");

	/*complete */
	wmb();
	rcb_com_write_reg(rcb_common, RCB_COM_CFG_SYS_FSH_REG, 1);

	/*has wrote RCB_COM_CFG_SYS_FSH_REG*/
	wmb();

}

/**
 *rcb_set_common_ops - rcb_set_common_ops
 *@ops: rcb_common_ops
 *return NULL
 */
void rcb_set_common_ops(struct rcb_common_ops *ops)
{
	ops->init = rcb_common_init_hw;
	ops->uninit = NULL;
	ops->init_commit = rcb_common_init_commit_hw;
	ops->set_port_desc_cnt = rcb_set_port_desc_cnt;
	ops->set_port_coalesced_frames = rcb_set_port_coalesced_frames;
	ops->set_port_timeout = rcb_set_port_time_out;

	ops->get_rx_coalesce_usecs = rcb_get_rx_coalesce_usecs;
	ops->get_rx_max_coalesced_frames = rcb_get_rx_max_coalesced_frames;
	ops->get_regs = rcb_get_regs;
	ops->dump_regs = rcb_dump_regs;
}
