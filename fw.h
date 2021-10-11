/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright(c) 2019-2020  Realtek Corporation
 */

#ifndef __RTW89_FW_H__
#define __RTW89_FW_H__

#include "core.h"

enum rtw89_fw_dl_status {
	RTW89_FWDL_INITIAL_STATE = 0,
	RTW89_FWDL_FWDL_ONGOING = 1,
	RTW89_FWDL_CHECKSUM_FAIL = 2,
	RTW89_FWDL_SECURITY_FAIL = 3,
	RTW89_FWDL_CV_NOT_MATCH = 4,
	RTW89_FWDL_RSVD0 = 5,
	RTW89_FWDL_WCPU_FWDL_RDY = 6,
	RTW89_FWDL_WCPU_FW_INIT_RDY = 7
};

#define RTW89_GET_C2H_HDR_FUNC(info) \
	u32_get_bits(info, GENMASK(6, 0))
#define RTW89_GET_C2H_HDR_LEN(info) \
	u32_get_bits(info, GENMASK(11, 8))

#define RTW89_SET_H2CREG_HDR_FUNC(info, val) \
	u32p_replace_bits(info, val, GENMASK(6, 0))
#define RTW89_SET_H2CREG_HDR_LEN(info, val) \
	u32p_replace_bits(info, val, GENMASK(11, 8))

#define RTW89_H2CREG_MAX 4
#define RTW89_C2HREG_MAX 4
#define RTW89_C2HREG_HDR_LEN 2
#define RTW89_H2CREG_HDR_LEN 2
#define RTW89_C2H_TIMEOUT 1000000
struct rtw89_mac_c2h_info {
	u8 id;
	u8 content_len;
	u32 c2hreg[RTW89_C2HREG_MAX];
};

struct rtw89_mac_h2c_info {
	u8 id;
	u8 content_len;
	u32 h2creg[RTW89_H2CREG_MAX];
};

enum rtw89_mac_h2c_type {
	RTW89_FWCMD_H2CREG_FUNC_H2CREG_LB = 0,
	RTW89_FWCMD_H2CREG_FUNC_CNSL_CMD,
	RTW89_FWCMD_H2CREG_FUNC_FWERR,
	RTW89_FWCMD_H2CREG_FUNC_GET_FEATURE,
	RTW89_FWCMD_H2CREG_FUNC_GETPKT_INFORM,
	RTW89_FWCMD_H2CREG_FUNC_SCH_TX_EN
};

enum rtw89_mac_c2h_type {
	RTW89_FWCMD_C2HREG_FUNC_C2HREG_LB = 0,
	RTW89_FWCMD_C2HREG_FUNC_ERR_RPT,
	RTW89_FWCMD_C2HREG_FUNC_ERR_MSG,
	RTW89_FWCMD_C2HREG_FUNC_PHY_CAP,
	RTW89_FWCMD_C2HREG_FUNC_TX_PAUSE_RPT,
	RTW89_FWCMD_C2HREG_FUNC_NULL = 0xFF
};

struct rtw89_c2h_phy_cap {
	u32 func:7;
	u32 ack:1;
	u32 len:4;
	u32 seq:4;
	u32 rx_nss:8;
	u32 bw:8;

	u32 tx_nss:8;
	u32 prot:8;
	u32 nic:8;
	u32 wl_func:8;

	u32 hw_type:8;
} __packed;

enum rtw89_fw_c2h_category {
	RTW89_C2H_CAT_TEST,
	RTW89_C2H_CAT_MAC,
	RTW89_C2H_CAT_OUTSRC,
};

enum rtw89_fw_log_level {
	RTW89_FW_LOG_LEVEL_OFF,
	RTW89_FW_LOG_LEVEL_CRT,
	RTW89_FW_LOG_LEVEL_SER,
	RTW89_FW_LOG_LEVEL_WARN,
	RTW89_FW_LOG_LEVEL_LOUD,
	RTW89_FW_LOG_LEVEL_TR,
};

enum rtw89_fw_log_path {
	RTW89_FW_LOG_LEVEL_UART,
	RTW89_FW_LOG_LEVEL_C2H,
	RTW89_FW_LOG_LEVEL_SNI,
};

enum rtw89_fw_log_comp {
	RTW89_FW_LOG_COMP_VER,
	RTW89_FW_LOG_COMP_INIT,
	RTW89_FW_LOG_COMP_TASK,
	RTW89_FW_LOG_COMP_CNS,
	RTW89_FW_LOG_COMP_H2C,
	RTW89_FW_LOG_COMP_C2H,
	RTW89_FW_LOG_COMP_TX,
	RTW89_FW_LOG_COMP_RX,
	RTW89_FW_LOG_COMP_IPSEC,
	RTW89_FW_LOG_COMP_TIMER,
	RTW89_FW_LOG_COMP_DBGPKT,
	RTW89_FW_LOG_COMP_PS,
	RTW89_FW_LOG_COMP_ERROR,
	RTW89_FW_LOG_COMP_WOWLAN,
	RTW89_FW_LOG_COMP_SECURE_BOOT,
	RTW89_FW_LOG_COMP_BTC,
	RTW89_FW_LOG_COMP_BB,
	RTW89_FW_LOG_COMP_TWT,
	RTW89_FW_LOG_COMP_RF,
	RTW89_FW_LOG_COMP_MCC = 20,
};

#define FWDL_SECTION_MAX_NUM 10
#define FWDL_SECTION_CHKSUM_LEN	8
#define FWDL_SECTION_PER_PKT_LEN 2020

struct rtw89_fw_hdr_section_info {
	u8 redl;
	const u8 *addr;
	u32 len;
	u32 dladdr;
};

struct rtw89_fw_bin_info {
	u8 section_num;
	u32 hdr_len;
	struct rtw89_fw_hdr_section_info section_info[FWDL_SECTION_MAX_NUM];
};

struct rtw89_fw_macid_pause_grp {
	__le32 pause_grp[4];
	__le32 mask_grp[4];
} __packed;

struct rtw89_h2creg_sch_tx_en {
	u8 func:7;
	u8 ack:1;
	u8 total_len:4;
	u8 seq_num:4;
	u16 tx_en:16;
	u16 mask:16;
	u8 band:1;
	u16 rsvd:15;
} __packed;

#define RTW89_SET_FWCMD_RA_IS_DIS(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, BIT(0))
#define RTW89_SET_FWCMD_RA_MODE(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, GENMASK(5, 1))
#define RTW89_SET_FWCMD_RA_BW_CAP(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, GENMASK(7, 6))
#define RTW89_SET_FWCMD_RA_MACID(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, GENMASK(15, 8))
#define RTW89_SET_FWCMD_RA_DCM(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, BIT(16))
#define RTW89_SET_FWCMD_RA_ER(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, BIT(17))
#define RTW89_SET_FWCMD_RA_INIT_RATE_LV(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, GENMASK(19, 18))
#define RTW89_SET_FWCMD_RA_UPD_ALL(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, BIT(20))
#define RTW89_SET_FWCMD_RA_SGI(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, BIT(21))
#define RTW89_SET_FWCMD_RA_LDPC(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, BIT(22))
#define RTW89_SET_FWCMD_RA_STBC(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, BIT(23))
#define RTW89_SET_FWCMD_RA_SS_NUM(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, GENMASK(26, 24))
#define RTW89_SET_FWCMD_RA_GILTF(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, GENMASK(29, 27))
#define RTW89_SET_FWCMD_RA_UPD_BW_NSS_MASK(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, BIT(30))
#define RTW89_SET_FWCMD_RA_UPD_MASK(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, BIT(31))
#define RTW89_SET_FWCMD_RA_MASK_0(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x01, val, GENMASK(7, 0))
#define RTW89_SET_FWCMD_RA_MASK_1(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x01, val, GENMASK(15, 8))
#define RTW89_SET_FWCMD_RA_MASK_2(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x01, val, GENMASK(23, 16))
#define RTW89_SET_FWCMD_RA_MASK_3(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x01, val, GENMASK(31, 24))
#define RTW89_SET_FWCMD_RA_MASK_4(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x02, val, GENMASK(7, 0))
#define RTW89_SET_FWCMD_RA_BFEE_CSI_CTL(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x02, val, BIT(31))
#define RTW89_SET_FWCMD_RA_BAND_NUM(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x03, val, GENMASK(7, 0))
#define RTW89_SET_FWCMD_RA_RA_CSI_RATE_EN(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x03, val, BIT(8))
#define RTW89_SET_FWCMD_RA_FIXED_CSI_RATE_EN(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x03, val, BIT(9))
#define RTW89_SET_FWCMD_RA_CR_TBL_SEL(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x03, val, BIT(10))
#define RTW89_SET_FWCMD_RA_FIXED_CSI_MCS_SS_IDX(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x03, val, GENMASK(23, 16))
#define RTW89_SET_FWCMD_RA_FIXED_CSI_MODE(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x03, val, GENMASK(25, 24))
#define RTW89_SET_FWCMD_RA_FIXED_CSI_GI_LTF(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x03, val, GENMASK(28, 26))
#define RTW89_SET_FWCMD_RA_FIXED_CSI_BW(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x03, val, GENMASK(31, 29))

#define RTW89_SET_FWCMD_SEC_IDX(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, GENMASK(7, 0))
#define RTW89_SET_FWCMD_SEC_OFFSET(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, GENMASK(15, 8))
#define RTW89_SET_FWCMD_SEC_LEN(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, GENMASK(23, 16))
#define RTW89_SET_FWCMD_SEC_TYPE(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x01, val, GENMASK(3, 0))
#define RTW89_SET_FWCMD_SEC_EXT_KEY(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x01, val, BIT(4))
#define RTW89_SET_FWCMD_SEC_SPP_MODE(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x01, val, BIT(5))
#define RTW89_SET_FWCMD_SEC_KEY0(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x02, val, GENMASK(31, 0))
#define RTW89_SET_FWCMD_SEC_KEY1(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x03, val, GENMASK(31, 0))
#define RTW89_SET_FWCMD_SEC_KEY2(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x04, val, GENMASK(31, 0))
#define RTW89_SET_FWCMD_SEC_KEY3(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x05, val, GENMASK(31, 0))

#define RTW89_SET_EDCA_SEL(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, GENMASK(1, 0))
#define RTW89_SET_EDCA_BAND(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, BIT(3))
#define RTW89_SET_EDCA_WMM(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, BIT(4))
#define RTW89_SET_EDCA_AC(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x00, val, GENMASK(6, 5))
#define RTW89_SET_EDCA_PARAM(cmd, val) \
	le32p_replace_bits((__le32 *)(cmd) + 0x01, val, GENMASK(31, 0))
#define FW_EDCA_PARAM_TXOPLMT_MSK GENMASK(26, 16)
#define FW_EDCA_PARAM_CWMAX_MSK GENMASK(15, 12)
#define FW_EDCA_PARAM_CWMIN_MSK GENMASK(11, 8)
#define FW_EDCA_PARAM_AIFS_MSK GENMASK(7, 0)

#define GET_FWSECTION_HDR_SEC_SIZE(fwhdr)	\
	le32_get_bits(*((__le32 *)(fwhdr) + 1), GENMASK(23, 0))
#define GET_FWSECTION_HDR_CHECKSUM(fwhdr)	\
	le32_get_bits(*((__le32 *)(fwhdr) + 1), BIT(28))
#define GET_FWSECTION_HDR_REDL(fwhdr)	\
	le32_get_bits(*((__le32 *)(fwhdr) + 1), BIT(29))
#define GET_FWSECTION_HDR_DL_ADDR(fwhdr)	\
	le32_get_bits(*((__le32 *)(fwhdr)), GENMASK(31, 0))

#define GET_FW_HDR_MAJOR_VERSION(fwhdr)	\
	le32_get_bits(*((__le32 *)(fwhdr) + 1), GENMASK(7, 0))
#define GET_FW_HDR_MINOR_VERSION(fwhdr)	\
	le32_get_bits(*((__le32 *)(fwhdr) + 1), GENMASK(15, 8))
#define GET_FW_HDR_SUBVERSION(fwhdr)	\
	le32_get_bits(*((__le32 *)(fwhdr) + 1), GENMASK(23, 16))
#define GET_FW_HDR_SUBINDEX(fwhdr)	\
	le32_get_bits(*((__le32 *)(fwhdr) + 1), GENMASK(31, 24))
#define GET_FW_HDR_MONTH(fwhdr)		\
	le32_get_bits(*((__le32 *)(fwhdr) + 4), GENMASK(7, 0))
#define GET_FW_HDR_DATE(fwhdr)		\
	le32_get_bits(*((__le32 *)(fwhdr) + 4), GENMASK(15, 8))
#define GET_FW_HDR_HOUR(fwhdr)		\
	le32_get_bits(*((__le32 *)(fwhdr) + 4), GENMASK(23, 16))
#define GET_FW_HDR_MIN(fwhdr)		\
	le32_get_bits(*((__le32 *)(fwhdr) + 4), GENMASK(31, 24))
#define GET_FW_HDR_YEAR(fwhdr)		\
	le32_get_bits(*((__le32 *)(fwhdr) + 5), GENMASK(31, 0))
#define GET_FW_HDR_SEC_NUM(fwhdr)	\
	le32_get_bits(*((__le32 *)(fwhdr) + 6), GENMASK(15, 8))
#define GET_FW_HDR_CMD_VERSERION(fwhdr)	\
	le32_get_bits(*((__le32 *)(fwhdr) + 7), GENMASK(31, 24))
#define SET_FW_HDR_PART_SIZE(fwhdr, val)	\
	le32p_replace_bits((__le32 *)(fwhdr) + 7, val, GENMASK(15, 0))

#define SET_CTRL_INFO_MACID(table, val) \
	le32p_replace_bits((__le32 *)(table) + 0, val, GENMASK(6, 0))
#define SET_CTRL_INFO_OPERATION(table, val) \
	le32p_replace_bits((__le32 *)(table) + 0, val, BIT(7))
#define SET_CMC_TBL_MASK_DATARATE GENMASK(8, 0)
#define SET_CMC_TBL_DATARATE(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 1, val, GENMASK(8, 0)); \
	le32p_replace_bits((__le32 *)(table) + 9, SET_CMC_TBL_MASK_DATARATE, \
			   GENMASK(8, 0)); \
} while (0)
#define SET_CMC_TBL_MASK_FORCE_TXOP BIT(0)
#define SET_CMC_TBL_FORCE_TXOP(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 1, val, BIT(9)); \
	le32p_replace_bits((__le32 *)(table) + 9, SET_CMC_TBL_MASK_FORCE_TXOP, \
			   BIT(9)); \
} while (0)
#define SET_CMC_TBL_MASK_DATA_BW GENMASK(1, 0)
#define SET_CMC_TBL_DATA_BW(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 1, val, GENMASK(11, 10)); \
	le32p_replace_bits((__le32 *)(table) + 9, SET_CMC_TBL_MASK_DATA_BW, \
			   GENMASK(11, 10)); \
} while (0)
#define SET_CMC_TBL_MASK_DATA_GI_LTF GENMASK(2, 0)
#define SET_CMC_TBL_DATA_GI_LTF(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 1, val, GENMASK(14, 12)); \
	le32p_replace_bits((__le32 *)(table) + 9, SET_CMC_TBL_MASK_DATA_GI_LTF, \
			   GENMASK(14, 12)); \
} while (0)
#define SET_CMC_TBL_MASK_DARF_TC_INDEX BIT(0)
#define SET_CMC_TBL_DARF_TC_INDEX(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 1, val, BIT(15)); \
	le32p_replace_bits((__le32 *)(table) + 9, SET_CMC_TBL_MASK_DARF_TC_INDEX, \
			   BIT(15)); \
} while (0)
#define SET_CMC_TBL_MASK_ARFR_CTRL GENMASK(3, 0)
#define SET_CMC_TBL_ARFR_CTRL(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 1, val, GENMASK(19, 16)); \
	le32p_replace_bits((__le32 *)(table) + 9, SET_CMC_TBL_MASK_ARFR_CTRL, \
			   GENMASK(19, 16)); \
} while (0)
#define SET_CMC_TBL_MASK_ACQ_RPT_EN BIT(0)
#define SET_CMC_TBL_ACQ_RPT_EN(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 1, val, BIT(20)); \
	le32p_replace_bits((__le32 *)(table) + 9, SET_CMC_TBL_MASK_ACQ_RPT_EN, \
			   BIT(20)); \
} while (0)
#define SET_CMC_TBL_MASK_MGQ_RPT_EN BIT(0)
#define SET_CMC_TBL_MGQ_RPT_EN(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 1, val, BIT(21)); \
	le32p_replace_bits((__le32 *)(table) + 9, SET_CMC_TBL_MASK_MGQ_RPT_EN, \
			   BIT(21)); \
} while (0)
#define SET_CMC_TBL_MASK_ULQ_RPT_EN BIT(0)
#define SET_CMC_TBL_ULQ_RPT_EN(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 1, val, BIT(22)); \
	le32p_replace_bits((__le32 *)(table) + 9, SET_CMC_TBL_MASK_ULQ_RPT_EN, \
			   BIT(22)); \
} while (0)
#define SET_CMC_TBL_MASK_TWTQ_RPT_EN BIT(0)
#define SET_CMC_TBL_TWTQ_RPT_EN(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 1, val, BIT(23)); \
	le32p_replace_bits((__le32 *)(table) + 9, SET_CMC_TBL_MASK_TWTQ_RPT_EN, \
			   BIT(23)); \
} while (0)
#define SET_CMC_TBL_MASK_DISRTSFB BIT(0)
#define SET_CMC_TBL_DISRTSFB(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 1, val, BIT(25)); \
	le32p_replace_bits((__le32 *)(table) + 9, SET_CMC_TBL_MASK_DISRTSFB, \
			   BIT(25)); \
} while (0)
#define SET_CMC_TBL_MASK_DISDATAFB BIT(0)
#define SET_CMC_TBL_DISDATAFB(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 1, val, BIT(26)); \
	le32p_replace_bits((__le32 *)(table) + 9, SET_CMC_TBL_MASK_DISDATAFB, \
			   BIT(26)); \
} while (0)
#define SET_CMC_TBL_MASK_TRYRATE BIT(0)
#define SET_CMC_TBL_TRYRATE(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 1, val, BIT(27)); \
	le32p_replace_bits((__le32 *)(table) + 9, SET_CMC_TBL_MASK_TRYRATE, \
			   BIT(27)); \
} while (0)
#define SET_CMC_TBL_MASK_AMPDU_DENSITY GENMASK(3, 0)
#define SET_CMC_TBL_AMPDU_DENSITY(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 1, val, GENMASK(31, 28)); \
	le32p_replace_bits((__le32 *)(table) + 9, SET_CMC_TBL_MASK_AMPDU_DENSITY, \
			   GENMASK(31, 28)); \
} while (0)
#define SET_CMC_TBL_MASK_DATA_RTY_LOWEST_RATE GENMASK(8, 0)
#define SET_CMC_TBL_DATA_RTY_LOWEST_RATE(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 2, val, GENMASK(8, 0)); \
	le32p_replace_bits((__le32 *)(table) + 10, SET_CMC_TBL_MASK_DATA_RTY_LOWEST_RATE, \
			   GENMASK(8, 0)); \
} while (0)
#define SET_CMC_TBL_MASK_AMPDU_TIME_SEL BIT(0)
#define SET_CMC_TBL_AMPDU_TIME_SEL(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 2, val, BIT(9)); \
	le32p_replace_bits((__le32 *)(table) + 10, SET_CMC_TBL_MASK_AMPDU_TIME_SEL, \
			   BIT(9)); \
} while (0)
#define SET_CMC_TBL_MASK_AMPDU_LEN_SEL BIT(0)
#define SET_CMC_TBL_AMPDU_LEN_SEL(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 2, val, BIT(10)); \
	le32p_replace_bits((__le32 *)(table) + 10, SET_CMC_TBL_MASK_AMPDU_LEN_SEL, \
			   BIT(10)); \
} while (0)
#define SET_CMC_TBL_MASK_RTS_TXCNT_LMT_SEL BIT(0)
#define SET_CMC_TBL_RTS_TXCNT_LMT_SEL(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 2, val, BIT(11)); \
	le32p_replace_bits((__le32 *)(table) + 10, SET_CMC_TBL_MASK_RTS_TXCNT_LMT_SEL, \
			   BIT(11)); \
} while (0)
#define SET_CMC_TBL_MASK_RTS_TXCNT_LMT GENMASK(3, 0)
#define SET_CMC_TBL_RTS_TXCNT_LMT(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 2, val, GENMASK(15, 12)); \
	le32p_replace_bits((__le32 *)(table) + 10, SET_CMC_TBL_MASK_RTS_TXCNT_LMT, \
			   GENMASK(15, 12)); \
} while (0)
#define SET_CMC_TBL_MASK_RTSRATE GENMASK(8, 0)
#define SET_CMC_TBL_RTSRATE(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 2, val, GENMASK(24, 16)); \
	le32p_replace_bits((__le32 *)(table) + 10, SET_CMC_TBL_MASK_RTSRATE, \
			   GENMASK(24, 16)); \
} while (0)
#define SET_CMC_TBL_MASK_VCS_STBC BIT(0)
#define SET_CMC_TBL_VCS_STBC(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 2, val, BIT(27)); \
	le32p_replace_bits((__le32 *)(table) + 10, SET_CMC_TBL_MASK_VCS_STBC, \
			   BIT(27)); \
} while (0)
#define SET_CMC_TBL_MASK_RTS_RTY_LOWEST_RATE GENMASK(3, 0)
#define SET_CMC_TBL_RTS_RTY_LOWEST_RATE(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 2, val, GENMASK(31, 28)); \
	le32p_replace_bits((__le32 *)(table) + 10, SET_CMC_TBL_MASK_RTS_RTY_LOWEST_RATE, \
			   GENMASK(31, 28)); \
} while (0)
#define SET_CMC_TBL_MASK_DATA_TX_CNT_LMT GENMASK(5, 0)
#define SET_CMC_TBL_DATA_TX_CNT_LMT(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 3, val, GENMASK(5, 0)); \
	le32p_replace_bits((__le32 *)(table) + 11, SET_CMC_TBL_MASK_DATA_TX_CNT_LMT, \
			   GENMASK(5, 0)); \
} while (0)
#define SET_CMC_TBL_MASK_DATA_TXCNT_LMT_SEL BIT(0)
#define SET_CMC_TBL_DATA_TXCNT_LMT_SEL(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 3, val, BIT(6)); \
	le32p_replace_bits((__le32 *)(table) + 11, SET_CMC_TBL_MASK_DATA_TXCNT_LMT_SEL, \
			   BIT(6)); \
} while (0)
#define SET_CMC_TBL_MASK_MAX_AGG_NUM_SEL BIT(0)
#define SET_CMC_TBL_MAX_AGG_NUM_SEL(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 3, val, BIT(7)); \
	le32p_replace_bits((__le32 *)(table) + 11, SET_CMC_TBL_MASK_MAX_AGG_NUM_SEL, \
			   BIT(7)); \
} while (0)
#define SET_CMC_TBL_MASK_RTS_EN BIT(0)
#define SET_CMC_TBL_RTS_EN(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 3, val, BIT(8)); \
	le32p_replace_bits((__le32 *)(table) + 11, SET_CMC_TBL_MASK_RTS_EN, \
			   BIT(8)); \
} while (0)
#define SET_CMC_TBL_MASK_CTS2SELF_EN BIT(0)
#define SET_CMC_TBL_CTS2SELF_EN(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 3, val, BIT(9)); \
	le32p_replace_bits((__le32 *)(table) + 11, SET_CMC_TBL_MASK_CTS2SELF_EN, \
			   BIT(9)); \
} while (0)
#define SET_CMC_TBL_MASK_CCA_RTS GENMASK(1, 0)
#define SET_CMC_TBL_CCA_RTS(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 3, val, GENMASK(11, 10)); \
	le32p_replace_bits((__le32 *)(table) + 11, SET_CMC_TBL_MASK_CCA_RTS, \
			   GENMASK(11, 10)); \
} while (0)
#define SET_CMC_TBL_MASK_HW_RTS_EN BIT(0)
#define SET_CMC_TBL_HW_RTS_EN(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 3, val, BIT(12)); \
	le32p_replace_bits((__le32 *)(table) + 11, SET_CMC_TBL_MASK_HW_RTS_EN, \
			   BIT(12)); \
} while (0)
#define SET_CMC_TBL_MASK_RTS_DROP_DATA_MODE GENMASK(1, 0)
#define SET_CMC_TBL_RTS_DROP_DATA_MODE(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 3, val, GENMASK(14, 13)); \
	le32p_replace_bits((__le32 *)(table) + 11, SET_CMC_TBL_MASK_RTS_DROP_DATA_MODE, \
			   GENMASK(14, 13)); \
} while (0)
#define SET_CMC_TBL_MASK_AMPDU_MAX_LEN GENMASK(10, 0)
#define SET_CMC_TBL_AMPDU_MAX_LEN(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 3, val, GENMASK(26, 16)); \
	le32p_replace_bits((__le32 *)(table) + 11, SET_CMC_TBL_MASK_AMPDU_MAX_LEN, \
			   GENMASK(26, 16)); \
} while (0)
#define SET_CMC_TBL_MASK_UL_MU_DIS BIT(0)
#define SET_CMC_TBL_UL_MU_DIS(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 3, val, BIT(27)); \
	le32p_replace_bits((__le32 *)(table) + 11, SET_CMC_TBL_MASK_UL_MU_DIS, \
			   BIT(27)); \
} while (0)
#define SET_CMC_TBL_MASK_AMPDU_MAX_TIME GENMASK(3, 0)
#define SET_CMC_TBL_AMPDU_MAX_TIME(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 3, val, GENMASK(31, 28)); \
	le32p_replace_bits((__le32 *)(table) + 11, SET_CMC_TBL_MASK_AMPDU_MAX_TIME, \
			   GENMASK(31, 28)); \
} while (0)
#define SET_CMC_TBL_MASK_MAX_AGG_NUM GENMASK(7, 0)
#define SET_CMC_TBL_MAX_AGG_NUM(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 4, val, GENMASK(7, 0)); \
	le32p_replace_bits((__le32 *)(table) + 12, SET_CMC_TBL_MASK_MAX_AGG_NUM, \
			   GENMASK(7, 0)); \
} while (0)
#define SET_CMC_TBL_MASK_BA_BMAP GENMASK(1, 0)
#define SET_CMC_TBL_BA_BMAP(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 4, val, GENMASK(9, 8)); \
	le32p_replace_bits((__le32 *)(table) + 12, SET_CMC_TBL_MASK_BA_BMAP, \
			   GENMASK(9, 8)); \
} while (0)
#define SET_CMC_TBL_MASK_VO_LFTIME_SEL GENMASK(2, 0)
#define SET_CMC_TBL_VO_LFTIME_SEL(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 4, val, GENMASK(18, 16)); \
	le32p_replace_bits((__le32 *)(table) + 12, SET_CMC_TBL_MASK_VO_LFTIME_SEL, \
			   GENMASK(18, 16)); \
} while (0)
#define SET_CMC_TBL_MASK_VI_LFTIME_SEL GENMASK(2, 0)
#define SET_CMC_TBL_VI_LFTIME_SEL(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 4, val, GENMASK(21, 19)); \
	le32p_replace_bits((__le32 *)(table) + 12, SET_CMC_TBL_MASK_VI_LFTIME_SEL, \
			   GENMASK(21, 19)); \
} while (0)
#define SET_CMC_TBL_MASK_BE_LFTIME_SEL GENMASK(2, 0)
#define SET_CMC_TBL_BE_LFTIME_SEL(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 4, val, GENMASK(24, 22)); \
	le32p_replace_bits((__le32 *)(table) + 12, SET_CMC_TBL_MASK_BE_LFTIME_SEL, \
			   GENMASK(24, 22)); \
} while (0)
#define SET_CMC_TBL_MASK_BK_LFTIME_SEL GENMASK(2, 0)
#define SET_CMC_TBL_BK_LFTIME_SEL(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 4, val, GENMASK(27, 25)); \
	le32p_replace_bits((__le32 *)(table) + 12, SET_CMC_TBL_MASK_BK_LFTIME_SEL, \
			   GENMASK(27, 25)); \
} while (0)
#define SET_CMC_TBL_MASK_SECTYPE GENMASK(3, 0)
#define SET_CMC_TBL_SECTYPE(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 4, val, GENMASK(31, 28)); \
	le32p_replace_bits((__le32 *)(table) + 12, SET_CMC_TBL_MASK_SECTYPE, \
			   GENMASK(31, 28)); \
} while (0)
#define SET_CMC_TBL_MASK_MULTI_PORT_ID GENMASK(2, 0)
#define SET_CMC_TBL_MULTI_PORT_ID(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 5, val, GENMASK(2, 0)); \
	le32p_replace_bits((__le32 *)(table) + 13, SET_CMC_TBL_MASK_MULTI_PORT_ID, \
			   GENMASK(2, 0)); \
} while (0)
#define SET_CMC_TBL_MASK_BMC BIT(0)
#define SET_CMC_TBL_BMC(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 5, val, BIT(3)); \
	le32p_replace_bits((__le32 *)(table) + 13, SET_CMC_TBL_MASK_BMC, \
			   BIT(3)); \
} while (0)
#define SET_CMC_TBL_MASK_MBSSID GENMASK(3, 0)
#define SET_CMC_TBL_MBSSID(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 5, val, GENMASK(7, 4)); \
	le32p_replace_bits((__le32 *)(table) + 13, SET_CMC_TBL_MASK_MBSSID, \
			   GENMASK(7, 4)); \
} while (0)
#define SET_CMC_TBL_MASK_NAVUSEHDR BIT(0)
#define SET_CMC_TBL_NAVUSEHDR(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 5, val, BIT(8)); \
	le32p_replace_bits((__le32 *)(table) + 13, SET_CMC_TBL_MASK_NAVUSEHDR, \
			   BIT(8)); \
} while (0)
#define SET_CMC_TBL_MASK_TXPWR_MODE GENMASK(2, 0)
#define SET_CMC_TBL_TXPWR_MODE(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 5, val, GENMASK(11, 9)); \
	le32p_replace_bits((__le32 *)(table) + 13, SET_CMC_TBL_MASK_TXPWR_MODE, \
			   GENMASK(11, 9)); \
} while (0)
#define SET_CMC_TBL_MASK_DATA_DCM BIT(0)
#define SET_CMC_TBL_DATA_DCM(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 5, val, BIT(12)); \
	le32p_replace_bits((__le32 *)(table) + 13, SET_CMC_TBL_MASK_DATA_DCM, \
			   BIT(12)); \
} while (0)
#define SET_CMC_TBL_MASK_DATA_ER BIT(0)
#define SET_CMC_TBL_DATA_ER(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 5, val, BIT(13)); \
	le32p_replace_bits((__le32 *)(table) + 13, SET_CMC_TBL_MASK_DATA_ER, \
			   BIT(13)); \
} while (0)
#define SET_CMC_TBL_MASK_DATA_LDPC BIT(0)
#define SET_CMC_TBL_DATA_LDPC(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 5, val, BIT(14)); \
	le32p_replace_bits((__le32 *)(table) + 13, SET_CMC_TBL_MASK_DATA_LDPC, \
			   BIT(14)); \
} while (0)
#define SET_CMC_TBL_MASK_DATA_STBC BIT(0)
#define SET_CMC_TBL_DATA_STBC(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 5, val, BIT(15)); \
	le32p_replace_bits((__le32 *)(table) + 13, SET_CMC_TBL_MASK_DATA_STBC, \
			   BIT(15)); \
} while (0)
#define SET_CMC_TBL_MASK_A_CTRL_BQR BIT(0)
#define SET_CMC_TBL_A_CTRL_BQR(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 5, val, BIT(16)); \
	le32p_replace_bits((__le32 *)(table) + 13, SET_CMC_TBL_MASK_A_CTRL_BQR, \
			   BIT(16)); \
} while (0)
#define SET_CMC_TBL_MASK_A_CTRL_UPH BIT(0)
#define SET_CMC_TBL_A_CTRL_UPH(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 5, val, BIT(17)); \
	le32p_replace_bits((__le32 *)(table) + 13, SET_CMC_TBL_MASK_A_CTRL_UPH, \
			   BIT(17)); \
} while (0)
#define SET_CMC_TBL_MASK_A_CTRL_BSR BIT(0)
#define SET_CMC_TBL_A_CTRL_BSR(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 5, val, BIT(18)); \
	le32p_replace_bits((__le32 *)(table) + 13, SET_CMC_TBL_MASK_A_CTRL_BSR, \
			   BIT(18)); \
} while (0)
#define SET_CMC_TBL_MASK_A_CTRL_CAS BIT(0)
#define SET_CMC_TBL_A_CTRL_CAS(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 5, val, BIT(19)); \
	le32p_replace_bits((__le32 *)(table) + 13, SET_CMC_TBL_MASK_A_CTRL_CAS, \
			   BIT(19)); \
} while (0)
#define SET_CMC_TBL_MASK_DATA_BW_ER BIT(0)
#define SET_CMC_TBL_DATA_BW_ER(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 5, val, BIT(20)); \
	le32p_replace_bits((__le32 *)(table) + 13, SET_CMC_TBL_MASK_DATA_BW_ER, \
			   BIT(20)); \
} while (0)
#define SET_CMC_TBL_MASK_LSIG_TXOP_EN BIT(0)
#define SET_CMC_TBL_LSIG_TXOP_EN(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 5, val, BIT(21)); \
	le32p_replace_bits((__le32 *)(table) + 13, SET_CMC_TBL_MASK_LSIG_TXOP_EN, \
			   BIT(21)); \
} while (0)
#define SET_CMC_TBL_MASK_CTRL_CNT_VLD BIT(0)
#define SET_CMC_TBL_CTRL_CNT_VLD(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 5, val, BIT(27)); \
	le32p_replace_bits((__le32 *)(table) + 13, SET_CMC_TBL_MASK_CTRL_CNT_VLD, \
			   BIT(27)); \
} while (0)
#define SET_CMC_TBL_MASK_CTRL_CNT GENMASK(3, 0)
#define SET_CMC_TBL_CTRL_CNT(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 5, val, GENMASK(31, 28)); \
	le32p_replace_bits((__le32 *)(table) + 13, SET_CMC_TBL_MASK_CTRL_CNT, \
			   GENMASK(31, 28)); \
} while (0)
#define SET_CMC_TBL_MASK_RESP_REF_RATE GENMASK(8, 0)
#define SET_CMC_TBL_RESP_REF_RATE(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 6, val, GENMASK(8, 0)); \
	le32p_replace_bits((__le32 *)(table) + 14, SET_CMC_TBL_MASK_RESP_REF_RATE, \
			   GENMASK(8, 0)); \
} while (0)
#define SET_CMC_TBL_MASK_ALL_ACK_SUPPORT BIT(0)
#define SET_CMC_TBL_ALL_ACK_SUPPORT(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 6, val, BIT(12)); \
	le32p_replace_bits((__le32 *)(table) + 14, SET_CMC_TBL_MASK_ALL_ACK_SUPPORT, \
			   BIT(12)); \
} while (0)
#define SET_CMC_TBL_MASK_BSR_QUEUE_SIZE_FORMAT BIT(0)
#define SET_CMC_TBL_BSR_QUEUE_SIZE_FORMAT(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 6, val, BIT(13)); \
	le32p_replace_bits((__le32 *)(table) + 14, SET_CMC_TBL_MASK_BSR_QUEUE_SIZE_FORMAT, \
			   BIT(13)); \
} while (0)
#define SET_CMC_TBL_MASK_NTX_PATH_EN GENMASK(3, 0)
#define SET_CMC_TBL_NTX_PATH_EN(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 6, val, GENMASK(19, 16)); \
	le32p_replace_bits((__le32 *)(table) + 14, SET_CMC_TBL_MASK_NTX_PATH_EN, \
			   GENMASK(19, 16)); \
} while (0)
#define SET_CMC_TBL_MASK_PATH_MAP_A GENMASK(1, 0)
#define SET_CMC_TBL_PATH_MAP_A(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 6, val, GENMASK(21, 20)); \
	le32p_replace_bits((__le32 *)(table) + 14, SET_CMC_TBL_MASK_PATH_MAP_A, \
			   GENMASK(21, 20)); \
} while (0)
#define SET_CMC_TBL_MASK_PATH_MAP_B GENMASK(1, 0)
#define SET_CMC_TBL_PATH_MAP_B(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 6, val, GENMASK(23, 22)); \
	le32p_replace_bits((__le32 *)(table) + 14, SET_CMC_TBL_MASK_PATH_MAP_B, \
			   GENMASK(23, 22)); \
} while (0)
#define SET_CMC_TBL_MASK_PATH_MAP_C GENMASK(1, 0)
#define SET_CMC_TBL_PATH_MAP_C(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 6, val, GENMASK(25, 24)); \
	le32p_replace_bits((__le32 *)(table) + 14, SET_CMC_TBL_MASK_PATH_MAP_C, \
			   GENMASK(25, 24)); \
} while (0)
#define SET_CMC_TBL_MASK_PATH_MAP_D GENMASK(1, 0)
#define SET_CMC_TBL_PATH_MAP_D(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 6, val, GENMASK(27, 26)); \
	le32p_replace_bits((__le32 *)(table) + 14, SET_CMC_TBL_MASK_PATH_MAP_D, \
			   GENMASK(27, 26)); \
} while (0)
#define SET_CMC_TBL_MASK_ANTSEL_A BIT(0)
#define SET_CMC_TBL_ANTSEL_A(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 6, val, BIT(28)); \
	le32p_replace_bits((__le32 *)(table) + 14, SET_CMC_TBL_MASK_ANTSEL_A, \
			   BIT(28)); \
} while (0)
#define SET_CMC_TBL_MASK_ANTSEL_B BIT(0)
#define SET_CMC_TBL_ANTSEL_B(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 6, val, BIT(29)); \
	le32p_replace_bits((__le32 *)(table) + 14, SET_CMC_TBL_MASK_ANTSEL_B, \
			   BIT(29)); \
} while (0)
#define SET_CMC_TBL_MASK_ANTSEL_C BIT(0)
#define SET_CMC_TBL_ANTSEL_C(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 6, val, BIT(30)); \
	le32p_replace_bits((__le32 *)(table) + 14, SET_CMC_TBL_MASK_ANTSEL_C, \
			   BIT(30)); \
} while (0)
#define SET_CMC_TBL_MASK_ANTSEL_D BIT(0)
#define SET_CMC_TBL_ANTSEL_D(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 6, val, BIT(31)); \
	le32p_replace_bits((__le32 *)(table) + 14, SET_CMC_TBL_MASK_ANTSEL_D, \
			   BIT(31)); \
} while (0)
#define SET_CMC_TBL_MASK_ADDR_CAM_INDEX GENMASK(7, 0)
#define SET_CMC_TBL_ADDR_CAM_INDEX(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 7, val, GENMASK(7, 0)); \
	le32p_replace_bits((__le32 *)(table) + 15, SET_CMC_TBL_MASK_ADDR_CAM_INDEX, \
			   GENMASK(7, 0)); \
} while (0)
#define SET_CMC_TBL_MASK_PAID GENMASK(8, 0)
#define SET_CMC_TBL_PAID(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 7, val, GENMASK(16, 8)); \
	le32p_replace_bits((__le32 *)(table) + 15, SET_CMC_TBL_MASK_PAID, \
			   GENMASK(16, 8)); \
} while (0)
#define SET_CMC_TBL_MASK_ULDL BIT(0)
#define SET_CMC_TBL_ULDL(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 7, val, BIT(17)); \
	le32p_replace_bits((__le32 *)(table) + 15, SET_CMC_TBL_MASK_ULDL, \
			   BIT(17)); \
} while (0)
#define SET_CMC_TBL_MASK_DOPPLER_CTRL GENMASK(1, 0)
#define SET_CMC_TBL_DOPPLER_CTRL(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 7, val, GENMASK(19, 18)); \
	le32p_replace_bits((__le32 *)(table) + 15, SET_CMC_TBL_MASK_DOPPLER_CTRL, \
			   GENMASK(19, 18)); \
} while (0)
#define SET_CMC_TBL_MASK_NOMINAL_PKT_PADDING GENMASK(1, 0)
#define SET_CMC_TBL_NOMINAL_PKT_PADDING(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 7, val, GENMASK(21, 20)); \
	le32p_replace_bits((__le32 *)(table) + 15, SET_CMC_TBL_MASK_NOMINAL_PKT_PADDING, \
			   GENMASK(21, 20)); \
} while (0)
#define SET_CMC_TBL_NOMINAL_PKT_PADDING40(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 7, val, GENMASK(23, 22)); \
	le32p_replace_bits((__le32 *)(table) + 15, SET_CMC_TBL_MASK_NOMINAL_PKT_PADDING, \
			   GENMASK(23, 22)); \
} while (0)
#define SET_CMC_TBL_MASK_TXPWR_TOLERENCE GENMASK(3, 0)
#define SET_CMC_TBL_TXPWR_TOLERENCE(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 7, val, GENMASK(27, 24)); \
	le32p_replace_bits((__le32 *)(table) + 15, SET_CMC_TBL_MASK_TXPWR_TOLERENCE, \
			   GENMASK(27, 24)); \
} while (0)
#define SET_CMC_TBL_NOMINAL_PKT_PADDING80(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 7, val, GENMASK(31, 30)); \
	le32p_replace_bits((__le32 *)(table) + 15, SET_CMC_TBL_MASK_NOMINAL_PKT_PADDING, \
			   GENMASK(31, 30)); \
} while (0)
#define SET_CMC_TBL_MASK_NC GENMASK(2, 0)
#define SET_CMC_TBL_NC(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 8, val, GENMASK(2, 0)); \
	le32p_replace_bits((__le32 *)(table) + 16, SET_CMC_TBL_MASK_NC, \
			   GENMASK(2, 0)); \
} while (0)
#define SET_CMC_TBL_MASK_NR GENMASK(2, 0)
#define SET_CMC_TBL_NR(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 8, val, GENMASK(5, 3)); \
	le32p_replace_bits((__le32 *)(table) + 16, SET_CMC_TBL_MASK_NR, \
			   GENMASK(5, 3)); \
} while (0)
#define SET_CMC_TBL_MASK_NG GENMASK(1, 0)
#define SET_CMC_TBL_NG(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 8, val, GENMASK(7, 6)); \
	le32p_replace_bits((__le32 *)(table) + 16, SET_CMC_TBL_MASK_NG, \
			   GENMASK(7, 6)); \
} while (0)
#define SET_CMC_TBL_MASK_CB GENMASK(1, 0)
#define SET_CMC_TBL_CB(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 8, val, GENMASK(9, 8)); \
	le32p_replace_bits((__le32 *)(table) + 16, SET_CMC_TBL_MASK_CB, \
			   GENMASK(9, 8)); \
} while (0)
#define SET_CMC_TBL_MASK_CS GENMASK(1, 0)
#define SET_CMC_TBL_CS(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 8, val, GENMASK(11, 10)); \
	le32p_replace_bits((__le32 *)(table) + 16, SET_CMC_TBL_MASK_CS, \
			   GENMASK(11, 10)); \
} while (0)
#define SET_CMC_TBL_MASK_CSI_TXBF_EN BIT(0)
#define SET_CMC_TBL_CSI_TXBF_EN(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 8, val, BIT(12)); \
	le32p_replace_bits((__le32 *)(table) + 16, SET_CMC_TBL_MASK_CSI_TXBF_EN, \
			   BIT(12)); \
} while (0)
#define SET_CMC_TBL_MASK_CSI_STBC_EN BIT(0)
#define SET_CMC_TBL_CSI_STBC_EN(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 8, val, BIT(13)); \
	le32p_replace_bits((__le32 *)(table) + 16, SET_CMC_TBL_MASK_CSI_STBC_EN, \
			   BIT(13)); \
} while (0)
#define SET_CMC_TBL_MASK_CSI_LDPC_EN BIT(0)
#define SET_CMC_TBL_CSI_LDPC_EN(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 8, val, BIT(14)); \
	le32p_replace_bits((__le32 *)(table) + 16, SET_CMC_TBL_MASK_CSI_LDPC_EN, \
			   BIT(14)); \
} while (0)
#define SET_CMC_TBL_MASK_CSI_PARA_EN BIT(0)
#define SET_CMC_TBL_CSI_PARA_EN(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 8, val, BIT(15)); \
	le32p_replace_bits((__le32 *)(table) + 16, SET_CMC_TBL_MASK_CSI_PARA_EN, \
			   BIT(15)); \
} while (0)
#define SET_CMC_TBL_MASK_CSI_FIX_RATE GENMASK(8, 0)
#define SET_CMC_TBL_CSI_FIX_RATE(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 8, val, GENMASK(24, 16)); \
	le32p_replace_bits((__le32 *)(table) + 16, SET_CMC_TBL_MASK_CSI_FIX_RATE, \
			   GENMASK(24, 16)); \
} while (0)
#define SET_CMC_TBL_MASK_CSI_GI_LTF GENMASK(2, 0)
#define SET_CMC_TBL_CSI_GI_LTF(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 8, val, GENMASK(27, 25)); \
	le32p_replace_bits((__le32 *)(table) + 16, SET_CMC_TBL_MASK_CSI_GI_LTF, \
			   GENMASK(27, 25)); \
} while (0)
#define SET_CMC_TBL_MASK_CSI_GID_SEL BIT(0)
#define SET_CMC_TBL_CSI_GID_SEL(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 8, val, BIT(29)); \
	le32p_replace_bits((__le32 *)(table) + 16, SET_CMC_TBL_MASK_CSI_GID_SEL, \
			   BIT(29)); \
} while (0)
#define SET_CMC_TBL_MASK_CSI_BW GENMASK(1, 0)
#define SET_CMC_TBL_CSI_BW(table, val) \
do { \
	le32p_replace_bits((__le32 *)(table) + 8, val, GENMASK(31, 30)); \
	le32p_replace_bits((__le32 *)(table) + 16, SET_CMC_TBL_MASK_CSI_BW, \
			   GENMASK(31, 30)); \
} while (0)

#define SET_FWROLE_MAINTAIN_MACID(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(7, 0))
#define SET_FWROLE_MAINTAIN_SELF_ROLE(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(9, 8))
#define SET_FWROLE_MAINTAIN_UPD_MODE(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(12, 10))
#define SET_FWROLE_MAINTAIN_WIFI_ROLE(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(16, 13))

#define SET_JOININFO_MACID(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(7, 0))
#define SET_JOININFO_OP(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, BIT(8))
#define SET_JOININFO_BAND(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, BIT(9))
#define SET_JOININFO_WMM(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(11, 10))
#define SET_JOININFO_TGR(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, BIT(12))
#define SET_JOININFO_ISHESTA(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, BIT(13))
#define SET_JOININFO_DLBW(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(15, 14))
#define SET_JOININFO_TF_MAC_PAD(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(17, 16))
#define SET_JOININFO_DL_T_PE(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(20, 18))
#define SET_JOININFO_PORT_ID(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(23, 21))
#define SET_JOININFO_NET_TYPE(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(25, 24))
#define SET_JOININFO_WIFI_ROLE(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(29, 26))
#define SET_JOININFO_SELF_ROLE(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(31, 30))

#define SET_GENERAL_PKT_MACID(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(7, 0))
#define SET_GENERAL_PKT_PROBRSP_ID(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(15, 8))
#define SET_GENERAL_PKT_PSPOLL_ID(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(23, 16))
#define SET_GENERAL_PKT_NULL_ID(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(31, 24))
#define SET_GENERAL_PKT_QOS_NULL_ID(h2c, val) \
	le32p_replace_bits((__le32 *)(h2c) + 1, val, GENMASK(7, 0))
#define SET_GENERAL_PKT_CTS2SELF_ID(h2c, val) \
	le32p_replace_bits((__le32 *)(h2c) + 1, val, GENMASK(15, 8))

#define SET_LOG_CFG_LEVEL(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(7, 0))
#define SET_LOG_CFG_PATH(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(15, 8))
#define SET_LOG_CFG_COMP(h2c, val) \
	le32p_replace_bits((__le32 *)(h2c) + 1, val, GENMASK(31, 0))
#define SET_LOG_CFG_COMP_EXT(h2c, val) \
	le32p_replace_bits((__le32 *)(h2c) + 2, val, GENMASK(31, 0))

#define SET_BA_CAM_VALID(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, BIT(0))
#define SET_BA_CAM_INIT_REQ(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, BIT(1))
#define SET_BA_CAM_ENTRY_IDX(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(3, 2))
#define SET_BA_CAM_TID(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(7, 4))
#define SET_BA_CAM_MACID(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(15, 8))
#define SET_BA_CAM_BMAP_SIZE(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(19, 16))
#define SET_BA_CAM_SSN(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(31, 20))

#define SET_LPS_PARM_MACID(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(7, 0))
#define SET_LPS_PARM_PSMODE(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(15, 8))
#define SET_LPS_PARM_RLBM(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(19, 16))
#define SET_LPS_PARM_SMARTPS(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(23, 20))
#define SET_LPS_PARM_AWAKEINTERVAL(h2c, val) \
	le32p_replace_bits((__le32 *)h2c, val, GENMASK(31, 24))
#define SET_LPS_PARM_VOUAPSD(h2c, val) \
	le32p_replace_bits((__le32 *)(h2c) + 1, val, BIT(0))
#define SET_LPS_PARM_VIUAPSD(h2c, val) \
	le32p_replace_bits((__le32 *)(h2c) + 1, val, BIT(1))
#define SET_LPS_PARM_BEUAPSD(h2c, val) \
	le32p_replace_bits((__le32 *)(h2c) + 1, val, BIT(2))
#define SET_LPS_PARM_BKUAPSD(h2c, val) \
	le32p_replace_bits((__le32 *)(h2c) + 1, val, BIT(3))
#define SET_LPS_PARM_LASTRPWM(h2c, val) \
	le32p_replace_bits((__le32 *)(h2c) + 1, val, GENMASK(15, 8))

enum rtw89_btc_btf_h2c_class {
	BTFC_SET = 0x10,
	BTFC_GET = 0x11,
	BTFC_FW_EVENT = 0x12,
};

enum rtw89_btc_btf_set {
	SET_REPORT_EN = 0x0,
	SET_SLOT_TABLE,
	SET_MREG_TABLE,
	SET_CX_POLICY,
	SET_GPIO_DBG,
	SET_DRV_INFO,
	SET_DRV_EVENT,
	SET_BT_WREG_ADDR,
	SET_BT_WREG_VAL,
	SET_BT_RREG_ADDR,
	SET_BT_WL_CH_INFO,
	SET_BT_INFO_REPORT,
	SET_BT_IGNORE_WLAN_ACT,
	SET_BT_TX_PWR,
	SET_BT_LNA_CONSTRAIN,
	SET_BT_GOLDEN_RX_RANGE,
	SET_BT_PSD_REPORT,
	SET_H2C_TEST,
	SET_MAX1,
};

enum rtw89_btc_cxdrvinfo {
	CXDRVINFO_INIT = 0,
	CXDRVINFO_ROLE,
	CXDRVINFO_DBCC,
	CXDRVINFO_SMAP,
	CXDRVINFO_RFK,
	CXDRVINFO_RUN,
	CXDRVINFO_CTRL,
	CXDRVINFO_SCAN,
	CXDRVINFO_MAX,
};

#define RTW89_SET_FWCMD_CXHDR_TYPE(cmd, val) \
	u8p_replace_bits((u8 *)(cmd) + 0, val, GENMASK(7, 0))
#define RTW89_SET_FWCMD_CXHDR_LEN(cmd, val) \
	u8p_replace_bits((u8 *)(cmd) + 1, val, GENMASK(7, 0))

#define RTW89_SET_FWCMD_CXINIT_ANT_TYPE(cmd, val) \
	u8p_replace_bits((u8 *)(cmd) + 2, val, GENMASK(7, 0))
#define RTW89_SET_FWCMD_CXINIT_ANT_NUM(cmd, val) \
	u8p_replace_bits((u8 *)(cmd) + 3, val, GENMASK(7, 0))
#define RTW89_SET_FWCMD_CXINIT_ANT_ISO(cmd, val) \
	u8p_replace_bits((u8 *)(cmd) + 4, val, GENMASK(7, 0))
#define RTW89_SET_FWCMD_CXINIT_ANT_POS(cmd, val) \
	u8p_replace_bits((u8 *)(cmd) + 5, val, BIT(0))
#define RTW89_SET_FWCMD_CXINIT_ANT_DIVERSITY(cmd, val) \
	u8p_replace_bits((u8 *)(cmd) + 5, val, BIT(1))
#define RTW89_SET_FWCMD_CXINIT_MOD_RFE(cmd, val) \
	u8p_replace_bits((u8 *)(cmd) + 6, val, GENMASK(7, 0))
#define RTW89_SET_FWCMD_CXINIT_MOD_CV(cmd, val) \
	u8p_replace_bits((u8 *)(cmd) + 7, val, GENMASK(7, 0))
#define RTW89_SET_FWCMD_CXINIT_MOD_BT_SOLO(cmd, val) \
	u8p_replace_bits((u8 *)(cmd) + 8, val, BIT(0))
#define RTW89_SET_FWCMD_CXINIT_MOD_BT_POS(cmd, val) \
	u8p_replace_bits((u8 *)(cmd) + 8, val, BIT(1))
#define RTW89_SET_FWCMD_CXINIT_MOD_SW_TYPE(cmd, val) \
	u8p_replace_bits((u8 *)(cmd) + 8, val, BIT(2))
#define RTW89_SET_FWCMD_CXINIT_WL_GCH(cmd, val) \
	u8p_replace_bits((u8 *)(cmd) + 10, val, GENMASK(7, 0))
#define RTW89_SET_FWCMD_CXINIT_WL_ONLY(cmd, val) \
	u8p_replace_bits((u8 *)(cmd) + 11, val, BIT(0))
#define RTW89_SET_FWCMD_CXINIT_WL_INITOK(cmd, val) \
	u8p_replace_bits((u8 *)(cmd) + 11, val, BIT(1))
#define RTW89_SET_FWCMD_CXINIT_DBCC_EN(cmd, val) \
	u8p_replace_bits((u8 *)(cmd) + 11, val, BIT(2))
#define RTW89_SET_FWCMD_CXINIT_CX_OTHER(cmd, val) \
	u8p_replace_bits((u8 *)(cmd) + 11, val, BIT(3))
#define RTW89_SET_FWCMD_CXINIT_BT_ONLY(cmd, val) \
	u8p_replace_bits((u8 *)(cmd) + 11, val, BIT(4))

#define RTW89_SET_FWCMD_CXROLE_CONNECT_CNT(cmd, val) \
	u8p_replace_bits((u8 *)(cmd) + 2, val, GENMASK(7, 0))
#define RTW89_SET_FWCMD_CXROLE_LINK_MODE(cmd, val) \
	u8p_replace_bits((u8 *)(cmd) + 3, val, GENMASK(7, 0))
#define RTW89_SET_FWCMD_CXROLE_ROLE_NONE(cmd, val) \
	le16p_replace_bits((__le16 *)((u8 *)(cmd) + 4), val, BIT(0))
#define RTW89_SET_FWCMD_CXROLE_ROLE_STA(cmd, val) \
	le16p_replace_bits((__le16 *)((u8 *)(cmd) + 4), val, BIT(1))
#define RTW89_SET_FWCMD_CXROLE_ROLE_AP(cmd, val) \
	le16p_replace_bits((__le16 *)((u8 *)(cmd) + 4), val, BIT(2))
#define RTW89_SET_FWCMD_CXROLE_ROLE_VAP(cmd, val) \
	le16p_replace_bits((__le16 *)((u8 *)(cmd) + 4), val, BIT(3))
#define RTW89_SET_FWCMD_CXROLE_ROLE_ADHOC(cmd, val) \
	le16p_replace_bits((__le16 *)((u8 *)(cmd) + 4), val, BIT(4))
#define RTW89_SET_FWCMD_CXROLE_ROLE_ADHOC_MASTER(cmd, val) \
	le16p_replace_bits((__le16 *)((u8 *)(cmd) + 4), val, BIT(5))
#define RTW89_SET_FWCMD_CXROLE_ROLE_MESH(cmd, val) \
	le16p_replace_bits((__le16 *)((u8 *)(cmd) + 4), val, BIT(6))
#define RTW89_SET_FWCMD_CXROLE_ROLE_MONITOR(cmd, val) \
	le16p_replace_bits((__le16 *)((u8 *)(cmd) + 4), val, BIT(7))
#define RTW89_SET_FWCMD_CXROLE_ROLE_P2P_DEV(cmd, val) \
	le16p_replace_bits((__le16 *)((u8 *)(cmd) + 4), val, BIT(8))
#define RTW89_SET_FWCMD_CXROLE_ROLE_P2P_GC(cmd, val) \
	le16p_replace_bits((__le16 *)((u8 *)(cmd) + 4), val, BIT(9))
#define RTW89_SET_FWCMD_CXROLE_ROLE_P2P_GO(cmd, val) \
	le16p_replace_bits((__le16 *)((u8 *)(cmd) + 4), val, BIT(10))
#define RTW89_SET_FWCMD_CXROLE_ROLE_NAN(cmd, val) \
	le16p_replace_bits((__le16 *)((u8 *)(cmd) + 4), val, BIT(11))
#define RTW89_SET_FWCMD_CXROLE_ACT_CONNECTED(cmd, val, n) \
	u8p_replace_bits((u8 *)(cmd) + (6 + 12 * (n)), val, BIT(0))
#define RTW89_SET_FWCMD_CXROLE_ACT_PID(cmd, val, n) \
	u8p_replace_bits((u8 *)(cmd) + (6 + 12 * (n)), val, GENMASK(3, 1))
#define RTW89_SET_FWCMD_CXROLE_ACT_PHY(cmd, val, n) \
	u8p_replace_bits((u8 *)(cmd) + (6 + 12 * (n)), val, BIT(4))
#define RTW89_SET_FWCMD_CXROLE_ACT_NOA(cmd, val, n) \
	u8p_replace_bits((u8 *)(cmd) + (6 + 12 * (n)), val, BIT(5))
#define RTW89_SET_FWCMD_CXROLE_ACT_BAND(cmd, val, n) \
	u8p_replace_bits((u8 *)(cmd) + (6 + 12 * (n)), val, GENMASK(7, 6))
#define RTW89_SET_FWCMD_CXROLE_ACT_CLIENT_PS(cmd, val, n) \
	u8p_replace_bits((u8 *)(cmd) + (7 + 12 * (n)), val, BIT(0))
#define RTW89_SET_FWCMD_CXROLE_ACT_BW(cmd, val, n) \
	u8p_replace_bits((u8 *)(cmd) + (7 + 12 * (n)), val, GENMASK(7, 1))
#define RTW89_SET_FWCMD_CXROLE_ACT_ROLE(cmd, val, n) \
	u8p_replace_bits((u8 *)(cmd) + (8 + 12 * (n)), val, GENMASK(7, 0))
#define RTW89_SET_FWCMD_CXROLE_ACT_CH(cmd, val, n) \
	u8p_replace_bits((u8 *)(cmd) + (9 + 12 * (n)), val, GENMASK(7, 0))
#define RTW89_SET_FWCMD_CXROLE_ACT_TX_LVL(cmd, val, n) \
	le16p_replace_bits((__le16 *)((u8 *)(cmd) + (10 + 12 * (n))), val, GENMASK(15, 0))
#define RTW89_SET_FWCMD_CXROLE_ACT_RX_LVL(cmd, val, n) \
	le16p_replace_bits((__le16 *)((u8 *)(cmd) + (12 + 12 * (n))), val, GENMASK(15, 0))
#define RTW89_SET_FWCMD_CXROLE_ACT_TX_RATE(cmd, val, n) \
	le16p_replace_bits((__le16 *)((u8 *)(cmd) + (14 + 12 * (n))), val, GENMASK(15, 0))
#define RTW89_SET_FWCMD_CXROLE_ACT_RX_RATE(cmd, val, n) \
	le16p_replace_bits((__le16 *)((u8 *)(cmd) + (16 + 12 * (n))), val, GENMASK(15, 0))

#define RTW89_SET_FWCMD_CXCTRL_MANUAL(cmd, val) \
	le32p_replace_bits((__le32 *)((u8 *)(cmd) + 2), val, BIT(0))
#define RTW89_SET_FWCMD_CXCTRL_IGNORE_BT(cmd, val) \
	le32p_replace_bits((__le32 *)((u8 *)(cmd) + 2), val, BIT(1))
#define RTW89_SET_FWCMD_CXCTRL_ALWAYS_FREERUN(cmd, val) \
	le32p_replace_bits((__le32 *)((u8 *)(cmd) + 2), val, BIT(2))
#define RTW89_SET_FWCMD_CXCTRL_TRACE_STEP(cmd, val) \
	le32p_replace_bits((__le32 *)((u8 *)(cmd) + 2), val, GENMASK(18, 3))

#define RTW89_SET_FWCMD_CXRFK_STATE(cmd, val) \
	le32p_replace_bits((__le32 *)((u8 *)(cmd) + 2), val, GENMASK(1, 0))
#define RTW89_SET_FWCMD_CXRFK_PATH_MAP(cmd, val) \
	le32p_replace_bits((__le32 *)((u8 *)(cmd) + 2), val, GENMASK(5, 2))
#define RTW89_SET_FWCMD_CXRFK_PHY_MAP(cmd, val) \
	le32p_replace_bits((__le32 *)((u8 *)(cmd) + 2), val, GENMASK(7, 6))
#define RTW89_SET_FWCMD_CXRFK_BAND(cmd, val) \
	le32p_replace_bits((__le32 *)((u8 *)(cmd) + 2), val, GENMASK(9, 8))
#define RTW89_SET_FWCMD_CXRFK_TYPE(cmd, val) \
	le32p_replace_bits((__le32 *)((u8 *)(cmd) + 2), val, GENMASK(17, 10))

#define RTW89_C2H_HEADER_LEN 8

#define RTW89_GET_C2H_CATEGORY(c2h) \
	le32_get_bits(*((__le32 *)c2h), GENMASK(1, 0))
#define RTW89_GET_C2H_CLASS(c2h) \
	le32_get_bits(*((__le32 *)c2h), GENMASK(7, 2))
#define RTW89_GET_C2H_FUNC(c2h) \
	le32_get_bits(*((__le32 *)c2h), GENMASK(15, 8))
#define RTW89_GET_C2H_LEN(c2h) \
	le32_get_bits(*((__le32 *)(c2h) + 1), GENMASK(13, 0))

#define RTW89_GET_C2H_LOG_SRT_PRT(c2h) (char *)((__le32 *)(c2h) + 2)
#define RTW89_GET_C2H_LOG_LEN(len) ((len) - RTW89_C2H_HEADER_LEN)

#define RTW89_GET_MAC_C2H_DONE_ACK_CAT(c2h) \
	le32_get_bits(*((__le32 *)(c2h) + 2), GENMASK(1, 0))
#define RTW89_GET_MAC_C2H_DONE_ACK_CLASS(c2h) \
	le32_get_bits(*((__le32 *)(c2h) + 2), GENMASK(7, 2))
#define RTW89_GET_MAC_C2H_DONE_ACK_FUNC(c2h) \
	le32_get_bits(*((__le32 *)(c2h) + 2), GENMASK(15, 8))
#define RTW89_GET_MAC_C2H_DONE_ACK_H2C_RETURN(c2h) \
	le32_get_bits(*((__le32 *)(c2h) + 2), GENMASK(23, 16))
#define RTW89_GET_MAC_C2H_DONE_ACK_H2C_SEQ(c2h) \
	le32_get_bits(*((__le32 *)(c2h) + 2), GENMASK(31, 24))

#define RTW89_GET_MAC_C2H_REV_ACK_CAT(c2h) \
	le32_get_bits(*((__le32 *)(c2h) + 2), GENMASK(1, 0))
#define RTW89_GET_MAC_C2H_REV_ACK_CLASS(c2h) \
	le32_get_bits(*((__le32 *)(c2h) + 2), GENMASK(7, 2))
#define RTW89_GET_MAC_C2H_REV_ACK_FUNC(c2h) \
	le32_get_bits(*((__le32 *)(c2h) + 2), GENMASK(15, 8))
#define RTW89_GET_MAC_C2H_REV_ACK_H2C_SEQ(c2h) \
	le32_get_bits(*((__le32 *)(c2h) + 2), GENMASK(23, 16))

#define RTW89_GET_PHY_C2H_RA_RPT_MACID(c2h) \
	le32_get_bits(*((__le32 *)(c2h) + 2), GENMASK(15, 0))
#define RTW89_GET_PHY_C2H_RA_RPT_RETRY_RATIO(c2h) \
	le32_get_bits(*((__le32 *)(c2h) + 2), GENMASK(23, 16))
#define RTW89_GET_PHY_C2H_RA_RPT_MCSNSS(c2h) \
	le32_get_bits(*((__le32 *)(c2h) + 3), GENMASK(6, 0))
#define RTW89_GET_PHY_C2H_RA_RPT_MD_SEL(c2h) \
	le32_get_bits(*((__le32 *)(c2h) + 3), GENMASK(9, 8))
#define RTW89_GET_PHY_C2H_RA_RPT_GILTF(c2h) \
	le32_get_bits(*((__le32 *)(c2h) + 3), GENMASK(12, 10))
#define RTW89_GET_PHY_C2H_RA_RPT_BW(c2h) \
	le32_get_bits(*((__le32 *)(c2h) + 3), GENMASK(14, 13))

/* VHT, HE, HT-old: [6:4]: NSS, [3:0]: MCS
 * HT-new: [6:5]: NA, [4:0]: MCS
 */
#define RTW89_RA_RATE_MASK_NSS GENMASK(6, 4)
#define RTW89_RA_RATE_MASK_MCS GENMASK(3, 0)
#define RTW89_RA_RATE_MASK_HT_MCS GENMASK(4, 0)
#define RTW89_MK_HT_RATE(nss, mcs) (FIELD_PREP(GENMASK(4, 3), nss) | \
				    FIELD_PREP(GENMASK(2, 0), mcs))

#define RTW89_FW_HDR_SIZE 32
#define RTW89_FW_SECTION_HDR_SIZE 16

#define RTW89_MFW_SIG	0xFF

struct rtw89_mfw_info {
	u8 cv;
	u8 type; /* enum rtw89_fw_type */
	u8 mp;
	u8 rsvd;
	__le32 shift;
	__le32 size;
	u8 rsvd2[4];
} __packed;

struct rtw89_mfw_hdr {
	u8 sig;	/* RTW89_MFW_SIG */
	u8 fw_nr;
	u8 rsvd[14];
	struct rtw89_mfw_info info[];
} __packed;

struct fwcmd_hdr {
	__le32 hdr0;
	__le32 hdr1;
};

#define RTW89_H2C_RF_PAGE_SIZE 500
#define RTW89_H2C_RF_PAGE_NUM 3
struct rtw89_fw_h2c_rf_reg_info {
	enum rtw89_rf_path rf_path;
	__le32 rtw89_phy_config_rf_h2c[RTW89_H2C_RF_PAGE_NUM][RTW89_H2C_RF_PAGE_SIZE];
	u16 curr_idx;
};

#define H2C_SEC_CAM_LEN			24

#define H2C_HEADER_LEN			8
#define H2C_HDR_CAT			GENMASK(1, 0)
#define H2C_HDR_CLASS			GENMASK(7, 2)
#define H2C_HDR_FUNC			GENMASK(15, 8)
#define H2C_HDR_DEL_TYPE		GENMASK(19, 16)
#define H2C_HDR_H2C_SEQ			GENMASK(31, 24)
#define H2C_HDR_TOTAL_LEN		GENMASK(13, 0)
#define H2C_HDR_REC_ACK			BIT(14)
#define H2C_HDR_DONE_ACK		BIT(15)

#define FWCMD_TYPE_H2C			0

#define H2C_CAT_MAC		0x1

/* CLASS 0 - FW INFO */
#define H2C_CL_FW_INFO			0x0
#define H2C_FUNC_LOG_CFG		0x0
#define H2C_FUNC_MAC_GENERAL_PKT	0x1

/* CLASS 2 - PS */
#define H2C_CL_MAC_PS			0x2
#define H2C_FUNC_MAC_LPS_PARM		0x0

/* CLASS 3 - FW download */
#define H2C_CL_MAC_FWDL		0x3
#define H2C_FUNC_MAC_FWHDR_DL		0x0

/* CLASS 5 - Frame Exchange */
#define H2C_CL_MAC_FR_EXCHG		0x5
#define H2C_FUNC_MAC_CCTLINFO_UD	0x2

/* CLASS 6 - Address CAM */
#define H2C_CL_MAC_ADDR_CAM_UPDATE	0x6
#define H2C_FUNC_MAC_ADDR_CAM_UPD	0x0

/* CLASS 8 - Media Status Report */
#define H2C_CL_MAC_MEDIA_RPT		0x8
#define H2C_FUNC_MAC_JOININFO		0x0
#define H2C_FUNC_MAC_FWROLE_MAINTAIN	0x4

/* CLASS 9 - FW offload */
#define H2C_CL_MAC_FW_OFLD		0x9
#define H2C_FUNC_MAC_MACID_PAUSE	0x8
#define H2C_FUNC_USR_EDCA		0xF
#define H2C_FUNC_OFLD_CFG		0x14

/* CLASS 10 - Security CAM */
#define H2C_CL_MAC_SEC_CAM		0xa
#define H2C_FUNC_MAC_SEC_UPD		0x1

/* CLASS 12 - BA CAM */
#define H2C_CL_BA_CAM			0xc
#define H2C_FUNC_MAC_BA_CAM		0x0

#define H2C_CAT_OUTSRC			0x2

#define H2C_CL_OUTSRC_RA		0x1
#define H2C_FUNC_OUTSRC_RA_MACIDCFG	0x0

#define H2C_CL_OUTSRC_RF_REG_A		0x8
#define H2C_CL_OUTSRC_RF_REG_B		0x9

int rtw89_fw_check_rdy(struct rtw89_dev *rtwdev);
int rtw89_fw_recognize(struct rtw89_dev *rtwdev);
int rtw89_fw_download(struct rtw89_dev *rtwdev, enum rtw89_fw_type type);
int rtw89_load_firmware(struct rtw89_dev *rtwdev);
void rtw89_unload_firmware(struct rtw89_dev *rtwdev);
int rtw89_wait_firmware_completion(struct rtw89_dev *rtwdev);
void rtw89_h2c_pkt_set_hdr(struct rtw89_dev *rtwdev, struct sk_buff *skb,
			   u8 type, u8 cat, u8 class, u8 func,
			   bool rack, bool dack, u32 len);
int rtw89_fw_h2c_default_cmac_tbl(struct rtw89_dev *rtwdev, u8 macid);
int rtw89_fw_h2c_assoc_cmac_tbl(struct rtw89_dev *rtwdev,
				struct ieee80211_vif *vif,
				struct ieee80211_sta *sta);
int rtw89_fw_h2c_txtime_cmac_tbl(struct rtw89_dev *rtwdev,
				 struct rtw89_sta *rtwsta);
int rtw89_fw_h2c_cam(struct rtw89_dev *rtwdev, struct rtw89_vif *vif);
void rtw89_fw_c2h_irqsafe(struct rtw89_dev *rtwdev, struct sk_buff *c2h);
void rtw89_fw_c2h_work(struct work_struct *work);
int rtw89_fw_h2c_vif_maintain(struct rtw89_dev *rtwdev,
			      struct rtw89_vif *rtwvif,
			      enum rtw89_upd_mode upd_mode);
int rtw89_fw_h2c_join_info(struct rtw89_dev *rtwdev, struct rtw89_vif *rtwvif,
			   u8 dis_conn);
int rtw89_fw_h2c_macid_pause(struct rtw89_dev *rtwdev, u8 sh, u8 grp,
			     bool pause);
int rtw89_fw_h2c_set_edca(struct rtw89_dev *rtwdev, struct rtw89_vif *rtwvif,
			  u8 ac, u32 val);
int rtw89_fw_h2c_set_ofld_cfg(struct rtw89_dev *rtwdev);
int rtw89_fw_h2c_ra(struct rtw89_dev *rtwdev, struct rtw89_ra_info *ra, bool csi);
int rtw89_fw_h2c_cxdrv_init(struct rtw89_dev *rtwdev);
int rtw89_fw_h2c_cxdrv_role(struct rtw89_dev *rtwdev);
int rtw89_fw_h2c_cxdrv_ctrl(struct rtw89_dev *rtwdev);
int rtw89_fw_h2c_cxdrv_rfk(struct rtw89_dev *rtwdev);
int rtw89_fw_h2c_rf_reg(struct rtw89_dev *rtwdev,
			struct rtw89_fw_h2c_rf_reg_info *info,
			u16 len, u8 page);
int rtw89_fw_h2c_raw_with_hdr(struct rtw89_dev *rtwdev,
			      u8 h2c_class, u8 h2c_func, u8 *buf, u16 len,
			      bool rack, bool dack);
int rtw89_fw_h2c_raw(struct rtw89_dev *rtwdev, const u8 *buf, u16 len);
void rtw89_fw_send_all_early_h2c(struct rtw89_dev *rtwdev);
void rtw89_fw_free_all_early_h2c(struct rtw89_dev *rtwdev);
int rtw89_fw_h2c_general_pkt(struct rtw89_dev *rtwdev, u8 macid);
int rtw89_fw_h2c_ba_cam(struct rtw89_dev *rtwdev, bool valid, u8 macid,
			struct ieee80211_ampdu_params *params);
int rtw89_fw_h2c_lps_parm(struct rtw89_dev *rtwdev,
			  struct rtw89_lps_parm *lps_param);
struct sk_buff *rtw89_fw_h2c_alloc_skb_with_hdr(u32 len);
struct sk_buff *rtw89_fw_h2c_alloc_skb_no_hdr(u32 len);
int rtw89_fw_msg_reg(struct rtw89_dev *rtwdev,
		     struct rtw89_mac_h2c_info *h2c_info,
		     struct rtw89_mac_c2h_info *c2h_info);
int rtw89_fw_h2c_fw_log(struct rtw89_dev *rtwdev, bool enable);
void rtw89_fw_st_dbg_dump(struct rtw89_dev *rtwdev);

#endif
