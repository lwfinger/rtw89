// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright(c) 2022-2023  Realtek Corporation
 */

#include "coex.h"
#include "fw.h"
#include "mac.h"
#include "phy.h"
#include "reg.h"
#include "rtw8851b.h"
#include "rtw8851b_table.h"
#include "txrx.h"
#include "util.h"

#define RTW8851B_FW_FORMAT_MAX 0
#define RTW8851B_FW_BASENAME "rtw89/rtw8851b_fw"
#define RTW8851B_MODULE_FIRMWARE \
	RTW8851B_FW_BASENAME ".bin"

static void rtw8851b_set_bb_gpio(struct rtw89_dev *rtwdev, u8 gpio_idx, bool inv,
				 u8 src_sel)
{
	u32 addr, mask;

	if (gpio_idx >= 32)
		return;

	/* 2 continual 32-bit registers for 32 GPIOs, and each GPIO occupies 2 bits */
	addr = R_RFE_SEL0_A2 + (gpio_idx / 16) * sizeof(u32);
	mask = B_RFE_SEL0_MASK << (gpio_idx % 16) * 2;

	rtw89_phy_write32_mask(rtwdev, addr, mask, RF_PATH_A);
	rtw89_phy_write32_mask(rtwdev, R_RFE_INV0, BIT(gpio_idx), inv);

	/* 4 continual 32-bit registers for 32 GPIOs, and each GPIO occupies 4 bits */
	addr = R_RFE_SEL0_BASE + (gpio_idx / 8) * sizeof(u32);
	mask = B_RFE_SEL0_SRC_MASK << (gpio_idx % 8) * 4;

	rtw89_phy_write32_mask(rtwdev, addr, mask, src_sel);
}

static void rtw8851b_set_mac_gpio(struct rtw89_dev *rtwdev, u8 func)
{
	static const struct rtw89_reg3_def func16 = {
		R_AX_GPIO16_23_FUNC_SEL, B_AX_PINMUX_GPIO16_FUNC_SEL_MASK, BIT(3)
	};
	static const struct rtw89_reg3_def func17 = {
		R_AX_GPIO16_23_FUNC_SEL, B_AX_PINMUX_GPIO17_FUNC_SEL_MASK, BIT(7) >> 4,
	};
	const struct rtw89_reg3_def *def;

	switch (func) {
	case 16:
		def = &func16;
		break;
	case 17:
		def = &func17;
		break;
	default:
		rtw89_warn(rtwdev, "undefined gpio func %d\n", func);
		return;
	}

	rtw89_write8_mask(rtwdev, def->addr, def->mask, def->data);
}

static void rtw8851b_rfe_gpio(struct rtw89_dev *rtwdev)
{
	u8 rfe_type = rtwdev->efuse.rfe_type;

	if (rfe_type > 50)
		return;

	if (rfe_type % 3 == 2) {
		rtw8851b_set_bb_gpio(rtwdev, 16, true, RFE_SEL0_SRC_ANTSEL_0);
		rtw8851b_set_bb_gpio(rtwdev, 17, false, RFE_SEL0_SRC_ANTSEL_0);

		rtw8851b_set_mac_gpio(rtwdev, 16);
		rtw8851b_set_mac_gpio(rtwdev, 17);
	}
}

static const struct rtw89_chip_ops rtw8851b_chip_ops = {
	.fem_setup		= NULL,
	.rfe_gpio		= rtw8851b_rfe_gpio,
	.fill_txdesc		= rtw89_core_fill_txdesc,
	.fill_txdesc_fwcmd	= rtw89_core_fill_txdesc,
	.h2c_dctl_sec_cam	= NULL,
};

const struct rtw89_chip_info rtw8851b_chip_info = {
	.chip_id		= RTL8851B,
	.ops			= &rtw8851b_chip_ops,
	.fw_basename		= RTW8851B_FW_BASENAME,
	.fw_format_max		= RTW8851B_FW_FORMAT_MAX,
	.try_ce_fw		= true,
	.fifo_size		= 196608,
	.dle_scc_rsvd_size	= 98304,
	.max_amsdu_limit	= 3500,
	.dis_2g_40m_ul_ofdma	= true,
	.rsvd_ple_ofst		= 0x2f800,
	.wde_qempty_acq_num     = 4,
	.wde_qempty_mgq_sel     = 4,
	.rf_base_addr		= {0xe000},
	.pwr_on_seq		= NULL,
	.pwr_off_seq		= NULL,
	.bb_table		= &rtw89_8851b_phy_bb_table,
	.bb_gain_table		= &rtw89_8851b_phy_bb_gain_table,
	.rf_table		= {&rtw89_8851b_phy_radioa_table,},
	.nctl_table		= &rtw89_8851b_phy_nctl_table,
	.byr_table		= &rtw89_8851b_byr_table,
	.dflt_parms		= &rtw89_8851b_dflt_parms,
	.rfe_parms_conf		= rtw89_8851b_rfe_parms_conf,
	.txpwr_factor_rf	= 2,
	.txpwr_factor_mac	= 1,
	.dig_table		= NULL,
	.tssi_dbw_table		= NULL,
	.support_chanctx_num	= 0,
	.support_bands		= BIT(NL80211_BAND_2GHZ) |
				  BIT(NL80211_BAND_5GHZ),
	.support_bw160		= false,
	.support_unii4		= true,
	.support_ul_tb_ctrl	= true,
	.hw_sec_hdr		= false,
	.rf_path_num		= 1,
	.tx_nss			= 1,
	.rx_nss			= 1,
	.acam_num		= 32,
	.bcam_num		= 20,
	.scam_num		= 128,
	.bacam_num		= 2,
	.bacam_dynamic_num	= 4,
	.bacam_v1		= false,
	.sec_ctrl_efuse_size	= 4,
	.physical_efuse_size	= 1216,
	.logical_efuse_size	= 2048,
	.limit_efuse_size	= 1280,
	.dav_phy_efuse_size	= 0,
	.dav_log_efuse_size	= 0,
	.phycap_addr		= 0x580,
	.phycap_size		= 128,
	.para_ver		= 0,
	.wlcx_desired		= 0x06000000,
	.btcx_desired		= 0x7,
	.scbd			= 0x1,
	.mailbox		= 0x1,

	.ps_mode_supported	= BIT(RTW89_PS_MODE_RFOFF) |
				  BIT(RTW89_PS_MODE_CLK_GATED),
	.low_power_hci_modes	= 0,
	.h2c_cctl_func_id	= H2C_FUNC_MAC_CCTLINFO_UD,
	.hci_func_en_addr	= R_AX_HCI_FUNC_EN,
	.h2c_desc_size		= sizeof(struct rtw89_txwd_body),
	.txwd_body_size		= sizeof(struct rtw89_txwd_body),
	.bss_clr_map_reg	= R_BSS_CLR_MAP_V1,
	.dma_ch_mask		= BIT(RTW89_DMA_ACH4) | BIT(RTW89_DMA_ACH5) |
				  BIT(RTW89_DMA_ACH6) | BIT(RTW89_DMA_ACH7) |
				  BIT(RTW89_DMA_B1MG) | BIT(RTW89_DMA_B1HI),
	.edcca_lvl_reg		= R_SEG0R_EDCCA_LVL_V1,
};
EXPORT_SYMBOL(rtw8851b_chip_info);

MODULE_FIRMWARE(RTW8851B_MODULE_FIRMWARE);
MODULE_AUTHOR("Realtek Corporation");
MODULE_DESCRIPTION("Realtek 802.11ax wireless 8851B driver");
MODULE_LICENSE("Dual BSD/GPL");
