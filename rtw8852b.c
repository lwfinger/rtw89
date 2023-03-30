// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright(c) 2019-2022  Realtek Corporation
 */

#include "coex.h"
#include "fw.h"
#include "mac.h"
#include "phy.h"
#include "reg.h"
#include "rtw8852b.h"
#include "rtw8852b_rfk.h"
#include "rtw8852b_table.h"
#include "txrx.h"

#define RTW8852B_FW_FORMAT_MAX 1
#define RTW8852B_FW_BASENAME "rtw89/rtw8852b_fw"
#define RTW8852B_MODULE_FIRMWARE \
	RTW8852B_FW_BASENAME "-" __stringify(RTW8852B_FW_FORMAT_MAX) ".bin"

static const struct rtw89_hfc_ch_cfg rtw8852b_hfc_chcfg_pcie[] = {
	{5, 343, grp_0}, /* ACH 0 */
	{5, 343, grp_0}, /* ACH 1 */
	{5, 343, grp_0}, /* ACH 2 */
	{5, 343, grp_0}, /* ACH 3 */
	{0, 0, grp_0}, /* ACH 4 */
	{0, 0, grp_0}, /* ACH 5 */
	{0, 0, grp_0}, /* ACH 6 */
	{0, 0, grp_0}, /* ACH 7 */
	{4, 344, grp_0}, /* B0MGQ */
	{4, 344, grp_0}, /* B0HIQ */
	{0, 0, grp_0}, /* B1MGQ */
	{0, 0, grp_0}, /* B1HIQ */
	{40, 0, 0} /* FWCMDQ */
};

static const struct rtw89_hfc_pub_cfg rtw8852b_hfc_pubcfg_pcie = {
	448, /* Group 0 */
	0, /* Group 1 */
	448, /* Public Max */
	0 /* WP threshold */
};

static const struct rtw89_hfc_param_ini rtw8852b_hfc_param_ini_pcie[] = {
	[RTW89_QTA_SCC] = {rtw8852b_hfc_chcfg_pcie, &rtw8852b_hfc_pubcfg_pcie,
			   &rtw89_mac_size.hfc_preccfg_pcie, RTW89_HCIFC_POH},
	[RTW89_QTA_DLFW] = {NULL, NULL, &rtw89_mac_size.hfc_preccfg_pcie,
			    RTW89_HCIFC_POH},
	[RTW89_QTA_INVALID] = {NULL},
};

static const struct rtw89_dle_mem rtw8852b_dle_mem_pcie[] = {
	[RTW89_QTA_SCC] = {RTW89_QTA_SCC, &rtw89_mac_size.wde_size6,
			   &rtw89_mac_size.ple_size6, &rtw89_mac_size.wde_qt6,
			   &rtw89_mac_size.wde_qt6, &rtw89_mac_size.ple_qt18,
			   &rtw89_mac_size.ple_qt58},
	[RTW89_QTA_DLFW] = {RTW89_QTA_DLFW, &rtw89_mac_size.wde_size9,
			    &rtw89_mac_size.ple_size8, &rtw89_mac_size.wde_qt4,
			    &rtw89_mac_size.wde_qt4, &rtw89_mac_size.ple_qt13,
			    &rtw89_mac_size.ple_qt13},
	[RTW89_QTA_INVALID] = {RTW89_QTA_INVALID, NULL, NULL, NULL, NULL, NULL,
			       NULL},
};

static const struct rtw89_reg3_def rtw8852b_pmac_ht20_mcs7_tbl[] = {
	{0x4580, 0x0000ffff, 0x0},
	{0x4580, 0xffff0000, 0x0},
	{0x4584, 0x0000ffff, 0x0},
	{0x4584, 0xffff0000, 0x0},
	{0x4580, 0x0000ffff, 0x1},
	{0x4578, 0x00ffffff, 0x2018b},
	{0x4570, 0x03ffffff, 0x7},
	{0x4574, 0x03ffffff, 0x32407},
	{0x45b8, 0x00000010, 0x0},
	{0x45b8, 0x00000100, 0x0},
	{0x45b8, 0x00000080, 0x0},
	{0x45b8, 0x00000008, 0x0},
	{0x45a0, 0x0000ff00, 0x0},
	{0x45a0, 0xff000000, 0x1},
	{0x45a4, 0x0000ff00, 0x2},
	{0x45a4, 0xff000000, 0x3},
	{0x45b8, 0x00000020, 0x0},
	{0x4568, 0xe0000000, 0x0},
	{0x45b8, 0x00000002, 0x1},
	{0x456c, 0xe0000000, 0x0},
	{0x45b4, 0x00006000, 0x0},
	{0x45b4, 0x00001800, 0x1},
	{0x45b8, 0x00000040, 0x0},
	{0x45b8, 0x00000004, 0x0},
	{0x45b8, 0x00000200, 0x0},
	{0x4598, 0xf8000000, 0x0},
	{0x45b8, 0x00100000, 0x0},
	{0x45a8, 0x00000fc0, 0x0},
	{0x45b8, 0x00200000, 0x0},
	{0x45b0, 0x00000038, 0x0},
	{0x45b0, 0x000001c0, 0x0},
	{0x45a0, 0x000000ff, 0x0},
	{0x45b8, 0x00400000, 0x0},
	{0x4590, 0x000007ff, 0x0},
	{0x45b0, 0x00000e00, 0x0},
	{0x45ac, 0x0000001f, 0x0},
	{0x45b8, 0x00800000, 0x0},
	{0x45a8, 0x0003f000, 0x0},
	{0x45b8, 0x01000000, 0x0},
	{0x45b0, 0x00007000, 0x0},
	{0x45b0, 0x00038000, 0x0},
	{0x45a0, 0x00ff0000, 0x0},
	{0x45b8, 0x02000000, 0x0},
	{0x4590, 0x003ff800, 0x0},
	{0x45b0, 0x001c0000, 0x0},
	{0x45ac, 0x000003e0, 0x0},
	{0x45b8, 0x04000000, 0x0},
	{0x45a8, 0x00fc0000, 0x0},
	{0x45b8, 0x08000000, 0x0},
	{0x45b0, 0x00e00000, 0x0},
	{0x45b0, 0x07000000, 0x0},
	{0x45a4, 0x000000ff, 0x0},
	{0x45b8, 0x10000000, 0x0},
	{0x4594, 0x000007ff, 0x0},
	{0x45b0, 0x38000000, 0x0},
	{0x45ac, 0x00007c00, 0x0},
	{0x45b8, 0x20000000, 0x0},
	{0x45a8, 0x3f000000, 0x0},
	{0x45b8, 0x40000000, 0x0},
	{0x45b4, 0x00000007, 0x0},
	{0x45b4, 0x00000038, 0x0},
	{0x45a4, 0x00ff0000, 0x0},
	{0x45b8, 0x80000000, 0x0},
	{0x4594, 0x003ff800, 0x0},
	{0x45b4, 0x000001c0, 0x0},
	{0x4598, 0xf8000000, 0x0},
	{0x45b8, 0x00100000, 0x0},
	{0x45a8, 0x00000fc0, 0x7},
	{0x45b8, 0x00200000, 0x0},
	{0x45b0, 0x00000038, 0x0},
	{0x45b0, 0x000001c0, 0x0},
	{0x45a0, 0x000000ff, 0x0},
	{0x45b4, 0x06000000, 0x0},
	{0x45b0, 0x00000007, 0x0},
	{0x45b8, 0x00080000, 0x0},
	{0x45a8, 0x0000003f, 0x0},
	{0x457c, 0xffe00000, 0x1},
	{0x4530, 0xffffffff, 0x0},
	{0x4588, 0x00003fff, 0x0},
	{0x4598, 0x000001ff, 0x0},
	{0x4534, 0xffffffff, 0x0},
	{0x4538, 0xffffffff, 0x0},
	{0x453c, 0xffffffff, 0x0},
	{0x4588, 0x0fffc000, 0x0},
	{0x4598, 0x0003fe00, 0x0},
	{0x4540, 0xffffffff, 0x0},
	{0x4544, 0xffffffff, 0x0},
	{0x4548, 0xffffffff, 0x0},
	{0x458c, 0x00003fff, 0x0},
	{0x4598, 0x07fc0000, 0x0},
	{0x454c, 0xffffffff, 0x0},
	{0x4550, 0xffffffff, 0x0},
	{0x4554, 0xffffffff, 0x0},
	{0x458c, 0x0fffc000, 0x0},
	{0x459c, 0x000001ff, 0x0},
	{0x4558, 0xffffffff, 0x0},
	{0x455c, 0xffffffff, 0x0},
	{0x4530, 0xffffffff, 0x4e790001},
	{0x4588, 0x00003fff, 0x0},
	{0x4598, 0x000001ff, 0x1},
	{0x4534, 0xffffffff, 0x0},
	{0x4538, 0xffffffff, 0x4b},
	{0x45ac, 0x38000000, 0x7},
	{0x4588, 0xf0000000, 0x0},
	{0x459c, 0x7e000000, 0x0},
	{0x45b8, 0x00040000, 0x0},
	{0x45b8, 0x00020000, 0x0},
	{0x4590, 0xffc00000, 0x0},
	{0x45b8, 0x00004000, 0x0},
	{0x4578, 0xff000000, 0x0},
	{0x45b8, 0x00000400, 0x0},
	{0x45b8, 0x00000800, 0x0},
	{0x45b8, 0x00001000, 0x0},
	{0x45b8, 0x00002000, 0x0},
	{0x45b4, 0x00018000, 0x0},
	{0x45ac, 0x07800000, 0x0},
	{0x45b4, 0x00000600, 0x2},
	{0x459c, 0x0001fe00, 0x80},
	{0x45ac, 0x00078000, 0x3},
	{0x459c, 0x01fe0000, 0x1},
};

static const struct rtw89_reg3_def rtw8852b_btc_preagc_en_defs[] = {
	{0x46D0, GENMASK(1, 0), 0x3},
	{0x4790, GENMASK(1, 0), 0x3},
	{0x4AD4, GENMASK(31, 0), 0xf},
	{0x4AE0, GENMASK(31, 0), 0xf},
	{0x4688, GENMASK(31, 24), 0x80},
	{0x476C, GENMASK(31, 24), 0x80},
	{0x4694, GENMASK(7, 0), 0x80},
	{0x4694, GENMASK(15, 8), 0x80},
	{0x4778, GENMASK(7, 0), 0x80},
	{0x4778, GENMASK(15, 8), 0x80},
	{0x4AE4, GENMASK(23, 0), 0x780D1E},
	{0x4AEC, GENMASK(23, 0), 0x780D1E},
	{0x469C, GENMASK(31, 26), 0x34},
	{0x49F0, GENMASK(31, 26), 0x34},
};

static DECLARE_PHY_REG3_TBL(rtw8852b_btc_preagc_en_defs);

static const struct rtw89_reg3_def rtw8852b_btc_preagc_dis_defs[] = {
	{0x46D0, GENMASK(1, 0), 0x0},
	{0x4790, GENMASK(1, 0), 0x0},
	{0x4AD4, GENMASK(31, 0), 0x60},
	{0x4AE0, GENMASK(31, 0), 0x60},
	{0x4688, GENMASK(31, 24), 0x1a},
	{0x476C, GENMASK(31, 24), 0x1a},
	{0x4694, GENMASK(7, 0), 0x2a},
	{0x4694, GENMASK(15, 8), 0x2a},
	{0x4778, GENMASK(7, 0), 0x2a},
	{0x4778, GENMASK(15, 8), 0x2a},
	{0x4AE4, GENMASK(23, 0), 0x79E99E},
	{0x4AEC, GENMASK(23, 0), 0x79E99E},
	{0x469C, GENMASK(31, 26), 0x26},
	{0x49F0, GENMASK(31, 26), 0x26},
};

static DECLARE_PHY_REG3_TBL(rtw8852b_btc_preagc_dis_defs);

static const u32 rtw8852b_h2c_regs[RTW89_H2CREG_MAX] = {
	R_AX_H2CREG_DATA0, R_AX_H2CREG_DATA1,  R_AX_H2CREG_DATA2,
	R_AX_H2CREG_DATA3
};

static const u32 rtw8852b_c2h_regs[RTW89_C2HREG_MAX] = {
	R_AX_C2HREG_DATA0, R_AX_C2HREG_DATA1, R_AX_C2HREG_DATA2,
	R_AX_C2HREG_DATA3
};

static const struct rtw89_page_regs rtw8852b_page_regs = {
	.hci_fc_ctrl	= R_AX_HCI_FC_CTRL,
	.ch_page_ctrl	= R_AX_CH_PAGE_CTRL,
	.ach_page_ctrl	= R_AX_ACH0_PAGE_CTRL,
	.ach_page_info	= R_AX_ACH0_PAGE_INFO,
	.pub_page_info3	= R_AX_PUB_PAGE_INFO3,
	.pub_page_ctrl1	= R_AX_PUB_PAGE_CTRL1,
	.pub_page_ctrl2	= R_AX_PUB_PAGE_CTRL2,
	.pub_page_info1	= R_AX_PUB_PAGE_INFO1,
	.pub_page_info2 = R_AX_PUB_PAGE_INFO2,
	.wp_page_ctrl1	= R_AX_WP_PAGE_CTRL1,
	.wp_page_ctrl2	= R_AX_WP_PAGE_CTRL2,
	.wp_page_info1	= R_AX_WP_PAGE_INFO1,
};

static const struct rtw89_reg_def rtw8852b_dcfo_comp = {
	R_DCFO_COMP_S0, B_DCFO_COMP_S0_MSK
};

static const struct rtw89_imr_info rtw8852b_imr_info = {
	.wdrls_imr_set		= B_AX_WDRLS_IMR_SET,
	.wsec_imr_reg		= R_AX_SEC_DEBUG,
	.wsec_imr_set		= B_AX_IMR_ERROR,
	.mpdu_tx_imr_set	= 0,
	.mpdu_rx_imr_set	= 0,
	.sta_sch_imr_set	= B_AX_STA_SCHEDULER_IMR_SET,
	.txpktctl_imr_b0_reg	= R_AX_TXPKTCTL_ERR_IMR_ISR,
	.txpktctl_imr_b0_clr	= B_AX_TXPKTCTL_IMR_B0_CLR,
	.txpktctl_imr_b0_set	= B_AX_TXPKTCTL_IMR_B0_SET,
	.txpktctl_imr_b1_reg	= R_AX_TXPKTCTL_ERR_IMR_ISR_B1,
	.txpktctl_imr_b1_clr	= B_AX_TXPKTCTL_IMR_B1_CLR,
	.txpktctl_imr_b1_set	= B_AX_TXPKTCTL_IMR_B1_SET,
	.wde_imr_clr		= B_AX_WDE_IMR_CLR,
	.wde_imr_set		= B_AX_WDE_IMR_SET,
	.ple_imr_clr		= B_AX_PLE_IMR_CLR,
	.ple_imr_set		= B_AX_PLE_IMR_SET,
	.host_disp_imr_clr	= B_AX_HOST_DISP_IMR_CLR,
	.host_disp_imr_set	= B_AX_HOST_DISP_IMR_SET,
	.cpu_disp_imr_clr	= B_AX_CPU_DISP_IMR_CLR,
	.cpu_disp_imr_set	= B_AX_CPU_DISP_IMR_SET,
	.other_disp_imr_clr	= B_AX_OTHER_DISP_IMR_CLR,
	.other_disp_imr_set	= 0,
	.bbrpt_com_err_imr_reg	= R_AX_BBRPT_COM_ERR_IMR_ISR,
	.bbrpt_chinfo_err_imr_reg = R_AX_BBRPT_CHINFO_ERR_IMR_ISR,
	.bbrpt_err_imr_set	= 0,
	.bbrpt_dfs_err_imr_reg	= R_AX_BBRPT_DFS_ERR_IMR_ISR,
	.ptcl_imr_clr		= B_AX_PTCL_IMR_CLR_ALL,
	.ptcl_imr_set		= B_AX_PTCL_IMR_SET,
	.cdma_imr_0_reg		= R_AX_DLE_CTRL,
	.cdma_imr_0_clr		= B_AX_DLE_IMR_CLR,
	.cdma_imr_0_set		= B_AX_DLE_IMR_SET,
	.cdma_imr_1_reg		= 0,
	.cdma_imr_1_clr		= 0,
	.cdma_imr_1_set		= 0,
	.phy_intf_imr_reg	= R_AX_PHYINFO_ERR_IMR,
	.phy_intf_imr_clr	= 0,
	.phy_intf_imr_set	= 0,
	.rmac_imr_reg		= R_AX_RMAC_ERR_ISR,
	.rmac_imr_clr		= B_AX_RMAC_IMR_CLR,
	.rmac_imr_set		= B_AX_RMAC_IMR_SET,
	.tmac_imr_reg		= R_AX_TMAC_ERR_IMR_ISR,
	.tmac_imr_clr		= B_AX_TMAC_IMR_CLR,
	.tmac_imr_set		= B_AX_TMAC_IMR_SET,
};

static const struct rtw89_rrsr_cfgs rtw8852b_rrsr_cfgs = {
	.ref_rate = {R_AX_TRXPTCL_RRSR_CTL_0, B_AX_WMAC_RESP_REF_RATE_SEL, 0},
	.rsc = {R_AX_TRXPTCL_RRSR_CTL_0, B_AX_WMAC_RESP_RSC_MASK, 2},
};

static const struct rtw89_dig_regs rtw8852b_dig_regs = {
	.seg0_pd_reg = R_SEG0R_PD_V1,
	.pd_lower_bound_mask = B_SEG0R_PD_LOWER_BOUND_MSK,
	.pd_spatial_reuse_en = B_SEG0R_PD_SPATIAL_REUSE_EN_MSK_V1,
	.p0_lna_init = {R_PATH0_LNA_INIT_V1, B_PATH0_LNA_INIT_IDX_MSK},
	.p1_lna_init = {R_PATH1_LNA_INIT_V1, B_PATH1_LNA_INIT_IDX_MSK},
	.p0_tia_init = {R_PATH0_TIA_INIT_V1, B_PATH0_TIA_INIT_IDX_MSK_V1},
	.p1_tia_init = {R_PATH1_TIA_INIT_V1, B_PATH1_TIA_INIT_IDX_MSK_V1},
	.p0_rxb_init = {R_PATH0_RXB_INIT_V1, B_PATH0_RXB_INIT_IDX_MSK_V1},
	.p1_rxb_init = {R_PATH1_RXB_INIT_V1, B_PATH1_RXB_INIT_IDX_MSK_V1},
	.p0_p20_pagcugc_en = {R_PATH0_P20_FOLLOW_BY_PAGCUGC_V2,
			      B_PATH0_P20_FOLLOW_BY_PAGCUGC_EN_MSK},
	.p0_s20_pagcugc_en = {R_PATH0_S20_FOLLOW_BY_PAGCUGC_V2,
			      B_PATH0_S20_FOLLOW_BY_PAGCUGC_EN_MSK},
	.p1_p20_pagcugc_en = {R_PATH1_P20_FOLLOW_BY_PAGCUGC_V2,
			      B_PATH1_P20_FOLLOW_BY_PAGCUGC_EN_MSK},
	.p1_s20_pagcugc_en = {R_PATH1_S20_FOLLOW_BY_PAGCUGC_V2,
			      B_PATH1_S20_FOLLOW_BY_PAGCUGC_EN_MSK},
};

static const struct rtw89_btc_rf_trx_para rtw89_btc_8852b_rf_ul[] = {
	{255, 0, 0, 7}, /* 0 -> original */
	{255, 2, 0, 7}, /* 1 -> for BT-connected ACI issue && BTG co-rx */
	{255, 0, 0, 7}, /* 2 ->reserved for shared-antenna */
	{255, 0, 0, 7}, /* 3- >reserved for shared-antenna */
	{255, 0, 0, 7}, /* 4 ->reserved for shared-antenna */
	{255, 0, 0, 7}, /* the below id is for non-shared-antenna free-run */
	{6, 1, 0, 7},
	{13, 1, 0, 7},
	{13, 1, 0, 7}
};

static const struct rtw89_btc_rf_trx_para rtw89_btc_8852b_rf_dl[] = {
	{255, 0, 0, 7}, /* 0 -> original */
	{255, 2, 0, 7}, /* 1 -> reserved for shared-antenna */
	{255, 0, 0, 7}, /* 2 ->reserved for shared-antenna */
	{255, 0, 0, 7}, /* 3- >reserved for shared-antenna */
	{255, 0, 0, 7}, /* 4 ->reserved for shared-antenna */
	{255, 0, 0, 7}, /* the below id is for non-shared-antenna free-run */
	{255, 1, 0, 7},
	{255, 1, 0, 7},
	{255, 1, 0, 7}
};

static const struct rtw89_btc_fbtc_mreg rtw89_btc_8852b_mon_reg[] = {
	RTW89_DEF_FBTC_MREG(REG_MAC, 4, 0xda24),
	RTW89_DEF_FBTC_MREG(REG_MAC, 4, 0xda28),
	RTW89_DEF_FBTC_MREG(REG_MAC, 4, 0xda2c),
	RTW89_DEF_FBTC_MREG(REG_MAC, 4, 0xda30),
	RTW89_DEF_FBTC_MREG(REG_MAC, 4, 0xda4c),
	RTW89_DEF_FBTC_MREG(REG_MAC, 4, 0xda10),
	RTW89_DEF_FBTC_MREG(REG_MAC, 4, 0xda20),
	RTW89_DEF_FBTC_MREG(REG_MAC, 4, 0xda34),
	RTW89_DEF_FBTC_MREG(REG_MAC, 4, 0xcef4),
	RTW89_DEF_FBTC_MREG(REG_MAC, 4, 0x8424),
	RTW89_DEF_FBTC_MREG(REG_MAC, 4, 0xd200),
	RTW89_DEF_FBTC_MREG(REG_MAC, 4, 0xd220),
	RTW89_DEF_FBTC_MREG(REG_BB, 4, 0x980),
	RTW89_DEF_FBTC_MREG(REG_BT_MODEM, 4, 0x178),
};

static const u8 rtw89_btc_8852b_wl_rssi_thres[BTC_WL_RSSI_THMAX] = {70, 60, 50, 40};
static const u8 rtw89_btc_8852b_bt_rssi_thres[BTC_BT_RSSI_THMAX] = {50, 40, 30, 20};

static int rtw8852b_pwr_on_func(struct rtw89_dev *rtwdev)
{
	u32 val32;
	u32 ret;

	rtw89_write32_clr(rtwdev, R_AX_SYS_PW_CTRL, B_AX_AFSM_WLSUS_EN |
						    B_AX_AFSM_PCIE_SUS_EN);
	rtw89_write32_set(rtwdev, R_AX_SYS_PW_CTRL, B_AX_DIS_WLBT_PDNSUSEN_SOPC);
	rtw89_write32_set(rtwdev, R_AX_WLLPS_CTRL, B_AX_DIS_WLBT_LPSEN_LOPC);
	rtw89_write32_clr(rtwdev, R_AX_SYS_PW_CTRL, B_AX_APDM_HPDN);
	rtw89_write32_clr(rtwdev, R_AX_SYS_PW_CTRL, B_AX_APFM_SWLPS);

	ret = read_poll_timeout(rtw89_read32, val32, val32 & B_AX_RDY_SYSPWR,
				1000, 20000, false, rtwdev, R_AX_SYS_PW_CTRL);
	if (ret)
		return ret;

	rtw89_write32_set(rtwdev, R_AX_AFE_LDO_CTRL, B_AX_AON_OFF_PC_EN);
	ret = read_poll_timeout(rtw89_read32, val32, val32 & B_AX_AON_OFF_PC_EN,
				1000, 20000, false, rtwdev, R_AX_AFE_LDO_CTRL);
	if (ret)
		return ret;

	rtw89_write32_mask(rtwdev, R_AX_SPS_DIG_OFF_CTRL0, B_AX_C1_L1_MASK, 0x1);
	rtw89_write32_mask(rtwdev, R_AX_SPS_DIG_OFF_CTRL0, B_AX_C3_L1_MASK, 0x3);
	rtw89_write32_set(rtwdev, R_AX_SYS_PW_CTRL, B_AX_EN_WLON);
	rtw89_write32_set(rtwdev, R_AX_SYS_PW_CTRL, B_AX_APFN_ONMAC);

	ret = read_poll_timeout(rtw89_read32, val32, !(val32 & B_AX_APFN_ONMAC),
				1000, 20000, false, rtwdev, R_AX_SYS_PW_CTRL);
	if (ret)
		return ret;

	rtw89_write8_set(rtwdev, R_AX_PLATFORM_ENABLE, B_AX_PLATFORM_EN);
	rtw89_write8_clr(rtwdev, R_AX_PLATFORM_ENABLE, B_AX_PLATFORM_EN);
	rtw89_write8_set(rtwdev, R_AX_PLATFORM_ENABLE, B_AX_PLATFORM_EN);
	rtw89_write8_clr(rtwdev, R_AX_PLATFORM_ENABLE, B_AX_PLATFORM_EN);

	rtw89_write8_set(rtwdev, R_AX_PLATFORM_ENABLE, B_AX_PLATFORM_EN);
	rtw89_write32_clr(rtwdev, R_AX_SYS_SDIO_CTRL, B_AX_PCIE_CALIB_EN_V1);

	rtw89_write32_set(rtwdev, R_AX_SYS_ADIE_PAD_PWR_CTRL, B_AX_SYM_PADPDN_WL_PTA_1P3);

	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL,
				      XTAL_SI_GND_SHDN_WL, XTAL_SI_GND_SHDN_WL);
	if (ret)
		return ret;

	rtw89_write32_set(rtwdev, R_AX_SYS_ADIE_PAD_PWR_CTRL, B_AX_SYM_PADPDN_WL_RFC_1P3);

	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL,
				      XTAL_SI_SHDN_WL, XTAL_SI_SHDN_WL);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, XTAL_SI_OFF_WEI,
				      XTAL_SI_OFF_WEI);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, XTAL_SI_OFF_EI,
				      XTAL_SI_OFF_EI);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, 0, XTAL_SI_RFC2RF);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, XTAL_SI_PON_WEI,
				      XTAL_SI_PON_WEI);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, XTAL_SI_PON_EI,
				      XTAL_SI_PON_EI);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, 0, XTAL_SI_SRAM2RFC);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_SRAM_CTRL, 0, XTAL_SI_SRAM_DIS);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_XTAL_XMD_2, 0, XTAL_SI_LDO_LPS);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_XTAL_XMD_4, 0, XTAL_SI_LPS_CAP);
	if (ret)
		return ret;

	rtw89_write32_set(rtwdev, R_AX_PMC_DBG_CTRL2, B_AX_SYSON_DIS_PMCR_AX_WRMSK);
	rtw89_write32_set(rtwdev, R_AX_SYS_ISO_CTRL, B_AX_ISO_EB2CORE);
	rtw89_write32_clr(rtwdev, R_AX_SYS_ISO_CTRL, B_AX_PWC_EV2EF_B15);

	fsleep_alt(1000);

	rtw89_write32_clr(rtwdev, R_AX_SYS_ISO_CTRL, B_AX_PWC_EV2EF_B14);
	rtw89_write32_clr(rtwdev, R_AX_PMC_DBG_CTRL2, B_AX_SYSON_DIS_PMCR_AX_WRMSK);

	if (!rtwdev->efuse.valid || rtwdev->efuse.power_k_valid)
		goto func_en;

	rtw89_write32_mask(rtwdev, R_AX_SPS_DIG_ON_CTRL0, B_AX_VOL_L1_MASK, 0x9);
	rtw89_write32_mask(rtwdev, R_AX_SPS_DIG_ON_CTRL0, B_AX_VREFPFM_L_MASK, 0xA);

	if (rtwdev->hal.cv == CHIP_CBV) {
		rtw89_write32_set(rtwdev, R_AX_PMC_DBG_CTRL2, B_AX_SYSON_DIS_PMCR_AX_WRMSK);
		rtw89_write16_mask(rtwdev, R_AX_HCI_LDO_CTRL, B_AX_R_AX_VADJ_MASK, 0xA);
		rtw89_write32_clr(rtwdev, R_AX_PMC_DBG_CTRL2, B_AX_SYSON_DIS_PMCR_AX_WRMSK);
	}

func_en:
	rtw89_write32_set(rtwdev, R_AX_DMAC_FUNC_EN,
			  B_AX_MAC_FUNC_EN | B_AX_DMAC_FUNC_EN | B_AX_MPDU_PROC_EN |
			  B_AX_WD_RLS_EN | B_AX_DLE_WDE_EN | B_AX_TXPKT_CTRL_EN |
			  B_AX_STA_SCH_EN | B_AX_DLE_PLE_EN | B_AX_PKT_BUF_EN |
			  B_AX_DMAC_TBL_EN | B_AX_PKT_IN_EN | B_AX_DLE_CPUIO_EN |
			  B_AX_DISPATCHER_EN | B_AX_BBRPT_EN | B_AX_MAC_SEC_EN |
			  B_AX_DMACREG_GCKEN);
	rtw89_write32_set(rtwdev, R_AX_CMAC_FUNC_EN,
			  B_AX_CMAC_EN | B_AX_CMAC_TXEN | B_AX_CMAC_RXEN |
			  B_AX_FORCE_CMACREG_GCKEN | B_AX_PHYINTF_EN | B_AX_CMAC_DMA_EN |
			  B_AX_PTCLTOP_EN | B_AX_SCHEDULER_EN | B_AX_TMAC_EN |
			  B_AX_RMAC_EN);

	rtw89_write32_mask(rtwdev, R_AX_EECS_EESK_FUNC_SEL, B_AX_PINMUX_EESK_FUNC_SEL_MASK,
			   PINMUX_EESK_FUNC_SEL_BT_LOG);

	return 0;
}

static int rtw8852b_pwr_off_func(struct rtw89_dev *rtwdev)
{
	u32 val32;
	u32 ret;

	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, XTAL_SI_RFC2RF,
				      XTAL_SI_RFC2RF);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, 0, XTAL_SI_OFF_EI);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, 0, XTAL_SI_OFF_WEI);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_WL_RFC_S0, 0, XTAL_SI_RF00);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_WL_RFC_S1, 0, XTAL_SI_RF10);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, XTAL_SI_SRAM2RFC,
				      XTAL_SI_SRAM2RFC);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, 0, XTAL_SI_PON_EI);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, 0, XTAL_SI_PON_WEI);
	if (ret)
		return ret;

	rtw89_write32_set(rtwdev, R_AX_SYS_PW_CTRL, B_AX_EN_WLON);
	rtw89_write8_clr(rtwdev, R_AX_SYS_FUNC_EN, B_AX_FEN_BB_GLB_RSTN | B_AX_FEN_BBRSTB);
	rtw89_write32_clr(rtwdev, R_AX_SYS_ADIE_PAD_PWR_CTRL, B_AX_SYM_PADPDN_WL_RFC_1P3);

	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, 0, XTAL_SI_SHDN_WL);
	if (ret)
		return ret;

	rtw89_write32_clr(rtwdev, R_AX_SYS_ADIE_PAD_PWR_CTRL, B_AX_SYM_PADPDN_WL_PTA_1P3);

	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, 0, XTAL_SI_GND_SHDN_WL);
	if (ret)
		return ret;

	rtw89_write32_set(rtwdev, R_AX_SYS_PW_CTRL, B_AX_APFM_OFFMAC);

	ret = read_poll_timeout(rtw89_read32, val32, !(val32 & B_AX_APFM_OFFMAC),
				1000, 20000, false, rtwdev, R_AX_SYS_PW_CTRL);
	if (ret)
		return ret;

	rtw89_write32(rtwdev, R_AX_WLLPS_CTRL, SW_LPS_OPTION);
	rtw89_write32_set(rtwdev, R_AX_SYS_SWR_CTRL1, B_AX_SYM_CTRL_SPS_PWMFREQ);
	rtw89_write32_mask(rtwdev, R_AX_SPS_DIG_ON_CTRL0, B_AX_REG_ZCDC_H_MASK, 0x3);
	rtw89_write32_set(rtwdev, R_AX_SYS_PW_CTRL, B_AX_APFM_SWLPS);

	return 0;
}

static void rtw8852be_efuse_parsing(struct rtw89_efuse *efuse,
				    struct rtw8852b_efuse *map)
{
	ether_addr_copy(efuse->addr, map->e.mac_addr);
	efuse->rfe_type = map->rfe_type;
	efuse->xtal_cap = map->xtal_k;
}

static void rtw8852b_efuse_parsing_tssi(struct rtw89_dev *rtwdev,
					struct rtw8852b_efuse *map)
{
	struct rtw89_tssi_info *tssi = &rtwdev->tssi;
	struct rtw8852b_tssi_offset *ofst[] = {&map->path_a_tssi, &map->path_b_tssi};
	u8 i, j;

	tssi->thermal[RF_PATH_A] = map->path_a_therm;
	tssi->thermal[RF_PATH_B] = map->path_b_therm;

	for (i = 0; i < RF_PATH_NUM_8852B; i++) {
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

static bool _decode_efuse_gain(u8 data, s8 *high, s8 *low)
{
	if (high)
		*high = sign_extend32(FIELD_GET(GENMASK(7,  4), data), 3);
	if (low)
		*low = sign_extend32(FIELD_GET(GENMASK(3,  0), data), 3);

	return data != 0xff;
}

static void rtw8852b_efuse_parsing_gain_offset(struct rtw89_dev *rtwdev,
					       struct rtw8852b_efuse *map)
{
	struct rtw89_phy_efuse_gain *gain = &rtwdev->efuse_gain;
	bool valid = false;

	valid |= _decode_efuse_gain(map->rx_gain_2g_cck,
				    &gain->offset[RF_PATH_A][RTW89_GAIN_OFFSET_2G_CCK],
				    &gain->offset[RF_PATH_B][RTW89_GAIN_OFFSET_2G_CCK]);
	valid |= _decode_efuse_gain(map->rx_gain_2g_ofdm,
				    &gain->offset[RF_PATH_A][RTW89_GAIN_OFFSET_2G_OFDM],
				    &gain->offset[RF_PATH_B][RTW89_GAIN_OFFSET_2G_OFDM]);
	valid |= _decode_efuse_gain(map->rx_gain_5g_low,
				    &gain->offset[RF_PATH_A][RTW89_GAIN_OFFSET_5G_LOW],
				    &gain->offset[RF_PATH_B][RTW89_GAIN_OFFSET_5G_LOW]);
	valid |= _decode_efuse_gain(map->rx_gain_5g_mid,
				    &gain->offset[RF_PATH_A][RTW89_GAIN_OFFSET_5G_MID],
				    &gain->offset[RF_PATH_B][RTW89_GAIN_OFFSET_5G_MID]);
	valid |= _decode_efuse_gain(map->rx_gain_5g_high,
				    &gain->offset[RF_PATH_A][RTW89_GAIN_OFFSET_5G_HIGH],
				    &gain->offset[RF_PATH_B][RTW89_GAIN_OFFSET_5G_HIGH]);

	gain->offset_valid = valid;
}

static int rtw8852b_read_efuse(struct rtw89_dev *rtwdev, u8 *log_map)
{
	struct rtw89_efuse *efuse = &rtwdev->efuse;
	struct rtw8852b_efuse *map;

	map = (struct rtw8852b_efuse *)log_map;

	efuse->country_code[0] = map->country_code[0];
	efuse->country_code[1] = map->country_code[1];
	rtw8852b_efuse_parsing_tssi(rtwdev, map);
	rtw8852b_efuse_parsing_gain_offset(rtwdev, map);

	switch (rtwdev->hci.type) {
	case RTW89_HCI_TYPE_PCIE:
		rtw8852be_efuse_parsing(efuse, map);
		break;
	default:
		return -EOPNOTSUPP;
	}

	rtw89_info(rtwdev, "chip rfe_type is %d\n", efuse->rfe_type);

	return 0;
}

static void rtw8852b_phycap_parsing_power_cal(struct rtw89_dev *rtwdev, u8 *phycap_map)
{
#define PWR_K_CHK_OFFSET 0x5E9
#define PWR_K_CHK_VALUE 0xAA
	u32 offset = PWR_K_CHK_OFFSET - rtwdev->chip->phycap_addr;

	if (phycap_map[offset] == PWR_K_CHK_VALUE)
		rtwdev->efuse.power_k_valid = true;
}

static void rtw8852b_phycap_parsing_tssi(struct rtw89_dev *rtwdev, u8 *phycap_map)
{
	struct rtw89_tssi_info *tssi = &rtwdev->tssi;
	static const u32 tssi_trim_addr[RF_PATH_NUM_8852B] = {0x5D6, 0x5AB};
	u32 addr = rtwdev->chip->phycap_addr;
	bool pg = false;
	u32 ofst;
	u8 i, j;

	for (i = 0; i < RF_PATH_NUM_8852B; i++) {
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

	for (i = 0; i < RF_PATH_NUM_8852B; i++)
		for (j = 0; j < TSSI_TRIM_CH_GROUP_NUM; j++)
			rtw89_debug(rtwdev, RTW89_DBG_TSSI,
				    "[TSSI] path=%d idx=%d trim=0x%x addr=0x%x\n",
				    i, j, tssi->tssi_trim[i][j],
				    tssi_trim_addr[i] - j);
}

static void rtw8852b_phycap_parsing_thermal_trim(struct rtw89_dev *rtwdev,
						 u8 *phycap_map)
{
	struct rtw89_power_trim_info *info = &rtwdev->pwr_trim;
	static const u32 thm_trim_addr[RF_PATH_NUM_8852B] = {0x5DF, 0x5DC};
	u32 addr = rtwdev->chip->phycap_addr;
	u8 i;

	for (i = 0; i < RF_PATH_NUM_8852B; i++) {
		info->thermal_trim[i] = phycap_map[thm_trim_addr[i] - addr];

		rtw89_debug(rtwdev, RTW89_DBG_RFK,
			    "[THERMAL][TRIM] path=%d thermal_trim=0x%x\n",
			    i, info->thermal_trim[i]);

		if (info->thermal_trim[i] != 0xff)
			info->pg_thermal_trim = true;
	}
}

static void rtw8852b_thermal_trim(struct rtw89_dev *rtwdev)
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

	for (i = 0; i < RF_PATH_NUM_8852B; i++) {
		val = __thm_setting(info->thermal_trim[i]);
		rtw89_write_rf(rtwdev, i, RR_TM2, RR_TM2_OFF, val);

		rtw89_debug(rtwdev, RTW89_DBG_RFK,
			    "[THERMAL][TRIM] path=%d thermal_setting=0x%x\n",
			    i, val);
	}
#undef __thm_setting
}

static void rtw8852b_phycap_parsing_pa_bias_trim(struct rtw89_dev *rtwdev,
						 u8 *phycap_map)
{
	struct rtw89_power_trim_info *info = &rtwdev->pwr_trim;
	static const u32 pabias_trim_addr[RF_PATH_NUM_8852B] = {0x5DE, 0x5DB};
	u32 addr = rtwdev->chip->phycap_addr;
	u8 i;

	for (i = 0; i < RF_PATH_NUM_8852B; i++) {
		info->pa_bias_trim[i] = phycap_map[pabias_trim_addr[i] - addr];

		rtw89_debug(rtwdev, RTW89_DBG_RFK,
			    "[PA_BIAS][TRIM] path=%d pa_bias_trim=0x%x\n",
			    i, info->pa_bias_trim[i]);

		if (info->pa_bias_trim[i] != 0xff)
			info->pg_pa_bias_trim = true;
	}
}

static void rtw8852b_pa_bias_trim(struct rtw89_dev *rtwdev)
{
	struct rtw89_power_trim_info *info = &rtwdev->pwr_trim;
	u8 pabias_2g, pabias_5g;
	u8 i;

	if (!info->pg_pa_bias_trim) {
		rtw89_debug(rtwdev, RTW89_DBG_RFK,
			    "[PA_BIAS][TRIM] no PG, do nothing\n");

		return;
	}

	for (i = 0; i < RF_PATH_NUM_8852B; i++) {
		pabias_2g = FIELD_GET(GENMASK(3, 0), info->pa_bias_trim[i]);
		pabias_5g = FIELD_GET(GENMASK(7, 4), info->pa_bias_trim[i]);

		rtw89_debug(rtwdev, RTW89_DBG_RFK,
			    "[PA_BIAS][TRIM] path=%d 2G=0x%x 5G=0x%x\n",
			    i, pabias_2g, pabias_5g);

		rtw89_write_rf(rtwdev, i, RR_BIASA, RR_BIASA_TXG, pabias_2g);
		rtw89_write_rf(rtwdev, i, RR_BIASA, RR_BIASA_TXA, pabias_5g);
	}
}

static void rtw8852b_phycap_parsing_gain_comp(struct rtw89_dev *rtwdev, u8 *phycap_map)
{
	static const u32 comp_addrs[][RTW89_SUBBAND_2GHZ_5GHZ_NR] = {
		{0x5BB, 0x5BA, 0, 0x5B9, 0x5B8},
		{0x590, 0x58F, 0, 0x58E, 0x58D},
	};
	struct rtw89_phy_efuse_gain *gain = &rtwdev->efuse_gain;
	u32 phycap_addr = rtwdev->chip->phycap_addr;
	bool valid = false;
	int path, i;
	u8 data;

	for (path = 0; path < 2; path++)
		for (i = 0; i < RTW89_SUBBAND_2GHZ_5GHZ_NR; i++) {
			if (comp_addrs[path][i] == 0)
				continue;

			data = phycap_map[comp_addrs[path][i] - phycap_addr];
			valid |= _decode_efuse_gain(data, NULL,
						    &gain->comp[path][i]);
		}

	gain->comp_valid = valid;
}

static int rtw8852b_read_phycap(struct rtw89_dev *rtwdev, u8 *phycap_map)
{
	rtw8852b_phycap_parsing_power_cal(rtwdev, phycap_map);
	rtw8852b_phycap_parsing_tssi(rtwdev, phycap_map);
	rtw8852b_phycap_parsing_thermal_trim(rtwdev, phycap_map);
	rtw8852b_phycap_parsing_pa_bias_trim(rtwdev, phycap_map);
	rtw8852b_phycap_parsing_gain_comp(rtwdev, phycap_map);

	return 0;
}

static void rtw8852b_power_trim(struct rtw89_dev *rtwdev)
{
	rtw8852b_thermal_trim(rtwdev);
	rtw8852b_pa_bias_trim(rtwdev);
}

static void rtw8852b_set_channel_mac(struct rtw89_dev *rtwdev,
				     const struct rtw89_chan *chan,
				     u8 mac_idx)
{
	u32 rf_mod = rtw89_mac_reg_by_idx(R_AX_WMAC_RFMOD, mac_idx);
	u32 sub_carr = rtw89_mac_reg_by_idx(R_AX_TX_SUB_CARRIER_VALUE, mac_idx);
	u32 chk_rate = rtw89_mac_reg_by_idx(R_AX_TXRATE_CHK, mac_idx);
	u8 txsc20 = 0, txsc40 = 0;

	switch (chan->band_width) {
	case RTW89_CHANNEL_WIDTH_80:
		txsc40 = rtw89_phy_get_txsc(rtwdev, chan, RTW89_CHANNEL_WIDTH_40);
		fallthrough;
	case RTW89_CHANNEL_WIDTH_40:
		txsc20 = rtw89_phy_get_txsc(rtwdev, chan, RTW89_CHANNEL_WIDTH_20);
		break;
	default:
		break;
	}

	switch (chan->band_width) {
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

	if (chan->channel > 14) {
		rtw89_write8_clr(rtwdev, chk_rate, B_AX_BAND_MODE);
		rtw89_write8_set(rtwdev, chk_rate,
				 B_AX_CHECK_CCK_EN | B_AX_RTS_LIMIT_IN_OFDM6);
	} else {
		rtw89_write8_set(rtwdev, chk_rate, B_AX_BAND_MODE);
		rtw89_write8_clr(rtwdev, chk_rate,
				 B_AX_CHECK_CCK_EN | B_AX_RTS_LIMIT_IN_OFDM6);
	}
}

static const u32 rtw8852b_sco_barker_threshold[14] = {
	0x1cfea, 0x1d0e1, 0x1d1d7, 0x1d2cd, 0x1d3c3, 0x1d4b9, 0x1d5b0, 0x1d6a6,
	0x1d79c, 0x1d892, 0x1d988, 0x1da7f, 0x1db75, 0x1ddc4
};

static const u32 rtw8852b_sco_cck_threshold[14] = {
	0x27de3, 0x27f35, 0x28088, 0x281da, 0x2832d, 0x2847f, 0x285d2, 0x28724,
	0x28877, 0x289c9, 0x28b1c, 0x28c6e, 0x28dc1, 0x290ed
};

static void rtw8852b_ctrl_sco_cck(struct rtw89_dev *rtwdev, u8 primary_ch)
{
	u8 ch_element = primary_ch - 1;

	rtw89_phy_write32_mask(rtwdev, R_RXSCOBC, B_RXSCOBC_TH,
			       rtw8852b_sco_barker_threshold[ch_element]);
	rtw89_phy_write32_mask(rtwdev, R_RXSCOCCK, B_RXSCOCCK_TH,
			       rtw8852b_sco_cck_threshold[ch_element]);
}

static u8 rtw8852b_sco_mapping(u8 central_ch)
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

struct rtw8852b_bb_gain {
	u32 gain_g[BB_PATH_NUM_8852B];
	u32 gain_a[BB_PATH_NUM_8852B];
	u32 gain_mask;
};

static const struct rtw8852b_bb_gain bb_gain_lna[LNA_GAIN_NUM] = {
	{ .gain_g = {0x4678, 0x475C}, .gain_a = {0x45DC, 0x4740},
	  .gain_mask = 0x00ff0000 },
	{ .gain_g = {0x4678, 0x475C}, .gain_a = {0x45DC, 0x4740},
	  .gain_mask = 0xff000000 },
	{ .gain_g = {0x467C, 0x4760}, .gain_a = {0x4660, 0x4744},
	  .gain_mask = 0x000000ff },
	{ .gain_g = {0x467C, 0x4760}, .gain_a = {0x4660, 0x4744},
	  .gain_mask = 0x0000ff00 },
	{ .gain_g = {0x467C, 0x4760}, .gain_a = {0x4660, 0x4744},
	  .gain_mask = 0x00ff0000 },
	{ .gain_g = {0x467C, 0x4760}, .gain_a = {0x4660, 0x4744},
	  .gain_mask = 0xff000000 },
	{ .gain_g = {0x4680, 0x4764}, .gain_a = {0x4664, 0x4748},
	  .gain_mask = 0x000000ff },
};

static const struct rtw8852b_bb_gain bb_gain_tia[TIA_GAIN_NUM] = {
	{ .gain_g = {0x4680, 0x4764}, .gain_a = {0x4664, 0x4748},
	  .gain_mask = 0x00ff0000 },
	{ .gain_g = {0x4680, 0x4764}, .gain_a = {0x4664, 0x4748},
	  .gain_mask = 0xff000000 },
};

static void rtw8852b_set_gain_error(struct rtw89_dev *rtwdev,
				    enum rtw89_subband subband,
				    enum rtw89_rf_path path)
{
	const struct rtw89_phy_bb_gain_info *gain = &rtwdev->bb_gain;
	u8 gain_band = rtw89_subband_to_bb_gain_band(subband);
	s32 val;
	u32 reg;
	u32 mask;
	int i;

	for (i = 0; i < LNA_GAIN_NUM; i++) {
		if (subband == RTW89_CH_2G)
			reg = bb_gain_lna[i].gain_g[path];
		else
			reg = bb_gain_lna[i].gain_a[path];

		mask = bb_gain_lna[i].gain_mask;
		val = gain->lna_gain[gain_band][path][i];
		rtw89_phy_write32_mask(rtwdev, reg, mask, val);
	}

	for (i = 0; i < TIA_GAIN_NUM; i++) {
		if (subband == RTW89_CH_2G)
			reg = bb_gain_tia[i].gain_g[path];
		else
			reg = bb_gain_tia[i].gain_a[path];

		mask = bb_gain_tia[i].gain_mask;
		val = gain->tia_gain[gain_band][path][i];
		rtw89_phy_write32_mask(rtwdev, reg, mask, val);
	}
}

static void rtw8852b_set_gain_offset(struct rtw89_dev *rtwdev,
				     enum rtw89_subband subband,
				     enum rtw89_phy_idx phy_idx)
{
	static const u32 gain_err_addr[2] = {R_P0_AGC_RSVD, R_P1_AGC_RSVD};
	static const u32 rssi_ofst_addr[2] = {R_PATH0_G_TIA1_LNA6_OP1DB_V1,
					      R_PATH1_G_TIA1_LNA6_OP1DB_V1};
	struct rtw89_hal *hal = &rtwdev->hal;
	struct rtw89_phy_efuse_gain *efuse_gain = &rtwdev->efuse_gain;
	enum rtw89_gain_offset gain_ofdm_band;
	s32 offset_a, offset_b;
	s32 offset_ofdm, offset_cck;
	s32 tmp;
	u8 path;

	if (!efuse_gain->comp_valid)
		goto next;

	for (path = RF_PATH_A; path < BB_PATH_NUM_8852B; path++) {
		tmp = efuse_gain->comp[path][subband];
		tmp = clamp_t(s32, tmp << 2, S8_MIN, S8_MAX);
		rtw89_phy_write32_mask(rtwdev, gain_err_addr[path], MASKBYTE0, tmp);
	}

next:
	if (!efuse_gain->offset_valid)
		return;

	gain_ofdm_band = rtw89_subband_to_gain_offset_band_of_ofdm(subband);

	offset_a = -efuse_gain->offset[RF_PATH_A][gain_ofdm_band];
	offset_b = -efuse_gain->offset[RF_PATH_B][gain_ofdm_band];

	tmp = -((offset_a << 2) + (efuse_gain->offset_base[RTW89_PHY_0] >> 2));
	tmp = clamp_t(s32, tmp, S8_MIN, S8_MAX);
	rtw89_phy_write32_mask(rtwdev, rssi_ofst_addr[RF_PATH_A], B_PATH0_R_G_OFST_MASK, tmp);

	tmp = -((offset_b << 2) + (efuse_gain->offset_base[RTW89_PHY_0] >> 2));
	tmp = clamp_t(s32, tmp, S8_MIN, S8_MAX);
	rtw89_phy_write32_mask(rtwdev, rssi_ofst_addr[RF_PATH_B], B_PATH0_R_G_OFST_MASK, tmp);

	if (hal->antenna_rx == RF_B) {
		offset_ofdm = -efuse_gain->offset[RF_PATH_B][gain_ofdm_band];
		offset_cck = -efuse_gain->offset[RF_PATH_B][0];
	} else {
		offset_ofdm = -efuse_gain->offset[RF_PATH_A][gain_ofdm_band];
		offset_cck = -efuse_gain->offset[RF_PATH_A][0];
	}

	tmp = (offset_ofdm << 4) + efuse_gain->offset_base[RTW89_PHY_0];
	tmp = clamp_t(s32, tmp, S8_MIN, S8_MAX);
	rtw89_phy_write32_idx(rtwdev, R_P0_RPL1, B_P0_RPL1_BIAS_MASK, tmp, phy_idx);

	tmp = (offset_ofdm << 4) + efuse_gain->rssi_base[RTW89_PHY_0];
	tmp = clamp_t(s32, tmp, S8_MIN, S8_MAX);
	rtw89_phy_write32_idx(rtwdev, R_P1_RPL1, B_P0_RPL1_BIAS_MASK, tmp, phy_idx);

	if (subband == RTW89_CH_2G) {
		tmp = (offset_cck << 3) + (efuse_gain->offset_base[RTW89_PHY_0] >> 1);
		tmp = clamp_t(s32, tmp, S8_MIN >> 1, S8_MAX >> 1);
		rtw89_phy_write32_mask(rtwdev, R_RX_RPL_OFST,
				       B_RX_RPL_OFST_CCK_MASK, tmp);
	}
}

static
void rtw8852b_set_rxsc_rpl_comp(struct rtw89_dev *rtwdev, enum rtw89_subband subband)
{
	const struct rtw89_phy_bb_gain_info *gain = &rtwdev->bb_gain;
	u8 band = rtw89_subband_to_bb_gain_band(subband);
	u32 val;

	val = FIELD_PREP(B_P0_RPL1_20_MASK, (gain->rpl_ofst_20[band][RF_PATH_A] +
					     gain->rpl_ofst_20[band][RF_PATH_B]) / 2) |
	      FIELD_PREP(B_P0_RPL1_40_MASK, (gain->rpl_ofst_40[band][RF_PATH_A][0] +
					     gain->rpl_ofst_40[band][RF_PATH_B][0]) / 2) |
	      FIELD_PREP(B_P0_RPL1_41_MASK, (gain->rpl_ofst_40[band][RF_PATH_A][1] +
					     gain->rpl_ofst_40[band][RF_PATH_B][1]) / 2);
	val >>= B_P0_RPL1_SHIFT;
	rtw89_phy_write32_mask(rtwdev, R_P0_RPL1, B_P0_RPL1_MASK, val);
	rtw89_phy_write32_mask(rtwdev, R_P1_RPL1, B_P0_RPL1_MASK, val);

	val = FIELD_PREP(B_P0_RTL2_42_MASK, (gain->rpl_ofst_40[band][RF_PATH_A][2] +
					     gain->rpl_ofst_40[band][RF_PATH_B][2]) / 2) |
	      FIELD_PREP(B_P0_RTL2_80_MASK, (gain->rpl_ofst_80[band][RF_PATH_A][0] +
					     gain->rpl_ofst_80[band][RF_PATH_B][0]) / 2) |
	      FIELD_PREP(B_P0_RTL2_81_MASK, (gain->rpl_ofst_80[band][RF_PATH_A][1] +
					     gain->rpl_ofst_80[band][RF_PATH_B][1]) / 2) |
	      FIELD_PREP(B_P0_RTL2_8A_MASK, (gain->rpl_ofst_80[band][RF_PATH_A][10] +
					     gain->rpl_ofst_80[band][RF_PATH_B][10]) / 2);
	rtw89_phy_write32(rtwdev, R_P0_RPL2, val);
	rtw89_phy_write32(rtwdev, R_P1_RPL2, val);

	val = FIELD_PREP(B_P0_RTL3_82_MASK, (gain->rpl_ofst_80[band][RF_PATH_A][2] +
					     gain->rpl_ofst_80[band][RF_PATH_B][2]) / 2) |
	      FIELD_PREP(B_P0_RTL3_83_MASK, (gain->rpl_ofst_80[band][RF_PATH_A][3] +
					     gain->rpl_ofst_80[band][RF_PATH_B][3]) / 2) |
	      FIELD_PREP(B_P0_RTL3_84_MASK, (gain->rpl_ofst_80[band][RF_PATH_A][4] +
					     gain->rpl_ofst_80[band][RF_PATH_B][4]) / 2) |
	      FIELD_PREP(B_P0_RTL3_89_MASK, (gain->rpl_ofst_80[band][RF_PATH_A][9] +
					     gain->rpl_ofst_80[band][RF_PATH_B][9]) / 2);
	rtw89_phy_write32(rtwdev, R_P0_RPL3, val);
	rtw89_phy_write32(rtwdev, R_P1_RPL3, val);
}

static void rtw8852b_ctrl_ch(struct rtw89_dev *rtwdev,
			     const struct rtw89_chan *chan,
			     enum rtw89_phy_idx phy_idx)
{
	u8 central_ch = chan->channel;
	u8 subband = chan->subband_type;
	u8 sco_comp;
	bool is_2g = central_ch <= 14;

	/* Path A */
	if (is_2g)
		rtw89_phy_write32_idx(rtwdev, R_PATH0_BAND_SEL_V1,
				      B_PATH0_BAND_SEL_MSK_V1, 1, phy_idx);
	else
		rtw89_phy_write32_idx(rtwdev, R_PATH0_BAND_SEL_V1,
				      B_PATH0_BAND_SEL_MSK_V1, 0, phy_idx);

	/* Path B */
	if (is_2g)
		rtw89_phy_write32_idx(rtwdev, R_PATH1_BAND_SEL_V1,
				      B_PATH1_BAND_SEL_MSK_V1, 1, phy_idx);
	else
		rtw89_phy_write32_idx(rtwdev, R_PATH1_BAND_SEL_V1,
				      B_PATH1_BAND_SEL_MSK_V1, 0, phy_idx);

	/* SCO compensate FC setting */
	sco_comp = rtw8852b_sco_mapping(central_ch);
	rtw89_phy_write32_idx(rtwdev, R_FC0_BW_V1, B_FC0_BW_INV, sco_comp, phy_idx);

	if (chan->band_type == RTW89_BAND_6G)
		return;

	/* CCK parameters */
	if (central_ch == 14) {
		rtw89_phy_write32_mask(rtwdev, R_TXFIR0, B_TXFIR_C01, 0x3b13ff);
		rtw89_phy_write32_mask(rtwdev, R_TXFIR2, B_TXFIR_C23, 0x1c42de);
		rtw89_phy_write32_mask(rtwdev, R_TXFIR4, B_TXFIR_C45, 0xfdb0ad);
		rtw89_phy_write32_mask(rtwdev, R_TXFIR6, B_TXFIR_C67, 0xf60f6e);
		rtw89_phy_write32_mask(rtwdev, R_TXFIR8, B_TXFIR_C89, 0xfd8f92);
		rtw89_phy_write32_mask(rtwdev, R_TXFIRA, B_TXFIR_CAB, 0x2d011);
		rtw89_phy_write32_mask(rtwdev, R_TXFIRC, B_TXFIR_CCD, 0x1c02c);
		rtw89_phy_write32_mask(rtwdev, R_TXFIRE, B_TXFIR_CEF, 0xfff00a);
	} else {
		rtw89_phy_write32_mask(rtwdev, R_TXFIR0, B_TXFIR_C01, 0x3d23ff);
		rtw89_phy_write32_mask(rtwdev, R_TXFIR2, B_TXFIR_C23, 0x29b354);
		rtw89_phy_write32_mask(rtwdev, R_TXFIR4, B_TXFIR_C45, 0xfc1c8);
		rtw89_phy_write32_mask(rtwdev, R_TXFIR6, B_TXFIR_C67, 0xfdb053);
		rtw89_phy_write32_mask(rtwdev, R_TXFIR8, B_TXFIR_C89, 0xf86f9a);
		rtw89_phy_write32_mask(rtwdev, R_TXFIRA, B_TXFIR_CAB, 0xfaef92);
		rtw89_phy_write32_mask(rtwdev, R_TXFIRC, B_TXFIR_CCD, 0xfe5fcc);
		rtw89_phy_write32_mask(rtwdev, R_TXFIRE, B_TXFIR_CEF, 0xffdff5);
	}

	rtw8852b_set_gain_error(rtwdev, subband, RF_PATH_A);
	rtw8852b_set_gain_error(rtwdev, subband, RF_PATH_B);
	rtw8852b_set_gain_offset(rtwdev, subband, phy_idx);
	rtw8852b_set_rxsc_rpl_comp(rtwdev, subband);
}

static void rtw8852b_bw_setting(struct rtw89_dev *rtwdev, u8 bw, u8 path)
{
	static const u32 adc_sel[2] = {0xC0EC, 0xC1EC};
	static const u32 wbadc_sel[2] = {0xC0E4, 0xC1E4};

	switch (bw) {
	case RTW89_CHANNEL_WIDTH_5:
		rtw89_phy_write32_mask(rtwdev, adc_sel[path], 0x6000, 0x1);
		rtw89_phy_write32_mask(rtwdev, wbadc_sel[path], 0x30, 0x0);
		break;
	case RTW89_CHANNEL_WIDTH_10:
		rtw89_phy_write32_mask(rtwdev, adc_sel[path], 0x6000, 0x2);
		rtw89_phy_write32_mask(rtwdev, wbadc_sel[path], 0x30, 0x1);
		break;
	case RTW89_CHANNEL_WIDTH_20:
		rtw89_phy_write32_mask(rtwdev, adc_sel[path], 0x6000, 0x0);
		rtw89_phy_write32_mask(rtwdev, wbadc_sel[path], 0x30, 0x2);
		break;
	case RTW89_CHANNEL_WIDTH_40:
		rtw89_phy_write32_mask(rtwdev, adc_sel[path], 0x6000, 0x0);
		rtw89_phy_write32_mask(rtwdev, wbadc_sel[path], 0x30, 0x2);
		break;
	case RTW89_CHANNEL_WIDTH_80:
		rtw89_phy_write32_mask(rtwdev, adc_sel[path], 0x6000, 0x0);
		rtw89_phy_write32_mask(rtwdev, wbadc_sel[path], 0x30, 0x2);
		break;
	default:
		rtw89_warn(rtwdev, "Fail to set ADC\n");
	}
}

static void rtw8852b_ctrl_bw(struct rtw89_dev *rtwdev, u8 pri_ch, u8 bw,
			     enum rtw89_phy_idx phy_idx)
{
	u32 rx_path_0;

	switch (bw) {
	case RTW89_CHANNEL_WIDTH_5:
		rtw89_phy_write32_idx(rtwdev, R_FC0_BW_V1, B_FC0_BW_SET, 0x0, phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_CHBW_MOD_V1, B_CHBW_MOD_SBW, 0x1, phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_CHBW_MOD_V1, B_CHBW_MOD_PRICH, 0x0, phy_idx);

		/*Set RF mode at 3 */
		rtw89_phy_write32_idx(rtwdev, R_P0_RFMODE_ORI_RX,
				      B_P0_RFMODE_ORI_RX_ALL, 0x333, phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_P1_RFMODE_ORI_RX,
				      B_P1_RFMODE_ORI_RX_ALL, 0x333, phy_idx);
		break;
	case RTW89_CHANNEL_WIDTH_10:
		rtw89_phy_write32_idx(rtwdev, R_FC0_BW_V1, B_FC0_BW_SET, 0x0, phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_CHBW_MOD_V1, B_CHBW_MOD_SBW, 0x2, phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_CHBW_MOD_V1, B_CHBW_MOD_PRICH, 0x0, phy_idx);

		/*Set RF mode at 3 */
		rtw89_phy_write32_idx(rtwdev, R_P0_RFMODE_ORI_RX,
				      B_P0_RFMODE_ORI_RX_ALL, 0x333, phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_P1_RFMODE_ORI_RX,
				      B_P1_RFMODE_ORI_RX_ALL, 0x333, phy_idx);
		break;
	case RTW89_CHANNEL_WIDTH_20:
		rtw89_phy_write32_idx(rtwdev, R_FC0_BW_V1, B_FC0_BW_SET, 0x0, phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_CHBW_MOD_V1, B_CHBW_MOD_SBW, 0x0, phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_CHBW_MOD_V1, B_CHBW_MOD_PRICH, 0x0, phy_idx);

		/*Set RF mode at 3 */
		rtw89_phy_write32_idx(rtwdev, R_P0_RFMODE_ORI_RX,
				      B_P0_RFMODE_ORI_RX_ALL, 0x333, phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_P1_RFMODE_ORI_RX,
				      B_P1_RFMODE_ORI_RX_ALL, 0x333, phy_idx);
		break;
	case RTW89_CHANNEL_WIDTH_40:
		rtw89_phy_write32_idx(rtwdev, R_FC0_BW_V1, B_FC0_BW_SET, 0x1, phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_CHBW_MOD_V1, B_CHBW_MOD_SBW, 0x0, phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_CHBW_MOD_V1, B_CHBW_MOD_PRICH,
				      pri_ch, phy_idx);

		/*Set RF mode at 3 */
		rtw89_phy_write32_idx(rtwdev, R_P0_RFMODE_ORI_RX,
				      B_P0_RFMODE_ORI_RX_ALL, 0x333, phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_P1_RFMODE_ORI_RX,
				      B_P1_RFMODE_ORI_RX_ALL, 0x333, phy_idx);
		/*CCK primary channel */
		if (pri_ch == RTW89_SC_20_UPPER)
			rtw89_phy_write32_mask(rtwdev, R_RXSC, B_RXSC_EN, 1);
		else
			rtw89_phy_write32_mask(rtwdev, R_RXSC, B_RXSC_EN, 0);

		break;
	case RTW89_CHANNEL_WIDTH_80:
		rtw89_phy_write32_idx(rtwdev, R_FC0_BW_V1, B_FC0_BW_SET, 0x2, phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_CHBW_MOD_V1, B_CHBW_MOD_SBW, 0x0, phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_CHBW_MOD_V1, B_CHBW_MOD_PRICH,
				      pri_ch, phy_idx);

		/*Set RF mode at A */
		rtw89_phy_write32_idx(rtwdev, R_P0_RFMODE_ORI_RX,
				      B_P0_RFMODE_ORI_RX_ALL, 0xaaa, phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_P1_RFMODE_ORI_RX,
				      B_P1_RFMODE_ORI_RX_ALL, 0xaaa, phy_idx);
		break;
	default:
		rtw89_warn(rtwdev, "Fail to switch bw (bw:%d, pri ch:%d)\n", bw,
			   pri_ch);
	}

	rtw8852b_bw_setting(rtwdev, bw, RF_PATH_A);
	rtw8852b_bw_setting(rtwdev, bw, RF_PATH_B);

	rx_path_0 = rtw89_phy_read32_idx(rtwdev, R_CHBW_MOD_V1, B_ANT_RX_SEG0,
					 phy_idx);
	if (rx_path_0 == 0x1)
		rtw89_phy_write32_idx(rtwdev, R_P1_RFMODE_ORI_RX,
				      B_P1_RFMODE_ORI_RX_ALL, 0x111, phy_idx);
	else if (rx_path_0 == 0x2)
		rtw89_phy_write32_idx(rtwdev, R_P0_RFMODE_ORI_RX,
				      B_P0_RFMODE_ORI_RX_ALL, 0x111, phy_idx);
}

static void rtw8852b_ctrl_cck_en(struct rtw89_dev *rtwdev, bool cck_en)
{
	if (cck_en) {
		rtw89_phy_write32_mask(rtwdev, R_UPD_CLK_ADC, B_ENABLE_CCK, 1);
		rtw89_phy_write32_mask(rtwdev, R_RXCCA, B_RXCCA_DIS, 0);
	} else {
		rtw89_phy_write32_mask(rtwdev, R_UPD_CLK_ADC, B_ENABLE_CCK, 0);
		rtw89_phy_write32_mask(rtwdev, R_RXCCA, B_RXCCA_DIS, 1);
	}
}

static void rtw8852b_5m_mask(struct rtw89_dev *rtwdev, const struct rtw89_chan *chan,
			     enum rtw89_phy_idx phy_idx)
{
	u8 pri_ch = chan->primary_channel;
	bool mask_5m_low;
	bool mask_5m_en;

	switch (chan->band_width) {
	case RTW89_CHANNEL_WIDTH_40:
		/* Prich=1: Mask 5M High, Prich=2: Mask 5M Low */
		mask_5m_en = true;
		mask_5m_low = pri_ch == 2;
		break;
	case RTW89_CHANNEL_WIDTH_80:
		/* Prich=3: Mask 5M High, Prich=4: Mask 5M Low, Else: Disable */
		mask_5m_en = pri_ch == 3 || pri_ch == 4;
		mask_5m_low = pri_ch == 4;
		break;
	default:
		mask_5m_en = false;
		break;
	}

	if (!mask_5m_en) {
		rtw89_phy_write32_mask(rtwdev, R_PATH0_5MDET_V1, B_PATH0_5MDET_EN, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_PATH1_5MDET_V1, B_PATH1_5MDET_EN, 0x0);
		rtw89_phy_write32_idx(rtwdev, R_ASSIGN_SBD_OPT_V1,
				      B_ASSIGN_SBD_OPT_EN_V1, 0x0, phy_idx);
		return;
	}

	if (mask_5m_low) {
		rtw89_phy_write32_mask(rtwdev, R_PATH0_5MDET_V1, B_PATH0_5MDET_TH, 0x4);
		rtw89_phy_write32_mask(rtwdev, R_PATH0_5MDET_V1, B_PATH0_5MDET_EN, 0x1);
		rtw89_phy_write32_mask(rtwdev, R_PATH0_5MDET_V1, B_PATH0_5MDET_SB2, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_PATH0_5MDET_V1, B_PATH0_5MDET_SB0, 0x1);
		rtw89_phy_write32_mask(rtwdev, R_PATH1_5MDET_V1, B_PATH1_5MDET_TH, 0x4);
		rtw89_phy_write32_mask(rtwdev, R_PATH1_5MDET_V1, B_PATH1_5MDET_EN, 0x1);
		rtw89_phy_write32_mask(rtwdev, R_PATH1_5MDET_V1, B_PATH1_5MDET_SB2, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_PATH1_5MDET_V1, B_PATH1_5MDET_SB0, 0x1);
	} else {
		rtw89_phy_write32_mask(rtwdev, R_PATH0_5MDET_V1, B_PATH0_5MDET_TH, 0x4);
		rtw89_phy_write32_mask(rtwdev, R_PATH0_5MDET_V1, B_PATH0_5MDET_EN, 0x1);
		rtw89_phy_write32_mask(rtwdev, R_PATH0_5MDET_V1, B_PATH0_5MDET_SB2, 0x1);
		rtw89_phy_write32_mask(rtwdev, R_PATH0_5MDET_V1, B_PATH0_5MDET_SB0, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_PATH1_5MDET_V1, B_PATH1_5MDET_TH, 0x4);
		rtw89_phy_write32_mask(rtwdev, R_PATH1_5MDET_V1, B_PATH1_5MDET_EN, 0x1);
		rtw89_phy_write32_mask(rtwdev, R_PATH1_5MDET_V1, B_PATH1_5MDET_SB2, 0x1);
		rtw89_phy_write32_mask(rtwdev, R_PATH1_5MDET_V1, B_PATH1_5MDET_SB0, 0x0);
	}
	rtw89_phy_write32_idx(rtwdev, R_ASSIGN_SBD_OPT_V1,
			      B_ASSIGN_SBD_OPT_EN_V1, 0x1, phy_idx);
}

static void rtw8852b_bb_reset_all(struct rtw89_dev *rtwdev, enum rtw89_phy_idx phy_idx)
{
	rtw89_phy_write32_idx(rtwdev, R_S0_HW_SI_DIS, B_S0_HW_SI_DIS_W_R_TRIG, 0x7, phy_idx);
	rtw89_phy_write32_idx(rtwdev, R_S1_HW_SI_DIS, B_S1_HW_SI_DIS_W_R_TRIG, 0x7, phy_idx);
	fsleep_alt(1);
	rtw89_phy_write32_idx(rtwdev, R_RSTB_ASYNC, B_RSTB_ASYNC_ALL, 1, phy_idx);
	rtw89_phy_write32_idx(rtwdev, R_RSTB_ASYNC, B_RSTB_ASYNC_ALL, 0, phy_idx);
	rtw89_phy_write32_idx(rtwdev, R_S0_HW_SI_DIS, B_S0_HW_SI_DIS_W_R_TRIG, 0x0, phy_idx);
	rtw89_phy_write32_idx(rtwdev, R_S1_HW_SI_DIS, B_S1_HW_SI_DIS_W_R_TRIG, 0x0, phy_idx);
	rtw89_phy_write32_idx(rtwdev, R_RSTB_ASYNC, B_RSTB_ASYNC_ALL, 1, phy_idx);
}

static void rtw8852b_bb_reset_en(struct rtw89_dev *rtwdev, enum rtw89_band band,
				 enum rtw89_phy_idx phy_idx, bool en)
{
	if (en) {
		rtw89_phy_write32_idx(rtwdev, R_S0_HW_SI_DIS,
				      B_S0_HW_SI_DIS_W_R_TRIG, 0x0, phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_S1_HW_SI_DIS,
				      B_S1_HW_SI_DIS_W_R_TRIG, 0x0, phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_RSTB_ASYNC, B_RSTB_ASYNC_ALL, 1, phy_idx);
		if (band == RTW89_BAND_2G)
			rtw89_phy_write32_mask(rtwdev, R_RXCCA, B_RXCCA_DIS, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_PD_CTRL, B_PD_HIT_DIS, 0x0);
	} else {
		rtw89_phy_write32_mask(rtwdev, R_RXCCA, B_RXCCA_DIS, 0x1);
		rtw89_phy_write32_mask(rtwdev, R_PD_CTRL, B_PD_HIT_DIS, 0x1);
		rtw89_phy_write32_idx(rtwdev, R_S0_HW_SI_DIS,
				      B_S0_HW_SI_DIS_W_R_TRIG, 0x7, phy_idx);
		rtw89_phy_write32_idx(rtwdev, R_S1_HW_SI_DIS,
				      B_S1_HW_SI_DIS_W_R_TRIG, 0x7, phy_idx);
		fsleep_alt(1);
		rtw89_phy_write32_idx(rtwdev, R_RSTB_ASYNC, B_RSTB_ASYNC_ALL, 0, phy_idx);
	}
}

static void rtw8852b_bb_reset(struct rtw89_dev *rtwdev,
			      enum rtw89_phy_idx phy_idx)
{
	rtw89_phy_write32_set(rtwdev, R_P0_TXPW_RSTB, B_P0_TXPW_RSTB_MANON);
	rtw89_phy_write32_set(rtwdev, R_P0_TSSI_TRK, B_P0_TSSI_TRK_EN);
	rtw89_phy_write32_set(rtwdev, R_P1_TXPW_RSTB, B_P1_TXPW_RSTB_MANON);
	rtw89_phy_write32_set(rtwdev, R_P1_TSSI_TRK, B_P1_TSSI_TRK_EN);
	rtw8852b_bb_reset_all(rtwdev, phy_idx);
	rtw89_phy_write32_clr(rtwdev, R_P0_TXPW_RSTB, B_P0_TXPW_RSTB_MANON);
	rtw89_phy_write32_clr(rtwdev, R_P0_TSSI_TRK, B_P0_TSSI_TRK_EN);
	rtw89_phy_write32_clr(rtwdev, R_P1_TXPW_RSTB, B_P1_TXPW_RSTB_MANON);
	rtw89_phy_write32_clr(rtwdev, R_P1_TSSI_TRK, B_P1_TSSI_TRK_EN);
}

static void rtw8852b_bb_macid_ctrl_init(struct rtw89_dev *rtwdev,
					enum rtw89_phy_idx phy_idx)
{
	u32 addr;

	for (addr = R_AX_PWR_MACID_LMT_TABLE0;
	     addr <= R_AX_PWR_MACID_LMT_TABLE127; addr += 4)
		rtw89_mac_txpwr_write32(rtwdev, phy_idx, addr, 0);
}

static void rtw8852b_bb_sethw(struct rtw89_dev *rtwdev)
{
	struct rtw89_phy_efuse_gain *gain = &rtwdev->efuse_gain;

	rtw89_phy_write32_clr(rtwdev, R_P0_EN_SOUND_WO_NDP, B_P0_EN_SOUND_WO_NDP);
	rtw89_phy_write32_clr(rtwdev, R_P1_EN_SOUND_WO_NDP, B_P1_EN_SOUND_WO_NDP);

	rtw8852b_bb_macid_ctrl_init(rtwdev, RTW89_PHY_0);

	/* read these registers after loading BB parameters */
	gain->offset_base[RTW89_PHY_0] =
		rtw89_phy_read32_mask(rtwdev, R_P0_RPL1, B_P0_RPL1_BIAS_MASK);
	gain->rssi_base[RTW89_PHY_0] =
		rtw89_phy_read32_mask(rtwdev, R_P1_RPL1, B_P0_RPL1_BIAS_MASK);
}

static void rtw8852b_bb_set_pop(struct rtw89_dev *rtwdev)
{
	if (rtwdev->hw->conf.flags & IEEE80211_CONF_MONITOR)
		rtw89_phy_write32_clr(rtwdev, R_PKT_CTRL, B_PKT_POP_EN);
}

static void rtw8852b_set_channel_bb(struct rtw89_dev *rtwdev, const struct rtw89_chan *chan,
				    enum rtw89_phy_idx phy_idx)
{
	bool cck_en = chan->channel <= 14;
	u8 pri_ch_idx = chan->pri_ch_idx;

	if (cck_en)
		rtw8852b_ctrl_sco_cck(rtwdev,  chan->primary_channel);

	rtw8852b_ctrl_ch(rtwdev, chan, phy_idx);
	rtw8852b_ctrl_bw(rtwdev, pri_ch_idx, chan->band_width, phy_idx);
	rtw8852b_ctrl_cck_en(rtwdev, cck_en);
	if (chan->band_type == RTW89_BAND_5G) {
		rtw89_phy_write32_mask(rtwdev, R_PATH0_BT_SHARE_V1,
				       B_PATH0_BT_SHARE_V1, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_PATH0_BTG_PATH_V1,
				       B_PATH0_BTG_PATH_V1, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_PATH1_BT_SHARE_V1,
				       B_PATH1_BT_SHARE_V1, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_PATH1_BTG_PATH_V1,
				       B_PATH1_BTG_PATH_V1, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_CHBW_MOD_V1, B_BT_SHARE, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_FC0_BW_V1, B_ANT_RX_BT_SEG0, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_BT_DYN_DC_EST_EN_V1,
				       B_BT_DYN_DC_EST_EN_MSK, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_GNT_BT_WGT_EN, B_GNT_BT_WGT_EN, 0x0);
	}
	rtw89_phy_write32_mask(rtwdev, R_MAC_PIN_SEL, B_CH_IDX_SEG0,
			       chan->primary_channel);
	rtw8852b_5m_mask(rtwdev, chan, phy_idx);
	rtw8852b_bb_set_pop(rtwdev);
	rtw8852b_bb_reset_all(rtwdev, phy_idx);
}

static void rtw8852b_set_channel(struct rtw89_dev *rtwdev,
				 const struct rtw89_chan *chan,
				 enum rtw89_mac_idx mac_idx,
				 enum rtw89_phy_idx phy_idx)
{
	rtw8852b_set_channel_mac(rtwdev, chan, mac_idx);
	rtw8852b_set_channel_bb(rtwdev, chan, phy_idx);
	rtw8852b_set_channel_rf(rtwdev, chan, phy_idx);
}

static void rtw8852b_tssi_cont_en(struct rtw89_dev *rtwdev, bool en,
				  enum rtw89_rf_path path)
{
	static const u32 tssi_trk[2] = {R_P0_TSSI_TRK, R_P1_TSSI_TRK};
	static const u32 ctrl_bbrst[2] = {R_P0_TXPW_RSTB, R_P1_TXPW_RSTB};

	if (en) {
		rtw89_phy_write32_mask(rtwdev, ctrl_bbrst[path], B_P0_TXPW_RSTB_MANON, 0x0);
		rtw89_phy_write32_mask(rtwdev, tssi_trk[path], B_P0_TSSI_TRK_EN, 0x0);
	} else {
		rtw89_phy_write32_mask(rtwdev, ctrl_bbrst[path], B_P0_TXPW_RSTB_MANON, 0x1);
		rtw89_phy_write32_mask(rtwdev, tssi_trk[path], B_P0_TSSI_TRK_EN, 0x1);
	}
}

static void rtw8852b_tssi_cont_en_phyidx(struct rtw89_dev *rtwdev, bool en,
					 u8 phy_idx)
{
	if (!rtwdev->dbcc_en) {
		rtw8852b_tssi_cont_en(rtwdev, en, RF_PATH_A);
		rtw8852b_tssi_cont_en(rtwdev, en, RF_PATH_B);
	} else {
		if (phy_idx == RTW89_PHY_0)
			rtw8852b_tssi_cont_en(rtwdev, en, RF_PATH_A);
		else
			rtw8852b_tssi_cont_en(rtwdev, en, RF_PATH_B);
	}
}

static void rtw8852b_adc_en(struct rtw89_dev *rtwdev, bool en)
{
	if (en)
		rtw89_phy_write32_mask(rtwdev, R_ADC_FIFO, B_ADC_FIFO_RST, 0x0);
	else
		rtw89_phy_write32_mask(rtwdev, R_ADC_FIFO, B_ADC_FIFO_RST, 0xf);
}

static void rtw8852b_set_channel_help(struct rtw89_dev *rtwdev, bool enter,
				      struct rtw89_channel_help_params *p,
				      const struct rtw89_chan *chan,
				      enum rtw89_mac_idx mac_idx,
				      enum rtw89_phy_idx phy_idx)
{
	if (enter) {
		rtw89_chip_stop_sch_tx(rtwdev, RTW89_MAC_0, &p->tx_en, RTW89_SCH_TX_SEL_ALL);
		rtw89_mac_cfg_ppdu_status(rtwdev, RTW89_MAC_0, false);
		rtw8852b_tssi_cont_en_phyidx(rtwdev, false, RTW89_PHY_0);
		rtw8852b_adc_en(rtwdev, false);
		fsleep_alt(40);
		rtw8852b_bb_reset_en(rtwdev, chan->band_type, phy_idx, false);
	} else {
		rtw89_mac_cfg_ppdu_status(rtwdev, RTW89_MAC_0, true);
		rtw8852b_adc_en(rtwdev, true);
		rtw8852b_tssi_cont_en_phyidx(rtwdev, true, RTW89_PHY_0);
		rtw8852b_bb_reset_en(rtwdev, chan->band_type, phy_idx, true);
		rtw89_chip_resume_sch_tx(rtwdev, RTW89_MAC_0, p->tx_en);
	}
}

static void rtw8852b_rfk_init(struct rtw89_dev *rtwdev)
{
	rtwdev->is_tssi_mode[RF_PATH_A] = false;
	rtwdev->is_tssi_mode[RF_PATH_B] = false;

	rtw8852b_dpk_init(rtwdev);
	rtw8852b_rck(rtwdev);
	rtw8852b_dack(rtwdev);
	rtw8852b_rx_dck(rtwdev, RTW89_PHY_0);
}

static void rtw8852b_rfk_channel(struct rtw89_dev *rtwdev)
{
	enum rtw89_phy_idx phy_idx = RTW89_PHY_0;

	rtw8852b_rx_dck(rtwdev, phy_idx);
	rtw8852b_iqk(rtwdev, phy_idx);
	rtw8852b_tssi(rtwdev, phy_idx, true);
	rtw8852b_dpk(rtwdev, phy_idx);
}

static void rtw8852b_rfk_band_changed(struct rtw89_dev *rtwdev,
				      enum rtw89_phy_idx phy_idx)
{
	rtw8852b_tssi_scan(rtwdev, phy_idx);
}

static void rtw8852b_rfk_scan(struct rtw89_dev *rtwdev, bool start)
{
	rtw8852b_wifi_scan_notify(rtwdev, start, RTW89_PHY_0);
}

static void rtw8852b_rfk_track(struct rtw89_dev *rtwdev)
{
	rtw8852b_dpk_track(rtwdev);
}

static u32 rtw8852b_bb_cal_txpwr_ref(struct rtw89_dev *rtwdev,
				     enum rtw89_phy_idx phy_idx, s16 ref)
{
	const u16 tssi_16dbm_cw = 0x12c;
	const u8 base_cw_0db = 0x27;
	const s8 ofst_int = 0;
	s16 pwr_s10_3;
	s16 rf_pwr_cw;
	u16 bb_pwr_cw;
	u32 pwr_cw;
	u32 tssi_ofst_cw;

	pwr_s10_3 = (ref << 1) + (s16)(ofst_int) + (s16)(base_cw_0db << 3);
	bb_pwr_cw = FIELD_GET(GENMASK(2, 0), pwr_s10_3);
	rf_pwr_cw = FIELD_GET(GENMASK(8, 3), pwr_s10_3);
	rf_pwr_cw = clamp_t(s16, rf_pwr_cw, 15, 63);
	pwr_cw = (rf_pwr_cw << 3) | bb_pwr_cw;

	tssi_ofst_cw = (u32)((s16)tssi_16dbm_cw + (ref << 1) - (16 << 3));
	rtw89_debug(rtwdev, RTW89_DBG_TXPWR,
		    "[TXPWR] tssi_ofst_cw=%d rf_cw=0x%x bb_cw=0x%x\n",
		    tssi_ofst_cw, rf_pwr_cw, bb_pwr_cw);

	return FIELD_PREP(B_DPD_TSSI_CW, tssi_ofst_cw) |
	       FIELD_PREP(B_DPD_PWR_CW, pwr_cw) |
	       FIELD_PREP(B_DPD_REF, ref);
}

static void rtw8852b_set_txpwr_ref(struct rtw89_dev *rtwdev,
				   enum rtw89_phy_idx phy_idx)
{
	static const u32 addr[RF_PATH_NUM_8852B] = {0x5800, 0x7800};
	const u32 mask = B_DPD_TSSI_CW | B_DPD_PWR_CW | B_DPD_REF;
	const u8 ofst_ofdm = 0x4;
	const u8 ofst_cck = 0x8;
	const s16 ref_ofdm = 0;
	const s16 ref_cck = 0;
	u32 val;
	u8 i;

	rtw89_debug(rtwdev, RTW89_DBG_TXPWR, "[TXPWR] set txpwr reference\n");

	rtw89_mac_txpwr_write32_mask(rtwdev, phy_idx, R_AX_PWR_RATE_CTRL,
				     B_AX_PWR_REF, 0x0);

	rtw89_debug(rtwdev, RTW89_DBG_TXPWR, "[TXPWR] set bb ofdm txpwr ref\n");
	val = rtw8852b_bb_cal_txpwr_ref(rtwdev, phy_idx, ref_ofdm);

	for (i = 0; i < RF_PATH_NUM_8852B; i++)
		rtw89_phy_write32_idx(rtwdev, addr[i] + ofst_ofdm, mask, val,
				      phy_idx);

	rtw89_debug(rtwdev, RTW89_DBG_TXPWR, "[TXPWR] set bb cck txpwr ref\n");
	val = rtw8852b_bb_cal_txpwr_ref(rtwdev, phy_idx, ref_cck);

	for (i = 0; i < RF_PATH_NUM_8852B; i++)
		rtw89_phy_write32_idx(rtwdev, addr[i] + ofst_cck, mask, val,
				      phy_idx);
}

static void rtw8852b_bb_set_tx_shape_dfir(struct rtw89_dev *rtwdev,
					  const struct rtw89_chan *chan,
					  u8 tx_shape_idx,
					  enum rtw89_phy_idx phy_idx)
{
#define __DFIR_CFG_ADDR(i) (R_TXFIR0 + ((i) << 2))
#define __DFIR_CFG_MASK 0xffffffff
#define __DFIR_CFG_NR 8
#define __DECL_DFIR_PARAM(_name, _val...) \
	static const u32 param_ ## _name[] = {_val}; \
	static_assert(ARRAY_SIZE(param_ ## _name) == __DFIR_CFG_NR)

	__DECL_DFIR_PARAM(flat,
			  0x023D23FF, 0x0029B354, 0x000FC1C8, 0x00FDB053,
			  0x00F86F9A, 0x06FAEF92, 0x00FE5FCC, 0x00FFDFF5);
	__DECL_DFIR_PARAM(sharp,
			  0x023D83FF, 0x002C636A, 0x0013F204, 0x00008090,
			  0x00F87FB0, 0x06F99F83, 0x00FDBFBA, 0x00003FF5);
	__DECL_DFIR_PARAM(sharp_14,
			  0x023B13FF, 0x001C42DE, 0x00FDB0AD, 0x00F60F6E,
			  0x00FD8F92, 0x0602D011, 0x0001C02C, 0x00FFF00A);
	u8 ch = chan->channel;
	const u32 *param;
	u32 addr;
	int i;

	if (ch > 14) {
		rtw89_warn(rtwdev,
			   "set tx shape dfir by unknown ch: %d on 2G\n", ch);
		return;
	}

	if (ch == 14)
		param = param_sharp_14;
	else
		param = tx_shape_idx == 0 ? param_flat : param_sharp;

	for (i = 0; i < __DFIR_CFG_NR; i++) {
		addr = __DFIR_CFG_ADDR(i);
		rtw89_debug(rtwdev, RTW89_DBG_TXPWR,
			    "set tx shape dfir: 0x%x: 0x%x\n", addr, param[i]);
		rtw89_phy_write32_idx(rtwdev, addr, __DFIR_CFG_MASK, param[i],
				      phy_idx);
	}

#undef __DECL_DFIR_PARAM
#undef __DFIR_CFG_NR
#undef __DFIR_CFG_MASK
#undef __DECL_CFG_ADDR
}

static void rtw8852b_set_tx_shape(struct rtw89_dev *rtwdev,
				  const struct rtw89_chan *chan,
				  enum rtw89_phy_idx phy_idx)
{
	u8 band = chan->band_type;
	u8 regd = rtw89_regd_get(rtwdev, band);
	u8 tx_shape_cck = rtw89_8852b_tx_shape[band][RTW89_RS_CCK][regd];
	u8 tx_shape_ofdm = rtw89_8852b_tx_shape[band][RTW89_RS_OFDM][regd];

	if (band == RTW89_BAND_2G)
		rtw8852b_bb_set_tx_shape_dfir(rtwdev, chan, tx_shape_cck, phy_idx);

	rtw89_phy_write32_mask(rtwdev, R_DCFO_OPT, B_TXSHAPE_TRIANGULAR_CFG,
			       tx_shape_ofdm);
}

static void rtw8852b_set_txpwr(struct rtw89_dev *rtwdev,
			       const struct rtw89_chan *chan,
			       enum rtw89_phy_idx phy_idx)
{
	rtw89_phy_set_txpwr_byrate(rtwdev, chan, phy_idx);
	rtw89_phy_set_txpwr_offset(rtwdev, chan, phy_idx);
	rtw8852b_set_tx_shape(rtwdev, chan, phy_idx);
	rtw89_phy_set_txpwr_limit(rtwdev, chan, phy_idx);
	rtw89_phy_set_txpwr_limit_ru(rtwdev, chan, phy_idx);
}

static void rtw8852b_set_txpwr_ctrl(struct rtw89_dev *rtwdev,
				    enum rtw89_phy_idx phy_idx)
{
	rtw8852b_set_txpwr_ref(rtwdev, phy_idx);
}

static
void rtw8852b_set_txpwr_ul_tb_offset(struct rtw89_dev *rtwdev,
				     s8 pw_ofst, enum rtw89_mac_idx mac_idx)
{
	u32 reg;

	if (pw_ofst < -16 || pw_ofst > 15) {
		rtw89_warn(rtwdev, "[ULTB] Err pwr_offset=%d\n", pw_ofst);
		return;
	}

	reg = rtw89_mac_reg_by_idx(R_AX_PWR_UL_TB_CTRL, mac_idx);
	rtw89_write32_set(rtwdev, reg, B_AX_PWR_UL_TB_CTRL_EN);

	reg = rtw89_mac_reg_by_idx(R_AX_PWR_UL_TB_1T, mac_idx);
	rtw89_write32_mask(rtwdev, reg, B_AX_PWR_UL_TB_1T_MASK, pw_ofst);

	pw_ofst = max_t(s8, pw_ofst - 3, -16);
	reg = rtw89_mac_reg_by_idx(R_AX_PWR_UL_TB_2T, mac_idx);
	rtw89_write32_mask(rtwdev, reg, B_AX_PWR_UL_TB_2T_MASK, pw_ofst);
}

static int
rtw8852b_init_txpwr_unit(struct rtw89_dev *rtwdev, enum rtw89_phy_idx phy_idx)
{
	int ret;

	ret = rtw89_mac_txpwr_write32(rtwdev, phy_idx, R_AX_PWR_UL_CTRL2, 0x07763333);
	if (ret)
		return ret;

	ret = rtw89_mac_txpwr_write32(rtwdev, phy_idx, R_AX_PWR_COEXT_CTRL, 0x01ebf000);
	if (ret)
		return ret;

	ret = rtw89_mac_txpwr_write32(rtwdev, phy_idx, R_AX_PWR_UL_CTRL0, 0x0002f8ff);
	if (ret)
		return ret;

	rtw8852b_set_txpwr_ul_tb_offset(rtwdev, 0, phy_idx == RTW89_PHY_1 ?
						   RTW89_MAC_1 : RTW89_MAC_0);

	return 0;
}

void rtw8852b_bb_set_plcp_tx(struct rtw89_dev *rtwdev)
{
	const struct rtw89_reg3_def *def = rtw8852b_pmac_ht20_mcs7_tbl;
	u8 i;

	for (i = 0; i < ARRAY_SIZE(rtw8852b_pmac_ht20_mcs7_tbl); i++, def++)
		rtw89_phy_write32_mask(rtwdev, def->addr, def->mask, def->data);
}

static void rtw8852b_stop_pmac_tx(struct rtw89_dev *rtwdev,
				  struct rtw8852b_bb_pmac_info *tx_info,
				  enum rtw89_phy_idx idx)
{
	rtw89_debug(rtwdev, RTW89_DBG_TSSI, "PMAC Stop Tx");
	if (tx_info->mode == CONT_TX)
		rtw89_phy_write32_idx(rtwdev, R_PMAC_TX_PRD, B_PMAC_CTX_EN, 0, idx);
	else if (tx_info->mode == PKTS_TX)
		rtw89_phy_write32_idx(rtwdev, R_PMAC_TX_PRD, B_PMAC_PTX_EN, 0, idx);
}

static void rtw8852b_start_pmac_tx(struct rtw89_dev *rtwdev,
				   struct rtw8852b_bb_pmac_info *tx_info,
				   enum rtw89_phy_idx idx)
{
	enum rtw8852b_pmac_mode mode = tx_info->mode;
	u32 pkt_cnt = tx_info->tx_cnt;
	u16 period = tx_info->period;

	if (mode == CONT_TX && !tx_info->is_cck) {
		rtw89_phy_write32_idx(rtwdev, R_PMAC_TX_PRD, B_PMAC_CTX_EN, 1, idx);
		rtw89_debug(rtwdev, RTW89_DBG_TSSI, "PMAC CTx Start");
	} else if (mode == PKTS_TX) {
		rtw89_phy_write32_idx(rtwdev, R_PMAC_TX_PRD, B_PMAC_PTX_EN, 1, idx);
		rtw89_phy_write32_idx(rtwdev, R_PMAC_TX_PRD,
				      B_PMAC_TX_PRD_MSK, period, idx);
		rtw89_phy_write32_idx(rtwdev, R_PMAC_TX_CNT, B_PMAC_TX_CNT_MSK,
				      pkt_cnt, idx);
		rtw89_debug(rtwdev, RTW89_DBG_TSSI, "PMAC PTx Start");
	}

	rtw89_phy_write32_idx(rtwdev, R_PMAC_TX_CTRL, B_PMAC_TXEN_DIS, 1, idx);
	rtw89_phy_write32_idx(rtwdev, R_PMAC_TX_CTRL, B_PMAC_TXEN_DIS, 0, idx);
}

void rtw8852b_bb_set_pmac_tx(struct rtw89_dev *rtwdev,
			     struct rtw8852b_bb_pmac_info *tx_info,
			     enum rtw89_phy_idx idx)
{
	const struct rtw89_chan *chan = rtw89_chan_get(rtwdev, RTW89_SUB_ENTITY_0);

	if (!tx_info->en_pmac_tx) {
		rtw8852b_stop_pmac_tx(rtwdev, tx_info, idx);
		rtw89_phy_write32_idx(rtwdev, R_PD_CTRL, B_PD_HIT_DIS, 0, idx);
		if (chan->band_type == RTW89_BAND_2G)
			rtw89_phy_write32_clr(rtwdev, R_RXCCA, B_RXCCA_DIS);
		return;
	}

	rtw89_debug(rtwdev, RTW89_DBG_TSSI, "PMAC Tx Enable");

	rtw89_phy_write32_idx(rtwdev, R_PMAC_GNT, B_PMAC_GNT_TXEN, 1, idx);
	rtw89_phy_write32_idx(rtwdev, R_PMAC_GNT, B_PMAC_GNT_RXEN, 1, idx);
	rtw89_phy_write32_idx(rtwdev, R_PMAC_RX_CFG1, B_PMAC_OPT1_MSK, 0x3f, idx);
	rtw89_phy_write32_idx(rtwdev, R_RSTB_ASYNC, B_RSTB_ASYNC_ALL, 0, idx);
	rtw89_phy_write32_idx(rtwdev, R_PD_CTRL, B_PD_HIT_DIS, 1, idx);
	rtw89_phy_write32_set(rtwdev, R_RXCCA, B_RXCCA_DIS);
	rtw89_phy_write32_idx(rtwdev, R_RSTB_ASYNC, B_RSTB_ASYNC_ALL, 1, idx);

	rtw8852b_start_pmac_tx(rtwdev, tx_info, idx);
}

void rtw8852b_bb_set_pmac_pkt_tx(struct rtw89_dev *rtwdev, u8 enable,
				 u16 tx_cnt, u16 period, u16 tx_time,
				 enum rtw89_phy_idx idx)
{
	struct rtw8852b_bb_pmac_info tx_info = {0};

	tx_info.en_pmac_tx = enable;
	tx_info.is_cck = 0;
	tx_info.mode = PKTS_TX;
	tx_info.tx_cnt = tx_cnt;
	tx_info.period = period;
	tx_info.tx_time = tx_time;

	rtw8852b_bb_set_pmac_tx(rtwdev, &tx_info, idx);
}

void rtw8852b_bb_set_power(struct rtw89_dev *rtwdev, s16 pwr_dbm,
			   enum rtw89_phy_idx idx)
{
	rtw89_debug(rtwdev, RTW89_DBG_TSSI, "PMAC CFG Tx PWR = %d", pwr_dbm);

	rtw89_phy_write32_idx(rtwdev, R_MAC_SEL, B_MAC_SEL_PWR_EN, 1, idx);
	rtw89_phy_write32_idx(rtwdev, R_TXPWR, B_TXPWR_MSK, pwr_dbm, idx);
}

void rtw8852b_bb_cfg_tx_path(struct rtw89_dev *rtwdev, u8 tx_path)
{
	rtw89_phy_write32_idx(rtwdev, R_MAC_SEL, B_MAC_SEL_MOD, 7, RTW89_PHY_0);

	rtw89_debug(rtwdev, RTW89_DBG_TSSI, "PMAC CFG Tx Path = %d", tx_path);

	if (tx_path == RF_PATH_A) {
		rtw89_phy_write32_mask(rtwdev, R_TXPATH_SEL, B_TXPATH_SEL_MSK, 1);
		rtw89_phy_write32_mask(rtwdev, R_TXNSS_MAP, B_TXNSS_MAP_MSK, 0);
	} else if (tx_path == RF_PATH_B) {
		rtw89_phy_write32_mask(rtwdev, R_TXPATH_SEL, B_TXPATH_SEL_MSK, 2);
		rtw89_phy_write32_mask(rtwdev, R_TXNSS_MAP, B_TXNSS_MAP_MSK, 0);
	} else if (tx_path == RF_PATH_AB) {
		rtw89_phy_write32_mask(rtwdev, R_TXPATH_SEL, B_TXPATH_SEL_MSK, 3);
		rtw89_phy_write32_mask(rtwdev, R_TXNSS_MAP, B_TXNSS_MAP_MSK, 4);
	} else {
		rtw89_debug(rtwdev, RTW89_DBG_TSSI, "Error Tx Path");
	}
}

void rtw8852b_bb_tx_mode_switch(struct rtw89_dev *rtwdev,
				enum rtw89_phy_idx idx, u8 mode)
{
	if (mode != 0)
		return;

	rtw89_debug(rtwdev, RTW89_DBG_TSSI, "Tx mode switch");

	rtw89_phy_write32_idx(rtwdev, R_PMAC_GNT, B_PMAC_GNT_TXEN, 0, idx);
	rtw89_phy_write32_idx(rtwdev, R_PMAC_GNT, B_PMAC_GNT_RXEN, 0, idx);
	rtw89_phy_write32_idx(rtwdev, R_PMAC_RX_CFG1, B_PMAC_OPT1_MSK, 0, idx);
	rtw89_phy_write32_idx(rtwdev, R_PMAC_RXMOD, B_PMAC_RXMOD_MSK, 0, idx);
	rtw89_phy_write32_idx(rtwdev, R_MAC_SEL, B_MAC_SEL_DPD_EN, 0, idx);
	rtw89_phy_write32_idx(rtwdev, R_MAC_SEL, B_MAC_SEL_MOD, 0, idx);
	rtw89_phy_write32_idx(rtwdev, R_MAC_SEL, B_MAC_SEL_PWR_EN, 0, idx);
}

void rtw8852b_bb_backup_tssi(struct rtw89_dev *rtwdev, enum rtw89_phy_idx idx,
			     struct rtw8852b_bb_tssi_bak *bak)
{
	s32 tmp;

	bak->tx_path = rtw89_phy_read32_idx(rtwdev, R_TXPATH_SEL, B_TXPATH_SEL_MSK, idx);
	bak->rx_path = rtw89_phy_read32_idx(rtwdev, R_CHBW_MOD_V1, B_ANT_RX_SEG0, idx);
	bak->p0_rfmode = rtw89_phy_read32_idx(rtwdev, R_P0_RFMODE, MASKDWORD, idx);
	bak->p0_rfmode_ftm = rtw89_phy_read32_idx(rtwdev, R_P0_RFMODE_FTM_RX, MASKDWORD, idx);
	bak->p1_rfmode = rtw89_phy_read32_idx(rtwdev, R_P1_RFMODE, MASKDWORD, idx);
	bak->p1_rfmode_ftm = rtw89_phy_read32_idx(rtwdev, R_P1_RFMODE_FTM_RX, MASKDWORD, idx);
	tmp = rtw89_phy_read32_idx(rtwdev, R_TXPWR, B_TXPWR_MSK, idx);
	bak->tx_pwr = sign_extend32(tmp, 8);
}

void rtw8852b_bb_restore_tssi(struct rtw89_dev *rtwdev, enum rtw89_phy_idx idx,
			      const struct rtw8852b_bb_tssi_bak *bak)
{
	rtw89_phy_write32_idx(rtwdev, R_TXPATH_SEL, B_TXPATH_SEL_MSK, bak->tx_path, idx);
	if (bak->tx_path == RF_AB)
		rtw89_phy_write32_mask(rtwdev, R_TXNSS_MAP, B_TXNSS_MAP_MSK, 0x4);
	else
		rtw89_phy_write32_mask(rtwdev, R_TXNSS_MAP, B_TXNSS_MAP_MSK, 0x0);
	rtw89_phy_write32_idx(rtwdev, R_CHBW_MOD_V1, B_ANT_RX_SEG0, bak->rx_path, idx);
	rtw89_phy_write32_idx(rtwdev, R_MAC_SEL, B_MAC_SEL_PWR_EN, 1, idx);
	rtw89_phy_write32_idx(rtwdev, R_P0_RFMODE, MASKDWORD, bak->p0_rfmode, idx);
	rtw89_phy_write32_idx(rtwdev, R_P0_RFMODE_FTM_RX, MASKDWORD, bak->p0_rfmode_ftm, idx);
	rtw89_phy_write32_idx(rtwdev, R_P1_RFMODE, MASKDWORD, bak->p1_rfmode, idx);
	rtw89_phy_write32_idx(rtwdev, R_P1_RFMODE_FTM_RX, MASKDWORD, bak->p1_rfmode_ftm, idx);
	rtw89_phy_write32_idx(rtwdev, R_TXPWR, B_TXPWR_MSK, bak->tx_pwr, idx);
}

static void rtw8852b_bb_ctrl_btc_preagc(struct rtw89_dev *rtwdev, bool bt_en)
{
	rtw89_phy_write_reg3_tbl(rtwdev, bt_en ? &rtw8852b_btc_preagc_en_defs_tbl :
						 &rtw8852b_btc_preagc_dis_defs_tbl);
}

static void rtw8852b_ctrl_btg(struct rtw89_dev *rtwdev, bool btg)
{
	if (btg) {
		rtw89_phy_write32_mask(rtwdev, R_PATH0_BT_SHARE_V1,
				       B_PATH0_BT_SHARE_V1, 0x1);
		rtw89_phy_write32_mask(rtwdev, R_PATH0_BTG_PATH_V1,
				       B_PATH0_BTG_PATH_V1, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_PATH1_G_LNA6_OP1DB_V1,
				       B_PATH1_G_LNA6_OP1DB_V1, 0x20);
		rtw89_phy_write32_mask(rtwdev, R_PATH1_G_TIA0_LNA6_OP1DB_V1,
				       B_PATH1_G_TIA0_LNA6_OP1DB_V1, 0x30);
		rtw89_phy_write32_mask(rtwdev, R_PATH1_BT_SHARE_V1,
				       B_PATH1_BT_SHARE_V1, 0x1);
		rtw89_phy_write32_mask(rtwdev, R_PATH1_BTG_PATH_V1,
				       B_PATH1_BTG_PATH_V1, 0x1);
		rtw89_phy_write32_mask(rtwdev, R_PMAC_GNT, B_PMAC_GNT_P1, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_CHBW_MOD_V1, B_BT_SHARE, 0x1);
		rtw89_phy_write32_mask(rtwdev, R_FC0_BW_V1, B_ANT_RX_BT_SEG0, 0x2);
		rtw89_phy_write32_mask(rtwdev, R_BT_DYN_DC_EST_EN_V1,
				       B_BT_DYN_DC_EST_EN_MSK, 0x1);
		rtw89_phy_write32_mask(rtwdev, R_GNT_BT_WGT_EN, B_GNT_BT_WGT_EN, 0x1);
	} else {
		rtw89_phy_write32_mask(rtwdev, R_PATH0_BT_SHARE_V1,
				       B_PATH0_BT_SHARE_V1, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_PATH0_BTG_PATH_V1,
				       B_PATH0_BTG_PATH_V1, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_PATH1_G_LNA6_OP1DB_V1,
				       B_PATH1_G_LNA6_OP1DB_V1, 0x1a);
		rtw89_phy_write32_mask(rtwdev, R_PATH1_G_TIA0_LNA6_OP1DB_V1,
				       B_PATH1_G_TIA0_LNA6_OP1DB_V1, 0x2a);
		rtw89_phy_write32_mask(rtwdev, R_PATH1_BT_SHARE_V1,
				       B_PATH1_BT_SHARE_V1, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_PATH1_BTG_PATH_V1,
				       B_PATH1_BTG_PATH_V1, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_PMAC_GNT, B_PMAC_GNT_P1, 0xc);
		rtw89_phy_write32_mask(rtwdev, R_CHBW_MOD_V1, B_BT_SHARE, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_FC0_BW_V1, B_ANT_RX_BT_SEG0, 0x0);
		rtw89_phy_write32_mask(rtwdev, R_BT_DYN_DC_EST_EN_V1,
				       B_BT_DYN_DC_EST_EN_MSK, 0x1);
		rtw89_phy_write32_mask(rtwdev, R_GNT_BT_WGT_EN, B_GNT_BT_WGT_EN, 0x0);
	}
}

void rtw8852b_bb_ctrl_rx_path(struct rtw89_dev *rtwdev,
			      enum rtw89_rf_path_bit rx_path)
{
	const struct rtw89_chan *chan = rtw89_chan_get(rtwdev, RTW89_SUB_ENTITY_0);
	u32 rst_mask0;
	u32 rst_mask1;

	if (rx_path == RF_A) {
		rtw89_phy_write32_mask(rtwdev, R_CHBW_MOD_V1, B_ANT_RX_SEG0, 1);
		rtw89_phy_write32_mask(rtwdev, R_FC0_BW_V1, B_ANT_RX_1RCCA_SEG0, 1);
		rtw89_phy_write32_mask(rtwdev, R_FC0_BW_V1, B_ANT_RX_1RCCA_SEG1, 1);
		rtw89_phy_write32_mask(rtwdev, R_RXHT_MCS_LIMIT, B_RXHT_MCS_LIMIT, 0);
		rtw89_phy_write32_mask(rtwdev, R_RXVHT_MCS_LIMIT, B_RXVHT_MCS_LIMIT, 0);
		rtw89_phy_write32_mask(rtwdev, R_RXHE, B_RXHE_USER_MAX, 4);
		rtw89_phy_write32_mask(rtwdev, R_RXHE, B_RXHE_MAX_NSS, 0);
		rtw89_phy_write32_mask(rtwdev, R_RXHE, B_RXHETB_MAX_NSS, 0);
	} else if (rx_path == RF_B) {
		rtw89_phy_write32_mask(rtwdev, R_CHBW_MOD_V1, B_ANT_RX_SEG0, 2);
		rtw89_phy_write32_mask(rtwdev, R_FC0_BW_V1, B_ANT_RX_1RCCA_SEG0, 2);
		rtw89_phy_write32_mask(rtwdev, R_FC0_BW_V1, B_ANT_RX_1RCCA_SEG1, 2);
		rtw89_phy_write32_mask(rtwdev, R_RXHT_MCS_LIMIT, B_RXHT_MCS_LIMIT, 0);
		rtw89_phy_write32_mask(rtwdev, R_RXVHT_MCS_LIMIT, B_RXVHT_MCS_LIMIT, 0);
		rtw89_phy_write32_mask(rtwdev, R_RXHE, B_RXHE_USER_MAX, 4);
		rtw89_phy_write32_mask(rtwdev, R_RXHE, B_RXHE_MAX_NSS, 0);
		rtw89_phy_write32_mask(rtwdev, R_RXHE, B_RXHETB_MAX_NSS, 0);
	} else if (rx_path == RF_AB) {
		rtw89_phy_write32_mask(rtwdev, R_CHBW_MOD_V1, B_ANT_RX_SEG0, 3);
		rtw89_phy_write32_mask(rtwdev, R_FC0_BW_V1, B_ANT_RX_1RCCA_SEG0, 3);
		rtw89_phy_write32_mask(rtwdev, R_FC0_BW_V1, B_ANT_RX_1RCCA_SEG1, 3);
		rtw89_phy_write32_mask(rtwdev, R_RXHT_MCS_LIMIT, B_RXHT_MCS_LIMIT, 1);
		rtw89_phy_write32_mask(rtwdev, R_RXVHT_MCS_LIMIT, B_RXVHT_MCS_LIMIT, 1);
		rtw89_phy_write32_mask(rtwdev, R_RXHE, B_RXHE_USER_MAX, 4);
		rtw89_phy_write32_mask(rtwdev, R_RXHE, B_RXHE_MAX_NSS, 1);
		rtw89_phy_write32_mask(rtwdev, R_RXHE, B_RXHETB_MAX_NSS, 1);
	}

	rtw8852b_set_gain_offset(rtwdev, chan->subband_type, RTW89_PHY_0);

	if (chan->band_type == RTW89_BAND_2G &&
	    (rx_path == RF_B || rx_path == RF_AB))
		rtw8852b_ctrl_btg(rtwdev, true);
	else
		rtw8852b_ctrl_btg(rtwdev, false);

	rst_mask0 = B_P0_TXPW_RSTB_MANON | B_P0_TXPW_RSTB_TSSI;
	rst_mask1 = B_P1_TXPW_RSTB_MANON | B_P1_TXPW_RSTB_TSSI;
	if (rx_path == RF_A) {
		rtw89_phy_write32_mask(rtwdev, R_P0_TXPW_RSTB, rst_mask0, 1);
		rtw89_phy_write32_mask(rtwdev, R_P0_TXPW_RSTB, rst_mask0, 3);
	} else {
		rtw89_phy_write32_mask(rtwdev, R_P1_TXPW_RSTB, rst_mask1, 1);
		rtw89_phy_write32_mask(rtwdev, R_P1_TXPW_RSTB, rst_mask1, 3);
	}
}

static void rtw8852b_bb_ctrl_rf_mode_rx_path(struct rtw89_dev *rtwdev,
					     enum rtw89_rf_path_bit rx_path)
{
	if (rx_path == RF_A) {
		rtw89_phy_write32_mask(rtwdev, R_P0_RFMODE,
				       B_P0_RFMODE_ORI_TXRX_FTM_TX, 0x1233312);
		rtw89_phy_write32_mask(rtwdev, R_P0_RFMODE_FTM_RX,
				       B_P0_RFMODE_FTM_RX, 0x333);
		rtw89_phy_write32_mask(rtwdev, R_P1_RFMODE,
				       B_P1_RFMODE_ORI_TXRX_FTM_TX, 0x1111111);
		rtw89_phy_write32_mask(rtwdev, R_P1_RFMODE_FTM_RX,
				       B_P1_RFMODE_FTM_RX, 0x111);
	} else if (rx_path == RF_B) {
		rtw89_phy_write32_mask(rtwdev, R_P0_RFMODE,
				       B_P0_RFMODE_ORI_TXRX_FTM_TX, 0x1111111);
		rtw89_phy_write32_mask(rtwdev, R_P0_RFMODE_FTM_RX,
				       B_P0_RFMODE_FTM_RX, 0x111);
		rtw89_phy_write32_mask(rtwdev, R_P1_RFMODE,
				       B_P1_RFMODE_ORI_TXRX_FTM_TX, 0x1233312);
		rtw89_phy_write32_mask(rtwdev, R_P1_RFMODE_FTM_RX,
				       B_P1_RFMODE_FTM_RX, 0x333);
	} else if (rx_path == RF_AB) {
		rtw89_phy_write32_mask(rtwdev, R_P0_RFMODE,
				       B_P0_RFMODE_ORI_TXRX_FTM_TX, 0x1233312);
		rtw89_phy_write32_mask(rtwdev, R_P0_RFMODE_FTM_RX,
				       B_P0_RFMODE_FTM_RX, 0x333);
		rtw89_phy_write32_mask(rtwdev, R_P1_RFMODE,
				       B_P1_RFMODE_ORI_TXRX_FTM_TX, 0x1233312);
		rtw89_phy_write32_mask(rtwdev, R_P1_RFMODE_FTM_RX,
				       B_P1_RFMODE_FTM_RX, 0x333);
	}
}

static void rtw8852b_bb_cfg_txrx_path(struct rtw89_dev *rtwdev)
{
	struct rtw89_hal *hal = &rtwdev->hal;
	enum rtw89_rf_path_bit rx_path = hal->antenna_rx ? hal->antenna_rx : RF_AB;

	rtw8852b_bb_ctrl_rx_path(rtwdev, rx_path);
	rtw8852b_bb_ctrl_rf_mode_rx_path(rtwdev, rx_path);

	if (rtwdev->hal.rx_nss == 1) {
		rtw89_phy_write32_mask(rtwdev, R_RXHT_MCS_LIMIT, B_RXHT_MCS_LIMIT, 0);
		rtw89_phy_write32_mask(rtwdev, R_RXVHT_MCS_LIMIT, B_RXVHT_MCS_LIMIT, 0);
		rtw89_phy_write32_mask(rtwdev, R_RXHE, B_RXHE_MAX_NSS, 0);
		rtw89_phy_write32_mask(rtwdev, R_RXHE, B_RXHETB_MAX_NSS, 0);
	} else {
		rtw89_phy_write32_mask(rtwdev, R_RXHT_MCS_LIMIT, B_RXHT_MCS_LIMIT, 1);
		rtw89_phy_write32_mask(rtwdev, R_RXVHT_MCS_LIMIT, B_RXVHT_MCS_LIMIT, 1);
		rtw89_phy_write32_mask(rtwdev, R_RXHE, B_RXHE_MAX_NSS, 1);
		rtw89_phy_write32_mask(rtwdev, R_RXHE, B_RXHETB_MAX_NSS, 1);
	}

	rtw89_phy_write32_idx(rtwdev, R_MAC_SEL, B_MAC_SEL_MOD, 0x0, RTW89_PHY_0);
}

static u8 rtw8852b_get_thermal(struct rtw89_dev *rtwdev, enum rtw89_rf_path rf_path)
{
	if (rtwdev->is_tssi_mode[rf_path]) {
		u32 addr = 0x1c10 + (rf_path << 13);

		return rtw89_phy_read32_mask(rtwdev, addr, 0x3F000000);
	}

	rtw89_write_rf(rtwdev, rf_path, RR_TM, RR_TM_TRI, 0x1);
	rtw89_write_rf(rtwdev, rf_path, RR_TM, RR_TM_TRI, 0x0);
	rtw89_write_rf(rtwdev, rf_path, RR_TM, RR_TM_TRI, 0x1);

	fsleep_alt(200);

	return rtw89_read_rf(rtwdev, rf_path, RR_TM, RR_TM_VAL);
}

static void rtw8852b_btc_set_rfe(struct rtw89_dev *rtwdev)
{
	struct rtw89_btc *btc = &rtwdev->btc;
	struct rtw89_btc_module *module = &btc->mdinfo;

	module->rfe_type = rtwdev->efuse.rfe_type;
	module->cv = rtwdev->hal.cv;
	module->bt_solo = 0;
	module->switch_type = BTC_SWITCH_INTERNAL;

	if (module->rfe_type > 0)
		module->ant.num = module->rfe_type % 2 ? 2 : 3;
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
void rtw8852b_set_trx_mask(struct rtw89_dev *rtwdev, u8 path, u8 group, u32 val)
{
	rtw89_write_rf(rtwdev, path, RR_LUTWE, RFREG_MASK, 0x20000);
	rtw89_write_rf(rtwdev, path, RR_LUTWA, RFREG_MASK, group);
	rtw89_write_rf(rtwdev, path, RR_LUTWD0, RFREG_MASK, val);
	rtw89_write_rf(rtwdev, path, RR_LUTWE, RFREG_MASK, 0x0);
}

static void rtw8852b_btc_init_cfg(struct rtw89_dev *rtwdev)
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
	chip->ops->btc_set_wl_pri(rtwdev, BTC_PRI_MASK_BEACON, true);

	/* set rf gnt debug off */
	rtw89_write_rf(rtwdev, RF_PATH_A, RR_WLSEL, RFREG_MASK, 0x0);
	rtw89_write_rf(rtwdev, RF_PATH_B, RR_WLSEL, RFREG_MASK, 0x0);

	/* set WL Tx thru in TRX mask table if GNT_WL = 0 && BT_S1 = ss group */
	if (module->ant.type == BTC_ANT_SHARED) {
		rtw8852b_set_trx_mask(rtwdev, RF_PATH_A, BTC_BT_SS_GROUP, 0x5ff);
		rtw8852b_set_trx_mask(rtwdev, RF_PATH_B, BTC_BT_SS_GROUP, 0x5ff);
		/* set path-A(S0) Tx/Rx no-mask if GNT_WL=0 && BT_S1=tx group */
		rtw8852b_set_trx_mask(rtwdev, RF_PATH_A, BTC_BT_TX_GROUP, 0x5ff);
		rtw8852b_set_trx_mask(rtwdev, RF_PATH_B, BTC_BT_TX_GROUP, 0x55f);
	} else { /* set WL Tx stb if GNT_WL = 0 && BT_S1 = ss group for 3-ant */
		rtw8852b_set_trx_mask(rtwdev, RF_PATH_A, BTC_BT_SS_GROUP, 0x5df);
		rtw8852b_set_trx_mask(rtwdev, RF_PATH_B, BTC_BT_SS_GROUP, 0x5df);
		rtw8852b_set_trx_mask(rtwdev, RF_PATH_A, BTC_BT_TX_GROUP, 0x5ff);
		rtw8852b_set_trx_mask(rtwdev, RF_PATH_B, BTC_BT_TX_GROUP, 0x5ff);
	}

	/* set PTA break table */
	rtw89_write32(rtwdev, R_BTC_BREAK_TABLE, BTC_BREAK_PARAM);

	 /* enable BT counter 0xda40[16,2] = 2b'11 */
	rtw89_write32_set(rtwdev, R_AX_CSR_MODE, B_AX_BT_CNT_RST | B_AX_STATIS_BT_EN);
	btc->cx.wl.status.map.init_ok = true;
}

static
void rtw8852b_btc_set_wl_pri(struct rtw89_dev *rtwdev, u8 map, bool state)
{
	u32 bitmap;
	u32 reg;

	switch (map) {
	case BTC_PRI_MASK_TX_RESP:
		reg = R_BTC_BT_COEX_MSK_TABLE;
		bitmap = B_BTC_PRI_MASK_TX_RESP_V1;
		break;
	case BTC_PRI_MASK_BEACON:
		reg = R_AX_WL_PRI_MSK;
		bitmap = B_AX_PTA_WL_PRI_MASK_BCNQ;
		break;
	case BTC_PRI_MASK_RX_CCK:
		reg = R_BTC_BT_COEX_MSK_TABLE;
		bitmap = B_BTC_PRI_MASK_RXCCK_V1;
		break;
	default:
		return;
	}

	if (state)
		rtw89_write32_set(rtwdev, reg, bitmap);
	else
		rtw89_write32_clr(rtwdev, reg, bitmap);
}

union rtw8852b_btc_wl_txpwr_ctrl {
	u32 txpwr_val;
	struct {
		union {
			u16 ctrl_all_time;
			struct {
				s16 data:9;
				u16 rsvd:6;
				u16 flag:1;
			} all_time;
		};
		union {
			u16 ctrl_gnt_bt;
			struct {
				s16 data:9;
				u16 rsvd:7;
			} gnt_bt;
		};
	};
} __packed;

static void
rtw8852b_btc_set_wl_txpwr_ctrl(struct rtw89_dev *rtwdev, u32 txpwr_val)
{
	union rtw8852b_btc_wl_txpwr_ctrl arg = { .txpwr_val = txpwr_val };
	s32 val;

#define __write_ctrl(_reg, _msk, _val, _en, _cond)		\
do {								\
	u32 _wrt = FIELD_PREP(_msk, _val);			\
	BUILD_BUG_ON(!!(_msk & _en));				\
	if (_cond)						\
		_wrt |= _en;					\
	else							\
		_wrt &= ~_en;					\
	rtw89_mac_txpwr_write32_mask(rtwdev, RTW89_PHY_0, _reg,	\
				     _msk | _en, _wrt);		\
} while (0)

	switch (arg.ctrl_all_time) {
	case 0xffff:
		val = 0;
		break;
	default:
		val = arg.all_time.data;
		break;
	}

	__write_ctrl(R_AX_PWR_RATE_CTRL, B_AX_FORCE_PWR_BY_RATE_VALUE_MASK,
		     val, B_AX_FORCE_PWR_BY_RATE_EN,
		     arg.ctrl_all_time != 0xffff);

	switch (arg.ctrl_gnt_bt) {
	case 0xffff:
		val = 0;
		break;
	default:
		val = arg.gnt_bt.data;
		break;
	}

	__write_ctrl(R_AX_PWR_COEXT_CTRL, B_AX_TXAGC_BT_MASK, val,
		     B_AX_TXAGC_BT_EN, arg.ctrl_gnt_bt != 0xffff);

#undef __write_ctrl
}

static
s8 rtw8852b_btc_get_bt_rssi(struct rtw89_dev *rtwdev, s8 val)
{
	return clamp_t(s8, val, -100, 0) + 100;
}

static
void rtw8852b_btc_update_bt_cnt(struct rtw89_dev *rtwdev)
{
	/* Feature move to firmware */
}

static void rtw8852b_btc_wl_s1_standby(struct rtw89_dev *rtwdev, bool state)
{
	rtw89_write_rf(rtwdev, RF_PATH_B, RR_LUTWE, RFREG_MASK, 0x80000);
	rtw89_write_rf(rtwdev, RF_PATH_B, RR_LUTWA, RFREG_MASK, 0x1);
	rtw89_write_rf(rtwdev, RF_PATH_B, RR_LUTWD1, RFREG_MASK, 0x31);

	/* set WL standby = Rx for GNT_BT_Tx = 1->0 settle issue */
	if (state)
		rtw89_write_rf(rtwdev, RF_PATH_B, RR_LUTWD0, RFREG_MASK, 0x579);
	else
		rtw89_write_rf(rtwdev, RF_PATH_B, RR_LUTWD0, RFREG_MASK, 0x20);

	rtw89_write_rf(rtwdev, RF_PATH_B, RR_LUTWE, RFREG_MASK, 0x0);
}

static void rtw8852b_btc_set_wl_rx_gain(struct rtw89_dev *rtwdev, u32 level)
{
}

static void rtw8852b_fill_freq_with_ppdu(struct rtw89_dev *rtwdev,
					 struct rtw89_rx_phy_ppdu *phy_ppdu,
					 struct ieee80211_rx_status *status)
{
	u16 chan = phy_ppdu->chan_idx;
	u8 band;

	if (chan == 0)
		return;

	band = chan <= 14 ? NL80211_BAND_2GHZ : NL80211_BAND_5GHZ;
	status->freq = ieee80211_channel_to_frequency(chan, band);
	status->band = band;
}

static void rtw8852b_query_ppdu(struct rtw89_dev *rtwdev,
				struct rtw89_rx_phy_ppdu *phy_ppdu,
				struct ieee80211_rx_status *status)
{
	u8 path;
	u8 *rx_power = phy_ppdu->rssi;

	status->signal = RTW89_RSSI_RAW_TO_DBM(max(rx_power[RF_PATH_A], rx_power[RF_PATH_B]));
	for (path = 0; path < rtwdev->chip->rf_path_num; path++) {
		status->chains |= BIT(path);
		status->chain_signal[path] = RTW89_RSSI_RAW_TO_DBM(rx_power[path]);
	}
	if (phy_ppdu->valid)
		rtw8852b_fill_freq_with_ppdu(rtwdev, phy_ppdu, status);
}

static int rtw8852b_mac_enable_bb_rf(struct rtw89_dev *rtwdev)
{
	int ret;

	rtw89_write8_set(rtwdev, R_AX_SYS_FUNC_EN,
			 B_AX_FEN_BBRSTB | B_AX_FEN_BB_GLB_RSTN);
	rtw89_write32_mask(rtwdev, R_AX_SPS_DIG_ON_CTRL0, B_AX_REG_ZCDC_H_MASK, 0x1);
	rtw89_write32_set(rtwdev, R_AX_WLRF_CTRL, B_AX_AFC_AFEDIG);
	rtw89_write32_clr(rtwdev, R_AX_WLRF_CTRL, B_AX_AFC_AFEDIG);
	rtw89_write32_set(rtwdev, R_AX_WLRF_CTRL, B_AX_AFC_AFEDIG);

	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_WL_RFC_S0, 0xC7,
				      FULL_BIT_MASK);
	if (ret)
		return ret;

	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_WL_RFC_S1, 0xC7,
				      FULL_BIT_MASK);
	if (ret)
		return ret;

	rtw89_write8(rtwdev, R_AX_PHYREG_SET, PHYREG_SET_XYN_CYCLE);

	return 0;
}

static int rtw8852b_mac_disable_bb_rf(struct rtw89_dev *rtwdev)
{
	u8 wl_rfc_s0;
	u8 wl_rfc_s1;
	int ret;

	rtw89_write8_clr(rtwdev, R_AX_SYS_FUNC_EN,
			 B_AX_FEN_BBRSTB | B_AX_FEN_BB_GLB_RSTN);

	ret = rtw89_mac_read_xtal_si(rtwdev, XTAL_SI_WL_RFC_S0, &wl_rfc_s0);
	if (ret)
		return ret;
	wl_rfc_s0 &= ~XTAL_SI_RF00S_EN;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_WL_RFC_S0, wl_rfc_s0,
				      FULL_BIT_MASK);
	if (ret)
		return ret;

	ret = rtw89_mac_read_xtal_si(rtwdev, XTAL_SI_WL_RFC_S1, &wl_rfc_s1);
	if (ret)
		return ret;
	wl_rfc_s1 &= ~XTAL_SI_RF10S_EN;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_WL_RFC_S1, wl_rfc_s1,
				      FULL_BIT_MASK);
	return ret;
}

static const struct rtw89_chip_ops rtw8852b_chip_ops = {
	.enable_bb_rf		= rtw8852b_mac_enable_bb_rf,
	.disable_bb_rf		= rtw8852b_mac_disable_bb_rf,
	.bb_reset		= rtw8852b_bb_reset,
	.bb_sethw		= rtw8852b_bb_sethw,
	.read_rf		= rtw89_phy_read_rf_v1,
	.write_rf		= rtw89_phy_write_rf_v1,
	.set_channel		= rtw8852b_set_channel,
	.set_channel_help	= rtw8852b_set_channel_help,
	.read_efuse		= rtw8852b_read_efuse,
	.read_phycap		= rtw8852b_read_phycap,
	.fem_setup		= NULL,
	.rfk_init		= rtw8852b_rfk_init,
	.rfk_channel		= rtw8852b_rfk_channel,
	.rfk_band_changed	= rtw8852b_rfk_band_changed,
	.rfk_scan		= rtw8852b_rfk_scan,
	.rfk_track		= rtw8852b_rfk_track,
	.power_trim		= rtw8852b_power_trim,
	.set_txpwr		= rtw8852b_set_txpwr,
	.set_txpwr_ctrl		= rtw8852b_set_txpwr_ctrl,
	.init_txpwr_unit	= rtw8852b_init_txpwr_unit,
	.get_thermal		= rtw8852b_get_thermal,
	.ctrl_btg		= rtw8852b_ctrl_btg,
	.query_ppdu		= rtw8852b_query_ppdu,
	.bb_ctrl_btc_preagc	= rtw8852b_bb_ctrl_btc_preagc,
	.cfg_txrx_path		= rtw8852b_bb_cfg_txrx_path,
	.set_txpwr_ul_tb_offset	= rtw8852b_set_txpwr_ul_tb_offset,
	.pwr_on_func		= rtw8852b_pwr_on_func,
	.pwr_off_func		= rtw8852b_pwr_off_func,
	.fill_txdesc		= rtw89_core_fill_txdesc,
	.fill_txdesc_fwcmd	= rtw89_core_fill_txdesc,
	.cfg_ctrl_path		= rtw89_mac_cfg_ctrl_path,
	.mac_cfg_gnt		= rtw89_mac_cfg_gnt,
	.stop_sch_tx		= rtw89_mac_stop_sch_tx,
	.resume_sch_tx		= rtw89_mac_resume_sch_tx,
	.h2c_dctl_sec_cam	= NULL,

	.btc_set_rfe		= rtw8852b_btc_set_rfe,
	.btc_init_cfg		= rtw8852b_btc_init_cfg,
	.btc_set_wl_pri		= rtw8852b_btc_set_wl_pri,
	.btc_set_wl_txpwr_ctrl	= rtw8852b_btc_set_wl_txpwr_ctrl,
	.btc_get_bt_rssi	= rtw8852b_btc_get_bt_rssi,
	.btc_update_bt_cnt	= rtw8852b_btc_update_bt_cnt,
	.btc_wl_s1_standby	= rtw8852b_btc_wl_s1_standby,
	.btc_set_wl_rx_gain	= rtw8852b_btc_set_wl_rx_gain,
	.btc_set_policy		= rtw89_btc_set_policy_v1,
};

const struct rtw89_chip_info rtw8852b_chip_info = {
	.chip_id		= RTL8852B,
	.ops			= &rtw8852b_chip_ops,
	.fw_basename		= RTW8852B_FW_BASENAME,
	.fw_format_max		= RTW8852B_FW_FORMAT_MAX,
	.try_ce_fw		= true,
	.fifo_size		= 196608,
	.dle_scc_rsvd_size	= 98304,
	.max_amsdu_limit	= 3500,
	.dis_2g_40m_ul_ofdma	= true,
	.rsvd_ple_ofst		= 0x2f800,
	.hfc_param_ini		= rtw8852b_hfc_param_ini_pcie,
	.dle_mem		= rtw8852b_dle_mem_pcie,
	.wde_qempty_acq_num	= 4,
	.wde_qempty_mgq_sel	= 4,
	.rf_base_addr		= {0xe000, 0xf000},
	.pwr_on_seq		= NULL,
	.pwr_off_seq		= NULL,
	.bb_table		= &rtw89_8852b_phy_bb_table,
	.bb_gain_table		= &rtw89_8852b_phy_bb_gain_table,
	.rf_table		= {&rtw89_8852b_phy_radioa_table,
				   &rtw89_8852b_phy_radiob_table,},
	.nctl_table		= &rtw89_8852b_phy_nctl_table,
	.byr_table		= &rtw89_8852b_byr_table,
	.dflt_parms		= &rtw89_8852b_dflt_parms,
	.rfe_parms_conf		= NULL,
	.txpwr_factor_rf	= 2,
	.txpwr_factor_mac	= 1,
	.dig_table		= NULL,
	.dig_regs		= &rtw8852b_dig_regs,
	.tssi_dbw_table		= NULL,
	.support_chanctx_num	= 0,
	.support_bands		= BIT(NL80211_BAND_2GHZ) |
				  BIT(NL80211_BAND_5GHZ),
	.support_bw160		= false,
	.support_ul_tb_ctrl	= true,
	.hw_sec_hdr		= false,
	.rf_path_num		= 2,
	.tx_nss			= 2,
	.rx_nss			= 2,
	.acam_num		= 128,
	.bcam_num		= 10,
	.scam_num		= 128,
	.bacam_num		= 2,
	.bacam_dynamic_num	= 4,
	.bacam_v1		= false,
	.sec_ctrl_efuse_size	= 4,
	.physical_efuse_size	= 1216,
	.logical_efuse_size	= 2048,
	.limit_efuse_size	= 1280,
	.dav_phy_efuse_size	= 96,
	.dav_log_efuse_size	= 16,
	.phycap_addr		= 0x580,
	.phycap_size		= 128,
	.para_ver		= 0,
	.wlcx_desired		= 0x05050000,
	.btcx_desired		= 0x5,
	.scbd			= 0x1,
	.mailbox		= 0x1,

	.afh_guard_ch		= 6,
	.wl_rssi_thres		= rtw89_btc_8852b_wl_rssi_thres,
	.bt_rssi_thres		= rtw89_btc_8852b_bt_rssi_thres,
	.rssi_tol		= 2,
	.mon_reg_num		= ARRAY_SIZE(rtw89_btc_8852b_mon_reg),
	.mon_reg		= rtw89_btc_8852b_mon_reg,
	.rf_para_ulink_num	= ARRAY_SIZE(rtw89_btc_8852b_rf_ul),
	.rf_para_ulink		= rtw89_btc_8852b_rf_ul,
	.rf_para_dlink_num	= ARRAY_SIZE(rtw89_btc_8852b_rf_dl),
	.rf_para_dlink		= rtw89_btc_8852b_rf_dl,
	.ps_mode_supported	= BIT(RTW89_PS_MODE_RFOFF) |
				  BIT(RTW89_PS_MODE_CLK_GATED) |
				  BIT(RTW89_PS_MODE_PWR_GATED),
	.low_power_hci_modes	= 0,
	.h2c_cctl_func_id	= H2C_FUNC_MAC_CCTLINFO_UD,
	.hci_func_en_addr	= R_AX_HCI_FUNC_EN,
	.h2c_desc_size		= sizeof(struct rtw89_txwd_body),
	.txwd_body_size		= sizeof(struct rtw89_txwd_body),
	.h2c_ctrl_reg		= R_AX_H2CREG_CTRL,
	.h2c_counter_reg	= {R_AX_UDM1 + 1, B_AX_UDM1_HALMAC_H2C_DEQ_CNT_MASK >> 8},
	.h2c_regs		= rtw8852b_h2c_regs,
	.c2h_ctrl_reg		= R_AX_C2HREG_CTRL,
	.c2h_counter_reg	= {R_AX_UDM1 + 1, B_AX_UDM1_HALMAC_C2H_ENQ_CNT_MASK >> 8},
	.c2h_regs		= rtw8852b_c2h_regs,
	.page_regs		= &rtw8852b_page_regs,
	.cfo_src_fd		= true,
	.cfo_hw_comp		= true,
	.dcfo_comp		= &rtw8852b_dcfo_comp,
	.dcfo_comp_sft		= 10,
	.imr_info		= &rtw8852b_imr_info,
	.rrsr_cfgs		= &rtw8852b_rrsr_cfgs,
	.bss_clr_map_reg	= R_BSS_CLR_MAP_V1,
	.dma_ch_mask		= BIT(RTW89_DMA_ACH4) | BIT(RTW89_DMA_ACH5) |
				  BIT(RTW89_DMA_ACH6) | BIT(RTW89_DMA_ACH7) |
				  BIT(RTW89_DMA_B1MG) | BIT(RTW89_DMA_B1HI),
	.edcca_lvl_reg		= R_SEG0R_EDCCA_LVL_V1,
};
EXPORT_SYMBOL(rtw8852b_chip_info);

MODULE_FIRMWARE(RTW8852B_MODULE_FIRMWARE);
MODULE_AUTHOR("Realtek Corporation");
MODULE_DESCRIPTION("Realtek 802.11ax wireless 8852B driver");
MODULE_LICENSE("Dual BSD/GPL");
