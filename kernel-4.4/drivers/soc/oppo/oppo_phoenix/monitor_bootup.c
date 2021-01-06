/*
###############################################################################
## Copyright 2018 OPPO Mobile Comm Corp., Ltd.
##
## File: boologhelper.te
## Description : add for project phenix(hang oppo)
##
##
##
## Version:  1.0
## Date:  2018/12/24
## Author:  bright.zhang@oppo.com
## ----------------- Revision History: ----------------------
## <author>       <data>           <desc>
## Bright Zhang   2018/12/24       create this file
################################################################################
*/
#ifdef VENDOR_EDIT
//Liang.Zhang@PSW.TECH.BOOTUP, 2018/10/19, Add for kernel monitor boot
#ifdef HANG_OPPO_ALL

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/timex.h>
#include <linux/rtc.h>
#include "../../kernel/op_panic_recovery.h"
#include "../../drivers/misc/mediatek/include/mt-plat/mtk_boot_common.h"

#ifdef CONFIG_OPPO_REALEASE_BUILD    // user version, not release
#define CHECK_BOOT_TIME 240
#else
#define CHECK_BOOT_TIME 480
#endif

#define DELAY_TIME 30

struct task_struct *boot_complete_timer;

extern struct file *filp_open(const char *, int, umode_t);
extern int filp_close(struct file *, fl_owner_t id);
extern void op_log_boot(char *str);
// Kun.Hu@PSW.TECH.RELIABILTY, 2018/11/15, add for project phoenix(hang oppo)
extern void phx_monit(const char *monitor_command);
extern void kernel_restart(char *cmd);
extern int write_to_reserve1(struct pon_struct *pon_info, int fatal_error);
extern int need_recovery(struct pon_struct *pon_info);
extern int creds_change_dac(void);
extern int creds_change_id(void);
#ifndef O_RDONLY
#define O_RDONLY  00
#endif

#ifndef ENOENT
#define ENOENT    2
#endif

//Kun.Hu@TECH.BSP.Stability, 2019/05/09, swith off phoenix in legacy reserve partition
#ifdef OPPO_RESERVE_USE_LEGACY
int hang_oppo_main_on = 0;
#else
int hang_oppo_main_on = 1;   //default on
#endif
int hang_oppo_recovery_method = RESTART_AND_RECOVERY;
static int hang_oppo_kernel_time = CHECK_BOOT_TIME;

static int __init HangOppoMainOn(char *str)
{
    get_option(&str,&hang_oppo_main_on);

    pr_info("hang_oppo_main_on %d\n", hang_oppo_main_on);

    return 1;
}
__setup("phx_rus_conf.main_on=", HangOppoMainOn);

static int __init HangOppoRecoveryMethod(char *str)
{
    get_option(&str,&hang_oppo_recovery_method);

    pr_info("hang_oppo_recovery_method %d\n", hang_oppo_recovery_method);

    return 1;
}
__setup("phx_rus_conf.recovery_method=", HangOppoRecoveryMethod);

static int __init HangOppoKernelTime(char *str)
{
    get_option(&str,&hang_oppo_kernel_time);

    pr_info("hang_oppo_kernel_time %d\n", hang_oppo_kernel_time);

    return 1;
}
__setup("phx_rus_conf.kernel_time=", HangOppoKernelTime);

int op_bootcompleted(void)
{
    struct file *opfile;
    ssize_t size;
    loff_t offsize;
    char data_info[6] = {'\0'};
    mm_segment_t old_fs;

    opfile = filp_open("/proc/opbootcomplete", O_RDONLY, 0444);
    if (IS_ERR(opfile)) {
        pr_err("open /proc/opbootcomplete error: (%ld)\n", PTR_ERR(opfile));
        return PTR_ERR(opfile);
    }

    offsize = 0;
    old_fs = get_fs();
    set_fs(KERNEL_DS);
    size = vfs_read(opfile,data_info,sizeof(data_info),&offsize);
    if (size < 0) {
        pr_err("data_info %s size %ld", data_info, size);
        set_fs(old_fs);
        return -1;
    }
    set_fs(old_fs);
    filp_close(opfile,NULL);

    if (strncmp(data_info, "true", 4) == 0) {
        return 1;
    }

    return 0;
}

int op_isLongTimeBoot(void)
{
    struct file *opfile;
    ssize_t size;
    loff_t offsize;
    char data_info[16] = {'\0'};
    mm_segment_t old_fs;

    opfile = filp_open("/proc/opbootfrom", O_RDONLY, 0444);
    if (IS_ERR(opfile)) {
        pr_err("open /proc/opbootfrom error:\n");
        return -1;
    }

    offsize = 0;
    old_fs = get_fs();
    set_fs(KERNEL_DS);
    size = vfs_read(opfile,data_info,sizeof(data_info),&offsize);
    if (size < 0) {
        pr_err("data_info %s size %ld", data_info, size);
        set_fs(old_fs);
        return -1;
    }
    set_fs(old_fs);
    filp_close(opfile,NULL);

    if (strncmp(data_info, "normal", 6) == 0) {
        return 0;
    }

    return 1;
}

int data_need_wipe(void)
{
    struct file *opfile;
    int rc;

    opfile = filp_open("/data/oppo/log/opporeserve/recovery_info", O_CREAT | O_RDWR, 0764);

    if (IS_ERR(opfile)) {
        pr_err("data_need_wipe open /data/oppo/log/opporeserve/recovery_info error: (%ld)\n", PTR_ERR(opfile));
        if((PTR_ERR(opfile) == (-EROFS)) || (PTR_ERR(opfile) == (-ENOENT)) || (PTR_ERR(opfile) == (-ENOSPC))) {
            return 1;
        } else {
            return 0;
        }
    }

    rc = vfs_fsync(opfile, 1);
    if (rc) {
        pr_err("sync returns %d\n", rc);
    }

    filp_close(opfile,NULL);

    return 0;
}

int is_gis_test(void)
{
    struct file *opfile;

    opfile = filp_open("/init.oppo.debug.rc", O_RDONLY, 0444);

    if (IS_ERR(opfile)) {
        pr_err("open init.oppo.debug.rc error: (%ld)\n", PTR_ERR(opfile));
        if(PTR_ERR(opfile) == (-ENOENT)) {
            return 1;
        } else {
            return 0;
        }
    }

    if(opfile) {
        filp_close(opfile,NULL);
    }

    pr_info("op system \n");

    return 0;
}

int set_fatal_err_to_recovery(void)
{
    int rc;
    ssize_t size;
    loff_t offsize = 0;
    struct file *opfile;
    mm_segment_t old_fs;
    char data_info[24] = {'\0'};

    creds_change_id();

    opfile = filp_open("/cache/recovery/command", O_CREAT | O_RDWR, 0764);

    if (IS_ERR(opfile)) {
        pr_err("open /cache/recovery/command error: (%ld)\n", PTR_ERR(opfile));
        return -1;
    }

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    if(data_need_wipe())
    {
        strcpy(data_info, OPPO_FATAL_ERR_TO_WIPEDATA);
    } else {
        strcpy(data_info, OPPO_FATAL_ERR_TO_RECOVERY);
    }

    pr_info("data_info %s\n", data_info);

    size = vfs_write(opfile, data_info, sizeof(data_info), &offsize);

    if (size < 0) {
         pr_err("vfs_write data_info %s size %ld \n", data_info, size);
         set_fs(old_fs);
         return -1;
    }
    rc = vfs_fsync(opfile, 1);
    if (rc) {
        pr_err("sync returns %d\n", rc);
    }

    set_fs(old_fs);
    filp_close(opfile,NULL);

    return 1;
}

static int op_start_check_bootup(void)
{
    int boot_completed = 0;
    int hang_oppo_8_mins = 0;

#ifndef CONFIG_OPPO_REALEASE_BUILD    // user version, not release
    hang_oppo_8_mins = 1;
    schedule_timeout_uninterruptible(hang_oppo_kernel_time * HZ);
#else
    if(op_isLongTimeBoot()) {
        hang_oppo_8_mins = 1;
        schedule_timeout_uninterruptible(hang_oppo_kernel_time * HZ);
    }
#endif

    boot_completed = op_bootcompleted();

    pr_err("op_start_check_bootup:\n");

    if (0 == boot_completed) {
        struct timespec ts;
        struct rtc_time tm;
        char err_info[64] = {0};

        getnstimeofday(&ts);
        rtc_time_to_tm(ts.tv_sec, &tm);

        if(hang_oppo_8_mins) {
            snprintf(err_info, sizeof(err_info), "SET_BOOTERROR@ERROR_HANG_OPPO_OVER_8_MINS@%d-%d-%d %d:%d:%d",
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        }
        #ifdef CONFIG_OPPO_REALEASE_BUILD    //  release
        if(!hang_oppo_8_mins) {
            snprintf(err_info, sizeof(err_info), "SET_BOOTERROR@ERROR_HANG_OPPO_OVER_4_MINS@%d-%d-%d %d:%d:%d",
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        }
        #endif
        // Kun.Hu@PSW.TECH.RELIABILTY.1890222, 2019/03/20, remove all fs opreration in kernel, fix extfs crash
        if(!is_gis_test()) {
            // Kun.Hu@PSW.TECH.RELIABILTY, 2018/11/15, add for project phoenix(hang oppo)
            phx_monit(err_info);
            schedule_timeout_uninterruptible(DELAY_TIME * HZ);
            panic("boot didn't complete in 240/480s");
        }
    } else if(1 == boot_completed) {
        pr_info("phoenix boot complete \n");
    } else if((-ENOENT) == boot_completed){
        panic("[PHOENIX] phone didn't boot to stage init in 240/480s");
    } else {
        pr_err("phoenix unknow state \n");
    }

    return 0;

}

//start with watchdog, kick a 240/480s timer for bootcomplete
static int opmonitor_boot_kthread(void *dummy)
{
    set_user_nice(current, 0);
    pr_err("opmonitor_boot_kthread:\n");

    if(hang_oppo_kernel_time) {
        schedule_timeout_uninterruptible(hang_oppo_kernel_time * HZ);
    } else {
        schedule_timeout_uninterruptible(CHECK_BOOT_TIME * HZ);
    }
    op_start_check_bootup();
    return 0;
}

bool op_is_monitorable_boot(void)
{
    int boot_mode = 0;

    boot_mode = get_boot_mode();
    pr_err("boot_mode: %d\n", boot_mode);

    if((boot_mode == NORMAL_BOOT) || (boot_mode == ALARM_BOOT)	) {
        return true;
    } else {
        return false;
    }
}

static int __init oppo_monitor_bootup_init(void)
{
    int ret = 0;
    int monitor_boot_mode = 0;

    pr_err("oppo_monitor_bootup_init hang_oppo_main_on: %d\n", hang_oppo_main_on);

    if(hang_oppo_main_on)
    {
        monitor_boot_mode = op_is_monitorable_boot();

        if (monitor_boot_mode)
        {
            boot_complete_timer = kthread_run(opmonitor_boot_kthread, NULL,	"opmonitor_boot");
        }
    }

    return ret;
}
postcore_initcall(oppo_monitor_bootup_init);

MODULE_DESCRIPTION("OPPO monitor bootup");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Bright.Zhang <bright.zhang@oppo.com>");

#endif
#endif  //VENDOR_EDIT

