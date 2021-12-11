rtw89 v7
===========
### A repo for the newest Realtek rtlwifi codes.

This branch was created from v7 of the code, which is the version merged into the wireless-drivers-next
repo, which means it will be in the 5.16 kernel.

This code will build on any kernel 5.4 and newer as long as the distro has not modified
any of the kernel APIs. IF YOU RUN UBUNTU, YOU CAN BE ASSURED THAT THE APIs HAVE CHANGED.
NO, I WILL NOT MODIFY THE SOURCE FOR YOU. YOU ARE ON YOUR OWN!!!!!

I am working on fixing builds on older kernels.

This repository includes drivers for the following card:

Realtek 8852AE

If you are looking for a driver for chips such as
RTL8188EE, RTL8192CE, RTL8192CU, RTL8192DE, RTL8192EE, RTL8192SE, RTL8723AE, RTL8723BE, or RTL8821AE,
these should be provided by your kernel. If not, then you should go to the Backports Project
(https://backports.wiki.kernel.org/index.php/Main_Page) to obtain the necessary code.

### Installation instruction
##### Requirements
You will need to install "make", "gcc", "kernel headers", "kernel build essentials", and "git".

For **Ubuntu**: You can install them with the following command
```bash
sudo apt-get update
sudo apt-get install make gcc linux-headers-$(uname -r) build-essential git
```
For **Fedora**: You can install them with the following command
```bash
sudo dnf install kernel-headers kernel-devel
sudo dnf group install "C Development Tools and Libraries"
```
If any of the packages above are not found check if your distro installs them like that.

##### Installation
For all distros:
```bash
git clone https://github.com/lwfinger/rtw89.git
cd rtw89
make
sudo make install
```

##### Installation with module signing for SecureBoot
For all distros:
```bash
git clone https://github.com/lwfinger/rtw89.git
cd rtw89
make
sudo make sign-install
```
You will be promted a password, please keep it in mind and use it in next steps.
Reboot to active the new installed module.
In the MOK managerment screen:
1. Select "Enroll key" and enroll the key created by above sign-install step
2. When promted, enter the password you entered when create sign key. 
3. If you enter wrong password, your computer won't not boot able. In this case, use the BOOT menu from your BIOS, to boot into your OS then do below steps:
```bash
sudo mokutil --reset
```
Restart your computer
Use BOOT menu from BIOS to boot into your OS
In the MOK managerment screen, select reset MOK list
Reboot then retry from the step make sign-install

##### How to disable/enable a Kernel module
 ```bash
sudo modprobe -r rtw89pci         #This unloads the module
sudo modprobe rtw89pci            #This loads the module
```

##### Problem with recovery after sleep or hibernation
Some BIOSs have trouble changing power state from D3hot to D0. If you have this problem, then

sudo cp suspend_rtw89 /usr/lib/systemd/system-sleep/.

That script will unload the driver before sleep or hibernation, and reload it following resumption.

##### Option configuration
If it turns out that your system needs one of the configuration options, then do the following:
```bash
sudo nano /etc/modprobe.d/<dev_name>.conf
```
There, enter the line below:
```bash
options <device_name> <<driver_option_name>>=<value>
```
The available options for rtw89pci are disable_clkreq, disable_aspm_l1, and disable_aspm_l1ss.
The available options for rtw89core are debug_mask, and disable_ps_mode

Normally, none of these will be needed; however, if you are getting firmware errors, one or both
of the disable_aspm_* options may help. Thay are needed when a buggy BIOS fails to implement the
PCI specs correctly.

***********************************************************************************************

When your kernel changes, then you need to do the following:
```bash
cd ~/rtw89
git pull
make
sudo make install
```

Remember, this MUST be done whenever you get a new kernel - no exceptions.

These drivers will not build for kernels older than 5.4. If you must use an older kernel,
submit a GitHub issue with a listing of the build errors. Without the errors, the issue
will be ignored. I am not a mind reader.

When you have problems where the driver builds and loads correctly, but fails to work, a GitHub
issue is NOT the best place to report it. I have no idea of the internal workings of any of the
chips, and the Realtek engineers who do will not read these issues. To reach them, send E-mail to
linux-wireless@vger.kernel.org. Include a detailed description of any messages in the kernel
logs and any steps that you have taken to analyze or fix the problem. If your description is
not complete, you are unlikely to get any satisfaction. One other thing - your mail MUST be plain test.
HTML mail is rejected.
