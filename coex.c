// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright(c) 2019-2020  Realtek Corporation
 */

#include "coex.h"
#include "debug.h"
#include "mac.h"

enum btc_phymap {
	BTC_PHY_0 = BIT(0),
	BTC_PHY_1 = BIT(1),
	BTC_PHY_ALL = BIT(0) | BIT(1),
};

enum btc_cx_state_map {
	BTC_WIDLE = 0,
	BTC_WBUSY_BNOSCAN,
	BTC_WBUSY_BSCAN,
	BTC_WSCAN_BNOSCAN,
	BTC_WSCAN_BSCAN,
	BTC_WLINKING
};

enum btc_ant_phase {
	BTC_ANT_WPOWERON = 0,
	BTC_ANT_WINIT,
	BTC_ANT_WONLY,
	BTC_ANT_WOFF,
	BTC_ANT_W2G,
	BTC_ANT_W5G,
	BTC_ANT_W25G,
	BTC_ANT_FREERUN,
	BTC_ANT_WRFK,
	BTC_ANT_BRFK,
	BTC_ANT_MAX
};

enum btc_plt {
	BTC_PLT_NONE = 0,
	BTC_PLT_LTE_RX = BIT(0),
	BTC_PLT_GNT_BT_TX = BIT(1),
	BTC_PLT_GNT_BT_RX = BIT(2),
	BTC_PLT_GNT_WL = BIT(3),
	BTC_PLT_BT = BIT(1) | BIT(2),
	BTC_PLT_ALL = 0xf
};

enum btc_cx_poicy_main_type {
	BTC_CXP_OFF = 0,
	BTC_CXP_OFFB,
	BTC_CXP_OFFE,
	BTC_CXP_FIX,
	BTC_CXP_PFIX,
	BTC_CXP_AUTO,
	BTC_CXP_PAUTO,
	BTC_CXP_AUTO2,
	BTC_CXP_PAUTO2,
	BTC_CXP_MANUAL,
	BTC_CXP_USERDEF0,
	BTC_CXP_MAIN_MAX
};

enum btc_cx_poicy_type {
	/* TDMA off + pri: BT > WL */
	BTC_CXP_OFF_BT = (BTC_CXP_OFF << 8) | 0,

	/* TDMA off + pri: WL > BT */
	BTC_CXP_OFF_WL = (BTC_CXP_OFF << 8) | 1,

	/* TDMA off + pri: BT = WL */
	BTC_CXP_OFF_EQ0 = (BTC_CXP_OFF << 8) | 2,

	/* TDMA off + pri: BT = WL > BT_Lo */
	BTC_CXP_OFF_EQ1 = (BTC_CXP_OFF << 8) | 3,

	/* TDMA off + pri: WL = BT, BT_Rx > WL_Lo_Tx */
	BTC_CXP_OFF_EQ2 = (BTC_CXP_OFF << 8) | 4,

	/* TDMA off + pri: WL_Rx = BT, BT_HI > WL_Tx > BT_Lo */
	BTC_CXP_OFF_EQ3 = (BTC_CXP_OFF << 8) | 5,

	/* TDMA off + pri: BT_Hi > WL > BT_Lo */
	BTC_CXP_OFF_BWB0 = (BTC_CXP_OFF << 8) | 6,

	/* TDMA off + pri: WL_Hi-Tx > BT_Hi_Rx, BT_Hi > WL > BT_Lo */
	BTC_CXP_OFF_BWB1 = (BTC_CXP_OFF << 8) | 7,

	/* TDMA off+Bcn-Protect + pri: WL_Hi-Tx > BT_Hi_Rx, BT_Hi > WL > BT_Lo*/
	BTC_CXP_OFFB_BWB0 = (BTC_CXP_OFFB << 8) | 0,

	/* TDMA off + Ext-Ctrl + pri: default */
	BTC_CXP_OFFE_DEF = (BTC_CXP_OFFE << 8) | 0,

	/* TDMA Fix slot-0: W1:B1 = 30:30 */
	BTC_CXP_FIX_TD3030 = (BTC_CXP_FIX << 8) | 0,

	/* TDMA Fix slot-1: W1:B1 = 50:50 */
	BTC_CXP_FIX_TD5050 = (BTC_CXP_FIX << 8) | 1,

	/* TDMA Fix slot-2: W1:B1 = 20:30 */
	BTC_CXP_FIX_TD2030 = (BTC_CXP_FIX << 8) | 2,

	/* TDMA Fix slot-3: W1:B1 = 40:10 */
	BTC_CXP_FIX_TD4010 = (BTC_CXP_FIX << 8) | 3,

	/* TDMA Fix slot-4: W1:B1 = 70:10 */
	BTC_CXP_FIX_TD7010 = (BTC_CXP_FIX << 8) | 4,

	/* TDMA Fix slot-5: W1:B1 = 20:60 */
	BTC_CXP_FIX_TD2060 = (BTC_CXP_FIX << 8) | 5,

	/* TDMA Fix slot-6: W1:B1 = 30:60 */
	BTC_CXP_FIX_TD3060 = (BTC_CXP_FIX << 8) | 6,

	/* TDMA Fix slot-6: W1:B1 = 20:80 */
	BTC_CXP_FIX_TD2080 = (BTC_CXP_FIX << 8) | 7,

	/* PS-TDMA Fix slot-0: W1:B1 = 30:30 */
	BTC_CXP_PFIX_TD3030 = (BTC_CXP_PFIX << 8) | 0,

	/* PS-TDMA Fix slot-1: W1:B1 = 50:50 */
	BTC_CXP_PFIX_TD5050 = (BTC_CXP_PFIX << 8) | 1,

	/* PS-TDMA Fix slot-2: W1:B1 = 20:30 */
	BTC_CXP_PFIX_TD2030 = (BTC_CXP_PFIX << 8) | 2,

	/* PS-TDMA Fix slot-3: W1:B1 = 20:60 */
	BTC_CXP_PFIX_TD2060 = (BTC_CXP_PFIX << 8) | 3,

	/* PS-TDMA Fix slot-4: W1:B1 = 30:70 */
	BTC_CXP_PFIX_TD3070 = (BTC_CXP_PFIX << 8) | 4,

	/* PS-TDMA Fix slot-5: W1:B1 = 20:80 */
	BTC_CXP_PFIX_TD2080 = (BTC_CXP_PFIX << 8) | 5,

	/* TDMA Auto slot-0: W1:B1 = 50:100 */
	BTC_CXP_AUTO_TD50100 = (BTC_CXP_AUTO << 8) | 0,

	/* TDMA Auto slot-1: W1:B1 = 60:150 */
	BTC_CXP_AUTO_TD60150 = (BTC_CXP_AUTO << 8) | 1,

	/* TDMA Auto slot-2: W1:B1 = 20:150 */
	BTC_CXP_AUTO_TD20150 = (BTC_CXP_AUTO << 8) | 2,

	/* PS-TDMA Auto slot-0: W1:B1 = 50:100 */
	BTC_CXP_PAUTO_TD50100 = (BTC_CXP_PAUTO << 8) | 0,

	/* PS-TDMA Auto slot-1: W1:B1 = 60:150 */
	BTC_CXP_PAUTO_TD60150 = (BTC_CXP_PAUTO << 8) | 1,

	/* PS-TDMA Auto slot-2: W1:B1 = 20:150 */
	BTC_CXP_PAUTO_TD20150 = (BTC_CXP_PAUTO << 8) | 2,

	/* TDMA Auto slot2-0: W1:B4 = 30:50 */
	BTC_CXP_AUTO2_TD3050 = (BTC_CXP_AUTO2 << 8) | 0,

	/* TDMA Auto slot2-1: W1:B4 = 30:70 */
	BTC_CXP_AUTO2_TD3070 = (BTC_CXP_AUTO2 << 8) | 1,

	/* TDMA Auto slot2-2: W1:B4 = 50:50 */
	BTC_CXP_AUTO2_TD5050 = (BTC_CXP_AUTO2 << 8) | 2,

	/* TDMA Auto slot2-3: W1:B4 = 60:60 */
	BTC_CXP_AUTO2_TD6060 = (BTC_CXP_AUTO2 << 8) | 3,

	/* TDMA Auto slot2-4: W1:B4 = 20:80 */
	BTC_CXP_AUTO2_TD2080 = (BTC_CXP_AUTO2 << 8) | 4,

	/* PS-TDMA Auto slot2-0: W1:B4 = 30:50 */
	BTC_CXP_PAUTO2_TD3050 = (BTC_CXP_PAUTO2 << 8) | 0,

	/* PS-TDMA Auto slot2-1: W1:B4 = 30:70 */
	BTC_CXP_PAUTO2_TD3070 = (BTC_CXP_PAUTO2 << 8) | 1,

	/* PS-TDMA Auto slot2-2: W1:B4 = 50:50 */
	BTC_CXP_PAUTO2_TD5050 = (BTC_CXP_PAUTO2 << 8) | 2,

	/* PS-TDMA Auto slot2-3: W1:B4 = 60:60 */
	BTC_CXP_PAUTO2_TD6060 = (BTC_CXP_PAUTO2 << 8) | 3,

	/* PS-TDMA Auto slot2-4: W1:B4 = 20:80 */
	BTC_CXP_PAUTO2_TD2080 = (BTC_CXP_PAUTO2 << 8) | 4,

	BTC_CXP_MAX = 0xffff
};

#define BTC_PHY_MAX 2
#define BTC_CXP_MASK GENMASK(15, 8)

enum btc_gnt_state {
	BTC_GNT_HW	= 0,
	BTC_GNT_SW_LO,
	BTC_GNT_SW_HI,
	BTC_GNT_MAX
};

enum btc_reason {
	BTC_RSN_NTFY_INIT,
	BTC_RSN_NTFY_SWBAND,
	BTC_RSN_NUM,
};

enum btc_action {
	BTC_ACT_WL_ONLY,
	BTC_ACT_NUM,
};

static void _set_policy(struct rtw89_dev *rtwdev, u16 policy_type,
			enum btc_action action)
{
}

static void _set_gnt_wl(struct rtw89_dev *rtwdev, u8 phy_map, u8 state)
{
	struct rtw89_btc *btc = &rtwdev->btc;
	struct rtw89_btc_dm *dm = &btc->dm;
	struct rtw89_mac_ax_gnt *g = dm->gnt.band;
	u8 i;

	if (phy_map > BTC_PHY_ALL)
		return;

	for (i = 0; i < BTC_PHY_MAX; i++) {
		if (!(phy_map & BIT(i)))
			continue;

		switch (state) {
		case BTC_GNT_HW:
			g[i].gnt_wl_sw_en = 0;
			g[i].gnt_wl = 0;
			break;
		case BTC_GNT_SW_LO:
			g[i].gnt_wl_sw_en = 1;
			g[i].gnt_wl = 0;
			break;
		case BTC_GNT_SW_HI:
			g[i].gnt_wl_sw_en = 1;
			g[i].gnt_wl = 1;
			break;
		}
	}

	rtw89_mac_cfg_gnt(rtwdev, &dm->gnt);
}

static void _set_gnt_bt(struct rtw89_dev *rtwdev, u8 phy_map, u8 state)
{
	struct rtw89_btc *btc = &rtwdev->btc;
	struct rtw89_btc_dm *dm = &btc->dm;
	struct rtw89_mac_ax_gnt *g = dm->gnt.band;
	u8 i;

	if (phy_map > BTC_PHY_ALL)
		return;

	for (i = 0; i < BTC_PHY_MAX; i++) {
		if (!(phy_map & BIT(i)))
			continue;

		switch (state) {
		case BTC_GNT_HW:
			g[i].gnt_bt_sw_en = 0;
			g[i].gnt_bt = 0;
			break;
		case BTC_GNT_SW_LO:
			g[i].gnt_bt_sw_en = 1;
			g[i].gnt_bt = 0;
			break;
		case BTC_GNT_SW_HI:
			g[i].gnt_bt_sw_en = 1;
			g[i].gnt_bt = 1;
			break;
		}
	}

	rtw89_mac_cfg_gnt(rtwdev, &dm->gnt);
}

static void _set_bt_plut(struct rtw89_dev *rtwdev, u8 phy_map, u8 tx_val, u8 rx_val)
{
	struct rtw89_mac_ax_plt plt;

	plt.band = RTW89_MAC_0;
	plt.tx = tx_val;
	plt.rx = rx_val;

	if (phy_map & BTC_PHY_0)
		rtw89_mac_cfg_plt(rtwdev, &plt);

	if (!rtwdev->dbcc_en)
		return;

	plt.band = RTW89_MAC_1;
	if (phy_map & BTC_PHY_1)
		rtw89_mac_cfg_plt(rtwdev, &plt);
}

static void _set_ant(struct rtw89_dev *rtwdev, bool force_exec, u8 phy_map, u8 type)
{
	switch (type) {
	case BTC_ANT_WONLY:
		_set_gnt_wl(rtwdev, phy_map, BTC_GNT_SW_HI);
		_set_gnt_bt(rtwdev, phy_map, BTC_GNT_SW_LO);
		rtw89_mac_cfg_ctrl_path(rtwdev, true);
		_set_bt_plut(rtwdev, BTC_PHY_ALL, BTC_PLT_NONE, BTC_PLT_NONE);
		break;
	default:
		break;
	}
}

static void _action_wl_only(struct rtw89_dev *rtwdev)
{
	_set_ant(rtwdev, FC_EXEC, BTC_PHY_ALL, BTC_ANT_WONLY);
	_set_policy(rtwdev, BTC_CXP_OFF_BT, BTC_ACT_WL_ONLY);
}

static void _run_coex(struct rtw89_dev *rtwdev, enum btc_reason reason)
{
	struct rtw89_btc_dm *dm = &rtwdev->btc.dm;

	lockdep_assert_held(&rtwdev->mutex);

	if (dm->wl_only)
		_action_wl_only(rtwdev);
}

void rtw89_btc_ntfy_init(struct rtw89_dev *rtwdev, u8 mode)
{
	struct rtw89_btc_dm *dm = &rtwdev->btc.dm;
	const struct rtw89_chip_info *chip = rtwdev->chip;

	dm->wl_only = mode == BTC_MODE_WL ? 1 : 0;

	chip->ops->btc_set_rfe(rtwdev);

	chip->ops->btc_init_cfg(rtwdev);

	_run_coex(rtwdev, BTC_RSN_NTFY_INIT);

	rtw89_ctrl_btg(rtwdev, true);
}

void rtw89_btc_ntfy_scan_start(struct rtw89_dev *rtwdev, u8 phy_idx, u8 band)
{
}

void rtw89_btc_ntfy_scan_finish(struct rtw89_dev *rtwdev, u8 phy_idx)
{
}

void rtw89_btc_ntfy_switch_band(struct rtw89_dev *rtwdev, u8 phy_idx, u8 band)
{
	_run_coex(rtwdev, BTC_RSN_NTFY_SWBAND);
}

static bool _ntfy_wl_rfk(struct rtw89_dev *rtwdev, u8 phy_path,
			 enum btc_wl_rfk_type type,
			 enum btc_wl_rfk_state state)
{
	return true;
}

void rtw89_btc_ntfy_wl_rfk(struct rtw89_dev *rtwdev, u8 phy_map,
			   enum btc_wl_rfk_type type,
			   enum btc_wl_rfk_state state)
{
	u8 band;

	band = FIELD_GET(BTC_RFK_BAND_MAP, phy_map);

	rtw89_debug(rtwdev, RTW89_DBG_RFK,
		    "[RFK] RFK notify (%s / PHY%u / K_type = %u / path_idx = %lu / process = %s)\n",
		    band == RTW89_BAND_2G ? "2G" :
		    band == RTW89_BAND_5G ? "5G" : "6G",
		    !!(FIELD_GET(BTC_RFK_PHY_MAP, phy_map) & BIT(RTW89_PHY_1)),
		    type,
		    FIELD_GET(BTC_RFK_PATH_MAP, phy_map),
		    state == BTC_WRFK_STOP ? "RFK_STOP" :
		    state == BTC_WRFK_START ? "RFK_START" :
		    state == BTC_WRFK_ONESHOT_START ? "ONE-SHOT_START" :
		    "ONE-SHOT_STOP");

	_ntfy_wl_rfk(rtwdev, phy_map, type, state);
}
