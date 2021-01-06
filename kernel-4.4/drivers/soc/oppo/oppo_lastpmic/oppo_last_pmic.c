/***************************************************************
** Copyright (C),  2019,  OPPO Mobile Comm Corp.,  Ltd
** VENDOR_EDIT
** File : oppo_last_pmic.h
** Description : PMIC PONSTS POFFSTS
** Version : 1.0
** Date : 2019/6/27
** Author : Wen.Luo@PSW.BSP.Kernel.Stability
**
** ------------------------------ Revision History: --------------------------------
**  <author>        <data>        <version >        <desc>
**   Wen.Luo          2019/06/27        1.0         Created for POFF_REASON/PON_REASON
******************************************************************/
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <oppo/oppo_last_pmic.h>

#include <linux/io.h>
#include <linux/of.h>


#define PON_POFF_DESC_MAX_SIZE 256
static struct proc_dir_entry *oppo_last_pmic = NULL;
static char pOFF_status[PON_POFF_DESC_MAX_SIZE] = {'\0'};
static char pON_status[PON_POFF_DESC_MAX_SIZE] = {'\0'};


static BootReason_Pon_Poff *pboot_reason =NULL;
static BootReason_Pon_Poff boot_reason_pon_off;


static int oppo_last_pmic_poff_parsar(BootReason_Pon_Poff *pboot_reason, u16 poff_mask, char *buf)
{
	int len = 0;
	int i = 0;
	for (i = 0; i < REG_INDEX_MAX_ID; i ++) {
		if (!pmic_test_bit(i, &poff_mask)) {
			if(pmic_test_bit(i, &(pboot_reason->poff_sts))) {
				len += snprintf(buf+len, (PON_POFF_DESC_MAX_SIZE - len), "%s ", DesPoff_STS[i]);
				if(i == 1) {
					len += snprintf(buf+len, (PON_POFF_DESC_MAX_SIZE - len), "%x ", pboot_reason->pmic_misc[i]);
				}
				if(i == 2) {
					len += snprintf(buf+len, (PON_POFF_DESC_MAX_SIZE - len), "%x ", pboot_reason->pmic_misc[i]);
					len += snprintf(buf+len, (PON_POFF_DESC_MAX_SIZE - len), "%x ", pboot_reason->pmic_misc[i+1]);
					len += snprintf(buf+len, (PON_POFF_DESC_MAX_SIZE - len), "%x ", pboot_reason->pmic_misc[i+2]);
				}
				if(i == 3) {
					len += snprintf(buf+len, (PON_POFF_DESC_MAX_SIZE - len), "%x ", pboot_reason->pmic_misc[5]);
				}
				break;
			}
		}
	}
	if (!len)
		snprintf(buf+len, (PON_POFF_DESC_MAX_SIZE - len), "%s ", "Normal\n");
	return 0;
}

static int oppo_last_pmic_pon_parsar(BootReason_Pon_Poff *pboot_reason, u16 pon_mask, char *buf)
{
	int len = 0;
	if (!len)
		snprintf(buf+len, (PON_POFF_DESC_MAX_SIZE - len), "%s ", "Normal\n");
	return 0;
}

static unsigned int init_last_pmic_info(void)
{
	struct device_node *np = NULL;
	u16 pmic_misc[10] = {0};
	int ret = 0;
	int i = 0;
	pr_info("init_last_pmic_info start\n");
	if(!pboot_reason)
		pboot_reason = &boot_reason_pon_off;

	np = of_find_node_by_name(NULL, "oppo_lastpmic");
	if(!np){
		pr_err("init_last_pmic_info find node error\n");
		return -1;
	}
	ret = of_property_read_u16(np,"pon_sts",&(pboot_reason->pon_sts));
	if(ret)
	{
		pr_err("init_last_pmic_info pon_sts");
		return ret;
	}
	pr_err("init_last_pmic_info pon_sts:%u\n",pboot_reason->pon_sts);

	ret = of_property_read_u16(np,"poff_sts",&(pboot_reason->poff_sts));
	if(ret)
	{
		pr_err("init_last_pmic_info poff_sts");
		return ret;
	}
	pr_err("init_last_pmic_info poff_sts:%u\n",pboot_reason->poff_sts);
	ret = of_property_read_u16_array(np, "pmic_misc", pmic_misc, ARRAY_SIZE(pmic_misc));
	if (ret) {
		pr_err("init_last_pmic_info pmic_misc\n");
	} else {
		for (i = 0; i < ARRAY_SIZE(pmic_misc); i ++) {
			pboot_reason->pmic_misc[i] = pmic_misc[i];
			pr_err("init_last_pmic_info pmic_misc[%d]:%u\n", i, pmic_misc[i]);
		}
	}
	return 0;
}

static ssize_t pon_sts_read_proc(struct file *file, char __user *buf,
		size_t count,loff_t *off)
{
	int len = 0;

	if (pON_status[0] == '\0')
		oppo_last_pmic_pon_parsar(pboot_reason, pon_mask, pON_status);//do parsar
	len = strlen(pON_status) + 1;

	return simple_read_from_buffer(buf, count, off, pON_status, len);
}

static struct file_operations pon_proc_fops = {
	.read = pon_sts_read_proc,
	.write = NULL,
};

static ssize_t poff_sts_read_proc(struct file *file, char __user *buf,
		size_t count,loff_t *off)
{
	int len = 0;

	if (pOFF_status[0] == '\0')
		oppo_last_pmic_poff_parsar(pboot_reason, poff_mask, pOFF_status);//do parsar
	len = strlen(pOFF_status) + 1;

	return simple_read_from_buffer(buf, count, off, pOFF_status, len);
}

static struct file_operations poff_proc_fops = {
	.read = poff_sts_read_proc,
	.write = NULL,
};

static int __init oppo_last_pmic_init(void)
{
	int ret = 0;
	struct proc_dir_entry *pentry;

	if(oppo_last_pmic) //inited
	{
		return ret;
	}

	oppo_last_pmic =  proc_mkdir("oppoLastPmic", NULL);
	if(!oppo_last_pmic) {
		pr_err("can't create oppo_last_pmic proc\n");
		return -ENOMEM;
	}
	pentry = proc_create("PON_REASON", S_IRUGO, oppo_last_pmic, &pon_proc_fops);
	if(!pentry) {
		pr_err("create PON_REASON proc failed.\n");
		goto ERROR_PON;
	}
	pentry = proc_create("POFF_REASON", S_IRUGO, oppo_last_pmic, &poff_proc_fops);
	if(!pentry) {
		pr_err("create POFF_REASON proc failed.\n");
		goto ERROR_POFF;
	}
	return ret;
ERROR_POFF:
//	remove_proc_entry("PON_REASON", NULL);
ERROR_PON:
	return -ENOMEM;
}


static int __init boot_oppo_lastpmic_core(void)
{
	int ret = 0;
	ret = init_last_pmic_info();
	if(ret < 0)
	{
		pr_err("init_last_pmic_info fail");
		return ret;
	}
	oppo_last_pmic_init();
	return 0;
}

core_initcall(boot_oppo_lastpmic_core);

MODULE_DESCRIPTION("OPPO project version");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Wen.luo <Wen.luo@oppo.com>");