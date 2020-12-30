// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright(c) 2019-2020  Realtek Corporation
 */

#include "core.h"
#include "debug.h"
#include "mac.h"
#include "cam.h"
#include "phy.h"
#include "reg.h"
#include "cam.h"
#include "fw.h"
#include "coex.h"

static void rtw89_ops_tx(struct ieee80211_hw *hw,
			 struct ieee80211_tx_control *control,
			 struct sk_buff *skb)
{
	struct rtw89_dev *rtwdev = hw->priv;
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	struct ieee80211_vif *vif = info->control.vif;
	struct ieee80211_sta *sta = control->sta;
	int ret, qsel;

	ret = rtw89_core_tx_write(rtwdev, vif, sta, skb, &qsel);
	if (ret) {
		rtw89_err(rtwdev, "failed to transmit skb: %d\n", ret);
		ieee80211_free_txskb(hw, skb);
	}
	rtw89_core_tx_kick_off(rtwdev, qsel);
}

static void rtw89_ops_wake_tx_queue(struct ieee80211_hw *hw,
				    struct ieee80211_txq *txq)
{
	struct rtw89_dev *rtwdev = hw->priv;

	ieee80211_schedule_txq(hw, txq);
	tasklet_schedule(&rtwdev->txq_tasklet);
}

static int __rtw89_ops_start(struct ieee80211_hw *hw)
{
	struct rtw89_dev *rtwdev = hw->priv;
	int ret;

	rtwdev->mac.qta_mode = RTW89_QTA_SCC;
	ret = rtw89_mac_init(rtwdev);
	if (ret) {
		rtw89_err(rtwdev, "mac init fail, ret:%d\n", ret);
		return ret;
	}

	/* btc power on notify */

	/* efuse process */

	/* pre-config BB/RF, BB reset/RFC reset */
	rtw89_mac_disable_bb_rf(rtwdev);
	rtw89_mac_enable_bb_rf(rtwdev);
	rtw89_phy_init_bb_reg(rtwdev);
	rtw89_phy_init_rf_reg(rtwdev);

	rtw89_btc_ntfy_init(rtwdev, BTC_MODE_WL);

	rtw89_phy_dm_init(rtwdev);

	rtw89_mac_cfg_ppdu_status(rtwdev, RTW89_MAC_0, true);

	ret = rtw89_hci_start(rtwdev);
	if (ret) {
		rtw89_err(rtwdev, "failed to start hci\n");
		return ret;
	}

	ieee80211_queue_delayed_work(rtwdev->hw, &rtwdev->track_work,
				     RTW89_TRACK_WORK_PERIOD);

	set_bit(RTW89_FLAG_RUNNING, rtwdev->flags);

	return 0;
}

static int rtw89_ops_start(struct ieee80211_hw *hw)
{
	struct rtw89_dev *rtwdev = hw->priv;
	int ret;

	mutex_lock(&rtwdev->mutex);
	ret = __rtw89_ops_start(hw);
	mutex_unlock(&rtwdev->mutex);

	return ret;
}

static void __rtw89_ops_stop(struct ieee80211_hw *hw)
{
	struct rtw89_dev *rtwdev = hw->priv;

	clear_bit(RTW89_FLAG_RUNNING, rtwdev->flags);

	mutex_unlock(&rtwdev->mutex);

	cancel_work_sync(&rtwdev->c2h_work);
	cancel_delayed_work_sync(&rtwdev->track_work);

	mutex_lock(&rtwdev->mutex);

	rtw89_mac_flush_txq(rtwdev, BIT(rtwdev->hw->queues) - 1, true);
	rtw89_hci_deinit(rtwdev);
	rtw89_mac_pwr_off(rtwdev);
	rtw89_hci_stop(rtwdev);
	rtw89_hci_reset(rtwdev);
}

static void rtw89_ops_stop(struct ieee80211_hw *hw)
{
	struct rtw89_dev *rtwdev = hw->priv;

	mutex_lock(&rtwdev->mutex);
	__rtw89_ops_stop(hw);
	mutex_unlock(&rtwdev->mutex);
}

static int rtw89_ops_config(struct ieee80211_hw *hw, u32 changed)
{
	struct rtw89_dev *rtwdev = hw->priv;

	mutex_lock(&rtwdev->mutex);

	if (changed & IEEE80211_CONF_CHANGE_CHANNEL)
		rtw89_set_channel(rtwdev);

	mutex_unlock(&rtwdev->mutex);

	return 0;
}

static int rtw89_ops_add_interface(struct ieee80211_hw *hw,
				   struct ieee80211_vif *vif)
{
	struct rtw89_dev *rtwdev = hw->priv;
	struct rtw89_vif *rtwvif = (struct rtw89_vif *)vif->drv_priv;
	int ret = 0;

	mutex_lock(&rtwdev->mutex);

	rtw89_vif_type_mapping(vif, false);
	rtwvif->port = rtw89_core_acquire_bit_map(rtwdev->hw_port,
						  RTW89_MAX_HW_PORT_NUM);
	if (rtwvif->port == RTW89_MAX_HW_PORT_NUM) {
		ret = -ENOSPC;
		goto out;
	}

	rtwvif->bcn_hit_cond = 0;
	rtwvif->mac_idx = RTW89_MAC_0;
	rtwvif->hit_rule = 0;
	ether_addr_copy(rtwvif->mac_addr, vif->addr);

	ret = rtw89_mac_add_vif(rtwdev, rtwvif);
	if (ret) {
		rtw89_core_release_bit_map(rtwdev->hw_port, rtwvif->port);
		goto out;
	}

	rtw89_core_txq_init(rtwdev, vif->txq);

out:
	mutex_unlock(&rtwdev->mutex);

	return ret;
}

static void rtw89_ops_remove_interface(struct ieee80211_hw *hw,
				       struct ieee80211_vif *vif)
{
	struct rtw89_dev *rtwdev = hw->priv;
	struct rtw89_vif *rtwvif = (struct rtw89_vif *)vif->drv_priv;

	mutex_lock(&rtwdev->mutex);

	rtw89_mac_remove_vif(rtwdev, rtwvif);
	rtw89_core_release_bit_map(rtwdev->hw_port, rtwvif->port);

	mutex_unlock(&rtwdev->mutex);
}

static void rtw89_ops_configure_filter(struct ieee80211_hw *hw,
				       unsigned int changed_flags,
				       unsigned int *new_flags,
				       u64 multicast)
{
	struct rtw89_dev *rtwdev = hw->priv;

	mutex_lock(&rtwdev->mutex);

	*new_flags &= FIF_ALLMULTI | FIF_OTHER_BSS | FIF_FCSFAIL |
		      FIF_BCN_PRBRESP_PROMISC;

	if (changed_flags & FIF_ALLMULTI) {
		if (*new_flags & FIF_ALLMULTI)
			rtwdev->hal.rx_fltr &= ~B_AX_A_MC;
		else
			rtwdev->hal.rx_fltr |= B_AX_A_MC;
	}
	if (changed_flags & FIF_FCSFAIL) {
		if (*new_flags & FIF_FCSFAIL)
			rtwdev->hal.rx_fltr |= B_AX_A_CRC32_ERR;
		else
			rtwdev->hal.rx_fltr &= ~B_AX_A_CRC32_ERR;
	}
	if (changed_flags & FIF_OTHER_BSS) {
		if (*new_flags & FIF_OTHER_BSS)
			rtwdev->hal.rx_fltr &= ~B_AX_A_A1_MATCH;
		else
			rtwdev->hal.rx_fltr |= B_AX_A_A1_MATCH;
	}
	if (changed_flags & FIF_BCN_PRBRESP_PROMISC) {
		if (*new_flags & FIF_BCN_PRBRESP_PROMISC) {
			rtwdev->hal.rx_fltr &= ~B_AX_A_BCN_CHK_EN;
			rtwdev->hal.rx_fltr &= ~B_AX_A_BC;
			rtwdev->hal.rx_fltr &= ~B_AX_A_A1_MATCH;
		} else {
			rtwdev->hal.rx_fltr |= B_AX_A_BCN_CHK_EN;
			rtwdev->hal.rx_fltr |= B_AX_A_BC;
			rtwdev->hal.rx_fltr |= B_AX_A_A1_MATCH;
		}
	}

	rtw89_write32(rtwdev,
		      rtw89_mac_reg_by_idx(R_AX_RX_FLTR_OPT, RTW89_MAC_0),
		      rtwdev->hal.rx_fltr);
	if (!rtwdev->dbcc_en)
		goto out;
	rtw89_write32(rtwdev,
		      rtw89_mac_reg_by_idx(R_AX_RX_FLTR_OPT, RTW89_MAC_1),
		      rtwdev->hal.rx_fltr);

out:
	mutex_unlock(&rtwdev->mutex);
}

static const u32 ac_to_edca_param[IEEE80211_NUM_ACS] = {
	[IEEE80211_AC_VO] = R_AX_EDCA_VO_PARAM_0,
	[IEEE80211_AC_VI] = R_AX_EDCA_VI_PARAM_0,
	[IEEE80211_AC_BE] = R_AX_EDCA_BE_PARAM_0,
	[IEEE80211_AC_BK] = R_AX_EDCA_BK_PARAM_0,
};

static u8 rtw89_aifsn_to_aifs(struct rtw89_dev *rtwdev,
			      struct rtw89_vif *rtwvif, u8 aifsn)
{
	struct ieee80211_vif *vif = rtwvif_to_vif(rtwvif);
	u8 slot_time;
	u8 sifs;

	slot_time = vif->bss_conf.use_short_slot ? 9 : 20;
	sifs = rtwdev->hal.current_band_type == RTW89_BAND_5G ? 16 : 10;

	return aifsn * slot_time + sifs;
}

static void __rtw89_conf_tx(struct rtw89_dev *rtwdev,
			    struct rtw89_vif *rtwvif, u16 ac)
{
	struct ieee80211_tx_queue_params *params = &rtwvif->tx_params[ac];
	u32 edca_param = rtw89_mac_reg_by_idx(ac_to_edca_param[ac], RTW89_MAC_0);
	u32 val;
	u8 ecw_max, ecw_min;
	u8 aifs;

	/* 2^ecw - 1 = cw; ecw = log2(cw + 1) */
	ecw_max = ilog2(params->cw_max + 1);
	ecw_min = ilog2(params->cw_min + 1);
	aifs = rtw89_aifsn_to_aifs(rtwdev, rtwvif, params->aifs);
	val = FIELD_PREP(B_AX_BE_0_TXOPLMT_MSK, params->txop) |
	      FIELD_PREP(B_AX_BE_0_CWMAX_MSK, ecw_max) |
	      FIELD_PREP(B_AX_BE_0_CWMIN_MSK, ecw_min) |
	      FIELD_PREP(B_AX_BE_0_AIFS_MSK, aifs);
	rtw89_write32(rtwdev, edca_param, val);
}

static void rtw89_conf_tx(struct rtw89_dev *rtwdev,
			  struct rtw89_vif *rtwvif)
{
	u16 ac;

	for (ac = 0; ac < IEEE80211_NUM_ACS; ac++)
		__rtw89_conf_tx(rtwdev, rtwvif, ac);
}

static void rtw89_ops_bss_info_changed(struct ieee80211_hw *hw,
				       struct ieee80211_vif *vif,
				       struct ieee80211_bss_conf *conf,
				       u32 changed)
{
	struct rtw89_dev *rtwdev = hw->priv;
	struct rtw89_vif *rtwvif = (struct rtw89_vif *)vif->drv_priv;

	mutex_lock(&rtwdev->mutex);

	if (changed & BSS_CHANGED_ASSOC) {
		rtw89_phy_set_bss_color(rtwdev, vif);
		rtw89_mac_port_update(rtwdev, rtwvif);
	}

	if (changed & BSS_CHANGED_BSSID) {
		ether_addr_copy(rtwvif->bssid, conf->bssid);
		rtw89_cam_bssid_changed(rtwdev, rtwvif);
		rtw89_fw_h2c_cam(rtwdev, rtwvif);
	}

	if (changed & BSS_CHANGED_ERP_SLOT)
		rtw89_conf_tx(rtwdev, rtwvif);

	if (changed & BSS_CHANGED_HE_BSS_COLOR)
		rtw89_phy_set_bss_color(rtwdev, vif);

	mutex_unlock(&rtwdev->mutex);
}

static int rtw89_ops_conf_tx(struct ieee80211_hw *hw,
			     struct ieee80211_vif *vif, u16 ac,
			     const struct ieee80211_tx_queue_params *params)
{
	struct rtw89_dev *rtwdev = hw->priv;
	struct rtw89_vif *rtwvif = (struct rtw89_vif *)vif->drv_priv;

	mutex_lock(&rtwdev->mutex);

	rtwvif->tx_params[ac] = *params;
	__rtw89_conf_tx(rtwdev, rtwvif, ac);

	mutex_unlock(&rtwdev->mutex);

	return 0;
}

static int __rtw89_ops_sta_state(struct ieee80211_hw *hw,
				 struct ieee80211_vif *vif,
				 struct ieee80211_sta *sta,
				 enum ieee80211_sta_state old_state,
				 enum ieee80211_sta_state new_state)
{
	struct rtw89_dev *rtwdev = hw->priv;

	if (old_state == IEEE80211_STA_NOTEXIST &&
	    new_state == IEEE80211_STA_NONE)
		return rtw89_core_sta_add(rtwdev, vif, sta);

	if (old_state == IEEE80211_STA_AUTH &&
	    new_state == IEEE80211_STA_ASSOC)
		return rtw89_core_sta_assoc(rtwdev, vif, sta);

	if (old_state == IEEE80211_STA_ASSOC &&
	    new_state == IEEE80211_STA_AUTH)
		return rtw89_core_sta_disassoc(rtwdev, vif, sta);

	if (old_state == IEEE80211_STA_AUTH &&
	    new_state == IEEE80211_STA_NONE)
		return rtw89_core_sta_disconnect(rtwdev, vif, sta);

	if (old_state == IEEE80211_STA_NONE &&
	    new_state == IEEE80211_STA_NOTEXIST)
		return rtw89_core_sta_remove(rtwdev, vif, sta);

	return 0;
}

static int rtw89_ops_sta_state(struct ieee80211_hw *hw,
			       struct ieee80211_vif *vif,
			       struct ieee80211_sta *sta,
			       enum ieee80211_sta_state old_state,
			       enum ieee80211_sta_state new_state)
{
	struct rtw89_dev *rtwdev = hw->priv;
	int ret;

	mutex_lock(&rtwdev->mutex);
	ret = __rtw89_ops_sta_state(hw, vif, sta, old_state, new_state);
	mutex_unlock(&rtwdev->mutex);

	return ret;
}

static int rtw89_ops_set_key(struct ieee80211_hw *hw, enum set_key_cmd cmd,
			     struct ieee80211_vif *vif,
			     struct ieee80211_sta *sta,
			     struct ieee80211_key_conf *key)
{
	struct rtw89_dev *rtwdev = hw->priv;
	int ret = 0;

	mutex_lock(&rtwdev->mutex);

	switch (cmd) {
	case SET_KEY:
		ret = rtw89_cam_sec_key_add(rtwdev, vif, sta, key);
		if (ret) {
			rtw89_err(rtwdev, "failed to add key to sec cam\n");
			goto out;
		}
		break;
	case DISABLE_KEY:
		rtw89_mac_flush_txq(rtwdev, BIT(rtwdev->hw->queues) - 1, false);
		ret = rtw89_cam_sec_key_del(rtwdev, vif, sta, key);
		if (ret) {
			rtw89_err(rtwdev, "failed to remove key from sec cam\n");
			goto out;
		}
		break;
	}

out:
	mutex_unlock(&rtwdev->mutex);

	return ret;
}

static int rtw89_ops_ampdu_action(struct ieee80211_hw *hw,
				  struct ieee80211_vif *vif,
				  struct ieee80211_ampdu_params *params)
{
	struct rtw89_dev *rtwdev = hw->priv;
	struct ieee80211_sta *sta = params->sta;
	struct rtw89_sta *rtwsta = (struct rtw89_sta *)sta->drv_priv;
	u16 tid = params->tid;
	struct ieee80211_txq *txq = sta->txq[tid];
	struct rtw89_txq *rtwtxq = (struct rtw89_txq *)txq->drv_priv;

	switch (params->action) {
	case IEEE80211_AMPDU_TX_START:
		return IEEE80211_AMPDU_TX_START_IMMEDIATE;
	case IEEE80211_AMPDU_TX_STOP_CONT:
	case IEEE80211_AMPDU_TX_STOP_FLUSH:
	case IEEE80211_AMPDU_TX_STOP_FLUSH_CONT:
		clear_bit(RTW89_TXQ_F_AMPDU, &rtwtxq->flags);
		rtw89_fw_h2c_ba_cam(rtwdev, false, rtwsta->mac_id, params);
		ieee80211_stop_tx_ba_cb_irqsafe(vif, sta->addr, tid);
		break;
	case IEEE80211_AMPDU_TX_OPERATIONAL:
		set_bit(RTW89_TXQ_F_AMPDU, &rtwtxq->flags);
		rtwsta->ampdu_params[tid].agg_num = params->buf_size;
		rtwsta->ampdu_params[tid].amsdu = params->amsdu;
		rtw89_fw_h2c_ba_cam(rtwdev, true, rtwsta->mac_id, params);
		break;
	case IEEE80211_AMPDU_RX_START:
	case IEEE80211_AMPDU_RX_STOP:
		break;
	default:
		WARN_ON(1);
		return -ENOTSUPP;
	}

	return 0;
}

static int rtw89_ops_set_rts_threshold(struct ieee80211_hw *hw, u32 value)
{
	return 0;
}

static void rtw89_ops_sta_statistics(struct ieee80211_hw *hw,
				     struct ieee80211_vif *vif,
				     struct ieee80211_sta *sta,
				     struct station_info *sinfo)
{
	struct rtw89_sta *rtwsta = (struct rtw89_sta *)sta->drv_priv;

	sinfo->txrate = rtwsta->ra_report.txrate;
	sinfo->filled |= BIT_ULL(NL80211_STA_INFO_TX_BITRATE);
}

static void rtw89_ops_flush(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
			    u32 queues, bool drop)
{
	struct rtw89_dev *rtwdev = hw->priv;

	mutex_lock(&rtwdev->mutex);
	rtw89_mac_flush_txq(rtwdev, queues, drop);
	mutex_unlock(&rtwdev->mutex);
}

static void rtw89_ops_sw_scan_start(struct ieee80211_hw *hw,
				    struct ieee80211_vif *vif,
				    const u8 *mac_addr)
{
	struct rtw89_dev *rtwdev = hw->priv;
	struct rtw89_hal *hal = &rtwdev->hal;

	rtw89_btc_ntfy_scan_start(rtwdev, RTW89_PHY_0, hal->current_band_type);
}

static void rtw89_ops_sw_scan_complete(struct ieee80211_hw *hw,
				       struct ieee80211_vif *vif)
{
	struct rtw89_dev *rtwdev = hw->priv;

	rtw89_btc_ntfy_scan_finish(rtwdev, RTW89_PHY_0);
}

const struct ieee80211_ops rtw89_ops = {
	.tx			= rtw89_ops_tx,
	.wake_tx_queue		= rtw89_ops_wake_tx_queue,
	.start			= rtw89_ops_start,
	.stop			= rtw89_ops_stop,
	.config			= rtw89_ops_config,
	.add_interface		= rtw89_ops_add_interface,
	.remove_interface	= rtw89_ops_remove_interface,
	.configure_filter	= rtw89_ops_configure_filter,
	.bss_info_changed	= rtw89_ops_bss_info_changed,
	.conf_tx		= rtw89_ops_conf_tx,
	.sta_state		= rtw89_ops_sta_state,
	.set_key		= rtw89_ops_set_key,
	.ampdu_action		= rtw89_ops_ampdu_action,
	.set_rts_threshold	= rtw89_ops_set_rts_threshold,
	.sta_statistics		= rtw89_ops_sta_statistics,
	.flush			= rtw89_ops_flush,
	.sw_scan_start		= rtw89_ops_sw_scan_start,
	.sw_scan_complete	= rtw89_ops_sw_scan_complete,
};
EXPORT_SYMBOL(rtw89_ops);
