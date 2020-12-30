/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright(c) 2019-2020  Realtek Corporation
 */

#ifndef __RTW89_COEX_H__
#define __RTW89_COEX_H__

enum btc_mode {
	BTC_MODE_NORMAL,
	BTC_MODE_WL,
	BTC_MODE_BT,
	BTC_MODE_WLOFF,
	BTC_MODE_MAX
};

enum btc_wl_rfk_type {
	BTC_WRFKT_IQK = 0,
	BTC_WRFKT_LCK = 1,
	BTC_WRFKT_DPK = 2,
	BTC_WRFKT_TXGAPK = 3,
	BTC_WRFKT_DACK = 4,
	BTC_WRFKT_RXDCK = 5,
	BTC_WRFKT_TSSI = 6,
};

#define NM_EXEC false
#define FC_EXEC true

#define BTC_RFK_PATH_MAP GENMASK(3, 0)
#define BTC_RFK_PHY_MAP GENMASK(5, 4)
#define BTC_RFK_BAND_MAP GENMASK(7, 6)

enum btc_wl_rfk_state {
	BTC_WRFK_STOP = 0,
	BTC_WRFK_START = 1,
	BTC_WRFK_ONESHOT_START = 2,
	BTC_WRFK_ONESHOT_STOP = 3,
};

enum btc_pri {
	BTC_PRI_MASK_RX_RESP = 0,
	BTC_PRI_MASK_TX_RESP,
	BTC_PRI_MASK_BEACON,
	BTC_PRI_MASK_RX_CCK,
	BTC_PRI_MASK_MAX
};

enum btc_wtrs {
	BTC_WTRX_SS_GROUP = 0x0,
	BTC_WTRX_TX_GROUP = 0x2,
	BTC_WTRX_RX_GROUP = 0x3,
	BTC_WTRX_MAX,
};

enum btc_ant {
	BTC_ANT_SHARED = 0,
	BTC_ANT_DEDICATED,
	BTC_ANTTYPE_MAX
};

enum btc_bt_btg {
	BTC_BT_ALONE = 0,
	BTC_BT_BTG
};

enum btc_switch {
	BTC_SWITCH_INTERNAL = 0,
	BTC_SWITCH_EXTERNAL
};

void rtw89_btc_ntfy_init(struct rtw89_dev *rtwdev, u8 mode);
void rtw89_btc_ntfy_scan_start(struct rtw89_dev *rtwdev, u8 phy_idx, u8 band);
void rtw89_btc_ntfy_scan_finish(struct rtw89_dev *rtwdev, u8 phy_idx);
void rtw89_btc_ntfy_switch_band(struct rtw89_dev *rtwdev, u8 phy_idx, u8 band);
void rtw89_btc_ntfy_wl_rfk(struct rtw89_dev *rtwdev, u8 phy_map,
			   enum btc_wl_rfk_type type,
			   enum btc_wl_rfk_state state);

static inline u8 rtw89_btc_phymap(struct rtw89_dev *rtwdev,
				  enum rtw89_phy_idx phy_idx,
				  enum rtw89_rf_path_bit paths)
{
	struct rtw89_hal *hal = &rtwdev->hal;
	u8 phy_map;

	phy_map = FIELD_PREP(BTC_RFK_PATH_MAP, paths) |
		  FIELD_PREP(BTC_RFK_PHY_MAP, BIT(phy_idx)) |
		  FIELD_PREP(BTC_RFK_BAND_MAP, hal->current_band_type);

	return phy_map;
}

static inline u8 rtw89_btc_path_phymap(struct rtw89_dev *rtwdev,
				       enum rtw89_phy_idx phy_idx,
				       enum rtw89_rf_path path)
{
	return rtw89_btc_phymap(rtwdev, phy_idx, BIT(path));
}

#endif
