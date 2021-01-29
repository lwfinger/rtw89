// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright(c) 2019-2020  Realtek Corporation
 */

#include "coex.h"
#include "mac.h"
#include "phy.h"
#include "reg.h"
#include "rtw8852a.h"
#include "rtw8852a_rfk.h"
#include "rtw8852a_table.h"
#include "txrx.h"

static struct rtw89_hfc_ch_cfg rtw8852a_hfc_chcfg_pcie_sutp[] = {
	{128, 256, grp_0}, /* ACH 0 */
	{0, 0, grp_1}, /* ACH 1 */
	{0, 0, grp_1}, /* ACH 2 */
	{0, 0, grp_1}, /* ACH 3 */
	{0, 0, grp_1}, /* ACH 4 */
	{0, 0, grp_1}, /* ACH 5 */
	{0, 0, grp_1}, /* ACH 6 */
	{0, 0, grp_1}, /* ACH 7 */
	{0, 0, grp_1}, /* B0MGQ */
	{0, 0, grp_1}, /* B0HIQ */
	{0, 0, grp_1}, /* B1MGQ */
	{0, 0, grp_1}, /* B1HIQ */
	{40, 0, 0} /* FWCMDQ */
};

static struct rtw89_hfc_ch_cfg rtw8852a_hfc_chcfg_pcie_stf[] = {
	{8, 256, grp_0}, /* ACH 0 */
	{8, 256, grp_0}, /* ACH 1 */
	{8, 256, grp_0}, /* ACH 2 */
	{8, 256, grp_0}, /* ACH 3 */
	{8, 256, grp_1}, /* ACH 4 */
	{8, 256, grp_1}, /* ACH 5 */
	{8, 256, grp_1}, /* ACH 6 */
	{8, 256, grp_1}, /* ACH 7 */
	{8, 256, grp_0}, /* B0MGQ */
	{8, 256, grp_0}, /* B0HIQ */
	{8, 256, grp_1}, /* B1MGQ */
	{8, 256, grp_1}, /* B1HIQ */
	{40, 0, 0} /* FWCMDQ */
};

static struct rtw89_hfc_ch_cfg rtw8852a_hfc_chcfg_pcie[] = {
	{128, 1896, grp_0}, /* ACH 0 */
	{128, 1896, grp_0}, /* ACH 1 */
	{128, 1896, grp_0}, /* ACH 2 */
	{128, 1896, grp_0}, /* ACH 3 */
	{128, 1896, grp_1}, /* ACH 4 */
	{128, 1896, grp_1}, /* ACH 5 */
	{128, 1896, grp_1}, /* ACH 6 */
	{128, 1896, grp_1}, /* ACH 7 */
	{32, 1896, grp_0}, /* B0MGQ */
	{128, 1896, grp_0}, /* B0HIQ */
	{32, 1896, grp_1}, /* B1MGQ */
	{128, 1896, grp_1}, /* B1HIQ */
	{40, 0, 0} /* FWCMDQ */
};

static struct rtw89_hfc_ch_cfg rtw8852a_hfc_chcfg_pcie_la[] = {
	{64, 586, grp_0}, /* ACH 0 */
	{64, 586, grp_0}, /* ACH 1 */
	{64, 586, grp_0}, /* ACH 2 */
	{64, 586, grp_0}, /* ACH 3 */
	{64, 586, grp_1}, /* ACH 4 */
	{64, 586, grp_1}, /* ACH 5 */
	{64, 586, grp_1}, /* ACH 6 */
	{64, 586, grp_1}, /* ACH 7 */
	{32, 586, grp_0}, /* B0MGQ */
	{64, 586, grp_0}, /* B0HIQ */
	{32, 586, grp_1}, /* B1MGQ */
	{64, 586, grp_1}, /* B1HIQ */
	{40, 0, 0} /* FWCMDQ */
};

static struct rtw89_hfc_pub_cfg rtw8852a_hfc_pubcfg_pcie = {
	1896, /* Group 0 */
	1896, /* Group 1 */
	3792, /* Public Max */
	0 /* WP threshold */
};

static struct rtw89_hfc_pub_cfg rtw8852a_hfc_pubcfg_pcie_stf = {
	256, /* Group 0 */
	256, /* Group 1 */
	512, /* Public Max */
	104 /* WP threshold */
};

static struct rtw89_hfc_pub_cfg rtw8852a_hfc_pubcfg_pcie_sutp = {
	256, /* Group 0 */
	0, /* Group 1 */
	256, /* Public Max */
	0 /* WP threshold */
};

static struct rtw89_hfc_pub_cfg rtw8852a_hfc_pubcfg_pcie_la = {
	586, /* Group 0 */
	586, /* Group 1 */
	1172, /* Public Max */
	0 /* WP threshold */
};

static struct rtw89_hfc_param_ini rtw8852a_hfc_param_ini_pcie[] = {
	[RTW89_QTA_SCC] = {rtw8852a_hfc_chcfg_pcie, &rtw8852a_hfc_pubcfg_pcie,
			   &rtw_hfc_preccfg_pcie, RTW89_HCIFC_POH},
	[RTW89_QTA_DBCC] = {rtw8852a_hfc_chcfg_pcie, &rtw8852a_hfc_pubcfg_pcie,
			    &rtw_hfc_preccfg_pcie, RTW89_HCIFC_POH},
	[RTW89_QTA_SCC_STF] = {rtw8852a_hfc_chcfg_pcie_stf,
			       &rtw8852a_hfc_pubcfg_pcie_stf,
			       &rtw_hfc_preccfg_pcie_stf, RTW89_HCIFC_STF},
	[RTW89_QTA_DBCC_STF] = {rtw8852a_hfc_chcfg_pcie_stf,
				&rtw8852a_hfc_pubcfg_pcie_stf,
				&rtw_hfc_preccfg_pcie_stf, RTW89_HCIFC_STF},
	[RTW89_QTA_SU_TP] = {rtw8852a_hfc_chcfg_pcie_sutp,
			     &rtw8852a_hfc_pubcfg_pcie_sutp,
			     &rtw_hfc_preccfg_pcie, RTW89_HCIFC_POH},
	[RTW89_QTA_DLFW] = {NULL, NULL, &rtw_hfc_preccfg_pcie, RTW89_HCIFC_POH},
	[RTW89_QTA_LAMODE] = {rtw8852a_hfc_chcfg_pcie_la,
			      &rtw8852a_hfc_pubcfg_pcie_la,
			      &rtw_hfc_preccfg_pcie, RTW89_HCIFC_POH},
	[RTW89_QTA_INVALID] = {NULL},
};

static struct rtw89_dle_mem rtw8852a_dle_mem_pcie[] = {
	[RTW89_QTA_SCC] = {RTW89_QTA_SCC, &wde_size0, &ple_size0, &wde_qt0,
			    &wde_qt0, &ple_qt4, &ple_qt5},
	[RTW89_QTA_DBCC] = {RTW89_QTA_DBCC, &wde_size0, &ple_size0, &wde_qt0,
			    &wde_qt0, &ple_qt0, &ple_qt1},
	[RTW89_QTA_SCC_STF] = {RTW89_QTA_SCC_STF, &wde_size1, &ple_size2,
			       &wde_qt1, &wde_qt1, &ple_qt8, &ple_qt9},
	[RTW89_QTA_DBCC_STF] = {RTW89_QTA_DBCC_STF, &wde_size1, &ple_size2,
				&wde_qt1, &wde_qt1, &ple_qt10, &ple_qt11},
	[RTW89_QTA_SU_TP] = {RTW89_QTA_SU_TP, &wde_size3, &ple_size3,
			     &wde_qt3, &wde_qt3, &ple_qt12, &ple_qt12},
	[RTW89_QTA_DLFW] = {RTW89_QTA_DLFW, &wde_size4, &ple_size4,
			    &wde_qt4, &wde_qt4, &ple_qt13, &ple_qt13},
	[RTW89_QTA_LAMODE] = {RTW89_QTA_LAMODE, &wde_size10, &ple_size10,
			      &wde_qt9, &wde_qt9, &ple_qt23, &ple_qt24},
	[RTW89_QTA_INVALID] = {RTW89_QTA_INVALID, NULL, NULL, NULL, NULL, NULL,
			       NULL},
};

static void rtw8852ae_efuse_parsing(struct rtw89_efuse *efuse,
				    struct rtw8852a_efuse *map)
{
	ether_addr_copy(efuse->addr, map->e.mac_addr);
	efuse->rfe_type = map->rfe_type;
	efuse->xtal_cap = map->xtal_k;
}

static void rtw8852a_efuse_parsing_tssi(struct rtw89_dev *rtwdev,
					struct rtw8852a_efuse *map)
{
	struct rtw89_tssi_info *tssi = &rtwdev->tssi;
	struct rtw8852a_tssi_offset *ofst[] = {&map->path_a_tssi, &map->path_b_tssi};
	u8 i, j;

	tssi->thermal[RF_PATH_A] = map->path_a_therm;
	tssi->thermal[RF_PATH_B] = map->path_b_therm;

	for (i = 0; i < RF_PATH_NUM_8852A; i++) {
		memcpy(tssi->tssi_cck[i], ofst[i]->cck_tssi,
		       sizeof(ofst[i]->cck_tssi));

		for (j = 0; j < TSSI_CCK_CH_GROUP_NUM; j++)
			rtw89_debug(rtwdev, RTW89_DBG_TSSI,
				    "[TSSI][EFUSE] path=%d cck[%d]=0x%x\n",
				    i, j, tssi->tssi_cck[i][j]);

		memcpy(tssi->tssi_mcs[i], ofst[i]->bw40_tssi,
		       sizeof(ofst[i]->bw40_tssi));
		memcpy(tssi->tssi_mcs[i] + TSSI_MCS_2G_CH_GROUP_NUM,
		       ofst[i]->bw40_1s_tssi_5g, sizeof(ofst[i]->bw40_1s_tssi_5g));

		for (j = 0; j < TSSI_MCS_CH_GROUP_NUM; j++)
			rtw89_debug(rtwdev, RTW89_DBG_TSSI,
				    "[TSSI][EFUSE] path=%d mcs[%d]=0x%x\n",
				    i, j, tssi->tssi_mcs[i][j]);
	}
}

static int rtw8852a_read_efuse(struct rtw89_dev *rtwdev, u8 *log_map)
{
	struct rtw89_efuse *efuse = &rtwdev->efuse;
	struct rtw8852a_efuse *map;

	map = (struct rtw8852a_efuse *)log_map;

	efuse->country_code[0] = map->country_code[0];
	efuse->country_code[1] = map->country_code[1];
	rtw8852a_efuse_parsing_tssi(rtwdev, map);

	switch (rtwdev->hci.type) {
	case RTW89_HCI_TYPE_PCIE:
		rtw8852ae_efuse_parsing(efuse, map);
		break;
	default:
		return -ENOTSUPP;
	}

	rtw89_info(rtwdev, "chip rfe_type is %d\n", efuse->rfe_type);

	return 0;
}

static void rtw8852a_phycap_parsing_tssi(struct rtw89_dev *rtwdev, u8 *phycap_map)
{
	struct rtw89_tssi_info *tssi = &rtwdev->tssi;
	static const u32 tssi_trim_addr[RF_PATH_NUM_8852A] = {0x5D6, 0x5AB};
	u32 addr = rtwdev->chip->phycap_addr;
	bool pg = false;
	u32 ofst;
	u8 i, j;

	for (i = 0; i < RF_PATH_NUM_8852A; i++) {
		for (j = 0; j < TSSI_TRIM_CH_GROUP_NUM; j++) {
			/* addrs are in decreasing order */
			ofst = tssi_trim_addr[i] - addr - j;
			tssi->tssi_trim[i][j] = phycap_map[ofst];

			if (phycap_map[ofst] != 0xff)
				pg = true;
		}
	}

	if (!pg) {
		memset(tssi->tssi_trim, 0, sizeof(tssi->tssi_trim));
		rtw89_debug(rtwdev, RTW89_DBG_TSSI,
			    "[TSSI][TRIM] no PG, set all trim info to 0\n");
	}

	for (i = 0; i < RF_PATH_NUM_8852A; i++)
		for (j = 0; j < TSSI_TRIM_CH_GROUP_NUM; j++)
			rtw89_debug(rtwdev, RTW89_DBG_TSSI,
				    "[TSSI] path=%d idx=%d trim=0x%x addr=0x%x\n",
				    i, j, tssi->tssi_trim[i][j],
				    tssi_trim_addr[i] - j);
}

static void rtw8852a_phycap_parsing_thermal_trim(struct rtw89_dev *rtwdev,
						 u8 *phycap_map)
{
	struct rtw89_power_trim_info *info = &rtwdev->pwr_trim;
	static const u32 thm_trim_addr[RF_PATH_NUM_8852A] = {0x5DF, 0x5DC};
	u32 addr = rtwdev->chip->phycap_addr;
	u8 i;

	for (i = 0; i < RF_PATH_NUM_8852A; i++) {
		info->thermal_trim[i] = phycap_map[thm_trim_addr[i] - addr];

		rtw89_debug(rtwdev, RTW89_DBG_RFK,
			    "[THERMAL][TRIM] path=%d thermal_trim=0x%x\n",
			    i, info->thermal_trim[i]);

		if (info->thermal_trim[i] != 0xff)
			info->pg_thermal_trim = true;
	}
}

static void rtw8852a_thermal_trim(struct rtw89_dev *rtwdev)
{
#define __thm_setting(raw)				\
({							\
	u8 __v = (raw);					\
	((__v & 0x1) << 3) | ((__v & 0x1f) >> 1);	\
})
	struct rtw89_power_trim_info *info = &rtwdev->pwr_trim;
	u8 i, val;

	if (!info->pg_thermal_trim) {
		rtw89_debug(rtwdev, RTW89_DBG_RFK,
			    "[THERMAL][TRIM] no PG, do nothing\n");

		return;
	}

	for (i = 0; i < RF_PATH_NUM_8852A; i++) {
		val = __thm_setting(info->thermal_trim[i]);
		rtw89_write_rf(rtwdev, i, RR_TM2, RR_TM2_OFF, val);

		rtw89_debug(rtwdev, RTW89_DBG_RFK,
			    "[THERMAL][TRIM] path=%d thermal_setting=0x%x\n",
			    i, val);
	}
#undef __thm_setting
}

static void rtw8852a_phycap_parsing_pa_bias_trim(struct rtw89_dev *rtwdev,
						 u8 *phycap_map)
{
	struct rtw89_power_trim_info *info = &rtwdev->pwr_trim;
	static const u32 pabias_trim_addr[RF_PATH_NUM_8852A] = {0x5DE, 0x5DB};
	u32 addr = rtwdev->chip->phycap_addr;
	u8 i;

	for (i = 0; i < RF_PATH_NUM_8852A; i++) {
		info->pa_bias_trim[i] = phycap_map[pabias_trim_addr[i] - addr];

		rtw89_debug(rtwdev, RTW89_DBG_RFK,
			    "[PA_BIAS][TRIM] path=%d pa_bias_trim=0x%x\n",
			    i, info->pa_bias_trim[i]);

		if (info->pa_bias_trim[i] != 0xff)
			info->pg_pa_bias_trim = true;
	}
}

static void rtw8852a_pa_bias_trim(struct rtw89_dev *rtwdev)
{
	struct rtw89_power_trim_info *info = &rtwdev->pwr_trim;
	u8 pabias_2g, pabias_5g;
	u8 i;

	if (!info->pg_pa_bias_trim) {
		rtw89_debug(rtwdev, RTW89_DBG_RFK,
			    "[PA_BIAS][TRIM] no PG, do nothing\n");

		return;
	}

	for (i = 0; i < RF_PATH_NUM_8852A; i++) {
		pabias_2g = FIELD_GET(GENMASK(3, 0), info->pa_bias_trim[i]);
		pabias_5g = FIELD_GET(GENMASK(7, 4), info->pa_bias_trim[i]);

		rtw89_debug(rtwdev, RTW89_DBG_RFK,
			    "[PA_BIAS][TRIM] path=%d 2G=0x%x 5G=0x%x\n",
			    i, pabias_2g, pabias_5g);

		rtw89_write_rf(rtwdev, i, RR_BIASA, RR_BIASA_TXG, pabias_2g);
		rtw89_write_rf(rtwdev, i, RR_BIASA, RR_BIASA_TXA, pabias_5g);
	}
}

static int rtw8852a_read_phycap(struct rtw89_dev *rtwdev, u8 *phycap_map)
{
	rtw8852a_phycap_parsing_tssi(rtwdev, phycap_map);
	rtw8852a_phycap_parsing_thermal_trim(rtwdev, phycap_map);
	rtw8852a_phycap_parsing_pa_bias_trim(rtwdev, phycap_map);

	return 0;
}

static void rtw8852a_power_trim(struct rtw89_dev *rtwdev)
{
	rtw8852a_thermal_trim(rtwdev);
	rtw8852a_pa_bias_trim(rtwdev);
}

static void rtw8852a_set_channel_mac(struct rtw89_dev *rtwdev,
				     struct rtw89_channel_params *param,
				     u8 mac_idx)
{
	u32 rf_mod = rtw89_mac_reg_by_idx(R_AX_WMAC_RFMOD, mac_idx);
	u32 sub_carr = rtw89_mac_reg_by_idx(R_AX_TX_SUB_CARRIER_VALUE,
					     mac_idx);
	u32 chk_rate = rtw89_mac_reg_by_idx(R_AX_TXRATE_CHK, mac_idx);
	u8 txsc20 = 0, txsc40 = 0;

	switch (param->bandwidth) {
	case RTW89_CHANNEL_WIDTH_80:
		txsc40 = rtw89_phy_get_txsc(rtwdev, param,
					    RTW89_CHANNEL_WIDTH_40);
		fallthrough;
	case RTW89_CHANNEL_WIDTH_40:
		txsc20 = rtw89_phy_get_txsc(rtwdev, param,
					    RTW89_CHANNEL_WIDTH_20);
		break;
	default:
		break;
	}

	switch (param->bandwidth) {
	case RTW89_CHANNEL_WIDTH_80:
		rtw89_write8_mask(rtwdev, rf_mod, B_AX_WMAC_RFMOD_MASK, BIT(1));
		rtw89_write32(rtwdev, sub_carr, txsc20 | (txsc40 << 4));
		break;
	case RTW89_CHANNEL_WIDTH_40:
		rtw89_write8_mask(rtwdev, rf_mod, B_AX_WMAC_RFMOD_MASK, BIT(0));
		rtw89_write32(rtwdev, sub_carr, txsc20);
		break;
	case RTW89_CHANNEL_WIDTH_20:
		rtw89_write8_clr(rtwdev, rf_mod, B_AX_WMAC_RFMOD_MASK);
		rtw89_write32(rtwdev, sub_carr, 0);
		break;
	default:
		break;
	}

	if (param->center_chan > 14)
		rtw89_write8_set(rtwdev, chk_rate,
				 B_AX_CHECK_CCK_EN | B_AX_RTS_LIMIT_IN_OFDM6);
	else
		rtw89_write8_clr(rtwdev, chk_rate,
				 B_AX_CHECK_CCK_EN | B_AX_RTS_LIMIT_IN_OFDM6);
}

static const u32 rtw8852a_sco_barker_threshold[14] = {
	0x1cfea, 0x1d0e1, 0x1d1d7, 0x1d2cd, 0x1d3c3, 0x1d4b9, 0x1d5b0, 0x1d6a6,
	0x1d79c, 0x1d892, 0x1d988, 0x1da7f, 0x1db75, 0x1ddc4
};

static const u32 rtw8852a_sco_cck_threshold[14] = {
	0x27de3, 0x27f35, 0x28088, 0x281da, 0x2832d, 0x2847f, 0x285d2, 0x28724,
	0x28877, 0x289c9, 0x28b1c, 0x28c6e, 0x28dc1, 0x290ed
};

static int rtw8852a_ctrl_sco_cck(struct rtw89_dev *rtwdev, u8 central_ch,
				 u8 primary_ch, enum rtw89_bandwidth bw)
{
	u8 ch_element;

	if (bw == RTW89_CHANNEL_WIDTH_20) {
		ch_element = central_ch - 1;
	} else if (bw == RTW89_CHANNEL_WIDTH_40) {
		if (primary_ch == 1)
			ch_element = central_ch - 1 + 2;
		else
			ch_element = central_ch - 1 - 2;
	} else {
		rtw89_warn(rtwdev, "Invalid BW:%d for CCK\n", bw);
		return -EINVAL;
	}
	rtw89_phy_write32_mask(rtwdev, R_RXSCOBC, B_RXSCOBC_TH,
			       rtw8852a_sco_barker_threshold[ch_element]);
	rtw89_phy_write32_mask(rtwdev, R_RXSCOCCK, B_RXSCOCCK_TH,
			       rtw8852a_sco_cck_threshold[ch_element]);

	return 0;
}

static void rtw8852a_ch_setting(struct rtw89_dev *rtwdev, u8 central_ch,
				u8 path)
{
	u32 val;

	val = rtw89_read_rf(rtwdev, path, RR_CFGCH, RFREG_MASK);
	if (val == INV_RF_DATA) {
		rtw89_warn(rtwdev, "Invalid RF_0x18 for Path-%d\n", path);
		return;
	}
	val &= ~0x303ff;
	val |= central_ch;
	if (central_ch > 14)
		val |= (BIT(16) | BIT(8));
	rtw89_write_rf(rtwdev, path, RR_CFGCH, RFREG_MASK, val);
}

static u8 rtw8852a_sco_mapping(u8 central_ch)
{
	if (central_ch == 1)
		return 109;
	else if (central_ch >= 2 && central_ch <= 6)
		return 108;
	else if (central_ch >= 7 && central_ch <= 10)
		return 107;
	else if (central_ch >= 11 && central_ch <= 14)
		return 106;
	else if (central_ch == 36 || central_ch == 38)
		return 51;
	else if (central_ch >= 40 && central_ch <= 58)
		return 50;
	else if (central_ch >= 60 && central_ch <= 64)
		return 49;
	else if (central_ch == 100 || central_ch == 102)
		return 48;
	else if (central_ch >= 104 && central_ch <= 126)
		return 47;
	else if (central_ch >= 128 && central_ch <= 151)
		return 46;
	else if (central_ch >= 153 && central_ch <= 177)
		return 45;
	else
		return 0;
}

static void rtw8852a_ctrl_ch(struct rtw89_dev *rtwdev, u8 central_ch,
			     enum rtw89_phy_idx phy_idx)
{
	u8 sco_comp;
	bool is_2g = central_ch <= 14;

	if (phy_idx == RTW89_PHY_0) {
		/* Path A */
		rtw8852a_ch_setting(rtwdev, central_ch, RF_PATH_A);
		if (is_2g)
			rtw89_phy_write32_idx(rtwdev, R_PATH0_TIA_ERR_G1,
					      B_PATH0_TIA_ERR_G1_SEL, 1,
					      phy_idx);
		else
			rtw89_phy_write32_idx(rtwdev, R_PATH0_TIA_ERR_G1,
					      B_PATH0_TIA_ERR_G1_SEL, 0,
					      phy_idx);

		/* Path B */
		if (!rtwdev->dbcc_en) {
			rtw8852a_ch_setting(rtwdev, central_ch, RF_PATH_B);
			if (is_2g)
				rtw89_phy_write32_idx(rtwdev, R_P1_MODE,
						      B_P1_MODE_SEL,
						      1, phy_idx);
			else
				rtw89_phy_write32_idx(rtwdev, R_P1_MODE,
						      B_P1_MODE_SEL,
						      0, phy_idx);
		} else {
			if (is_2g)
				rtw89_phy_write32_clr(rtwdev, R_2P4G_BAND,
						      B_2P4G_BAND_SEL);
			else
				rtw89_phy_write32_set(rtwdev, R_2P4G_BAND,
						      B_2P4G_BAND_SEL);
		}
		/* SCO compensate FC setting */
		sco_comp = rtw8852a_sco_mapping(central_ch);
		rtw89_phy_write32_idx(rtwdev, R_FC0_BW, B_FC0_BW_INV,
				      sco_comp, phy_idx);
	} else {
		/* Path B */
		rtw8852a_ch_setting(rtwdev, central_ch, RF_PATH_B);
		if (is_2g)
			rtw89_phy_write32_idx(rtwdev, R_P1_MODE,
					      B_P1_MODE_SEL,
					      1, phy_idx);
		else
			rtw89_phy_write32_idx(rtwdev, R_P1_MODE,
					      B_P1_MODE_SEL,
					      1, phy_idx);
		/* SCO compensate FC setting */
		sco_comp = rtw8852a_sco_mapping(central_ch);
		rtw89_phy_write32_idx(rtwdev, R_FC0_BW, B_FC0_BW_INV,
				      sco_comp, phy_idx);
	}

	/* Band edge */
	if (is_2g)
		rtw89_phy_write32_idx(rtwdev, R_BANDEDGE, B_BANDEDGE_EN, 1,
				      phy_idx);
	else
		rtw89_phy_write32_idx(rtwdev, R_BANDEDGE, B_BANDEDGE_EN, 0,
				      phy_idx);

	/* CCK parameters */
	if (central_ch == 14) {
		rtw89_phy_write32_mask(rtwdev, R_TXFIR0, B_TXFIR_C01,
				       0x3b13ff);
		rtw89_phy_write32_mask(rtwdev, R_TXFIR2, B_TXFIR_C23,
				       0x1c42de);
		rtw89_phy_write32_mask(rtwdev, R_TXFIR4, B_TXFIR_C45,
				       0xfdb0ad);
		rtw89_phy_write32_mask(rtwdev, R_TXFIR6, B_TXFIR_C67,
				       0xf60f6e);
		rtw89_phy_write32_mask(rtwdev, R_TXFIR8, B_TXFIR_C89,
				       0xfd8f92);
		rtw89_phy_write32_mask(rtwdev, R_TXFIRA, B_TXFIR_CAB, 0x2d011);
		rtw89_phy_write32_mask(rtwdev, R_TXFIRC, B_TXFIR_CCD, 0x1c02c);
		rtw89_phy_write32_mask(rtwdev, R_TXFIRE, B_TXFIR_CEF,
				       0xfff00a);
	} else {
		rtw89_phy_write32_mask(rtwdev, R_TXFIR0, B_TXFIR_C01,
				       0x3d23ff);
		rtw89_phy_write32_mask(rtwdev, R_TXFIR2, B_TXFIR_C23,
				       0x29b354);
		rtw89_phy_write32_mask(rtwdev, R_TXFIR4, B_TXFIR_C45, 0xfc1c8);
		rtw89_phy_write32_mask(rtwdev, R_TXFIR6, B_TXFIR_C67,
				       0xfdb053);
		rtw89_phy_write32_mask(rtwdev, R_TXFIR8, B_TXFIR_C89,
				       0xf86f9a);
		rtw89_phy_write32_mask(rtwdev, R_TXFIRA, B_TXFIR_CAB,
				       0xfaef92);
		rtw89_phy_write32_mask(rtwdev, R_TXFIRC, B_TXFIR_CCD,
				       0xfe5fcc);
		rtw89_phy_write32_mask(rtwdev, R_TXFIRE, B_TXFIR_CEF,
				       0xffdff5);
	}
}

static void rtw8852a_bw_setting(struct rtw89_dev *rtwdev, u8 bw, u8 path)
{
	u32 val = 0;
	u32 adc_sel[2] = {0x12d0, 0x32d0};
	u32 wbadc_sel[2] = {0x12ec, 0x32ec};

	val = rtw89_read_rf(rtwdev, path, RR_CFGCH, RFREG_MASK);
	if (val == INV_RF_DATA) {
		rtw89_warn(rtwdev, "Invalid RF_0x18 for Path-%d\n", path);
		return;
	}
	val &= ~(BIT(11) | BIT(10));
	switch (bw) {
	case RTW89_CHANNEL_WIDTH_5:
		rtw89_phy_write32_mask(rtwdev, adc_sel[path], 0x6000, 0x1);
		rtw89_phy_write32_mask(rtwdev, wbadc_sel[path], 0x30, 0x0);
		val |= (BIT(11) | BIT(10));
		break;
	case RTW89_CHANNEL_WIDTH_10:
		rtw89_phy_write32_mask(rtwdev, adc_sel[path], 0x6000, 0x2);
		rtw89_phy_write32_mask(rtwdev, wbadc_sel[path], 0x30, 0x1);
		val |= (BIT(11) | BIT(10));
		break;
	case RTW89_CHANNEL_WIDTH_20:
		rtw89_phy_write32_mask(rtwdev, adc_sel[path], 0x6000, 0x0);
		rtw89_phy_write32_mask(rtwdev, wbadc_sel[path], 0x30, 0x2);
		val |= (BIT(11) | BIT(10));
		break;
	case RTW89_CHANNEL_WIDTH_40:
		rtw89_phy_write32_mask(rtwdev, adc_sel[path], 0x6000, 0x0);
		rtw89_phy_write32_mask(rtwdev, wbadc_sel[path], 0x30, 0x2);
		val |= BIT(11);
		break;
	case RTW89_CHANNEL_WIDTH_80:
		rtw89_phy_write32_mask(rtwdev, adc_sel[path], 0x6000, 0x0);
		rtw89_phy_write32_mask(rtwdev, wbadc_sel[path], 0x30, 0x2);
		val |= BIT(10);
		break;
	default:
		rtw89_warn(rtwdev, "Fail to set ADC\n");
	}

	rtw89_write_rf(rtwdev, path, RR_CFGCH, RFREG_MASK, val);
}

static void
rtw8852a_ctrl_bw(struct rtw89_dev *rtwdev, u8 pri_ch, u8 bw,
		 enum rtw89_phy_idx phy_idx)
{
	/* Switch bandwidth */
	switch (bw) {
	case RTW89_CHANNEL_WIDTH_5:
		rtw89_phy_write32_idx(rtwdev, R_FC0_BW, B_FC0_BW_SET, 0x0,
				      phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_CHBW_MOD, B_CHBW_MOD_SBW, 0x1,
				      phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_CHBW_MOD, B_CHBW_MOD_PRICH,
				      0x0, phy_idx);
		break;
	case RTW89_CHANNEL_WIDTH_10:
		rtw89_phy_write32_idx(rtwdev, R_FC0_BW, B_FC0_BW_SET, 0x0,
				      phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_CHBW_MOD, B_CHBW_MOD_SBW, 0x2,
				      phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_CHBW_MOD, B_CHBW_MOD_PRICH,
				      0x0, phy_idx);
		break;
	case RTW89_CHANNEL_WIDTH_20:
		rtw89_phy_write32_idx(rtwdev, R_FC0_BW, B_FC0_BW_SET, 0x0,
				      phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_CHBW_MOD, B_CHBW_MOD_SBW, 0x0,
				      phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_CHBW_MOD, B_CHBW_MOD_PRICH,
				      0x0, phy_idx);
		break;
	case RTW89_CHANNEL_WIDTH_40:
		rtw89_phy_write32_idx(rtwdev, R_FC0_BW, B_FC0_BW_SET, 0x1,
				      phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_CHBW_MOD, B_CHBW_MOD_SBW, 0x0,
				      phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_CHBW_MOD, B_CHBW_MOD_PRICH,
				      pri_ch,
				      phy_idx);
		if (pri_ch == RTW89_SC_20_UPPER)
			rtw89_phy_write32_mask(rtwdev, R_RXSC, B_RXSC_EN, 1);
		else
			rtw89_phy_write32_mask(rtwdev, R_RXSC, B_RXSC_EN, 0);
		break;
	case RTW89_CHANNEL_WIDTH_80:
		rtw89_phy_write32_idx(rtwdev, R_FC0_BW, B_FC0_BW_SET, 0x2,
				      phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_CHBW_MOD, B_CHBW_MOD_SBW, 0x0,
				      phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_CHBW_MOD, B_CHBW_MOD_PRICH,
				      pri_ch,
				      phy_idx);
		break;
	default:
		rtw89_warn(rtwdev, "Fail to switch bw (bw:%d, pri ch:%d)\n", bw,
			   pri_ch);
	}

	if (phy_idx == RTW89_PHY_0) {
		rtw8852a_bw_setting(rtwdev, bw, RF_PATH_A);
		if (!rtwdev->dbcc_en)
			rtw8852a_bw_setting(rtwdev, bw, RF_PATH_B);
	} else {
		rtw8852a_bw_setting(rtwdev, bw, RF_PATH_B);
	}
}

static void rtw8852a_spur_elimination(struct rtw89_dev *rtwdev, u8 central_ch)
{
	if (central_ch == 153) {
		rtw89_phy_write32_mask(rtwdev, R_P0_NBIIDX, B_P0_NBIIDX_VAL,
				       0x210);
		rtw89_phy_write32_mask(rtwdev, R_P1_NBIIDX, B_P1_NBIIDX_VAL,
				       0x210);
		rtw89_phy_write32_mask(rtwdev, R_SEG0CSI, 0xfff, 0x7c0);
		rtw89_phy_write32_mask(rtwdev, R_P0_NBIIDX,
				       B_P0_NBIIDX_NOTCH_EN, 0x1);
		rtw89_phy_write32_mask(rtwdev, R_P1_NBIIDX,
				       B_P1_NBIIDX_NOTCH_EN, 0x1);
		rtw89_phy_write32_mask(rtwdev, R_SEG0CSI_EN, B_SEG0CSI_EN,
				       0x1);
	} else if (central_ch == 151) {
		rtw89_phy_write32_mask(rtwdev, R_P0_NBIIDX, B_P0_NBIIDX_VAL,
				       0x210);
		rtw89_phy_write32_mask(rtwdev, R_P1_NBIIDX, B_P1_NBIIDX_VAL,
				       0x210);
		rtw89_phy_write32_mask(rtwdev, R_SEG0CSI, 0xfff, 0x40);
		rtw89_phy_write32_mask(rtwdev, R_P0_NBIIDX,
				       B_P0_NBIIDX_NOTCH_EN, 0x1);
		rtw89_phy_write32_mask(rtwdev, R_P1_NBIIDX,
				       B_P1_NBIIDX_NOTCH_EN, 0x1);
		rtw89_phy_write32_mask(rtwdev, R_SEG0CSI_EN, B_SEG0CSI_EN,
				       0x1);
	} else if (central_ch == 155) {
		rtw89_phy_write32_mask(rtwdev, R_P0_NBIIDX, B_P0_NBIIDX_VAL,
				       0x2d0);
		rtw89_phy_write32_mask(rtwdev, R_P1_NBIIDX, B_P1_NBIIDX_VAL,
				       0x2d0);
		rtw89_phy_write32_mask(rtwdev, R_SEG0CSI, 0xfff, 0x740);
		rtw89_phy_write32_mask(rtwdev, R_P0_NBIIDX,
				       B_P0_NBIIDX_NOTCH_EN, 0x1);
		rtw89_phy_write32_mask(rtwdev, R_P1_NBIIDX,
				       B_P1_NBIIDX_NOTCH_EN, 0x1);
		rtw89_phy_write32_mask(rtwdev, R_SEG0CSI_EN, B_SEG0CSI_EN,
				       0x1);
	} else {
		rtw89_phy_write32_mask(rtwdev, R_P0_NBIIDX,
				       B_P0_NBIIDX_NOTCH_EN, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_P1_NBIIDX,
				       B_P1_NBIIDX_NOTCH_EN, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_SEG0CSI_EN, B_SEG0CSI_EN,
				       0x0);
	}
}

static void rtw8852a_bb_reset_all(struct rtw89_dev *rtwdev,
				  enum rtw89_phy_idx phy_idx)
{
	rtw89_phy_write32_idx(rtwdev, R_RSTB_ASYNC, B_RSTB_ASYNC_ALL, 1,
			      phy_idx);
	rtw89_phy_write32_idx(rtwdev, R_RSTB_ASYNC, B_RSTB_ASYNC_ALL, 0,
			      phy_idx);
	rtw89_phy_write32_idx(rtwdev, R_RSTB_ASYNC, B_RSTB_ASYNC_ALL, 1,
			      phy_idx);
}

static void rtw8852a_bb_reset_en(struct rtw89_dev *rtwdev,
				 enum rtw89_phy_idx phy_idx, bool en)
{
	if (en)
		rtw89_phy_write32_idx(rtwdev, R_RSTB_ASYNC, B_RSTB_ASYNC_ALL,
				      1,
				      phy_idx);
	else
		rtw89_phy_write32_idx(rtwdev, R_RSTB_ASYNC, B_RSTB_ASYNC_ALL,
				      0,
				      phy_idx);
}

static void rtw8852a_bb_reset(struct rtw89_dev *rtwdev,
			      enum rtw89_phy_idx phy_idx)
{
	rtw89_phy_write32_set(rtwdev, R_P0_TXPW_RSTB, B_P0_TXPW_RSTB_MANON);
	rtw89_phy_write32_set(rtwdev, R_P0_TSSI_TRK, B_P0_TSSI_TRK_EN);
	rtw89_phy_write32_set(rtwdev, R_P1_TXPW_RSTB, B_P1_TXPW_RSTB_MANON);
	rtw89_phy_write32_set(rtwdev, R_P1_TSSI_TRK, B_P1_TSSI_TRK_EN);
	rtw8852a_bb_reset_all(rtwdev, phy_idx);
	rtw89_phy_write32_clr(rtwdev, R_P0_TXPW_RSTB, B_P0_TXPW_RSTB_MANON);
	rtw89_phy_write32_clr(rtwdev, R_P0_TSSI_TRK, B_P0_TSSI_TRK_EN);
	rtw89_phy_write32_clr(rtwdev, R_P1_TXPW_RSTB, B_P1_TXPW_RSTB_MANON);
	rtw89_phy_write32_clr(rtwdev, R_P1_TSSI_TRK, B_P1_TSSI_TRK_EN);
}

static void rtw8852a_bb_sethw(struct rtw89_dev *rtwdev)
{
	rtw89_phy_write32_clr(rtwdev, R_P0_EN_SOUND_WO_NDP, B_P0_EN_SOUND_WO_NDP);
	rtw89_phy_write32_clr(rtwdev, R_P1_EN_SOUND_WO_NDP, B_P1_EN_SOUND_WO_NDP);

	if (rtwdev->hal.cut_version <= CHIP_CUT_C) {
		rtw89_phy_write32_set(rtwdev, R_RSTB_WATCH_DOG, B_P0_RSTB_WATCH_DOG);
		rtw89_phy_write32(rtwdev, R_BRK_ASYNC_RST_EN_1, 0x864FA000);
		rtw89_phy_write32(rtwdev, R_BRK_ASYNC_RST_EN_2, 0x3F);
		rtw89_phy_write32(rtwdev, R_BRK_ASYNC_RST_EN_3, 0x7FFF);
		rtw89_phy_write32_set(rtwdev, R_SPOOF_ASYNC_RST, B_SPOOF_ASYNC_RST);
		rtw89_phy_write32_set(rtwdev, R_P0_TXPW_RSTB, B_P0_TXPW_RSTB_MANON);
		rtw89_phy_write32_set(rtwdev, R_P1_TXPW_RSTB, B_P1_TXPW_RSTB_MANON);
	}
}

static void rtw8852a_set_channel_bb(struct rtw89_dev *rtwdev,
				    struct rtw89_channel_params *param,
				    enum rtw89_phy_idx phy_idx)
{
	bool cck_en = param->center_chan > 14 ? false : true;
	u8 pri_ch_idx = param->pri_ch_idx;

	if (param->center_chan <= 14)
		rtw8852a_ctrl_sco_cck(rtwdev, param->center_chan,
				      param->primary_chan, param->bandwidth);

	rtw8852a_ctrl_ch(rtwdev, param->center_chan, phy_idx);
	rtw8852a_ctrl_bw(rtwdev, pri_ch_idx, param->bandwidth, phy_idx);
	if (cck_en)
		rtw89_phy_write32_mask(rtwdev, R_RXCCA, B_RXCCA_DIS, 0);
	else
		rtw89_phy_write32_mask(rtwdev, R_RXCCA, B_RXCCA_DIS, 1);
	rtw8852a_spur_elimination(rtwdev, param->center_chan);
	rtw8852a_bb_reset_all(rtwdev, phy_idx);
}

static void rtw8852a_set_channel(struct rtw89_dev *rtwdev,
				 struct rtw89_channel_params *params)
{
	rtw8852a_set_channel_mac(rtwdev, params, RTW89_MAC_0);
	rtw8852a_set_channel_bb(rtwdev, params, RTW89_PHY_0);
}

static void rtw8852a_dfs_en(struct rtw89_dev *rtwdev, bool en)
{
	if (en)
		rtw89_phy_write32_mask(rtwdev, R_UPD_P0, B_UPD_P0_EN, 1);
	else
		rtw89_phy_write32_mask(rtwdev, R_UPD_P0, B_UPD_P0_EN, 0);
}

static void rtw8852a_tssi_cont_en(struct rtw89_dev *rtwdev, bool en,
				  enum rtw89_rf_path path)
{
	static const u32 tssi_trk[2] = {0x5818, 0x7818};
	static const u32 ctrl_bbrst[2] = {0x58dc, 0x78dc};

	if (en) {
		rtw89_phy_write32_mask(rtwdev, ctrl_bbrst[path], BIT(30), 0x0);
		rtw89_phy_write32_mask(rtwdev, tssi_trk[path], BIT(30), 0x0);
	} else {
		rtw89_phy_write32_mask(rtwdev, ctrl_bbrst[path], BIT(30), 0x1);
		rtw89_phy_write32_mask(rtwdev, tssi_trk[path], BIT(30), 0x1);
	}
}

static void rtw8852a_tssi_cont_en_phyidx(struct rtw89_dev *rtwdev, bool en,
					 u8 phy_idx)
{
	if (!rtwdev->dbcc_en) {
		rtw8852a_tssi_cont_en(rtwdev, en, RF_PATH_A);
		rtw8852a_tssi_cont_en(rtwdev, en, RF_PATH_B);
	} else {
		if (phy_idx == RTW89_PHY_0)
			rtw8852a_tssi_cont_en(rtwdev, en, RF_PATH_A);
		else
			rtw8852a_tssi_cont_en(rtwdev, en, RF_PATH_B);
	}
}

static void rtw8852a_adc_en(struct rtw89_dev *rtwdev, bool en)
{
	if (en)
		rtw89_phy_write32_mask(rtwdev, R_ADC_FIFO, B_ADC_FIFO_RST,
				       0x0);
	else
		rtw89_phy_write32_mask(rtwdev, R_ADC_FIFO, B_ADC_FIFO_RST,
				       0xf);
}

static void rtw8852a_set_channel_help(struct rtw89_dev *rtwdev, bool enter,
				      struct rtw89_channel_help_params *p)
{
	u8 phy_idx = RTW89_PHY_0;

	if (enter) {
		rtw89_mac_stop_sch_tx(rtwdev, RTW89_MAC_0, &p->tx_en, RTW89_SCH_TX_SEL_ALL);
		rtw89_mac_cfg_ppdu_status(rtwdev, RTW89_MAC_0, false);
		rtw8852a_dfs_en(rtwdev, false);
		rtw8852a_tssi_cont_en_phyidx(rtwdev, false, RTW89_PHY_0);
		rtw8852a_adc_en(rtwdev, false);
		fsleep(40);
		rtw8852a_bb_reset_en(rtwdev, phy_idx, false);
	} else {
		rtw89_mac_cfg_ppdu_status(rtwdev, RTW89_MAC_0, true);
		rtw8852a_adc_en(rtwdev, true);
		rtw8852a_dfs_en(rtwdev, true);
		rtw8852a_tssi_cont_en_phyidx(rtwdev, true, RTW89_PHY_0);
		rtw8852a_bb_reset_en(rtwdev, phy_idx, true);
		rtw89_mac_resume_sch_tx(rtwdev, RTW89_MAC_0, p->tx_en);
	}
}

static void rtw8852a_fem_setup(struct rtw89_dev *rtwdev)
{
	struct rtw89_efuse *efuse = &rtwdev->efuse;

	switch (efuse->rfe_type) {
	case 11:
	case 12:
	case 17:
	case 18:
	case 51:
	case 53:
		rtwdev->fem.epa_2g = true;
		rtwdev->fem.elna_2g = true;
		fallthrough;
	case 9:
	case 10:
	case 15:
	case 16:
		rtwdev->fem.epa_5g = true;
		rtwdev->fem.elna_5g = true;
		break;
	default:
		break;
	}
}

static void rtw8852a_rfk_init(struct rtw89_dev *rtwdev)
{
	rtwdev->is_tssi_mode[RF_PATH_A] = false;
	rtwdev->is_tssi_mode[RF_PATH_B] = false;

	rtw8852a_rck(rtwdev);
	rtw8852a_dack(rtwdev);
	rtw8852a_rx_dck(rtwdev, RTW89_PHY_0, true);
}

static void rtw8852a_rfk_channel(struct rtw89_dev *rtwdev)
{
	enum rtw89_phy_idx phy_idx = RTW89_PHY_0;

	rtw8852a_rx_dck(rtwdev, phy_idx, true);
	rtw8852a_iqk(rtwdev, phy_idx);
	rtw8852a_tssi(rtwdev, phy_idx);
	rtw8852a_dpk(rtwdev, phy_idx);
}

static void rtw8852a_rfk_track(struct rtw89_dev *rtwdev)
{
	rtw8852a_dpk_track(rtwdev);
	rtw8852a_iqk_track(rtwdev);
}

static u32 rtw8852a_bb_cal_txpwr_ref(struct rtw89_dev *rtwdev,
				     enum rtw89_phy_idx phy_idx, s16 ref)
{
	s8 ofst_int = 0;
	u8 base_cw_0db = 0x27;
	u16 tssi_16dbm_cw = 0x12c;
	s16 pwr_s10_3 = 0;
	s16 rf_pwr_cw = 0;
	u16 bb_pwr_cw = 0;
	u32 pwr_cw = 0;
	u32 tssi_ofst_cw = 0;

	pwr_s10_3 = (ref << 1) + (s16)(ofst_int) + (s16)(base_cw_0db << 3);
	bb_pwr_cw = FIELD_GET(GENMASK(2, 0), pwr_s10_3);
	rf_pwr_cw = FIELD_GET(GENMASK(8, 3), pwr_s10_3);
	rf_pwr_cw = clamp_t(s16, rf_pwr_cw, 15, 63);
	pwr_cw = (rf_pwr_cw << 3) | bb_pwr_cw;

	tssi_ofst_cw = (u32)((s16)tssi_16dbm_cw + (ref << 1) - (16 << 3));
	rtw89_debug(rtwdev, RTW89_DBG_TXPWR,
		    "[TXPWR] tssi_ofst_cw=%d rf_cw=0x%x bb_cw=0x%x\n",
		    tssi_ofst_cw, rf_pwr_cw, bb_pwr_cw);

	return (tssi_ofst_cw << 18) | (pwr_cw << 9) | (ref & GENMASK(8, 0));
}

static void rtw8852a_set_txpwr_ref(struct rtw89_dev *rtwdev,
				   enum rtw89_phy_idx phy_idx)
{
	static const u32 addr[RF_PATH_NUM_8852A] = {0x5800, 0x7800};
	const u32 mask = 0x7FFFFFF;
	const u8 ofst_ofdm = 0x4;
	const u8 ofst_cck = 0x8;
	s16 ref_ofdm = 0;
	s16 ref_cck = 0;
	u32 val;
	u8 i;

	rtw89_debug(rtwdev, RTW89_DBG_TXPWR, "[TXPWR] set txpwr reference\n");

	rtw89_mac_txpwr_write32_mask(rtwdev, phy_idx, R_AX_PWR_RATE_CTRL,
				     GENMASK(27, 10), 0x0);

	rtw89_debug(rtwdev, RTW89_DBG_TXPWR, "[TXPWR] set bb ofdm txpwr ref\n");
	val = rtw8852a_bb_cal_txpwr_ref(rtwdev, phy_idx, ref_ofdm);

	for (i = 0; i < RF_PATH_NUM_8852A; i++)
		rtw89_phy_write32_idx(rtwdev, addr[i] + ofst_ofdm, mask, val,
				      phy_idx);

	rtw89_debug(rtwdev, RTW89_DBG_TXPWR, "[TXPWR] set bb cck txpwr ref\n");
	val = rtw8852a_bb_cal_txpwr_ref(rtwdev, phy_idx, ref_cck);

	for (i = 0; i < RF_PATH_NUM_8852A; i++)
		rtw89_phy_write32_idx(rtwdev, addr[i] + ofst_cck, mask, val,
				      phy_idx);
}

static void rtw8852a_set_txpwr_byrate(struct rtw89_dev *rtwdev,
				      enum rtw89_phy_idx phy_idx)
{
	u8 ch = rtwdev->hal.current_channel;
	static const u8 rs[] = {
		RTW89_RS_CCK,
		RTW89_RS_OFDM,
		RTW89_RS_MCS,
		RTW89_RS_HEDCM,
	};
	s8 tmp;
	u8 i, j;
	u32 val, shf, addr = R_AX_PWR_BY_RATE;
	struct rtw89_rate_desc cur;

	rtw89_debug(rtwdev, RTW89_DBG_TXPWR,
		    "[TXPWR] set txpwr byrate with ch=%d\n", ch);

	for (cur.nss = 0; cur.nss <= RTW89_NSS_2; cur.nss++) {
		for (i = 0; i < ARRAY_SIZE(rs); i++) {
			if (cur.nss >= rtw89_rs_nss_max[rs[i]])
				continue;

			val = 0;
			cur.rs = rs[i];

			for (j = 0; j < rtw89_rs_idx_max[rs[i]]; j++) {
				cur.idx = j;
				shf = (j % 4) * 8;
				tmp = rtw89_phy_read_txpwr_byrate(rtwdev, &cur);
				val |= (tmp << shf);

				if ((j + 1) % 4)
					continue;

				rtw89_mac_txpwr_write32(rtwdev, phy_idx, addr, val);
				val = 0;
				addr += 4;
			}
		}
	}
}

static void rtw8852a_set_txpwr_offset(struct rtw89_dev *rtwdev,
				      enum rtw89_phy_idx phy_idx)
{
	struct rtw89_rate_desc desc = {
		.nss = RTW89_NSS_1,
		.rs = RTW89_RS_OFFSET,
	};
	u32 val = 0;
	s8 v;

	rtw89_debug(rtwdev, RTW89_DBG_TXPWR, "[TXPWR] set txpwr offset\n");

	for (desc.idx = 0; desc.idx < RTW89_RATE_OFFSET_MAX; desc.idx++) {
		v = rtw89_phy_read_txpwr_byrate(rtwdev, &desc);
		val |= ((v & 0xf) << (4 * desc.idx));
	}

	rtw89_mac_txpwr_write32_mask(rtwdev, phy_idx, R_AX_PWR_RATE_OFST_CTRL,
				     GENMASK(19, 0), val);
}

static void rtw8852a_set_txpwr_limit(struct rtw89_dev *rtwdev,
				     enum rtw89_phy_idx phy_idx)
{
#define __MAC_TXPWR_LMT_PAGE_SIZE 40
	u8 ch = rtwdev->hal.current_channel;
	u8 bw = rtwdev->hal.current_band_width;
	struct rtw89_txpwr_limit lmt[NTX_NUM_8852A];
	u32 addr, val;
	const s8 *ptr;
	u8 i, j, k;

	rtw89_debug(rtwdev, RTW89_DBG_TXPWR,
		    "[TXPWR] set txpwr limit with ch=%d bw=%d\n", ch, bw);

	for (i = 0; i < NTX_NUM_8852A; i++) {
		rtw89_phy_fill_txpwr_limit(rtwdev, &lmt[i], i);

		for (j = 0; j < __MAC_TXPWR_LMT_PAGE_SIZE; j += 4) {
			addr = R_AX_PWR_LMT + j + __MAC_TXPWR_LMT_PAGE_SIZE * i;
			ptr = (s8 *)&lmt[i] + j;
			val = 0;

			for (k = 0; k < 4; k++)
				val |= (ptr[k] << (8 * k));

			rtw89_mac_txpwr_write32(rtwdev, phy_idx, addr, val);
		}
	}
#undef __MAC_TXPWR_LMT_PAGE_SIZE
}

static void rtw8852a_set_txpwr_limit_ru(struct rtw89_dev *rtwdev,
					enum rtw89_phy_idx phy_idx)
{
#define __MAC_TXPWR_LMT_RU_PAGE_SIZE 24
	u8 ch = rtwdev->hal.current_channel;
	u8 bw = rtwdev->hal.current_band_width;
	struct rtw89_txpwr_limit_ru lmt_ru[NTX_NUM_8852A];
	u32 addr, val;
	const s8 *ptr;
	u8 i, j, k;

	rtw89_debug(rtwdev, RTW89_DBG_TXPWR,
		    "[TXPWR] set txpwr limit ru with ch=%d bw=%d\n", ch, bw);

	for (i = 0; i < NTX_NUM_8852A; i++) {
		rtw89_phy_fill_txpwr_limit_ru(rtwdev, &lmt_ru[i], i);

		for (j = 0; j < __MAC_TXPWR_LMT_RU_PAGE_SIZE; j += 4) {
			addr = R_AX_PWR_RU_LMT + j +
			       __MAC_TXPWR_LMT_RU_PAGE_SIZE * i;
			ptr = (s8 *)&lmt_ru[i] + j;
			val = 0;

			for (k = 0; k < 4; k++)
				val |= (ptr[k] << (8 * k));

			rtw89_mac_txpwr_write32(rtwdev, phy_idx, addr, val);
		}
	}

#undef __MAC_TXPWR_LMT_RU_PAGE_SIZE
}

static void rtw8852a_set_txpwr(struct rtw89_dev *rtwdev)
{
	rtw8852a_set_txpwr_byrate(rtwdev, RTW89_PHY_0);
	rtw8852a_set_txpwr_limit(rtwdev, RTW89_PHY_0);
	rtw8852a_set_txpwr_limit_ru(rtwdev, RTW89_PHY_0);
}

static void rtw8852a_set_txpwr_ctrl(struct rtw89_dev *rtwdev)
{
	rtw8852a_set_txpwr_ref(rtwdev, RTW89_PHY_0);
	rtw8852a_set_txpwr_offset(rtwdev, RTW89_PHY_0);
}

static int
rtw8852a_init_txpwr_unit(struct rtw89_dev *rtwdev, enum rtw89_phy_idx phy_idx)
{
	int ret;

	ret = rtw89_mac_txpwr_write32(rtwdev, phy_idx, R_AX_PWR_UL_CTRL2, 0x07763333);
	if (ret)
		return ret;

	ret = rtw89_mac_txpwr_write32(rtwdev, phy_idx, R_AX_PWR_COEXT_CTRL, 0x01ebf004);
	if (ret)
		return ret;

	return 0;
}

static u8 rtw8852a_get_thermal(struct rtw89_dev *rtwdev, enum rtw89_rf_path rf_path)
{
	if (rtwdev->is_tssi_mode[rf_path]) {
		u32 addr = 0x1c10 + (rf_path << 13);

		return (u8)rtw89_phy_read32_mask(rtwdev, addr, 0x3F000000);
	}

	rtw89_write_rf(rtwdev, rf_path, RR_TM, RR_TM_TRI, 0x1);
	rtw89_write_rf(rtwdev, rf_path, RR_TM, RR_TM_TRI, 0x0);
	rtw89_write_rf(rtwdev, rf_path, RR_TM, RR_TM_TRI, 0x1);

	fsleep(200);

	return (u8)rtw89_read_rf(rtwdev, rf_path, RR_TM, RR_TM_VAL);
}

static void rtw8852a_btc_set_rfe(struct rtw89_dev *rtwdev)
{
	struct rtw89_btc *btc = &rtwdev->btc;
	struct rtw89_btc_module *module = &btc->mdinfo;

	module->rfe_type = rtwdev->efuse.rfe_type;
	module->kt_ver = rtwdev->hal.cut_version;
	module->bt_solo = 0;
	module->switch_type = BTC_SWITCH_INTERNAL;

	if (module->rfe_type > 0)
		module->ant.num = (module->rfe_type % 2 ? 2 : 3);
	else
		module->ant.num = 2;

	module->ant.diversity = 0;
	module->ant.isolation = 10;

	if (module->ant.num == 3) {
		module->ant.type = BTC_ANT_DEDICATED;
		module->bt_pos = BTC_BT_ALONE;
	} else {
		module->ant.type = BTC_ANT_SHARED;
		module->bt_pos = BTC_BT_BTG;
	}
}

static
void rtw8852a_set_trx_mask(struct rtw89_dev *rtwdev, u8 path, u8 group, u32 val)
{
	rtw89_write_rf(rtwdev, path, RR_LUTWE, 0xfffff, 0x20000);
	rtw89_write_rf(rtwdev, path, RR_LUTWA, 0xfffff, group);
	rtw89_write_rf(rtwdev, path, RR_LUTWD0, 0xfffff, val);
	rtw89_write_rf(rtwdev, path, RR_LUTWE, 0xfffff, 0x0);
}

static void rtw8852a_ctrl_btg(struct rtw89_dev *rtwdev, bool btg)
{
	if (btg) {
		rtw89_phy_write32_mask(rtwdev, R_PATH0_BTG, B_PATH0_BTG_SHEN, 0x1);
		rtw89_phy_write32_mask(rtwdev, R_PATH1_BTG, B_PATH1_BTG_SHEN, 0x3);
		rtw89_phy_write32_mask(rtwdev, R_PMAC_GNT, B_PMAC_GNT_P1, 0x0);
	} else {
		rtw89_phy_write32_mask(rtwdev, R_PATH0_BTG, B_PATH0_BTG_SHEN, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_PATH1_BTG, B_PATH1_BTG_SHEN, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_PMAC_GNT, B_PMAC_GNT_P1, 0xf);
		rtw89_phy_write32_mask(rtwdev, R_PMAC_GNT, B_PMAC_GNT_P2, 0x4);
	}
}

static void rtw8852a_btc_init_cfg(struct rtw89_dev *rtwdev)
{
	struct rtw89_btc *btc = &rtwdev->btc;
	struct rtw89_btc_module *module = &btc->mdinfo;
	const struct rtw89_chip_info *chip = rtwdev->chip;
	const struct rtw89_mac_ax_coex coex_params = {
		.pta_mode = RTW89_MAC_AX_COEX_RTK_MODE,
		.direction = RTW89_MAC_AX_COEX_INNER,
	};

	/* PTA init  */
	rtw89_mac_coex_init(rtwdev, &coex_params);

	/* set WL Tx response = Hi-Pri */
	chip->ops->btc_set_wl_pri(rtwdev, BTC_PRI_MASK_TX_RESP, true);

	/* set rf gnt debug off */
	rtw89_write_rf(rtwdev, RF_PATH_A, RR_WLSEL, 0xfffff, 0x0);
	rtw89_write_rf(rtwdev, RF_PATH_B, RR_WLSEL, 0xfffff, 0x0);

	/* set WL Tx thru in TRX mask table if GNT_WL = 0 && BT_S1 = ss group */
	if (module->ant.type == BTC_ANT_SHARED) {
		rtw8852a_set_trx_mask(rtwdev, RF_PATH_A, BTC_WTRX_SS_GROUP, 0x5ff);
		rtw8852a_set_trx_mask(rtwdev, RF_PATH_B, BTC_WTRX_SS_GROUP, 0x5ff);
	} else { /* set WL Tx stb if GNT_WL = 0 && BT_S1 = ss group for 3-ant */
		rtw8852a_set_trx_mask(rtwdev, RF_PATH_A, BTC_WTRX_SS_GROUP, 0x5df);
		rtw8852a_set_trx_mask(rtwdev, RF_PATH_B, BTC_WTRX_SS_GROUP, 0x5df);
	}

	/* set PTA break table */
	rtw89_write32(rtwdev, R_BTC_BREAK_TABLE, BTC_BREAK_PARAM);

	 /* enable BT counter 0xda40[16,2] = 2b'11 */
	rtw89_write32_set(rtwdev, R_AX_CSR_MODE, B_AX_BT_CNT_REST | B_AX_STATIS_BT_EN);
}

static
void rtw8852a_btc_set_wl_pri(struct rtw89_dev *rtwdev, u8 map, bool state)
{
	u32 bitmap = 0;

	switch (map) {
	case BTC_PRI_MASK_TX_RESP:
		bitmap = B_BTC_PRI_MASK_TX_RESP_V1;
		break;
	default:
		return;
	}

	if (state)
		rtw89_write32_set(rtwdev, R_BTC_BT_COEX_MSK_TABLE, bitmap);
	else
		rtw89_write32_clr(rtwdev, R_BTC_BT_COEX_MSK_TABLE, bitmap);
}

static void rtw8852a_query_ppdu(struct rtw89_dev *rtwdev,
				struct rtw89_rx_phy_ppdu *phy_ppdu,
				struct ieee80211_rx_status *status)
{
	u8 path;
	s8 *rx_power = phy_ppdu->rssi;

	status->signal = max_t(s8, rx_power[RF_PATH_A], rx_power[RF_PATH_B]);
	for (path = 0; path < rtwdev->chip->rf_path_num; path++) {
		status->chains |= BIT(path);
		status->chain_signal[path] = rx_power[path];
	}
}

static const struct rtw89_chip_ops rtw8852a_chip_ops = {
	.bb_reset		= rtw8852a_bb_reset,
	.bb_sethw		= rtw8852a_bb_sethw,
	.read_rf		= rtw89_phy_read_rf,
	.write_rf		= rtw89_phy_write_rf,
	.set_channel		= rtw8852a_set_channel,
	.set_channel_help	= rtw8852a_set_channel_help,
	.read_efuse		= rtw8852a_read_efuse,
	.read_phycap		= rtw8852a_read_phycap,
	.fem_setup		= rtw8852a_fem_setup,
	.rfk_init		= rtw8852a_rfk_init,
	.rfk_channel		= rtw8852a_rfk_channel,
	.rfk_track		= rtw8852a_rfk_track,
	.power_trim		= rtw8852a_power_trim,
	.set_txpwr		= rtw8852a_set_txpwr,
	.set_txpwr_ctrl		= rtw8852a_set_txpwr_ctrl,
	.init_txpwr_unit	= rtw8852a_init_txpwr_unit,
	.get_thermal		= rtw8852a_get_thermal,
	.ctrl_btg		= rtw8852a_ctrl_btg,
	.query_ppdu		= rtw8852a_query_ppdu,

	.btc_set_rfe		= rtw8852a_btc_set_rfe,
	.btc_init_cfg		= rtw8852a_btc_init_cfg,
	.btc_set_wl_pri		= rtw8852a_btc_set_wl_pri,
};

const struct rtw89_chip_info rtw8852a_chip_info = {
	.chip_id		= RTL8852A,
	.ops			= &rtw8852a_chip_ops,
	.fw_name		= "rtw89/rtw8852a_fw.bin",
	.fifo_size		= 458752,
	.dle_lamode_size	= 262144,
	.max_amsdu_limit	= 3500,
	.hfc_param_ini		= rtw8852a_hfc_param_ini_pcie,
	.dle_mem		= rtw8852a_dle_mem_pcie,
	.rf_base_addr		= {0xc000, 0xd000},
	.bb_table		= &rtw89_8852a_phy_bb_table,
	.rf_table		= {&rtw89_8852a_phy_radioa_table,
				   &rtw89_8852a_phy_radiob_table,},
	.nctl_table		= &rtw89_8852a_phy_nctl_table,
	.byr_table		= &rtw89_8852a_byr_table,
	.txpwr_lmt_2g		= &rtw89_8852a_txpwr_lmt_2g,
	.txpwr_lmt_5g		= &rtw89_8852a_txpwr_lmt_5g,
	.txpwr_lmt_ru_2g	= &rtw89_8852a_txpwr_lmt_ru_2g,
	.txpwr_lmt_ru_5g	= &rtw89_8852a_txpwr_lmt_ru_5g,
	.txpwr_factor_rf	= 2,
	.txpwr_factor_mac	= 1,
	.dig_table		= &rtw89_8852a_phy_dig_table,
	.rf_path_num		= 2,
	.tx_nss			= 2,
	.rx_nss			= 2,
	.acam_num		= 128,
	.bcam_num		= 10,
	.scam_num		= 128,
	.sec_ctrl_efuse_size	= 4,
	.physical_efuse_size	= 1216,
	.logical_efuse_size	= 1536,
	.limit_efuse_size	= 1152,
	.phycap_addr		= 0x580,
	.phycap_size		= 128,
};
EXPORT_SYMBOL(rtw8852a_chip_info);

MODULE_FIRMWARE("rtw89/rtw8852a_fw.bin");
