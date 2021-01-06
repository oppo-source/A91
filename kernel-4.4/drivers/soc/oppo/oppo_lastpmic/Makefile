# include $(srctree)/drivers/misc/mediatek/Makefile.custom
#ifdef VENDOR_EDIT
#wen.luo@BSP.Kernel.Stability, 2018/12/24,  for enable KASAN, use GCC complile tool, add flag -Wno-misleading-indentation
ifeq ($(CONFIG_KASAN),y)
subdir-ccflags-y += -Wno-format
endif
#endif /*VENDOR_EDIT*/
obj-y += oppo_last_pmic.o
