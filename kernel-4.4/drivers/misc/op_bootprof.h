/**************************************************************
* Copyright (c) 201X- 201X OPPO Mobile Comm Corp., Ltd.
*
* File : Op_bootprof.c
* Description: Source file for bootlog.
* Storage the boot log on proc.
* Version : 1.0
* Date : 2018-10-18
* Author : bright.zhang@oppo.com
* ---------------- Revision History: --------------------------
* <version> <date> < author > <desc>
****************************************************************/

/*=============================================================================

                            INCLUDE FILES FOR MODULE

=============================================================================*/
#ifdef VENDOR_EDIT
//Liang.Zhang@PSW.TECH.BOOTUP, 2018/10/19, Add for get bootup log
#ifdef HANG_OPPO_ALL

#define SEQ_printf(m, x...)	    \
	do {			    \
		if (m)		    \
			seq_printf(m, x);	\
		else		    \
			pr_info(x);	    \
	} while (0)

/*
 * Ease the printing of nsec fields:
 */
static long long nsec_high(unsigned long long nsec)
{
	if ((long long)nsec < 0) {
		nsec = -nsec;
		do_div(nsec, 1000000);
		return -nsec;
	}
	do_div(nsec, 1000000);

	return nsec;
}

static unsigned long nsec_low(unsigned long long nsec)
{
	if ((long long)nsec < 0)
		nsec = -nsec;

	return do_div(nsec, 1000000);
}

extern void op_log_boot(char *str);

#endif  //HANG_OPPO_ALL
#endif

