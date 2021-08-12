Fork of [lwfinger/rtw89](https://github.com/lwfinger/rtw89) - for newest versions see also upstream repository

WLAN Driver for Lenovo IdeaPad 5 (Realtek 8852AE WiFi 6 WLAN Adapter)

## How-to install

```bash
sudo apt update
sudo apt install make gcc linux-headers-$(uname -r) build-essential git
git clone https://github.com/lwfinger/rtw89.git -b v5
cd rtw89 && make && sudo make install
sudo modprobe rtw89pci
```

After that, the WLAN network adapter should apear.

If you still have any problems, also ensure that the `Secure boot` is disabled in the bios
