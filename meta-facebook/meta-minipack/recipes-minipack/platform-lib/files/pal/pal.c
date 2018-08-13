/*
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

 /*
  * This file contains functions and logics that depends on Wedge100 specific
  * hardware and kernel drivers. Here, some of the empty "default" functions
  * are overridden with simple functions that returns non-zero error code.
  * This is for preventing any potential escape of failures through the
  * default functions that will return 0 no matter what.
  */

//  #define DEBUG

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include "pal.h"
#include <facebook/bic.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-sensor.h>
#include <openbmc/sensor-correction.h>

typedef struct {
  char name[32];
} sensor_desc_t;

static sensor_desc_t m_snr_desc[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};

/* List of BIC Discrete sensors to be monitored */
const uint8_t bic_discrete_list[] = {
  BIC_SENSOR_SYSTEM_STATUS,
  BIC_SENSOR_PROC_FAIL,
  BIC_SENSOR_SYS_BOOT_STAT,
  BIC_SENSOR_CPU_DIMM_HOT,
  BIC_SENSOR_VR_HOT,
  BIC_SENSOR_POWER_THRESH_EVENT,
  BIC_SENSOR_POST_ERR,
  BIC_SENSOR_POWER_ERR,
  BIC_SENSOR_PROC_HOT_EXT,
  BIC_SENSOR_MACHINE_CHK_ERR,
  BIC_SENSOR_PCIE_ERR,
  BIC_SENSOR_OTHER_IIO_ERR,
  BIC_SENSOR_MEM_ECC_ERR,
  BIC_SENSOR_SPS_FW_HLTH,
  BIC_SENSOR_CAT_ERR,
};

/* List of SCM sensors to be monitored */
const uint8_t scm_sensor_list[] = {
  SCM_SENSOR_OUTLET_LOCAL_TEMP,
  SCM_SENSOR_OUTLET_REMOTE_TEMP,
  SCM_SENSOR_INLET_LOCAL_TEMP,
  SCM_SENSOR_INLET_REMOTE_TEMP,
  SCM_SENSOR_HSC_VOLT,
  SCM_SENSOR_HSC_CURR,
  SCM_SENSOR_HSC_POWER,
};

/* List of SCM and BIC sensors to be monitored */
const uint8_t scm_all_sensor_list[] = {
  SCM_SENSOR_OUTLET_LOCAL_TEMP,
  SCM_SENSOR_OUTLET_REMOTE_TEMP,
  SCM_SENSOR_INLET_LOCAL_TEMP,
  SCM_SENSOR_INLET_REMOTE_TEMP,
  SCM_SENSOR_HSC_VOLT,
  SCM_SENSOR_HSC_CURR,
  SCM_SENSOR_HSC_POWER,
  BIC_SENSOR_MB_OUTLET_TEMP,
  BIC_SENSOR_MB_INLET_TEMP,
  BIC_SENSOR_PCH_TEMP,
  BIC_SENSOR_VCCIN_VR_TEMP,
  BIC_SENSOR_1V05MIX_VR_TEMP,
  BIC_SENSOR_SOC_TEMP,
  BIC_SENSOR_SOC_THERM_MARGIN,
  BIC_SENSOR_VDDR_VR_TEMP,
  BIC_SENSOR_SOC_DIMMA0_TEMP,
  BIC_SENSOR_SOC_DIMMB0_TEMP,
  BIC_SENSOR_SOC_PACKAGE_PWR,
  BIC_SENSOR_VCCIN_VR_POUT,
  BIC_SENSOR_VDDR_VR_POUT,
  BIC_SENSOR_SOC_TJMAX,
  BIC_SENSOR_P3V3_MB,
  BIC_SENSOR_P12V_MB,
  BIC_SENSOR_P1V05_PCH,
  BIC_SENSOR_P3V3_STBY_MB,
  BIC_SENSOR_P5V_STBY_MB,
  BIC_SENSOR_PV_BAT,
  BIC_SENSOR_PVDDR,
  BIC_SENSOR_P1V05_MIX,
  BIC_SENSOR_1V05MIX_VR_CURR,
  BIC_SENSOR_VDDR_VR_CURR,
  BIC_SENSOR_VCCIN_VR_CURR,
  BIC_SENSOR_VCCIN_VR_VOL,
  BIC_SENSOR_VDDR_VR_VOL,
  BIC_SENSOR_P1V05MIX_VR_VOL,
  BIC_SENSOR_P1V05MIX_VR_POUT,
  BIC_SENSOR_INA230_POWER,
  BIC_SENSOR_INA230_VOL,
};

/* List of SMB sensors to be monitored */
const uint8_t smb_sensor_list[] = {
  SMB_SENSOR_1220_VMON1,
  SMB_SENSOR_1220_VMON2,
  SMB_SENSOR_1220_VMON3,
  SMB_SENSOR_1220_VMON4,
  SMB_SENSOR_1220_VMON5,
  SMB_SENSOR_1220_VMON6,
  SMB_SENSOR_1220_VMON7,
  SMB_SENSOR_1220_VMON8,
  SMB_SENSOR_1220_VMON9,
  SMB_SENSOR_1220_VMON10,
  SMB_SENSOR_1220_VMON11,
  SMB_SENSOR_1220_VMON12,
  SMB_SENSOR_1220_VCCA,
  SMB_SENSOR_1220_VCCINP,
  SMB_SENSOR_TH3_SERDES_VOLT,
  SMB_SENSOR_TH3_SERDES_CURR,
  SMB_SENSOR_TH3_SERDES_TEMP,
  SMB_SENSOR_TH3_CORE_VOLT,
  SMB_SENSOR_TH3_CORE_CURR,
  SMB_SENSOR_TH3_CORE_TEMP,
  SMB_SENSOR_TEMP1,
  SMB_SENSOR_TEMP2,
  SMB_SENSOR_TEMP3,
  SMB_SENSOR_TEMP4,
  SMB_SENSOR_TEMP5,
  SMB_SENSOR_TH3_DIE_TEMP1,
  SMB_SENSOR_TH3_DIE_TEMP2,
  /* Sensors on PDB */
  SMB_SENSOR_PDB_L_TEMP1,
  SMB_SENSOR_PDB_L_TEMP2,
  SMB_SENSOR_PDB_R_TEMP1,
  SMB_SENSOR_PDB_R_TEMP2,
  /* Sensors on FCM */
  SMB_SENSOR_FCM_T_TEMP1,
  SMB_SENSOR_FCM_T_TEMP2,
  SMB_SENSOR_FCM_B_TEMP1,
  SMB_SENSOR_FCM_B_TEMP2,
  SMB_SENSOR_FCM_T_HSC_VOLT,
  SMB_SENSOR_FCM_T_HSC_CURR,
  SMB_SENSOR_FCM_T_HSC_POWER,
  SMB_SENSOR_FCM_B_HSC_VOLT,
  SMB_SENSOR_FCM_B_HSC_CURR,
  SMB_SENSOR_FCM_B_HSC_POWER,
  /* Sensors FAN Speed */
  SMB_SENSOR_FAN1_FRONT_TACH,
  SMB_SENSOR_FAN1_REAR_TACH,
  SMB_SENSOR_FAN2_FRONT_TACH,
  SMB_SENSOR_FAN2_REAR_TACH,
  SMB_SENSOR_FAN3_FRONT_TACH,
  SMB_SENSOR_FAN3_REAR_TACH,
  SMB_SENSOR_FAN4_FRONT_TACH,
  SMB_SENSOR_FAN4_REAR_TACH,
  SMB_SENSOR_FAN5_FRONT_TACH,
  SMB_SENSOR_FAN5_REAR_TACH,
  SMB_SENSOR_FAN6_FRONT_TACH,
  SMB_SENSOR_FAN6_REAR_TACH,
  SMB_SENSOR_FAN7_FRONT_TACH,
  SMB_SENSOR_FAN7_REAR_TACH,
  SMB_SENSOR_FAN8_FRONT_TACH,
  SMB_SENSOR_FAN8_REAR_TACH,
};

const uint8_t pim1_sensor_list[] = {
  PIM1_SENSOR_TEMP1,
  PIM1_SENSOR_TEMP2,
  PIM1_SENSOR_HSC_VOLT,
  PIM1_SENSOR_HSC_CURR,
  PIM1_SENSOR_HSC_POWER,
  PIM1_SENSOR_34461_VOLT1,
  PIM1_SENSOR_34461_VOLT2,
  PIM1_SENSOR_34461_VOLT3,
  PIM1_SENSOR_34461_VOLT4,
  PIM1_SENSOR_34461_VOLT5,
  PIM1_SENSOR_34461_VOLT6,
  PIM1_SENSOR_34461_VOLT7,
  PIM1_SENSOR_34461_VOLT8,
  PIM1_SENSOR_34461_VOLT9,
  PIM1_SENSOR_34461_VOLT10,
  PIM1_SENSOR_34461_VOLT11,
  PIM1_SENSOR_34461_VOLT12,
  PIM1_SENSOR_34461_VOLT13,
  PIM1_SENSOR_34461_VOLT14,
  PIM1_SENSOR_34461_VOLT15,
  PIM1_SENSOR_34461_VOLT16,
};

const uint8_t pim2_sensor_list[] = {
  PIM2_SENSOR_TEMP1,
  PIM2_SENSOR_TEMP2,
  PIM2_SENSOR_HSC_VOLT,
  PIM2_SENSOR_HSC_CURR,
  PIM2_SENSOR_HSC_POWER,
  PIM2_SENSOR_34461_VOLT1,
  PIM2_SENSOR_34461_VOLT2,
  PIM2_SENSOR_34461_VOLT3,
  PIM2_SENSOR_34461_VOLT4,
  PIM2_SENSOR_34461_VOLT5,
  PIM2_SENSOR_34461_VOLT6,
  PIM2_SENSOR_34461_VOLT7,
  PIM2_SENSOR_34461_VOLT8,
  PIM2_SENSOR_34461_VOLT9,
  PIM2_SENSOR_34461_VOLT10,
  PIM2_SENSOR_34461_VOLT11,
  PIM2_SENSOR_34461_VOLT12,
  PIM2_SENSOR_34461_VOLT13,
  PIM2_SENSOR_34461_VOLT14,
  PIM2_SENSOR_34461_VOLT15,
  PIM2_SENSOR_34461_VOLT16,
};

const uint8_t pim3_sensor_list[] = {
  PIM3_SENSOR_TEMP1,
  PIM3_SENSOR_TEMP2,
  PIM3_SENSOR_HSC_VOLT,
  PIM3_SENSOR_HSC_CURR,
  PIM3_SENSOR_HSC_POWER,
  PIM3_SENSOR_34461_VOLT1,
  PIM3_SENSOR_34461_VOLT2,
  PIM3_SENSOR_34461_VOLT3,
  PIM3_SENSOR_34461_VOLT4,
  PIM3_SENSOR_34461_VOLT5,
  PIM3_SENSOR_34461_VOLT6,
  PIM3_SENSOR_34461_VOLT7,
  PIM3_SENSOR_34461_VOLT8,
  PIM3_SENSOR_34461_VOLT9,
  PIM3_SENSOR_34461_VOLT10,
  PIM3_SENSOR_34461_VOLT11,
  PIM3_SENSOR_34461_VOLT12,
  PIM3_SENSOR_34461_VOLT13,
  PIM3_SENSOR_34461_VOLT14,
  PIM3_SENSOR_34461_VOLT15,
  PIM3_SENSOR_34461_VOLT16,
};

const uint8_t pim4_sensor_list[] = {
  PIM4_SENSOR_TEMP1,
  PIM4_SENSOR_TEMP2,
  PIM4_SENSOR_HSC_VOLT,
  PIM4_SENSOR_HSC_CURR,
  PIM4_SENSOR_HSC_POWER,
  PIM4_SENSOR_34461_VOLT1,
  PIM4_SENSOR_34461_VOLT2,
  PIM4_SENSOR_34461_VOLT3,
  PIM4_SENSOR_34461_VOLT4,
  PIM4_SENSOR_34461_VOLT5,
  PIM4_SENSOR_34461_VOLT6,
  PIM4_SENSOR_34461_VOLT7,
  PIM4_SENSOR_34461_VOLT8,
  PIM4_SENSOR_34461_VOLT9,
  PIM4_SENSOR_34461_VOLT10,
  PIM4_SENSOR_34461_VOLT11,
  PIM4_SENSOR_34461_VOLT12,
  PIM4_SENSOR_34461_VOLT13,
  PIM4_SENSOR_34461_VOLT14,
  PIM4_SENSOR_34461_VOLT15,
  PIM4_SENSOR_34461_VOLT16,
};

const uint8_t pim5_sensor_list[] = {
  PIM5_SENSOR_TEMP1,
  PIM5_SENSOR_TEMP2,
  PIM5_SENSOR_HSC_VOLT,
  PIM5_SENSOR_HSC_CURR,
  PIM5_SENSOR_HSC_POWER,
  PIM5_SENSOR_34461_VOLT1,
  PIM5_SENSOR_34461_VOLT2,
  PIM5_SENSOR_34461_VOLT3,
  PIM5_SENSOR_34461_VOLT4,
  PIM5_SENSOR_34461_VOLT5,
  PIM5_SENSOR_34461_VOLT6,
  PIM5_SENSOR_34461_VOLT7,
  PIM5_SENSOR_34461_VOLT8,
  PIM5_SENSOR_34461_VOLT9,
  PIM5_SENSOR_34461_VOLT10,
  PIM5_SENSOR_34461_VOLT11,
  PIM5_SENSOR_34461_VOLT12,
  PIM5_SENSOR_34461_VOLT13,
  PIM5_SENSOR_34461_VOLT14,
  PIM5_SENSOR_34461_VOLT15,
  PIM5_SENSOR_34461_VOLT16,
};

const uint8_t pim6_sensor_list[] = {
  PIM6_SENSOR_TEMP1,
  PIM6_SENSOR_TEMP2,
  PIM6_SENSOR_HSC_VOLT,
  PIM6_SENSOR_HSC_CURR,
  PIM6_SENSOR_HSC_POWER,
  PIM6_SENSOR_34461_VOLT1,
  PIM6_SENSOR_34461_VOLT2,
  PIM6_SENSOR_34461_VOLT3,
  PIM6_SENSOR_34461_VOLT4,
  PIM6_SENSOR_34461_VOLT5,
  PIM6_SENSOR_34461_VOLT6,
  PIM6_SENSOR_34461_VOLT7,
  PIM6_SENSOR_34461_VOLT8,
  PIM6_SENSOR_34461_VOLT9,
  PIM6_SENSOR_34461_VOLT10,
  PIM6_SENSOR_34461_VOLT11,
  PIM6_SENSOR_34461_VOLT12,
  PIM6_SENSOR_34461_VOLT13,
  PIM6_SENSOR_34461_VOLT14,
  PIM6_SENSOR_34461_VOLT15,
  PIM6_SENSOR_34461_VOLT16,
};

const uint8_t pim7_sensor_list[] = {
  PIM7_SENSOR_TEMP1,
  PIM7_SENSOR_TEMP2,
  PIM7_SENSOR_HSC_VOLT,
  PIM7_SENSOR_HSC_CURR,
  PIM7_SENSOR_HSC_POWER,
  PIM7_SENSOR_34461_VOLT1,
  PIM7_SENSOR_34461_VOLT2,
  PIM7_SENSOR_34461_VOLT3,
  PIM7_SENSOR_34461_VOLT4,
  PIM7_SENSOR_34461_VOLT5,
  PIM7_SENSOR_34461_VOLT6,
  PIM7_SENSOR_34461_VOLT7,
  PIM7_SENSOR_34461_VOLT8,
  PIM7_SENSOR_34461_VOLT9,
  PIM7_SENSOR_34461_VOLT10,
  PIM7_SENSOR_34461_VOLT11,
  PIM7_SENSOR_34461_VOLT12,
  PIM7_SENSOR_34461_VOLT13,
  PIM7_SENSOR_34461_VOLT14,
  PIM7_SENSOR_34461_VOLT15,
  PIM7_SENSOR_34461_VOLT16,
};

const uint8_t pim8_sensor_list[] = {
  PIM8_SENSOR_TEMP1,
  PIM8_SENSOR_TEMP2,
  PIM8_SENSOR_HSC_VOLT,
  PIM8_SENSOR_HSC_CURR,
  PIM8_SENSOR_HSC_POWER,
  PIM8_SENSOR_34461_VOLT1,
  PIM8_SENSOR_34461_VOLT2,
  PIM8_SENSOR_34461_VOLT3,
  PIM8_SENSOR_34461_VOLT4,
  PIM8_SENSOR_34461_VOLT5,
  PIM8_SENSOR_34461_VOLT6,
  PIM8_SENSOR_34461_VOLT7,
  PIM8_SENSOR_34461_VOLT8,
  PIM8_SENSOR_34461_VOLT9,
  PIM8_SENSOR_34461_VOLT10,
  PIM8_SENSOR_34461_VOLT11,
  PIM8_SENSOR_34461_VOLT12,
  PIM8_SENSOR_34461_VOLT13,
  PIM8_SENSOR_34461_VOLT14,
  PIM8_SENSOR_34461_VOLT15,
  PIM8_SENSOR_34461_VOLT16,
};

const uint8_t psu1_sensor_list[] = {
  PSU1_SENSOR_IN_VOLT,
  PSU1_SENSOR_12V_VOLT,
  PSU1_SENSOR_STBY_VOLT,
  PSU1_SENSOR_IN_CURR,
  PSU1_SENSOR_12V_CURR,
  PSU1_SENSOR_STBY_CURR,
  PSU1_SENSOR_IN_POWER,
  PSU1_SENSOR_12V_POWER,
  PSU1_SENSOR_STBY_POWER,
  PSU1_SENSOR_FAN_TACH,
  PSU1_SENSOR_TEMP1,
  PSU1_SENSOR_TEMP2,
  PSU1_SENSOR_TEMP3,
};

const uint8_t psu2_sensor_list[] = {
  PSU2_SENSOR_IN_VOLT,
  PSU2_SENSOR_12V_VOLT,
  PSU2_SENSOR_STBY_VOLT,
  PSU2_SENSOR_IN_CURR,
  PSU2_SENSOR_12V_CURR,
  PSU2_SENSOR_STBY_CURR,
  PSU2_SENSOR_IN_POWER,
  PSU2_SENSOR_12V_POWER,
  PSU2_SENSOR_STBY_POWER,
  PSU2_SENSOR_FAN_TACH,
  PSU2_SENSOR_TEMP1,
  PSU2_SENSOR_TEMP2,
  PSU2_SENSOR_TEMP3,
};

const uint8_t psu3_sensor_list[] = {
  PSU3_SENSOR_IN_VOLT,
  PSU3_SENSOR_12V_VOLT,
  PSU3_SENSOR_STBY_VOLT,
  PSU3_SENSOR_IN_CURR,
  PSU3_SENSOR_12V_CURR,
  PSU3_SENSOR_STBY_CURR,
  PSU3_SENSOR_IN_POWER,
  PSU3_SENSOR_12V_POWER,
  PSU3_SENSOR_STBY_POWER,
  PSU3_SENSOR_FAN_TACH,
  PSU3_SENSOR_TEMP1,
  PSU3_SENSOR_TEMP2,
  PSU3_SENSOR_TEMP3,
};

const uint8_t psu4_sensor_list[] = {
  PSU4_SENSOR_IN_VOLT,
  PSU4_SENSOR_12V_VOLT,
  PSU4_SENSOR_STBY_VOLT,
  PSU4_SENSOR_IN_CURR,
  PSU4_SENSOR_12V_CURR,
  PSU4_SENSOR_STBY_CURR,
  PSU4_SENSOR_IN_POWER,
  PSU4_SENSOR_12V_POWER,
  PSU4_SENSOR_STBY_POWER,
  PSU4_SENSOR_FAN_TACH,
  PSU4_SENSOR_TEMP1,
  PSU4_SENSOR_TEMP2,
  PSU4_SENSOR_TEMP3,
};

float scm_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float smb_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float pim_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};
float psu_sensor_threshold[MAX_SENSOR_NUM][MAX_SENSOR_THRESHOLD + 1] = {0};

size_t bic_discrete_cnt = sizeof(bic_discrete_list)/sizeof(uint8_t);
size_t scm_sensor_cnt = sizeof(scm_sensor_list)/sizeof(uint8_t);
size_t scm_all_sensor_cnt = sizeof(scm_all_sensor_list)/sizeof(uint8_t);
size_t smb_sensor_cnt = sizeof(smb_sensor_list)/sizeof(uint8_t);
size_t pim1_sensor_cnt = sizeof(pim1_sensor_list)/sizeof(uint8_t);
size_t pim2_sensor_cnt = sizeof(pim2_sensor_list)/sizeof(uint8_t);
size_t pim3_sensor_cnt = sizeof(pim3_sensor_list)/sizeof(uint8_t);
size_t pim4_sensor_cnt = sizeof(pim4_sensor_list)/sizeof(uint8_t);
size_t pim5_sensor_cnt = sizeof(pim5_sensor_list)/sizeof(uint8_t);
size_t pim6_sensor_cnt = sizeof(pim6_sensor_list)/sizeof(uint8_t);
size_t pim7_sensor_cnt = sizeof(pim7_sensor_list)/sizeof(uint8_t);
size_t pim8_sensor_cnt = sizeof(pim8_sensor_list)/sizeof(uint8_t);
size_t psu1_sensor_cnt = sizeof(psu1_sensor_list)/sizeof(uint8_t);
size_t psu2_sensor_cnt = sizeof(psu2_sensor_list)/sizeof(uint8_t);
size_t psu3_sensor_cnt = sizeof(psu3_sensor_list)/sizeof(uint8_t);
size_t psu4_sensor_cnt = sizeof(psu4_sensor_list)/sizeof(uint8_t);

static sensor_info_t g_sinfo[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};

const char pal_fru_list[] = "all, scm, smb, pim1, pim2, pim3, \
pim4, pim5, pim6, pim7, pim8, psu1, psu2, psu3, psu4";

char * key_list[] = {
"pwr_server1_last_state",
"sysfw_ver_slot1",
"timestamp_sled",
/* Add more Keys here */
LAST_KEY /* This is the last key of the list */
};

char * def_val_list[] = {
  "on", /* pwr_server1_last_state */
  "0", /* sysfw_ver_slot1 */
  "0", /* timestamp_sled */
/* Add more Keys here */
  LAST_KEY /* Same as last entry of the key_list */
};

// Helper Functions
static int
read_device(const char *device, int *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", device);
#endif
    return err;
  }

  rc = fscanf(fp, "%i", value);
  fclose(fp);
  if (rc != 1) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to read device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

static int
write_device(const char *device, const char *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "w");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device for write %s", device);
#endif
    return err;
  }

  rc = fputs(value, fp);
  fclose(fp);

  if (rc < 0) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to write device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

static int
i2c_device_detect(uint8_t bus_num, uint8_t addr) {
  int fd = -1, rc = -1;
  char fn[32];

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus_num);
  fd = open(fn, O_RDWR);
  if (fd == -1) {
    syslog(LOG_WARNING, "Failed to open i2c device %s", fn);
    return -1;
  }

  rc = ioctl(fd, I2C_SLAVE, addr);
  if (rc < 0) {
    syslog(LOG_WARNING, "Failed to open slave @ address 0x%x", addr);
    close(fd);
    return -1;
  }

  rc = i2c_smbus_read_byte(fd);
  close(fd);

  if (rc < 0) {
    return -1;
  } else {
    return 0;
  }
}

/* -1->unknown; 0->16Q; 1->4DD */
int
pal_get_pim_type(uint8_t fru){
  int ret = -1;

  switch(fru) {
    case FRU_PIM1:
      if (!i2c_device_detect(80, 0x60)) {
        ret = 0;
      } else if (!i2c_device_detect(80, 0x61)) {
        ret = 1;
      } else {
        ret = -1;
      }
      break;
    case FRU_PIM2:
      if (!i2c_device_detect(88, 0x60)) {
        ret = 0;
      } else if (!i2c_device_detect(88, 0x61)) {
        ret = 1;
      } else {
        ret = -1;
      }
      break;
    case FRU_PIM3:
      if (!i2c_device_detect(96, 0x60)) {
        ret = 0;
      } else if (!i2c_device_detect(96, 0x61)) {
        ret = 1;
      } else {
        ret = -1;
      }
      break;
    case FRU_PIM4:
      if (!i2c_device_detect(104, 0x60)) {
        ret = 0;
      } else if (!i2c_device_detect(104, 0x61)) {
        ret = 1;
      } else {
        ret = -1;
      }
      break;
    case FRU_PIM5:
      if (!i2c_device_detect(112, 0x60)) {
        ret = 0;
      } else if (!i2c_device_detect(112, 0x61)) {
        ret = 1;
      } else {
        ret = -1;
      }
      break;
    case FRU_PIM6:
      if (!i2c_device_detect(120, 0x60)) {
        ret = 0;
      } else if (!i2c_device_detect(120, 0x61)) {
        ret = 1;
      } else {
        ret = -1;
      }
      break;
    case FRU_PIM7:
      if (!i2c_device_detect(128, 0x60)) {
        ret = 0;
      } else if (!i2c_device_detect(128, 0x61)) {
        ret = 1;
      } else {
        ret = -1;
      }
      break;
    case FRU_PIM8:
      if (!i2c_device_detect(136, 0x60)) {
        ret = 0;
      } else if (!i2c_device_detect(136, 0x61)) {
        ret = 1;
      } else {
        ret = -1;
      }
      break;
    default:
      return -1;
  }
  return ret;
}

void
pal_inform_bic_mode(uint8_t fru, uint8_t mode) {
  switch(mode) {
  case BIC_MODE_NORMAL:
    // Bridge IC entered normal mode
    // Inform BIOS that BMC is ready
    bic_set_gpio(fru, BMC_READY_N, 0);
    break;
  case BIC_MODE_UPDATE:
    // Bridge IC entered update mode
    // TODO: Might need to handle in future
    break;
  default:
    break;
  }
}

//For OEM command "CMD_OEM_GET_PLAT_INFO" 0x7e
int
pal_get_plat_sku_id(void){
  return 0x06; // Minipack
}


//Use part of the function for OEM Command "CMD_OEM_GET_POSS_PCIE_CONFIG" 0xF4
int
pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len,
                         uint8_t *res_data, uint8_t *res_len) {

  uint8_t completion_code = CC_SUCCESS;
  uint8_t pcie_conf = 0x02;//Minipack
  uint8_t *data = res_data;

  *data++ = pcie_conf;
  *res_len = data - res_data;
  return completion_code;
}

static int
pal_key_check(char *key) {
  int i;

  i = 0;
  while(strcmp(key_list[i], LAST_KEY)) {
    // If Key is valid, return success
    if (!strcmp(key, key_list[i]))
      return 0;

    i++;
  }

#ifdef DEBUG
  syslog(LOG_WARNING, "pal_key_check: invalid key - %s", key);
#endif
  return -1;
}

int
pal_get_key_value(char *key, char *value) {
  int ret;
  // Check is key is defined and valid
  if (pal_key_check(key))
    return -1;

  ret = kv_get(key, value, NULL, KV_FPERSIST);
  return ret;
}

int
pal_set_key_value(char *key, char *value) {

  // Check is key is defined and valid
  if (pal_key_check(key))
    return -1;

  return kv_set(key, value, 0, KV_FPERSIST);
}

int
pal_set_sysfw_ver(uint8_t slot, uint8_t *ver) {
  int i;
  char key[MAX_KEY_LEN];
  char str[MAX_VALUE_LEN] = {0};
  char tstr[10];
  sprintf(key, "sysfw_ver_slot%d", (int) slot);

  for (i = 0; i < SIZE_SYSFW_VER; i++) {
    sprintf(tstr, "%02x", ver[i]);
    strcat(str, tstr);
  }

  return pal_set_key_value(key, str);
}

int
pal_get_sysfw_ver(uint8_t slot, uint8_t *ver) {
  int i;
  int j = 0;
  int ret;
  int msb, lsb;
  char key[MAX_KEY_LEN];
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4];

  sprintf(key, "sysfw_ver_slot%d", (int) slot);

  ret = pal_get_key_value(key, str);
  if (ret) {
    return ret;
  }

  for (i = 0; i < 2*SIZE_SYSFW_VER; i += 2) {
    sprintf(tstr, "%c\n", str[i]);
    msb = strtol(tstr, NULL, 16);

    sprintf(tstr, "%c\n", str[i+1]);
    lsb = strtol(tstr, NULL, 16);
    ver[j++] = (msb << 4) | lsb;
  }

  return 0;
}

int
pal_get_fru_list(char *list) {
  strcpy(list, pal_fru_list);
  return 0;
}

int
pal_get_fru_id(char *str, uint8_t *fru) {
  if (!strcmp(str, "all")) {
    *fru = FRU_ALL;
  } else if (!strcmp(str, "smb")) {
    *fru = FRU_SMB;
  } else if (!strcmp(str, "scm")) {
    *fru = FRU_SCM;
  } else if (!strcmp(str, "pim1")) {
    *fru = FRU_PIM1;
  } else if (!strcmp(str, "pim2")) {
    *fru = FRU_PIM2;
  } else if (!strcmp(str, "pim3")) {
    *fru = FRU_PIM3;
  } else if (!strcmp(str, "pim4")) {
    *fru = FRU_PIM4;
  } else if (!strcmp(str, "pim5")) {
    *fru = FRU_PIM5;
  } else if (!strcmp(str, "pim6")) {
    *fru = FRU_PIM6;
  } else if (!strcmp(str, "pim7")) {
    *fru = FRU_PIM7;
  } else if (!strcmp(str, "pim8")) {
    *fru = FRU_PIM8;
  } else if (!strcmp(str, "psu1")) {
    *fru = FRU_PSU1;
  } else if (!strcmp(str, "psu2")) {
    *fru = FRU_PSU2;
  } else if (!strcmp(str, "psu3")) {
    *fru = FRU_PSU3;
  } else if (!strcmp(str, "psu4")) {
    *fru = FRU_PSU4;
  } else if (!strcmp(str, "fan1")) {
    *fru = FRU_FAN1;
  } else if (!strcmp(str, "fan2")) {
    *fru = FRU_FAN2;
  } else if (!strcmp(str, "fan3")) {
    *fru = FRU_FAN3;
  } else if (!strcmp(str, "fan4")) {
    *fru = FRU_FAN4;
  } else if (!strcmp(str, "fan5")) {
    *fru = FRU_FAN5;
  } else if (!strcmp(str, "fan6")) {
    *fru = FRU_FAN6;
  } else if (!strcmp(str, "fan7")) {
    *fru = FRU_FAN7;
  } else if (!strcmp(str, "fan8")) {
    *fru = FRU_FAN8;
  } else {
    syslog(LOG_WARNING, "pal_get_fru_id: Wrong fru#%s", str);
    return -1;
  }

  return 0;
}

int
pal_get_fru_name(uint8_t fru, char *name) {
  switch(fru) {
    case FRU_SMB:
      strcpy(name, "smb");
      break;
    case FRU_SCM:
      strcpy(name, "scm");
      break;
    case FRU_PIM1:
      strcpy(name, "pim1");
      break;
    case FRU_PIM2:
      strcpy(name, "pim2");
      break;
    case FRU_PIM3:
      strcpy(name, "pim3");
      break;
    case FRU_PIM4:
      strcpy(name, "pim4");
      break;
    case FRU_PIM5:
      strcpy(name, "pim5");
      break;
    case FRU_PIM6:
      strcpy(name, "pim6");
      break;
    case FRU_PIM7:
      strcpy(name, "pim7");
      break;
    case FRU_PIM8:
      strcpy(name, "pim8");
      break;
    case FRU_PSU1:
      strcpy(name, "psu1");
      break;
    case FRU_PSU2:
      strcpy(name, "psu2");
      break;
    case FRU_PSU3:
      strcpy(name, "psu3");
      break;
    case FRU_PSU4:
      strcpy(name, "psu4");
      break;
    case FRU_FAN1:
      strcpy(name, "fan1");
      break;
    case FRU_FAN2:
      strcpy(name, "fan2");
      break;
    case FRU_FAN3:
      strcpy(name, "fan3");
      break;
    case FRU_FAN4:
      strcpy(name, "fan4");
      break;
    case FRU_FAN5:
      strcpy(name, "fan5");
      break;
    case FRU_FAN6:
      strcpy(name, "fan6");
      break;
    case FRU_FAN7:
      strcpy(name, "fan7");
      break;
    case FRU_FAN8:
      strcpy(name, "fan8");
      break;
    default:
      if (fru > MAX_NUM_FRUS)
        return -1;
      sprintf(name, "fru%d", fru);
      break;
  }
  return 0;
}

// Platform Abstraction Layer (PAL) Functions
int
pal_get_platform_name(char *name) {
  strcpy(name, PLATFORM_NAME);

  return 0;
}

int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {
  int val;
  char tmp[LARGEST_DEVICE_NAME];
  char path[LARGEST_DEVICE_NAME + 1];
  *status = 0;

  switch (fru) {
    case FRU_SMB:
      *status = 1;
      return 0;
    case FRU_SCM:
      snprintf(path, LARGEST_DEVICE_NAME, SMB_SYSFS, SCM_PRSNT_STATUS);
      break;
    case FRU_PIM1:
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
      snprintf(tmp, LARGEST_DEVICE_NAME, SMB_SYSFS, PIM_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, fru - 2);
      break;
    case FRU_PSU1:
      snprintf(tmp, LARGEST_DEVICE_NAME, SMB_SYSFS, PSU_L_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, 2);
      break;
    case FRU_PSU2:
      snprintf(tmp, LARGEST_DEVICE_NAME, SMB_SYSFS, PSU_L_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, 1);
      break;
    case FRU_PSU3:
      snprintf(tmp, LARGEST_DEVICE_NAME, SMB_SYSFS, PSU_R_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, 2);
      break;
    case FRU_PSU4:
      snprintf(tmp, LARGEST_DEVICE_NAME, SMB_SYSFS, PSU_R_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, 1);
      break;
    case FRU_FAN1:
    case FRU_FAN3:
    case FRU_FAN5:
    case FRU_FAN7:
      snprintf(tmp, LARGEST_DEVICE_NAME, FCM_T_SYSFS, FAN_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, ((fru - 14) / 2) + 1);
      break;
    case FRU_FAN2:
    case FRU_FAN4:
    case FRU_FAN6:
    case FRU_FAN8:
      snprintf(tmp, LARGEST_DEVICE_NAME, FCM_B_SYSFS, FAN_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, (fru - 14) / 2);
      break;
    default:
      return -1;
    }

    if (read_device(path, &val)) {
      return -1;
    }

    if (val == 0x0) {
      *status = 1;
    } else {
      *status = 0;
    }
  return 0;
}

int
pal_is_fru_ready(uint8_t fru, uint8_t *status) {
  *status = 1;

  return 0;
}

int
pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  switch(fru) {
  case FRU_SCM:
    *sensor_list = (uint8_t *) scm_all_sensor_list;
    *cnt = scm_all_sensor_cnt;
    break;
  case FRU_SMB:
    *sensor_list = (uint8_t *) smb_sensor_list;
    *cnt = smb_sensor_cnt;
    break;
  case FRU_PIM1:
    *sensor_list = (uint8_t *) pim1_sensor_list;
    *cnt = pim1_sensor_cnt;
    break;
  case FRU_PIM2:
    *sensor_list = (uint8_t *) pim2_sensor_list;
    *cnt = pim2_sensor_cnt;
    break;
  case FRU_PIM3:
    *sensor_list = (uint8_t *) pim3_sensor_list;
    *cnt = pim3_sensor_cnt;
    break;
  case FRU_PIM4:
    *sensor_list = (uint8_t *) pim4_sensor_list;
    *cnt = pim4_sensor_cnt;
    break;
  case FRU_PIM5:
    *sensor_list = (uint8_t *) pim5_sensor_list;
    *cnt = pim5_sensor_cnt;
    break;
  case FRU_PIM6:
    *sensor_list = (uint8_t *) pim6_sensor_list;
    *cnt = pim6_sensor_cnt;
    break;
  case FRU_PIM7:
    *sensor_list = (uint8_t *) pim7_sensor_list;
    *cnt = pim7_sensor_cnt;
    break;
  case FRU_PIM8:
    *sensor_list = (uint8_t *) pim8_sensor_list;
    *cnt = pim8_sensor_cnt;
    break;
  case FRU_PSU1:
    *sensor_list = (uint8_t *) psu1_sensor_list;
    *cnt = psu1_sensor_cnt;
    break;
  case FRU_PSU2:
    *sensor_list = (uint8_t *) psu2_sensor_list;
    *cnt = psu2_sensor_cnt;
    break;
  case FRU_PSU3:
    *sensor_list = (uint8_t *) psu3_sensor_list;
    *cnt = psu3_sensor_cnt;
    break;
  case FRU_PSU4:
    *sensor_list = (uint8_t *) psu4_sensor_list;
    *cnt = psu4_sensor_cnt;
    break;
  default:
    if (fru > MAX_NUM_FRUS)
      return -1;
    // Nothing to read yet.
    *sensor_list = NULL;
    *cnt = 0;
  }
  return 0;
}

void
pal_update_ts_sled()
{
  char key[MAX_KEY_LEN];
  char tstr[MAX_VALUE_LEN];
  struct timespec ts;

  clock_gettime(CLOCK_REALTIME, &ts);
  sprintf(tstr, "%ld", ts.tv_sec);

  sprintf(key, "timestamp_sled");

  pal_set_key_value(key, tstr);
}

int
pal_get_fru_discrete_list(uint8_t fru, uint8_t **sensor_list, int *cnt) {
  switch(fru) {
  case FRU_SCM:
    *sensor_list = (uint8_t *) bic_discrete_list;
    *cnt = bic_discrete_cnt;
    break;
  default:
    if (fru > MAX_NUM_FRUS)
      return -1;
    // Nothing to read yet.
    *sensor_list = NULL;
    *cnt = 0;
  }
    return 0;
}

static void
_print_sensor_discrete_log(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t val, char *event) {
  if (val) {
    syslog(LOG_CRIT, "ASSERT: %s discrete - raised - FRU: %d, num: 0x%X,"
        " snr: %-16s val: %d", event, fru, snr_num, snr_name, val);
  } else {
    syslog(LOG_CRIT, "DEASSERT: %s discrete - settled - FRU: %d, num: 0x%X,"
        " snr: %-16s val: %d", event, fru, snr_num, snr_name, val);
  }
  pal_update_ts_sled();
}

int
pal_sensor_discrete_check(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t o_val, uint8_t n_val) {

  char name[32], crisel[128];
  bool valid = false;
  uint8_t diff = o_val ^ n_val;

  if (GETBIT(diff, 0)) {
    switch(snr_num) {
      case BIC_SENSOR_SYSTEM_STATUS:
        sprintf(name, "SOC_Thermal_Trip");
        valid = true;

        sprintf(crisel, "%s - %s,FRU:%u",
                        name, GETBIT(n_val, 0)?"ASSERT":"DEASSERT", fru);
        pal_add_cri_sel(crisel);
        break;
      case BIC_SENSOR_VR_HOT:
        sprintf(name, "SOC_VR_Hot");
        valid = true;
        break;
    }
    if (valid) {
      _print_sensor_discrete_log(fru, snr_num, snr_name,
                                      GETBIT(n_val, 0), name);
      valid = false;
    }
  }

  if (GETBIT(diff, 1)) {
    switch(snr_num) {
      case BIC_SENSOR_SYSTEM_STATUS:
        sprintf(name, "SOC_FIVR_Fault");
        valid = true;
        break;
      case BIC_SENSOR_VR_HOT:
        sprintf(name, "SOC_DIMM_AB_VR_Hot");
        valid = true;
        break;
      case BIC_SENSOR_CPU_DIMM_HOT:
        sprintf(name, "SOC_MEMHOT");
        valid = true;
        break;
    }
    if (valid) {
      _print_sensor_discrete_log(fru, snr_num, snr_name,
                                      GETBIT(n_val, 1), name);
      valid = false;
    }
  }

  if (GETBIT(diff, 2)) {
    switch(snr_num) {
      case BIC_SENSOR_SYSTEM_STATUS:
        sprintf(name, "SYS_Throttle");
        valid = true;
        break;
      case BIC_SENSOR_VR_HOT:
        sprintf(name, "SOC_DIMM_DE_VR_Hot");
        valid = true;
        break;
    }
    if (valid) {
      _print_sensor_discrete_log(fru, snr_num, snr_name,
                                      GETBIT(n_val, 2), name);
      valid = false;
    }
  }

  if (GETBIT(diff, 4)) {
    if (snr_num == BIC_SENSOR_PROC_FAIL) {
        _print_sensor_discrete_log(fru, snr_num, snr_name,
                                        GETBIT(n_val, 4), "FRB3");
    }
  }

  return 0;
}

int
pal_is_debug_card_prsnt(uint8_t *status) {
  int val;
  char path[64];

  sprintf(path, GPIO_VAL, GPIO_SCM_USB_PRSNT);

  if (read_device(path, &val)) {
    return -1;
  }

  if (val) {
    *status = 1;
  } else {
    *status = 0;
  }

  return 0;
}

// Enable POST buffer for the server in given slot
int
pal_post_enable(uint8_t slot) {
  int ret;

  bic_config_t config = {0};
  bic_config_u *t = (bic_config_u *) &config;

  ret = bic_get_config(IPMB_BUS, &config);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, 
           "post_enable: bic_get_config failed for fru: %d\n", slot);
#endif
    return ret;
  }

  if (0 == t->bits.post) {
    t->bits.post = 1;
    ret = bic_set_config(IPMB_BUS, &config);
    if (ret) {
#ifdef DEBUG
      syslog(LOG_WARNING, "post_enable: bic_set_config failed\n");
#endif
      return ret;
    }
  }

  return 0;
}

// Disable POST buffer for the server in given slot
int
pal_post_disable(uint8_t slot) {
  int ret;

  bic_config_t config = {0};
  bic_config_u *t = (bic_config_u *) &config;

  ret = bic_get_config(IPMB_BUS, &config);
  if (ret) {
    return ret;
  }

  t->bits.post = 0;

  ret = bic_set_config(IPMB_BUS, &config);
  if (ret) {
    return ret;
  }

  return 0;
}

// Get the last post code of the given slot
int
pal_post_get_last(uint8_t slot, uint8_t *status) {
  int ret;
  uint8_t buf[MAX_IPMB_RES_LEN] = {0x0};
  uint8_t len;


  ret = bic_get_post_buf(IPMB_BUS, buf, &len);
  if (ret) {
    return ret;
  }

  *status = buf[0];

  return 0;
}

static int
pal_set_post_gpio_out(void) {
  char path[64];
  int ret;
  char *val = "out";

  sprintf(path, GPIO_DIR, GPIO_POSTCODE_0);
  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_DIR, GPIO_POSTCODE_1);
  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_DIR, GPIO_POSTCODE_2);
  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_DIR, GPIO_POSTCODE_3);
  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_DIR, GPIO_POSTCODE_4);
  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_DIR, GPIO_POSTCODE_5);
  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_DIR, GPIO_POSTCODE_6);
  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_DIR, GPIO_POSTCODE_7);
  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  post_exit:
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, "write_device failed for %s\n", path);
#endif
    return -1;
  } else {
    return 0;
  }
}

// Display the given POST code using GPIO port
static int
pal_post_display(uint8_t status) {
  char path[64];
  int ret;
  char *val;

#ifdef DEBUG
  syslog(LOG_WARNING, "pal_post_display: status is %d\n", status);
#endif

  ret = pal_set_post_gpio_out();
  if (ret) {
    return -1;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_0);

  if (BIT(status, 0)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_1);
  if (BIT(status, 1)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_2);
  if (BIT(status, 2)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_3);
  if (BIT(status, 3)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_4);
  if (BIT(status, 4)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_5);
  if (BIT(status, 5)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_6);
  if (BIT(status, 6)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

  sprintf(path, GPIO_VAL, GPIO_POSTCODE_7);
  if (BIT(status, 7)) {
    val = "1";
  } else {
    val = "0";
  }

  ret = write_device(path, val);
  if (ret) {
    goto post_exit;
  }

post_exit:
  if (ret) {
#ifdef DEBUG
    syslog(LOG_WARNING, "write_device failed for %s\n", path);
#endif
    return -1;
  } else {
    return 0;
  }
}

// Handle the received post code, for now display it on debug card
int
pal_post_handle(uint8_t slot, uint8_t status) {
  uint8_t prsnt;
  int ret;

  // Check for debug card presence
  ret = pal_is_debug_card_prsnt(&prsnt);
  if (ret) {
    return ret;
  }

  // No debug card present, return
  if (!prsnt) {
    return 0;
  }

  // Display the post code in the debug card
  ret = pal_post_display(status);
  if (ret) {
    return ret;
  }

  return 0;
}

// Return the Front panel Power Button
int
pal_get_board_rev(int *rev) {
  char path[64];
  int val_id_0, val_id_1, val_id_2;

  sprintf(path, GPIO_VAL, GPIO_MB_REV_ID_0);
  if (read_device(path, &val_id_0)) {
    return -1;
  }

  sprintf(path, GPIO_VAL, GPIO_MB_REV_ID_1);
  if (read_device(path, &val_id_1)) {
    return -1;
  }

  sprintf(path, GPIO_VAL, GPIO_MB_REV_ID_2);
  if (read_device(path, &val_id_2)) {
    return -1;
  }

  *rev = val_id_0 | (val_id_1 << 1) | (val_id_2 << 2);
  syslog(LOG_ERR, "Board rev: %d\n", *rev);

  return 0;
}

int
pal_set_last_pwr_state(uint8_t fru, char *state) {

  int ret;
  char key[MAX_KEY_LEN];

  sprintf(key, "pwr_server%d_last_state", (int) fru);

  ret = pal_set_key_value(key, state);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_set_last_pwr_state: pal_set_key_value failed for "
        "fru %u", fru);
#endif
  }
  return ret;
}

int
pal_get_last_pwr_state(uint8_t fru, char *state) {
  int ret;
  char key[MAX_KEY_LEN];

  sprintf(key, "pwr_server%d_last_state", (int) fru);

  ret = pal_get_key_value(key, state);
  if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "pal_get_last_pwr_state: pal_get_key_value failed for "
      "fru %u", fru);
#endif
  }
  return ret;
}

int
pal_set_com_pwr_btn_n(char *status) {
  char path[64];
  int ret;
  sprintf(path, SCM_SYSFS, COM_PWR_BTN_N);

  ret = write_device(path, status);
  if (ret) {
#ifdef DEBUG
  syslog(LOG_WARNING, "write_device failed for %s\n", path);
#endif
    return -1;
  }

  return 0;
}

// Power On the server
static int
server_power_on(uint8_t slot_id) {
  int ret;

  ret = run_command("/usr/local/bin/wedge_power.sh on");
  if (ret)
    return -1;
  return 0;
}

// Power Off the server in given slot
static int
server_power_off(uint8_t slot_id, bool gs_flag) {
  int ret;

  ret = pal_set_com_pwr_btn_n("0");
  if (ret)
    return -1;
  sleep(DELAY_POWER_OFF);
  ret = pal_set_com_pwr_btn_n("1");
  if (ret)
    return -1;

  return 0;
}

int
pal_get_server_power(uint8_t slot_id, uint8_t *status) {

  int ret;
  char value[MAX_VALUE_LEN];
  bic_gpio_t gpio;
  uint8_t retry = MAX_READ_RETRY;

  /* check if the CPU is turned on or not */
  while (retry) {
    ret = bic_get_gpio(IPMB_BUS, &gpio);
    if (!ret)
      break;
    msleep(50);
    retry--;
  }
  if (ret) {
    // Check for if the BIC is irresponsive due to 12V_OFF or 12V_CYCLE
    syslog(LOG_INFO, "pal_get_server_power: bic_get_gpio returned error hence"
        " reading the kv_store for last power state  for fru %d", slot_id);
    pal_get_last_pwr_state(slot_id, value);
    if (!(strcmp(value, "off"))) {
      *status = SERVER_POWER_OFF;
    } else if (!(strcmp(value, "on"))) {
      *status = SERVER_POWER_ON;
    } else {
      return ret;
    }
    return 0;
  }

  if (gpio.pwrgood_cpu) {
    *status = SERVER_POWER_ON;
  } else {
    *status = SERVER_POWER_OFF;
  }

  return 0;
}

// Power Off, Power On, or Power Reset the server in given slot
int
pal_set_server_power(uint8_t slot_id, uint8_t cmd) {
  int ret;
  uint8_t status;
  bool gs_flag = false;

  if (pal_get_server_power(slot_id, &status) < 0) {
    return -1;
  }

  switch(cmd) {
    case SERVER_POWER_ON:
      if (status == SERVER_POWER_ON)
        return 1;
      else
        return server_power_on(slot_id);
      break;

    case SERVER_POWER_OFF:
      if (status == SERVER_POWER_OFF)
        return 1;
      else
        return server_power_off(slot_id, gs_flag);
      break;

    case SERVER_POWER_CYCLE:
      if (status == SERVER_POWER_ON) {
        if (server_power_off(slot_id, gs_flag))
          return -1;

        sleep(DELAY_POWER_CYCLE);

        return server_power_on(slot_id);

      } else if (status == SERVER_POWER_OFF) {

        return (server_power_on(slot_id));
      }
      break;

    case SERVER_POWER_RESET:
      if (status == SERVER_POWER_ON) {
        ret = pal_set_rst_btn(slot_id, 0);
        if (ret < 0)
          return ret;
        msleep(100); //some server miss to detect a quick pulse, so delay 100ms between low high
        ret = pal_set_rst_btn(slot_id, 1);
        if (ret < 0)
          return ret;
      } else if (status == SERVER_POWER_OFF) {
        printf("Ignore to execute power reset action when the \
                power status of server is off\n");
        return -2;
      }
      break;

    case SERVER_GRACEFUL_SHUTDOWN:
      if (status == SERVER_POWER_OFF) {
        return 1;
      } else {
        gs_flag = true;
        return server_power_off(slot_id, gs_flag);
      }
      break;

    default:
      return -1;
  }

  return 0;
}

static bool
is_server_on(void) {
  int ret;
  uint8_t status;
  ret = pal_get_server_power(FRU_SCM, &status);
  if (ret) {
    return false;
  }

  if (status == SERVER_POWER_ON) {
    return true;
  } else {
    return false;
  }
}

static int
get_current_dir(const char *device, char *dir_name) {
  char cmd[LARGEST_DEVICE_NAME + 1];
  FILE *fp;
  int ret=-1;
  int size;

  // Get current working directory
  snprintf(
      cmd, LARGEST_DEVICE_NAME, "cd %s;pwd", device);

  fp = popen(cmd, "r");
  if(NULL == fp)
     return -1;
  fgets(dir_name, LARGEST_DEVICE_NAME, fp);

  ret = pclose(fp);
  if(-1 == ret)
     syslog(LOG_ERR, "%s pclose() fail ", __func__);

  // Remove the newline character at the end
  size = strlen(dir_name);
  dir_name[size-1] = '\0';

  return 0;
}

static int
read_attr(const char *device, const char *attr, float *value) {
  char full_name[LARGEST_DEVICE_NAME + 1];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  int tmp;

  // Get current working directory
  if (get_current_dir(device, dir_name)) {
    return -1;
  }

  snprintf(
      full_name, LARGEST_DEVICE_NAME, "%s/%s", dir_name, attr);

  if (read_device(full_name, &tmp)) {
    return -1;
  }

  *value = ((float)tmp)/UNIT_DIV;

  return 0;
}

static int
read_hsc_attr(const char *device,
              const char* attr, float r_sense, float *value) {
  char full_dir_name[LARGEST_DEVICE_NAME];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  int tmp;

  // Get current working directory
  if (get_current_dir(device, dir_name))
  {
    return -1;
  }
  snprintf(
      full_dir_name, LARGEST_DEVICE_NAME, "%s/%s", dir_name, attr);

  if (read_device(full_dir_name, &tmp)) {
    return -1;
  }

  if ((strncmp(attr, CURR(1), 11) == 0) ||
      (strncmp(attr, POWER(1), 12) == 0)) {
    *value = ((float)tmp)/r_sense/UNIT_DIV;
  } else {
    *value = ((float)tmp)/UNIT_DIV;
  }

  return 0;
}

static int
read_hsc_volt(const char *device, float r_sense, float *value) {
  return read_hsc_attr(device, VOLT(1), r_sense, value);
}

static int
read_hsc_curr(const char *device, float r_sense, float *value) {
  return read_hsc_attr(device, CURR(1), r_sense, value);
}

static int
read_hsc_power(const char *device, float r_sense, float *value) {
  return read_hsc_attr(device, POWER(1), r_sense, value);
}

static int
read_fan_rpm_f(const char *device, uint8_t fan, float *value) {
  char full_name[LARGEST_DEVICE_NAME + 1];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  char device_name[11];
  int tmp;

  /* Get current working directory */
  if (get_current_dir(device, dir_name)) {
    return -1;
  }

  snprintf(device_name, 11, "fan%d_input", fan);
  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", dir_name, device_name);
  if (read_device(full_name, &tmp)) {
    return -1;
  }

  *value = (float)tmp;

  return 0;
}

static int
read_fan_rpm(const char *device, uint8_t fan, int *value) {
  char full_name[LARGEST_DEVICE_NAME + 1];
  char dir_name[LARGEST_DEVICE_NAME + 1];
  char device_name[11];
  int tmp;

  /* Get current working directory */
  if (get_current_dir(device, dir_name)) {
    return -1;
  }

  snprintf(device_name, 11, "fan%d_input", fan);
  snprintf(full_name, LARGEST_DEVICE_NAME, "%s/%s", dir_name, device_name);
  if (read_device(full_name, &tmp)) {
    return -1;
  }

  *value = tmp;

  return 0;
}

int
pal_get_fan_speed(uint8_t fan, int *rpm) {

  if (fan > 15) {
    syslog(LOG_INFO, "get_fan_speed: invalid fan#:%d", fan);
    return -1;
  }
  if (fan <= 7) {
    fan = fan + 1;
    return read_fan_rpm(SMB_FCM_T_TACH_DEVICE, (fan + 1), rpm);
  } else {
    return read_fan_rpm(SMB_FCM_B_TACH_DEVICE, (fan - 7), rpm);
  }
}

static int
bic_sensor_sdr_path(uint8_t fru, char *path) {

  char fru_name[16];

  switch(fru) {
    case FRU_SCM:
      sprintf(fru_name, "%s", "scm");
      break;
    default:
  #ifdef DEBUG
      syslog(LOG_WARNING, "bic_sensor_sdr_path: Wrong Slot ID\n");
  #endif
      return -1;
  }

  sprintf(path, MINIPACK_SDR_PATH, fru_name);

  if (access(path, F_OK) == -1) {
    return -1;
  }

  return 0;
}

/* Populates all sensor_info_t struct using the path to SDR dump */
static int
sdr_init(char *path, sensor_info_t *sinfo) {
  int fd;
  int ret = 0;
  uint8_t buf[MAX_SDR_LEN] = {0};
  uint8_t bytes_rd = 0;
  uint8_t snr_num = 0;
  sdr_full_t *sdr;

  while (access(path, F_OK) == -1) {
    sleep(5);
  }

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    syslog(LOG_ERR, "%s: open failed for %s\n", __func__, path);
    return -1;
  }

  ret = pal_flock_retry(fd);
  if (ret == -1) {
   syslog(LOG_WARNING, "%s: failed to flock on %s", __func__, path);
   close(fd);
   return -1;
  }

  while ((bytes_rd = read(fd, buf, sizeof(sdr_full_t))) > 0) {
    if (bytes_rd != sizeof(sdr_full_t)) {
      syslog(LOG_ERR, "%s: read returns %d bytes\n", __func__, bytes_rd);
      pal_unflock_retry(fd);
      close(fd);
      return -1;
    }

    sdr = (sdr_full_t *) buf;
    snr_num = sdr->sensor_num;
    sinfo[snr_num].valid = true;
    memcpy(&sinfo[snr_num].sdr, sdr, sizeof(sdr_full_t));
  }

  ret = pal_unflock_retry(fd);
  if (ret == -1) {
    syslog(LOG_WARNING, "%s: failed to unflock on %s", __func__, path);
    close(fd);
    return -1;
  }

  close(fd);
  return 0;
}

static int
bic_sensor_sdr_init(uint8_t fru, sensor_info_t *sinfo) {
  char path[64];
  int retry = 0;

  switch(fru) {
    case FRU_SCM:
      if (bic_sensor_sdr_path(fru, path) < 0) {
#ifdef DEBUG
        syslog(LOG_WARNING, 
               "bic_sensor_sdr_path: get_fru_sdr_path failed\n");
#endif
        return ERR_NOT_READY;
      }
      while (retry <= 3) {
        if (sdr_init(path, sinfo) < 0) {
          if (retry == 3) { //if the third retry still failed, return -1
#ifdef DEBUG
            syslog(LOG_ERR, 
                   "bic_sensor_sdr_init: sdr_init failed for FRU %d", fru);
#endif
            return -1;
          }
          retry++;
          sleep(1);
        } else {
          break;
        }
      }
      break;
  }

  return 0;
}

static int
bic_sdr_init(uint8_t fru) {

  static bool init_done[MAX_NUM_FRUS] = {false};

  if (!init_done[fru - 1]) {

    sensor_info_t *sinfo = g_sinfo[fru-1];

    if (bic_sensor_sdr_init(fru, sinfo) < 0)
      return ERR_NOT_READY;

    init_done[fru - 1] = true;
  }

  return 0;
}

static int
bic_read_sensor_wrapper(uint8_t fru, uint8_t sensor_num, bool discrete,
    void *value) {

  int ret;
  ipmi_sensor_reading_t sensor;
  sdr_full_t *sdr;

  ret = bic_read_sensor(IPMB_BUS, sensor_num, &sensor);
  if (ret) {
    return ret;
  }

  if (sensor.flags & BIC_SENSOR_READ_NA) {
#ifdef DEBUG
    syslog(LOG_ERR, "bic_read_sensor_wrapper: Reading Not Available");
    syslog(LOG_ERR, "bic_read_sensor_wrapper: sensor_num: 0x%X, flag: 0x%X",
        sensor_num, sensor.flags);
#endif
    return -1;
  }

  if (discrete) {
    *(float *) value = (float) sensor.status;
    return 0;
  }

  sdr = &g_sinfo[fru-1][sensor_num].sdr;

  // If the SDR is not type1, no need for conversion
  if (sdr->type !=1) {
    *(float *) value = sensor.value;
    return 0;
  }

  // y = (mx + b * 10^b_exp) * 10^r_exp
  uint8_t x;
  uint8_t m_lsb, m_msb;
  uint16_t m = 0;
  uint8_t b_lsb, b_msb;
  uint16_t b = 0;
  int8_t b_exp, r_exp;

  x = sensor.value;

  m_lsb = sdr->m_val;
  m_msb = sdr->m_tolerance >> 6;
  m = (m_msb << 8) | m_lsb;

  b_lsb = sdr->b_val;
  b_msb = sdr->b_accuracy >> 6;
  b = (b_msb << 8) | b_lsb;

  // exponents are 2's complement 4-bit number
  b_exp = sdr->rb_exp & 0xF;
  if (b_exp > 7) {
    b_exp = (~b_exp + 1) & 0xF;
    b_exp = -b_exp;
  }
  r_exp = (sdr->rb_exp >> 4) & 0xF;
  if (r_exp > 7) {
    r_exp = (~r_exp + 1) & 0xF;
    r_exp = -r_exp;
  }

  * (float *) value = ((m * x) + (b * pow(10, b_exp))) * (pow(10, r_exp));

  if ((sensor_num == BIC_SENSOR_SOC_THERM_MARGIN) && (* (float *) value > 0)) {
   * (float *) value -= (float) THERMAL_CONSTANT;
  }

  return 0;
}

static int
get_fan_speed(uint8_t snr_num, float *rpm) {

  int tmp;
  char device_name[LARGEST_DEVICE_NAME + 1];

  if (snr_num >= SMB_SENSOR_FAN1_FRONT_TACH &&
      snr_num <= SMB_SENSOR_FAN8_REAR_TACH) {

    snprintf(device_name, LARGEST_DEVICE_NAME,
             "/tmp/cache_store/smb_sensor%d", snr_num);
    if (read_device(device_name, &tmp)) {
      return -1;
    }
    *rpm = (float)tmp;
  } else {
    return -1;
  }
  return 0;
}

static void apply_inlet_correction(float *value) {
  float rpm[16] = {0};
  float avg_rpm = 0;
  uint8_t i;
  uint8_t cnt = 0;
  static bool inited = false;

  /* Get RPM value */
  for (i = SMB_SENSOR_FAN1_FRONT_TACH; i <= SMB_SENSOR_FAN8_REAR_TACH; i++) {
    if (get_fan_speed(i, &rpm[cnt]) == 0) {
      avg_rpm += rpm[cnt];
      cnt++;
    }
  }
  if (cnt) {
    avg_rpm = avg_rpm / (float)cnt;

    if (!inited) {
      if (sensor_correction_init("/etc/sensor-correction-conf.json")) {
        syslog(LOG_ERR, "sensor_correction_init fail!");
      }
      inited = true;
    }
    sensor_correction_apply(FRU_SCM,
                            SCM_SENSOR_INLET_REMOTE_TEMP, avg_rpm, value);
  }
}

static int
scm_sensor_read(uint8_t sensor_num, float *value) {

  int ret = -1;
  int i = 0;
  int j = 0;
  bool discrete = false;
  bool scm_sensor = false;

  while (i < scm_sensor_cnt) {
    if (sensor_num == scm_sensor_list[i++]) {
      scm_sensor = true;
      break;
    }
  }
  if (scm_sensor) {
    switch(sensor_num) {
      case SCM_SENSOR_OUTLET_LOCAL_TEMP:
        ret = read_attr(SCM_OUTLET_TEMP_DEVICE, TEMP(1), value);
        break;
      case SCM_SENSOR_OUTLET_REMOTE_TEMP:
        ret = read_attr(SCM_OUTLET_TEMP_DEVICE, TEMP(2), value);
        break;
      case SCM_SENSOR_INLET_LOCAL_TEMP:
        ret = read_attr(SCM_INLET_TEMP_DEVICE, TEMP(1), value);
        break;
      case SCM_SENSOR_INLET_REMOTE_TEMP:
        ret = read_attr(SCM_INLET_TEMP_DEVICE, TEMP(2), value);
        if (!ret)
          apply_inlet_correction(value);
        break;
      case SCM_SENSOR_HSC_VOLT:
        ret = read_hsc_volt(SCM_HSC_DEVICE, SCM_RSENSE, value);
        break;
      case SCM_SENSOR_HSC_CURR:
        ret = read_hsc_curr(SCM_HSC_DEVICE, SCM_RSENSE, value);
        break;
      case SCM_SENSOR_HSC_POWER:
        ret = read_hsc_power(SCM_HSC_DEVICE, SCM_RSENSE, value);
        break;
      default:
        ret = READING_NA;
        break;
    }
  } else if (!scm_sensor && is_server_on()) {
    ret = bic_sdr_init(FRU_SCM);
    if (ret < 0) {
#ifdef DEBUG
    syslog(LOG_INFO, "bic_sdr_init fail\n");
#endif
      return ret;
    }

    while (j < bic_discrete_cnt) {
      if (sensor_num == bic_discrete_list[j++]) {
        discrete = true;
        break;
      }
    }
    ret = bic_read_sensor_wrapper(FRU_SCM, sensor_num, discrete, value);
  } else {
    ret = READING_NA;
  }
  return ret;
}

static int
smb_sensor_read(uint8_t sensor_num, float *value) {

  int ret = -1;

  switch(sensor_num) {
    case SMB_SENSOR_TH3_SERDES_TEMP:
      ret = read_attr(SMB_IR_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_TH3_CORE_TEMP:
      ret = read_attr(SMB_ISL_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_TEMP1:
      ret = read_attr(SMB_TEMP1_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_TEMP2:
      ret = read_attr(SMB_TEMP2_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_TEMP3:
      ret = read_attr(SMB_TEMP3_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_TEMP4:
      ret = read_attr(SMB_TEMP4_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_TEMP5:
      ret = read_attr(SMB_TH3_TEMP_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_TH3_DIE_TEMP1:
      ret = read_attr(SMB_TH3_TEMP_DEVICE, TEMP(2), value);
      break;
    case SMB_SENSOR_TH3_DIE_TEMP2:
      ret = read_attr(SMB_TH3_TEMP_DEVICE, TEMP(3), value);
      break;
    case SMB_SENSOR_PDB_L_TEMP1:
      ret = read_attr(SMB_PDB_L_TEMP1_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_PDB_L_TEMP2:
      ret = read_attr(SMB_PDB_L_TEMP2_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_PDB_R_TEMP1:
      ret = read_attr(SMB_PDB_R_TEMP1_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_PDB_R_TEMP2:
      ret = read_attr(SMB_PDB_R_TEMP2_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_FCM_T_TEMP1:
      ret = read_attr(SMB_FCM_T_TEMP1_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_FCM_T_TEMP2:
      ret = read_attr(SMB_FCM_T_TEMP2_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_FCM_B_TEMP1:
      ret = read_attr(SMB_FCM_B_TEMP1_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_FCM_B_TEMP2:
      ret = read_attr(SMB_FCM_B_TEMP2_DEVICE, TEMP(1), value);
      break;
    case SMB_SENSOR_1220_VMON1:
      ret = read_attr(SMB_1220_DEVICE, VOLT(0), value);
      break;
    case SMB_SENSOR_1220_VMON2:
      ret = read_attr(SMB_1220_DEVICE, VOLT(1), value);
      break;
    case SMB_SENSOR_1220_VMON3:
      ret = read_attr(SMB_1220_DEVICE, VOLT(2), value);
      break;
    case SMB_SENSOR_1220_VMON4:
      ret = read_attr(SMB_1220_DEVICE, VOLT(3), value);
      break;
    case SMB_SENSOR_1220_VMON5:
      ret = read_attr(SMB_1220_DEVICE, VOLT(4), value);
      break;
    case SMB_SENSOR_1220_VMON6:
      ret = read_attr(SMB_1220_DEVICE, VOLT(5), value);
      break;
    case SMB_SENSOR_1220_VMON7:
      ret = read_attr(SMB_1220_DEVICE, VOLT(6), value);
      break;
    case SMB_SENSOR_1220_VMON8:
      ret = read_attr(SMB_1220_DEVICE, VOLT(7), value);
      break;
    case SMB_SENSOR_1220_VMON9:
      ret = read_attr(SMB_1220_DEVICE, VOLT(8), value);
      break;
    case SMB_SENSOR_1220_VMON10:
      ret = read_attr(SMB_1220_DEVICE, VOLT(9), value);
      break;
    case SMB_SENSOR_1220_VMON11:
      ret = read_attr(SMB_1220_DEVICE, VOLT(10), value);
      break;
    case SMB_SENSOR_1220_VMON12:
      ret = read_attr(SMB_1220_DEVICE, VOLT(11), value);
      break;
    case SMB_SENSOR_1220_VCCA:
      ret = read_attr(SMB_1220_DEVICE, VOLT(12), value);
      break;
    case SMB_SENSOR_1220_VCCINP:
      ret = read_attr(SMB_1220_DEVICE, VOLT(13), value);
      break;
    case SMB_SENSOR_TH3_SERDES_VOLT:
      ret = read_attr(SMB_IR_DEVICE, VOLT(0), value);
      break;
    case SMB_SENSOR_TH3_CORE_VOLT:
      ret = read_attr(SMB_ISL_DEVICE, VOLT(0), value);
      break;
    case SMB_SENSOR_FCM_T_HSC_VOLT:
      ret = read_hsc_volt(SMB_FCM_T_HSC_DEVICE, FCM_RSENSE, value);
      break;
    case SMB_SENSOR_FCM_B_HSC_VOLT:
      ret = read_hsc_volt(SMB_FCM_B_HSC_DEVICE, FCM_RSENSE, value);
      break;
    case SMB_SENSOR_TH3_SERDES_CURR:
      ret = read_attr(SMB_IR_DEVICE, CURR(1), value);
      break;
    case SMB_SENSOR_TH3_CORE_CURR:
      ret = read_attr(SMB_ISL_DEVICE,  CURR(1), value);
      break;
    case SMB_SENSOR_FCM_T_HSC_CURR:
      ret = read_hsc_curr(SMB_FCM_T_HSC_DEVICE, FCM_RSENSE, value);
      break;
    case SMB_SENSOR_FCM_B_HSC_CURR:
      ret = read_hsc_curr(SMB_FCM_B_HSC_DEVICE, FCM_RSENSE, value);
      break;
    case SMB_SENSOR_FCM_T_HSC_POWER:
      ret = read_hsc_power(SMB_FCM_T_HSC_DEVICE, FCM_RSENSE, value);
      break;
    case SMB_SENSOR_FCM_B_HSC_POWER:
      ret = read_hsc_power(SMB_FCM_B_HSC_DEVICE, FCM_RSENSE, value);
      break;
    case SMB_SENSOR_FAN1_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_T_TACH_DEVICE, 1, value);
      break;
    case SMB_SENSOR_FAN1_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_T_TACH_DEVICE, 2, value);
      break;
    case SMB_SENSOR_FAN2_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_B_TACH_DEVICE, 1, value);
      break;
    case SMB_SENSOR_FAN2_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_B_TACH_DEVICE, 2, value);
      break;
    case SMB_SENSOR_FAN3_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_T_TACH_DEVICE, 3, value);
      break;
    case SMB_SENSOR_FAN3_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_T_TACH_DEVICE, 4, value);
      break;
    case SMB_SENSOR_FAN4_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_B_TACH_DEVICE, 3, value);
      break;
    case SMB_SENSOR_FAN4_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_B_TACH_DEVICE, 4, value);
      break;
    case SMB_SENSOR_FAN5_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_T_TACH_DEVICE, 5, value);
      break;
    case SMB_SENSOR_FAN5_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_T_TACH_DEVICE, 6, value);
      break;
    case SMB_SENSOR_FAN6_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_B_TACH_DEVICE, 5, value);
      break;
    case SMB_SENSOR_FAN6_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_B_TACH_DEVICE, 6, value);
      break;
    case SMB_SENSOR_FAN7_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_T_TACH_DEVICE, 7, value);
      break;
    case SMB_SENSOR_FAN7_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_T_TACH_DEVICE, 8, value);
      break;
    case SMB_SENSOR_FAN8_FRONT_TACH:
      ret = read_fan_rpm_f(SMB_FCM_B_TACH_DEVICE, 7, value);
      break;
    case SMB_SENSOR_FAN8_REAR_TACH:
      ret = read_fan_rpm_f(SMB_FCM_B_TACH_DEVICE, 8, value);
      break;
    default:
      ret = READING_NA;
      break;
  }
  return ret;
}

static int
pim_sensor_read(uint8_t sensor_num, float *value) {

  int ret = -1;

  switch(sensor_num) {
    case PIM1_SENSOR_TEMP1:
      ret = read_attr(PIM1_TEMP1_DEVICE, TEMP(1), value);
      break;
    case PIM1_SENSOR_TEMP2:
      ret = read_attr(PIM1_TEMP2_DEVICE, TEMP(1), value);
      break;
    case PIM1_SENSOR_HSC_VOLT:
      ret = read_hsc_volt(PIM1_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM1_SENSOR_HSC_CURR:
      ret = read_hsc_curr(PIM1_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM1_SENSOR_HSC_POWER:
      ret = read_hsc_power(PIM1_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM1_SENSOR_34461_VOLT1:
      ret = read_attr(PIM1_34461_DEVICE, VOLT(1), value);
      break;
    case PIM1_SENSOR_34461_VOLT2:
      ret = read_attr(PIM1_34461_DEVICE, VOLT(2), value);
      break;
    case PIM1_SENSOR_34461_VOLT3:
      ret = read_attr(PIM1_34461_DEVICE, VOLT(3), value);
      break;
    case PIM1_SENSOR_34461_VOLT4:
      ret = read_attr(PIM1_34461_DEVICE, VOLT(4), value);
      break;
    case PIM1_SENSOR_34461_VOLT5:
      ret = read_attr(PIM1_34461_DEVICE, VOLT(5), value);
      break;
    case PIM1_SENSOR_34461_VOLT6:
      ret = read_attr(PIM1_34461_DEVICE, VOLT(6), value);
      break;
    case PIM1_SENSOR_34461_VOLT7:
      ret = read_attr(PIM1_34461_DEVICE, VOLT(7), value);
      break;
    case PIM1_SENSOR_34461_VOLT8:
      ret = read_attr(PIM1_34461_DEVICE, VOLT(8), value);
      break;
    case PIM1_SENSOR_34461_VOLT9:
      ret = read_attr(PIM1_34461_DEVICE, VOLT(9), value);
      break;
    case PIM1_SENSOR_34461_VOLT10:
      ret = read_attr(PIM1_34461_DEVICE, VOLT(10), value);
      break;
    case PIM1_SENSOR_34461_VOLT11:
      ret = read_attr(PIM1_34461_DEVICE, VOLT(11), value);
      break;
    case PIM1_SENSOR_34461_VOLT12:
      ret = read_attr(PIM1_34461_DEVICE, VOLT(12), value);
      break;
    case PIM1_SENSOR_34461_VOLT13:
      ret = read_attr(PIM1_34461_DEVICE, VOLT(13), value);
      break;
    case PIM1_SENSOR_34461_VOLT14:
      ret = read_attr(PIM1_34461_DEVICE, VOLT(14), value);
      break;
    case PIM1_SENSOR_34461_VOLT15:
      ret = read_attr(PIM1_34461_DEVICE, VOLT(15), value);
      break;
    case PIM1_SENSOR_34461_VOLT16:
      ret = read_attr(PIM1_34461_DEVICE, VOLT(16), value);
      break;
    case PIM2_SENSOR_TEMP1:
      ret = read_attr(PIM2_TEMP1_DEVICE, TEMP(1), value);
      break;
    case PIM2_SENSOR_TEMP2:
      ret = read_attr(PIM2_TEMP2_DEVICE, TEMP(1), value);
      break;
    case PIM2_SENSOR_HSC_VOLT:
      ret = read_hsc_volt(PIM2_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM2_SENSOR_HSC_CURR:
      ret = read_hsc_curr(PIM2_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM2_SENSOR_HSC_POWER:
      ret = read_hsc_power(PIM2_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM2_SENSOR_34461_VOLT1:
      ret = read_attr(PIM2_34461_DEVICE, VOLT(1), value);
      break;
    case PIM2_SENSOR_34461_VOLT2:
      ret = read_attr(PIM2_34461_DEVICE, VOLT(2), value);
      break;
    case PIM2_SENSOR_34461_VOLT3:
      ret = read_attr(PIM2_34461_DEVICE, VOLT(3), value);
      break;
    case PIM2_SENSOR_34461_VOLT4:
      ret = read_attr(PIM2_34461_DEVICE, VOLT(4), value);
      break;
    case PIM2_SENSOR_34461_VOLT5:
      ret = read_attr(PIM2_34461_DEVICE, VOLT(5), value);
      break;
    case PIM2_SENSOR_34461_VOLT6:
      ret = read_attr(PIM2_34461_DEVICE, VOLT(6), value);
      break;
    case PIM2_SENSOR_34461_VOLT7:
      ret = read_attr(PIM2_34461_DEVICE, VOLT(7), value);
      break;
    case PIM2_SENSOR_34461_VOLT8:
      ret = read_attr(PIM2_34461_DEVICE, VOLT(8), value);
      break;
    case PIM2_SENSOR_34461_VOLT9:
      ret = read_attr(PIM2_34461_DEVICE, VOLT(9), value);
      break;
    case PIM2_SENSOR_34461_VOLT10:
      ret = read_attr(PIM2_34461_DEVICE, VOLT(10), value);
      break;
    case PIM2_SENSOR_34461_VOLT11:
      ret = read_attr(PIM2_34461_DEVICE, VOLT(11), value);
      break;
    case PIM2_SENSOR_34461_VOLT12:
      ret = read_attr(PIM2_34461_DEVICE, VOLT(12), value);
      break;
    case PIM2_SENSOR_34461_VOLT13:
      ret = read_attr(PIM2_34461_DEVICE, VOLT(13), value);
      break;
    case PIM2_SENSOR_34461_VOLT14:
      ret = read_attr(PIM2_34461_DEVICE, VOLT(14), value);
      break;
    case PIM2_SENSOR_34461_VOLT15:
      ret = read_attr(PIM2_34461_DEVICE, VOLT(15), value);
      break;
    case PIM2_SENSOR_34461_VOLT16:
      ret = read_attr(PIM2_34461_DEVICE, VOLT(16), value);
      break;
    case PIM3_SENSOR_TEMP1:
      ret = read_attr(PIM3_TEMP1_DEVICE, TEMP(1), value);
      break;
    case PIM3_SENSOR_TEMP2:
      ret = read_attr(PIM3_TEMP2_DEVICE, TEMP(1), value);
      break;
    case PIM3_SENSOR_HSC_VOLT:
      ret = read_hsc_volt(PIM3_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM3_SENSOR_HSC_CURR:
      ret = read_hsc_curr(PIM3_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM3_SENSOR_HSC_POWER:
      ret = read_hsc_power(PIM3_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM3_SENSOR_34461_VOLT1:
      ret = read_attr(PIM3_34461_DEVICE, VOLT(1), value);
      break;
    case PIM3_SENSOR_34461_VOLT2:
      ret = read_attr(PIM3_34461_DEVICE, VOLT(2), value);
      break;
    case PIM3_SENSOR_34461_VOLT3:
      ret = read_attr(PIM3_34461_DEVICE, VOLT(3), value);
      break;
    case PIM3_SENSOR_34461_VOLT4:
      ret = read_attr(PIM3_34461_DEVICE, VOLT(4), value);
      break;
    case PIM3_SENSOR_34461_VOLT5:
      ret = read_attr(PIM3_34461_DEVICE, VOLT(5), value);
      break;
    case PIM3_SENSOR_34461_VOLT6:
      ret = read_attr(PIM3_34461_DEVICE, VOLT(6), value);
      break;
    case PIM3_SENSOR_34461_VOLT7:
      ret = read_attr(PIM3_34461_DEVICE, VOLT(7), value);
      break;
    case PIM3_SENSOR_34461_VOLT8:
      ret = read_attr(PIM3_34461_DEVICE, VOLT(8), value);
      break;
    case PIM3_SENSOR_34461_VOLT9:
      ret = read_attr(PIM3_34461_DEVICE, VOLT(9), value);
      break;
    case PIM3_SENSOR_34461_VOLT10:
      ret = read_attr(PIM3_34461_DEVICE, VOLT(10), value);
      break;
    case PIM3_SENSOR_34461_VOLT11:
      ret = read_attr(PIM3_34461_DEVICE, VOLT(11), value);
      break;
    case PIM3_SENSOR_34461_VOLT12:
      ret = read_attr(PIM3_34461_DEVICE, VOLT(12), value);
      break;
    case PIM3_SENSOR_34461_VOLT13:
      ret = read_attr(PIM3_34461_DEVICE, VOLT(13), value);
      break;
    case PIM3_SENSOR_34461_VOLT14:
      ret = read_attr(PIM3_34461_DEVICE, VOLT(14), value);
      break;
    case PIM3_SENSOR_34461_VOLT15:
      ret = read_attr(PIM3_34461_DEVICE, VOLT(15), value);
      break;
    case PIM3_SENSOR_34461_VOLT16:
      ret = read_attr(PIM3_34461_DEVICE, VOLT(16), value);
      break;
    case PIM4_SENSOR_TEMP1:
      ret = read_attr(PIM4_TEMP1_DEVICE, TEMP(1), value);
      break;
    case PIM4_SENSOR_TEMP2:
      ret = read_attr(PIM4_TEMP2_DEVICE, TEMP(1), value);
      break;
    case PIM4_SENSOR_HSC_VOLT:
      ret = read_hsc_volt(PIM4_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM4_SENSOR_HSC_CURR:
      ret = read_hsc_curr(PIM4_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM4_SENSOR_HSC_POWER:
      ret = read_hsc_power(PIM4_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM4_SENSOR_34461_VOLT1:
      ret = read_attr(PIM4_34461_DEVICE, VOLT(1), value);
      break;
    case PIM4_SENSOR_34461_VOLT2:
      ret = read_attr(PIM4_34461_DEVICE, VOLT(2), value);
      break;
    case PIM4_SENSOR_34461_VOLT3:
      ret = read_attr(PIM4_34461_DEVICE, VOLT(3), value);
      break;
    case PIM4_SENSOR_34461_VOLT4:
      ret = read_attr(PIM4_34461_DEVICE, VOLT(4), value);
      break;
    case PIM4_SENSOR_34461_VOLT5:
      ret = read_attr(PIM4_34461_DEVICE, VOLT(5), value);
      break;
    case PIM4_SENSOR_34461_VOLT6:
      ret = read_attr(PIM4_34461_DEVICE, VOLT(6), value);
      break;
    case PIM4_SENSOR_34461_VOLT7:
      ret = read_attr(PIM4_34461_DEVICE, VOLT(7), value);
      break;
    case PIM4_SENSOR_34461_VOLT8:
      ret = read_attr(PIM4_34461_DEVICE, VOLT(8), value);
      break;
    case PIM4_SENSOR_34461_VOLT9:
      ret = read_attr(PIM4_34461_DEVICE, VOLT(9), value);
      break;
    case PIM4_SENSOR_34461_VOLT10:
      ret = read_attr(PIM4_34461_DEVICE, VOLT(10), value);
      break;
    case PIM4_SENSOR_34461_VOLT11:
      ret = read_attr(PIM4_34461_DEVICE, VOLT(11), value);
      break;
    case PIM4_SENSOR_34461_VOLT12:
      ret = read_attr(PIM4_34461_DEVICE, VOLT(12), value);
      break;
    case PIM4_SENSOR_34461_VOLT13:
      ret = read_attr(PIM4_34461_DEVICE, VOLT(13), value);
      break;
    case PIM4_SENSOR_34461_VOLT14:
      ret = read_attr(PIM4_34461_DEVICE, VOLT(14), value);
      break;
    case PIM4_SENSOR_34461_VOLT15:
      ret = read_attr(PIM4_34461_DEVICE, VOLT(15), value);
      break;
    case PIM4_SENSOR_34461_VOLT16:
      ret = read_attr(PIM4_34461_DEVICE, VOLT(16), value);
      break;
    case PIM5_SENSOR_TEMP1:
      ret = read_attr(PIM5_TEMP1_DEVICE, TEMP(1), value);
      break;
    case PIM5_SENSOR_TEMP2:
      ret = read_attr(PIM5_TEMP2_DEVICE, TEMP(1), value);
      break;
    case PIM5_SENSOR_HSC_VOLT:
      ret = read_hsc_volt(PIM5_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM5_SENSOR_HSC_CURR:
      ret = read_hsc_curr(PIM5_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM5_SENSOR_HSC_POWER:
      ret = read_hsc_power(PIM5_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM5_SENSOR_34461_VOLT1:
      ret = read_attr(PIM5_34461_DEVICE, VOLT(1), value);
      break;
    case PIM5_SENSOR_34461_VOLT2:
      ret = read_attr(PIM5_34461_DEVICE, VOLT(2), value);
      break;
    case PIM5_SENSOR_34461_VOLT3:
      ret = read_attr(PIM5_34461_DEVICE, VOLT(3), value);
      break;
    case PIM5_SENSOR_34461_VOLT4:
      ret = read_attr(PIM5_34461_DEVICE, VOLT(4), value);
      break;
    case PIM5_SENSOR_34461_VOLT5:
      ret = read_attr(PIM5_34461_DEVICE, VOLT(5), value);
      break;
    case PIM5_SENSOR_34461_VOLT6:
      ret = read_attr(PIM5_34461_DEVICE, VOLT(6), value);
      break;
    case PIM5_SENSOR_34461_VOLT7:
      ret = read_attr(PIM5_34461_DEVICE, VOLT(7), value);
      break;
    case PIM5_SENSOR_34461_VOLT8:
      ret = read_attr(PIM5_34461_DEVICE, VOLT(8), value);
      break;
    case PIM5_SENSOR_34461_VOLT9:
      ret = read_attr(PIM5_34461_DEVICE, VOLT(9), value);
      break;
    case PIM5_SENSOR_34461_VOLT10:
      ret = read_attr(PIM5_34461_DEVICE, VOLT(10), value);
      break;
    case PIM5_SENSOR_34461_VOLT11:
      ret = read_attr(PIM5_34461_DEVICE, VOLT(11), value);
      break;
    case PIM5_SENSOR_34461_VOLT12:
      ret = read_attr(PIM5_34461_DEVICE, VOLT(12), value);
      break;
    case PIM5_SENSOR_34461_VOLT13:
      ret = read_attr(PIM5_34461_DEVICE, VOLT(13), value);
      break;
    case PIM5_SENSOR_34461_VOLT14:
      ret = read_attr(PIM5_34461_DEVICE, VOLT(14), value);
      break;
    case PIM5_SENSOR_34461_VOLT15:
      ret = read_attr(PIM5_34461_DEVICE, VOLT(15), value);
      break;
    case PIM5_SENSOR_34461_VOLT16:
      ret = read_attr(PIM5_34461_DEVICE, VOLT(16), value);
      break;
    case PIM6_SENSOR_TEMP1:
      ret = read_attr(PIM6_TEMP1_DEVICE, TEMP(1), value);
      break;
    case PIM6_SENSOR_TEMP2:
      ret = read_attr(PIM6_TEMP2_DEVICE, TEMP(1), value);
      break;
    case PIM6_SENSOR_HSC_VOLT:
      ret = read_hsc_volt(PIM6_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM6_SENSOR_HSC_CURR:
      ret = read_hsc_curr(PIM6_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM6_SENSOR_HSC_POWER:
      ret = read_hsc_power(PIM6_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM6_SENSOR_34461_VOLT1:
      ret = read_attr(PIM6_34461_DEVICE, VOLT(1), value);
      break;
    case PIM6_SENSOR_34461_VOLT2:
      ret = read_attr(PIM6_34461_DEVICE, VOLT(2), value);
      break;
    case PIM6_SENSOR_34461_VOLT3:
      ret = read_attr(PIM6_34461_DEVICE, VOLT(3), value);
      break;
    case PIM6_SENSOR_34461_VOLT4:
      ret = read_attr(PIM6_34461_DEVICE, VOLT(4), value);
      break;
    case PIM6_SENSOR_34461_VOLT5:
      ret = read_attr(PIM6_34461_DEVICE, VOLT(5), value);
      break;
    case PIM6_SENSOR_34461_VOLT6:
      ret = read_attr(PIM6_34461_DEVICE, VOLT(6), value);
      break;
    case PIM6_SENSOR_34461_VOLT7:
      ret = read_attr(PIM6_34461_DEVICE, VOLT(7), value);
      break;
    case PIM6_SENSOR_34461_VOLT8:
      ret = read_attr(PIM6_34461_DEVICE, VOLT(8), value);
      break;
    case PIM6_SENSOR_34461_VOLT9:
      ret = read_attr(PIM6_34461_DEVICE, VOLT(9), value);
      break;
    case PIM6_SENSOR_34461_VOLT10:
      ret = read_attr(PIM6_34461_DEVICE, VOLT(10), value);
      break;
    case PIM6_SENSOR_34461_VOLT11:
      ret = read_attr(PIM6_34461_DEVICE, VOLT(11), value);
      break;
    case PIM6_SENSOR_34461_VOLT12:
      ret = read_attr(PIM6_34461_DEVICE, VOLT(12), value);
      break;
    case PIM6_SENSOR_34461_VOLT13:
      ret = read_attr(PIM6_34461_DEVICE, VOLT(13), value);
      break;
    case PIM6_SENSOR_34461_VOLT14:
      ret = read_attr(PIM6_34461_DEVICE, VOLT(14), value);
      break;
    case PIM6_SENSOR_34461_VOLT15:
      ret = read_attr(PIM6_34461_DEVICE, VOLT(15), value);
      break;
    case PIM6_SENSOR_34461_VOLT16:
      ret = read_attr(PIM6_34461_DEVICE, VOLT(16), value);
      break;
    case PIM7_SENSOR_TEMP1:
      ret = read_attr(PIM7_TEMP1_DEVICE, TEMP(1), value);
      break;
    case PIM7_SENSOR_TEMP2:
      ret = read_attr(PIM7_TEMP2_DEVICE, TEMP(1), value);
      break;
    case PIM7_SENSOR_HSC_VOLT:
      ret = read_hsc_volt(PIM7_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM7_SENSOR_HSC_CURR:
      ret = read_hsc_curr(PIM7_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM7_SENSOR_HSC_POWER:
      ret = read_hsc_power(PIM7_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM7_SENSOR_34461_VOLT1:
      ret = read_attr(PIM7_34461_DEVICE, VOLT(1), value);
      break;
    case PIM7_SENSOR_34461_VOLT2:
      ret = read_attr(PIM7_34461_DEVICE, VOLT(2), value);
      break;
    case PIM7_SENSOR_34461_VOLT3:
      ret = read_attr(PIM7_34461_DEVICE, VOLT(3), value);
      break;
    case PIM7_SENSOR_34461_VOLT4:
      ret = read_attr(PIM7_34461_DEVICE, VOLT(4), value);
      break;
    case PIM7_SENSOR_34461_VOLT5:
      ret = read_attr(PIM7_34461_DEVICE, VOLT(5), value);
      break;
    case PIM7_SENSOR_34461_VOLT6:
      ret = read_attr(PIM7_34461_DEVICE, VOLT(6), value);
      break;
    case PIM7_SENSOR_34461_VOLT7:
      ret = read_attr(PIM7_34461_DEVICE, VOLT(7), value);
      break;
    case PIM7_SENSOR_34461_VOLT8:
      ret = read_attr(PIM7_34461_DEVICE, VOLT(8), value);
      break;
    case PIM7_SENSOR_34461_VOLT9:
      ret = read_attr(PIM7_34461_DEVICE, VOLT(9), value);
      break;
    case PIM7_SENSOR_34461_VOLT10:
      ret = read_attr(PIM7_34461_DEVICE, VOLT(10), value);
      break;
    case PIM7_SENSOR_34461_VOLT11:
      ret = read_attr(PIM7_34461_DEVICE, VOLT(11), value);
      break;
    case PIM7_SENSOR_34461_VOLT12:
      ret = read_attr(PIM7_34461_DEVICE, VOLT(12), value);
      break;
    case PIM7_SENSOR_34461_VOLT13:
      ret = read_attr(PIM7_34461_DEVICE, VOLT(13), value);
      break;
    case PIM7_SENSOR_34461_VOLT14:
      ret = read_attr(PIM7_34461_DEVICE, VOLT(14), value);
      break;
    case PIM7_SENSOR_34461_VOLT15:
      ret = read_attr(PIM7_34461_DEVICE, VOLT(15), value);
      break;
    case PIM7_SENSOR_34461_VOLT16:
      ret = read_attr(PIM7_34461_DEVICE, VOLT(16), value);
      break;
    case PIM8_SENSOR_TEMP1:
      ret = read_attr(PIM8_TEMP1_DEVICE, TEMP(1), value);
      break;
    case PIM8_SENSOR_TEMP2:
      ret = read_attr(PIM8_TEMP2_DEVICE, TEMP(1), value);
      break;
    case PIM8_SENSOR_HSC_VOLT:
      ret = read_hsc_volt(PIM8_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM8_SENSOR_HSC_CURR:
      ret = read_hsc_curr(PIM8_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM8_SENSOR_HSC_POWER:
      ret = read_hsc_power(PIM8_HSC_DEVICE, PIM_RSENSE, value);
      break;
    case PIM8_SENSOR_34461_VOLT1:
      ret = read_attr(PIM8_34461_DEVICE, VOLT(1), value);
      break;
    case PIM8_SENSOR_34461_VOLT2:
      ret = read_attr(PIM8_34461_DEVICE, VOLT(2), value);
      break;
    case PIM8_SENSOR_34461_VOLT3:
      ret = read_attr(PIM8_34461_DEVICE, VOLT(3), value);
      break;
    case PIM8_SENSOR_34461_VOLT4:
      ret = read_attr(PIM8_34461_DEVICE, VOLT(4), value);
      break;
    case PIM8_SENSOR_34461_VOLT5:
      ret = read_attr(PIM8_34461_DEVICE, VOLT(5), value);
      break;
    case PIM8_SENSOR_34461_VOLT6:
      ret = read_attr(PIM8_34461_DEVICE, VOLT(6), value);
      break;
    case PIM8_SENSOR_34461_VOLT7:
      ret = read_attr(PIM8_34461_DEVICE, VOLT(7), value);
      break;
    case PIM8_SENSOR_34461_VOLT8:
      ret = read_attr(PIM8_34461_DEVICE, VOLT(8), value);
      break;
    case PIM8_SENSOR_34461_VOLT9:
      ret = read_attr(PIM8_34461_DEVICE, VOLT(9), value);
      break;
    case PIM8_SENSOR_34461_VOLT10:
      ret = read_attr(PIM8_34461_DEVICE, VOLT(10), value);
      break;
    case PIM8_SENSOR_34461_VOLT11:
      ret = read_attr(PIM8_34461_DEVICE, VOLT(11), value);
      break;
    case PIM8_SENSOR_34461_VOLT12:
      ret = read_attr(PIM8_34461_DEVICE, VOLT(12), value);
      break;
    case PIM8_SENSOR_34461_VOLT13:
      ret = read_attr(PIM8_34461_DEVICE, VOLT(13), value);
      break;
    case PIM8_SENSOR_34461_VOLT14:
      ret = read_attr(PIM8_34461_DEVICE, VOLT(14), value);
      break;
    case PIM8_SENSOR_34461_VOLT15:
      ret = read_attr(PIM8_34461_DEVICE, VOLT(15), value);
      break;
    case PIM8_SENSOR_34461_VOLT16:
      ret = read_attr(PIM8_34461_DEVICE, VOLT(16), value);
      break;
    default:
      ret = READING_NA;
      break;
  }
  return ret;
}

static int
psu_sensor_read(uint8_t sensor_num, float *value) {

  int ret = -1;

  switch(sensor_num) {
    case PSU1_SENSOR_IN_VOLT:
      ret = read_attr(PSU1_DEVICE, VOLT(0), value);
      break;
    case PSU1_SENSOR_12V_VOLT:
      ret = read_attr(PSU1_DEVICE, VOLT(1), value);
      break;
    case PSU1_SENSOR_STBY_VOLT:
      ret = read_attr(PSU1_DEVICE, VOLT(2), value);
      break;
    case PSU1_SENSOR_IN_CURR:
      ret = read_attr(PSU1_DEVICE, CURR(1), value);
      break;
    case PSU1_SENSOR_12V_CURR:
      ret = read_attr(PSU1_DEVICE, CURR(2), value);
      break;
    case PSU1_SENSOR_STBY_CURR:
      ret = read_attr(PSU1_DEVICE, CURR(3), value);
      break;
    case PSU1_SENSOR_IN_POWER:
      ret = read_attr(PSU1_DEVICE, POWER(1), value);
      break;
    case PSU1_SENSOR_12V_POWER:
      ret = read_attr(PSU1_DEVICE, POWER(2), value);
      break;
    case PSU1_SENSOR_STBY_POWER:
      ret = read_attr(PSU1_DEVICE, POWER(3), value);
      break;
    case PSU1_SENSOR_FAN_TACH:
      ret = read_fan_rpm_f(PSU1_DEVICE, 1, value);
      break;
    case PSU1_SENSOR_TEMP1:
      ret = read_attr(PSU1_DEVICE, TEMP(1), value);
      break;
    case PSU1_SENSOR_TEMP2:
      ret = read_attr(PSU1_DEVICE, TEMP(2), value);
      break;
    case PSU1_SENSOR_TEMP3:
      ret = read_attr(PSU1_DEVICE, TEMP(3), value);
      break;
    case PSU2_SENSOR_IN_VOLT:
      ret = read_attr(PSU2_DEVICE, VOLT(0), value);
      break;
    case PSU2_SENSOR_12V_VOLT:
      ret = read_attr(PSU2_DEVICE, VOLT(1), value);
      break;
    case PSU2_SENSOR_STBY_VOLT:
      ret = read_attr(PSU2_DEVICE, VOLT(2), value);
      break;
    case PSU2_SENSOR_IN_CURR:
      ret = read_attr(PSU2_DEVICE, CURR(1), value);
      break;
    case PSU2_SENSOR_12V_CURR:
      ret = read_attr(PSU2_DEVICE, CURR(2), value);
      break;
    case PSU2_SENSOR_STBY_CURR:
      ret = read_attr(PSU2_DEVICE, CURR(3), value);
      break;
    case PSU2_SENSOR_IN_POWER:
      ret = read_attr(PSU2_DEVICE, POWER(1), value);
      break;
    case PSU2_SENSOR_12V_POWER:
      ret = read_attr(PSU2_DEVICE, POWER(2), value);
      break;
    case PSU2_SENSOR_STBY_POWER:
      ret = read_attr(PSU2_DEVICE, POWER(3), value);
      break;
    case PSU2_SENSOR_FAN_TACH:
      ret = read_fan_rpm_f(PSU2_DEVICE, 1, value);
      break;
    case PSU2_SENSOR_TEMP1:
      ret = read_attr(PSU2_DEVICE, TEMP(1), value);
      break;
    case PSU2_SENSOR_TEMP2:
      ret = read_attr(PSU2_DEVICE, TEMP(2), value);
      break;
    case PSU2_SENSOR_TEMP3:
      ret = read_attr(PSU2_DEVICE, TEMP(3), value);
      break;
    case PSU3_SENSOR_IN_VOLT:
      ret = read_attr(PSU3_DEVICE, VOLT(0), value);
      break;
    case PSU3_SENSOR_12V_VOLT:
      ret = read_attr(PSU3_DEVICE, VOLT(1), value);
      break;
    case PSU3_SENSOR_STBY_VOLT:
      ret = read_attr(PSU3_DEVICE, VOLT(2), value);
      break;
    case PSU3_SENSOR_IN_CURR:
      ret = read_attr(PSU3_DEVICE, CURR(1), value);
      break;
    case PSU3_SENSOR_12V_CURR:
      ret = read_attr(PSU3_DEVICE, CURR(2), value);
      break;
    case PSU3_SENSOR_STBY_CURR:
      ret = read_attr(PSU3_DEVICE, CURR(3), value);
      break;
    case PSU3_SENSOR_IN_POWER:
      ret = read_attr(PSU3_DEVICE, POWER(1), value);
      break;
    case PSU3_SENSOR_12V_POWER:
      ret = read_attr(PSU3_DEVICE, POWER(2), value);
      break;
    case PSU3_SENSOR_STBY_POWER:
      ret = read_attr(PSU3_DEVICE, POWER(3), value);
      break;
    case PSU3_SENSOR_FAN_TACH:
      ret = read_fan_rpm_f(PSU3_DEVICE, 1, value);
      break;
    case PSU3_SENSOR_TEMP1:
      ret = read_attr(PSU3_DEVICE, TEMP(1), value);
      break;
    case PSU3_SENSOR_TEMP2:
      ret = read_attr(PSU3_DEVICE, TEMP(2), value);
      break;
    case PSU3_SENSOR_TEMP3:
      ret = read_attr(PSU3_DEVICE, TEMP(3), value);
      break;
    case PSU4_SENSOR_IN_VOLT:
      ret = read_attr(PSU4_DEVICE, VOLT(0), value);
      break;
    case PSU4_SENSOR_12V_VOLT:
      ret = read_attr(PSU4_DEVICE, VOLT(1), value);
      break;
    case PSU4_SENSOR_STBY_VOLT:
      ret = read_attr(PSU4_DEVICE, VOLT(2), value);
      break;
    case PSU4_SENSOR_IN_CURR:
      ret = read_attr(PSU4_DEVICE, CURR(1), value);
      break;
    case PSU4_SENSOR_12V_CURR:
      ret = read_attr(PSU4_DEVICE, CURR(2), value);
      break;
    case PSU4_SENSOR_STBY_CURR:
      ret = read_attr(PSU4_DEVICE, CURR(3), value);
      break;
    case PSU4_SENSOR_IN_POWER:
      ret = read_attr(PSU4_DEVICE, POWER(1), value);
      break;
    case PSU4_SENSOR_12V_POWER:
      ret = read_attr(PSU4_DEVICE, POWER(2), value);
      break;
    case PSU4_SENSOR_STBY_POWER:
      ret = read_attr(PSU4_DEVICE, POWER(3), value);
      break;
    case PSU4_SENSOR_FAN_TACH:
      ret = read_fan_rpm_f(PSU4_DEVICE, 1, value);
      break;
    case PSU4_SENSOR_TEMP1:
      ret = read_attr(PSU4_DEVICE, TEMP(1), value);
      break;
    case PSU4_SENSOR_TEMP2:
      ret = read_attr(PSU4_DEVICE, TEMP(2), value);
      break;
    case PSU4_SENSOR_TEMP3:
      ret = read_attr(PSU4_DEVICE, TEMP(3), value);
      break;
    default:
      ret = READING_NA;
      break;
  }
  return ret;
}

int
pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value) {

  char key[MAX_KEY_LEN];
  char str[MAX_VALUE_LEN];
  char fru_name[32];
  int ret;
  uint8_t prsnt = 0;

  pal_get_fru_name(fru, fru_name);

  ret = pal_is_fru_prsnt(fru, &prsnt);
  if (ret) {
    return ret;
  }
  if (!prsnt) {
#ifdef DEBUG
  syslog(LOG_INFO, "pal_sensor_read_raw(): %s is not present\n", fru_name);
#endif
    return -1;
  }

  sprintf(key, "%s_sensor%d", fru_name, sensor_num);
  switch(fru) {
    case FRU_SCM:
      ret = scm_sensor_read(sensor_num, value);
      break;
    case FRU_SMB:
      ret = smb_sensor_read(sensor_num, value);
      break;
    case FRU_PIM1:
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
      ret = pim_sensor_read(sensor_num, value);
      break;
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      ret = psu_sensor_read(sensor_num, value);
      break;
    default:
      return -1;
  }

  if (ret) {
    if (ret == READING_NA || ret == -1) {
      strcpy(str, "NA");
    } else {
      return ret;
    }
  } else {
    sprintf(str, "%.2f",*((float*)value));
  }
  if(kv_set(key, str, 0, 0) < 0) {
#ifdef DEBUG
     syslog(LOG_WARNING,
        "pal_sensor_read_raw: cache_set key = %s, str = %s failed.", key, str);
#endif
    return -1;
  } else {
    return ret;
  }

  return 0;
}

static int
get_scm_sensor_name(uint8_t sensor_num, char *name) {

  switch(sensor_num) {
    case SCM_SENSOR_OUTLET_LOCAL_TEMP:
      sprintf(name, "SCM_OUTLET_LOCAL_TEMP");
      break;
    case SCM_SENSOR_OUTLET_REMOTE_TEMP:
      sprintf(name, "SCM_OUTLET_REMOTE_TEMP");
      break;
    case SCM_SENSOR_INLET_LOCAL_TEMP:
      sprintf(name, "SCM_INLET_LOCAL_TEMP");
      break;
    case SCM_SENSOR_INLET_REMOTE_TEMP:
      sprintf(name, "SCM_INLET_REMOTE_TEMP");
      break;
    case SCM_SENSOR_HSC_VOLT:
      sprintf(name, "SCM_HSC_VOLT");
      break;
    case SCM_SENSOR_HSC_CURR:
      sprintf(name, "SCM_HSC_CURR");
      break;
    case SCM_SENSOR_HSC_POWER:
      sprintf(name, "SCM_HSC_POWER");
      break;
    case BIC_SENSOR_MB_OUTLET_TEMP:
      sprintf(name, "MB_OUTLET_TEMP");
      break;
    case BIC_SENSOR_MB_INLET_TEMP:
      sprintf(name, "MB_INLET_TEMP");
      break;
    case BIC_SENSOR_PCH_TEMP:
      sprintf(name, "PCH_TEMP");
      break;
    case BIC_SENSOR_VCCIN_VR_TEMP:
      sprintf(name, "VCCIN_VR_TEMP");
      break;
    case BIC_SENSOR_1V05MIX_VR_TEMP:
      sprintf(name, "1V05MIX_VR_TEMP");
      break;
    case BIC_SENSOR_SOC_TEMP:
      sprintf(name, "SOC_TEMP");
      break;
    case BIC_SENSOR_SOC_THERM_MARGIN:
      sprintf(name, "SOC_THERM_MARGIN_TEMP");
      break;
    case BIC_SENSOR_VDDR_VR_TEMP:
      sprintf(name, "VDDR_VR_TEMP");
      break;
    case BIC_SENSOR_SOC_DIMMA0_TEMP:
      sprintf(name, "SOC_DIMMA0_TEMP");
      break;
    case BIC_SENSOR_SOC_DIMMB0_TEMP:
      sprintf(name, "SOC_DIMMB0_TEMP");
      break;
    case BIC_SENSOR_SOC_PACKAGE_PWR:
      sprintf(name, "SOC_PACKAGE_POWER");
      break;
    case BIC_SENSOR_VCCIN_VR_POUT:
      sprintf(name, "VCCIN_VR_OUT_POWER");
      break;
      case BIC_SENSOR_VDDR_VR_POUT:
      sprintf(name, "VDDR_VR_OUT_POWER");
      break;
    case BIC_SENSOR_SOC_TJMAX:
      sprintf(name, "SOC_TJMAX_TEMP");
      break;
    case BIC_SENSOR_P3V3_MB:
      sprintf(name, "P3V3_MB_VOLT");
      break;
    case BIC_SENSOR_P12V_MB:
      sprintf(name, "P12V_MB_VOLT");
      break;
    case BIC_SENSOR_P1V05_PCH:
      sprintf(name, "P1V05_PCH_VOLT");
      break;
    case BIC_SENSOR_P3V3_STBY_MB:
      sprintf(name, "P3V3_STBY_MB_VOLT");
      break;
    case BIC_SENSOR_P5V_STBY_MB:
      sprintf(name, "P5V_STBY_MB_VOLT");
      break;
    case BIC_SENSOR_PV_BAT:
      sprintf(name, "PV_BAT_VOLT");
      break;
    case BIC_SENSOR_P1V05_MIX:
      sprintf(name, "P1V05_MIX_VOLT");
      break;
    case BIC_SENSOR_1V05MIX_VR_CURR:
      sprintf(name, "1V05MIX_VR_CURR");
      break;
    case BIC_SENSOR_VDDR_VR_CURR:
      sprintf(name, "VDDR_VR_CURR");
      break;
    case BIC_SENSOR_VCCIN_VR_CURR:
      sprintf(name, "VCCIN_VR_CURR");
      break;
    case BIC_SENSOR_VCCIN_VR_VOL:
      sprintf(name, "VCCIN_VR_VOLT");
      break;
    case BIC_SENSOR_P1V05MIX_VR_POUT:
      sprintf(name, "BP1V05MIX_VR_OUT_POWER");
      break;
    case BIC_SENSOR_INA230_POWER:
      sprintf(name, "INA230_POWER");
      break;
    case BIC_SENSOR_INA230_VOL:
      sprintf(name, "INA230_VOLT");
      break;
    case BIC_SENSOR_SYSTEM_STATUS:
      sprintf(name, "SYSTEM_STATUS");
      break;
    case BIC_SENSOR_SYS_BOOT_STAT:
      sprintf(name, "SYS_BOOT_STAT");
      break;
    case BIC_SENSOR_CPU_DIMM_HOT:
      sprintf(name, "CPU_DIMM_HOT");
      break;
    case BIC_SENSOR_PROC_FAIL:
      sprintf(name, "PROC_FAIL");
      break;
    case BIC_SENSOR_VR_HOT:
      sprintf(name, "VR_HOT");
      break;
    default:
      return -1;
  }
  return 0;
}

static int
get_smb_sensor_name(uint8_t sensor_num, char *name) {

  switch(sensor_num) {
    case SMB_SENSOR_TH3_SERDES_TEMP:
      sprintf(name, "TH3_SERDES_TEMP");
      break;
    case SMB_SENSOR_TH3_CORE_TEMP:
      sprintf(name, "TH3_CORE_TEMP");
      break;
    case SMB_SENSOR_TEMP1:
      sprintf(name, "SMB_TEMP1");
      break;
    case SMB_SENSOR_TEMP2:
      sprintf(name, "SMB_TEMP2");
      break;
    case SMB_SENSOR_TEMP3:
      sprintf(name, "SMB_TEMP3");
      break;
    case SMB_SENSOR_TEMP4:
      sprintf(name, "SMB_TEMP4");
      break;
    case SMB_SENSOR_TEMP5:
      sprintf(name, "SMB_TEMP5");
      break;
    case SMB_SENSOR_TH3_DIE_TEMP1:
      sprintf(name, "TH3_DIE_TEMP1");
      break;
    case SMB_SENSOR_TH3_DIE_TEMP2:
      sprintf(name, "TH3_DIE_TEMP2");
      break;
    case SMB_SENSOR_PDB_L_TEMP1:
      sprintf(name, "PDB_L_TEMP1");
      break;
    case SMB_SENSOR_PDB_L_TEMP2:
      sprintf(name, "PDB_L_TEMP2");
      break;
    case SMB_SENSOR_PDB_R_TEMP1:
      sprintf(name, "PDB_R_TEMP1");
      break;
    case SMB_SENSOR_PDB_R_TEMP2:
      sprintf(name, "PDB_R_TEMP2");
      break;
    case SMB_SENSOR_FCM_T_TEMP1:
      sprintf(name, "FCM_T_TEMP1");
      break;
    case SMB_SENSOR_FCM_T_TEMP2:
      sprintf(name, "FCM_T_TEMP2");
      break;
    case SMB_SENSOR_FCM_B_TEMP1:
      sprintf(name, "FCM_B_TEMP1");
      break;
    case SMB_SENSOR_FCM_B_TEMP2:
      sprintf(name, "FCM_B_TEMP2");
      break;
    case SMB_SENSOR_1220_VMON1:
      sprintf(name, "POWR1220_VMON1");
      break;
    case SMB_SENSOR_1220_VMON2:
      sprintf(name, "POWR1220_VMON2");
      break;
    case SMB_SENSOR_1220_VMON3:
      sprintf(name, "POWR1220_VMON3");
      break;
    case SMB_SENSOR_1220_VMON4:
      sprintf(name, "POWR1220_VMON4");
      break;
    case SMB_SENSOR_1220_VMON5:
      sprintf(name, "POWR1220_VMON5");
      break;
    case SMB_SENSOR_1220_VMON6:
      sprintf(name, "POWR1220_VMON6");
      break;
    case SMB_SENSOR_1220_VMON7:
      sprintf(name, "POWR1220_VMON7");
      break;
    case SMB_SENSOR_1220_VMON8:
      sprintf(name, "POWR1220_VMON8");
      break;
    case SMB_SENSOR_1220_VMON9:
      sprintf(name, "POWR1220_VMON9");
      break;
    case SMB_SENSOR_1220_VMON10:
      sprintf(name, "POWR1220_VMON10");
      break;
    case SMB_SENSOR_1220_VMON11:
      sprintf(name, "POWR1220_VMON11");
      break;
    case SMB_SENSOR_1220_VMON12:
      sprintf(name, "POWR1220_VMON12");
      break;
    case SMB_SENSOR_1220_VCCA:
      sprintf(name, "POWR1220_VCCA");
      break;
    case SMB_SENSOR_1220_VCCINP:
      sprintf(name, "POWR1220_VCCINP");
      break;
    case SMB_SENSOR_TH3_SERDES_VOLT:
      sprintf(name, "TH3_SERDES_VOLT");
      break;
    case SMB_SENSOR_TH3_CORE_VOLT:
      sprintf(name, "TH3_CORE_VOLT");
      break;
    case SMB_SENSOR_FCM_T_HSC_VOLT:
      sprintf(name, "FCM_T_HSC_VOLT");
      break;
    case SMB_SENSOR_FCM_B_HSC_VOLT:
      sprintf(name, "FCM_B_HSC_VOLT");
      break;
    case SMB_SENSOR_TH3_SERDES_CURR:
      sprintf(name, "TH3_SERDES_CURR");
      break;
    case SMB_SENSOR_TH3_CORE_CURR:
      sprintf(name, "TH3_CORE_CURR");
      break;
    case SMB_SENSOR_FCM_T_HSC_CURR:
      sprintf(name, "FCM_T_HSC_CURR");
      break;
    case SMB_SENSOR_FCM_B_HSC_CURR:
      sprintf(name, "FCM_B_HSC_CURR");
      break;
    case SMB_SENSOR_FCM_T_HSC_POWER:
      sprintf(name, "FCM_T_HSC_POWER");
      break;
    case SMB_SENSOR_FCM_B_HSC_POWER:
      sprintf(name, "FCM_B_HSC_POWER");
      break;
    case SMB_SENSOR_FAN1_FRONT_TACH:
      sprintf(name, "FAN1_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN1_REAR_TACH:
      sprintf(name, "FAN1_REAR_SPEED");
      break;
    case SMB_SENSOR_FAN2_FRONT_TACH:
      sprintf(name, "FAN2_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN2_REAR_TACH:
      sprintf(name, "FAN2_REAR_SPEED");
      break;
    case SMB_SENSOR_FAN3_FRONT_TACH:
      sprintf(name, "FAN3_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN3_REAR_TACH:
      sprintf(name, "FAN3_REAR_SPEED");
      break;
    case SMB_SENSOR_FAN4_FRONT_TACH:
      sprintf(name, "FAN4_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN4_REAR_TACH:
      sprintf(name, "FAN4_REAR_SPEED");
      break;
    case SMB_SENSOR_FAN5_FRONT_TACH:
      sprintf(name, "FAN5_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN5_REAR_TACH:
      sprintf(name, "FAN5_REAR_SPEED");
      break;
    case SMB_SENSOR_FAN6_FRONT_TACH:
      sprintf(name, "FAN6_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN6_REAR_TACH:
      sprintf(name, "FAN6_REAR_SPEED");
      break;
    case SMB_SENSOR_FAN7_FRONT_TACH:
      sprintf(name, "FAN7_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN7_REAR_TACH:
      sprintf(name, "FAN7_REAR_SPEED");
      break;
    case SMB_SENSOR_FAN8_FRONT_TACH:
      sprintf(name, "FAN8_FRONT_SPEED");
      break;
    case SMB_SENSOR_FAN8_REAR_TACH:
      sprintf(name, "FAN8_REAR_SPEED");
      break;
    default:
      return -1;
  }
  return 0;
}

static int
get_pim_sensor_name(uint8_t sensor_num, char *name) {

  switch(sensor_num) {
    case PIM1_SENSOR_TEMP1:
      sprintf(name, "PIM1_TEMP1");
      break;
    case PIM1_SENSOR_TEMP2:
      sprintf(name, "PIM1_TEMP2");
      break;
    case PIM1_SENSOR_HSC_VOLT:
      sprintf(name, "PIM1_HSC_VOLT");
      break;
    case PIM1_SENSOR_HSC_CURR:
      sprintf(name, "PIM1_HSC_CURR");
      break;
    case PIM1_SENSOR_HSC_POWER:
      sprintf(name, "PIM1_HSC_POWER");
      break;
    case PIM1_SENSOR_34461_VOLT1:
      sprintf(name, "PIM1_MAX34461_VOLT1");
      break;
    case PIM1_SENSOR_34461_VOLT2:
      sprintf(name, "PIM1_MAX34461_VOLT2");
      break;
    case PIM1_SENSOR_34461_VOLT3:
      sprintf(name, "PIM1_MAX34461_VOLT3");
      break;
    case PIM1_SENSOR_34461_VOLT4:
      sprintf(name, "PIM1_MAX34461_VOLT4");
      break;
    case PIM1_SENSOR_34461_VOLT5:
      sprintf(name, "PIM1_MAX34461_VOLT5");
      break;
    case PIM1_SENSOR_34461_VOLT6:
      sprintf(name, "PIM1_MAX34461_VOLT6");
      break;
    case PIM1_SENSOR_34461_VOLT7:
      sprintf(name, "PIM1_MAX34461_VOLT7");
      break;
    case PIM1_SENSOR_34461_VOLT8:
      sprintf(name, "PIM1_MAX34461_VOLT8");
      break;
    case PIM1_SENSOR_34461_VOLT9:
      sprintf(name, "PIM1_MAX34461_VOLT9");
      break;
    case PIM1_SENSOR_34461_VOLT10:
      sprintf(name, "PIM1_MAX34461_VOLT10");
      break;
    case PIM1_SENSOR_34461_VOLT11:
      sprintf(name, "PIM1_MAX34461_VOLT11");
      break;
    case PIM1_SENSOR_34461_VOLT12:
      sprintf(name, "PIM1_MAX34461_VOLT12");
      break;
    case PIM1_SENSOR_34461_VOLT13:
      sprintf(name, "PIM1_MAX34461_VOLT13");
      break;
    case PIM1_SENSOR_34461_VOLT14:
      sprintf(name, "PIM1_MAX34461_VOLT14");
      break;
    case PIM1_SENSOR_34461_VOLT15:
      sprintf(name, "PIM1_MAX34461_VOLT15");
      break;
    case PIM1_SENSOR_34461_VOLT16:
      sprintf(name, "PIM1_MAX34461_VOLT16");
      break;
    case PIM2_SENSOR_TEMP1:
      sprintf(name, "PIM2_TEMP1");
      break;
    case PIM2_SENSOR_TEMP2:
      sprintf(name, "PIM2_TEMP2");
      break;
    case PIM2_SENSOR_HSC_VOLT:
      sprintf(name, "PIM2_HSC_VOLT");
      break;
    case PIM2_SENSOR_HSC_CURR:
      sprintf(name, "PIM2_HSC_CURR");
      break;
    case PIM2_SENSOR_HSC_POWER:
      sprintf(name, "PIM2_HSC_POWER");
      break;
    case PIM2_SENSOR_34461_VOLT1:
      sprintf(name, "PIM2_MAX34461_VOLT1");
      break;
    case PIM2_SENSOR_34461_VOLT2:
      sprintf(name, "PIM2_MAX34461_VOLT2");
      break;
    case PIM2_SENSOR_34461_VOLT3:
      sprintf(name, "PIM2_MAX34461_VOLT3");
      break;
    case PIM2_SENSOR_34461_VOLT4:
      sprintf(name, "PIM2_MAX34461_VOLT4");
      break;
    case PIM2_SENSOR_34461_VOLT5:
      sprintf(name, "PIM2_MAX34461_VOLT5");
      break;
    case PIM2_SENSOR_34461_VOLT6:
      sprintf(name, "PIM2_MAX34461_VOLT6");
      break;
    case PIM2_SENSOR_34461_VOLT7:
      sprintf(name, "PIM2_MAX34461_VOLT7");
      break;
    case PIM2_SENSOR_34461_VOLT8:
      sprintf(name, "PIM2_MAX34461_VOLT8");
      break;
    case PIM2_SENSOR_34461_VOLT9:
      sprintf(name, "PIM2_MAX34461_VOLT9");
      break;
    case PIM2_SENSOR_34461_VOLT10:
      sprintf(name, "PIM2_MAX34461_VOLT10");
      break;
    case PIM2_SENSOR_34461_VOLT11:
      sprintf(name, "PIM2_MAX34461_VOLT11");
      break;
    case PIM2_SENSOR_34461_VOLT12:
      sprintf(name, "PIM2_MAX34461_VOLT12");
      break;
    case PIM2_SENSOR_34461_VOLT13:
      sprintf(name, "PIM2_MAX34461_VOLT13");
      break;
    case PIM2_SENSOR_34461_VOLT14:
      sprintf(name, "PIM2_MAX34461_VOLT14");
      break;
    case PIM2_SENSOR_34461_VOLT15:
      sprintf(name, "PIM2_MAX34461_VOLT15");
      break;
    case PIM2_SENSOR_34461_VOLT16:
      sprintf(name, "PIM2_MAX34461_VOLT16");
      break;
    case PIM3_SENSOR_TEMP1:
      sprintf(name, "PIM3_TEMP1");
      break;
    case PIM3_SENSOR_TEMP2:
      sprintf(name, "PIM3_TEMP2");
      break;
    case PIM3_SENSOR_HSC_VOLT:
      sprintf(name, "PIM3_HSC_VOLT");
      break;
    case PIM3_SENSOR_HSC_CURR:
      sprintf(name, "PIM3_HSC_CURR");
      break;
    case PIM3_SENSOR_HSC_POWER:
      sprintf(name, "PIM3_HSC_POWER");
      break;
    case PIM3_SENSOR_34461_VOLT1:
      sprintf(name, "PIM3_MAX34461_VOLT1");
      break;
    case PIM3_SENSOR_34461_VOLT2:
      sprintf(name, "PIM3_MAX34461_VOLT2");
      break;
    case PIM3_SENSOR_34461_VOLT3:
      sprintf(name, "PIM3_MAX34461_VOLT3");
      break;
    case PIM3_SENSOR_34461_VOLT4:
      sprintf(name, "PIM3_MAX34461_VOLT4");
      break;
    case PIM3_SENSOR_34461_VOLT5:
      sprintf(name, "PIM3_MAX34461_VOLT5");
      break;
    case PIM3_SENSOR_34461_VOLT6:
      sprintf(name, "PIM3_MAX34461_VOLT6");
      break;
    case PIM3_SENSOR_34461_VOLT7:
      sprintf(name, "PIM3_MAX34461_VOLT7");
      break;
    case PIM3_SENSOR_34461_VOLT8:
      sprintf(name, "PIM3_MAX34461_VOLT8");
      break;
    case PIM3_SENSOR_34461_VOLT9:
      sprintf(name, "PIM3_MAX34461_VOLT9");
      break;
    case PIM3_SENSOR_34461_VOLT10:
      sprintf(name, "PIM3_MAX34461_VOLT10");
      break;
    case PIM3_SENSOR_34461_VOLT11:
      sprintf(name, "PIM3_MAX34461_VOLT11");
      break;
    case PIM3_SENSOR_34461_VOLT12:
      sprintf(name, "PIM3_MAX34461_VOLT12");
      break;
    case PIM3_SENSOR_34461_VOLT13:
      sprintf(name, "PIM3_MAX34461_VOLT13");
      break;
    case PIM3_SENSOR_34461_VOLT14:
      sprintf(name, "PIM3_MAX34461_VOLT14");
      break;
    case PIM3_SENSOR_34461_VOLT15:
      sprintf(name, "PIM3_MAX34461_VOLT15");
      break;
    case PIM3_SENSOR_34461_VOLT16:
      sprintf(name, "PIM3_MAX34461_VOLT16");
      break;
    case PIM4_SENSOR_TEMP1:
      sprintf(name, "PIM4_TEMP1");
      break;
    case PIM4_SENSOR_TEMP2:
      sprintf(name, "PIM4_TEMP2");
      break;
    case PIM4_SENSOR_HSC_VOLT:
      sprintf(name, "PIM4_HSC_VOLT");
      break;
    case PIM4_SENSOR_HSC_CURR:
      sprintf(name, "PIM4_HSC_CURR");
      break;
    case PIM4_SENSOR_HSC_POWER:
      sprintf(name, "PIM4_HSC_POWER");
      break;
    case PIM4_SENSOR_34461_VOLT1:
      sprintf(name, "PIM4_MAX34461_VOLT1");
      break;
    case PIM4_SENSOR_34461_VOLT2:
      sprintf(name, "PIM4_MAX34461_VOLT2");
      break;
    case PIM4_SENSOR_34461_VOLT3:
      sprintf(name, "PIM4_MAX34461_VOLT3");
      break;
    case PIM4_SENSOR_34461_VOLT4:
      sprintf(name, "PIM4_MAX34461_VOLT4");
      break;
    case PIM4_SENSOR_34461_VOLT5:
      sprintf(name, "PIM4_MAX34461_VOLT5");
      break;
    case PIM4_SENSOR_34461_VOLT6:
      sprintf(name, "PIM4_MAX34461_VOLT6");
      break;
    case PIM4_SENSOR_34461_VOLT7:
      sprintf(name, "PIM4_MAX34461_VOLT7");
      break;
    case PIM4_SENSOR_34461_VOLT8:
      sprintf(name, "PIM4_MAX34461_VOLT8");
      break;
    case PIM4_SENSOR_34461_VOLT9:
      sprintf(name, "PIM4_MAX34461_VOLT9");
      break;
    case PIM4_SENSOR_34461_VOLT10:
      sprintf(name, "PIM4_MAX34461_VOLT10");
      break;
    case PIM4_SENSOR_34461_VOLT11:
      sprintf(name, "PIM4_MAX34461_VOLT11");
      break;
    case PIM4_SENSOR_34461_VOLT12:
      sprintf(name, "PIM4_MAX34461_VOLT12");
      break;
    case PIM4_SENSOR_34461_VOLT13:
      sprintf(name, "PIM4_MAX34461_VOLT13");
      break;
    case PIM4_SENSOR_34461_VOLT14:
      sprintf(name, "PIM4_MAX34461_VOLT14");
      break;
    case PIM4_SENSOR_34461_VOLT15:
      sprintf(name, "PIM4_MAX34461_VOLT15");
      break;
    case PIM4_SENSOR_34461_VOLT16:
      sprintf(name, "PIM4_MAX34461_VOLT16");
      break;
    case PIM5_SENSOR_TEMP1:
      sprintf(name, "PIM5_TEMP1");
      break;
    case PIM5_SENSOR_TEMP2:
      sprintf(name, "PIM5_TEMP2");
      break;
    case PIM5_SENSOR_HSC_VOLT:
      sprintf(name, "PIM5_HSC_VOLT");
      break;
    case PIM5_SENSOR_HSC_CURR:
      sprintf(name, "PIM5_HSC_CURR");
      break;
    case PIM5_SENSOR_HSC_POWER:
      sprintf(name, "PIM5_HSC_POWER");
      break;
    case PIM5_SENSOR_34461_VOLT1:
      sprintf(name, "PIM5_MAX34461_VOLT1");
      break;
    case PIM5_SENSOR_34461_VOLT2:
      sprintf(name, "PIM5_MAX34461_VOLT2");
      break;
    case PIM5_SENSOR_34461_VOLT3:
      sprintf(name, "PIM5_MAX34461_VOLT3");
      break;
    case PIM5_SENSOR_34461_VOLT4:
      sprintf(name, "PIM5_MAX34461_VOLT4");
      break;
    case PIM5_SENSOR_34461_VOLT5:
      sprintf(name, "PIM5_MAX34461_VOLT5");
      break;
    case PIM5_SENSOR_34461_VOLT6:
      sprintf(name, "PIM5_MAX34461_VOLT6");
      break;
    case PIM5_SENSOR_34461_VOLT7:
      sprintf(name, "PIM5_MAX34461_VOLT7");
      break;
    case PIM5_SENSOR_34461_VOLT8:
      sprintf(name, "PIM5_MAX34461_VOLT8");
      break;
    case PIM5_SENSOR_34461_VOLT9:
      sprintf(name, "PIM5_MAX34461_VOLT9");
      break;
    case PIM5_SENSOR_34461_VOLT10:
      sprintf(name, "PIM5_MAX34461_VOLT10");
      break;
    case PIM5_SENSOR_34461_VOLT11:
      sprintf(name, "PIM5_MAX34461_VOLT11");
      break;
    case PIM5_SENSOR_34461_VOLT12:
      sprintf(name, "PIM5_MAX34461_VOLT12");
      break;
    case PIM5_SENSOR_34461_VOLT13:
      sprintf(name, "PIM5_MAX34461_VOLT13");
      break;
    case PIM5_SENSOR_34461_VOLT14:
      sprintf(name, "PIM5_MAX34461_VOLT14");
      break;
    case PIM5_SENSOR_34461_VOLT15:
      sprintf(name, "PIM5_MAX34461_VOLT15");
      break;
    case PIM5_SENSOR_34461_VOLT16:
      sprintf(name, "PIM5_MAX34461_VOLT16");
      break;
    case PIM6_SENSOR_TEMP1:
      sprintf(name, "PIM6_TEMP1");
      break;
    case PIM6_SENSOR_TEMP2:
      sprintf(name, "PIM6_TEMP2");
      break;
    case PIM6_SENSOR_HSC_VOLT:
      sprintf(name, "PIM6_HSC_VOLT");
      break;
    case PIM6_SENSOR_HSC_CURR:
      sprintf(name, "PIM6_HSC_CURR");
      break;
    case PIM6_SENSOR_HSC_POWER:
      sprintf(name, "PIM6_HSC_POWER");
      break;
    case PIM6_SENSOR_34461_VOLT1:
      sprintf(name, "PIM6_MAX34461_VOLT1");
      break;
    case PIM6_SENSOR_34461_VOLT2:
      sprintf(name, "PIM6_MAX34461_VOLT2");
      break;
    case PIM6_SENSOR_34461_VOLT3:
      sprintf(name, "PIM6_MAX34461_VOLT3");
      break;
    case PIM6_SENSOR_34461_VOLT4:
      sprintf(name, "PIM6_MAX34461_VOLT4");
      break;
    case PIM6_SENSOR_34461_VOLT5:
      sprintf(name, "PIM6_MAX34461_VOLT5");
      break;
    case PIM6_SENSOR_34461_VOLT6:
      sprintf(name, "PIM6_MAX34461_VOLT6");
      break;
    case PIM6_SENSOR_34461_VOLT7:
      sprintf(name, "PIM6_MAX34461_VOLT7");
      break;
    case PIM6_SENSOR_34461_VOLT8:
      sprintf(name, "PIM6_MAX34461_VOLT8");
      break;
    case PIM6_SENSOR_34461_VOLT9:
      sprintf(name, "PIM6_MAX34461_VOLT9");
      break;
    case PIM6_SENSOR_34461_VOLT10:
      sprintf(name, "PIM6_MAX34461_VOLT10");
      break;
    case PIM6_SENSOR_34461_VOLT11:
      sprintf(name, "PIM6_MAX34461_VOLT11");
      break;
    case PIM6_SENSOR_34461_VOLT12:
      sprintf(name, "PIM6_MAX34461_VOLT12");
      break;
    case PIM6_SENSOR_34461_VOLT13:
      sprintf(name, "PIM6_MAX34461_VOLT13");
      break;
    case PIM6_SENSOR_34461_VOLT14:
      sprintf(name, "PIM6_MAX34461_VOLT14");
      break;
    case PIM6_SENSOR_34461_VOLT15:
      sprintf(name, "PIM6_MAX34461_VOLT15");
      break;
    case PIM6_SENSOR_34461_VOLT16:
      sprintf(name, "PIM6_MAX34461_VOLT16");
      break;
    case PIM7_SENSOR_TEMP1:
      sprintf(name, "PIM7_TEMP1");
      break;
    case PIM7_SENSOR_TEMP2:
      sprintf(name, "PIM7_TEMP2");
      break;
    case PIM7_SENSOR_HSC_VOLT:
      sprintf(name, "PIM7_HSC_VOLT");
      break;
    case PIM7_SENSOR_HSC_CURR:
      sprintf(name, "PIM7_HSC_CURR");
      break;
    case PIM7_SENSOR_HSC_POWER:
      sprintf(name, "PIM7_HSC_POWER");
      break;
    case PIM7_SENSOR_34461_VOLT1:
      sprintf(name, "PIM7_MAX34461_VOLT1");
      break;
    case PIM7_SENSOR_34461_VOLT2:
      sprintf(name, "PIM7_MAX34461_VOLT2");
      break;
    case PIM7_SENSOR_34461_VOLT3:
      sprintf(name, "PIM7_MAX34461_VOLT3");
      break;
    case PIM7_SENSOR_34461_VOLT4:
      sprintf(name, "PIM7_MAX34461_VOLT4");
      break;
    case PIM7_SENSOR_34461_VOLT5:
      sprintf(name, "PIM7_MAX34461_VOLT5");
      break;
    case PIM7_SENSOR_34461_VOLT6:
      sprintf(name, "PIM7_MAX34461_VOLT6");
      break;
    case PIM7_SENSOR_34461_VOLT7:
      sprintf(name, "PIM7_MAX34461_VOLT7");
      break;
    case PIM7_SENSOR_34461_VOLT8:
      sprintf(name, "PIM7_MAX34461_VOLT8");
      break;
    case PIM7_SENSOR_34461_VOLT9:
      sprintf(name, "PIM7_MAX34461_VOLT9");
      break;
    case PIM7_SENSOR_34461_VOLT10:
      sprintf(name, "PIM7_MAX34461_VOLT10");
      break;
    case PIM7_SENSOR_34461_VOLT11:
      sprintf(name, "PIM7_MAX34461_VOLT11");
      break;
    case PIM7_SENSOR_34461_VOLT12:
      sprintf(name, "PIM7_MAX34461_VOLT12");
      break;
    case PIM7_SENSOR_34461_VOLT13:
      sprintf(name, "PIM7_MAX34461_VOLT13");
      break;
    case PIM7_SENSOR_34461_VOLT14:
      sprintf(name, "PIM7_MAX34461_VOLT14");
      break;
    case PIM7_SENSOR_34461_VOLT15:
      sprintf(name, "PIM7_MAX34461_VOLT15");
      break;
    case PIM7_SENSOR_34461_VOLT16:
      sprintf(name, "PIM7_MAX34461_VOLT16");
      break;
    case PIM8_SENSOR_TEMP1:
      sprintf(name, "PIM8_TEMP1");
      break;
    case PIM8_SENSOR_TEMP2:
      sprintf(name, "PIM8_TEMP2");
      break;
    case PIM8_SENSOR_HSC_VOLT:
      sprintf(name, "PIM8_HSC_VOLT");
      break;
    case PIM8_SENSOR_HSC_CURR:
      sprintf(name, "PIM8_HSC_CURR");
      break;
    case PIM8_SENSOR_HSC_POWER:
      sprintf(name, "PIM8_HSC_POWER");
      break;
    case PIM8_SENSOR_34461_VOLT1:
      sprintf(name, "PIM8_MAX34461_VOLT1");
      break;
    case PIM8_SENSOR_34461_VOLT2:
      sprintf(name, "PIM8_MAX34461_VOLT2");
      break;
    case PIM8_SENSOR_34461_VOLT3:
      sprintf(name, "PIM8_MAX34461_VOLT3");
      break;
    case PIM8_SENSOR_34461_VOLT4:
      sprintf(name, "PIM8_MAX34461_VOLT4");
      break;
    case PIM8_SENSOR_34461_VOLT5:
      sprintf(name, "PIM8_MAX34461_VOLT5");
      break;
    case PIM8_SENSOR_34461_VOLT6:
      sprintf(name, "PIM8_MAX34461_VOLT6");
      break;
    case PIM8_SENSOR_34461_VOLT7:
      sprintf(name, "PIM8_MAX34461_VOLT7");
      break;
    case PIM8_SENSOR_34461_VOLT8:
      sprintf(name, "PIM8_MAX34461_VOLT8");
      break;
    case PIM8_SENSOR_34461_VOLT9:
      sprintf(name, "PIM8_MAX34461_VOLT9");
      break;
    case PIM8_SENSOR_34461_VOLT10:
      sprintf(name, "PIM8_MAX34461_VOLT10");
      break;
    case PIM8_SENSOR_34461_VOLT11:
      sprintf(name, "PIM8_MAX34461_VOLT11");
      break;
    case PIM8_SENSOR_34461_VOLT12:
      sprintf(name, "PIM8_MAX34461_VOLT12");
      break;
    case PIM8_SENSOR_34461_VOLT13:
      sprintf(name, "PIM8_MAX34461_VOLT13");
      break;
    case PIM8_SENSOR_34461_VOLT14:
      sprintf(name, "PIM8_MAX34461_VOLT14");
      break;
    case PIM8_SENSOR_34461_VOLT15:
      sprintf(name, "PIM8_MAX34461_VOLT15");
      break;
    case PIM8_SENSOR_34461_VOLT16:
      sprintf(name, "PIM8_MAX34461_VOLT16");
      break;
    default:
      return -1;
  }
  return 0;
}

static int
get_psu_sensor_name(uint8_t sensor_num, char *name) {

  switch(sensor_num) {
    case PSU1_SENSOR_IN_VOLT:
      sprintf(name, "PSU1_IN_VOLT");
      break;
    case PSU1_SENSOR_12V_VOLT:
      sprintf(name, "PSU1_12V_VOLT");
      break;
    case PSU1_SENSOR_STBY_VOLT:
      sprintf(name, "PSU1_STBY_VOLT");
      break;
    case PSU1_SENSOR_IN_CURR:
      sprintf(name, "PSU1_IN_CURR");
      break;
    case PSU1_SENSOR_12V_CURR:
      sprintf(name, "PSU1_12V_CURR");
      break;
    case PSU1_SENSOR_STBY_CURR:
      sprintf(name, "PSU1_STBY_CURR");
      break;
    case PSU1_SENSOR_IN_POWER:
      sprintf(name, "PSU1_IN_POWER");
      break;
    case PSU1_SENSOR_12V_POWER:
      sprintf(name, "PSU1_12V_POWER");
      break;
    case PSU1_SENSOR_STBY_POWER:
      sprintf(name, "PSU1_STBY_POWER");
      break;
    case PSU1_SENSOR_FAN_TACH:
      sprintf(name, "PSU1_FAN_SPEED");
      break;
    case PSU1_SENSOR_TEMP1:
      sprintf(name, "PSU1_TEMP1");
      break;
    case PSU1_SENSOR_TEMP2:
      sprintf(name, "PSU1_TEMP2");
      break;
    case PSU1_SENSOR_TEMP3:
      sprintf(name, "PSU1_TEMP3");
      break;
    case PSU2_SENSOR_IN_VOLT:
      sprintf(name, "PSU2_IN_VOLT");
      break;
    case PSU2_SENSOR_12V_VOLT:
      sprintf(name, "PSU2_12V_VOLT");
      break;
    case PSU2_SENSOR_STBY_VOLT:
      sprintf(name, "PSU2_STBY_VOLT");
      break;
    case PSU2_SENSOR_IN_CURR:
      sprintf(name, "PSU2_IN_CURR");
      break;
    case PSU2_SENSOR_12V_CURR:
      sprintf(name, "PSU2_12V_CURR");
      break;
    case PSU2_SENSOR_STBY_CURR:
      sprintf(name, "PSU2_STBY_CURR");
      break;
    case PSU2_SENSOR_IN_POWER:
      sprintf(name, "PSU2_IN_POWER");
      break;
    case PSU2_SENSOR_12V_POWER:
      sprintf(name, "PSU2_12V_POWER");
      break;
    case PSU2_SENSOR_STBY_POWER:
      sprintf(name, "PSU2_STBY_POWER");
      break;
    case PSU2_SENSOR_FAN_TACH:
      sprintf(name, "PSU2_FAN_SPEED");
      break;
    case PSU2_SENSOR_TEMP1:
      sprintf(name, "PSU2_TEMP1");
      break;
    case PSU2_SENSOR_TEMP2:
      sprintf(name, "PSU2_TEMP2");
      break;
    case PSU2_SENSOR_TEMP3:
      sprintf(name, "PSU2_TEMP3");
      break;
    case PSU3_SENSOR_IN_VOLT:
      sprintf(name, "PSU3_IN_VOLT");
      break;
    case PSU3_SENSOR_12V_VOLT:
      sprintf(name, "PSU3_12V_VOLT");
      break;
    case PSU3_SENSOR_STBY_VOLT:
      sprintf(name, "PSU3_STBY_VOLT");
      break;
    case PSU3_SENSOR_IN_CURR:
      sprintf(name, "PSU3_IN_CURR");
      break;
    case PSU3_SENSOR_12V_CURR:
      sprintf(name, "PSU3_12V_CURR");
      break;
    case PSU3_SENSOR_STBY_CURR:
      sprintf(name, "PSU3_STBY_CURR");
      break;
    case PSU3_SENSOR_IN_POWER:
      sprintf(name, "PSU3_IN_POWER");
      break;
    case PSU3_SENSOR_12V_POWER:
      sprintf(name, "PSU3_12V_POWER");
      break;
    case PSU3_SENSOR_STBY_POWER:
      sprintf(name, "PSU3_STBY_POWER");
      break;
    case PSU3_SENSOR_FAN_TACH:
      sprintf(name, "PSU3_FAN_SPEED");
      break;
    case PSU3_SENSOR_TEMP1:
      sprintf(name, "PSU3_TEMP1");
      break;
    case PSU3_SENSOR_TEMP2:
      sprintf(name, "PSU3_TEMP2");
      break;
    case PSU3_SENSOR_TEMP3:
      sprintf(name, "PSU3_TEMP3");
      break;
    case PSU4_SENSOR_IN_VOLT:
      sprintf(name, "PSU4_IN_VOLT");
      break;
    case PSU4_SENSOR_12V_VOLT:
      sprintf(name, "PSU4_12V_VOLT");
      break;
    case PSU4_SENSOR_STBY_VOLT:
      sprintf(name, "PSU4_STBY_VOLT");
      break;
    case PSU4_SENSOR_IN_CURR:
      sprintf(name, "PSU4_IN_CURR");
      break;
    case PSU4_SENSOR_12V_CURR:
      sprintf(name, "PSU4_12V_CURR");
      break;
    case PSU4_SENSOR_STBY_CURR:
      sprintf(name, "PSU4_STBY_CURR");
      break;
    case PSU4_SENSOR_IN_POWER:
      sprintf(name, "PSU4_IN_POWER");
      break;
    case PSU4_SENSOR_12V_POWER:
      sprintf(name, "PSU4_12V_POWER");
      break;
    case PSU4_SENSOR_STBY_POWER:
      sprintf(name, "PSU4_STBY_POWER");
      break;
    case PSU4_SENSOR_FAN_TACH:
      sprintf(name, "PSU4_FAN_SPEED");
      break;
    case PSU4_SENSOR_TEMP1:
      sprintf(name, "PSU4_TEMP1");
      break;
    case PSU4_SENSOR_TEMP2:
      sprintf(name, "PSU4_TEMP2");
      break;
    case PSU4_SENSOR_TEMP3:
      sprintf(name, "PSU4_TEMP3");
      break;
    default:
      return -1;
  }
  return 0;
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {

  int ret = -1;

  switch(fru) {
    case FRU_SCM:
      ret = get_scm_sensor_name(sensor_num, name);
      break;
    case FRU_SMB:
      ret = get_smb_sensor_name(sensor_num, name);
      break;
    case FRU_PIM1:
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
      ret = get_pim_sensor_name(sensor_num, name);
      break;
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      ret = get_psu_sensor_name(sensor_num, name);
      break;
    default:
      return -1;
  }
  return ret;
}

static int
get_scm_sensor_units(uint8_t sensor_num, char *units) {

  switch(sensor_num) {
    case SCM_SENSOR_OUTLET_LOCAL_TEMP:
    case SCM_SENSOR_OUTLET_REMOTE_TEMP:
    case SCM_SENSOR_INLET_LOCAL_TEMP:
    case SCM_SENSOR_INLET_REMOTE_TEMP:
    case BIC_SENSOR_MB_OUTLET_TEMP:
    case BIC_SENSOR_MB_INLET_TEMP:
    case BIC_SENSOR_PCH_TEMP:
    case BIC_SENSOR_VCCIN_VR_TEMP:
    case BIC_SENSOR_1V05MIX_VR_TEMP:
    case BIC_SENSOR_SOC_TEMP:
    case BIC_SENSOR_SOC_THERM_MARGIN:
    case BIC_SENSOR_VDDR_VR_TEMP:
    case BIC_SENSOR_SOC_DIMMA0_TEMP:
    case BIC_SENSOR_SOC_DIMMB0_TEMP:
    case BIC_SENSOR_SOC_TJMAX:
      sprintf(units, "C");
      break;
    case SCM_SENSOR_HSC_VOLT:
    case BIC_SENSOR_P3V3_MB:
    case BIC_SENSOR_P12V_MB:
    case BIC_SENSOR_P1V05_PCH:
    case BIC_SENSOR_P3V3_STBY_MB:
    case BIC_SENSOR_P5V_STBY_MB:
    case BIC_SENSOR_PV_BAT:
    case BIC_SENSOR_P1V05_MIX:
    case BIC_SENSOR_VCCIN_VR_VOL:
    case BIC_SENSOR_INA230_VOL:
      sprintf(units, "Volts");
      break;
    case SCM_SENSOR_HSC_CURR:
    case BIC_SENSOR_1V05MIX_VR_CURR:
    case BIC_SENSOR_VDDR_VR_CURR:
    case BIC_SENSOR_VCCIN_VR_CURR:
      sprintf(units, "Amps");
      break;
    case SCM_SENSOR_HSC_POWER:
    case BIC_SENSOR_SOC_PACKAGE_PWR:
    case BIC_SENSOR_VCCIN_VR_POUT:
    case BIC_SENSOR_VDDR_VR_POUT:
    case BIC_SENSOR_P1V05MIX_VR_POUT:
    case BIC_SENSOR_INA230_POWER:
      sprintf(units, "Watts");
      break;
    default:
      return -1;
  }
  return 0;
}

static int
get_smb_sensor_units(uint8_t sensor_num, char *units) {

  switch(sensor_num) {
    case SMB_SENSOR_TH3_SERDES_TEMP:
    case SMB_SENSOR_TH3_CORE_TEMP:
    case SMB_SENSOR_TEMP1:
    case SMB_SENSOR_TEMP2:
    case SMB_SENSOR_TEMP3:
    case SMB_SENSOR_TEMP4:
    case SMB_SENSOR_TEMP5:
    case SMB_SENSOR_TH3_DIE_TEMP1:
    case SMB_SENSOR_TH3_DIE_TEMP2:
    case SMB_SENSOR_PDB_L_TEMP1:
    case SMB_SENSOR_PDB_L_TEMP2:
    case SMB_SENSOR_PDB_R_TEMP1:
    case SMB_SENSOR_PDB_R_TEMP2:
    case SMB_SENSOR_FCM_T_TEMP1:
    case SMB_SENSOR_FCM_T_TEMP2:
    case SMB_SENSOR_FCM_B_TEMP1:
    case SMB_SENSOR_FCM_B_TEMP2:
      sprintf(units, "C");
      break;
    case SMB_SENSOR_1220_VMON1:
    case SMB_SENSOR_1220_VMON2:
    case SMB_SENSOR_1220_VMON3:
    case SMB_SENSOR_1220_VMON4:
    case SMB_SENSOR_1220_VMON5:
    case SMB_SENSOR_1220_VMON6:
    case SMB_SENSOR_1220_VMON7:
    case SMB_SENSOR_1220_VMON8:
    case SMB_SENSOR_1220_VMON9:
    case SMB_SENSOR_1220_VMON10:
    case SMB_SENSOR_1220_VMON11:
    case SMB_SENSOR_1220_VMON12:
    case SMB_SENSOR_1220_VCCA:
    case SMB_SENSOR_1220_VCCINP:
    case SMB_SENSOR_TH3_SERDES_VOLT:
    case SMB_SENSOR_TH3_CORE_VOLT:
    case SMB_SENSOR_FCM_T_HSC_VOLT:
    case SMB_SENSOR_FCM_B_HSC_VOLT:
      sprintf(units, "Volts");
      break;
    case SMB_SENSOR_TH3_SERDES_CURR:
    case SMB_SENSOR_TH3_CORE_CURR:
    case SMB_SENSOR_FCM_T_HSC_CURR:
    case SMB_SENSOR_FCM_B_HSC_CURR:
      sprintf(units, "Amps");
      break;
    case SMB_SENSOR_FCM_T_HSC_POWER:
    case SMB_SENSOR_FCM_B_HSC_POWER:
      sprintf(units, "Watts");
      break;
    case SMB_SENSOR_FAN1_FRONT_TACH:
    case SMB_SENSOR_FAN1_REAR_TACH:
    case SMB_SENSOR_FAN2_FRONT_TACH:
    case SMB_SENSOR_FAN2_REAR_TACH:
    case SMB_SENSOR_FAN3_FRONT_TACH:
    case SMB_SENSOR_FAN3_REAR_TACH:
    case SMB_SENSOR_FAN4_FRONT_TACH:
    case SMB_SENSOR_FAN4_REAR_TACH:
    case SMB_SENSOR_FAN5_FRONT_TACH:
    case SMB_SENSOR_FAN5_REAR_TACH:
    case SMB_SENSOR_FAN6_FRONT_TACH:
    case SMB_SENSOR_FAN6_REAR_TACH:
    case SMB_SENSOR_FAN7_FRONT_TACH:
    case SMB_SENSOR_FAN7_REAR_TACH:
    case SMB_SENSOR_FAN8_FRONT_TACH:
    case SMB_SENSOR_FAN8_REAR_TACH:
      sprintf(units, "RPM");
      break;
    default:
      return -1;
  }
  return 0;
}

static int
get_pim_sensor_units(uint8_t sensor_num, char *units) {

  switch(sensor_num) {
    case PIM1_SENSOR_TEMP1:
    case PIM1_SENSOR_TEMP2:
    case PIM2_SENSOR_TEMP1:
    case PIM2_SENSOR_TEMP2:
    case PIM3_SENSOR_TEMP1:
    case PIM3_SENSOR_TEMP2:
    case PIM4_SENSOR_TEMP1:
    case PIM4_SENSOR_TEMP2:
    case PIM5_SENSOR_TEMP1:
    case PIM5_SENSOR_TEMP2:
    case PIM6_SENSOR_TEMP1:
    case PIM6_SENSOR_TEMP2:
    case PIM7_SENSOR_TEMP1:
    case PIM7_SENSOR_TEMP2:
    case PIM8_SENSOR_TEMP1:
    case PIM8_SENSOR_TEMP2:
      sprintf(units, "C");
      break;
    case PIM1_SENSOR_HSC_CURR:
    case PIM2_SENSOR_HSC_CURR:
    case PIM3_SENSOR_HSC_CURR:
    case PIM4_SENSOR_HSC_CURR:
    case PIM5_SENSOR_HSC_CURR:
    case PIM6_SENSOR_HSC_CURR:
    case PIM7_SENSOR_HSC_CURR:
    case PIM8_SENSOR_HSC_CURR:
      sprintf(units, "Amps");
      break;
    case PIM1_SENSOR_HSC_POWER:
    case PIM2_SENSOR_HSC_POWER:
    case PIM3_SENSOR_HSC_POWER:
    case PIM4_SENSOR_HSC_POWER:
    case PIM5_SENSOR_HSC_POWER:
    case PIM6_SENSOR_HSC_POWER:
    case PIM7_SENSOR_HSC_POWER:
    case PIM8_SENSOR_HSC_POWER:
      sprintf(units, "Watts");
      break;
    case PIM1_SENSOR_HSC_VOLT:
    case PIM2_SENSOR_HSC_VOLT:
    case PIM3_SENSOR_HSC_VOLT:
    case PIM4_SENSOR_HSC_VOLT:
    case PIM5_SENSOR_HSC_VOLT:
    case PIM6_SENSOR_HSC_VOLT:
    case PIM7_SENSOR_HSC_VOLT:
    case PIM8_SENSOR_HSC_VOLT:
    case PIM1_SENSOR_34461_VOLT1:
    case PIM1_SENSOR_34461_VOLT2:
    case PIM1_SENSOR_34461_VOLT3:
    case PIM1_SENSOR_34461_VOLT4:
    case PIM1_SENSOR_34461_VOLT5:
    case PIM1_SENSOR_34461_VOLT6:
    case PIM1_SENSOR_34461_VOLT7:
    case PIM1_SENSOR_34461_VOLT8:
    case PIM1_SENSOR_34461_VOLT9:
    case PIM1_SENSOR_34461_VOLT10:
    case PIM1_SENSOR_34461_VOLT11:
    case PIM1_SENSOR_34461_VOLT12:
    case PIM1_SENSOR_34461_VOLT13:
    case PIM1_SENSOR_34461_VOLT14:
    case PIM1_SENSOR_34461_VOLT15:
    case PIM1_SENSOR_34461_VOLT16:
    case PIM2_SENSOR_34461_VOLT1:
    case PIM2_SENSOR_34461_VOLT2:
    case PIM2_SENSOR_34461_VOLT3:
    case PIM2_SENSOR_34461_VOLT4:
    case PIM2_SENSOR_34461_VOLT5:
    case PIM2_SENSOR_34461_VOLT6:
    case PIM2_SENSOR_34461_VOLT7:
    case PIM2_SENSOR_34461_VOLT8:
    case PIM2_SENSOR_34461_VOLT9:
    case PIM2_SENSOR_34461_VOLT10:
    case PIM2_SENSOR_34461_VOLT11:
    case PIM2_SENSOR_34461_VOLT12:
    case PIM2_SENSOR_34461_VOLT13:
    case PIM2_SENSOR_34461_VOLT14:
    case PIM2_SENSOR_34461_VOLT15:
    case PIM2_SENSOR_34461_VOLT16:
    case PIM3_SENSOR_34461_VOLT1:
    case PIM3_SENSOR_34461_VOLT2:
    case PIM3_SENSOR_34461_VOLT3:
    case PIM3_SENSOR_34461_VOLT4:
    case PIM3_SENSOR_34461_VOLT5:
    case PIM3_SENSOR_34461_VOLT6:
    case PIM3_SENSOR_34461_VOLT7:
    case PIM3_SENSOR_34461_VOLT8:
    case PIM3_SENSOR_34461_VOLT9:
    case PIM3_SENSOR_34461_VOLT10:
    case PIM3_SENSOR_34461_VOLT11:
    case PIM3_SENSOR_34461_VOLT12:
    case PIM3_SENSOR_34461_VOLT13:
    case PIM3_SENSOR_34461_VOLT14:
    case PIM3_SENSOR_34461_VOLT15:
    case PIM3_SENSOR_34461_VOLT16:
    case PIM4_SENSOR_34461_VOLT1:
    case PIM4_SENSOR_34461_VOLT2:
    case PIM4_SENSOR_34461_VOLT3:
    case PIM4_SENSOR_34461_VOLT4:
    case PIM4_SENSOR_34461_VOLT5:
    case PIM4_SENSOR_34461_VOLT6:
    case PIM4_SENSOR_34461_VOLT7:
    case PIM4_SENSOR_34461_VOLT8:
    case PIM4_SENSOR_34461_VOLT9:
    case PIM4_SENSOR_34461_VOLT10:
    case PIM4_SENSOR_34461_VOLT11:
    case PIM4_SENSOR_34461_VOLT12:
    case PIM4_SENSOR_34461_VOLT13:
    case PIM4_SENSOR_34461_VOLT14:
    case PIM4_SENSOR_34461_VOLT15:
    case PIM4_SENSOR_34461_VOLT16:
    case PIM5_SENSOR_34461_VOLT1:
    case PIM5_SENSOR_34461_VOLT2:
    case PIM5_SENSOR_34461_VOLT3:
    case PIM5_SENSOR_34461_VOLT4:
    case PIM5_SENSOR_34461_VOLT5:
    case PIM5_SENSOR_34461_VOLT6:
    case PIM5_SENSOR_34461_VOLT7:
    case PIM5_SENSOR_34461_VOLT8:
    case PIM5_SENSOR_34461_VOLT9:
    case PIM5_SENSOR_34461_VOLT10:
    case PIM5_SENSOR_34461_VOLT11:
    case PIM5_SENSOR_34461_VOLT12:
    case PIM5_SENSOR_34461_VOLT13:
    case PIM5_SENSOR_34461_VOLT14:
    case PIM5_SENSOR_34461_VOLT15:
    case PIM5_SENSOR_34461_VOLT16:
    case PIM6_SENSOR_34461_VOLT1:
    case PIM6_SENSOR_34461_VOLT2:
    case PIM6_SENSOR_34461_VOLT3:
    case PIM6_SENSOR_34461_VOLT4:
    case PIM6_SENSOR_34461_VOLT5:
    case PIM6_SENSOR_34461_VOLT6:
    case PIM6_SENSOR_34461_VOLT7:
    case PIM6_SENSOR_34461_VOLT8:
    case PIM6_SENSOR_34461_VOLT9:
    case PIM6_SENSOR_34461_VOLT10:
    case PIM6_SENSOR_34461_VOLT11:
    case PIM6_SENSOR_34461_VOLT12:
    case PIM6_SENSOR_34461_VOLT13:
    case PIM6_SENSOR_34461_VOLT14:
    case PIM6_SENSOR_34461_VOLT15:
    case PIM6_SENSOR_34461_VOLT16:
    case PIM7_SENSOR_34461_VOLT1:
    case PIM7_SENSOR_34461_VOLT2:
    case PIM7_SENSOR_34461_VOLT3:
    case PIM7_SENSOR_34461_VOLT4:
    case PIM7_SENSOR_34461_VOLT5:
    case PIM7_SENSOR_34461_VOLT6:
    case PIM7_SENSOR_34461_VOLT7:
    case PIM7_SENSOR_34461_VOLT8:
    case PIM7_SENSOR_34461_VOLT9:
    case PIM7_SENSOR_34461_VOLT10:
    case PIM7_SENSOR_34461_VOLT11:
    case PIM7_SENSOR_34461_VOLT12:
    case PIM7_SENSOR_34461_VOLT13:
    case PIM7_SENSOR_34461_VOLT14:
    case PIM7_SENSOR_34461_VOLT15:
    case PIM7_SENSOR_34461_VOLT16:
    case PIM8_SENSOR_34461_VOLT1:
    case PIM8_SENSOR_34461_VOLT2:
    case PIM8_SENSOR_34461_VOLT3:
    case PIM8_SENSOR_34461_VOLT4:
    case PIM8_SENSOR_34461_VOLT5:
    case PIM8_SENSOR_34461_VOLT6:
    case PIM8_SENSOR_34461_VOLT7:
    case PIM8_SENSOR_34461_VOLT8:
    case PIM8_SENSOR_34461_VOLT9:
    case PIM8_SENSOR_34461_VOLT10:
    case PIM8_SENSOR_34461_VOLT11:
    case PIM8_SENSOR_34461_VOLT12:
    case PIM8_SENSOR_34461_VOLT13:
    case PIM8_SENSOR_34461_VOLT14:
    case PIM8_SENSOR_34461_VOLT15:
    case PIM8_SENSOR_34461_VOLT16:
      sprintf(units, "Volts");
      break;
    default:
      return -1;
  }
  return 0;
}

static int
get_psu_sensor_units(uint8_t sensor_num, char *units) {

  switch(sensor_num) {
    case PSU1_SENSOR_IN_VOLT:
    case PSU1_SENSOR_12V_VOLT:
    case PSU1_SENSOR_STBY_VOLT:
    case PSU2_SENSOR_IN_VOLT:
    case PSU2_SENSOR_12V_VOLT:
    case PSU2_SENSOR_STBY_VOLT:
    case PSU3_SENSOR_IN_VOLT:
    case PSU3_SENSOR_12V_VOLT:
    case PSU3_SENSOR_STBY_VOLT:
    case PSU4_SENSOR_IN_VOLT:
    case PSU4_SENSOR_12V_VOLT:
    case PSU4_SENSOR_STBY_VOLT:
      sprintf(units, "Volts");
      break;
    case PSU1_SENSOR_IN_CURR:
    case PSU1_SENSOR_12V_CURR:
    case PSU1_SENSOR_STBY_CURR:
    case PSU2_SENSOR_IN_CURR:
    case PSU2_SENSOR_12V_CURR:
    case PSU2_SENSOR_STBY_CURR:
    case PSU3_SENSOR_IN_CURR:
    case PSU3_SENSOR_12V_CURR:
    case PSU3_SENSOR_STBY_CURR:
    case PSU4_SENSOR_IN_CURR:
    case PSU4_SENSOR_12V_CURR:
    case PSU4_SENSOR_STBY_CURR:
      sprintf(units, "Amps");
      break;
    case PSU1_SENSOR_IN_POWER:
    case PSU1_SENSOR_12V_POWER:
    case PSU1_SENSOR_STBY_POWER:
    case PSU2_SENSOR_IN_POWER:
    case PSU2_SENSOR_12V_POWER:
    case PSU2_SENSOR_STBY_POWER:
    case PSU3_SENSOR_IN_POWER:
    case PSU3_SENSOR_12V_POWER:
    case PSU3_SENSOR_STBY_POWER:
    case PSU4_SENSOR_IN_POWER:
    case PSU4_SENSOR_12V_POWER:
    case PSU4_SENSOR_STBY_POWER:
      sprintf(units, "Watts");
      break;
    case PSU1_SENSOR_FAN_TACH:
    case PSU2_SENSOR_FAN_TACH:
    case PSU3_SENSOR_FAN_TACH:
    case PSU4_SENSOR_FAN_TACH:
      sprintf(units, "RPM");
      break;
    case PSU1_SENSOR_TEMP1:
    case PSU1_SENSOR_TEMP2:
    case PSU1_SENSOR_TEMP3:
    case PSU2_SENSOR_TEMP1:
    case PSU2_SENSOR_TEMP2:
    case PSU2_SENSOR_TEMP3:
    case PSU3_SENSOR_TEMP1:
    case PSU3_SENSOR_TEMP2:
    case PSU3_SENSOR_TEMP3:
    case PSU4_SENSOR_TEMP1:
    case PSU4_SENSOR_TEMP2:
    case PSU4_SENSOR_TEMP3:
      sprintf(units, "C");
      break;
     default:
      return -1;
  }
  return 0;
}

int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  int ret = -1;

  switch(fru) {
    case FRU_SCM:
      ret = get_scm_sensor_units(sensor_num, units);
      break;
    case FRU_SMB:
      ret = get_smb_sensor_units(sensor_num, units);
      break;
    case FRU_PIM1:
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
      ret = get_pim_sensor_units(sensor_num, units);
      break;
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      ret = get_psu_sensor_units(sensor_num, units);
      break;
    default:
      return -1;
  }
  return ret;
}

static void
sensor_thresh_array_init() {

  static bool init_done = false;
  int i = 0;

  if (init_done)
    return;

  /* SCM */
  scm_sensor_threshold[SCM_SENSOR_OUTLET_LOCAL_TEMP][UCR_THRESH] = 70;
  scm_sensor_threshold[SCM_SENSOR_OUTLET_REMOTE_TEMP][UCR_THRESH] = 70;
  scm_sensor_threshold[SCM_SENSOR_INLET_LOCAL_TEMP][UCR_THRESH] = 40;
  scm_sensor_threshold[SCM_SENSOR_INLET_REMOTE_TEMP][UCR_THRESH] = 40;
  scm_sensor_threshold[SCM_SENSOR_HSC_VOLT][UCR_THRESH] = 12.960;
  scm_sensor_threshold[SCM_SENSOR_HSC_VOLT][LCR_THRESH] = 11.040;
  scm_sensor_threshold[SCM_SENSOR_HSC_CURR][UCR_THRESH] = 10;
  scm_sensor_threshold[SCM_SENSOR_HSC_POWER][UCR_THRESH] = 100;
  /* SMB */
  smb_sensor_threshold[SMB_SENSOR_1220_VMON1][UCR_THRESH] = 4.32;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON1][LCR_THRESH] = 3.68;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON2][UCR_THRESH] = 3.564;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON2][LCR_THRESH] = 3.036;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON3][UCR_THRESH] = 5.4;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON3][LCR_THRESH] = 4.6;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON4][UCR_THRESH] = 3.564;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON4][LCR_THRESH] = 3.036;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON5][UCR_THRESH] = 2.7;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON5][LCR_THRESH] = 2.3;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON6][UCR_THRESH] = 1.296;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON6][LCR_THRESH] = 1.104;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON7][UCR_THRESH] = 1.296;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON7][LCR_THRESH] = 1.104;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON8][UCR_THRESH] = 3.564;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON8][LCR_THRESH] = 3.036;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON9][UCR_THRESH] = 0.972;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON9][LCR_THRESH] = 0.828;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON10][UCR_THRESH] = 0.864;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON10][LCR_THRESH] = 0.736;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON11][UCR_THRESH] = 1.944;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON11][LCR_THRESH] = 1.656;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON12][UCR_THRESH] = 1.296;
  smb_sensor_threshold[SMB_SENSOR_1220_VMON12][LCR_THRESH] = 1.104;
  smb_sensor_threshold[SMB_SENSOR_1220_VCCA][UCR_THRESH] = 3.564;
  smb_sensor_threshold[SMB_SENSOR_1220_VCCA][LCR_THRESH] = 3.036;
  smb_sensor_threshold[SMB_SENSOR_1220_VCCINP][UCR_THRESH] = 3.564;
  smb_sensor_threshold[SMB_SENSOR_1220_VCCINP][LCR_THRESH] = 3.036;
  smb_sensor_threshold[SMB_SENSOR_TH3_SERDES_VOLT][UCR_THRESH] = 0.864;
  smb_sensor_threshold[SMB_SENSOR_TH3_SERDES_VOLT][LCR_THRESH] = 0.736;
  smb_sensor_threshold[SMB_SENSOR_TH3_SERDES_CURR][UCR_THRESH] = 300;
  smb_sensor_threshold[SMB_SENSOR_TH3_SERDES_TEMP][UCR_THRESH] = 70;
  smb_sensor_threshold[SMB_SENSOR_TH3_CORE_VOLT][UCR_THRESH] = 0.972;
  smb_sensor_threshold[SMB_SENSOR_TH3_CORE_VOLT][LCR_THRESH] = 0.828;
  smb_sensor_threshold[SMB_SENSOR_TH3_CORE_CURR][UCR_THRESH] = 300;
  smb_sensor_threshold[SMB_SENSOR_TH3_CORE_TEMP][UCR_THRESH] = 70;
  smb_sensor_threshold[SMB_SENSOR_TEMP1][UCR_THRESH] = 70;
  smb_sensor_threshold[SMB_SENSOR_TEMP2][UCR_THRESH] = 70;
  smb_sensor_threshold[SMB_SENSOR_TEMP3][UCR_THRESH] = 70;
  smb_sensor_threshold[SMB_SENSOR_TEMP4][UCR_THRESH] = 70;
  smb_sensor_threshold[SMB_SENSOR_TEMP5][UCR_THRESH] = 70;
  smb_sensor_threshold[SMB_SENSOR_TH3_DIE_TEMP1][UCR_THRESH] = 105;
  smb_sensor_threshold[SMB_SENSOR_TH3_DIE_TEMP2][UCR_THRESH] = 105;
  smb_sensor_threshold[SMB_SENSOR_PDB_L_TEMP1][UCR_THRESH] = 70;
  smb_sensor_threshold[SMB_SENSOR_PDB_L_TEMP2][UCR_THRESH] = 70;
  smb_sensor_threshold[SMB_SENSOR_PDB_R_TEMP1][UCR_THRESH] = 70;
  smb_sensor_threshold[SMB_SENSOR_PDB_R_TEMP2][UCR_THRESH] = 70;
  smb_sensor_threshold[SMB_SENSOR_FCM_T_TEMP1][UCR_THRESH] = 70;
  smb_sensor_threshold[SMB_SENSOR_FCM_T_TEMP2][UCR_THRESH] = 70;
  smb_sensor_threshold[SMB_SENSOR_FCM_B_TEMP1][UCR_THRESH] = 70;
  smb_sensor_threshold[SMB_SENSOR_FCM_B_TEMP2][UCR_THRESH] = 70;
  smb_sensor_threshold[SMB_SENSOR_FCM_T_HSC_VOLT][UCR_THRESH] = 12.960;
  smb_sensor_threshold[SMB_SENSOR_FCM_T_HSC_VOLT][LCR_THRESH] = 11.040;
  smb_sensor_threshold[SMB_SENSOR_FCM_T_HSC_CURR][UCR_THRESH] = 25.59;
  smb_sensor_threshold[SMB_SENSOR_FCM_T_HSC_POWER][UCR_THRESH] = 300;
  smb_sensor_threshold[SMB_SENSOR_FCM_B_HSC_VOLT][UCR_THRESH] = 12.960;
  smb_sensor_threshold[SMB_SENSOR_FCM_B_HSC_VOLT][LCR_THRESH] = 11.040;
  smb_sensor_threshold[SMB_SENSOR_FCM_B_HSC_CURR][UCR_THRESH] = 25.59;
  smb_sensor_threshold[SMB_SENSOR_FCM_B_HSC_POWER][UCR_THRESH] = 300;
  smb_sensor_threshold[SMB_SENSOR_FAN1_FRONT_TACH][UCR_THRESH] = 11500;
  smb_sensor_threshold[SMB_SENSOR_FAN1_FRONT_TACH][UNC_THRESH] = 8500;
  smb_sensor_threshold[SMB_SENSOR_FAN1_FRONT_TACH][LCR_THRESH] = 500;
  smb_sensor_threshold[SMB_SENSOR_FAN1_REAR_TACH][UCR_THRESH] = 11500;
  smb_sensor_threshold[SMB_SENSOR_FAN1_REAR_TACH][UNC_THRESH] = 8500;
  smb_sensor_threshold[SMB_SENSOR_FAN1_REAR_TACH][LCR_THRESH] = 500;
  smb_sensor_threshold[SMB_SENSOR_FAN2_FRONT_TACH][UCR_THRESH] = 11500;
  smb_sensor_threshold[SMB_SENSOR_FAN2_FRONT_TACH][UNC_THRESH] = 8500;
  smb_sensor_threshold[SMB_SENSOR_FAN2_FRONT_TACH][LCR_THRESH] = 500;
  smb_sensor_threshold[SMB_SENSOR_FAN2_REAR_TACH][UCR_THRESH] = 11500;
  smb_sensor_threshold[SMB_SENSOR_FAN2_REAR_TACH][UNC_THRESH] = 8500;
  smb_sensor_threshold[SMB_SENSOR_FAN2_REAR_TACH][LCR_THRESH] = 500;
  smb_sensor_threshold[SMB_SENSOR_FAN3_FRONT_TACH][UCR_THRESH] = 11500;
  smb_sensor_threshold[SMB_SENSOR_FAN3_FRONT_TACH][UNC_THRESH] = 8500;
  smb_sensor_threshold[SMB_SENSOR_FAN3_FRONT_TACH][LCR_THRESH] = 500;
  smb_sensor_threshold[SMB_SENSOR_FAN3_REAR_TACH][UCR_THRESH] = 11500;
  smb_sensor_threshold[SMB_SENSOR_FAN3_REAR_TACH][UNC_THRESH] = 8500;
  smb_sensor_threshold[SMB_SENSOR_FAN3_REAR_TACH][LCR_THRESH] = 500;
  smb_sensor_threshold[SMB_SENSOR_FAN4_FRONT_TACH][UCR_THRESH] = 11500;
  smb_sensor_threshold[SMB_SENSOR_FAN4_FRONT_TACH][UNC_THRESH] = 8500;
  smb_sensor_threshold[SMB_SENSOR_FAN4_FRONT_TACH][LCR_THRESH] = 500;
  smb_sensor_threshold[SMB_SENSOR_FAN4_REAR_TACH][UCR_THRESH] = 11500;
  smb_sensor_threshold[SMB_SENSOR_FAN4_REAR_TACH][UNC_THRESH] = 8500;
  smb_sensor_threshold[SMB_SENSOR_FAN4_REAR_TACH][LCR_THRESH] = 500;
  smb_sensor_threshold[SMB_SENSOR_FAN5_FRONT_TACH][UCR_THRESH] = 11500;
  smb_sensor_threshold[SMB_SENSOR_FAN5_FRONT_TACH][UNC_THRESH] = 8500;
  smb_sensor_threshold[SMB_SENSOR_FAN5_FRONT_TACH][LCR_THRESH] = 500;
  smb_sensor_threshold[SMB_SENSOR_FAN5_REAR_TACH][UCR_THRESH] = 11500;
  smb_sensor_threshold[SMB_SENSOR_FAN5_REAR_TACH][UNC_THRESH] = 8500;
  smb_sensor_threshold[SMB_SENSOR_FAN5_REAR_TACH][LCR_THRESH] = 500;
  smb_sensor_threshold[SMB_SENSOR_FAN6_FRONT_TACH][UCR_THRESH] = 11500;
  smb_sensor_threshold[SMB_SENSOR_FAN6_FRONT_TACH][UNC_THRESH] = 8500;
  smb_sensor_threshold[SMB_SENSOR_FAN6_FRONT_TACH][LCR_THRESH] = 500;
  smb_sensor_threshold[SMB_SENSOR_FAN6_REAR_TACH][UCR_THRESH] = 11500;
  smb_sensor_threshold[SMB_SENSOR_FAN6_REAR_TACH][UNC_THRESH] = 8500;
  smb_sensor_threshold[SMB_SENSOR_FAN6_REAR_TACH][LCR_THRESH] = 500;
  smb_sensor_threshold[SMB_SENSOR_FAN7_FRONT_TACH][UCR_THRESH] = 11500;
  smb_sensor_threshold[SMB_SENSOR_FAN7_FRONT_TACH][UNC_THRESH] = 8500;
  smb_sensor_threshold[SMB_SENSOR_FAN7_FRONT_TACH][LCR_THRESH] = 500;
  smb_sensor_threshold[SMB_SENSOR_FAN7_REAR_TACH][UCR_THRESH] = 11500;
  smb_sensor_threshold[SMB_SENSOR_FAN7_REAR_TACH][UNC_THRESH] = 8500;
  smb_sensor_threshold[SMB_SENSOR_FAN7_REAR_TACH][LCR_THRESH] = 500;
  smb_sensor_threshold[SMB_SENSOR_FAN8_FRONT_TACH][UCR_THRESH] = 11500;
  smb_sensor_threshold[SMB_SENSOR_FAN8_FRONT_TACH][UNC_THRESH] = 8500;
  smb_sensor_threshold[SMB_SENSOR_FAN8_FRONT_TACH][LCR_THRESH] = 500;
  smb_sensor_threshold[SMB_SENSOR_FAN8_REAR_TACH][UCR_THRESH] = 11500;
  smb_sensor_threshold[SMB_SENSOR_FAN8_REAR_TACH][UNC_THRESH] = 8500;
  smb_sensor_threshold[SMB_SENSOR_FAN8_REAR_TACH][LCR_THRESH] = 500;
  /* PIM */
  for (i = 0; i < 8; i++) {
    pim_sensor_threshold[PIM1_SENSOR_TEMP1+(i*0x15)][UCR_THRESH] = 70;
    pim_sensor_threshold[PIM1_SENSOR_TEMP2+(i*0x15)][UCR_THRESH] = 70;
    pim_sensor_threshold[PIM1_SENSOR_HSC_VOLT+(i*0x15)][UCR_THRESH] = 12.960;
    pim_sensor_threshold[PIM1_SENSOR_HSC_VOLT+(i*0x15)][LCR_THRESH] = 11.040;
    pim_sensor_threshold[PIM1_SENSOR_HSC_CURR+(i*0x15)][UCR_THRESH] = 25.59;
    pim_sensor_threshold[PIM1_SENSOR_HSC_POWER+(i*0x15)][UCR_THRESH] = 200;

    if (pal_get_pim_type((i+3)) == 0) {
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT1+(i*0x15)][UCR_THRESH] = 0.864;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT1+(i*0x15)][LCR_THRESH] = 0.662;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT2+(i*0x15)][UCR_THRESH] = 0.864;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT2+(i*0x15)][LCR_THRESH] = 0.662;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT3+(i*0x15)][UCR_THRESH] = 0.864;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT3+(i*0x15)][LCR_THRESH] = 0.662;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT4+(i*0x15)][UCR_THRESH] = 1.944;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT4+(i*0x15)][LCR_THRESH] = 1.656;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT5+(i*0x15)][UCR_THRESH] = 0.864;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT5+(i*0x15)][LCR_THRESH] = 0.662;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT6+(i*0x15)][UCR_THRESH] = 0.864;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT6+(i*0x15)][LCR_THRESH] = 0.662;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT7+(i*0x15)][UCR_THRESH] = 0.864;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT7+(i*0x15)][LCR_THRESH] = 0.662;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT8+(i*0x15)][UCR_THRESH] = 1.944;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT8+(i*0x15)][LCR_THRESH] = 1.656;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT9+(i*0x15)][UCR_THRESH] = 0.864;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT9+(i*0x15)][LCR_THRESH] = 0.662;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT10+(i*0x15)][UCR_THRESH] = 0.864;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT10+(i*0x15)][LCR_THRESH] = 0.662;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT11+(i*0x15)][UCR_THRESH] = 0.864;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT11+(i*0x15)][LCR_THRESH] = 0.662;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT12+(i*0x15)][UCR_THRESH] = 1.944;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT12+(i*0x15)][LCR_THRESH] = 1.656;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT13+(i*0x15)][UCR_THRESH] = 0.864;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT13+(i*0x15)][LCR_THRESH] = 0.662;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT14+(i*0x15)][UCR_THRESH] = 0.864;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT14+(i*0x15)][LCR_THRESH] = 0.662;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT15+(i*0x15)][UCR_THRESH] = 0.864;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT15+(i*0x15)][LCR_THRESH] = 0.662;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT16+(i*0x15)][UCR_THRESH] = 1.944;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT16+(i*0x15)][LCR_THRESH] = 1.656;
    } else if (pal_get_pim_type((i+3)) == 1) {
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT1+(i*0x15)][UCR_THRESH] = 0.864;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT1+(i*0x15)][LCR_THRESH] = 0.662;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT2+(i*0x15)][UCR_THRESH] = 0.864;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT2+(i*0x15)][LCR_THRESH] = 0.662;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT3+(i*0x15)][UCR_THRESH] = 1.944;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT3+(i*0x15)][LCR_THRESH] = 1.656;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT5+(i*0x15)][UCR_THRESH] = 0.864;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT5+(i*0x15)][LCR_THRESH] = 0.662;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT6+(i*0x15)][UCR_THRESH] = 0.864;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT6+(i*0x15)][LCR_THRESH] = 0.662;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT7+(i*0x15)][UCR_THRESH] = 1.944;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT7+(i*0x15)][LCR_THRESH] = 1.656;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT9+(i*0x15)][UCR_THRESH] = 0.864;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT9+(i*0x15)][LCR_THRESH] = 0.662;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT10+(i*0x15)][UCR_THRESH] = 0.864;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT10+(i*0x15)][LCR_THRESH] = 0.662;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT11+(i*0x15)][UCR_THRESH] = 1.944;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT11+(i*0x15)][LCR_THRESH] = 1.656;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT13+(i*0x15)][UCR_THRESH] = 0.864;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT13+(i*0x15)][LCR_THRESH] = 0.662;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT14+(i*0x15)][UCR_THRESH] = 0.864;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT14+(i*0x15)][LCR_THRESH] = 0.662;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT15+(i*0x15)][UCR_THRESH] = 1.944;
      pim_sensor_threshold[PIM1_SENSOR_34461_VOLT15+(i*0x15)][LCR_THRESH] = 1.656;
    }
  }
  /* PSU */
  for (i = 0; i < 4; i++) {
    psu_sensor_threshold[PSU1_SENSOR_IN_VOLT+(i*0x0d)][UCR_THRESH] = 259.2;
    psu_sensor_threshold[PSU1_SENSOR_IN_VOLT+(i*0x0d)][LCR_THRESH] = 92;
    psu_sensor_threshold[PSU1_SENSOR_12V_VOLT+(i*0x0d)][UCR_THRESH] = 12.960;
    psu_sensor_threshold[PSU1_SENSOR_12V_VOLT+(i*0x0d)][LCR_THRESH] = 11.040;
    psu_sensor_threshold[PSU1_SENSOR_STBY_VOLT+(i*0x0d)][UCR_THRESH] = 3.564;
    psu_sensor_threshold[PSU1_SENSOR_STBY_VOLT+(i*0x0d)][LCR_THRESH] = 3.036;
    psu_sensor_threshold[PSU1_SENSOR_IN_CURR+(i*0x0d)][UCR_THRESH] = 10;
    psu_sensor_threshold[PSU1_SENSOR_12V_CURR+(i*0x0d)][UCR_THRESH] = 125;
    psu_sensor_threshold[PSU1_SENSOR_STBY_CURR+(i*0x0d)][UCR_THRESH] = 5;
    psu_sensor_threshold[PSU1_SENSOR_IN_POWER+(i*0x0d)][UCR_THRESH] = 1500;
    psu_sensor_threshold[PSU1_SENSOR_12V_POWER+(i*0x0d)][UCR_THRESH] = 1500;
    psu_sensor_threshold[PSU1_SENSOR_STBY_POWER+(i*0x0d)][UCR_THRESH] = 15;
    psu_sensor_threshold[PSU1_SENSOR_FAN_TACH+(i*0x0d)][UCR_THRESH] = 11500;
    psu_sensor_threshold[PSU1_SENSOR_FAN_TACH+(i*0x0d)][UNC_THRESH] = 8500;
    psu_sensor_threshold[PSU1_SENSOR_FAN_TACH+(i*0x0d)][LCR_THRESH] = 500;
    psu_sensor_threshold[PSU1_SENSOR_TEMP1+(i*0x0d)][UCR_THRESH] = 70;
    psu_sensor_threshold[PSU1_SENSOR_TEMP2+(i*0x0d)][UCR_THRESH] = 70;
    psu_sensor_threshold[PSU1_SENSOR_TEMP3+(i*0x0d)][UCR_THRESH] = 70;
  }

  init_done = true;
}

int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num,
                                      uint8_t thresh, void *value) {
  float *val = (float*) value;

  sensor_thresh_array_init();

  switch(fru) {
  case FRU_SCM:
    *val = scm_sensor_threshold[sensor_num][thresh];
    break;
  case FRU_SMB:
    *val = smb_sensor_threshold[sensor_num][thresh];
    break;
  case FRU_PIM1:
  case FRU_PIM2:
  case FRU_PIM3:
  case FRU_PIM4:
  case FRU_PIM5:
  case FRU_PIM6:
  case FRU_PIM7:
  case FRU_PIM8:
    *val = pim_sensor_threshold[sensor_num][thresh];
    break;
  case FRU_PSU1:
  case FRU_PSU2:
  case FRU_PSU3:
  case FRU_PSU4:
    *val = psu_sensor_threshold[sensor_num][thresh];
    break;
  default:
    return -1;
  }
  return 0;
}

bool
pal_is_fw_update_ongoing(uint8_t fru) {

  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN] = {0};
  int ret;
  struct timespec ts;

  switch (fru) {
    case FRU_SCM:
      sprintf(key, "slot%d_fwupd", IPMB_BUS);
      break;
    default:
      return false;
  }

  ret = kv_get(key, value, NULL, 0);
  if (ret < 0) {
     return false;
  }

  clock_gettime(CLOCK_MONOTONIC, &ts);
  if (strtoul(value, NULL, 10) > ts.tv_sec)
     return true;

  return false;
}

int
pal_get_fw_info(uint8_t fru, unsigned char target,
                unsigned char* res, unsigned char* res_len) {
  return -1;
}

static sensor_desc_t *
get_sensor_desc(uint8_t fru, uint8_t snr_num) {

  if (fru < 1 || fru > MAX_NUM_FRUS) {
    syslog(LOG_WARNING, "get_sensor_desc: Wrong FRU ID %d\n", fru);
    return NULL;
  }

  return &m_snr_desc[fru-1][snr_num];
}

void
pal_sensor_assert_handle(uint8_t fru, uint8_t snr_num,
                                      float val, uint8_t thresh) {
  char crisel[128];
  char thresh_name[10];
  sensor_desc_t *snr_desc;

  switch (thresh) {
    case UNR_THRESH:
        sprintf(thresh_name, "UNR");
      break;
    case UCR_THRESH:
        sprintf(thresh_name, "UCR");
      break;
    case UNC_THRESH:
        sprintf(thresh_name, "UNCR");
      break;
    case LNR_THRESH:
        sprintf(thresh_name, "LNR");
      break;
    case LCR_THRESH:
        sprintf(thresh_name, "LCR");
      break;
    case LNC_THRESH:
        sprintf(thresh_name, "LNCR");
      break;
    default:
      syslog(LOG_WARNING, "pal_sensor_assert_handle: wrong thresh enum value");
      exit(-1);
  }

  switch(snr_num) {
    case BIC_SENSOR_P3V3_MB:
    case BIC_SENSOR_P12V_MB:
    case BIC_SENSOR_P1V05_PCH:
    case BIC_SENSOR_P3V3_STBY_MB:
    case BIC_SENSOR_PV_BAT:
    case BIC_SENSOR_VCCIN_VR_VOL:
    case BIC_SENSOR_INA230_VOL:
      snr_desc = get_sensor_desc(fru, snr_num);
      sprintf(crisel, "%s %s %.2fV - ASSERT,FRU:%u",
                      snr_desc->name, thresh_name, val, fru);
      break;
    default:
      return;
  }
  pal_add_cri_sel(crisel);
  return;
}

void
pal_sensor_deassert_handle(uint8_t fru, uint8_t snr_num,
                                        float val, uint8_t thresh) {
  char crisel[128];
  char thresh_name[8];
  sensor_desc_t *snr_desc;

  switch (thresh) {
    case UNR_THRESH:
      sprintf(thresh_name, "UNR");
      break;
    case UCR_THRESH:
      sprintf(thresh_name, "UCR");
      break;
    case UNC_THRESH:
      sprintf(thresh_name, "UNCR");
      break;
    case LNR_THRESH:
      sprintf(thresh_name, "LNR");
      break;
    case LCR_THRESH:
      sprintf(thresh_name, "LCR");
      break;
    case LNC_THRESH:
      sprintf(thresh_name, "LNCR");
      break;
    default:
      syslog(LOG_WARNING,
             "pal_sensor_deassert_handle: wrong thresh enum value");
      return;
  }
  switch (fru) {
    case FRU_SCM:
      switch (snr_num) {
        case BIC_SENSOR_SOC_TEMP:
          sprintf(crisel, "SOC Temp %s %.0fC - DEASSERT,FRU:%u",
                          thresh_name, val, fru);
          break;
        case BIC_SENSOR_P3V3_MB:
        case BIC_SENSOR_P12V_MB:
        case BIC_SENSOR_P1V05_PCH:
        case BIC_SENSOR_P3V3_STBY_MB:
        case BIC_SENSOR_PV_BAT:
        case BIC_SENSOR_VCCIN_VR_VOL:
        case BIC_SENSOR_INA230_VOL:
          snr_desc = get_sensor_desc(FRU_SCM, snr_num);
          sprintf(crisel, "%s %s %.2fV - DEASSERT,FRU:%u",
                          snr_desc->name, thresh_name, val, fru);
          break;
        default:
          return;
      }
      break;
  }
  pal_add_cri_sel(crisel);
  return;
}

int
pal_sensor_threshold_flag(uint8_t fru, uint8_t snr_num, uint16_t *flag) {

  switch(fru) {
    case FRU_SCM:
      if (snr_num == BIC_SENSOR_SOC_THERM_MARGIN)
        *flag = GETMASK(SENSOR_VALID) | GETMASK(UCR_THRESH);
      else if (snr_num == BIC_SENSOR_SOC_PACKAGE_PWR)
        *flag = GETMASK(SENSOR_VALID);
      else if (snr_num == BIC_SENSOR_SOC_TJMAX)
        *flag = GETMASK(SENSOR_VALID);
      break;
    default:
      break;
  }
  return 0;
}

static void
scm_sensor_poll_interval(uint8_t sensor_num, uint8_t *value) {

  switch(sensor_num) {
    case SCM_SENSOR_OUTLET_LOCAL_TEMP:
    case SCM_SENSOR_OUTLET_REMOTE_TEMP:
    case SCM_SENSOR_INLET_LOCAL_TEMP:
      *value = 30;
      break;
    case SCM_SENSOR_INLET_REMOTE_TEMP:
      *value = 2;
      break;
    case SCM_SENSOR_HSC_VOLT:
    case SCM_SENSOR_HSC_CURR:
    case SCM_SENSOR_HSC_POWER:
      *value = 30;
      break;
    default:
      *value = 10;
      break;
  }
}

static void
smb_sensor_poll_interval(uint8_t sensor_num, uint8_t *value) {

  switch(sensor_num) {
    case SMB_SENSOR_TH3_SERDES_TEMP:
    case SMB_SENSOR_TH3_CORE_TEMP:
    case SMB_SENSOR_TEMP1:
    case SMB_SENSOR_TEMP2:
    case SMB_SENSOR_TEMP3:
    case SMB_SENSOR_TEMP4:
    case SMB_SENSOR_TEMP5:
    case SMB_SENSOR_PDB_L_TEMP1:
    case SMB_SENSOR_PDB_L_TEMP2:
    case SMB_SENSOR_PDB_R_TEMP1:
    case SMB_SENSOR_PDB_R_TEMP2:
    case SMB_SENSOR_FCM_T_TEMP1:
    case SMB_SENSOR_FCM_T_TEMP2:
    case SMB_SENSOR_FCM_B_TEMP1:
    case SMB_SENSOR_FCM_B_TEMP2:
      *value = 30;
      break;
    case SMB_SENSOR_TH3_DIE_TEMP1:
    case SMB_SENSOR_TH3_DIE_TEMP2:
      *value = 2;
      break;
    case SMB_SENSOR_1220_VMON1:
    case SMB_SENSOR_1220_VMON2:
    case SMB_SENSOR_1220_VMON3:
    case SMB_SENSOR_1220_VMON4:
    case SMB_SENSOR_1220_VMON5:
    case SMB_SENSOR_1220_VMON6:
    case SMB_SENSOR_1220_VMON7:
    case SMB_SENSOR_1220_VMON8:
    case SMB_SENSOR_1220_VMON9:
    case SMB_SENSOR_1220_VMON10:
    case SMB_SENSOR_1220_VMON11:
    case SMB_SENSOR_1220_VMON12:
    case SMB_SENSOR_1220_VCCA:
    case SMB_SENSOR_1220_VCCINP:
    case SMB_SENSOR_TH3_SERDES_VOLT:
    case SMB_SENSOR_TH3_CORE_VOLT:
    case SMB_SENSOR_FCM_T_HSC_VOLT:
    case SMB_SENSOR_FCM_B_HSC_VOLT:
    case SMB_SENSOR_TH3_SERDES_CURR:
    case SMB_SENSOR_TH3_CORE_CURR:
    case SMB_SENSOR_FCM_T_HSC_CURR:
    case SMB_SENSOR_FCM_B_HSC_CURR:
    case SMB_SENSOR_FCM_T_HSC_POWER:
    case SMB_SENSOR_FCM_B_HSC_POWER:
      *value = 30;
      break;
    case SMB_SENSOR_FAN1_FRONT_TACH:
    case SMB_SENSOR_FAN1_REAR_TACH:
    case SMB_SENSOR_FAN2_FRONT_TACH:
    case SMB_SENSOR_FAN2_REAR_TACH:
    case SMB_SENSOR_FAN3_FRONT_TACH:
    case SMB_SENSOR_FAN3_REAR_TACH:
    case SMB_SENSOR_FAN4_FRONT_TACH:
    case SMB_SENSOR_FAN4_REAR_TACH:
    case SMB_SENSOR_FAN5_FRONT_TACH:
    case SMB_SENSOR_FAN5_REAR_TACH:
    case SMB_SENSOR_FAN6_FRONT_TACH:
    case SMB_SENSOR_FAN6_REAR_TACH:
    case SMB_SENSOR_FAN7_FRONT_TACH:
    case SMB_SENSOR_FAN7_REAR_TACH:
    case SMB_SENSOR_FAN8_FRONT_TACH:
    case SMB_SENSOR_FAN8_REAR_TACH:
      *value = 2;
      break;
    default:
      *value = 10;
      break;
  }
}

static void
pim_sensor_poll_interval(uint8_t sensor_num, uint8_t *value) {

  switch(sensor_num) {
    case PIM1_SENSOR_TEMP1:
    case PIM1_SENSOR_TEMP2:
    case PIM1_SENSOR_HSC_VOLT:
    case PIM1_SENSOR_HSC_CURR:
    case PIM1_SENSOR_HSC_POWER:
      *value = 30;
      break;
    case PIM1_SENSOR_34461_VOLT1:
    case PIM1_SENSOR_34461_VOLT2:
    case PIM1_SENSOR_34461_VOLT3:
    case PIM1_SENSOR_34461_VOLT4:
    case PIM1_SENSOR_34461_VOLT5:
    case PIM1_SENSOR_34461_VOLT6:
    case PIM1_SENSOR_34461_VOLT7:
    case PIM1_SENSOR_34461_VOLT8:
    case PIM1_SENSOR_34461_VOLT9:
    case PIM1_SENSOR_34461_VOLT10:
    case PIM1_SENSOR_34461_VOLT11:
    case PIM1_SENSOR_34461_VOLT12:
    case PIM1_SENSOR_34461_VOLT13:
    case PIM1_SENSOR_34461_VOLT14:
    case PIM1_SENSOR_34461_VOLT15:
    case PIM1_SENSOR_34461_VOLT16:
      *value = 60;
      break;
    case PIM2_SENSOR_TEMP1:
    case PIM2_SENSOR_TEMP2:
    case PIM2_SENSOR_HSC_VOLT:
    case PIM2_SENSOR_HSC_CURR:
    case PIM2_SENSOR_HSC_POWER:
      *value = 30;
      break;
    case PIM2_SENSOR_34461_VOLT1:
    case PIM2_SENSOR_34461_VOLT2:
    case PIM2_SENSOR_34461_VOLT3:
    case PIM2_SENSOR_34461_VOLT4:
    case PIM2_SENSOR_34461_VOLT5:
    case PIM2_SENSOR_34461_VOLT6:
    case PIM2_SENSOR_34461_VOLT7:
    case PIM2_SENSOR_34461_VOLT8:
    case PIM2_SENSOR_34461_VOLT9:
    case PIM2_SENSOR_34461_VOLT10:
    case PIM2_SENSOR_34461_VOLT11:
    case PIM2_SENSOR_34461_VOLT12:
    case PIM2_SENSOR_34461_VOLT13:
    case PIM2_SENSOR_34461_VOLT14:
    case PIM2_SENSOR_34461_VOLT15:
    case PIM2_SENSOR_34461_VOLT16:
      *value = 60;
      break;
    case PIM3_SENSOR_TEMP1:
    case PIM3_SENSOR_TEMP2:
    case PIM3_SENSOR_HSC_VOLT:
    case PIM3_SENSOR_HSC_CURR:
    case PIM3_SENSOR_HSC_POWER:
      *value = 30;
      break;
    case PIM3_SENSOR_34461_VOLT1:
    case PIM3_SENSOR_34461_VOLT2:
    case PIM3_SENSOR_34461_VOLT3:
    case PIM3_SENSOR_34461_VOLT4:
    case PIM3_SENSOR_34461_VOLT5:
    case PIM3_SENSOR_34461_VOLT6:
    case PIM3_SENSOR_34461_VOLT7:
    case PIM3_SENSOR_34461_VOLT8:
    case PIM3_SENSOR_34461_VOLT9:
    case PIM3_SENSOR_34461_VOLT10:
    case PIM3_SENSOR_34461_VOLT11:
    case PIM3_SENSOR_34461_VOLT12:
    case PIM3_SENSOR_34461_VOLT13:
    case PIM3_SENSOR_34461_VOLT14:
    case PIM3_SENSOR_34461_VOLT15:
    case PIM3_SENSOR_34461_VOLT16:
      *value = 60;
      break;
    case PIM4_SENSOR_TEMP1:
    case PIM4_SENSOR_TEMP2:
    case PIM4_SENSOR_HSC_VOLT:
    case PIM4_SENSOR_HSC_CURR:
    case PIM4_SENSOR_HSC_POWER:
      *value = 30;
      break;
    case PIM4_SENSOR_34461_VOLT1:
    case PIM4_SENSOR_34461_VOLT2:
    case PIM4_SENSOR_34461_VOLT3:
    case PIM4_SENSOR_34461_VOLT4:
    case PIM4_SENSOR_34461_VOLT5:
    case PIM4_SENSOR_34461_VOLT6:
    case PIM4_SENSOR_34461_VOLT7:
    case PIM4_SENSOR_34461_VOLT8:
    case PIM4_SENSOR_34461_VOLT9:
    case PIM4_SENSOR_34461_VOLT10:
    case PIM4_SENSOR_34461_VOLT11:
    case PIM4_SENSOR_34461_VOLT12:
    case PIM4_SENSOR_34461_VOLT13:
    case PIM4_SENSOR_34461_VOLT14:
    case PIM4_SENSOR_34461_VOLT15:
    case PIM4_SENSOR_34461_VOLT16:
      *value = 60;
      break;
    case PIM5_SENSOR_TEMP1:
    case PIM5_SENSOR_TEMP2:
    case PIM5_SENSOR_HSC_VOLT:
    case PIM5_SENSOR_HSC_CURR:
    case PIM5_SENSOR_HSC_POWER:
      *value = 30;
      break;
    case PIM5_SENSOR_34461_VOLT1:
    case PIM5_SENSOR_34461_VOLT2:
    case PIM5_SENSOR_34461_VOLT3:
    case PIM5_SENSOR_34461_VOLT4:
    case PIM5_SENSOR_34461_VOLT5:
    case PIM5_SENSOR_34461_VOLT6:
    case PIM5_SENSOR_34461_VOLT7:
    case PIM5_SENSOR_34461_VOLT8:
    case PIM5_SENSOR_34461_VOLT9:
    case PIM5_SENSOR_34461_VOLT10:
    case PIM5_SENSOR_34461_VOLT11:
    case PIM5_SENSOR_34461_VOLT12:
    case PIM5_SENSOR_34461_VOLT13:
    case PIM5_SENSOR_34461_VOLT14:
    case PIM5_SENSOR_34461_VOLT15:
    case PIM5_SENSOR_34461_VOLT16:
      *value = 60;
      break;
    case PIM6_SENSOR_TEMP1:
    case PIM6_SENSOR_TEMP2:
    case PIM6_SENSOR_HSC_VOLT:
    case PIM6_SENSOR_HSC_CURR:
    case PIM6_SENSOR_HSC_POWER:
      *value = 30;
      break;
    case PIM6_SENSOR_34461_VOLT1:
    case PIM6_SENSOR_34461_VOLT2:
    case PIM6_SENSOR_34461_VOLT3:
    case PIM6_SENSOR_34461_VOLT4:
    case PIM6_SENSOR_34461_VOLT5:
    case PIM6_SENSOR_34461_VOLT6:
    case PIM6_SENSOR_34461_VOLT7:
    case PIM6_SENSOR_34461_VOLT8:
    case PIM6_SENSOR_34461_VOLT9:
    case PIM6_SENSOR_34461_VOLT10:
    case PIM6_SENSOR_34461_VOLT11:
    case PIM6_SENSOR_34461_VOLT12:
    case PIM6_SENSOR_34461_VOLT13:
    case PIM6_SENSOR_34461_VOLT14:
    case PIM6_SENSOR_34461_VOLT15:
    case PIM6_SENSOR_34461_VOLT16:
      *value = 60;
      break;
    case PIM7_SENSOR_TEMP1:
    case PIM7_SENSOR_TEMP2:
    case PIM7_SENSOR_HSC_VOLT:
    case PIM7_SENSOR_HSC_CURR:
    case PIM7_SENSOR_HSC_POWER:
      *value = 30;
      break;
    case PIM7_SENSOR_34461_VOLT1:
    case PIM7_SENSOR_34461_VOLT2:
    case PIM7_SENSOR_34461_VOLT3:
    case PIM7_SENSOR_34461_VOLT4:
    case PIM7_SENSOR_34461_VOLT5:
    case PIM7_SENSOR_34461_VOLT6:
    case PIM7_SENSOR_34461_VOLT7:
    case PIM7_SENSOR_34461_VOLT8:
    case PIM7_SENSOR_34461_VOLT9:
    case PIM7_SENSOR_34461_VOLT10:
    case PIM7_SENSOR_34461_VOLT11:
    case PIM7_SENSOR_34461_VOLT12:
    case PIM7_SENSOR_34461_VOLT13:
    case PIM7_SENSOR_34461_VOLT14:
    case PIM7_SENSOR_34461_VOLT15:
    case PIM7_SENSOR_34461_VOLT16:
      *value = 60;
      break;
    case PIM8_SENSOR_TEMP1:
    case PIM8_SENSOR_TEMP2:
    case PIM8_SENSOR_HSC_VOLT:
    case PIM8_SENSOR_HSC_CURR:
    case PIM8_SENSOR_HSC_POWER:
      *value = 30;
      break;
    case PIM8_SENSOR_34461_VOLT1:
    case PIM8_SENSOR_34461_VOLT2:
    case PIM8_SENSOR_34461_VOLT3:
    case PIM8_SENSOR_34461_VOLT4:
    case PIM8_SENSOR_34461_VOLT5:
    case PIM8_SENSOR_34461_VOLT6:
    case PIM8_SENSOR_34461_VOLT7:
    case PIM8_SENSOR_34461_VOLT8:
    case PIM8_SENSOR_34461_VOLT9:
    case PIM8_SENSOR_34461_VOLT10:
    case PIM8_SENSOR_34461_VOLT11:
    case PIM8_SENSOR_34461_VOLT12:
    case PIM8_SENSOR_34461_VOLT13:
    case PIM8_SENSOR_34461_VOLT14:
    case PIM8_SENSOR_34461_VOLT15:
    case PIM8_SENSOR_34461_VOLT16:
      *value = 60;
      break;
    default:
      *value = 10;
      break;
  }
}

static void
psu_sensor_poll_interval(uint8_t sensor_num, uint8_t *value) {

  switch(sensor_num) {
    case PSU1_SENSOR_IN_VOLT:
    case PSU1_SENSOR_12V_VOLT:
    case PSU1_SENSOR_STBY_VOLT:
    case PSU1_SENSOR_IN_CURR:
    case PSU1_SENSOR_12V_CURR:
    case PSU1_SENSOR_STBY_CURR:
    case PSU1_SENSOR_IN_POWER:
    case PSU1_SENSOR_12V_POWER:
    case PSU1_SENSOR_STBY_POWER:
    case PSU1_SENSOR_FAN_TACH:
    case PSU1_SENSOR_TEMP1:
    case PSU1_SENSOR_TEMP2:
    case PSU1_SENSOR_TEMP3:
    case PSU2_SENSOR_IN_VOLT:
    case PSU2_SENSOR_12V_VOLT:
    case PSU2_SENSOR_STBY_VOLT:
    case PSU2_SENSOR_IN_CURR:
    case PSU2_SENSOR_12V_CURR:
    case PSU2_SENSOR_STBY_CURR:
    case PSU2_SENSOR_IN_POWER:
    case PSU2_SENSOR_12V_POWER:
    case PSU2_SENSOR_STBY_POWER:
    case PSU2_SENSOR_FAN_TACH:
    case PSU2_SENSOR_TEMP1:
    case PSU2_SENSOR_TEMP2:
    case PSU2_SENSOR_TEMP3:
    case PSU3_SENSOR_IN_VOLT:
    case PSU3_SENSOR_12V_VOLT:
    case PSU3_SENSOR_STBY_VOLT:
    case PSU3_SENSOR_IN_CURR:
    case PSU3_SENSOR_12V_CURR:
    case PSU3_SENSOR_STBY_CURR:
    case PSU3_SENSOR_IN_POWER:
    case PSU3_SENSOR_12V_POWER:
    case PSU3_SENSOR_STBY_POWER:
    case PSU3_SENSOR_FAN_TACH:
    case PSU3_SENSOR_TEMP1:
    case PSU3_SENSOR_TEMP2:
    case PSU3_SENSOR_TEMP3:
    case PSU4_SENSOR_IN_VOLT:
    case PSU4_SENSOR_12V_VOLT:
    case PSU4_SENSOR_STBY_VOLT:
    case PSU4_SENSOR_IN_CURR:
    case PSU4_SENSOR_12V_CURR:
    case PSU4_SENSOR_STBY_CURR:
    case PSU4_SENSOR_IN_POWER:
    case PSU4_SENSOR_12V_POWER:
    case PSU4_SENSOR_STBY_POWER:
    case PSU4_SENSOR_FAN_TACH:
    case PSU4_SENSOR_TEMP1:
    case PSU4_SENSOR_TEMP2:
    case PSU4_SENSOR_TEMP3:
      *value = 30;
      break;
    default:
      *value = 10;
      break;
  }
}

int
pal_get_sensor_poll_interval(uint8_t fru, uint8_t sensor_num, uint8_t *value) {

  switch(fru) {
    case FRU_SCM:
      scm_sensor_poll_interval(sensor_num, value);
      break;
    case FRU_SMB:
      smb_sensor_poll_interval(sensor_num, value);
      break;
    case FRU_PIM1:
    case FRU_PIM2:
    case FRU_PIM3:
    case FRU_PIM4:
    case FRU_PIM5:
    case FRU_PIM6:
    case FRU_PIM7:
    case FRU_PIM8:
      pim_sensor_poll_interval(sensor_num, value);
      break;
    case FRU_PSU1:
    case FRU_PSU2:
    case FRU_PSU3:
    case FRU_PSU4:
      psu_sensor_poll_interval(sensor_num, value);
      break;
    default:
      *value = 2;
      break;
  }
  return 0;
}
