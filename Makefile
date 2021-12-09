SHELL := /bin/bash
KVER  ?= $(shell uname -r)
KSRC := /lib/modules/$(KVER)/build
FIRMWAREDIR := /lib/firmware/
PWD := $(shell pwd)
CLR_MODULE_FILES := *.mod.c *.mod *.o .*.cmd *.ko *~ .tmp_versions* modules.order Module.symvers
SYMBOL_FILE := Module.symvers
# Handle the move of the entire rtw88 tree
ifneq ("","$(wildcard /lib/modules/$(KVER)/kernel/drivers/net/wireless/realtek)")
MODDESTDIR := /lib/modules/$(KVER)/kernel/drivers/net/wireless/realtek/rtw89
else
MODDESTDIR := /lib/modules/$(KVER)/kernel/drivers/net/wireless/rtw89
endif

#Handle the compression option for modules in 3.18+
ifneq ("","$(wildcard $(MODDESTDIR)/*.ko.gz)")
COMPRESS_GZIP := y
endif
ifneq ("","$(wildcard $(MODDESTDIR)/*.ko.xz)")
COMPRESS_XZ := y
endif
#MOK_KEY_DIR#\ ?= /var/lib/shim-signed/mok #No where use this variable, remove

EXTRA_CFLAGS += -O2
EXTRA_CFLAGS += -DCONFIG_RTW89_DEBUGMSG
EXTRA_CFLAGS += -DCONFIG_RTW89_DEBUGFS

obj-m += rtw89core.o
rtw89core-y +=  core.o \
		debug.o \
		mac80211.o \
		mac.o \
		phy.o \
		fw.o \
		rtw8852a.o \
		rtw8852a_table.o \
		rtw8852a_rfk.o \
		rtw8852a_rfk_table.o \
		cam.o \
		efuse.o \
		regd.o \
		coex.o \
		ps.o \
		sar.o \
		ser.o \
		usb.o

obj-m += rtw89pci.o
rtw89pci-y := pci.o

obj-m += rtw89usb.o
rtw89usb-y := usb.o

ccflags-y += -D__CHECK_ENDIAN__

.PHONY: all install uninstall clean sign sign-install

all:
	$(MAKE) -C $(KSRC) M=$(PWD) modules
install: all
	@rm -f $(MODDESTDIR)/rtw89*.ko

	@mkdir -p $(MODDESTDIR)
	@install -p -D -m 644 *.ko $(MODDESTDIR)
ifeq ($(COMPRESS_GZIP), y)
	@gzip -f $(MODDESTDIR)/*.ko
endif
ifeq ($(COMPRESS_XZ), y)
	@xz -f $(MODDESTDIR)/*.ko
endif
	@depmod -a $(KVER)

	@mkdir -p /lib/firmware/rtw89/
	@cp rtw8852a_fw.bin /lib/firmware/rtw89/.
	@mkdir -p /lib/firmware/rtl_bt/
	@cp rtl8852au*.bin /lib/firmware/rtl_bt/.

	@echo "Install rtw89 SUCCESS"

uninstall:
	@rm -f $(MODDESTDIR)/rtw89*.ko

	@depmod -a

	@echo "Uninstall rtw89 SUCCESS"

clean:
	@rm -fr *.mod.c *.mod *.o .*.cmd .*.o.cmd *.ko *~ .*.o.d .cache.mk
	@rm -fr .tmp_versions
	@rm -fr Modules.symvers
	@rm -fr Module.symvers
	@rm -fr Module.markers
	@rm -fr modules.order

sign:
#mkdir -p ~/mok && pushd ~/mok #Remove because we don't need it anymore
#Store the key under home is better for future, we can re-sign without rebuild
	@mkdir -p ~/.ssl 
	@openssl req -new -x509 -newkey rsa:2048 -keyout ~/.ssl/MOK.priv -outform DER -out ~/.ssl/MOK.der -nodes -days 36500 -subj "/CN=Custom MOK/"
	@mokutil --import ~/.ssl/MOK.der
#popd #Remove, it cause error
	
	@/usr/src/kernels/$(KVER)/scripts/sign-file sha256 ~/.ssl/MOK.priv ~/.ssl/MOK.der rtw89core.ko
	@/usr/src/kernels/$(KVER)/scripts/sign-file sha256 ~/.ssl/MOK.priv ~/.ssl/MOK.der rtw89pci.ko
	@/usr/src/kernels/$(KVER)/scripts/sign-file sha256 ~/.ssl/MOK.priv ~/.ssl/MOK.der rtw89usb.ko

sign-install: all sign install

