/******************************************************************************
 * @file           : nrf_nus.h
 * @brief          : Nordic UART Service definitions
 ******************************************************************************
 * @attention
 *
 * 
 * Author:    Bor Buh
 * All rights reserved.</center></h2>
 *
 ******************************************************************************
 */

#ifndef __NRF_NUS_H__
#define __NRF_NUS_H__

#include <BLEDevice.h>
#include "Arduino.h"

// The remote service we wish to connect to.
static BLEUUID NRF_NUS("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
// The characteristic of the remote service we are interested in.
static BLEUUID NRF_NUS_CCCD("2902");
static BLEUUID NRF_NUS_RX_CHAR("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
static BLEUUID NRF_NUS_TX_CHAR("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

typedef struct
{
  BLERemoteCharacteristic *pNusRXCharacteristic;
  BLERemoteCharacteristic *pNusTXCharacteristic;
  uint8_t nextCommand;
  uint32_t last_change_check;
  uint32_t change_check_time; // speed of sending next command

} nus_t;

typedef struct
{
  uint8_t addr[6];        // MAC address of the target
  uint32_t timeout;       // timeout untill command is dropped
  uint32_t timeReceived;  // Time when command was received
  char cmd[50];           // string command to send
  uint8_t status;         // indikator ali je bilo poslano
} blec_send_target;

uint8_t nus_transfer_cmd(blec_send_target send_target, nus_t nus_instance);
uint8_t nus_manage_flags();
void cvs_ble_nus_process(nus_t *hndl);

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#endif /* __NRF_NUS_H__ */