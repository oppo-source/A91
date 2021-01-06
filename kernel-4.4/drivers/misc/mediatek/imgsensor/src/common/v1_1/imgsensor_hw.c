/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/delay.h>
#include <linux/string.h>

#include "kd_camera_typedef.h"
#include "kd_camera_feature.h"

#include "imgsensor_sensor.h"
#include "imgsensor_hw.h"

#ifndef VENDOR_EDIT
#define VENDOR_EDIT
#endif

#ifdef VENDOR_EDIT
/*Yijun.Tan@Camera.Driver  add for 17197  board 20180101*/
#include<soc/oppo/oppo_project.h>
#endif

char *imgsensor_sensor_idx_name[IMGSENSOR_SENSOR_IDX_MAX_NUM] = {
    IMGSENSOR_SENSOR_IDX_NAME_MAIN,
    IMGSENSOR_SENSOR_IDX_NAME_SUB,
    IMGSENSOR_SENSOR_IDX_NAME_MAIN2,
    IMGSENSOR_SENSOR_IDX_NAME_SUB2,
    IMGSENSOR_SENSOR_IDX_NAME_MAIN3
};

enum IMGSENSOR_RETURN imgsensor_hw_init(struct IMGSENSOR_HW *phw)
{
    struct IMGSENSOR_HW_SENSOR_POWER      *psensor_pwr;
    struct IMGSENSOR_HW_CFG               *pcust_pwr_cfg;
    struct IMGSENSOR_HW_CUSTOM_POWER_INFO *ppwr_info;
    int i, j;

    for (i = 0; i < IMGSENSOR_HW_ID_MAX_NUM; i++) {
        if (hw_open[i] != NULL)
            (hw_open[i]) (&phw->pdev[i]);

        if (phw->pdev[i]->init != NULL)
            (phw->pdev[i]->init) (phw->pdev[i]->pinstance, &phw->common);
    }


    for (i = 0; i < IMGSENSOR_SENSOR_IDX_MAX_NUM; i++) {
        psensor_pwr = &phw->sensor_pwr[i];

        pcust_pwr_cfg = imgsensor_hw_get_cfg((enum IMGSENSOR_SENSOR_IDX)i);
        if (pcust_pwr_cfg == NULL)
            continue;
        while (pcust_pwr_cfg->sensor_idx != i)
            pcust_pwr_cfg++;

        ppwr_info = pcust_pwr_cfg->pwr_info;
        while (ppwr_info->pin != IMGSENSOR_HW_PIN_NONE) {
            for (j = 0;
                    j < IMGSENSOR_HW_ID_MAX_NUM && ppwr_info->id != phw->pdev[j]->id;
                    j++) {
            }

            psensor_pwr->id[ppwr_info->pin] = j;
            ppwr_info++;
        }
    }

    mutex_init(&phw->common.pinctrl_mutex);

    return IMGSENSOR_RETURN_SUCCESS;
}

enum IMGSENSOR_RETURN imgsensor_hw_release_all(struct IMGSENSOR_HW *phw)
{
    int i;

    for (i = 0; i < IMGSENSOR_HW_ID_MAX_NUM; i++) {
        if (phw->pdev[i]->release != NULL)
            (phw->pdev[i]->release)(phw->pdev[i]->pinstance);
    }
    return IMGSENSOR_RETURN_SUCCESS;
}

#ifdef VENDOR_EDIT
/*weiriqin@Camera.Drv, 20190820, add for pull-up gc02's avdd when main sensor is powered*/
static bool g_is_sub2_gc02m0 = false;
static bool g_is_main3_gc02m0 = false;
void set_gc02m0_flag(enum IMGSENSOR_SENSOR_IDX sensor_idx)
{
    if (sensor_idx == IMGSENSOR_SENSOR_IDX_SUB2)
        g_is_sub2_gc02m0 = true;
    else if (sensor_idx == IMGSENSOR_SENSOR_IDX_MAIN3)
        g_is_main3_gc02m0 = true;
}
/*weiriqin@Camera.Drv, 2019/08/20, add for wait sem to pull rst*/
extern void global_i2c_mutex_lock(void);
extern void global_i2c_mutex_unlock(void);
#endif

static enum IMGSENSOR_RETURN imgsensor_hw_power_sequence(
        struct IMGSENSOR_HW             *phw,
        enum   IMGSENSOR_SENSOR_IDX      sensor_idx,
        enum   IMGSENSOR_HW_POWER_STATUS pwr_status,
        struct IMGSENSOR_HW_POWER_SEQ   *ppower_sequence,
        char *pcurr_idx)
{
    struct IMGSENSOR_HW_SENSOR_POWER *psensor_pwr = &phw->sensor_pwr[sensor_idx];
    struct IMGSENSOR_HW_POWER_SEQ    *ppwr_seq = ppower_sequence;
    struct IMGSENSOR_HW_POWER_INFO   *ppwr_info;
    struct IMGSENSOR_HW_DEVICE       *pdev;
    int                               pin_cnt = 0;

    static DEFINE_RATELIMIT_STATE(ratelimit, 1 * HZ, 30);

    while (ppwr_seq->idx != NULL &&
        ppwr_seq < ppower_sequence + IMGSENSOR_HW_SENSOR_MAX_NUM &&
        strcmp(ppwr_seq->idx, pcurr_idx)) {
        ppwr_seq++;
    }

    if (ppwr_seq->idx == NULL)
        return IMGSENSOR_RETURN_ERROR;

    ppwr_info = ppwr_seq->pwr_info;

    while (ppwr_info->pin != IMGSENSOR_HW_PIN_NONE &&
           ppwr_info < ppwr_seq->pwr_info + IMGSENSOR_HW_POWER_INFO_MAX) {

        if (pwr_status == IMGSENSOR_HW_POWER_STATUS_ON) {
            if (ppwr_info->pin != IMGSENSOR_HW_PIN_UNDEF) {
                pdev = phw->pdev[psensor_pwr->id[ppwr_info->pin]];

                if (__ratelimit(&ratelimit))
                    PK_DBG
                    ("sensor_idx %d, ppwr_info->pin %d, ppwr_info->pin_state_on %d\n",
                    sensor_idx, ppwr_info->pin, ppwr_info->pin_state_on);

                if (pdev->set != NULL) {
                    #ifdef VENDOR_EDIT
                    /*weiriqin@Camera.Drv, 2019/08/20, add for wait sem to pull rst*/
                    if(is_project(OPPO_19531) || is_project(OPPO_19151) || is_project(OPPO_19350)) {
                        if ((sensor_idx == IMGSENSOR_SENSOR_IDX_SUB) && (ppwr_info->pin == IMGSENSOR_HW_PIN_RST) &&
                        (ppwr_info->pin_state_on == Vol_High)) {
                        global_i2c_mutex_lock();
                        mdelay(5);
                        pdev->set(pdev->pinstance,
                            sensor_idx, ppwr_info->pin, ppwr_info->pin_state_on);
                        mdelay(5);
                        global_i2c_mutex_unlock();
                        } else {
                        pdev->set(pdev->pinstance,
                            sensor_idx, ppwr_info->pin, ppwr_info->pin_state_on);
                        }
                    } else {
                        pdev->set(pdev->pinstance,
                        sensor_idx,
                        ppwr_info->pin, ppwr_info->pin_state_on);
                    }
                    #else
                    pdev->set(pdev->pinstance,
                          sensor_idx,
                          ppwr_info->pin, ppwr_info->pin_state_on);
                    #endif
                }
            }

            mdelay(ppwr_info->pin_on_delay);
        }

        ppwr_info++;
        pin_cnt++;
    }

    if (pwr_status == IMGSENSOR_HW_POWER_STATUS_OFF) {
        while (pin_cnt) {
            ppwr_info--;
            pin_cnt--;

            if (__ratelimit(&ratelimit))
                PK_DBG
                ("sensor_idx %d, ppwr_info->pin %d, ppwr_info->pin_state_off %d\n",
                sensor_idx, ppwr_info->pin, ppwr_info->pin_state_off);

            if (ppwr_info->pin != IMGSENSOR_HW_PIN_UNDEF) {
                pdev = phw->pdev[psensor_pwr->id[ppwr_info->pin]];

                if (pdev->set != NULL) {
                    #ifdef VENDOR_EDIT
                    /*weiriqin@Camera.Drv, 2019/08/20, add for wait sem to pull rst*/
                    if(is_project(OPPO_19531) || is_project(OPPO_19151) || is_project(OPPO_19350)) {
                        if ((sensor_idx == IMGSENSOR_SENSOR_IDX_SUB) && (ppwr_info->pin == IMGSENSOR_HW_PIN_RST) &&
                        (ppwr_info->pin_state_on == Vol_Low)) {
                        global_i2c_mutex_lock();
                        mdelay(5);
                        pdev->set(pdev->pinstance,
                            sensor_idx,
                            ppwr_info->pin, ppwr_info->pin_state_off);
                        mdelay(5);
                        global_i2c_mutex_unlock();
                        } else {
                        pdev->set(pdev->pinstance,
                            sensor_idx,
                            ppwr_info->pin, ppwr_info->pin_state_off);
                        }
                    } else {
                                            pdev->set(pdev->pinstance,
                                                 sensor_idx,
                                                 ppwr_info->pin, ppwr_info->pin_state_off);
                    }
                    #else
                                        pdev->set(pdev->pinstance,
                                                 sensor_idx,
                                                 ppwr_info->pin, ppwr_info->pin_state_off);
                    #endif
                }
            }

            mdelay(ppwr_info->pin_on_delay);
        }
    }

    #ifdef VENDOR_EDIT
    /*weiriqin@Camera.Drv, 20190820, add for pull-up gc02's avdd when main sensor is powered*/
    if ((sensor_idx == IMGSENSOR_SENSOR_IDX_MAIN)
        && (is_project(OPPO_19531) || is_project(OPPO_19151) || is_project(OPPO_19350))) {
        if (pwr_status == IMGSENSOR_HW_POWER_STATUS_ON) {//power up
            pdev = phw->pdev[IMGSENSOR_HW_ID_REGULATOR];
            if (g_is_sub2_gc02m0 && pdev->set != NULL) {
                PK_DBG("force sub2_gc02 avdd to high");
                pdev->set(pdev->pinstance, IMGSENSOR_SENSOR_IDX_SUB2, IMGSENSOR_HW_PIN_AVDD, Vol_2800);
            }
            if (g_is_main3_gc02m0 && pdev->set != NULL) {
                PK_DBG("force main3_gc02 avdd to high");
                pdev->set(pdev->pinstance, IMGSENSOR_SENSOR_IDX_MAIN3, IMGSENSOR_HW_PIN_AVDD, Vol_2800);
            }
        } else if (pwr_status == IMGSENSOR_HW_POWER_STATUS_OFF) {//power down
            pdev = phw->pdev[IMGSENSOR_HW_ID_REGULATOR];
            if (g_is_sub2_gc02m0 && pdev->set != NULL) {
                PK_DBG("force sub2_gc02 avdd to low");
                pdev->set(pdev->pinstance, IMGSENSOR_SENSOR_IDX_SUB2, IMGSENSOR_HW_PIN_AVDD, Vol_Low);
            }
            if (g_is_main3_gc02m0 && pdev->set != NULL) {
                PK_DBG("force main3_gc02 avdd to low");
                pdev->set(pdev->pinstance, IMGSENSOR_SENSOR_IDX_MAIN3, IMGSENSOR_HW_PIN_AVDD, Vol_Low);
            }
        }
    }
    #endif

    return IMGSENSOR_RETURN_SUCCESS;
}

enum IMGSENSOR_RETURN imgsensor_hw_power(
        struct IMGSENSOR_HW *phw,
        struct IMGSENSOR_SENSOR *psensor,
        enum IMGSENSOR_HW_POWER_STATUS pwr_status)
{
    enum IMGSENSOR_SENSOR_IDX sensor_idx = psensor->inst.sensor_idx;
    char *curr_sensor_name = psensor->inst.psensor_list->name;

#if defined(CONFIG_IMGSENSOR_MAIN)  || \
        defined(CONFIG_IMGSENSOR_SUB)   || \
        defined(CONFIG_IMGSENSOR_MAIN2) || \
        defined(CONFIG_IMGSENSOR_SUB2)  || \
        defined(CONFIG_IMGSENSOR_MAIN3)

    char *pcustomize_sensor = NULL;

    switch (sensor_idx) {
#ifdef CONFIG_IMGSENSOR_MAIN
    case IMGSENSOR_SENSOR_IDX_MAIN:
        pcustomize_sensor = IMGSENSOR_STRINGIZE(CONFIG_IMGSENSOR_MAIN);
        break;
#endif
#ifdef CONFIG_IMGSENSOR_SUB
    case IMGSENSOR_SENSOR_IDX_SUB:
        pcustomize_sensor = IMGSENSOR_STRINGIZE(CONFIG_IMGSENSOR_SUB);
        break;
#endif
#ifdef CONFIG_IMGSENSOR_MAIN2
    case IMGSENSOR_SENSOR_IDX_MAIN2:
        pcustomize_sensor = IMGSENSOR_STRINGIZE(CONFIG_IMGSENSOR_MAIN2);
        break;
#endif
#ifdef CONFIG_IMGSENSOR_SUB2
    case IMGSENSOR_SENSOR_IDX_SUB2:
        pcustomize_sensor = IMGSENSOR_STRINGIZE(CONFIG_IMGSENSOR_SUB2);
        break;
#endif
#ifdef CONFIG_IMGSENSOR_MAIN3
    case IMGSENSOR_SENSOR_IDX_MAIN3:
        pcustomize_sensor = IMGSENSOR_STRINGIZE(CONFIG_IMGSENSOR_MAIN3);
        break;
#endif
    default:
        break;
    }

    if (pcustomize_sensor &&
            strlen(pcustomize_sensor) > 2 &&
            !strstr(pcustomize_sensor, curr_sensor_name))
        return IMGSENSOR_RETURN_ERROR;
#endif

    PK_DBG("sensor_idx %d, power %d curr_sensor_name %s\n", sensor_idx, pwr_status,
           curr_sensor_name);
        #ifdef VENDOR_EDIT
    /* weiriqin@Camera.Driver add for 19531 19391 special mipi switch 20190521 */
        if (is_project(OPPO_19391) || is_project(OPPO_19531)
            || is_project(OPPO_19151) || is_project(OPPO_19350)) {
            imgsensor_hw_power_sequence(
                    phw,
                    sensor_idx,
                    pwr_status,
                    platform_power_sequence_19391,imgsensor_sensor_idx_name[sensor_idx]);
        } else {
            imgsensor_hw_power_sequence(
                    phw,
                    sensor_idx,
                    pwr_status,
                    platform_power_sequence, imgsensor_sensor_idx_name[sensor_idx]);
        }
        #else
        imgsensor_hw_power_sequence(
                        phw,
                        sensor_idx,
                        pwr_status,
                        platform_power_sequence, imgsensor_sensor_idx_name[sensor_idx]);
        #endif

#ifdef VENDOR_EDIT
    if (is_project(OPPO_18311) || is_project(OPPO_18011)) {
    imgsensor_hw_power_sequence(
            phw,
            sensor_idx,
            pwr_status, sensor_power_sequence_for_18311, curr_sensor_name);
    } else {
    imgsensor_hw_power_sequence(
            phw,
            sensor_idx,
            pwr_status, sensor_power_sequence, curr_sensor_name);
    }
#else
    imgsensor_hw_power_sequence(
            phw,
            sensor_idx,
            pwr_status, sensor_power_sequence, curr_sensor_name);
#endif

    return IMGSENSOR_RETURN_SUCCESS;
}

enum IMGSENSOR_RETURN imgsensor_hw_dump(struct IMGSENSOR_HW *phw)
{
    int i;

    for (i = 0; i < IMGSENSOR_HW_ID_MAX_NUM; i++) {
        if (phw->pdev[i]->dump != NULL)
            (phw->pdev[i]->dump)(phw->pdev[i]->pinstance);
    }
    return IMGSENSOR_RETURN_SUCCESS;
}

struct IMGSENSOR_HW_CFG *imgsensor_hw_get_cfg(enum IMGSENSOR_SENSOR_IDX sensor_idx)
{
    struct IMGSENSOR_HW_CFG *pcust_cfg;
    #ifdef VENDOR_EDIT
        /*Yijun.Tan@Camera.Driver  add for 17197 & 18531  board 20181009*/
        if (is_project(OPPO_17197)) {
            PK_DBG("17197: imgsensor_set_driver_17197\n");
            pcust_cfg = imgsensor_custom_config_for_17197;
        } else if (is_project(OPPO_18531)) {
            PK_DBG("18531: imgsensor_set_driver_18531\n");
            pcust_cfg = imgsensor_custom_config_for_18531;
        } else if (is_project(OPPO_19391) || is_project(OPPO_19531)
            || is_project(OPPO_19151) || is_project(OPPO_19350)) {
            PK_DBG("19531 and 19391: imgsensor_set_driver_19531 or imgsensor_set_driver_19391\n");
            pcust_cfg = imgsensor_custom_config_for_19391;
        } else if (is_project(OPPO_18561)) {
            PK_DBG("18561: imgsensor_set_driver_18561\n");
            pcust_cfg = imgsensor_custom_config_for_18531;
        } else if (is_project(OPPO_18161)) {
            PK_DBG("18161: imgsensor_set_driver_18161\n");
            pcust_cfg = imgsensor_custom_config_for_18531;
        } else if (is_project(OPPO_18311) || is_project(OPPO_18011)) {
            PK_DBG("18311||18011: imgsensor_set_driver_18311\n");
            pcust_cfg = imgsensor_custom_config_for_18311;
        } else if (is_project(OPPO_17331)) {
            PK_DBG("17331: imgsensor_set_driver_17331\n");
            pcust_cfg = imgsensor_custom_config_for_17331;
        } else if (is_project(OPPO_17175)) {
            switch (get_Operator_Version()) {
                case OPERATOR_17171_ASIA_SIMPLE:
                case OPERATOR_17172_ASIA:
                case OPERATOR_17173_ALL_BAND:
                {
                    pcust_cfg = imgsensor_custom_config_for_17171;
                    PK_DBG("17171's pwr_cfg and Operator_Version = %d\n", get_Operator_Version());
                    break;
                }
                default:
                {
                    pcust_cfg = imgsensor_custom_config_for_17175;
                    PK_DBG("17175's pwr_cfg and Operator_Version = %d\n", get_Operator_Version());
                    break;
                }
            }
        } else {
            PK_DBG("normal: imgsensor_set_driver_normal\n");
            pcust_cfg = imgsensor_custom_config;

        }
    #else
        pcust_cfg = imgsensor_custom_config;
    #endif


    while (pcust_cfg->sensor_idx != sensor_idx && pcust_cfg->sensor_idx != IMGSENSOR_SENSOR_IDX_NONE)
        pcust_cfg++;

    return (pcust_cfg->sensor_idx == IMGSENSOR_SENSOR_IDX_NONE) ? NULL : pcust_cfg;
}

