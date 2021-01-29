/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright(c) 2019-2020  Realtek Corporation
 */

#ifndef __RTW89_CORE_H__
#define __RTW89_CORE_H__

#include <net/mac80211.h>
#include <linux/bitfield.h>
#include <linux/firmware.h>
#include <linux/average.h>
#include <linux/iopoll.h>

struct rtw89_dev;

extern const struct ieee80211_ops rtw89_ops;
extern const struct rtw89_chip_info rtw8852a_chip_info;

#define MASKBYTE0 0xff
#define MASKBYTE1 0xff00
#define MASKBYTE2 0xff0000
#define MASKBYTE3 0xff000000
#define MASKBYTE4 0xff00000000ULL
#define MASKHWORD 0xffff0000
#define MASKLWORD 0x0000ffff
#define MASKDWORD 0xffffffff
#define RFREG_MASK 0xfffff
#define INV_RF_DATA 0xffffffff

#define RTW89_TRACK_WORK_PERIOD	round_jiffies_relative(HZ * 2)
#define CFO_TRACK_MAX_USER 128
#define MAX_RSSI 110

enum rtw89_subband {
	RTW89_CH_2G = 0,
	RTW89_CH_5G_BAND_1 = 1,
	/* RTW89_CH_5G_BAND_2 = 2, unused */
	RTW89_CH_5G_BAND_3 = 3,
	RTW89_CH_5G_BAND_4 = 4,
};

enum rtw89_hci_type {
	RTW89_HCI_TYPE_PCIE,
	RTW89_HCI_TYPE_USB,
	RTW89_HCI_TYPE_SDIO,
};

enum rtw89_core_chip_id {
	RTL8852A,
	RTL8852B,
};

enum rtw89_cut_version {
	CHIP_CUT_A,
	CHIP_CUT_B,
	CHIP_CUT_C,
	CHIP_CUT_D,
	CHIP_CUT_E,
	CHIP_CUT_F,
	CHIP_CUT_MAX,
	CHIP_CUT_INVALID = CHIP_CUT_MAX,
};

enum rtw89_core_tx_type {
	RTW89_CORE_TX_TYPE_DATA,
	RTW89_CORE_TX_TYPE_MGMT,
	RTW89_CORE_TX_TYPE_FWCMD,
};

enum rtw89_core_rx_type {
	RTW89_CORE_RX_TYPE_WIFI		= 0,
	RTW89_CORE_RX_TYPE_PPDU_STAT	= 1,
	RTW89_CORE_RX_TYPE_CHAN_INFO	= 2,
	RTW89_CORE_RX_TYPE_BB_SCOPE	= 3,
	RTW89_CORE_RX_TYPE_F2P_TXCMD	= 4,
	RTW89_CORE_RX_TYPE_SS2FW	= 5,
	RTW89_CORE_RX_TYPE_TX_REPORT	= 6,
	RTW89_CORE_RX_TYPE_TX_REL_HOST	= 7,
	RTW89_CORE_RX_TYPE_DFS_REPORT	= 8,
	RTW89_CORE_RX_TYPE_TX_REL_CPU	= 9,
	RTW89_CORE_RX_TYPE_C2H		= 10,
	RTW89_CORE_RX_TYPE_CSI		= 11,
	RTW89_CORE_RX_TYPE_CQI		= 12,
};

enum rtw89_txq_flags {
	RTW89_TXQ_F_AMPDU		= 0,
	RTW89_TXQ_F_BLOCK_BA		= 1,
};

enum rtw89_net_type {
	RTW89_NET_TYPE_NO_LINK		= 0,
	RTW89_NET_TYPE_AD_HOC		= 1,
	RTW89_NET_TYPE_INFRA		= 2,
	RTW89_NET_TYPE_AP_MODE		= 3,
};

enum rtw89_wifi_role {
	RTW89_WIFI_ROLE_NONE,
	RTW89_WIFI_ROLE_STATION,
	RTW89_WIFI_ROLE_AP,
	RTW89_WIFI_ROLE_AP_VLAN,
	RTW89_WIFI_ROLE_ADHOC,
	RTW89_WIFI_ROLE_ADHOC_MASTER,
	RTW89_WIFI_ROLE_MESH_POINT,
	RTW89_WIFI_ROLE_MONITOR,
	RTW89_WIFI_ROLE_P2P_DEVICE,
	RTW89_WIFI_ROLE_P2P_CLIENT,
	RTW89_WIFI_ROLE_P2P_GO,
	RTW89_WIFI_ROLE_NAN,
	RTW89_WIFI_ROLE_MLME_MAX
};

enum rtw89_upd_mode {
	RTW89_VIF_CREATE,
	RTW89_VIF_REMOVE,
	RTW89_VIF_TYPE_CHANGE,
	RTW89_VIF_INFO_CHANGE,
	RTW89_VIF_CON_DISCONN
};

enum rtw89_self_role {
	RTW89_SELF_ROLE_CLIENT,
	RTW89_SELF_ROLE_AP,
	RTW89_SELF_ROLE_AP_CLIENT
};

enum rtw89_msk_sO_el {
	RTW89_NO_MSK,
	RTW89_SMA,
	RTW89_TMA,
	RTW89_BSSID
};

enum rtw89_sch_tx_sel {
	RTW89_SCH_TX_SEL_ALL,
	RTW89_SCH_TX_SEL_HIQ,
	RTW89_SCH_TX_SEL_MG0,
	RTW89_SCH_TX_SEL_MACID,
};

/* RTW89_ADDR_CAM_SEC_NONE	: not enabled
 * RTW89_ADDR_CAM_SEC_ALL_UNI	: 0 - 6 unicast
 * RTW89_ADDR_CAM_SEC_NORMAL	: 0 - 1 unicast, 2 - 4 group, 5 - 6 BIP
 * RTW89_ADDR_CAM_SEC_4GROUP	: 0 - 1 unicast, 2 - 5 group, 6 BIP
 */
enum rtw89_add_cam_sec_mode {
	RTW89_ADDR_CAM_SEC_NONE		= 0,
	RTW89_ADDR_CAM_SEC_ALL_UNI	= 1,
	RTW89_ADDR_CAM_SEC_NORMAL	= 2,
	RTW89_ADDR_CAM_SEC_4GROUP	= 3,
};

enum rtw89_sec_key_type {
	RTW89_SEC_KEY_TYPE_NONE		= 0,
	RTW89_SEC_KEY_TYPE_WEP40	= 1,
	RTW89_SEC_KEY_TYPE_WEP104	= 2,
	RTW89_SEC_KEY_TYPE_TKIP		= 3,
	RTW89_SEC_KEY_TYPE_WAPI		= 4,
	RTW89_SEC_KEY_TYPE_GCMSMS4	= 5,
	RTW89_SEC_KEY_TYPE_CCMP128	= 6,
	RTW89_SEC_KEY_TYPE_CCMP256	= 7,
	RTW89_SEC_KEY_TYPE_GCMP128	= 8,
	RTW89_SEC_KEY_TYPE_GCMP256	= 9,
	RTW89_SEC_KEY_TYPE_BIP_CCMP128	= 10,
};

enum rtw89_port {
	RTW89_PORT_0 = 0,
	RTW89_PORT_1 = 1,
	RTW89_PORT_2 = 2,
	RTW89_PORT_3 = 3,
	RTW89_PORT_4 = 4,
	RTW89_PORT_NUM
};

enum rtw89_band {
	RTW89_BAND_2G = 0,
	RTW89_BAND_5G = 1,
	RTW89_BAND_MAX,
};

/* 2G channels,
 * 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14
 */
#define RTW89_2G_CH_NUM 14

/* 5G channels,
 * 36, 38, 40, 42, 44, 46, 48, 50,
 * 52, 54, 56, 58, 60, 62, 64,
 * 100, 102, 104, 106, 108, 110, 112, 114,
 * 116, 118, 120, 122, 124, 126, 128, 130,
 * 132, 134, 136, 138, 140, 142, 144,
 * 149, 151, 153, 155, 157, 159, 161, 163,
 * 165, 167, 169, 171, 173, 175, 177
 */
#define RTW89_5G_CH_NUM 53

enum rtw89_rate_section {
	RTW89_RS_CCK,
	RTW89_RS_OFDM,
	RTW89_RS_MCS, /* for HT/VHT/HE */
	RTW89_RS_HEDCM,
	RTW89_RS_OFFSET,
	RTW89_RS_MAX,
	RTW89_RS_LMT_NUM = RTW89_RS_MCS + 1,
};

enum rtw89_rate_max {
	RTW89_RATE_CCK_MAX	= 4,
	RTW89_RATE_OFDM_MAX	= 8,
	RTW89_RATE_MCS_MAX	= 12,
	RTW89_RATE_HEDCM_MAX	= 4, /* for HEDCM MCS0/1/3/4 */
	RTW89_RATE_OFFSET_MAX	= 5, /* for HE(HEDCM)/VHT/HT/OFDM/CCK offset */
};

enum rtw89_nss {
	RTW89_NSS_1		= 0,
	RTW89_NSS_2		= 1,
	/* HE DCM only support 1ss and 2ss */
	RTW89_NSS_HEDCM_MAX	= RTW89_NSS_2 + 1,
	RTW89_NSS_3		= 2,
	RTW89_NSS_4		= 3,
	RTW89_NSS_MAX,
};

enum rtw89_ntx {
	RTW89_1TX	= 0,
	RTW89_2TX	= 1,
	RTW89_NTX_NUM,
};

enum rtw89_beamforming_type {
	RTW89_NONBF	= 0,
	RTW89_BF	= 1,
	RTW89_BF_NUM,
};

enum rtw89_regulation_type {
	RTW89_WW	= 0,
	RTW89_ETSI	= 1,
	RTW89_FCC	= 2,
	RTW89_MKK	= 3,
	RTW89_NA	= 4,
	RTW89_IC	= 5,
	RTW89_KCC	= 6,
	RTW89_NCC	= 7,
	RTW89_CHILE	= 8,
	RTW89_ACMA	= 9,
	RTW89_MEXICO	= 10,
	RTW89_UKRAINE	= 11,
	RTW89_CN	= 12,
	RTW89_REGD_NUM,
};

extern const u8 rtw89_rs_idx_max[RTW89_RS_MAX];
extern const u8 rtw89_rs_nss_max[RTW89_RS_MAX];

struct rtw89_txpwr_byrate {
	s8 cck[RTW89_RATE_CCK_MAX];
	s8 ofdm[RTW89_RATE_OFDM_MAX];
	s8 mcs[RTW89_NSS_MAX][RTW89_RATE_MCS_MAX];
	s8 hedcm[RTW89_NSS_HEDCM_MAX][RTW89_RATE_HEDCM_MAX];
	s8 offset[RTW89_RATE_OFFSET_MAX];
};

enum rtw89_bandwidth_section_num {
	RTW89_BW20_SEC_NUM = 8,
	RTW89_BW40_SEC_NUM = 4,
	RTW89_BW80_SEC_NUM = 2,
};

struct rtw89_txpwr_limit {
	s8 cck_20m[RTW89_BF_NUM];
	s8 cck_40m[RTW89_BF_NUM];
	s8 ofdm[RTW89_BF_NUM];
	s8 mcs_20m[RTW89_BW20_SEC_NUM][RTW89_BF_NUM];
	s8 mcs_40m[RTW89_BW40_SEC_NUM][RTW89_BF_NUM];
	s8 mcs_80m[RTW89_BW80_SEC_NUM][RTW89_BF_NUM];
	s8 mcs_160m[RTW89_BF_NUM];
	s8 mcs_40m_0p5[RTW89_BF_NUM];
	s8 mcs_40m_2p5[RTW89_BF_NUM];
};

#define RTW89_RU_SEC_NUM 8

struct rtw89_txpwr_limit_ru {
	s8 ru26[RTW89_RU_SEC_NUM];
	s8 ru52[RTW89_RU_SEC_NUM];
	s8 ru106[RTW89_RU_SEC_NUM];
};

struct rtw89_rate_desc {
	enum rtw89_nss nss;
	enum rtw89_rate_section rs;
	u8 idx;
};

#define PHY_STS_HDR_LEN 8
#define RF_PATH_MAX 4
#define RTW89_MAX_PPDU_CNT 8
struct rtw89_rx_phy_ppdu {
	u8 *buf;
	u32 len;
	u8 rssi_avg;
	s8 rssi[RF_PATH_MAX];
	u8 mac_id;
	bool to_self;
	bool valid;
};

enum rtw89_mac_idx {
	RTW89_MAC_0 = 0,
	RTW89_MAC_1 = 1,
};

enum rtw89_phy_idx {
	RTW89_PHY_0 = 0,
	RTW89_PHY_1 = 1,
	RTW89_PHY_MAX
};

enum rtw89_rf_path {
	RF_PATH_A = 0,
	RF_PATH_B = 1,
	RF_PATH_C = 2,
	RF_PATH_D = 3,
	RF_PATH_AB,
	RF_PATH_AC,
	RF_PATH_AD,
	RF_PATH_BC,
	RF_PATH_BD,
	RF_PATH_CD,
	RF_PATH_ABC,
	RF_PATH_ABD,
	RF_PATH_ACD,
	RF_PATH_BCD,
	RF_PATH_ABCD,
};

enum rtw89_rf_path_bit {
	RF_A	= BIT(0),
	RF_B	= BIT(1),
	RF_C	= BIT(2),
	RF_D	= BIT(3),

	RF_AB	= (RF_A | RF_B),
	RF_AC	= (RF_A | RF_C),
	RF_AD	= (RF_A | RF_D),
	RF_BC	= (RF_B | RF_C),
	RF_BD	= (RF_B | RF_D),
	RF_CD	= (RF_C | RF_D),

	RF_ABC	= (RF_A | RF_B | RF_C),
	RF_ABD	= (RF_A | RF_B | RF_D),
	RF_ACD	= (RF_A | RF_C | RF_D),
	RF_BCD	= (RF_B | RF_C | RF_D),

	RF_ABCD	= (RF_A | RF_B | RF_C | RF_D),
};

enum rtw89_bandwidth {
	RTW89_CHANNEL_WIDTH_20	= 0,
	RTW89_CHANNEL_WIDTH_40	= 1,
	RTW89_CHANNEL_WIDTH_80	= 2,
	RTW89_CHANNEL_WIDTH_160	= 3,
	RTW89_CHANNEL_WIDTH_80_80	= 4,
	RTW89_CHANNEL_WIDTH_5	= 5,
	RTW89_CHANNEL_WIDTH_10	= 6,
};

#define RTW89_MAX_CHANNEL_WIDTH RTW89_CHANNEL_WIDTH_80
#define RTW89_2G_BW_NUM (RTW89_CHANNEL_WIDTH_40 + 1)
#define RTW89_5G_BW_NUM (RTW89_CHANNEL_WIDTH_80 + 1)
#define RTW89_PPE_BW_NUM (RTW89_CHANNEL_WIDTH_80 + 1)

enum rtw89_ru_bandwidth {
	RTW89_RU26 = 0,
	RTW89_RU52 = 1,
	RTW89_RU106 = 2,
	RTW89_RU_NUM,
};

enum rtw89_sc_offset {
	RTW89_SC_DONT_CARE	= 0,
	RTW89_SC_20_UPPER	= 1,
	RTW89_SC_20_LOWER	= 2,
	RTW89_SC_20_UPMOST	= 3,
	RTW89_SC_20_LOWEST	= 4,
	RTW89_SC_40_UPPER	= 9,
	RTW89_SC_40_LOWER	= 10,
};

struct rtw89_channel_params {
	u8 center_chan;
	u8 primary_chan;
	u8 bandwidth;
	u8 pri_ch_idx;
	u8 cch_by_bw[RTW89_MAX_CHANNEL_WIDTH + 1];
};

struct rtw89_channel_help_params {
	u16 tx_en;
};

struct rtw89_port_reg {
	u32 port_cfg;
	u32 tbtt_prohib;
	u32 bcn_area;
	u32 bcn_early;
	u32 tbtt_early;
	u32 tbtt_agg;
	u32 bcn_space;
	u32 bcn_forcetx;
	u32 bcn_err_cnt;
	u32 bcn_err_flag;
	u32 dtim_ctrl;
	u32 tbtt_shift;
	u32 bcn_cnt_tmr;
	u32 tsftr_l;
	u32 tsftr_h;
};

struct rtw89_txwd_body {
	__le32 dword0;
	__le32 dword1;
	__le32 dword2;
	__le32 dword3;
	__le32 dword4;
	__le32 dword5;
} __packed;

struct rtw89_txwd_info {
	__le32 dword0;
	__le32 dword1;
	__le32 dword2;
	__le32 dword3;
	__le32 dword4;
	__le32 dword5;
} __packed;

struct rtw89_rx_desc_info {
	u16 pkt_size;
	u8 pkt_type;
	u8 drv_info_size;
	u8 shift;
	u8 wl_hd_iv_len;
	bool long_rxdesc;
	bool bb_sel;
	bool mac_info_valid;
	u16 data_rate;
	u8 gi_ltf;
	u8 bw;
	u32 free_run_cnt;
	u8 user_id;
	bool sr_en;
	u8 ppdu_cnt;
	u8 ppdu_type;
	bool icv_err;
	bool crc32_err;
	bool hw_dec;
	bool sw_dec;
	bool addr1_match;
	u8 frag;
	u16 seq;
	u8 frame_type;
	u8 rx_pl_id;
	bool addr_cam_valid;
	u8 addr_cam_id;
	u8 sec_cam_id;
	u8 mac_id;
	u16 offset;
	bool ready;
};

struct rtw89_rxdesc_short {
	__le32 dword0;
	__le32 dword1;
	__le32 dword2;
	__le32 dword3;
} __packed;

struct rtw89_rxdesc_long {
	__le32 dword0;
	__le32 dword1;
	__le32 dword2;
	__le32 dword3;
	__le32 dword4;
	__le32 dword5;
	__le32 dword6;
	__le32 dword7;
} __packed;

struct rtw89_tx_desc_info {
	u16 pkt_size;
	u8 wp_offset;
	u8 qsel;
	u8 ch_dma;
	u8 hdr_llc_len;
	bool is_bmc;
	bool en_wd_info;
	bool wd_page;
	bool use_rate;
	bool dis_data_fb;
	bool tid_indicate;
	bool agg_en;
	u8 ampdu_density;
	u8 ampdu_num;
	bool sec_en;
	u8 sec_type;
	u8 sec_cam_idx;
	u16 data_rate;
	bool fw_dl;
	u16 seq;
};

struct rtw89_core_tx_request {
	enum rtw89_core_tx_type tx_type;

	struct sk_buff *skb;
	struct ieee80211_vif *vif;
	struct ieee80211_sta *sta;
	struct rtw89_tx_desc_info desc_info;
};

struct rtw89_txq {
	struct list_head list;
	unsigned long flags;
};

struct rtw89_mac_ax_gnt {
	u8 gnt_bt_sw_en;
	u8 gnt_bt;
	u8 gnt_wl_sw_en;
	u8 gnt_wl;
};

#define RTW89_MAC_AX_COEX_GNT_NR 2
struct rtw89_mac_ax_coex_gnt {
	struct rtw89_mac_ax_gnt band[RTW89_MAC_AX_COEX_GNT_NR];
};

struct rtw89_btc_ant_info {
	u8 type;  /* shared, dedicated */
	u8 num;
	u8 isolation;

	u8 single_pos: 1;/* Single antenna at S0 or S1 */
	u8 diversity: 1;
};

struct rtw89_btc_dm {
	struct rtw89_mac_ax_coex_gnt gnt;

	u32 wl_only: 1; /* drv->Fw if offload  */
};

struct rtw89_btc_module {
	struct rtw89_btc_ant_info ant;
	u8 rfe_type;
	u8 kt_ver;

	u8 bt_solo: 1;
	u8 bt_pos: 1; /* wl-end view: get from efuse, must compare bt.btg_type */
	u8 switch_type: 1; /* WL/BT switch type: 0: internal, 1: external */

	u8 rsvd;
};

struct rtw89_btc {
	struct rtw89_btc_dm dm;
	struct rtw89_btc_module mdinfo;
};

enum rtw89_ra_mode {
	RTW89_RA_MODE_CCK = BIT(0),
	RTW89_RA_MODE_OFDM = BIT(1),
	RTW89_RA_MODE_HT = BIT(2),
	RTW89_RA_MODE_VHT = BIT(3),
	RTW89_RA_MODE_HE = BIT(4),
};

enum rtw89_ra_report_mode {
	RTW89_RA_RPT_MODE_LEGACY,
	RTW89_RA_RPT_MODE_HT,
	RTW89_RA_RPT_MODE_VHT,
	RTW89_RA_RPT_MODE_HE,
};

enum rtw89_dig_noisy_level {
	RTW89_DIG_NOISY_LEVEL0 = -1,
	RTW89_DIG_NOISY_LEVEL1 = 0,
	RTW89_DIG_NOISY_LEVEL2 = 1,
	RTW89_DIG_NOISY_LEVEL3 = 2,
	RTW89_DIG_NOISY_LEVEL_MAX = 3,
};

enum rtw89_gi_ltf {
	RTW89_GILTF_LGI_4XHE32 = 0,
	RTW89_GILTF_SGI_4XHE08 = 1,
	RTW89_GILTF_2XHE16 = 2,
	RTW89_GILTF_2XHE08 = 3,
	RTW89_GILTF_1XHE16 = 4,
	RTW89_GILTF_1XHE08 = 5,
	RTW89_GILTF_MAX
};

struct rtw89_ra_info {
	u8 is_dis_ra:1;
	/* Bit0 : CCK
	 * Bit1 : OFDM
	 * Bit2 : HT
	 * Bit3 : VHT
	 * Bit4 : HE
	 */
	u8 mode_ctrl:5;
	u8 bw_cap:2;
	u8 macid;
	u8 dcm_cap:1;
	u8 er_cap:1;
	u8 init_rate_lv:2;
	u8 upd_all:1;
	u8 en_sgi:1;
	u8 ldpc_cap:1;
	u8 stbc_cap:1;
	u8 ss_num:3;
	u8 giltf:3;
	u8 upd_bw_nss_mask:1;
	u8 upd_mask:1;
	u64 ra_mask;
};

#define RTW89_PPDU_MAX_USR 4
#define RTW89_PPDU_MAC_INFO_USR_SIZE 4
#define RTW89_PPDU_MAC_INFO_SIZE 8
#define RTW89_PPDU_MAC_RX_CNT_SIZE 96

#define RTW89_MAX_AGG_NUM 128

struct rtw89_ampdu_params {
	u16 agg_num;
	bool amsdu;
};

struct rtw89_ra_report {
	struct rate_info txrate;
	u32 bit_rate;
};

DECLARE_EWMA(rssi, 10, 16);

struct rtw89_sta {
	u8 mac_id;
	struct rtw89_ra_info ra;
	struct rtw89_ra_report ra_report;
	struct ewma_rssi avg_rssi;
	struct rtw89_ampdu_params ampdu_params[IEEE80211_NUM_TIDS];
};

#define RTW89_MAX_ADDR_CAM_NUM		128
#define RTW89_MAX_BSSID_CAM_NUM		20
#define RTW89_MAX_SEC_CAM_NUM		128
#define RTW89_SEC_CAM_IN_ADDR_CAM	7

struct rtw89_addr_cam_entry {
	u8 addr_cam_idx;
	u8 offset;
	u8 len;
	u8 valid	: 1;
	u8 addr_mask	: 6;
	u8 wapi		: 1;
	u8 mask_sel	: 2;
	u8 bssid_cam_idx: 6;
	u8 tma[ETH_ALEN];
	u8 sma[ETH_ALEN];

	u8 sec_ent_mode;
	DECLARE_BITMAP(sec_cam_map, RTW89_SEC_CAM_IN_ADDR_CAM);
	u8 sec_ent_keyid[RTW89_SEC_CAM_IN_ADDR_CAM];
	u8 sec_ent[RTW89_SEC_CAM_IN_ADDR_CAM];
	struct rtw89_sec_cam_entry *sec_entries[RTW89_SEC_CAM_IN_ADDR_CAM];
};

struct rtw89_bssid_cam_entry {
	u8 bssid[ETH_ALEN];
	u8 phy_idx;
	u8 bssid_cam_idx;
	u8 offset;
	u8 len;
	u8 valid : 1;
	u8 num;
};

struct rtw89_sec_cam_entry {
	u8 sec_cam_idx;
	u8 offset;
	u8 len;
	u8 type : 4;
	u8 ext_key : 1;
	u8 spp_mode : 1;
	/* 256 bits */
	u8 key[32];
};

struct rtw89_efuse {
	bool valid;
	u8 xtal_cap;
	u8 addr[ETH_ALEN];
	u8 rfe_type;
	char country_code[2];
};

struct rtw89_vif {
	u8 mac_id;
	u8 port;
	u8 mac_addr[ETH_ALEN];
	u8 bssid[ETH_ALEN];
	u8 phy_idx;
	u8 mac_idx;
	u8 net_type;
	u8 wifi_role;
	u8 self_role;
	u8 wmm;
	u8 bcn_hit_cond;
	u8 hit_rule;
	bool trigger;
	bool lsig_txop;
	u8 tgt_ind;
	u8 frm_tgt_ind;
	bool wowlan_pattern;
	bool wowlan_uc;
	bool wowlan_magic;
	bool is_hesta;
	union {
		struct {
			struct ieee80211_sta *ap;
		} mgd;
		struct {
			struct list_head sta_list;
		} ap;
	};
	struct rtw89_addr_cam_entry addr_cam;
	struct rtw89_bssid_cam_entry bssid_cam;
	struct ieee80211_tx_queue_params tx_params[IEEE80211_NUM_ACS];
};

struct rtw89_hci_ops {
	int (*tx_write)(struct rtw89_dev *rtwdev, struct rtw89_core_tx_request *tx_req);
	void (*tx_kick_off)(struct rtw89_dev *rtwdev, u8 txch);
	void (*reset)(struct rtw89_dev *rtwdev);
	int (*start)(struct rtw89_dev *rtwdev);
	void (*stop)(struct rtw89_dev *rtwdev);
	void (*link_ps)(struct rtw89_dev *rtwdev, bool enter);

	u8 (*read8)(struct rtw89_dev *rtwdev, u32 addr);
	u16 (*read16)(struct rtw89_dev *rtwdev, u32 addr);
	u32 (*read32)(struct rtw89_dev *rtwdev, u32 addr);
	void (*write8)(struct rtw89_dev *rtwdev, u32 addr, u8 data);
	void (*write16)(struct rtw89_dev *rtwdev, u32 addr, u16 data);
	void (*write32)(struct rtw89_dev *rtwdev, u32 addr, u32 data);

	int (*mac_pre_init)(struct rtw89_dev *rtwdev);
	int (*mac_post_init)(struct rtw89_dev *rtwdev);
	int (*deinit)(struct rtw89_dev *rtwdev);

	u32 (*check_and_reclaim_tx_resource)(struct rtw89_dev *rtwdev, u8 txch);
	int (*mac_lv1_rcvy)(struct rtw89_dev *rtwdev, u8 step);
};

struct rtw89_hci_info {
	const struct rtw89_hci_ops *ops;
	enum rtw89_hci_type type;
};

struct rtw89_chip_ops {
	void (*bb_reset)(struct rtw89_dev *rtwdev,
			 enum rtw89_phy_idx phy_idx);
	void (*bb_sethw)(struct rtw89_dev *rtwdev);
	u32 (*read_rf)(struct rtw89_dev *rtwdev, enum rtw89_rf_path rf_path,
		       u32 addr, u32 mask);
	bool (*write_rf)(struct rtw89_dev *rtwdev, enum rtw89_rf_path rf_path,
			 u32 addr, u32 mask, u32 data);
	void (*set_channel)(struct rtw89_dev *rtwdev,
			    struct rtw89_channel_params *param);
	void (*set_channel_help)(struct rtw89_dev *rtwdev, bool enter,
				 struct rtw89_channel_help_params *p);
	int (*read_efuse)(struct rtw89_dev *rtwdev, u8 *log_map);
	int (*read_phycap)(struct rtw89_dev *rtwdev, u8 *phycap_map);
	void (*fem_setup)(struct rtw89_dev *rtwdev);
	void (*rfk_init)(struct rtw89_dev *rtwdev);
	void (*rfk_channel)(struct rtw89_dev *rtwdev);
	void (*rfk_track)(struct rtw89_dev *rtwdev);
	void (*power_trim)(struct rtw89_dev *rtwdev);
	void (*set_txpwr)(struct rtw89_dev *rtwdev);
	void (*set_txpwr_ctrl)(struct rtw89_dev *rtwdev);
	int (*init_txpwr_unit)(struct rtw89_dev *rtwdev, enum rtw89_phy_idx phy_idx);
	u8 (*get_thermal)(struct rtw89_dev *rtwdev, enum rtw89_rf_path rf_path);
	void (*ctrl_btg)(struct rtw89_dev *rtwdev, bool btg);
	void (*query_ppdu)(struct rtw89_dev *rtwdev,
			   struct rtw89_rx_phy_ppdu *phy_ppdu,
			   struct ieee80211_rx_status *status);

	void (*btc_set_rfe)(struct rtw89_dev *rtwdev);
	void (*btc_init_cfg)(struct rtw89_dev *rtwdev);
	void (*btc_set_wl_pri)(struct rtw89_dev *rtwdev, u8 map, bool state);
};

enum rtw89_dma_ch {
	RTW89_DMA_ACH0 = 0,
	RTW89_DMA_ACH1 = 1,
	RTW89_DMA_ACH2 = 2,
	RTW89_DMA_ACH3 = 3,
	RTW89_DMA_ACH4 = 4,
	RTW89_DMA_ACH5 = 5,
	RTW89_DMA_ACH6 = 6,
	RTW89_DMA_ACH7 = 7,
	RTW89_DMA_B0MG = 8,
	RTW89_DMA_B0HI = 9,
	RTW89_DMA_B1MG = 10,
	RTW89_DMA_B1HI = 11,
	RTW89_DMA_H2C = 12,
	RTW89_DMA_CH_NUM = 13
};

enum rtw89_qta_mode {
	RTW89_QTA_SCC,
	RTW89_QTA_DBCC,
	RTW89_QTA_SCC_WD128,
	RTW89_QTA_DBCC_WD128,
	RTW89_QTA_SCC_STF,
	RTW89_QTA_DBCC_STF,
	RTW89_QTA_SU_TP,
	RTW89_QTA_DLFW,
	RTW89_QTA_BCN_TEST,
	RTW89_QTA_LAMODE,

	/* keep last */
	RTW89_QTA_INVALID,
};

struct rtw89_hfc_ch_cfg {
	u16 min;
	u16 max;
#define grp_0 0
#define grp_1 1
#define grp_num 2
	u8 grp;
};

struct rtw89_hfc_ch_info {
	u16 aval;
	u16 used;
};

struct rtw89_hfc_pub_cfg {
	u16 grp0;
	u16 grp1;
	u16 pub_max;
	u16 wp_thrd;
};

struct rtw89_hfc_pub_info {
	u16 g0_used;
	u16 g1_used;
	u16 g0_aval;
	u16 g1_aval;
	u16 pub_aval;
	u16 wp_aval;
};

struct rtw89_hfc_prec_cfg {
	u16 ch011_prec;
	u16 h2c_prec;
	u16 wp_ch07_prec;
	u16 wp_ch811_prec;
	u8 ch011_full_cond;
	u8 h2c_full_cond;
	u8 wp_ch07_full_cond;
	u8 wp_ch811_full_cond;
};

struct rtw89_hfc_param {
	bool en;
	bool h2c_en;
	u8 mode;
	struct rtw89_hfc_ch_cfg *ch_cfg;
	struct rtw89_hfc_ch_info ch_info[RTW89_DMA_CH_NUM];
	struct rtw89_hfc_pub_cfg *pub_cfg;
	struct rtw89_hfc_pub_info pub_info;
	struct rtw89_hfc_prec_cfg *prec_cfg;
};

struct rtw89_hfc_param_ini {
	struct rtw89_hfc_ch_cfg *ch_cfg;
	struct rtw89_hfc_pub_cfg *pub_cfg;
	struct rtw89_hfc_prec_cfg *prec_cfg;
	u8 mode;
};

struct rtw89_dle_size {
	u16 pge_size;
	u16 lnk_pge_num;
	u16 unlnk_pge_num;
};

struct rtw89_wde_quota {
	u16 hif;
	u16 wcpu;
	u16 pkt_in;
	u16 cpu_io;
};

struct rtw89_ple_quota {
	u16 cma0_tx;
	u16 cma1_tx;
	u16 c2h;
	u16 h2c;
	u16 wcpu;
	u16 mpdu_proc;
	u16 cma0_dma;
	u16 cma1_dma;
	u16 bb_rpt;
	u16 wd_rel;
	u16 cpu_io;
};

struct rtw89_dle_mem {
	enum rtw89_qta_mode mode;
	struct rtw89_dle_size *wde_size;
	struct rtw89_dle_size *ple_size;
	struct rtw89_wde_quota *wde_min_qt;
	struct rtw89_wde_quota *wde_max_qt;
	struct rtw89_ple_quota *ple_min_qt;
	struct rtw89_ple_quota *ple_max_qt;
};

struct rtw89_reg_def {
	u32 addr;
	u32 mask;
};

struct rtw89_reg2_def {
	u32 addr;
	u32 data;
};

struct rtw89_reg3_def {
	u32 addr;
	u32 mask;
	u32 data;
};

struct rtw89_reg5_def {
	u8 flag; /* recognized by parsers */
	u8 path;
	u32 addr;
	u32 mask;
	u32 data;
};

struct rtw89_phy_table {
	const struct rtw89_reg2_def *regs;
	u32 n_regs;
	enum rtw89_rf_path rf_path;
};

struct rtw89_txpwr_table {
	const void *data;
	u32 size;
	void (*load)(struct rtw89_dev *rtwdev,
		     const struct rtw89_txpwr_table *tbl);
};

struct rtw89_chip_info {
	enum rtw89_core_chip_id chip_id;
	const struct rtw89_chip_ops *ops;
	const char *fw_name;
	u32 fifo_size;
	u32 dle_lamode_size;
	u16 max_amsdu_limit;
	struct rtw89_hfc_param_ini *hfc_param_ini;
	struct rtw89_dle_mem *dle_mem;
	u32 rf_base_addr[2];
	u8 rf_path_num;
	u8 tx_nss;
	u8 rx_nss;
	u8 acam_num;
	u8 bcam_num;
	u8 scam_num;

	u8 sec_ctrl_efuse_size;
	u32 physical_efuse_size;
	u32 logical_efuse_size;
	u32 limit_efuse_size;
	u32 phycap_addr;
	u32 phycap_size;

	const struct rtw89_phy_table *bb_table;
	const struct rtw89_phy_table *rf_table[RF_PATH_MAX];
	const struct rtw89_phy_table *nctl_table;
	const struct rtw89_txpwr_table *byr_table;
	const struct rtw89_phy_dig_gain_table *dig_table;
	const s8 (*txpwr_lmt_2g)[RTW89_2G_BW_NUM][RTW89_NTX_NUM]
				[RTW89_RS_LMT_NUM][RTW89_BF_NUM]
				[RTW89_REGD_NUM][RTW89_2G_CH_NUM];
	const s8 (*txpwr_lmt_5g)[RTW89_5G_BW_NUM][RTW89_NTX_NUM]
				[RTW89_RS_LMT_NUM][RTW89_BF_NUM]
				[RTW89_REGD_NUM][RTW89_5G_CH_NUM];
	const s8 (*txpwr_lmt_ru_2g)[RTW89_RU_NUM][RTW89_NTX_NUM]
				   [RTW89_REGD_NUM][RTW89_2G_CH_NUM];
	const s8 (*txpwr_lmt_ru_5g)[RTW89_RU_NUM][RTW89_NTX_NUM]
				   [RTW89_REGD_NUM][RTW89_5G_CH_NUM];

	u8 txpwr_factor_rf;
	u8 txpwr_factor_mac;
};

enum rtw89_hcifc_mode {
	RTW89_HCIFC_POH = 0,
	RTW89_HCIFC_STF = 1,
	RTW89_HCIFC_SDIO = 2,

	/* keep last */
	RTW89_HCIFC_MODE_INVALID,
};

struct rtw89_dle_info {
	enum rtw89_qta_mode qta_mode;
	u16 wde_pg_size;
	u16 ple_pg_size;
	u16 c0_rx_qta;
	u16 c1_rx_qta;
};

enum rtw89_host_rpr_mode {
	RTW89_RPR_MODE_POH = 0,
	RTW89_RPR_MODE_STF
};

struct rtw89_mac_info {
	struct rtw89_dle_info dle_info;
	struct rtw89_hfc_param hfc_param;
	enum rtw89_qta_mode qta_mode;
	u8 rpwm_seq_num;
};

struct rtw89_fw_info {
	const struct firmware *firmware;
	struct rtw89_dev *rtwdev;
	struct completion completion;
	u8 major_ver;
	u8 minor_ver;
	u8 sub_ver;
	u8 sub_idex;
	u16 build_year;
	u16 build_mon;
	u16 build_date;
	u16 build_hour;
	u16 build_min;
	u8 cmd_ver;
	u8 h2c_seq;
	u8 rec_seq;
};

struct rtw89_cam_info {
	DECLARE_BITMAP(addr_cam_map, RTW89_MAX_ADDR_CAM_NUM);
	DECLARE_BITMAP(bssid_cam_map, RTW89_MAX_BSSID_CAM_NUM);
	DECLARE_BITMAP(sec_cam_map, RTW89_MAX_SEC_CAM_NUM);
};

struct rtw89_hal {
	u32 rx_fltr;
	u8 cut_version;
	u8 current_channel;
	u8 current_primary_channel;
	enum rtw89_subband current_subband;
	u8 current_band_width;
	u8 current_band_type;
	/* center channel for different available bandwidth,
	 * val of (bw > current_band_width) is invalid
	 */
	u8 cch_by_bw[RTW89_MAX_CHANNEL_WIDTH + 1];
	u32 sw_amsdu_max_size;
};

#define RTW89_MAX_HW_PORT_NUM 5
#define RTW89_MAX_MAC_ID_NUM 128

enum rtw89_flags {
	RTW89_FLAG_POWERON,
	RTW89_FLAG_FW_RDY,
	RTW89_FLAG_RUNNING,

	NUM_OF_RTW89_FLAGS,
};

DECLARE_EWMA(thermal, 4, 4);

struct rtw89_phy_stat {
	struct ewma_thermal avg_thermal[RF_PATH_MAX];
};

#define RTW89_DACK_PATH_NR 2
#define RTW89_DACK_IDX_NR 2
#define RTW89_DACK_MSBK_NR 16
struct rtw89_dack_info {
	bool dack_done;
	u8 msbk_d[RTW89_DACK_PATH_NR][RTW89_DACK_IDX_NR][RTW89_DACK_MSBK_NR];
	u8 dadck_d[RTW89_DACK_PATH_NR][RTW89_DACK_IDX_NR];
	u16 addck_d[RTW89_DACK_PATH_NR][RTW89_DACK_IDX_NR];
	u16 biask_d[RTW89_DACK_PATH_NR][RTW89_DACK_IDX_NR];
	u32 dack_cnt;
	bool addck_timeout[RTW89_DACK_PATH_NR];
	bool dadck_timeout[RTW89_DACK_PATH_NR];
	bool msbk_timeout[RTW89_DACK_PATH_NR];
};

#define RTW89_IQK_CHS_NR 2
#define RTW89_IQK_PATH_NR 4
struct rtw89_iqk_info {
	bool lok_cor_fail[RTW89_IQK_CHS_NR][RTW89_IQK_PATH_NR];
	bool lok_fin_fail[RTW89_IQK_CHS_NR][RTW89_IQK_PATH_NR];
	bool iqk_tx_fail[RTW89_IQK_CHS_NR][RTW89_IQK_PATH_NR];
	bool iqk_rx_fail[RTW89_IQK_CHS_NR][RTW89_IQK_PATH_NR];
	u32 iqk_fail_cnt;
	bool is_iqk_init;
	u32 iqk_channel[RTW89_IQK_CHS_NR];
	u8 iqk_band[RTW89_IQK_PATH_NR];
	u8 iqk_ch[RTW89_IQK_PATH_NR];
	u8 iqk_bw[RTW89_IQK_PATH_NR];
	u8 kcount;
	u8 iqk_times;
	u8 version;
	u32 nb_txcfir[RTW89_IQK_PATH_NR];
	u32 nb_rxcfir[RTW89_IQK_PATH_NR];
	u32 bp_txkresult[RTW89_IQK_PATH_NR];
	u32 bp_rxkresult[RTW89_IQK_PATH_NR];
	u32 bp_iqkenable[RTW89_IQK_PATH_NR];
	bool is_wb_txiqk[RTW89_IQK_PATH_NR];
	bool is_wb_rxiqk[RTW89_IQK_PATH_NR];
	bool is_nbiqk;
	bool iqk_fft_en;
	bool iqk_xym_en;
	bool iqk_sram_en;
	bool iqk_cfir_en;
	u8 thermal[RTW89_IQK_PATH_NR];
	bool thermal_rek_en;
};

#define RTW89_DPK_RF_PATH 2
#define RTW89_DPK_AVG_THERMAL_NUM 8
#define RTW89_DPK_BKUP_NUM 2
struct rtw89_dpk_bkup_para {
	enum rtw89_band band;
	enum rtw89_bandwidth bw;
	u8 ch;
	bool path_ok;
	u8 txagc_dpk;
	u8 ther_dpk;
	u8 gs;
	u16 pwsf;
};

struct rtw89_dpk_info {
	bool is_dpk_enable;
	u16 dc_i[RTW89_DPK_RF_PATH];
	u16 dc_q[RTW89_DPK_RF_PATH];
	u8 corr_val[RTW89_DPK_RF_PATH];
	u8 corr_idx[RTW89_DPK_RF_PATH];
	u8 cur_idx[RTW89_DPK_RF_PATH];
	struct rtw89_dpk_bkup_para bp[RTW89_DPK_RF_PATH][RTW89_DPK_BKUP_NUM];
};

struct rtw89_fem_info {
	bool elna_2g;
	bool elna_5g;
	bool epa_2g;
	bool epa_5g;
};

struct rtw89_phy_ch_info {
	u8 rssi_min;
	u16 rssi_min_macid;
	u8 pre_rssi_min;
	u8 rssi_max;
	u16 rssi_max_macid;
	u8 rxsc_160;
	u8 rxsc_80;
	u8 rxsc_40;
	u8 rxsc_20;
	u8 rxsc_l;
	u8 is_noisy;
};

struct rtw89_agc_gaincode_set {
	u8 lna_idx;
	u8 tia_idx;
	u8 rxb_idx;
};

#define IGI_RSSI_TH_NUM 5
#define FA_TH_NUM 4
#define LNA_GAIN_NUM 7
#define TIA_GAIN_NUM 2
struct rtw89_dig_info {
	struct rtw89_agc_gaincode_set cur_gaincode;
	enum rtw89_dig_noisy_level cur_noisy_lv;
	bool force_gaincode_idx_en;
	struct rtw89_agc_gaincode_set force_gaincode;
	u8 igi_rssi_th[IGI_RSSI_TH_NUM];
	u8 fa_th[FA_TH_NUM];
	u8 igi_rssi;
	u8 igi_fa_rssi;
	u8 fa_rssi_ofst;
	u8 dyn_igi_max;
	u8 dyn_igi_min;
	bool dyn_pd_th_en;
	u8 dyn_pd_th_max;
	u8 pd_low_th_ofst;
	u8 ib_pbk;
	s8 ib_pkpwr;
	s8 lna_gain_a[LNA_GAIN_NUM];
	s8 lna_gain_g[LNA_GAIN_NUM];
	s8 *lna_gain;
	s8 tia_gain_a[TIA_GAIN_NUM];
	s8 tia_gain_g[TIA_GAIN_NUM];
	s8 *tia_gain;
	bool reset;
};

struct rtw89_cfo_tracking_info {
	bool is_adjust;
	bool apply_compensation;
	u8 crystal_cap;
	u8 crystal_cap_default;
	u8 def_x_cap;
	s32 cfo_tail[CFO_TRACK_MAX_USER];
	u16 cfo_cnt[CFO_TRACK_MAX_USER];
	s32 cfo_avg_pre;
	u32 packet_count;
	u32 packet_count_pre;
	s32 residual_cfo_acc;
};

/* 2GL, 2GH, 5GL1, 5GH1, 5GM1, 5GM2, 5GH1, 5GH2 */
#define TSSI_TRIM_CH_GROUP_NUM 8

#define TSSI_CCK_CH_GROUP_NUM 6
#define TSSI_MCS_2G_CH_GROUP_NUM 5
#define TSSI_MCS_5G_CH_GROUP_NUM 14
#define TSSI_MCS_CH_GROUP_NUM \
	(TSSI_MCS_2G_CH_GROUP_NUM + TSSI_MCS_5G_CH_GROUP_NUM)

struct rtw89_tssi_info {
	u8 thermal[RF_PATH_MAX];
	s8 tssi_trim[RF_PATH_MAX][TSSI_TRIM_CH_GROUP_NUM];
	s8 tssi_cck[RF_PATH_MAX][TSSI_CCK_CH_GROUP_NUM];
	s8 tssi_mcs[RF_PATH_MAX][TSSI_MCS_CH_GROUP_NUM];
};

struct rtw89_power_trim_info {
	bool pg_thermal_trim;
	bool pg_pa_bias_trim;
	u8 thermal_trim[RF_PATH_MAX];
	u8 pa_bias_trim[RF_PATH_MAX];
};

struct rtw89_regulatory {
	char alpha2[3];
	u8 txpwr_regd[RTW89_BAND_MAX];
};

enum rtw89_ifs_clm_application {
	RTW89_IFS_CLM_INIT = 0,
	RTW89_IFS_CLM_BACKGROUND = 1,
	RTW89_IFS_CLM_ACS = 2,
	RTW89_IFS_CLM_DIG = 3,
	RTW89_IFS_CLM_TDMA_DIG = 4,
	RTW89_IFS_CLM_DBG = 5,
	RTW89_IFS_CLM_DBG_MANUAL = 6
};

enum rtw89_env_racing_lv {
	RTW89_RAC_RELEASE = 0,
	RTW89_RAC_LV_1 = 1,
	RTW89_RAC_LV_2 = 2,
	RTW89_RAC_LV_3 = 3,
	RTW89_RAC_LV_4 = 4,
	RTW89_RAC_MAX_NUM = 5
};

struct rtw89_ccx_para_info {
	enum rtw89_env_racing_lv rac_lv;
	u16 mntr_time;
	u8 nhm_manual_th_ofst;
	u8 nhm_manual_th0;
	enum rtw89_ifs_clm_application ifs_clm_app;
	u32 ifs_clm_manual_th_times;
	u32 ifs_clm_manual_th0;
	u8 fahm_manual_th_ofst;
	u8 fahm_manual_th0;
	u8 fahm_numer_opt;
	u8 fahm_denom_opt;
};

enum rtw89_ccx_edcca_opt_sc_idx {
	RTW89_CCX_EDCCA_SEG0_P0 = 0,
	RTW89_CCX_EDCCA_SEG0_S1 = 1,
	RTW89_CCX_EDCCA_SEG0_S2 = 2,
	RTW89_CCX_EDCCA_SEG0_S3 = 3,
	RTW89_CCX_EDCCA_SEG1_P0 = 4,
	RTW89_CCX_EDCCA_SEG1_S1 = 5,
	RTW89_CCX_EDCCA_SEG1_S2 = 6,
	RTW89_CCX_EDCCA_SEG1_S3 = 7
};

enum rtw89_ccx_edcca_opt_bw_idx {
	RTW89_CCX_EDCCA_BW20_0 = 0,
	RTW89_CCX_EDCCA_BW20_1 = 1,
	RTW89_CCX_EDCCA_BW20_2 = 2,
	RTW89_CCX_EDCCA_BW20_3 = 3,
	RTW89_CCX_EDCCA_BW20_4 = 4,
	RTW89_CCX_EDCCA_BW20_5 = 5,
	RTW89_CCX_EDCCA_BW20_6 = 6,
	RTW89_CCX_EDCCA_BW20_7 = 7
};

#define RTW89_NHM_TH_NUM 11
#define RTW89_FAHM_TH_NUM 11
#define RTW89_NHM_RPT_NUM 12
#define RTW89_FAHM_RPT_NUM 12
#define RTW89_IFS_CLM_NUM 4
struct rtw89_env_monitor_info {
	u32 ccx_trigger_time;
	u64 start_time;
	u8 ccx_rpt_stamp;
	u8 ccx_watchdog_result;
	bool ccx_ongoing;
	u8 ccx_rac_lv;
	bool ccx_manual_ctrl;
	u8 ccx_pre_rssi;
	u16 clm_mntr_time;
	u16 nhm_mntr_time;
	u16 ifs_clm_mntr_time;
	enum rtw89_ifs_clm_application ifs_clm_app;
	u16 fahm_mntr_time;
	u16 edcca_clm_mntr_time;
	u16 ccx_period;
	u8 ccx_unit_idx;
	enum rtw89_ccx_edcca_opt_bw_idx ccx_edcca_opt_bw_idx;
	u8 nhm_th[RTW89_NHM_TH_NUM];
	u16 ifs_clm_th_l[RTW89_IFS_CLM_NUM];
	u16 ifs_clm_th_h[RTW89_IFS_CLM_NUM];
	u8 fahm_numer_opt;
	u8 fahm_denom_opt;
	u8 fahm_th[RTW89_FAHM_TH_NUM];
	u16 clm_result;
	u16 nhm_result[RTW89_NHM_RPT_NUM];
	u8 nhm_wgt[RTW89_NHM_RPT_NUM];
	u16 nhm_tx_cnt;
	u16 nhm_cca_cnt;
	u16 nhm_idle_cnt;
	u16 ifs_clm_tx;
	u16 ifs_clm_edcca_excl_cca;
	u16 ifs_clm_ofdmfa;
	u16 ifs_clm_ofdmcca_excl_fa;
	u16 ifs_clm_cckfa;
	u16 ifs_clm_cckcca_excl_fa;
	u16 ifs_clm_total_ifs;
	u8 ifs_clm_his[RTW89_IFS_CLM_NUM];
	u16 ifs_clm_avg[RTW89_IFS_CLM_NUM];
	u16 ifs_clm_cca[RTW89_IFS_CLM_NUM];
	u16 fahm_result[RTW89_FAHM_RPT_NUM];
	u16 fahm_denom_result;
	u16 edcca_clm_result;
	u8 clm_ratio;
	u8 nhm_rpt[RTW89_NHM_RPT_NUM];
	u8 nhm_tx_ratio;
	u8 nhm_cca_ratio;
	u8 nhm_idle_ratio;
	u8 nhm_ratio;
	u16 nhm_result_sum;
	u8 nhm_pwr;
	u8 ifs_clm_tx_ratio;
	u8 ifs_clm_edcca_excl_cca_ratio;
	u8 ifs_clm_cck_fa_ratio;
	u8 ifs_clm_ofdm_fa_ratio;
	u8 ifs_clm_cck_cca_excl_fa_ratio;
	u8 ifs_clm_ofdm_cca_excl_fa_ratio;
	u16 ifs_clm_cck_fa_permil;
	u16 ifs_clm_ofdm_fa_permil;
	u32 ifs_clm_ifs_avg[RTW89_IFS_CLM_NUM];
	u32 ifs_clm_cca_avg[RTW89_IFS_CLM_NUM];
	u8 fahm_rpt[RTW89_FAHM_RPT_NUM];
	u16 fahm_result_sum;
	u8 fahm_ratio;
	u8 fahm_denom_ratio;
	u8 fahm_pwr;
	u8 edcca_clm_ratio;
};

enum rtw89_ser_rcvy_step {
	RTW89_SER_DRV_STOP_TX,
	RTW89_SER_DRV_STOP_RX,
	RTW89_SER_DRV_STOP_RUN,
	RTW89_SER_HAL_STOP_DMA,
	RTW89_NUM_OF_SER_FLAGS
};

struct rtw89_ser {
	u8 state;
	u8 alarm_event;

	struct work_struct ser_hdl_work;
	struct delayed_work ser_alarm_work;
	struct state_ent *st_tbl;
	struct event_ent *ev_tbl;
	struct list_head msg_q;
	spinlock_t msg_q_lock; /* lock when read/write ser msg */
	DECLARE_BITMAP(flags, RTW89_NUM_OF_SER_FLAGS);
};

struct rtw89_ppdu_sts_info {
	struct sk_buff_head rx_queue[RTW89_PHY_MAX];
	u8 curr_rx_ppdu_cnt[RTW89_PHY_MAX];
};

struct rtw89_dev {
	struct ieee80211_hw *hw;
	struct device *dev;

	bool dbcc_en;
	const struct rtw89_chip_info *chip;
	struct rtw89_hal hal;
	struct rtw89_mac_info mac;
	struct rtw89_fw_info fw;
	struct rtw89_hci_info hci;
	struct rtw89_efuse efuse;

	/* ensures exclusive access from mac80211 callbacks */
	struct mutex mutex;
	/* used to protect rf read write */
	struct mutex rf_mutex;
	struct tasklet_struct txq_tasklet;
	/* used to protect ba_list */
	spinlock_t ba_lock;
	/* txqs to setup ba session */
	struct list_head ba_list;
	struct work_struct ba_work;

	struct rtw89_cam_info cam_info;

	struct sk_buff_head c2h_queue;
	struct work_struct c2h_work;

	struct rtw89_ser ser;

	DECLARE_BITMAP(hw_port, RTW89_MAX_HW_PORT_NUM);
	DECLARE_BITMAP(mac_id_map, RTW89_MAX_MAC_ID_NUM);
	DECLARE_BITMAP(flags, NUM_OF_RTW89_FLAGS);

	struct rtw89_phy_stat phystat;
	struct rtw89_dack_info dack;
	struct rtw89_iqk_info iqk;
	struct rtw89_dpk_info dpk;
	bool is_tssi_mode[RF_PATH_MAX];

	struct rtw89_fem_info fem;
	struct rtw89_txpwr_byrate byr[RTW89_BAND_MAX];
	struct rtw89_tssi_info tssi;
	struct rtw89_power_trim_info pwr_trim;

	struct rtw89_cfo_tracking_info cfo_tracking;
	struct rtw89_env_monitor_info env_monitor;
	struct rtw89_dig_info dig;
	struct rtw89_phy_ch_info ch_info;
	struct delayed_work track_work;
	struct rtw89_ppdu_sts_info ppdu_sts;
	u8 total_sta_assoc;

	const struct rtw89_regulatory *regd;

	struct rtw89_btc btc;

	/* HCI related data, keep last */
	u8 priv[0] __aligned(sizeof(void *));
};

static inline int rtw89_hci_tx_write(struct rtw89_dev *rtwdev,
				     struct rtw89_core_tx_request *tx_req)
{
	return rtwdev->hci.ops->tx_write(rtwdev, tx_req);
}

static inline void rtw89_hci_reset(struct rtw89_dev *rtwdev)
{
	rtwdev->hci.ops->reset(rtwdev);
}

static inline int rtw89_hci_start(struct rtw89_dev *rtwdev)
{
	return rtwdev->hci.ops->start(rtwdev);
}

static inline void rtw89_hci_stop(struct rtw89_dev *rtwdev)
{
	rtwdev->hci.ops->stop(rtwdev);
}

static inline int rtw89_hci_deinit(struct rtw89_dev *rtwdev)
{
	return rtwdev->hci.ops->deinit(rtwdev);
}

static inline void rtw89_hci_link_ps(struct rtw89_dev *rtwdev, bool enter)
{
	rtwdev->hci.ops->link_ps(rtwdev, enter);
}

static inline u32 rtw89_hci_check_and_reclaim_tx_resource(struct rtw89_dev *rtwdev, u8 txch)
{
	return rtwdev->hci.ops->check_and_reclaim_tx_resource(rtwdev, txch);
}

static inline void rtw89_hci_tx_kick_off(struct rtw89_dev *rtwdev, u8 txch)
{
	return rtwdev->hci.ops->tx_kick_off(rtwdev, txch);
}

static inline u8 rtw89_read8(struct rtw89_dev *rtwdev, u32 addr)
{
	return rtwdev->hci.ops->read8(rtwdev, addr);
}

static inline u16 rtw89_read16(struct rtw89_dev *rtwdev, u32 addr)
{
	return rtwdev->hci.ops->read16(rtwdev, addr);
}

static inline u32 rtw89_read32(struct rtw89_dev *rtwdev, u32 addr)
{
	return rtwdev->hci.ops->read32(rtwdev, addr);
}

static inline void rtw89_write8(struct rtw89_dev *rtwdev, u32 addr, u8 data)
{
	rtwdev->hci.ops->write8(rtwdev, addr, data);
}

static inline void rtw89_write16(struct rtw89_dev *rtwdev, u32 addr, u16 data)
{
	rtwdev->hci.ops->write16(rtwdev, addr, data);
}

static inline void rtw89_write32(struct rtw89_dev *rtwdev, u32 addr, u32 data)
{
	rtwdev->hci.ops->write32(rtwdev, addr, data);
}

static inline void
rtw89_write8_set(struct rtw89_dev *rtwdev, u32 addr, u8 bit)
{
	u8 val;

	val = rtw89_read8(rtwdev, addr);
	rtw89_write8(rtwdev, addr, val | bit);
}

static inline void
rtw89_write16_set(struct rtw89_dev *rtwdev, u32 addr, u16 bit)
{
	u16 val;

	val = rtw89_read16(rtwdev, addr);
	rtw89_write16(rtwdev, addr, val | bit);
}

static inline void
rtw89_write32_set(struct rtw89_dev *rtwdev, u32 addr, u32 bit)
{
	u32 val;

	val = rtw89_read32(rtwdev, addr);
	rtw89_write32(rtwdev, addr, val | bit);
}

static inline void
rtw89_write8_clr(struct rtw89_dev *rtwdev, u32 addr, u8 bit)
{
	u8 val;

	val = rtw89_read8(rtwdev, addr);
	rtw89_write8(rtwdev, addr, val & ~bit);
}

static inline void
rtw89_write16_clr(struct rtw89_dev *rtwdev, u32 addr, u16 bit)
{
	u16 val;

	val = rtw89_read16(rtwdev, addr);
	rtw89_write16(rtwdev, addr, val & ~bit);
}

static inline void
rtw89_write32_clr(struct rtw89_dev *rtwdev, u32 addr, u32 bit)
{
	u32 val;

	val = rtw89_read32(rtwdev, addr);
	rtw89_write32(rtwdev, addr, val & ~bit);
}

static inline u32
rtw89_read32_mask(struct rtw89_dev *rtwdev, u32 addr, u32 mask)
{
	u32 shift = __ffs(mask);
	u32 orig;
	u32 ret;

	orig = rtw89_read32(rtwdev, addr);
	ret = (orig & mask) >> shift;

	return ret;
}

static inline u16
rtw89_read16_mask(struct rtw89_dev *rtwdev, u32 addr, u32 mask)
{
	u32 shift = __ffs(mask);
	u32 orig;
	u32 ret;

	orig = rtw89_read16(rtwdev, addr);
	ret = (orig & mask) >> shift;

	return ret;
}

static inline u8
rtw89_read8_mask(struct rtw89_dev *rtwdev, u32 addr, u32 mask)
{
	u32 shift = __ffs(mask);
	u32 orig;
	u32 ret;

	orig = rtw89_read8(rtwdev, addr);
	ret = (orig & mask) >> shift;

	return ret;
}

static inline void
rtw89_write32_mask(struct rtw89_dev *rtwdev, u32 addr, u32 mask, u32 data)
{
	u32 shift = __ffs(mask);
	u32 orig;
	u32 set;

	WARN(addr & 0x3, "should be 4-byte aligned, addr = 0x%08x\n", addr);

	orig = rtw89_read32(rtwdev, addr);
	set = (orig & ~mask) | ((data << shift) & mask);
	rtw89_write32(rtwdev, addr, set);
}

static inline void
rtw89_write16_mask(struct rtw89_dev *rtwdev, u32 addr, u32 mask, u16 data)
{
	u32 shift;
	u16 orig, set;

	mask &= 0xffff;
	shift = __ffs(mask);

	orig = rtw89_read16(rtwdev, addr);
	set = (orig & ~mask) | ((data << shift) & mask);
	rtw89_write16(rtwdev, addr, set);
}

static inline void
rtw89_write8_mask(struct rtw89_dev *rtwdev, u32 addr, u32 mask, u8 data)
{
	u32 shift;
	u8 orig, set;

	mask &= 0xff;
	shift = __ffs(mask);

	orig = rtw89_read8(rtwdev, addr);
	set = (orig & ~mask) | ((data << shift) & mask);
	rtw89_write8(rtwdev, addr, set);
}

static inline u32
rtw89_read_rf(struct rtw89_dev *rtwdev, enum rtw89_rf_path rf_path,
	      u32 addr, u32 mask)
{
	u32 val;

	mutex_lock(&rtwdev->rf_mutex);
	val = rtwdev->chip->ops->read_rf(rtwdev, rf_path, addr, mask);
	mutex_unlock(&rtwdev->rf_mutex);

	return val;
}

static inline void
rtw89_write_rf(struct rtw89_dev *rtwdev, enum rtw89_rf_path rf_path,
	       u32 addr, u32 mask, u32 data)
{
	mutex_lock(&rtwdev->rf_mutex);
	rtwdev->chip->ops->write_rf(rtwdev, rf_path, addr, mask, data);
	mutex_unlock(&rtwdev->rf_mutex);
}

static inline struct ieee80211_txq *rtw89_txq_to_txq(struct rtw89_txq *rtwtxq)
{
	void *p = rtwtxq;

	return container_of(p, struct ieee80211_txq, drv_priv);
}

static inline void rtw89_core_txq_init(struct rtw89_dev *rtwdev,
				       struct ieee80211_txq *txq)
{
	struct rtw89_txq *rtwtxq;

	if (!txq)
		return;

	rtwtxq = (struct rtw89_txq *)txq->drv_priv;
	INIT_LIST_HEAD(&rtwtxq->list);
}

static inline struct ieee80211_vif *rtwvif_to_vif(struct rtw89_vif *rtwvif)
{
	void *p = rtwvif;

	return container_of(p, struct ieee80211_vif, drv_priv);
}

static inline
void rtw89_chip_set_channel_prepare(struct rtw89_dev *rtwdev,
				    struct rtw89_channel_help_params *p)
{
	rtwdev->chip->ops->set_channel_help(rtwdev, true, p);
}

static inline
void rtw89_chip_set_channel_done(struct rtw89_dev *rtwdev,
				 struct rtw89_channel_help_params *p)
{
	rtwdev->chip->ops->set_channel_help(rtwdev, false, p);
}

static inline void rtw89_chip_fem_setup(struct rtw89_dev *rtwdev)
{
	const struct rtw89_chip_info *chip = rtwdev->chip;

	if (chip->ops->fem_setup)
		chip->ops->fem_setup(rtwdev);
}

static inline void rtw89_chip_bb_sethw(struct rtw89_dev *rtwdev)
{
	const struct rtw89_chip_info *chip = rtwdev->chip;

	if (chip->ops->bb_sethw)
		chip->ops->bb_sethw(rtwdev);
}

static inline void rtw89_chip_rfk_init(struct rtw89_dev *rtwdev)
{
	const struct rtw89_chip_info *chip = rtwdev->chip;

	if (chip->ops->rfk_init)
		chip->ops->rfk_init(rtwdev);
}

static inline void rtw89_chip_rfk_channel(struct rtw89_dev *rtwdev)
{
	const struct rtw89_chip_info *chip = rtwdev->chip;

	if (chip->ops->rfk_channel)
		chip->ops->rfk_channel(rtwdev);
}

static inline void rtw89_chip_rfk_track(struct rtw89_dev *rtwdev)
{
	const struct rtw89_chip_info *chip = rtwdev->chip;

	if (chip->ops->rfk_track)
		chip->ops->rfk_track(rtwdev);
}

static inline void rtw89_chip_set_txpwr_ctrl(struct rtw89_dev *rtwdev)
{
	const struct rtw89_chip_info *chip = rtwdev->chip;

	if (chip->ops->set_txpwr_ctrl)
		chip->ops->set_txpwr_ctrl(rtwdev);
}

static inline void rtw89_chip_set_txpwr(struct rtw89_dev *rtwdev)
{
	const struct rtw89_chip_info *chip = rtwdev->chip;
	u8 ch = rtwdev->hal.current_channel;

	if (!ch)
		return;

	if (chip->ops->set_txpwr)
		chip->ops->set_txpwr(rtwdev);
}

static inline void rtw89_chip_power_trim(struct rtw89_dev *rtwdev)
{
	const struct rtw89_chip_info *chip = rtwdev->chip;

	if (chip->ops->power_trim)
		chip->ops->power_trim(rtwdev);
}

static inline void rtw89_chip_init_txpwr_unit(struct rtw89_dev *rtwdev,
					      enum rtw89_phy_idx phy_idx)
{
	const struct rtw89_chip_info *chip = rtwdev->chip;

	if (chip->ops->init_txpwr_unit)
		chip->ops->init_txpwr_unit(rtwdev, phy_idx);
}

static inline u8 rtw89_chip_get_thermal(struct rtw89_dev *rtwdev,
					enum rtw89_rf_path rf_path)
{
	const struct rtw89_chip_info *chip = rtwdev->chip;

	if (!chip->ops->get_thermal)
		return 0x10;

	return chip->ops->get_thermal(rtwdev, rf_path);
}

static inline void rtw89_chip_query_ppdu(struct rtw89_dev *rtwdev,
					 struct rtw89_rx_phy_ppdu *phy_ppdu,
					 struct ieee80211_rx_status *status)
{
	const struct rtw89_chip_info *chip = rtwdev->chip;

	if (chip->ops->query_ppdu)
		chip->ops->query_ppdu(rtwdev, phy_ppdu, status);
}

static inline void rtw89_load_txpwr_table(struct rtw89_dev *rtwdev,
					  const struct rtw89_txpwr_table *tbl)
{
	tbl->load(rtwdev, tbl);
}

static inline u8 rtw89_regd_get(struct rtw89_dev *rtwdev, u8 band)
{
	return rtwdev->regd->txpwr_regd[band];
}

static inline void rtw89_ctrl_btg(struct rtw89_dev *rtwdev, bool btg)
{
	const struct rtw89_chip_info *chip = rtwdev->chip;

	if (chip->ops->ctrl_btg)
		chip->ops->ctrl_btg(rtwdev, btg);
}

int rtw89_core_tx_write(struct rtw89_dev *rtwdev, struct ieee80211_vif *vif,
			struct ieee80211_sta *sta, struct sk_buff *skb, int *qsel);
int rtw89_h2c_tx(struct rtw89_dev *rtwdev,
		 struct sk_buff *skb, bool fwdl);
void rtw89_core_tx_kick_off(struct rtw89_dev *rtwdev, u8 qsel);
void rtw89_core_fill_txdesc(struct rtw89_dev *rtwdev,
			    struct rtw89_tx_desc_info *desc_info,
			    void *txdesc);
void rtw89_core_rx(struct rtw89_dev *rtwdev,
		   struct rtw89_rx_desc_info *desc_info,
		   struct sk_buff *skb);
void rtw89_core_query_rxdesc(struct rtw89_dev *rtwdev,
			     struct rtw89_rx_desc_info *desc_info,
			     u8 *data, u32 data_offset);
int rtw89_core_power_on(struct rtw89_dev *rtwdev);
int rtw89_core_sta_add(struct rtw89_dev *rtwdev,
		       struct ieee80211_vif *vif,
		       struct ieee80211_sta *sta);
int rtw89_core_sta_assoc(struct rtw89_dev *rtwdev,
			 struct ieee80211_vif *vif,
			 struct ieee80211_sta *sta);
int rtw89_core_sta_disassoc(struct rtw89_dev *rtwdev,
			    struct ieee80211_vif *vif,
			    struct ieee80211_sta *sta);
int rtw89_core_sta_disconnect(struct rtw89_dev *rtwdev,
			      struct ieee80211_vif *vif,
			      struct ieee80211_sta *sta);
int rtw89_core_sta_remove(struct rtw89_dev *rtwdev,
			  struct ieee80211_vif *vif,
			  struct ieee80211_sta *sta);
int rtw89_core_init(struct rtw89_dev *rtwdev);
void rtw89_core_deinit(struct rtw89_dev *rtwdev);
int rtw89_core_register(struct rtw89_dev *rtwdev);
void rtw89_core_unregister(struct rtw89_dev *rtwdev);
void rtw89_set_channel(struct rtw89_dev *rtwdev);
u8 rtw89_core_acquire_bit_map(unsigned long *addr, unsigned long size);
void rtw89_core_release_bit_map(unsigned long *addr, u8 bit);
void rtw89_vif_type_mapping(struct ieee80211_vif *vif, bool assoc);
int rtw89_chip_info_setup(struct rtw89_dev *rtwdev);
u16 rtw89_ra_report_to_bitrate(struct rtw89_dev *rtwdev, u8 rpt_rate);
int rtw89_regd_init(struct rtw89_dev *rtwdev,
		    void (*reg_notifier)(struct wiphy *wiphy, struct regulatory_request *request));
void rtw89_regd_notifier(struct wiphy *wiphy, struct regulatory_request *request);

#endif
