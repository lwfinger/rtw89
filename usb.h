#ifndef __RTW_USB_H_
#define __RTW_USB_H_

#define RTW_USB_CMD_READ		0xc0
#define RTW_USB_CMD_WRITE		0x40
#define RTW_USB_CMD_REQ			0x05

#define RTW_USB_IS_FULL_SPEED_USB(rtwusb) \
	((rtwusb)->usb_speed == RTW_USB_SPEED_1_1)
#define RTW_USB_IS_HIGH_SPEED(rtwusb)	((rtwusb)->usb_speed == RTW_USB_SPEED_2)
#define RTW_USB_IS_SUPER_SPEED(rtwusb)	((rtwusb)->usb_speed == RTW_USB_SPEED_3)

#define RTW_USB_SUPER_SPEED_BULK_SIZE	1024
#define RTW_USB_HIGH_SPEED_BULK_SIZE	512
#define RTW_USB_FULL_SPEED_BULK_SIZE	64

#define RTW_USB_TX_SEL_HQ		BIT(0)
#define RTW_USB_TX_SEL_LQ		BIT(1)
#define RTW_USB_TX_SEL_NQ		BIT(2)
#define RTW_USB_TX_SEL_EQ		BIT(3)

#define RTW_USB_BULK_IN_ADDR		0x80
#define RTW_USB_INT_IN_ADDR		0x81

#define RTW_USB_HW_QUEUE_ENTRY		8

#define RTW_USB_PACKET_OFFSET_SZ	8
#define RTW_USB_MAX_XMITBUF_SZ		(1592 * 3)
#define RTW_USB_MAX_RECVBUF_SZ		(512 * 60)

#define RTW_USB_RECVBUFF_ALIGN_SZ	8

#define RTW_USB_RXAGG_SIZE		6
#define RTW_USB_RXAGG_TIMEOUT		10

#define RTW_USB_RXCB_NUM		4

#define REG_SYS_CFG2		0x00FC
#define REG_USB_USBSTAT		0xFE11
#define REG_RXDMA_MODE		0x785
#define REG_TXDMA_OFFSET_CHK	0x20C
#define BIT_DROP_DATA_EN	BIT(9)

/* USB Vendor/Product IDs */
#define RTW_USB_VENDOR_ID_REALTEK		0x0BDA
#define RTW_USB_VENDOR_ID_NETGEAR		0x0846
#define RTW_USB_VENDOR_ID_EDIMAX		0x7392

/* Register definitions for USB */

#define R_AX_USB3_MAC_NPI_CONFIG_INTF_0 0x1114
#define B_AX_SSPHY_LFPS_FILTER BIT(31)

#define R_AX_RXDMA_SETTING 0x8908

#define R_AX_USB_ENDPOINT_0 0x1060
#define R_AX_USB_ENDPOINT_1 0x1064
#define R_AX_USB_ENDPOINT_2 0x1068
#define R_AX_USB_ENDPOINT_3 0x106C

#define R_AX_USB_HOST_REQUEST_2 0x1078
#define B_AX_R_USBIO_MODE BIT(4)

#define R_AX_USB_D2F_F2D_INFO 0x1200

enum rtw89_bulkout_id {
	RTW89_BULKOUT_ID0 = 0,
	RTW89_BULKOUT_ID1 = 1,
	RTW89_BULKOUT_ID2 = 2,
	RTW89_BULKOUT_ID3 = 3,
	RTW89_BULKOUT_ID4 = 4,
	RTW89_BULKOUT_ID5 = 5,
	RTW89_BULKOUT_ID6 = 6,
	RTW89_BULKOUT_NUM = 7
};

/* helper for USB Ids */

#define RTK_USB_DEVICE(vend, dev, hw_config)	\
	USB_DEVICE(vend, dev),			\
	.driver_info = (kernel_ulong_t)&(hw_config),

/* defined functions */
#define rtw_get_usb_priv(rtwdev) (struct rtw89_usb *)((rtwdev)->priv)

enum rtw_usb_burst_size {
	USB_BURST_SIZE_3_0 = 0x0,
	USB_BURST_SIZE_2_0_HS = 0x1,
	USB_BURST_SIZE_2_0_FS = 0x2,
	USB_BURST_SIZE_2_0_OTHERS = 0x3,
	USB_BURST_SIZE_UNDEFINE = 0x7F,
};

enum rtw_usb_speed {
	RTW_USB_SPEED_UNKNOWN	= 0,
	RTW_USB_SPEED_1_1	= 1,
	RTW_USB_SPEED_2		= 2,
	RTW_USB_SPEED_3		= 3,
};

struct rx_usb_ctrl_block {
	u8 *data;
	struct urb *rx_urb;
	struct sk_buff *rx_skb;
	u8 ep_num;
};

struct rtw89_usb_work_data {
	struct work_struct work;
	struct rtw89_dev *rtwdev;
};

struct rtw89_usb_tx_data {
	u8 sn;
};

struct rtw89_usb {
	struct rtw89_dev *rtwdev;
	struct usb_device *udev;

	u32 bulkout_size;
	u8 num_in_pipes;
	u8 num_out_pipes;
	u8 pipe_interrupt;
	u8 pipe_in;
	u8 out_ep[8];
	u8 out_ep_queue_sel;
	u8 queue_to_pipe[8];
	u8 usb_speed;
	u8 usb_txagg_num;

	atomic_t is_bus_drv_ready;

	struct workqueue_struct *txwq;
	struct sk_buff_head tx_queue[RTW89_BULKOUT_NUM];
	struct rtw89_usb_work_data *tx_handler_data;

	struct workqueue_struct *rxwq;
	struct rx_usb_ctrl_block rx_cb[RTW_USB_RXCB_NUM];
	struct sk_buff_head rx_queue;
	struct rtw89_usb_work_data *rx_handler_data;
};

static inline struct rtw89_usb_tx_data *rtw_usb_get_tx_data(struct sk_buff *skb)
{
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);

	BUILD_BUG_ON(sizeof(struct rtw89_usb_tx_data) >
		sizeof(info->status.status_driver_data));

	return (struct rtw89_usb_tx_data *)info->status.status_driver_data;
}

#endif
