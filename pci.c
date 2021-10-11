// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright(c) 2020  Realtek Corporation
 */

#include <linux/pci.h>

#include "mac.h"
#include "pci.h"
#include "reg.h"
#include "ser.h"

static bool rtw89_pci_disable_clkreq;
static bool rtw89_pci_disable_aspm_l1;
static bool rtw89_pci_disable_l1ss;
module_param_named(disable_clkreq, rtw89_pci_disable_clkreq, bool, 0644);
module_param_named(disable_aspm_l1, rtw89_pci_disable_aspm_l1, bool, 0644);
module_param_named(disable_aspm_l1ss, rtw89_pci_disable_l1ss, bool, 0644);
MODULE_PARM_DESC(disable_clkreq, "Set Y to disable PCI clkreq support");
MODULE_PARM_DESC(disable_aspm_l1, "Set Y to disable PCI ASPM L1 support");
MODULE_PARM_DESC(disable_aspm_l1ss, "Set Y to disable PCI L1SS support");

static int rtw89_pci_rst_bdram_pcie(struct rtw89_dev *rtwdev)
{
	u32 val;
	int ret;

	rtw89_write32(rtwdev, R_AX_PCIE_INIT_CFG1,
		      rtw89_read32(rtwdev, R_AX_PCIE_INIT_CFG1) | B_AX_RST_BDRAM);

	ret = read_poll_timeout_atomic(rtw89_read32, val, !(val & B_AX_RST_BDRAM),
				       1, RTW89_PCI_POLL_BDRAM_RST_CNT, false,
				       rtwdev, R_AX_PCIE_INIT_CFG1);

	if (ret)
		return -EBUSY;

	return 0;
}

static u32 rtw89_pci_dma_recalc(struct rtw89_dev *rtwdev,
				struct rtw89_pci_dma_ring *bd_ring,
				u32 cur_idx, bool tx)
{
	u32 cnt, cur_rp, wp, rp, len;

	rp = bd_ring->rp;
	wp = bd_ring->wp;
	len = bd_ring->len;

	cur_rp = FIELD_GET(TXBD_HW_IDX_MASK, cur_idx);
	if (tx)
		cnt = cur_rp >= rp ? cur_rp - rp : len - (rp - cur_rp);
	else
		cnt = cur_rp >= wp ? cur_rp - wp : len - (wp - cur_rp);

	bd_ring->rp = cur_rp;

	return cnt;
}

static u32 rtw89_pci_txbd_recalc(struct rtw89_dev *rtwdev,
				 struct rtw89_pci_tx_ring *tx_ring)
{
	struct rtw89_pci_dma_ring *bd_ring = &tx_ring->bd_ring;
	u32 addr_idx = bd_ring->addr_idx;
	u32 cnt, idx;

	idx = rtw89_read32(rtwdev, addr_idx);
	cnt = rtw89_pci_dma_recalc(rtwdev, bd_ring, idx, true);

	return cnt;
}

static void rtw89_pci_release_fwcmd(struct rtw89_dev *rtwdev,
				    struct rtw89_pci *rtwpci,
				    u32 cnt, bool release_all)
{
	struct rtw89_pci_tx_data *tx_data;
	struct sk_buff *skb;
	u32 qlen;

	while (cnt--) {
		skb = skb_dequeue(&rtwpci->h2c_queue);
		if (!skb) {
			rtw89_err(rtwdev, "failed to pre-release fwcmd\n");
			return;
		}
		skb_queue_tail(&rtwpci->h2c_release_queue, skb);
	}

	qlen = skb_queue_len(&rtwpci->h2c_release_queue);
	if (!release_all)
	       qlen = qlen > RTW89_PCI_MULTITAG ? qlen - RTW89_PCI_MULTITAG : 0;

	while (qlen--) {
		skb = skb_dequeue(&rtwpci->h2c_release_queue);
		if (!skb) {
			rtw89_err(rtwdev, "failed to release fwcmd\n");
			return;
		}
		tx_data = RTW89_PCI_TX_SKB_CB(skb);
		dma_unmap_single(&rtwpci->pdev->dev, tx_data->dma, skb->len,
				 DMA_TO_DEVICE);
		dev_kfree_skb_any(skb);
	}
}

static void rtw89_pci_reclaim_tx_fwcmd(struct rtw89_dev *rtwdev,
				       struct rtw89_pci *rtwpci)
{
	struct rtw89_pci_tx_ring *tx_ring = &rtwpci->tx_rings[RTW89_TXCH_CH12];
	u32 cnt;

	cnt = rtw89_pci_txbd_recalc(rtwdev, tx_ring);
	if (!cnt)
		return;
	rtw89_pci_release_fwcmd(rtwdev, rtwpci, cnt, false);
}

static u32 rtw89_pci_rxbd_recalc(struct rtw89_dev *rtwdev,
				 struct rtw89_pci_rx_ring *rx_ring)
{
	struct rtw89_pci_dma_ring *bd_ring = &rx_ring->bd_ring;
	u32 addr_idx = bd_ring->addr_idx;
	u32 cnt, idx;

	idx = rtw89_read32(rtwdev, addr_idx);
	cnt = rtw89_pci_dma_recalc(rtwdev, bd_ring, idx, false);

	return cnt;
}

static void rtw89_pci_sync_skb_for_cpu(struct rtw89_dev *rtwdev,
				       struct sk_buff *skb)
{
	struct rtw89_pci_rx_info *rx_info;
	dma_addr_t dma;

	rx_info = RTW89_PCI_RX_SKB_CB(skb);
	dma = rx_info->dma;
	dma_sync_single_for_cpu(rtwdev->dev, dma, RTW89_PCI_RX_BUF_SIZE,
				DMA_FROM_DEVICE);
}

static void rtw89_pci_sync_skb_for_device(struct rtw89_dev *rtwdev,
					  struct sk_buff *skb)
{
	struct rtw89_pci_rx_info *rx_info;
	dma_addr_t dma;

	rx_info = RTW89_PCI_RX_SKB_CB(skb);
	dma = rx_info->dma;
	dma_sync_single_for_device(rtwdev->dev, dma, RTW89_PCI_RX_BUF_SIZE,
				   DMA_FROM_DEVICE);
}

static int rtw89_pci_rxbd_info_update(struct rtw89_dev *rtwdev,
				      struct sk_buff *skb)
{
	struct rtw89_pci_rxbd_info *rxbd_info;
	struct rtw89_pci_rx_info *rx_info = RTW89_PCI_RX_SKB_CB(skb);

	rxbd_info = (struct rtw89_pci_rxbd_info *)skb->data;
	rx_info->fs = le32_get_bits(rxbd_info->dword, RTW89_PCI_RXBD_FS);
	rx_info->ls = le32_get_bits(rxbd_info->dword, RTW89_PCI_RXBD_LS);
	rx_info->len = le32_get_bits(rxbd_info->dword, RTW89_PCI_RXBD_WRITE_SIZE);
	rx_info->tag = le32_get_bits(rxbd_info->dword, RTW89_PCI_RXBD_TAG);

	return 0;
}

static bool
rtw89_skb_put_rx_data(struct rtw89_dev *rtwdev, bool fs, bool ls,
		      struct sk_buff *new,
		      const struct sk_buff *skb, u32 offset,
		      const struct rtw89_pci_rx_info *rx_info,
		      const struct rtw89_rx_desc_info *desc_info)
{
	u32 copy_len = rx_info->len - offset;

	if (unlikely(skb_tailroom(new) < copy_len)) {
		rtw89_debug(rtwdev, RTW89_DBG_TXRX,
			    "invalid rx data length bd_len=%d desc_len=%d offset=%d (fs=%d ls=%d)\n",
			    rx_info->len, desc_info->pkt_size, offset, fs, ls);
		rtw89_hex_dump(rtwdev, RTW89_DBG_TXRX, "rx_data: ",
			       skb->data, rx_info->len);
		/* length of a single segment skb is desc_info->pkt_size */
		if (fs && ls) {
			copy_len = desc_info->pkt_size;
		} else {
			rtw89_info(rtwdev, "drop rx data due to invalid length\n");
			return false;
		}
	}

	skb_put_data(new, skb->data + offset, copy_len);

	return true;
}

static u32 rtw89_pci_rxbd_deliver_skbs(struct rtw89_dev *rtwdev,
				       struct rtw89_pci_rx_ring *rx_ring)
{
	struct rtw89_pci_dma_ring *bd_ring = &rx_ring->bd_ring;
	struct rtw89_pci_rx_info *rx_info;
	struct rtw89_rx_desc_info *desc_info = &rx_ring->diliver_desc;
	struct sk_buff *new = rx_ring->diliver_skb;
	struct sk_buff *skb;
	u32 rxinfo_size = sizeof(struct rtw89_pci_rxbd_info);
	u32 offset;
	u32 cnt = 1;
	bool fs, ls;
	int ret;

	skb = rx_ring->buf[bd_ring->wp];
	rtw89_pci_sync_skb_for_cpu(rtwdev, skb);

	ret = rtw89_pci_rxbd_info_update(rtwdev, skb);
	if (ret) {
		rtw89_err(rtwdev, "failed to update %d RXBD info: %d\n",
			  bd_ring->wp, ret);
		goto err_sync_device;
	}

	rx_info = RTW89_PCI_RX_SKB_CB(skb);
	fs = rx_info->fs;
	ls = rx_info->ls;

	if (fs) {
		if (new) {
			rtw89_err(rtwdev, "skb should not be ready before first segment start\n");
			goto err_sync_device;
		}
		if (desc_info->ready) {
			rtw89_warn(rtwdev, "desc info should not be ready before first segment start\n");
			goto err_sync_device;
		}

		rtw89_core_query_rxdesc(rtwdev, desc_info, skb->data, rxinfo_size);

		new = dev_alloc_skb(desc_info->pkt_size);
		if (!new)
			goto err_sync_device;

		rx_ring->diliver_skb = new;

		/* first segment has RX desc */
		offset = desc_info->offset;
		offset += desc_info->long_rxdesc ? sizeof(struct rtw89_rxdesc_long) :
			  sizeof(struct rtw89_rxdesc_short);
	} else {
		offset = sizeof(struct rtw89_pci_rxbd_info);
		if (!new) {
			rtw89_warn(rtwdev, "no last skb\n");
			goto err_sync_device;
		}
	}
	if (!rtw89_skb_put_rx_data(rtwdev, fs, ls, new, skb, offset, rx_info, desc_info))
		goto err_sync_device;
	rtw89_pci_sync_skb_for_device(rtwdev, skb);
	rtw89_pci_rxbd_increase(rx_ring, 1);

	if (!desc_info->ready) {
		rtw89_warn(rtwdev, "no rx desc information\n");
		goto err_free_resource;
	}
	if (ls) {
		rtw89_core_rx(rtwdev, desc_info, new);
		rx_ring->diliver_skb = NULL;
		desc_info->ready = false;
	}

	return cnt;

err_sync_device:
	rtw89_pci_sync_skb_for_device(rtwdev, skb);
	rtw89_pci_rxbd_increase(rx_ring, 1);
err_free_resource:
	if (new)
		dev_kfree_skb_any(new);
	rx_ring->diliver_skb = NULL;
	desc_info->ready = false;

	return cnt;
}

static void rtw89_pci_rxbd_deliver(struct rtw89_dev *rtwdev,
				   struct rtw89_pci_rx_ring *rx_ring,
				   u32 cnt)
{
	struct rtw89_pci_dma_ring *bd_ring = &rx_ring->bd_ring;
	u32 rx_cnt;

	while (cnt && rtwdev->napi_budget_countdown > 0) {
		rx_cnt = rtw89_pci_rxbd_deliver_skbs(rtwdev, rx_ring);
		if (!rx_cnt) {
			rtw89_err(rtwdev, "failed to deliver RXBD skb\n");

			/* skip the rest RXBD bufs */
			rtw89_pci_rxbd_increase(rx_ring, cnt);
			break;
		}

		cnt -= rx_cnt;
	}

	rtw89_write16(rtwdev, bd_ring->addr_idx, bd_ring->wp);
}

static int rtw89_pci_poll_rxq_dma(struct rtw89_dev *rtwdev,
				  struct rtw89_pci *rtwpci, int budget)
{
	struct rtw89_pci_rx_ring *rx_ring;
	int countdown = rtwdev->napi_budget_countdown;
	u32 cnt;

	rx_ring = &rtwpci->rx_rings[RTW89_RXCH_RXQ];

	cnt = rtw89_pci_rxbd_recalc(rtwdev, rx_ring);
	if (!cnt)
		return 0;

	cnt = min_t(u32, budget, cnt);

	rtw89_pci_rxbd_deliver(rtwdev, rx_ring, cnt);

	/* In case of flushing pending SKBs, the countdown may exceed. */
	if (rtwdev->napi_budget_countdown <= 0)
		return budget;

	return budget - countdown;
}

static void rtw89_pci_tx_status(struct rtw89_dev *rtwdev,
				struct rtw89_pci_tx_ring *tx_ring,
				struct sk_buff *skb, u8 tx_status)
{
	struct ieee80211_tx_info *info;

	info = IEEE80211_SKB_CB(skb);
	ieee80211_tx_info_clear_status(info);

	if (info->flags & IEEE80211_TX_CTL_NO_ACK)
		info->flags |= IEEE80211_TX_STAT_NOACK_TRANSMITTED;
	if (tx_status == RTW89_TX_DONE) {
		info->flags |= IEEE80211_TX_STAT_ACK;
		tx_ring->tx_acked++;
	} else {
		if (info->flags & IEEE80211_TX_CTL_REQ_TX_STATUS)
			rtw89_debug(rtwdev, RTW89_DBG_FW,
				    "failed to TX of status %x\n", tx_status);
		switch (tx_status) {
		case RTW89_TX_RETRY_LIMIT:
			tx_ring->tx_retry_lmt++;
			break;
		case RTW89_TX_LIFE_TIME:
			tx_ring->tx_life_time++;
			break;
		case RTW89_TX_MACID_DROP:
			tx_ring->tx_mac_id_drop++;
			break;
		default:
			rtw89_warn(rtwdev, "invalid TX status %x\n", tx_status);
			break;
		}
	}

	ieee80211_tx_status_ni(rtwdev->hw, skb);
}

static void rtw89_pci_reclaim_txbd(struct rtw89_dev *rtwdev, struct rtw89_pci_tx_ring *tx_ring)
{
	struct rtw89_pci_tx_wd *txwd;
	u32 cnt;

	cnt = rtw89_pci_txbd_recalc(rtwdev, tx_ring);
	while (cnt--) {
		txwd = list_first_entry_or_null(&tx_ring->busy_pages, struct rtw89_pci_tx_wd, list);
		if (!txwd) {
			rtw89_warn(rtwdev, "No busy txwd pages available\n");
			break;
		}

		list_del_init(&txwd->list);
	}
}

static void rtw89_pci_release_busy_txwd(struct rtw89_dev *rtwdev,
					struct rtw89_pci_tx_ring *tx_ring)
{
	struct rtw89_pci_tx_wd_ring *wd_ring = &tx_ring->wd_ring;
	struct rtw89_pci_tx_wd *txwd;
	int i;

	for (i = 0; i < wd_ring->page_num; i++) {
		txwd = list_first_entry_or_null(&tx_ring->busy_pages, struct rtw89_pci_tx_wd, list);
		if (!txwd)
			break;

		list_del_init(&txwd->list);
	}
}

static void rtw89_pci_release_txwd_skb(struct rtw89_dev *rtwdev,
				       struct rtw89_pci_tx_ring *tx_ring,
				       struct rtw89_pci_tx_wd *txwd, u16 seq,
				       u8 tx_status)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	struct rtw89_pci_tx_data *tx_data;
	struct sk_buff *skb, *tmp;
	u8 txch = tx_ring->txch;

	if (!list_empty(&txwd->list)) {
		rtw89_warn(rtwdev, "queue %d txwd %d is not idle\n",
			   txch, seq);
		return;
	}

	/* currently, support for only one frame */
	if (skb_queue_len(&txwd->queue) != 1) {
		rtw89_warn(rtwdev, "empty pending queue %d page %d\n",
			   txch, seq);
		return;
	}

	skb_queue_walk_safe(&txwd->queue, skb, tmp) {
		skb_unlink(skb, &txwd->queue);

		tx_data = RTW89_PCI_TX_SKB_CB(skb);
		dma_unmap_single(&rtwpci->pdev->dev, tx_data->dma, skb->len,
				 DMA_TO_DEVICE);

		rtw89_pci_tx_status(rtwdev, tx_ring, skb, tx_status);
	}

	rtw89_pci_enqueue_txwd(tx_ring, txwd);
}

static void rtw89_pci_release_rpp(struct rtw89_dev *rtwdev,
				  struct rtw89_pci_rpp_fmt *rpp)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	struct rtw89_pci_tx_ring *tx_ring;
	struct rtw89_pci_tx_wd_ring *wd_ring;
	struct rtw89_pci_tx_wd *txwd;
	u16 seq;
	u8 qsel, tx_status, txch;

	seq = le32_get_bits(rpp->dword, RTW89_PCI_RPP_SEQ);
	qsel = le32_get_bits(rpp->dword, RTW89_PCI_RPP_QSEL);
	tx_status = le32_get_bits(rpp->dword, RTW89_PCI_RPP_TX_STATUS);
	txch = rtw89_core_get_ch_dma(rtwdev, qsel);

	if (txch == RTW89_TXCH_CH12) {
		rtw89_warn(rtwdev, "should no fwcmd release report\n");
		return;
	}

	tx_ring = &rtwpci->tx_rings[txch];
	rtw89_pci_reclaim_txbd(rtwdev, tx_ring);
	wd_ring = &tx_ring->wd_ring;
	txwd = &wd_ring->pages[seq];

	rtw89_pci_release_txwd_skb(rtwdev, tx_ring, txwd, seq, tx_status);
}

static void rtw89_pci_release_pending_txwd_skb(struct rtw89_dev *rtwdev,
					       struct rtw89_pci_tx_ring *tx_ring)
{
	struct rtw89_pci_tx_wd_ring *wd_ring = &tx_ring->wd_ring;
	struct rtw89_pci_tx_wd *txwd;
	int i;

	for (i = 0; i < wd_ring->page_num; i++) {
		txwd = &wd_ring->pages[i];

		if (!list_empty(&txwd->list))
			continue;

		rtw89_pci_release_txwd_skb(rtwdev, tx_ring, txwd, i, RTW89_TX_MACID_DROP);
	}
}

static u32 rtw89_pci_release_tx_skbs(struct rtw89_dev *rtwdev,
				     struct rtw89_pci_rx_ring *rx_ring,
				     u32 max_cnt)
{
	struct rtw89_pci_dma_ring *bd_ring = &rx_ring->bd_ring;
	struct rtw89_pci_rx_info *rx_info;
	struct rtw89_pci_rpp_fmt *rpp;
	struct rtw89_rx_desc_info desc_info = {};
	struct sk_buff *skb;
	u32 cnt = 0;
	u32 rpp_size = sizeof(struct rtw89_pci_rpp_fmt);
	u32 rxinfo_size = sizeof(struct rtw89_pci_rxbd_info);
	u32 offset;
	int ret;

	skb = rx_ring->buf[bd_ring->wp];
	rtw89_pci_sync_skb_for_cpu(rtwdev, skb);

	ret = rtw89_pci_rxbd_info_update(rtwdev, skb);
	if (ret) {
		rtw89_err(rtwdev, "failed to update %d RXBD info: %d\n",
			  bd_ring->wp, ret);
		goto err_sync_device;
	}

	rx_info = RTW89_PCI_RX_SKB_CB(skb);
	if (!rx_info->fs || !rx_info->ls) {
		rtw89_err(rtwdev, "cannot process RP frame not set FS/LS\n");
		return cnt;
	}

	rtw89_core_query_rxdesc(rtwdev, &desc_info, skb->data, rxinfo_size);

	/* first segment has RX desc */
	offset = desc_info.offset;
	offset += desc_info.long_rxdesc ? sizeof(struct rtw89_rxdesc_long) :
					  sizeof(struct rtw89_rxdesc_short);
	for (; offset + rpp_size <= rx_info->len; offset += rpp_size) {
		rpp = (struct rtw89_pci_rpp_fmt *)(skb->data + offset);
		rtw89_pci_release_rpp(rtwdev, rpp);
	}

	rtw89_pci_sync_skb_for_device(rtwdev, skb);
	rtw89_pci_rxbd_increase(rx_ring, 1);
	cnt++;

	return cnt;

err_sync_device:
	rtw89_pci_sync_skb_for_device(rtwdev, skb);
	return 0;
}

static void rtw89_pci_release_tx(struct rtw89_dev *rtwdev,
				 struct rtw89_pci_rx_ring *rx_ring,
				 u32 cnt)
{
	struct rtw89_pci_dma_ring *bd_ring = &rx_ring->bd_ring;
	u32 release_cnt;

	while (cnt) {
		release_cnt = rtw89_pci_release_tx_skbs(rtwdev, rx_ring, cnt);
		if (!release_cnt) {
			rtw89_err(rtwdev, "failed to release TX skbs\n");

			/* skip the rest RXBD bufs */
			rtw89_pci_rxbd_increase(rx_ring, cnt);
			break;
		}

		cnt -= release_cnt;
	}

	rtw89_write16(rtwdev, bd_ring->addr_idx, bd_ring->wp);
}

static int rtw89_pci_poll_rpq_dma(struct rtw89_dev *rtwdev,
				  struct rtw89_pci *rtwpci, int budget)
{
	struct rtw89_pci_rx_ring *rx_ring;
	u32 cnt;
	int work_done;

	rx_ring = &rtwpci->rx_rings[RTW89_RXCH_RPQ];

	spin_lock_bh(&rtwpci->trx_lock);

	cnt = rtw89_pci_rxbd_recalc(rtwdev, rx_ring);
	if (cnt == 0)
		goto out_unlock;

	rtw89_pci_release_tx(rtwdev, rx_ring, cnt);

out_unlock:
	spin_unlock_bh(&rtwpci->trx_lock);

	/* always release all RPQ */
	work_done = min_t(int, cnt, budget);
	rtwdev->napi_budget_countdown -= work_done;

	return work_done;
}

static void rtw89_pci_isr_rxd_unavail(struct rtw89_dev *rtwdev,
				      struct rtw89_pci *rtwpci)
{
	struct rtw89_pci_rx_ring *rx_ring;
	struct rtw89_pci_dma_ring *bd_ring;
	u32 reg_idx;
	u16 hw_idx, hw_idx_next, host_idx;
	int i;

	for (i = 0; i < RTW89_RXCH_NUM; i++) {
		rx_ring = &rtwpci->rx_rings[i];
		bd_ring = &rx_ring->bd_ring;

		reg_idx = rtw89_read32(rtwdev, bd_ring->addr_idx);
		hw_idx = FIELD_GET(TXBD_HW_IDX_MASK, reg_idx);
		host_idx = FIELD_GET(TXBD_HOST_IDX_MASK, reg_idx);
		hw_idx_next = (hw_idx + 1) % bd_ring->len;

		if (hw_idx_next == host_idx)
			rtw89_warn(rtwdev, "%d RXD unavailable\n", i);

		rtw89_debug(rtwdev, RTW89_DBG_TXRX,
			    "%d RXD unavailable, idx=0x%08x, len=%d\n",
			    i, reg_idx, bd_ring->len);
	}
}

static void rtw89_pci_recognize_intrs(struct rtw89_dev *rtwdev,
				      struct rtw89_pci *rtwpci,
				      struct rtw89_pci_isrs *isrs)
{
	isrs->halt_c2h_isrs = rtw89_read32(rtwdev, R_AX_HISR0) & rtwpci->halt_c2h_intrs;
	isrs->isrs[0] = rtw89_read32(rtwdev, R_AX_PCIE_HISR00) & rtwpci->intrs[0];
	isrs->isrs[1] = rtw89_read32(rtwdev, R_AX_PCIE_HISR10) & rtwpci->intrs[1];

	rtw89_write32(rtwdev, R_AX_HISR0, isrs->halt_c2h_isrs);
	rtw89_write32(rtwdev, R_AX_PCIE_HISR00, isrs->isrs[0]);
	rtw89_write32(rtwdev, R_AX_PCIE_HISR10, isrs->isrs[1]);
}

static void rtw89_pci_clear_isr0(struct rtw89_dev *rtwdev, u32 isr00)
{
	/* write 1 clear */
	rtw89_write32(rtwdev, R_AX_PCIE_HISR00, isr00);
}

static void rtw89_pci_enable_intr(struct rtw89_dev *rtwdev,
				  struct rtw89_pci *rtwpci)
{
	rtw89_write32(rtwdev, R_AX_HIMR0, rtwpci->halt_c2h_intrs);
	rtw89_write32(rtwdev, R_AX_PCIE_HIMR00, rtwpci->intrs[0]);
	rtw89_write32(rtwdev, R_AX_PCIE_HIMR10, rtwpci->intrs[1]);
}

static void rtw89_pci_disable_intr(struct rtw89_dev *rtwdev,
				   struct rtw89_pci *rtwpci)
{
	rtw89_write32(rtwdev, R_AX_HIMR0, 0);
	rtw89_write32(rtwdev, R_AX_PCIE_HIMR00, 0);
	rtw89_write32(rtwdev, R_AX_PCIE_HIMR10, 0);
}

static irqreturn_t rtw89_pci_interrupt_threadfn(int irq, void *dev)
{
	struct rtw89_dev *rtwdev = dev;
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	struct rtw89_pci_isrs isrs;
	unsigned long flags;

	spin_lock_irqsave(&rtwpci->irq_lock, flags);
	rtw89_pci_recognize_intrs(rtwdev, rtwpci, &isrs);
	spin_unlock_irqrestore(&rtwpci->irq_lock, flags);

	if (unlikely(isrs.isrs[0] & B_AX_RDU_INT))
		rtw89_pci_isr_rxd_unavail(rtwdev, rtwpci);

	if (unlikely(isrs.halt_c2h_isrs & B_AX_HALT_C2H_INT_EN))
		rtw89_ser_notify(rtwdev, rtw89_mac_get_err_status(rtwdev));

	if (likely(rtwpci->running)) {
		local_bh_disable();
		napi_schedule(&rtwdev->napi);
		local_bh_enable();
	}

	return IRQ_HANDLED;
}

static irqreturn_t rtw89_pci_interrupt_handler(int irq, void *dev)
{
	struct rtw89_dev *rtwdev = dev;
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	unsigned long flags;
	irqreturn_t irqret = IRQ_WAKE_THREAD;

	spin_lock_irqsave(&rtwpci->irq_lock, flags);

	/* If interrupt event is on the road, it is still trigger interrupt
	 * even we have done pci_stop() to turn off IMR.
	 */
	if (unlikely(!rtwpci->running)) {
		irqret = IRQ_HANDLED;
		goto exit;
	}

	rtw89_pci_disable_intr(rtwdev, rtwpci);
exit:
	spin_unlock_irqrestore(&rtwpci->irq_lock, flags);

	return irqret;
}

#define case_TXCHADDRS(txch) \
	case RTW89_TXCH_##txch: \
		*addr_num = R_AX_##txch##_TXBD_NUM; \
		*addr_idx = R_AX_##txch##_TXBD_IDX; \
		*addr_bdram = R_AX_##txch##_BDRAM_CTRL; \
		*addr_desa_l = R_AX_##txch##_TXBD_DESA_L; \
		*addr_desa_h = R_AX_##txch##_TXBD_DESA_H; \
		break

static int rtw89_pci_get_txch_addrs(enum rtw89_tx_channel txch,
				    u32 *addr_num,
				    u32 *addr_idx,
				    u32 *addr_bdram,
				    u32 *addr_desa_l,
				    u32 *addr_desa_h)
{
	switch (txch) {
	case_TXCHADDRS(ACH0);
	case_TXCHADDRS(ACH1);
	case_TXCHADDRS(ACH2);
	case_TXCHADDRS(ACH3);
	case_TXCHADDRS(ACH4);
	case_TXCHADDRS(ACH5);
	case_TXCHADDRS(ACH6);
	case_TXCHADDRS(ACH7);
	case_TXCHADDRS(CH8);
	case_TXCHADDRS(CH9);
	case_TXCHADDRS(CH10);
	case_TXCHADDRS(CH11);
	case_TXCHADDRS(CH12);
	default:
		return -EINVAL;
	}

	return 0;
}

#undef case_TXCHADDRS

#define case_RXCHADDRS(rxch) \
	case RTW89_RXCH_##rxch: \
		*addr_num = R_AX_##rxch##_RXBD_NUM; \
		*addr_idx = R_AX_##rxch##_RXBD_IDX; \
		*addr_desa_l = R_AX_##rxch##_RXBD_DESA_L; \
		*addr_desa_h = R_AX_##rxch##_RXBD_DESA_H; \
		break

static int rtw89_pci_get_rxch_addrs(enum rtw89_rx_channel rxch,
				    u32 *addr_num,
				    u32 *addr_idx,
				    u32 *addr_desa_l,
				    u32 *addr_desa_h)
{
	switch (rxch) {
	case_RXCHADDRS(RXQ);
	case_RXCHADDRS(RPQ);
	default:
		return -EINVAL;
	}

	return 0;
}

#undef case_RXCHADDRS

static u32 rtw89_pci_get_avail_txbd_num(struct rtw89_pci_tx_ring *ring)
{
	struct rtw89_pci_dma_ring *bd_ring = &ring->bd_ring;

	/* reserved 1 desc check ring is full or not */
	if (bd_ring->rp > bd_ring->wp)
		return bd_ring->rp - bd_ring->wp - 1;

	return bd_ring->len - (bd_ring->wp - bd_ring->rp) - 1;
}

static
u32 __rtw89_pci_check_and_reclaim_tx_fwcmd_resource(struct rtw89_dev *rtwdev)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	struct rtw89_pci_tx_ring *tx_ring = &rtwpci->tx_rings[RTW89_TXCH_CH12];
	u32 cnt;

	spin_lock_bh(&rtwpci->trx_lock);
	rtw89_pci_reclaim_tx_fwcmd(rtwdev, rtwpci);
	cnt = rtw89_pci_get_avail_txbd_num(tx_ring);
	spin_unlock_bh(&rtwpci->trx_lock);

	return cnt;
}

static u32 __rtw89_pci_check_and_reclaim_tx_resource(struct rtw89_dev *rtwdev,
						     u8 txch)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	struct rtw89_pci_tx_ring *tx_ring = &rtwpci->tx_rings[txch];
	struct rtw89_pci_tx_wd_ring *wd_ring = &tx_ring->wd_ring;
	u32 bd_cnt, wd_cnt, min_cnt = 0;
	struct rtw89_pci_rx_ring *rx_ring;
	u32 cnt;

	rx_ring = &rtwpci->rx_rings[RTW89_RXCH_RPQ];

	spin_lock_bh(&rtwpci->trx_lock);
	bd_cnt = rtw89_pci_get_avail_txbd_num(tx_ring);
	wd_cnt = wd_ring->curr_num;

	if (wd_cnt == 0 || bd_cnt == 0) {
		cnt = rtw89_pci_rxbd_recalc(rtwdev, rx_ring);
		if (!cnt)
			goto out_unlock;
		rtw89_pci_release_tx(rtwdev, rx_ring, cnt);
	}

	bd_cnt = rtw89_pci_get_avail_txbd_num(tx_ring);
	wd_cnt = wd_ring->curr_num;
	min_cnt = min(bd_cnt, wd_cnt);
	if (min_cnt == 0)
		rtw89_warn(rtwdev, "still no tx resource after reclaim\n");

out_unlock:
	spin_unlock_bh(&rtwpci->trx_lock);

	return min_cnt;
}

static u32 rtw89_pci_check_and_reclaim_tx_resource(struct rtw89_dev *rtwdev,
						   u8 txch)
{
	if (txch == RTW89_TXCH_CH12)
		return __rtw89_pci_check_and_reclaim_tx_fwcmd_resource(rtwdev);

	return __rtw89_pci_check_and_reclaim_tx_resource(rtwdev, txch);
}

static void __rtw89_pci_tx_kick_off(struct rtw89_dev *rtwdev, struct rtw89_pci_tx_ring *tx_ring)
{
	struct rtw89_pci_dma_ring *bd_ring = &tx_ring->bd_ring;
	u32 host_idx, addr;

	addr = bd_ring->addr_idx;
	host_idx = bd_ring->wp;
	rtw89_write16(rtwdev, addr, host_idx);
}

static void rtw89_pci_tx_bd_ring_update(struct rtw89_dev *rtwdev, struct rtw89_pci_tx_ring *tx_ring,
					int n_txbd)
{
	struct rtw89_pci_dma_ring *bd_ring = &tx_ring->bd_ring;
	u32 host_idx, len;

	len = bd_ring->len;
	host_idx = bd_ring->wp + n_txbd;
	host_idx = host_idx < len ? host_idx : host_idx - len;

	bd_ring->wp = host_idx;
}

static void rtw89_pci_ops_tx_kick_off(struct rtw89_dev *rtwdev, u8 txch)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	struct rtw89_pci_tx_ring *tx_ring = &rtwpci->tx_rings[txch];

	spin_lock_bh(&rtwpci->trx_lock);
	__rtw89_pci_tx_kick_off(rtwdev, tx_ring);
	spin_unlock_bh(&rtwpci->trx_lock);
}

static void __pci_flush_txch(struct rtw89_dev *rtwdev, u8 txch, bool drop)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	struct rtw89_pci_tx_ring *tx_ring = &rtwpci->tx_rings[txch];
	struct rtw89_pci_dma_ring *bd_ring = &tx_ring->bd_ring;
	u32 cur_idx, cur_rp;
	u8 i;

	/* Because the time taked by the I/O is a bit dynamic, it's hard to
	 * define a reasonable fixed total timeout to use read_poll_timeout*
	 * helper. Instead, we can ensure a reasonable polling times, so we
	 * just use for loop with udelay here.
	 */
	for (i = 0; i < 60; i++) {
		cur_idx = rtw89_read32(rtwdev, bd_ring->addr_idx);
		cur_rp = FIELD_GET(TXBD_HW_IDX_MASK, cur_idx);
		if (cur_rp == bd_ring->wp)
			return;

		udelay(1);
	}

	if (!drop)
		rtw89_info(rtwdev, "timed out to flush pci txch: %d\n", txch);
}

static void __rtw89_pci_ops_flush_txchs(struct rtw89_dev *rtwdev, u32 txchs,
					bool drop)
{
	u8 i;

	for (i = 0; i < RTW89_TXCH_NUM; i++) {
		/* It may be unnecessary to flush FWCMD queue. */
		if (i == RTW89_TXCH_CH12)
			continue;

		if (txchs & BIT(i))
			__pci_flush_txch(rtwdev, i, drop);
	}
}

static void rtw89_pci_ops_flush_queues(struct rtw89_dev *rtwdev, u32 queues,
				       bool drop)
{
	__rtw89_pci_ops_flush_txchs(rtwdev, BIT(RTW89_TXCH_NUM) - 1, drop);
}

static int rtw89_pci_txwd_submit(struct rtw89_dev *rtwdev,
				 struct rtw89_pci_tx_ring *tx_ring,
				 struct rtw89_pci_tx_wd *txwd,
				 struct rtw89_core_tx_request *tx_req)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	struct rtw89_tx_desc_info *desc_info = &tx_req->desc_info;
	struct rtw89_txwd_body *txwd_body;
	struct rtw89_txwd_info *txwd_info;
	struct rtw89_pci_tx_wp_info *txwp_info;
	struct rtw89_pci_tx_addr_info_32 *txaddr_info;
	struct pci_dev *pdev = rtwpci->pdev;
	struct sk_buff *skb = tx_req->skb;
	struct rtw89_pci_tx_data *tx_data = RTW89_PCI_TX_SKB_CB(skb);
	bool en_wd_info = desc_info->en_wd_info;
	u32 txwd_len;
	u32 txwp_len;
	u32 txaddr_info_len;
	dma_addr_t dma;
	int ret;

	rtw89_core_fill_txdesc(rtwdev, desc_info, txwd->vaddr);

	dma = dma_map_single(&pdev->dev, skb->data, skb->len, DMA_TO_DEVICE);
	if (dma_mapping_error(&pdev->dev, dma)) {
		rtw89_err(rtwdev, "failed to map skb dma data\n");
		ret = -EBUSY;
		goto err;
	}

	tx_data->dma = dma;

	txaddr_info_len = sizeof(*txaddr_info);
	txwp_len = sizeof(*txwp_info);
	txwd_len = sizeof(*txwd_body);
	txwd_len += en_wd_info ? sizeof(*txwd_info) : 0;

	txwp_info = txwd->vaddr + txwd_len;
	txwp_info->seq0 = cpu_to_le16(txwd->seq | RTW89_PCI_TXWP_VALID);
	txwp_info->seq1 = 0;
	txwp_info->seq2 = 0;
	txwp_info->seq3 = 0;

	tx_ring->tx_cnt++;
	txaddr_info = txwd->vaddr + txwd_len + txwp_len;
	txaddr_info->length = cpu_to_le16(skb->len);
	txaddr_info->option = cpu_to_le16(RTW89_PCI_ADDR_MSDU_LS |
					  RTW89_PCI_ADDR_NUM(1));
	txaddr_info->dma = cpu_to_le32(dma);

	txwd->len = txwd_len + txwp_len + txaddr_info_len;

	skb_queue_tail(&txwd->queue, skb);

	return 0;

err:
	return ret;
}

static int rtw89_pci_fwcmd_submit(struct rtw89_dev *rtwdev,
				  struct rtw89_pci_tx_ring *tx_ring,
				  struct rtw89_pci_tx_bd_32 *txbd,
				  struct rtw89_core_tx_request *tx_req)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	struct rtw89_tx_desc_info *desc_info = &tx_req->desc_info;
	struct rtw89_txwd_body *txwd_body;
	struct pci_dev *pdev = rtwpci->pdev;
	struct sk_buff *skb = tx_req->skb;
	struct rtw89_pci_tx_data *tx_data = RTW89_PCI_TX_SKB_CB(skb);
	dma_addr_t dma;

	txwd_body = (struct rtw89_txwd_body *)skb_push(skb, sizeof(*txwd_body));
	memset(txwd_body, 0, sizeof(*txwd_body));
	rtw89_core_fill_txdesc(rtwdev, desc_info, txwd_body);

	dma = dma_map_single(&pdev->dev, skb->data, skb->len, DMA_TO_DEVICE);
	if (dma_mapping_error(&pdev->dev, dma)) {
		rtw89_err(rtwdev, "failed to map fwcmd dma data\n");
		return -EBUSY;
	}

	tx_data->dma = dma;
	txbd->option = cpu_to_le16(RTW89_PCI_TXBD_OPTION_LS);
	txbd->length = cpu_to_le16(skb->len);
	txbd->dma = cpu_to_le32(tx_data->dma);
	skb_queue_tail(&rtwpci->h2c_queue, skb);

	rtw89_pci_tx_bd_ring_update(rtwdev, tx_ring, 1);

	return 0;
}

static int rtw89_pci_txbd_submit(struct rtw89_dev *rtwdev,
				 struct rtw89_pci_tx_ring *tx_ring,
				 struct rtw89_pci_tx_bd_32 *txbd,
				 struct rtw89_core_tx_request *tx_req)
{
	struct rtw89_pci_tx_wd *txwd;
	int ret;

	/* FWCMD queue doesn't have wd pages. Instead, it submits the CMD
	 * buffer with WD BODY only. So here we don't need to check the free
	 * pages of the wd ring.
	 */
	if (tx_ring->txch == RTW89_TXCH_CH12)
		return rtw89_pci_fwcmd_submit(rtwdev, tx_ring, txbd, tx_req);

	txwd = rtw89_pci_dequeue_txwd(tx_ring);
	if (!txwd) {
		rtw89_err(rtwdev, "no available TXWD\n");
		ret = -ENOSPC;
		goto err;
	}

	ret = rtw89_pci_txwd_submit(rtwdev, tx_ring, txwd, tx_req);
	if (ret) {
		rtw89_err(rtwdev, "failed to submit TXWD %d\n", txwd->seq);
		goto err_enqueue_wd;
	}

	list_add_tail(&txwd->list, &tx_ring->busy_pages);

	txbd->option = cpu_to_le16(RTW89_PCI_TXBD_OPTION_LS);
	txbd->length = cpu_to_le16(txwd->len);
	txbd->dma = cpu_to_le32(txwd->paddr);

	rtw89_pci_tx_bd_ring_update(rtwdev, tx_ring, 1);

	return 0;

err_enqueue_wd:
	rtw89_pci_enqueue_txwd(tx_ring, txwd);
err:
	return ret;
}

static int rtw89_pci_tx_write(struct rtw89_dev *rtwdev, struct rtw89_core_tx_request *tx_req,
			      u8 txch)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	struct rtw89_pci_tx_ring *tx_ring;
	struct rtw89_pci_tx_bd_32 *txbd;
	u32 n_avail_txbd;
	int ret = 0;

	/* check the tx type and dma channel for fw cmd queue */
	if ((txch == RTW89_TXCH_CH12 ||
	     tx_req->tx_type == RTW89_CORE_TX_TYPE_FWCMD) &&
	    (txch != RTW89_TXCH_CH12 ||
	     tx_req->tx_type != RTW89_CORE_TX_TYPE_FWCMD)) {
		rtw89_err(rtwdev, "only fw cmd uses dma channel 12\n");
		return -EINVAL;
	}

	tx_ring = &rtwpci->tx_rings[txch];
	spin_lock_bh(&rtwpci->trx_lock);

	n_avail_txbd = rtw89_pci_get_avail_txbd_num(tx_ring);
	if (n_avail_txbd == 0) {
		rtw89_err(rtwdev, "no available TXBD\n");
		ret = -ENOSPC;
		goto err_unlock;
	}

	txbd = rtw89_pci_get_next_txbd(tx_ring);
	ret = rtw89_pci_txbd_submit(rtwdev, tx_ring, txbd, tx_req);
	if (ret) {
		rtw89_err(rtwdev, "failed to submit TXBD\n");
		goto err_unlock;
	}

	spin_unlock_bh(&rtwpci->trx_lock);
	return 0;

err_unlock:
	spin_unlock_bh(&rtwpci->trx_lock);
	return ret;
}

static int rtw89_pci_ops_tx_write(struct rtw89_dev *rtwdev, struct rtw89_core_tx_request *tx_req)
{
	struct rtw89_tx_desc_info *desc_info = &tx_req->desc_info;
	int ret;

	ret = rtw89_pci_tx_write(rtwdev, tx_req, desc_info->ch_dma);
	if (ret) {
		rtw89_err(rtwdev, "failed to TX Queue %d\n", desc_info->ch_dma);
		return ret;
	}

	return 0;
}

static const struct rtw89_pci_bd_ram bd_ram_table[RTW89_TXCH_NUM] = {
	[RTW89_TXCH_ACH0] = {.start_idx = 0,  .max_num = 5, .min_num = 2},
	[RTW89_TXCH_ACH1] = {.start_idx = 5,  .max_num = 5, .min_num = 2},
	[RTW89_TXCH_ACH2] = {.start_idx = 10, .max_num = 5, .min_num = 2},
	[RTW89_TXCH_ACH3] = {.start_idx = 15, .max_num = 5, .min_num = 2},
	[RTW89_TXCH_ACH4] = {.start_idx = 20, .max_num = 5, .min_num = 2},
	[RTW89_TXCH_ACH5] = {.start_idx = 25, .max_num = 5, .min_num = 2},
	[RTW89_TXCH_ACH6] = {.start_idx = 30, .max_num = 5, .min_num = 2},
	[RTW89_TXCH_ACH7] = {.start_idx = 35, .max_num = 5, .min_num = 2},
	[RTW89_TXCH_CH8]  = {.start_idx = 40, .max_num = 5, .min_num = 1},
	[RTW89_TXCH_CH9]  = {.start_idx = 45, .max_num = 5, .min_num = 1},
	[RTW89_TXCH_CH10] = {.start_idx = 50, .max_num = 5, .min_num = 1},
	[RTW89_TXCH_CH11] = {.start_idx = 55, .max_num = 5, .min_num = 1},
	[RTW89_TXCH_CH12] = {.start_idx = 60, .max_num = 4, .min_num = 1},
};

static void rtw89_pci_reset_trx_rings(struct rtw89_dev *rtwdev)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	struct rtw89_pci_tx_ring *tx_ring;
	struct rtw89_pci_rx_ring *rx_ring;
	struct rtw89_pci_dma_ring *bd_ring;
	const struct rtw89_pci_bd_ram *bd_ram;
	u32 addr_num;
	u32 addr_bdram;
	u32 addr_desa_l;
	u32 val32;
	int i;

	for (i = 0; i < RTW89_TXCH_NUM; i++) {
		tx_ring = &rtwpci->tx_rings[i];
		bd_ring = &tx_ring->bd_ring;
		bd_ram = &bd_ram_table[i];
		addr_num = bd_ring->addr_num;
		addr_bdram = bd_ring->addr_bdram;
		addr_desa_l = bd_ring->addr_desa_l;
		bd_ring->wp = 0;
		bd_ring->rp = 0;

		val32 = FIELD_PREP(BDRAM_SIDX_MASK, bd_ram->start_idx) |
			FIELD_PREP(BDRAM_MAX_MASK, bd_ram->max_num) |
			FIELD_PREP(BDRAM_MIN_MASK, bd_ram->min_num);

		rtw89_write16(rtwdev, addr_num, bd_ring->len);
		rtw89_write32(rtwdev, addr_bdram, val32);
		rtw89_write32(rtwdev, addr_desa_l, bd_ring->dma);
	}

	for (i = 0; i < RTW89_RXCH_NUM; i++) {
		rx_ring = &rtwpci->rx_rings[i];
		bd_ring = &rx_ring->bd_ring;
		addr_num = bd_ring->addr_num;
		addr_desa_l = bd_ring->addr_desa_l;
		bd_ring->wp = 0;
		bd_ring->rp = 0;
		rx_ring->diliver_skb = NULL;
		rx_ring->diliver_desc.ready = false;

		rtw89_write16(rtwdev, addr_num, bd_ring->len);
		rtw89_write32(rtwdev, addr_desa_l, bd_ring->dma);
	}
}

static void rtw89_pci_release_tx_ring(struct rtw89_dev *rtwdev,
				      struct rtw89_pci_tx_ring *tx_ring)
{
	rtw89_pci_release_busy_txwd(rtwdev, tx_ring);
	rtw89_pci_release_pending_txwd_skb(rtwdev, tx_ring);
}

static void rtw89_pci_ops_reset(struct rtw89_dev *rtwdev)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	int txch;

	rtw89_pci_reset_trx_rings(rtwdev);

	spin_lock_bh(&rtwpci->trx_lock);
	for (txch = 0; txch < RTW89_TXCH_NUM; txch++) {
		if (txch == RTW89_TXCH_CH12) {
			rtw89_pci_release_fwcmd(rtwdev, rtwpci,
						skb_queue_len(&rtwpci->h2c_queue), true);
			continue;
		}
		rtw89_pci_release_tx_ring(rtwdev, &rtwpci->tx_rings[txch]);
	}
	spin_unlock_bh(&rtwpci->trx_lock);
}

static int rtw89_pci_ops_start(struct rtw89_dev *rtwdev)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	unsigned long flags;

	rtw89_core_napi_start(rtwdev);

	spin_lock_irqsave(&rtwpci->irq_lock, flags);
	rtwpci->running = true;
	rtw89_pci_enable_intr(rtwdev, rtwpci);
	spin_unlock_irqrestore(&rtwpci->irq_lock, flags);

	return 0;
}

static void rtw89_pci_ops_stop(struct rtw89_dev *rtwdev)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	struct pci_dev *pdev = rtwpci->pdev;
	unsigned long flags;

	spin_lock_irqsave(&rtwpci->irq_lock, flags);
	rtwpci->running = false;
	rtw89_pci_disable_intr(rtwdev, rtwpci);
	spin_unlock_irqrestore(&rtwpci->irq_lock, flags);

	synchronize_irq(pdev->irq);
	rtw89_core_napi_stop(rtwdev);
}

static void rtw89_pci_ops_write32(struct rtw89_dev *rtwdev, u32 addr, u32 data);

static u32 rtw89_pci_ops_read32_cmac(struct rtw89_dev *rtwdev, u32 addr)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	u32 val = readl(rtwpci->mmap + addr);
	int count;

	for (count = 0; ; count++) {
		if (val != RTW89_R32_DEAD)
			return val;
		if (count >= MAC_REG_POOL_COUNT) {
			rtw89_warn(rtwdev, "addr %#x = %#x\n", addr, val);
			return RTW89_R32_DEAD;
		}
		rtw89_pci_ops_write32(rtwdev, R_AX_CK_EN, B_AX_CMAC_ALLCKEN);
		val = readl(rtwpci->mmap + addr);
	}

	return val;
}

static u8 rtw89_pci_ops_read8(struct rtw89_dev *rtwdev, u32 addr)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	u32 addr32, val32, shift;

	if (!ACCESS_CMAC(addr))
		return readb(rtwpci->mmap + addr);

	addr32 = addr & ~0x3;
	shift = (addr & 0x3) * 8;
	val32 = rtw89_pci_ops_read32_cmac(rtwdev, addr32);
	return val32 >> shift;
}

static u16 rtw89_pci_ops_read16(struct rtw89_dev *rtwdev, u32 addr)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	u32 addr32, val32, shift;

	if (!ACCESS_CMAC(addr))
		return readw(rtwpci->mmap + addr);

	addr32 = addr & ~0x3;
	shift = (addr & 0x3) * 8;
	val32 = rtw89_pci_ops_read32_cmac(rtwdev, addr32);
	return val32 >> shift;
}

static u32 rtw89_pci_ops_read32(struct rtw89_dev *rtwdev, u32 addr)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;

	if (!ACCESS_CMAC(addr))
		return readl(rtwpci->mmap + addr);

	return rtw89_pci_ops_read32_cmac(rtwdev, addr);
}

static void rtw89_pci_ops_write8(struct rtw89_dev *rtwdev, u32 addr, u8 data)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;

	writeb(data, rtwpci->mmap + addr);
}

static void rtw89_pci_ops_write16(struct rtw89_dev *rtwdev, u32 addr, u16 data)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;

	writew(data, rtwpci->mmap + addr);
}

static void rtw89_pci_ops_write32(struct rtw89_dev *rtwdev, u32 addr, u32 data)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;

	writel(data, rtwpci->mmap + addr);
}

static void rtw89_pci_ctrl_dma_all(struct rtw89_dev *rtwdev, bool enable)
{
	if (enable) {
		rtw89_write32_set(rtwdev, R_AX_PCIE_INIT_CFG1,
				  B_AX_TXHCI_EN | B_AX_RXHCI_EN);
		rtw89_write32_clr(rtwdev, R_AX_PCIE_DMA_STOP1,
				  B_AX_STOP_PCIEIO);
	} else {
		rtw89_write32_set(rtwdev, R_AX_PCIE_DMA_STOP1,
				  B_AX_STOP_PCIEIO);
		rtw89_write32_clr(rtwdev, R_AX_PCIE_INIT_CFG1,
				  B_AX_TXHCI_EN | B_AX_RXHCI_EN);
	}
}

static int rtw89_pci_check_mdio(struct rtw89_dev *rtwdev, u8 addr, u8 speed, u16 rw_bit)
{
	u16 val;

	rtw89_write8(rtwdev, R_AX_MDIO_CFG, addr & 0x1F);

	val = rtw89_read16(rtwdev, R_AX_MDIO_CFG);
	switch (speed) {
	case PCIE_PHY_GEN1:
		if (addr < 0x20)
			val = u16_replace_bits(val, MDIO_PG0_G1, B_AX_MDIO_PHY_ADDR_MASK);
		else
			val = u16_replace_bits(val, MDIO_PG1_G1, B_AX_MDIO_PHY_ADDR_MASK);
		break;
	case PCIE_PHY_GEN2:
		if (addr < 0x20)
			val = u16_replace_bits(val, MDIO_PG0_G2, B_AX_MDIO_PHY_ADDR_MASK);
		else
			val = u16_replace_bits(val, MDIO_PG1_G2, B_AX_MDIO_PHY_ADDR_MASK);
		break;
	default:
		rtw89_err(rtwdev, "[ERR]Error Speed %d!\n", speed);
		return -EINVAL;
	};
	rtw89_write16(rtwdev, R_AX_MDIO_CFG, val);
	rtw89_write16_set(rtwdev, R_AX_MDIO_CFG, rw_bit);

	return read_poll_timeout(rtw89_read16, val, !(val & rw_bit), 10, 2000,
				 false, rtwdev, R_AX_MDIO_CFG);
}

static int
rtw89_read16_mdio(struct rtw89_dev *rtwdev, u8 addr, u8 speed, u16 *val)
{
	int ret;

	ret = rtw89_pci_check_mdio(rtwdev, addr, speed, B_AX_MDIO_RFLAG);
	if (ret) {
		rtw89_err(rtwdev, "[ERR]MDIO R16 0x%X fail ret=%d!\n", addr, ret);
		return ret;
	}
	*val = rtw89_read16(rtwdev, R_AX_MDIO_RDATA);

	return 0;
}

static int
rtw89_write16_mdio(struct rtw89_dev *rtwdev, u8 addr, u16 data, u8 speed)
{
	int ret;

	rtw89_write16(rtwdev, R_AX_MDIO_WDATA, data);
	ret = rtw89_pci_check_mdio(rtwdev, addr, speed, B_AX_MDIO_WFLAG);
	if (ret) {
		rtw89_err(rtwdev, "[ERR]MDIO W16 0x%X = %x fail ret=%d!\n", addr, data, ret);
		return ret;
	}

	return 0;
}

static int rtw89_write16_mdio_set(struct rtw89_dev *rtwdev, u8 addr, u16 mask, u8 speed)
{
	int ret;
	u16 val;

	ret = rtw89_read16_mdio(rtwdev, addr, speed, &val);
	if (ret)
		return ret;
	ret = rtw89_write16_mdio(rtwdev, addr, val | mask, speed);
	if (ret)
		return ret;

	return 0;
}

static int rtw89_write16_mdio_clr(struct rtw89_dev *rtwdev, u8 addr, u16 mask, u8 speed)
{
	int ret;
	u16 val;

	ret = rtw89_read16_mdio(rtwdev, addr, speed, &val);
	if (ret)
		return ret;
	ret = rtw89_write16_mdio(rtwdev, addr, val & ~mask, speed);
	if (ret)
		return ret;

	return 0;
}

static int rtw89_dbi_write8(struct rtw89_dev *rtwdev, u16 addr, u8 data)
{
	u16 write_addr;
	u16 remainder = addr & ~(B_AX_DBI_ADDR_MSK | B_AX_DBI_WREN_MSK);
	u8 flag;
	int ret;

	write_addr = addr & B_AX_DBI_ADDR_MSK;
	write_addr |= u16_encode_bits(BIT(remainder), B_AX_DBI_WREN_MSK);
	rtw89_write8(rtwdev, R_AX_DBI_WDATA + remainder, data);
	rtw89_write16(rtwdev, R_AX_DBI_FLAG, write_addr);
	rtw89_write8(rtwdev, R_AX_DBI_FLAG + 2, B_AX_DBI_WFLAG >> 16);

	ret = read_poll_timeout_atomic(rtw89_read8, flag, !flag, 10,
				       10 * RTW89_PCI_WR_RETRY_CNT, false,
				       rtwdev, R_AX_DBI_FLAG + 2);
	if (ret)
		WARN(flag, "failed to write to DBI register, addr=0x%04x\n",
		     addr);

	return ret;
}

static int rtw89_dbi_read8(struct rtw89_dev *rtwdev, u16 addr, u8 *value)
{
	u16 read_addr = addr & B_AX_DBI_ADDR_MSK;
	u8 flag;
	int ret;

	rtw89_write16(rtwdev, R_AX_DBI_FLAG, read_addr);
	rtw89_write8(rtwdev, R_AX_DBI_FLAG + 2, B_AX_DBI_RFLAG >> 16);

	ret = read_poll_timeout_atomic(rtw89_read8, flag, !flag, 10,
				       10 * RTW89_PCI_WR_RETRY_CNT, false,
				       rtwdev, R_AX_DBI_FLAG + 2);

	if (!ret) {
		read_addr = R_AX_DBI_RDATA + (addr & 3);
		*value = rtw89_read8(rtwdev, read_addr);
	} else {
		WARN(1, "failed to read DBI register, addr=0x%04x\n", addr);
		ret = -EIO;
	}

	return ret;
}

static int rtw89_dbi_write8_set(struct rtw89_dev *rtwdev, u16 addr, u8 bit)
{
	u8 value;
	int ret;

	ret = rtw89_dbi_read8(rtwdev, addr, &value);
	if (ret)
		return ret;

	value |= bit;
	ret = rtw89_dbi_write8(rtwdev, addr, value);

	return ret;
}

static int rtw89_dbi_write8_clr(struct rtw89_dev *rtwdev, u16 addr, u8 bit)
{
	u8 value;
	int ret;

	ret = rtw89_dbi_read8(rtwdev, addr, &value);
	if (ret)
		return ret;

	value &= ~bit;
	ret = rtw89_dbi_write8(rtwdev, addr, value);

	return ret;
}

static int
__get_target(struct rtw89_dev *rtwdev, u16 *target, enum rtw89_pcie_phy phy_rate)
{
	u16 val, tar;
	int ret;

	/* Enable counter */
	ret = rtw89_read16_mdio(rtwdev, RAC_CTRL_PPR_V1, phy_rate, &val);
	if (ret)
		return ret;
	ret = rtw89_write16_mdio(rtwdev, RAC_CTRL_PPR_V1, val & ~B_AX_CLK_CALIB_EN,
				 phy_rate);
	if (ret)
		return ret;
	ret = rtw89_write16_mdio(rtwdev, RAC_CTRL_PPR_V1, val | B_AX_CLK_CALIB_EN,
				 phy_rate);
	if (ret)
		return ret;

	fsleep(300);

	ret = rtw89_read16_mdio(rtwdev, RAC_CTRL_PPR_V1, phy_rate, &tar);
	if (ret)
		return ret;
	ret = rtw89_write16_mdio(rtwdev, RAC_CTRL_PPR_V1, val & ~B_AX_CLK_CALIB_EN,
				 phy_rate);
	if (ret)
		return ret;

	tar = tar & 0x0FFF;
	if (tar == 0 || tar == 0x0FFF) {
		rtw89_err(rtwdev, "[ERR]Get target failed.\n");
		return -EINVAL;
	}

	*target = tar;

	return 0;
}

static int rtw89_pci_auto_refclk_cal(struct rtw89_dev *rtwdev, bool autook_en)
{
	enum rtw89_pcie_phy phy_rate;
	u16 val16, mgn_set, div_set, tar;
	u8 val8, bdr_ori;
	bool l1_flag = false;
	int ret = 0;

	if ((rtwdev->chip->chip_id == RTL8852A && rtwdev->hal.cv == CHIP_CBV) ||
	    rtwdev->chip->chip_id == RTL8852C)
		return 0;

	ret = rtw89_dbi_read8(rtwdev, RTW89_PCIE_PHY_RATE, &val8);
	if (ret) {
		rtw89_err(rtwdev, "[ERR]dbi_r8_pcie %X\n", RTW89_PCIE_PHY_RATE);
		return ret;
	}

	if (FIELD_GET(RTW89_PCIE_PHY_RATE_MASK, val8) == 0x1) {
		phy_rate = PCIE_PHY_GEN1;
	} else if (FIELD_GET(RTW89_PCIE_PHY_RATE_MASK, val8) == 0x2) {
		phy_rate = PCIE_PHY_GEN2;
	} else {
		rtw89_err(rtwdev, "[ERR]PCIe PHY rate %#x not support\n", val8);
		return -EOPNOTSUPP;
	}
	/* Disable L1BD */
	ret = rtw89_dbi_read8(rtwdev, RTW89_PCIE_L1_CTRL, &bdr_ori);
	if (ret) {
		rtw89_err(rtwdev, "[ERR]dbi_r8_pcie %X\n", RTW89_PCIE_L1_CTRL);
		return ret;
	}

	if (bdr_ori & RTW89_PCIE_BIT_L1) {
		ret = rtw89_dbi_write8(rtwdev, RTW89_PCIE_L1_CTRL,
				       bdr_ori & ~RTW89_PCIE_BIT_L1);
		if (ret) {
			rtw89_err(rtwdev, "[ERR]dbi_w8_pcie %X\n", RTW89_PCIE_L1_CTRL);
			return ret;
		}
		l1_flag = true;
	}

	ret = rtw89_read16_mdio(rtwdev, RAC_CTRL_PPR_V1, phy_rate, &val16);
	if (ret) {
		rtw89_err(rtwdev, "[ERR]mdio_r16_pcie %X\n", RAC_CTRL_PPR_V1);
		goto end;
	}

	if (val16 & B_AX_CALIB_EN) {
		ret = rtw89_write16_mdio(rtwdev, RAC_CTRL_PPR_V1,
					 val16 & ~B_AX_CALIB_EN, phy_rate);
		if (ret) {
			rtw89_err(rtwdev, "[ERR]mdio_w16_pcie %X\n", RAC_CTRL_PPR_V1);
			goto end;
		}
	}

	if (!autook_en)
		goto end;
	/* Set div */
	ret = rtw89_write16_mdio_clr(rtwdev, RAC_CTRL_PPR_V1, B_AX_DIV, phy_rate);
	if (ret) {
		rtw89_err(rtwdev, "[ERR]mdio_w16_pcie %X\n", RAC_CTRL_PPR_V1);
		goto end;
	}

	/* Obtain div and margin */
	ret = __get_target(rtwdev, &tar, phy_rate);
	if (ret) {
		rtw89_err(rtwdev, "[ERR]1st get target fail %d\n", ret);
		goto end;
	}

	mgn_set = tar * INTF_INTGRA_HOSTREF_V1 / INTF_INTGRA_MINREF_V1 - tar;

	if (mgn_set >= 128) {
		div_set = 0x0003;
		mgn_set = 0x000F;
	} else if (mgn_set >= 64) {
		div_set = 0x0003;
		mgn_set >>= 3;
	} else if (mgn_set >= 32) {
		div_set = 0x0002;
		mgn_set >>= 2;
	} else if (mgn_set >= 16) {
		div_set = 0x0001;
		mgn_set >>= 1;
	} else if (mgn_set == 0) {
		rtw89_err(rtwdev, "[ERR]cal mgn is 0,tar = %d\n", tar);
		goto end;
	} else {
		div_set = 0x0000;
	}

	ret = rtw89_read16_mdio(rtwdev, RAC_CTRL_PPR_V1, phy_rate, &val16);
	if (ret) {
		rtw89_err(rtwdev, "[ERR]mdio_r16_pcie %X\n", RAC_CTRL_PPR_V1);
		goto end;
	}

	val16 |= u16_encode_bits(div_set, B_AX_DIV);

	ret = rtw89_write16_mdio(rtwdev, RAC_CTRL_PPR_V1, val16, phy_rate);
	if (ret) {
		rtw89_err(rtwdev, "[ERR]mdio_w16_pcie %X\n", RAC_CTRL_PPR_V1);
		goto end;
	}

	ret = __get_target(rtwdev, &tar, phy_rate);
	if (ret) {
		rtw89_err(rtwdev, "[ERR]2nd get target fail %d\n", ret);
		goto end;
	}

	rtw89_debug(rtwdev, RTW89_DBG_HCI, "[TRACE]target = 0x%X, div = 0x%X, margin = 0x%X\n",
		    tar, div_set, mgn_set);
	ret = rtw89_write16_mdio(rtwdev, RAC_SET_PPR_V1,
				 (tar & 0x0FFF) | (mgn_set << 12), phy_rate);
	if (ret) {
		rtw89_err(rtwdev, "[ERR]mdio_w16_pcie %X\n", RAC_SET_PPR_V1);
		goto end;
	}

	/* Enable function */
	ret = rtw89_write16_mdio_set(rtwdev, RAC_CTRL_PPR_V1, B_AX_CALIB_EN, phy_rate);
	if (ret) {
		rtw89_err(rtwdev, "[ERR]mdio_w16_pcie %X\n", RAC_CTRL_PPR_V1);
		goto end;
	}

	/* CLK delay = 0 */
	ret = rtw89_dbi_write8(rtwdev, RTW89_PCIE_CLK_CTRL, PCIE_CLKDLY_HW_0);

end:
	/* Set L1BD to ori */
	if (l1_flag) {
		ret = rtw89_dbi_write8(rtwdev, RTW89_PCIE_L1_CTRL, bdr_ori);
		if (ret) {
			rtw89_err(rtwdev, "[ERR]dbi_w8_pcie %X\n", RTW89_PCIE_L1_CTRL);
			return ret;
		}
	}

	return ret;
}

static int rtw89_pci_deglitch_setting(struct rtw89_dev *rtwdev)
{
	int ret;

	if (rtwdev->chip->chip_id != RTL8852A)
		return 0;

	ret = rtw89_write16_mdio_clr(rtwdev, RAC_ANA24, B_AX_DEGLITCH,
				     PCIE_PHY_GEN1);
	if (ret)
		return ret;
	ret = rtw89_write16_mdio_clr(rtwdev, RAC_ANA24, B_AX_DEGLITCH,
				     PCIE_PHY_GEN2);
	if (ret)
		return ret;

	return 0;
}

static void rtw89_pci_rxdma_prefth(struct rtw89_dev *rtwdev)
{
	rtw89_write32_set(rtwdev, R_AX_PCIE_INIT_CFG1, B_AX_DIS_RXDMA_PRE);
}

static void rtw89_pci_l1off_pwroff(struct rtw89_dev *rtwdev)
{
	if (rtwdev->chip->chip_id == RTL8852C)
		return;

	rtw89_write32_clr(rtwdev, R_AX_PCIE_PS_CTRL, B_AX_L1OFF_PWR_OFF_EN);
}

static u32 rtw89_pci_l2_rxen_lat(struct rtw89_dev *rtwdev)
{
	int ret;

	if (rtwdev->chip->chip_id == RTL8852C)
		return 0;

	ret = rtw89_write16_mdio_clr(rtwdev, RAC_ANA26, B_AX_RXEN,
				     PCIE_PHY_GEN1);
	if (ret)
		return ret;

	ret = rtw89_write16_mdio_clr(rtwdev, RAC_ANA26, B_AX_RXEN,
				     PCIE_PHY_GEN2);
	if (ret)
		return ret;

	return 0;
}

static void rtw89_pci_aphy_pwrcut(struct rtw89_dev *rtwdev)
{
	if (rtwdev->chip->chip_id != RTL8852A)
		return;

	rtw89_write32_clr(rtwdev, R_AX_SYS_PW_CTRL, B_AX_PSUS_OFF_CAPC_EN);
}

static void rtw89_pci_hci_ldo(struct rtw89_dev *rtwdev)
{
	if (rtwdev->chip->chip_id != RTL8852A)
		return;

	rtw89_write32_set(rtwdev, R_AX_SYS_SDIO_CTRL,
			  B_AX_PCIE_DIS_L2_CTRL_LDO_HCI);
	rtw89_write32_clr(rtwdev, R_AX_SYS_SDIO_CTRL,
			  B_AX_PCIE_DIS_WLSUS_AFT_PDN);
}

static void rtw89_pci_set_sic(struct rtw89_dev *rtwdev)
{
	if (rtwdev->chip->chip_id == RTL8852C)
		return;

	rtw89_write32_clr(rtwdev, R_AX_PCIE_EXP_CTRL,
			  B_AX_SIC_EN_FORCE_CLKREQ);
}

static void rtw89_pci_set_dbg(struct rtw89_dev *rtwdev)
{
	if (rtwdev->chip->chip_id == RTL8852C)
		return;

	rtw89_write32_set(rtwdev, R_AX_PCIE_DBG_CTRL,
			  B_AX_ASFF_FULL_NO_STK | B_AX_EN_STUCK_DBG);

	if (rtwdev->chip->chip_id == RTL8852A)
		rtw89_write32_set(rtwdev, R_AX_PCIE_EXP_CTRL,
				  B_AX_EN_CHKDSC_NO_RX_STUCK);
}

static void rtw89_pci_clr_idx_all(struct rtw89_dev *rtwdev)
{
	u32 val = B_AX_CLR_ACH0_IDX | B_AX_CLR_ACH1_IDX | B_AX_CLR_ACH2_IDX |
		  B_AX_CLR_ACH3_IDX | B_AX_CLR_CH8_IDX | B_AX_CLR_CH9_IDX |
		  B_AX_CLR_CH12_IDX;

	if (rtwdev->chip->chip_id == RTL8852A)
		val |= B_AX_CLR_ACH4_IDX | B_AX_CLR_ACH5_IDX |
		       B_AX_CLR_ACH6_IDX | B_AX_CLR_ACH7_IDX;
	/* clear DMA indexes */
	rtw89_write32_set(rtwdev, R_AX_TXBD_RWPTR_CLR1, val);
	if (rtwdev->chip->chip_id == RTL8852A)
		rtw89_write32_set(rtwdev, R_AX_TXBD_RWPTR_CLR2,
				  B_AX_CLR_CH10_IDX | B_AX_CLR_CH11_IDX);
	rtw89_write32_set(rtwdev, R_AX_RXBD_RWPTR_CLR,
			  B_AX_CLR_RXQ_IDX | B_AX_CLR_RPQ_IDX);
}

static int rtw89_pci_ops_deinit(struct rtw89_dev *rtwdev)
{
	if (rtwdev->chip->chip_id == RTL8852A) {
		/* ltr sw trigger */
		rtw89_write32_set(rtwdev, R_AX_LTR_CTRL_0, B_AX_APP_LTR_IDLE);
	}
	rtw89_pci_ctrl_dma_all(rtwdev, false);
	rtw89_pci_clr_idx_all(rtwdev);

	return 0;
}

static int rtw89_pci_ops_mac_pre_init(struct rtw89_dev *rtwdev)
{
	u32 dma_busy;
	u32 check;
	u32 lbc;
	int ret;

	rtw89_pci_rxdma_prefth(rtwdev);
	rtw89_pci_l1off_pwroff(rtwdev);
	rtw89_pci_deglitch_setting(rtwdev);
	ret = rtw89_pci_l2_rxen_lat(rtwdev);
	if (ret) {
		rtw89_err(rtwdev, "[ERR] pcie l2 rxen lat %d\n", ret);
		return ret;
	}

	rtw89_pci_aphy_pwrcut(rtwdev);
	rtw89_pci_hci_ldo(rtwdev);

	ret = rtw89_pci_auto_refclk_cal(rtwdev, false);
	if (ret) {
		rtw89_err(rtwdev, "[ERR] pcie autok fail %d\n", ret);
		return ret;
	}

	rtw89_pci_set_sic(rtwdev);
	rtw89_pci_set_dbg(rtwdev);

	if (rtwdev->chip->chip_id == RTL8852A)
		rtw89_write32_clr(rtwdev, R_AX_SYS_SDIO_CTRL,
				  B_AX_PCIE_AUXCLK_GATE);

	lbc = rtw89_read32(rtwdev, R_AX_LBC_WATCHDOG);
	lbc = u32_replace_bits(lbc, RTW89_MAC_LBC_TMR_128US, B_AX_LBC_TIMER);
	lbc |= B_AX_LBC_FLAG | B_AX_LBC_EN;
	rtw89_write32(rtwdev, R_AX_LBC_WATCHDOG, lbc);

	rtw89_write32_set(rtwdev, R_AX_PCIE_INIT_CFG1,
			  B_AX_PCIE_TXRST_KEEP_REG | B_AX_PCIE_RXRST_KEEP_REG);
	rtw89_write32_set(rtwdev, R_AX_PCIE_DMA_STOP1, B_AX_STOP_WPDMA);

	/* stop DMA activities */
	rtw89_pci_ctrl_dma_all(rtwdev, false);

	/* check PCI at idle state */
	check = B_AX_PCIEIO_BUSY | B_AX_PCIEIO_TX_BUSY | B_AX_PCIEIO_RX_BUSY;
	ret = read_poll_timeout(rtw89_read32, dma_busy, (dma_busy & check) == 0,
				100, 3000, false, rtwdev, R_AX_PCIE_DMA_BUSY1);
	if (ret) {
		rtw89_err(rtwdev, "failed to poll io busy\n");
		return ret;
	}

	rtw89_pci_clr_idx_all(rtwdev);

	/* configure TX/RX op modes */
	rtw89_write32_set(rtwdev, R_AX_PCIE_INIT_CFG1, B_AX_TX_TRUNC_MODE |
						       B_AX_RX_TRUNC_MODE);
	rtw89_write32_clr(rtwdev, R_AX_PCIE_INIT_CFG1, B_AX_RXBD_MODE);
	rtw89_write32_mask(rtwdev, R_AX_PCIE_INIT_CFG1, B_AX_PCIE_MAX_TXDMA_MASK, 7);
	rtw89_write32_mask(rtwdev, R_AX_PCIE_INIT_CFG1, B_AX_PCIE_MAX_RXDMA_MASK, 3);
	/* multi-tag mode */
	rtw89_write32_set(rtwdev, R_AX_PCIE_INIT_CFG1, B_AX_LATENCY_CONTROL);
	rtw89_write32_mask(rtwdev, R_AX_PCIE_EXP_CTRL, B_AX_MAX_TAG_NUM,
			   RTW89_MAC_TAG_NUM_8);
	rtw89_write32_mask(rtwdev, R_AX_PCIE_INIT_CFG2, B_AX_WD_ITVL_IDLE,
			   RTW89_MAC_WD_DMA_INTVL_256NS);
	rtw89_write32_mask(rtwdev, R_AX_PCIE_INIT_CFG2, B_AX_WD_ITVL_ACT,
			   RTW89_MAC_WD_DMA_INTVL_256NS);

	/* fill TRX BD indexes */
	rtw89_pci_ops_reset(rtwdev);

	ret = rtw89_pci_rst_bdram_pcie(rtwdev);
	if (ret) {
		rtw89_warn(rtwdev, "reset bdram busy\n");
		return ret;
	}

	/* enable FW CMD queue to download firmware */
	rtw89_write32_set(rtwdev, R_AX_PCIE_DMA_STOP1, B_AX_TX_STOP1_ALL);
	rtw89_write32_clr(rtwdev, R_AX_PCIE_DMA_STOP1, B_AX_STOP_CH12);
	rtw89_write32_set(rtwdev, R_AX_PCIE_DMA_STOP2, B_AX_TX_STOP2_ALL);

	/* start DMA activities */
	rtw89_pci_ctrl_dma_all(rtwdev, true);

	return 0;
}

static int rtw89_pci_ltr_set(struct rtw89_dev *rtwdev)
{
	u32 val;

	val = rtw89_read32(rtwdev, R_AX_LTR_CTRL_0);
	if (rtw89_pci_ltr_is_err_reg_val(val))
		return -EINVAL;
	val = rtw89_read32(rtwdev, R_AX_LTR_CTRL_1);
	if (rtw89_pci_ltr_is_err_reg_val(val))
		return -EINVAL;
	val = rtw89_read32(rtwdev, R_AX_LTR_IDLE_LATENCY);
	if (rtw89_pci_ltr_is_err_reg_val(val))
		return -EINVAL;
	val = rtw89_read32(rtwdev, R_AX_LTR_ACTIVE_LATENCY);
	if (rtw89_pci_ltr_is_err_reg_val(val))
		return -EINVAL;

	rtw89_write32_clr(rtwdev, R_AX_LTR_CTRL_0, B_AX_LTR_HW_EN);
	rtw89_write32_set(rtwdev, R_AX_LTR_CTRL_0, B_AX_LTR_EN);
	rtw89_write32_mask(rtwdev, R_AX_LTR_CTRL_0, B_AX_LTR_SPACE_IDX_MASK,
			   PCI_LTR_SPC_500US);
	rtw89_write32_mask(rtwdev, R_AX_LTR_CTRL_0, B_AX_LTR_IDLE_TIMER_IDX_MASK,
			   PCI_LTR_IDLE_TIMER_800US);
	rtw89_write32_mask(rtwdev, R_AX_LTR_CTRL_1, B_AX_LTR_RX0_TH_MASK, 0x28);
	rtw89_write32_mask(rtwdev, R_AX_LTR_CTRL_1, B_AX_LTR_RX1_TH_MASK, 0x28);
	rtw89_write32(rtwdev, R_AX_LTR_IDLE_LATENCY, 0x88e088e0);
	rtw89_write32(rtwdev, R_AX_LTR_ACTIVE_LATENCY, 0x880b880b);

	return 0;
}

static int rtw89_pci_ops_mac_post_init(struct rtw89_dev *rtwdev)
{
	int ret;

	ret = rtw89_pci_ltr_set(rtwdev);
	if (ret) {
		rtw89_err(rtwdev, "pci ltr set fail\n");
		return ret;
	}
	if (rtwdev->chip->chip_id == RTL8852A) {
		/* ltr sw trigger */
		rtw89_write32_set(rtwdev, R_AX_LTR_CTRL_0, B_AX_APP_LTR_ACT);
	}
	/* ADDR info 8-byte mode */
	rtw89_write32_set(rtwdev, R_AX_TX_ADDRESS_INFO_MODE_SETTING,
			  B_AX_HOST_ADDR_INFO_8B_SEL);
	rtw89_write32_clr(rtwdev, R_AX_PKTIN_SETTING, B_AX_WD_ADDR_INFO_LENGTH);

	/* enable DMA for all queues */
	rtw89_write32_clr(rtwdev, R_AX_PCIE_DMA_STOP1, B_AX_TX_STOP1_ALL);
	rtw89_write32_clr(rtwdev, R_AX_PCIE_DMA_STOP2, B_AX_TX_STOP2_ALL);

	/* Release PCI IO */
	rtw89_write32_clr(rtwdev, R_AX_PCIE_DMA_STOP1,
			  B_AX_STOP_WPDMA | B_AX_STOP_PCIEIO);

	return 0;
}

static int rtw89_pci_claim_device(struct rtw89_dev *rtwdev,
				  struct pci_dev *pdev)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	int ret;

	ret = pci_enable_device(pdev);
	if (ret) {
		rtw89_err(rtwdev, "failed to enable pci device\n");
		return ret;
	}

	pci_set_master(pdev);
	pci_set_drvdata(pdev, rtwdev->hw);

	rtwpci->pdev = pdev;

	return 0;
}

static void rtw89_pci_declaim_device(struct rtw89_dev *rtwdev,
				     struct pci_dev *pdev)
{
	pci_clear_master(pdev);
	pci_disable_device(pdev);
}

static int rtw89_pci_setup_mapping(struct rtw89_dev *rtwdev,
				   struct pci_dev *pdev)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	unsigned long resource_len;
	u8 bar_id = 2;
	int ret;

	ret = pci_request_regions(pdev, KBUILD_MODNAME);
	if (ret) {
		rtw89_err(rtwdev, "failed to request pci regions\n");
		goto err;
	}

	ret = dma_set_mask(&pdev->dev, DMA_BIT_MASK(32));
	if (ret) {
		rtw89_err(rtwdev, "failed to set dma mask to 32-bit\n");
		goto err_release_regions;
	}

	ret = dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(32));
	if (ret) {
		rtw89_err(rtwdev, "failed to set consistent dma mask to 32-bit\n");
		goto err_release_regions;
	}

	resource_len = pci_resource_len(pdev, bar_id);
	rtwpci->mmap = pci_iomap(pdev, bar_id, resource_len);
	if (!rtwpci->mmap) {
		rtw89_err(rtwdev, "failed to map pci io\n");
		ret = -EIO;
		goto err_release_regions;
	}

	return 0;

err_release_regions:
	pci_release_regions(pdev);
err:
	return ret;
}

static void rtw89_pci_clear_mapping(struct rtw89_dev *rtwdev,
				    struct pci_dev *pdev)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;

	if (rtwpci->mmap) {
		pci_iounmap(pdev, rtwpci->mmap);
		pci_release_regions(pdev);
	}
}

static void rtw89_pci_free_tx_wd_ring(struct rtw89_dev *rtwdev,
				      struct pci_dev *pdev,
				      struct rtw89_pci_tx_ring *tx_ring)
{
	struct rtw89_pci_tx_wd_ring *wd_ring = &tx_ring->wd_ring;
	u8 *head = wd_ring->head;
	dma_addr_t dma = wd_ring->dma;
	u32 page_size = wd_ring->page_size;
	u32 page_num = wd_ring->page_num;
	u32 ring_sz = page_size * page_num;

	dma_free_coherent(&pdev->dev, ring_sz, head, dma);
	wd_ring->head = NULL;
}

static void rtw89_pci_free_tx_ring(struct rtw89_dev *rtwdev,
				   struct pci_dev *pdev,
				   struct rtw89_pci_tx_ring *tx_ring)
{
	int ring_sz;
	u8 *head;
	dma_addr_t dma;

	head = tx_ring->bd_ring.head;
	dma = tx_ring->bd_ring.dma;
	ring_sz = tx_ring->bd_ring.desc_size * tx_ring->bd_ring.len;
	dma_free_coherent(&pdev->dev, ring_sz, head, dma);

	tx_ring->bd_ring.head = NULL;
}

static void rtw89_pci_free_tx_rings(struct rtw89_dev *rtwdev,
				    struct pci_dev *pdev)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	struct rtw89_pci_tx_ring *tx_ring;
	int i;

	for (i = 0; i < RTW89_TXCH_NUM; i++) {
		tx_ring = &rtwpci->tx_rings[i];
		rtw89_pci_free_tx_wd_ring(rtwdev, pdev, tx_ring);
		rtw89_pci_free_tx_ring(rtwdev, pdev, tx_ring);
	}
}

static void rtw89_pci_free_rx_ring(struct rtw89_dev *rtwdev,
				   struct pci_dev *pdev,
				   struct rtw89_pci_rx_ring *rx_ring)
{
	struct rtw89_pci_rx_info *rx_info;
	struct sk_buff *skb;
	dma_addr_t dma;
	u32 buf_sz;
	u8 *head;
	int ring_sz = rx_ring->bd_ring.desc_size * rx_ring->bd_ring.len;
	int i;

	buf_sz = rx_ring->buf_sz;
	for (i = 0; i < rx_ring->bd_ring.len; i++) {
		skb = rx_ring->buf[i];
		if (!skb)
			continue;

		rx_info = RTW89_PCI_RX_SKB_CB(skb);
		dma = rx_info->dma;
		dma_unmap_single(&pdev->dev, dma, buf_sz, DMA_FROM_DEVICE);
		dev_kfree_skb(skb);
		rx_ring->buf[i] = NULL;
	}

	head = rx_ring->bd_ring.head;
	dma = rx_ring->bd_ring.dma;
	dma_free_coherent(&pdev->dev, ring_sz, head, dma);

	rx_ring->bd_ring.head = NULL;
}

static void rtw89_pci_free_rx_rings(struct rtw89_dev *rtwdev,
				    struct pci_dev *pdev)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	struct rtw89_pci_rx_ring *rx_ring;
	int i;

	for (i = 0; i < RTW89_RXCH_NUM; i++) {
		rx_ring = &rtwpci->rx_rings[i];
		rtw89_pci_free_rx_ring(rtwdev, pdev, rx_ring);
	}
}

static void rtw89_pci_free_trx_rings(struct rtw89_dev *rtwdev,
				     struct pci_dev *pdev)
{
	rtw89_pci_free_rx_rings(rtwdev, pdev);
	rtw89_pci_free_tx_rings(rtwdev, pdev);
}

static int rtw89_pci_init_rx_bd(struct rtw89_dev *rtwdev, struct pci_dev *pdev,
				struct rtw89_pci_rx_ring *rx_ring,
				struct sk_buff *skb, int buf_sz, u32 idx)
{
	struct rtw89_pci_rx_info *rx_info;
	struct rtw89_pci_rx_bd_32 *rx_bd;
	dma_addr_t dma;

	if (!skb)
		return -EINVAL;

	dma = dma_map_single(&pdev->dev, skb->data, buf_sz, DMA_FROM_DEVICE);
	if (dma_mapping_error(&pdev->dev, dma))
		return -EBUSY;

	rx_info = RTW89_PCI_RX_SKB_CB(skb);
	rx_bd = RTW89_PCI_RX_BD(rx_ring, idx);

	memset(rx_bd, 0, sizeof(*rx_bd));
	rx_bd->buf_size = cpu_to_le16(buf_sz);
	rx_bd->dma = cpu_to_le32(dma);
	rx_info->dma = dma;

	return 0;
}

static int rtw89_pci_alloc_tx_wd_ring(struct rtw89_dev *rtwdev,
				      struct pci_dev *pdev,
				      struct rtw89_pci_tx_ring *tx_ring,
				      enum rtw89_tx_channel txch)
{
	struct rtw89_pci_tx_wd_ring *wd_ring = &tx_ring->wd_ring;
	struct rtw89_pci_tx_wd *txwd;
	dma_addr_t dma;
	dma_addr_t cur_paddr;
	u8 *head;
	u8 *cur_vaddr;
	u32 page_size = RTW89_PCI_TXWD_PAGE_SIZE;
	u32 page_num = RTW89_PCI_TXWD_NUM_MAX;
	u32 ring_sz = page_size * page_num;
	u32 page_offset;
	int i;

	/* FWCMD queue doesn't use txwd as pages */
	if (txch == RTW89_TXCH_CH12)
		return 0;

	head = dma_alloc_coherent(&pdev->dev, ring_sz, &dma, GFP_KERNEL);
	if (!head)
		return -ENOMEM;

	INIT_LIST_HEAD(&wd_ring->free_pages);
	wd_ring->head = head;
	wd_ring->dma = dma;
	wd_ring->page_size = page_size;
	wd_ring->page_num = page_num;

	page_offset = 0;
	for (i = 0; i < page_num; i++) {
		txwd = &wd_ring->pages[i];
		cur_paddr = dma + page_offset;
		cur_vaddr = head + page_offset;

		skb_queue_head_init(&txwd->queue);
		INIT_LIST_HEAD(&txwd->list);
		txwd->paddr = cur_paddr;
		txwd->vaddr = cur_vaddr;
		txwd->len = page_size;
		txwd->seq = i;
		rtw89_pci_enqueue_txwd(tx_ring, txwd);

		page_offset += page_size;
	}

	return 0;
}

static int rtw89_pci_alloc_tx_ring(struct rtw89_dev *rtwdev,
				   struct pci_dev *pdev,
				   struct rtw89_pci_tx_ring *tx_ring,
				   u32 desc_size, u32 len,
				   enum rtw89_tx_channel txch)
{
	int ring_sz = desc_size * len;
	u8 *head;
	dma_addr_t dma;
	u32 addr_num;
	u32 addr_idx;
	u32 addr_bdram;
	u32 addr_desa_l;
	u32 addr_desa_h;
	int ret;

	ret = rtw89_pci_alloc_tx_wd_ring(rtwdev, pdev, tx_ring, txch);
	if (ret) {
		rtw89_err(rtwdev, "failed to alloc txwd ring of txch %d\n", txch);
		goto err;
	}

	ret = rtw89_pci_get_txch_addrs(txch, &addr_num, &addr_idx, &addr_bdram,
				       &addr_desa_l, &addr_desa_h);
	if (ret) {
		rtw89_err(rtwdev, "failed to get address of txch %d", txch);
		goto err_free_wd_ring;
	}

	head = dma_alloc_coherent(&pdev->dev, ring_sz, &dma, GFP_KERNEL);
	if (!head) {
		ret = -ENOMEM;
		goto err_free_wd_ring;
	}

	INIT_LIST_HEAD(&tx_ring->busy_pages);
	tx_ring->bd_ring.head = head;
	tx_ring->bd_ring.dma = dma;
	tx_ring->bd_ring.len = len;
	tx_ring->bd_ring.desc_size = desc_size;
	tx_ring->bd_ring.addr_num = addr_num;
	tx_ring->bd_ring.addr_idx = addr_idx;
	tx_ring->bd_ring.addr_bdram = addr_bdram;
	tx_ring->bd_ring.addr_desa_l = addr_desa_l;
	tx_ring->bd_ring.addr_desa_h = addr_desa_h;
	tx_ring->bd_ring.wp = 0;
	tx_ring->bd_ring.rp = 0;
	tx_ring->txch = txch;

	return 0;

err_free_wd_ring:
	rtw89_pci_free_tx_wd_ring(rtwdev, pdev, tx_ring);
err:
	return ret;
}

static int rtw89_pci_alloc_tx_rings(struct rtw89_dev *rtwdev,
				    struct pci_dev *pdev)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	struct rtw89_pci_tx_ring *tx_ring;
	u32 desc_size;
	u32 len;
	u32 i, tx_allocated;
	int ret;

	for (i = 0; i < RTW89_TXCH_NUM; i++) {
		tx_ring = &rtwpci->tx_rings[i];
		desc_size = sizeof(struct rtw89_pci_tx_bd_32);
		len = RTW89_PCI_TXBD_NUM_MAX;
		ret = rtw89_pci_alloc_tx_ring(rtwdev, pdev, tx_ring,
					      desc_size, len, i);
		if (ret) {
			rtw89_err(rtwdev, "failed to alloc tx ring %d\n", i);
			goto err_free;
		}
	}

	return 0;

err_free:
	tx_allocated = i;
	for (i = 0; i < tx_allocated; i++) {
		tx_ring = &rtwpci->tx_rings[i];
		rtw89_pci_free_tx_ring(rtwdev, pdev, tx_ring);
	}

	return ret;
}

static int rtw89_pci_alloc_rx_ring(struct rtw89_dev *rtwdev,
				   struct pci_dev *pdev,
				   struct rtw89_pci_rx_ring *rx_ring,
				   u32 desc_size, u32 len, u32 rxch)
{
	struct sk_buff *skb;
	u8 *head;
	dma_addr_t dma;
	u32 addr_num;
	u32 addr_idx;
	u32 addr_desa_l;
	u32 addr_desa_h;
	int ring_sz = desc_size * len;
	int buf_sz = RTW89_PCI_RX_BUF_SIZE;
	int i, allocated;
	int ret;

	ret = rtw89_pci_get_rxch_addrs(rxch, &addr_num, &addr_idx,
				       &addr_desa_l, &addr_desa_h);
	if (ret) {
		rtw89_err(rtwdev, "failed to get address of rxch %d", rxch);
		return ret;
	}

	head = dma_alloc_coherent(&pdev->dev, ring_sz, &dma, GFP_KERNEL);
	if (!head) {
		ret = -ENOMEM;
		goto err;
	}

	rx_ring->bd_ring.head = head;
	rx_ring->bd_ring.dma = dma;
	rx_ring->bd_ring.len = len;
	rx_ring->bd_ring.desc_size = desc_size;
	rx_ring->bd_ring.addr_num = addr_num;
	rx_ring->bd_ring.addr_idx = addr_idx;
	rx_ring->bd_ring.addr_desa_l = addr_desa_l;
	rx_ring->bd_ring.addr_desa_h = addr_desa_h;
	rx_ring->bd_ring.wp = 0;
	rx_ring->bd_ring.rp = 0;
	rx_ring->buf_sz = buf_sz;
	rx_ring->diliver_skb = NULL;
	rx_ring->diliver_desc.ready = false;

	for (i = 0; i < len; i++) {
		skb = dev_alloc_skb(buf_sz);
		if (!skb) {
			ret = -ENOMEM;
			goto err_free;
		}

		memset(skb->data, 0, buf_sz);
		rx_ring->buf[i] = skb;
		ret = rtw89_pci_init_rx_bd(rtwdev, pdev, rx_ring, skb,
					   buf_sz, i);
		if (ret) {
			rtw89_err(rtwdev, "failed to init rx buf %d\n", i);
			dev_kfree_skb_any(skb);
			rx_ring->buf[i] = NULL;
			goto err_free;
		}
	}

	return 0;

err_free:
	allocated = i;
	for (i = 0; i < allocated; i++) {
		skb = rx_ring->buf[i];
		if (!skb)
			continue;
		dma = *((dma_addr_t *)skb->cb);
		dma_unmap_single(&pdev->dev, dma, buf_sz, DMA_FROM_DEVICE);
		dev_kfree_skb(skb);
		rx_ring->buf[i] = NULL;
	}

	head = rx_ring->bd_ring.head;
	dma = rx_ring->bd_ring.dma;
	dma_free_coherent(&pdev->dev, ring_sz, head, dma);

	rx_ring->bd_ring.head = NULL;
err:
	return ret;
}

static int rtw89_pci_alloc_rx_rings(struct rtw89_dev *rtwdev,
				    struct pci_dev *pdev)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	struct rtw89_pci_rx_ring *rx_ring;
	u32 desc_size;
	u32 len;
	int i, rx_allocated;
	int ret;

	for (i = 0; i < RTW89_RXCH_NUM; i++) {
		rx_ring = &rtwpci->rx_rings[i];
		desc_size = sizeof(struct rtw89_pci_rx_bd_32);
		len = RTW89_PCI_RXBD_NUM_MAX;
		ret = rtw89_pci_alloc_rx_ring(rtwdev, pdev, rx_ring,
					      desc_size, len, i);
		if (ret) {
			rtw89_err(rtwdev, "failed to alloc rx ring %d\n", i);
			goto err_free;
		}
	}

	return 0;

err_free:
	rx_allocated = i;
	for (i = 0; i < rx_allocated; i++) {
		rx_ring = &rtwpci->rx_rings[i];
		rtw89_pci_free_rx_ring(rtwdev, pdev, rx_ring);
	}

	return ret;
}

static int rtw89_pci_alloc_trx_rings(struct rtw89_dev *rtwdev,
				     struct pci_dev *pdev)
{
	int ret;

	ret = rtw89_pci_alloc_tx_rings(rtwdev, pdev);
	if (ret) {
		rtw89_err(rtwdev, "failed to alloc dma tx rings\n");
		goto err;
	}

	ret = rtw89_pci_alloc_rx_rings(rtwdev, pdev);
	if (ret) {
		rtw89_err(rtwdev, "failed to alloc dma rx rings\n");
		goto err_free_tx_rings;
	}

	return 0;

err_free_tx_rings:
	rtw89_pci_free_tx_rings(rtwdev, pdev);
err:
	return ret;
}

static void rtw89_pci_h2c_init(struct rtw89_dev *rtwdev,
			       struct rtw89_pci *rtwpci)
{
	skb_queue_head_init(&rtwpci->h2c_queue);
	skb_queue_head_init(&rtwpci->h2c_release_queue);
}

static int rtw89_pci_setup_resource(struct rtw89_dev *rtwdev,
				    struct pci_dev *pdev)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	int ret;

	ret = rtw89_pci_setup_mapping(rtwdev, pdev);
	if (ret) {
		rtw89_err(rtwdev, "failed to setup pci mapping\n");
		goto err;
	}

	ret = rtw89_pci_alloc_trx_rings(rtwdev, pdev);
	if (ret) {
		rtw89_err(rtwdev, "failed to alloc pci trx rings\n");
		goto err_pci_unmap;
	}

	rtw89_pci_h2c_init(rtwdev, rtwpci);

	spin_lock_init(&rtwpci->irq_lock);
	spin_lock_init(&rtwpci->trx_lock);

	return 0;

err_pci_unmap:
	rtw89_pci_clear_mapping(rtwdev, pdev);
err:
	return ret;
}

static void rtw89_pci_clear_resource(struct rtw89_dev *rtwdev,
				     struct pci_dev *pdev)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;

	rtw89_pci_free_trx_rings(rtwdev, pdev);
	rtw89_pci_clear_mapping(rtwdev, pdev);
	rtw89_pci_release_fwcmd(rtwdev, rtwpci,
				skb_queue_len(&rtwpci->h2c_queue), true);
}

static void rtw89_pci_default_intr_mask(struct rtw89_dev *rtwdev)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;

	rtwpci->halt_c2h_intrs = B_AX_HALT_C2H_INT_EN | 0;
	rtwpci->intrs[0] = B_AX_TXDMA_STUCK_INT_EN |
			   B_AX_RXDMA_INT_EN |
			   B_AX_RXP1DMA_INT_EN |
			   B_AX_RPQDMA_INT_EN |
			   B_AX_RXDMA_STUCK_INT_EN |
			   B_AX_RDU_INT_EN |
			   B_AX_RPQBD_FULL_INT_EN |
			   B_AX_HS0ISR_IND_INT_EN;

	rtwpci->intrs[1] = B_AX_HC10ISR_IND_INT_EN;
}

static int rtw89_pci_request_irq(struct rtw89_dev *rtwdev,
				 struct pci_dev *pdev)
{
	unsigned long flags = 0;
	int ret;

	flags |= PCI_IRQ_LEGACY | PCI_IRQ_MSI;
	ret = pci_alloc_irq_vectors(pdev, 1, 1, flags);
	if (ret < 0) {
		rtw89_err(rtwdev, "failed to alloc irq vectors, ret %d\n", ret);
		goto err;
	}

	ret = devm_request_threaded_irq(rtwdev->dev, pdev->irq,
					rtw89_pci_interrupt_handler,
					rtw89_pci_interrupt_threadfn,
					IRQF_SHARED, KBUILD_MODNAME, rtwdev);
	if (ret) {
		rtw89_err(rtwdev, "failed to request threaded irq\n");
		goto err_free_vector;
	}

	rtw89_pci_default_intr_mask(rtwdev);

	return 0;

err_free_vector:
	pci_free_irq_vectors(pdev);
err:
	return ret;
}

static void rtw89_pci_free_irq(struct rtw89_dev *rtwdev,
			       struct pci_dev *pdev)
{
	devm_free_irq(rtwdev->dev, pdev->irq, rtwdev);
	pci_free_irq_vectors(pdev);
}

static void rtw89_pci_clkreq_set(struct rtw89_dev *rtwdev, bool enable)
{
	int ret;

	if (rtw89_pci_disable_clkreq)
		return;

	ret = rtw89_dbi_write8(rtwdev, RTW89_PCIE_CLK_CTRL,
			       PCIE_CLKDLY_HW_30US);
	if (ret)
		rtw89_err(rtwdev, "failed to set CLKREQ Delay\n");

	if (enable)
		ret = rtw89_dbi_write8_set(rtwdev, RTW89_PCIE_L1_CTRL,
					   RTW89_PCIE_BIT_CLK);
	else
		ret = rtw89_dbi_write8_clr(rtwdev, RTW89_PCIE_L1_CTRL,
					   RTW89_PCIE_BIT_CLK);
	if (ret)
		rtw89_err(rtwdev, "failed to %s CLKREQ_L1, ret=%d",
			  enable ? "set" : "unset", ret);
}

static void rtw89_pci_aspm_set(struct rtw89_dev *rtwdev, bool enable)
{
	u8 value = 0;
	int ret;

	if (rtw89_pci_disable_aspm_l1)
		return;

	ret = rtw89_dbi_read8(rtwdev, RTW89_PCIE_ASPM_CTRL, &value);
	if (ret)
		rtw89_err(rtwdev, "failed to read ASPM Delay\n");

	value &= ~(RTW89_L1DLY_MASK | RTW89_L0DLY_MASK);
	value |= FIELD_PREP(RTW89_L1DLY_MASK, PCIE_L1DLY_16US) |
		 FIELD_PREP(RTW89_L0DLY_MASK, PCIE_L0SDLY_4US);

	ret = rtw89_dbi_write8(rtwdev, RTW89_PCIE_ASPM_CTRL, value);
	if (ret)
		rtw89_err(rtwdev, "failed to read ASPM Delay\n");

	if (enable)
		ret = rtw89_dbi_write8_set(rtwdev, RTW89_PCIE_L1_CTRL,
					   RTW89_PCIE_BIT_L1);
	else
		ret = rtw89_dbi_write8_clr(rtwdev, RTW89_PCIE_L1_CTRL,
					   RTW89_PCIE_BIT_L1);
	if (ret)
		rtw89_err(rtwdev, "failed to %s ASPM L1, ret=%d",
			  enable ? "set" : "unset", ret);
}

static void rtw89_pci_recalc_int_mit(struct rtw89_dev *rtwdev)
{
	struct rtw89_traffic_stats *stats = &rtwdev->stats;
	enum rtw89_tfc_lv tx_tfc_lv = stats->tx_tfc_lv;
	enum rtw89_tfc_lv rx_tfc_lv = stats->rx_tfc_lv;
	u32 val = 0;

	if (!rtwdev->scanning &&
	    (tx_tfc_lv >= RTW89_TFC_HIGH || rx_tfc_lv >= RTW89_TFC_HIGH))
		val = B_AX_RXMIT_RXP2_SEL | B_AX_RXMIT_RXP1_SEL |
		      FIELD_PREP(B_AX_RXCOUNTER_MATCH_MASK, RTW89_PCI_RXBD_NUM_MAX / 2) |
		      FIELD_PREP(B_AX_RXTIMER_UNIT_MASK, AX_RXTIMER_UNIT_64US) |
		      FIELD_PREP(B_AX_RXTIMER_MATCH_MASK, 2048 / 64);

	rtw89_write32(rtwdev, R_AX_INT_MIT_RX, val);
}

static void rtw89_pci_link_cfg(struct rtw89_dev *rtwdev)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	struct pci_dev *pdev = rtwpci->pdev;
	u16 link_ctrl;
	int ret;

	/* Though there is standard PCIE configuration space to set the
	 * link control register, but by Realtek's design, driver should
	 * check if host supports CLKREQ/ASPM to enable the HW module.
	 *
	 * These functions are implemented by two HW modules associated,
	 * one is responsible to access PCIE configuration space to
	 * follow the host settings, and another is in charge of doing
	 * CLKREQ/ASPM mechanisms, it is default disabled. Because sometimes
	 * the host does not support it, and due to some reasons or wrong
	 * settings (ex. CLKREQ# not Bi-Direction), it could lead to device
	 * loss if HW misbehaves on the link.
	 *
	 * Hence it's designed that driver should first check the PCIE
	 * configuration space is sync'ed and enabled, then driver can turn
	 * on the other module that is actually working on the mechanism.
	 */
	ret = pcie_capability_read_word(pdev, PCI_EXP_LNKCTL, &link_ctrl);
	if (ret) {
		rtw89_err(rtwdev, "failed to read PCI cap, ret=%d\n", ret);
		return;
	}

	if (link_ctrl & PCI_EXP_LNKCTL_CLKREQ_EN)
		rtw89_pci_clkreq_set(rtwdev, true);

	if (link_ctrl & PCI_EXP_LNKCTL_ASPM_L1)
		rtw89_pci_aspm_set(rtwdev, true);
}

static void rtw89_pci_l1ss_set(struct rtw89_dev *rtwdev, bool enable)
{
	int ret;

	if (enable)
		ret = rtw89_dbi_write8_set(rtwdev, RTW89_PCIE_TIMER_CTRL,
					   RTW89_PCIE_BIT_L1SUB);
	else
		ret = rtw89_dbi_write8_clr(rtwdev, RTW89_PCIE_TIMER_CTRL,
					   RTW89_PCIE_BIT_L1SUB);
	if (ret)
		rtw89_err(rtwdev, "failed to %s L1SS, ret=%d",
			  enable ? "set" : "unset", ret);
}

static void rtw89_pci_l1ss_cfg(struct rtw89_dev *rtwdev)
{
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	struct pci_dev *pdev = rtwpci->pdev;
	u32 l1ss_cap_ptr, l1ss_ctrl;

	if (rtw89_pci_disable_l1ss)
		return;

	l1ss_cap_ptr = pci_find_ext_capability(pdev, PCI_EXT_CAP_ID_L1SS);
	if (!l1ss_cap_ptr)
		return;

	pci_read_config_dword(pdev, l1ss_cap_ptr + PCI_L1SS_CTL1, &l1ss_ctrl);

	if (l1ss_ctrl & PCI_L1SS_CTL1_L1SS_MASK)
		rtw89_pci_l1ss_set(rtwdev, true);
}

static void rtw89_pci_ctrl_dma_all_pcie(struct rtw89_dev *rtwdev, u8 en)
{
	u32 val32;

	if (en == MAC_AX_FUNC_EN) {
		val32 = B_AX_STOP_PCIEIO;
		rtw89_write32_clr(rtwdev, R_AX_PCIE_DMA_STOP1, val32);

		val32 = B_AX_TXHCI_EN | B_AX_RXHCI_EN;
		rtw89_write32_set(rtwdev, R_AX_PCIE_INIT_CFG1, val32);
	} else {
		val32 = B_AX_STOP_PCIEIO;
		rtw89_write32_set(rtwdev, R_AX_PCIE_DMA_STOP1, val32);

		val32 = B_AX_TXHCI_EN | B_AX_RXHCI_EN;
		rtw89_write32_clr(rtwdev, R_AX_PCIE_INIT_CFG1, val32);
	}
}

static int rtw89_pci_poll_io_idle(struct rtw89_dev *rtwdev)
{
	int ret = 0;
	u32 sts;
	u32 busy = B_AX_PCIEIO_BUSY | B_AX_PCIEIO_TX_BUSY | B_AX_PCIEIO_RX_BUSY;

	ret = read_poll_timeout_atomic(rtw89_read32, sts, (sts & busy) == 0x0,
				       10, 1000, false, rtwdev,
				       R_AX_PCIE_DMA_BUSY1);
	if (ret) {
		rtw89_err(rtwdev, "pci dmach busy1 0x%X\n",
			  rtw89_read32(rtwdev, R_AX_PCIE_DMA_BUSY1));
		return -EINVAL;
	}
	return ret;
}

static int rtw89_pci_lv1rst_stop_dma(struct rtw89_dev *rtwdev)
{
	u32 val, dma_rst = 0;
	int ret;

	rtw89_pci_ctrl_dma_all_pcie(rtwdev, MAC_AX_FUNC_DIS);
	ret = rtw89_pci_poll_io_idle(rtwdev);
	if (ret) {
		val = rtw89_read32(rtwdev, R_AX_DBG_ERR_FLAG);
		rtw89_debug(rtwdev, RTW89_DBG_HCI,
			    "[PCIe] poll_io_idle fail, before 0x%08x: 0x%08x\n",
			    R_AX_DBG_ERR_FLAG, val);
		if (val & B_AX_TX_STUCK || val & B_AX_PCIE_TXBD_LEN0)
			dma_rst |= B_AX_HCI_TXDMA_EN;
		if (val & B_AX_RX_STUCK)
			dma_rst |= B_AX_HCI_RXDMA_EN;
		val = rtw89_read32(rtwdev, R_AX_HCI_FUNC_EN);
		rtw89_write32(rtwdev, R_AX_HCI_FUNC_EN, val & ~dma_rst);
		rtw89_write32(rtwdev, R_AX_HCI_FUNC_EN, val | dma_rst);
		ret = rtw89_pci_poll_io_idle(rtwdev);
		val = rtw89_read32(rtwdev, R_AX_DBG_ERR_FLAG);
		rtw89_debug(rtwdev, RTW89_DBG_HCI,
			    "[PCIe] poll_io_idle fail, after 0x%08x: 0x%08x\n",
			    R_AX_DBG_ERR_FLAG, val);
	}

	return ret;
}

static void rtw89_pci_ctrl_hci_dma_en(struct rtw89_dev *rtwdev, u8 en)
{
	u32 val32;

	if (en == MAC_AX_FUNC_EN) {
		val32 = B_AX_HCI_TXDMA_EN | B_AX_HCI_RXDMA_EN;
		rtw89_write32_set(rtwdev, R_AX_HCI_FUNC_EN, val32);
	} else {
		val32 = B_AX_HCI_TXDMA_EN | B_AX_HCI_RXDMA_EN;
		rtw89_write32_clr(rtwdev, R_AX_HCI_FUNC_EN, val32);
	}
}

static int rtw89_pci_rst_bdram(struct rtw89_dev *rtwdev)
{
	int ret = 0;
	u32 val32, sts;

	val32 = B_AX_RST_BDRAM;
	rtw89_write32_set(rtwdev, R_AX_PCIE_INIT_CFG1, val32);

	ret = read_poll_timeout_atomic(rtw89_read32, sts,
				       (sts & B_AX_RST_BDRAM) == 0x0, 1, 100,
				       true, rtwdev, R_AX_PCIE_INIT_CFG1);
	return ret;
}

static int rtw89_pci_lv1rst_start_dma(struct rtw89_dev *rtwdev)
{
	u32 ret;

	rtw89_pci_ctrl_hci_dma_en(rtwdev, MAC_AX_FUNC_DIS);
	rtw89_pci_ctrl_hci_dma_en(rtwdev, MAC_AX_FUNC_EN);
	rtw89_pci_clr_idx_all(rtwdev);

	ret = rtw89_pci_rst_bdram(rtwdev);
	if (ret)
		return ret;

	rtw89_pci_ctrl_dma_all_pcie(rtwdev, MAC_AX_FUNC_EN);
	return ret;
}

static int rtw89_pci_ops_mac_lv1_recovery(struct rtw89_dev *rtwdev,
					  enum rtw89_lv1_rcvy_step step)
{
	int ret;

	switch (step) {
	case RTW89_LV1_RCVY_STEP_1:
		ret = rtw89_pci_lv1rst_stop_dma(rtwdev);
		if (ret)
			rtw89_err(rtwdev, "lv1 rcvy pci stop dma fail\n");

		break;

	case RTW89_LV1_RCVY_STEP_2:
		ret = rtw89_pci_lv1rst_start_dma(rtwdev);
		if (ret)
			rtw89_err(rtwdev, "lv1 rcvy pci start dma fail\n");
		break;

	default:
		return -EINVAL;
	}

	return ret;
}

static void rtw89_pci_ops_dump_err_status(struct rtw89_dev *rtwdev)
{
	rtw89_info(rtwdev, "R_AX_RPQ_RXBD_IDX =0x%08x\n",
		   rtw89_read32(rtwdev, R_AX_RPQ_RXBD_IDX));
	rtw89_info(rtwdev, "R_AX_DBG_ERR_FLAG=0x%08x\n",
		   rtw89_read32(rtwdev, R_AX_DBG_ERR_FLAG));
	rtw89_info(rtwdev, "R_AX_LBC_WATCHDOG=0x%08x\n",
		   rtw89_read32(rtwdev, R_AX_LBC_WATCHDOG));
}

static int rtw89_pci_napi_poll(struct napi_struct *napi, int budget)
{
	struct rtw89_dev *rtwdev = container_of(napi, struct rtw89_dev, napi);
	struct rtw89_pci *rtwpci = (struct rtw89_pci *)rtwdev->priv;
	unsigned long flags;
	int work_done;

	rtwdev->napi_budget_countdown = budget;

	rtw89_pci_clear_isr0(rtwdev, B_AX_RPQDMA_INT | B_AX_RPQBD_FULL_INT);
	work_done = rtw89_pci_poll_rpq_dma(rtwdev, rtwpci, rtwdev->napi_budget_countdown);
	if (work_done == budget)
		return budget;

	rtw89_pci_clear_isr0(rtwdev, B_AX_RXP1DMA_INT | B_AX_RXDMA_INT | B_AX_RDU_INT);
	work_done += rtw89_pci_poll_rxq_dma(rtwdev, rtwpci, rtwdev->napi_budget_countdown);
	if (work_done < budget && napi_complete_done(napi, work_done)) {
		spin_lock_irqsave(&rtwpci->irq_lock, flags);
		if (likely(rtwpci->running))
			rtw89_pci_enable_intr(rtwdev, rtwpci);
		spin_unlock_irqrestore(&rtwpci->irq_lock, flags);
	}

	return work_done;
}

static int __maybe_unused rtw89_pci_suspend(struct device *dev)
{
	struct ieee80211_hw *hw = dev_get_drvdata(dev);
	struct rtw89_dev *rtwdev = hw->priv;

	rtw89_write32_clr(rtwdev, R_AX_SYS_SDIO_CTRL,
			  B_AX_PCIE_DIS_L2_CTRL_LDO_HCI);
	rtw89_write32_set(rtwdev, R_AX_RSV_CTRL, B_AX_WLOCK_1C_BIT6);
	rtw89_write32_set(rtwdev, R_AX_RSV_CTRL, B_AX_R_DIS_PRST);
	rtw89_write32_clr(rtwdev, R_AX_RSV_CTRL, B_AX_WLOCK_1C_BIT6);
	rtw89_write32_set(rtwdev, R_AX_PCIE_INIT_CFG1,
			  B_AX_PCIE_PERST_KEEP_REG | B_AX_PCIE_TRAIN_KEEP_REG);

	return 0;
}

static void rtw89_pci_l2_hci_ldo(struct rtw89_dev *rtwdev)
{
	if (rtwdev->chip->chip_id == RTL8852C)
		return;

	/* Hardware need write the reg twice to ensure the setting work */
	rtw89_dbi_write8_set(rtwdev, RTW89_PCIE_RST_MSTATE,
			     RTW89_PCIE_BIT_CFG_RST_MSTATE);
	rtw89_dbi_write8_set(rtwdev, RTW89_PCIE_RST_MSTATE,
			     RTW89_PCIE_BIT_CFG_RST_MSTATE);
}

static int __maybe_unused rtw89_pci_resume(struct device *dev)
{
	struct ieee80211_hw *hw = dev_get_drvdata(dev);
	struct rtw89_dev *rtwdev = hw->priv;

	rtw89_write32_set(rtwdev, R_AX_SYS_SDIO_CTRL,
			  B_AX_PCIE_DIS_L2_CTRL_LDO_HCI);
	rtw89_write32_set(rtwdev, R_AX_RSV_CTRL, B_AX_WLOCK_1C_BIT6);
	rtw89_write32_clr(rtwdev, R_AX_RSV_CTRL, B_AX_R_DIS_PRST);
	rtw89_write32_clr(rtwdev, R_AX_RSV_CTRL, B_AX_WLOCK_1C_BIT6);
	rtw89_write32_clr(rtwdev, R_AX_PCIE_INIT_CFG1,
			  B_AX_PCIE_PERST_KEEP_REG | B_AX_PCIE_TRAIN_KEEP_REG);
	rtw89_pci_l2_hci_ldo(rtwdev);
	rtw89_pci_link_cfg(rtwdev);
	rtw89_pci_l1ss_cfg(rtwdev);

	return 0;
}

SIMPLE_DEV_PM_OPS(rtw89_pm_ops, rtw89_pci_suspend, rtw89_pci_resume);
EXPORT_SYMBOL(rtw89_pm_ops);

static const struct rtw89_hci_ops rtw89_pci_ops = {
	.tx_write	= rtw89_pci_ops_tx_write,
	.tx_kick_off	= rtw89_pci_ops_tx_kick_off,
	.flush_queues	= rtw89_pci_ops_flush_queues,
	.reset		= rtw89_pci_ops_reset,
	.start		= rtw89_pci_ops_start,
	.stop		= rtw89_pci_ops_stop,
	.recalc_int_mit = rtw89_pci_recalc_int_mit,

	.read8		= rtw89_pci_ops_read8,
	.read16		= rtw89_pci_ops_read16,
	.read32		= rtw89_pci_ops_read32,
	.write8		= rtw89_pci_ops_write8,
	.write16	= rtw89_pci_ops_write16,
	.write32	= rtw89_pci_ops_write32,

	.mac_pre_init	= rtw89_pci_ops_mac_pre_init,
	.mac_post_init	= rtw89_pci_ops_mac_post_init,
	.deinit		= rtw89_pci_ops_deinit,

	.check_and_reclaim_tx_resource = rtw89_pci_check_and_reclaim_tx_resource,
	.mac_lv1_rcvy	= rtw89_pci_ops_mac_lv1_recovery,
	.dump_err_status = rtw89_pci_ops_dump_err_status,
	.napi_poll	= rtw89_pci_napi_poll,
};

static int rtw89_pci_probe(struct pci_dev *pdev,
			   const struct pci_device_id *id)
{
	struct ieee80211_hw *hw;
	struct rtw89_dev *rtwdev;
	int driver_data_size;
	int ret;

	driver_data_size = sizeof(struct rtw89_dev) + sizeof(struct rtw89_pci);
	hw = ieee80211_alloc_hw(driver_data_size, &rtw89_ops);
	if (!hw) {
		dev_err(&pdev->dev, "failed to allocate hw\n");
		return -ENOMEM;
	}

	rtwdev = hw->priv;
	rtwdev->hw = hw;
	rtwdev->dev = &pdev->dev;
	rtwdev->hci.ops = &rtw89_pci_ops;
	rtwdev->hci.type = RTW89_HCI_TYPE_PCIE;
	rtwdev->hci.rpwm_addr = R_AX_PCIE_HRPWM;
	rtwdev->hci.cpwm_addr = R_AX_CPWM;

	SET_IEEE80211_DEV(rtwdev->hw, &pdev->dev);

	switch (id->driver_data) {
	case RTL8852A:
		rtwdev->chip = &rtw8852a_chip_info;
		break;
	default:
		return -ENOENT;
	}

	ret = rtw89_core_init(rtwdev);
	if (ret) {
		rtw89_err(rtwdev, "failed to initialise core\n");
		goto err_release_hw;
	}

	ret = rtw89_pci_claim_device(rtwdev, pdev);
	if (ret) {
		rtw89_err(rtwdev, "failed to claim pci device\n");
		goto err_core_deinit;
	}

	ret = rtw89_pci_setup_resource(rtwdev, pdev);
	if (ret) {
		rtw89_err(rtwdev, "failed to setup pci resource\n");
		goto err_declaim_pci;
	}

	ret = rtw89_chip_info_setup(rtwdev);
	if (ret) {
		rtw89_err(rtwdev, "failed to setup chip information\n");
		goto err_clear_resource;
	}

	rtw89_pci_link_cfg(rtwdev);
	rtw89_pci_l1ss_cfg(rtwdev);

	ret = rtw89_core_register(rtwdev);
	if (ret) {
		rtw89_err(rtwdev, "failed to register core\n");
		goto err_clear_resource;
	}

	rtw89_core_napi_init(rtwdev);

	ret = rtw89_pci_request_irq(rtwdev, pdev);
	if (ret) {
		rtw89_err(rtwdev, "failed to request pci irq\n");
		goto err_unregister;
	}

	return 0;

err_unregister:
	rtw89_core_napi_deinit(rtwdev);
	rtw89_core_unregister(rtwdev);
err_clear_resource:
	rtw89_pci_clear_resource(rtwdev, pdev);
err_declaim_pci:
	rtw89_pci_declaim_device(rtwdev, pdev);
err_core_deinit:
	rtw89_core_deinit(rtwdev);
err_release_hw:
	ieee80211_free_hw(hw);

	return ret;
}

static void rtw89_pci_remove(struct pci_dev *pdev)
{
	struct ieee80211_hw *hw = pci_get_drvdata(pdev);
	struct rtw89_dev *rtwdev;

	rtwdev = hw->priv;

	rtw89_pci_free_irq(rtwdev, pdev);
	rtw89_core_napi_deinit(rtwdev);
	rtw89_core_unregister(rtwdev);
	rtw89_pci_clear_resource(rtwdev, pdev);
	rtw89_pci_declaim_device(rtwdev, pdev);
	rtw89_core_deinit(rtwdev);
	ieee80211_free_hw(hw);
}

static const struct pci_device_id rtw89_pci_id_table[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_REALTEK, 0x8852), .driver_data = RTL8852A },
	{ PCI_DEVICE(PCI_VENDOR_ID_REALTEK, 0xa85a), .driver_data = RTL8852A },
	{},
};
MODULE_DEVICE_TABLE(pci, rtw89_pci_id_table);

static struct pci_driver rtw89_pci_driver = {
	.name		= "rtw89_pci",
	.id_table	= rtw89_pci_id_table,
	.probe		= rtw89_pci_probe,
	.remove		= rtw89_pci_remove,
	.driver.pm	= &rtw89_pm_ops,
};
module_pci_driver(rtw89_pci_driver);

MODULE_AUTHOR("Realtek Corporation");
MODULE_DESCRIPTION("Realtek 802.11ax wireless PCI driver");
MODULE_LICENSE("Dual BSD/GPL");
