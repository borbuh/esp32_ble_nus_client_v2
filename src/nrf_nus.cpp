/*
    cvs_nrf_nus.cpp
    Author: bor.buh



*/
#include "Arduino.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "nrf_nus.h"

/**
 * Function to process
 */
void cvs_ble_nus_process(nus_t *hndl)
{
    uint32_t curr = millis();
    if ((curr - hndl->last_change_check) >= hndl->change_check_time)
    {
        hndl->last_change_check = curr;
        if (hndl->nextCommand)
        {
            //nus_transfer_cmd();
        }

    }
}

uint8_t nus_transfer_cmd(blec_send_target send_target, nus_t nus_instance)
{
  Serial.println("Sending NUS command, nus_transfer_cmd");
  Serial.println(send_target.cmd);
  nus_instance.pNusRXCharacteristic->writeValue((uint8_t*)send_target.cmd, strlen(send_target.cmd), true);
  send_target.status = 1;
  Serial.println("Sent");
  return send_target.status;
}