#ifdef VENDOR_EDIT
# Bin.Li@EXP.BSP.bootloader.bootflow, 2017/05/15, Add for oppo device info and oppo project
obj-$(CONFIG_OPPO_COMMON_SOFT)  +=  oppo_project/
obj-$(CONFIG_OPPO_DEVICE_IFNO)  +=  oppo_devinfo/
obj-y += oppo_custom/
#endif /* VENDOR_EDIT */

#ifdef VENDOR_EDIT
# Bin.Li@EXP.BSP.bootloader.bootflow, 2017/05/15, Add for critical_log
obj-y     +=  oppo_criticallog/
#endif /* VENDOR_EDIT */
# ifdef VENDOR_EDIT 

#ifdef VENDOR_EDIT
# Bin.Li@EXP.BSP.bootloader.bootflow, 2017/05/15, Add hypnus helper
obj-y     +=  oppo_hypnus/
#endif /* VENDOR_EDIT */

#ifdef VENDOR_EDIT
#//wenbin.liu@PSW.Kernel.MM, 2018/05/23, Add for Healthinfo monitor
obj-y     +=  oppo_healthinfo/
#endif /* VENDOR_EDIT */

#ifdef VENDOR_EDIT
#//Kun.Hu@PSW.TECH 2018/11/15, Add phoenix base
obj-y     +=  oppo_phoenix/
#endif /* VENDOR_EDIT */

#ifdef VENDOR_EDIT
#//qiuzuolin@PSW.BSP.Sensor , 2018-05-15,Add for light sensor cali
obj-$(CONFIG_OPPO_ALS_CALI)   +=  oppo_als_cali_value/
#endif /* VENDOR_EDIT */

#ifdef VENDOR_EDIT
#* wen.luo@PSW.BSP.kernel, 2019/6/25, Add for parsar powoff/powon */
obj-y     +=  oppo_lastpmic/

#Liang.Zhang@TECH.Storage.Stability.OPPO_SHUTDOWN_DETECT, 2019/04/28, Add for shutdown detect
obj-$(CONFIG_OPPO_SHUTDOWN_DETECT)  +=  oppo_shutdown_detect/
#endif /* VENDOR_EDIT */
