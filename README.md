# rtw89 📡🐧
### A repo for the newest Realtek rtlwifi codes.

🌟 **Up-to-Date Drivers**: The code in this repo stays in sync with the `wireless-next` repository, with additional changes to accommodate kernel API changes over time.

📌 **Note**: The `wireless-next` repo contains the code set for the next kernel version. At present, kernel 6.5 is out, kernel mainline repo is about to become 6.6-rc1, and `wireless-next` contains the code that will be in kernel 6.7.

⚠️ **Reminder**: You must blacklist the in-tree kernel versions of this driver when using the version in this repository! Failing to do so will result in all manner of strange errors!!! See **Blacklisting** under [Important Information](#important-information) section.

---

### Compatibility
Compatible with **Linux kernel versions 5.7 and newer** as long as your distro hasn't modified any kernel APIs.

We are working on fixing builds on older kernels.

⚠️ **Ubuntu users, expect API changes!** We **will not** modify the source for you. You are on your own!

#### Supported Cards
- **PCIe**: Realtek 8852AE, 8851BE, 8852BE, and 8852CE


**Are you looking for support for these drivers?** 🔎 ⚠️
```bash
# We do not support the following:
RTL8188EE
RTL8192CE
RTL8192CU
RTL8192DE
RTL8192EE
RTL8192SE
RTL8723AE
RTL8723BE
RTL8821AE
```
Check your current kernel or visit the [Backports Project](https://backports.wiki.kernel.org/index.php/Main_Page).

## Troubleshooting & Support

These drivers **won't build for kernels older than 5.7**. Submit GitHub issues **only** for build errors.

For operational problems, reach out to Realtek engineers via E-mail at [linux-wireless@vger.kernel.org](mailto:linux-wireless@vger.kernel.org).

## Issues 🚨
Report any build problems and [see the FAQ](#faq) at bottom of this README.


⚠️ If you see a line such as:
`make[1]: *** /lib/modules/5.17.5-300.fc36.x86_64/build: No such file or directory.` **Stop.**

This indicates **you have NOT installed the kernel headers.** 

Use the following instructions for that step.

## Installation Guide

### Prerequisites 📋
Below are prerequisites for common Linux distributions __before__ you do a [basic installation](#basic-installation-for-all-distros-) or [installation with SecureBoot](#installation-with-secureboot-for-all-distros-):

#### Ubuntu
```bash
sudo apt-get update
sudo apt-get install make gcc linux-headers-$(uname -r) build-essential git
```

#### Fedora
```bash
sudo dnf install kernel-headers kernel-devel
sudo dnf group install "C Development Tools and Libraries"
```

#### openSUSE
```bash
sudo zypper install make gcc kernel-devel kernel-default-devel git libopenssl-devel
```

#### Arch
```bash
git clone https://aur.archlinux.org/rtw89-dkms-git.git
cd rtw89-dkms-git
makepkg -sri
```
---
### Basic Installation for All Distros 🛠

```bash
git clone https://github.com/lwfinger/rtw89.git
cd rtw89
make
sudo make install
```
---
### Installation with SecureBoot for All Distros 🔒

```bash
git clone https://github.com/lwfinger/rtw89.git
cd rtw89
make
sudo make sign-install
```

You will be prompted a password, **please keep it in mind and use it in next steps.**

Reboot to activate the new installed module, then in the MOK managerment screen:
1. Select "Enroll key" and enroll the key created by above sign-install step
2. When promted, enter the password you entered when create sign key. 
3. If you enter wrong password, your computer won't not bebootable. In this case,
   use the BOOT menu from your BIOS, to boot into your OS then do below steps:
```bash
sudo mokutil --reset
```
- Restart your computer.
- Use BOOT menu from BIOS to boot into your OS.
- In the MOK managerment screen, select reset MOK list.
- Reboot then retry from the step `make sign-install`.
---
### Uninstall Drivers
For all distros:
 ```bash
sudo make uninstall
```

## Important Information
Below is important information for using this driver.

### 1. Blacklisting 🚫
If your system has ANY conflicting drivers installed, you must blacklist them as well. For kernels
5.6 and newer. Here is a useful [link](https://askubuntu.com/questions/110341/how-to-blacklist-kernel-modules) on how to blacklist a module.

### 2. Recovery Problems After Sleep/Hibernation 🛌
Some BIOSs have trouble changing power state from D3hot to D0. If you have this problem, then:

``` bash
sudo cp suspend_rtw89 /usr/lib/systemd/system-sleep/.
```

That script will unload the driver before sleep or hibernation, and reload it following resumption.

### 3. How to Disable/Enable a Kernel Module 🪛
```bash
# Do the following in your terminal:

sudo modprobe -rv rtw_8852ae
sudo modprobe -rv rtw_core  # These two statements unload the module.

# Due to the behavior of the modprobe utility, it takes both to unload.

sudo modprobe -v rtw_8852ae # This loads the module.

# A single modprobe call will reload the module.
```

### 4. Option Configuration 📝
```bash
sudo nano /etc/modprobe.d/<dev_name>.conf
```

There, enter the line below:
```bash
options <device_name> <<driver_option_name>>=<value>
```

The available options for rtw89pci are disable_clkreq, disable_aspm_l1, and disable_aspm_l1ss.

The available options for rtw89core are debug_mask, and disable_ps_mode.

Normally, none of these will be needed; however, if you are getting firmware errors, one or both of the disable_aspm_* options may help. They are needed when a buggy BIOS fails to implement the PCI specs correctly.

### 5. Firmware
Firmware from userspace is required to use this driver. This package will attempt to pull the firmware in automatically as a Recommends.

However, if your distro does not provide one of firmware-realtek >= 20230117-1 or linux-firmware >= 20220329.git681281e4-0ubuntu3.10, the driver will fail to load, and dmesg will show an error about a specific missing firmware file. In this case, you can download the firmware files directly from https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git/tree/rtw89.

---

## Kernel Updates 🔄

When your kernel updates, run:
```bash
cd ~/rtw89
git pull
make clean
make
sudo make install
# or just:
sudo make sign-install
```
---

## DKMS Packaging for Debian and it's Derivatives

DKMS is commonly used on Debian and derivatives, like Ubuntu, to streamline building extra kernel modules.

By following the instructions below and installing the resulting package, the rtw89 driver will automatically rebuild on kernel updates. Secure boot signing will happen automatically as well, as long as the dkms signing key (usually located at /var/lib/dkms/mok.key) is enrolled. See your distro's secure boot documentation for more details. 

### Prerequisites:

``` bash
sudo apt install dh-sequence-dkms debhelper build-essential devscripts
```

This workflow uses devscripts, which has quite a few perl dependencies.  
You may wish to build inside a chroot to avoid unnecessary clutter on your system. The debian wiki page for [chroot](https://wiki.debian.org/chroot) has simple instructions for debian, which you can adapt to other distros as needed by changing the release codename and mirror url.

If you do, make sure to install the package on your host system, as it will fail if you try to install inside the chroot. 

### Build and Installation

```bash
# If you've already built as above clean up your workspace or check one out specially (otherwise some temp files can end up in your package)
git clean -xfd

git deborig HEAD
dpkg-buildpackage -us -uc
sudo apt install ../rtw89-dkms_1.0.2-3_all.deb 
```

This will install the package, and build the module for your
currently active kernel.  You should then be able to `modprobe` as
above. It will also load automatically on boot.

## FAQ

Below is a set Frequently Asked Questions when using this repository.

---

### Q1: My driver builds and loads correctly, but fails to work properly. Can you help me?
When you have problems where the driver builds and loads correctly, but it fails to work, a GitHub
issue on this repository is **NOT** the place to report it. 

We have no idea about the internal workings of any of the chips, and the Realtek engineers who do will not read these issues. To reach them, send E-mail to [linux-wireless@vger.kernel.org](mailto:linux-wireless@vger.kernel.org). 

Be sure to include a detailed description of any messages in the kernel logs and any steps that you have taken to analyze or fix the problem. If your description is not complete, you are unlikely to get the help you need.

Start with this page for guidance: https://wireless.wiki.kernel.org/en/users/support

---

### Q2: I'm using Ubuntu, and the build failed. What do I do?
Ubuntu often modifies kernel APIs, which can cause build issues. You'll need to manually adjust the source code or look for solutions specific to your distribution. We cannot support these types of issues.

---

### Q3: How do I update the driver after a kernel update?
You will need to pull the latest code from this repository and recompile the driver. [Follow the steps in the Maintenance section](#kernel-updates-).

---

### Q4: Is Secure Boot supported?
Yes, this repository provides a way to sign the kernel modules to be compatible with Secure Boot. [Check out the Installation with SecureBoot section](#installation-with-secureboot-).

---

### Q5: My card isn't listed. Can I request a feature?
For feature requests like supporting a new card, you should reach out to Realtek engineers via E-mail at [linux-wireless@vger.kernel.org](mailto:linux-wireless@vger.kernel.org).
