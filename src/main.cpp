#include <Arduino.h>
#include "BLEDevice.h"
#include "nrf_nus.h"

#define TARGET_NAME "BLE_SRVR"

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLEAdvertisedDevice *myDevice;
BLERemoteService *pNusService;
static nus_t nus;
static uint8_t allowedToConnect = 0;
char receivedString[1024];
uint16_t serialRecLen = 0;

static void notifyCallback(
    BLERemoteCharacteristic *pBLERemoteCharacteristic,
    uint8_t *pData,
    size_t length,
    bool isNotify)
{
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);
  Serial.print("data: ");
  Serial.write(pData, length);
  Serial.println();
}

class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient *pclient)
  {
    Serial.println("Connected to BLE server!");
  }

  void onDisconnect(BLEClient *pclient)
  {
    connected = false;
    Serial.println("Disconnected!");
    BLEDevice::getScan()->start(0);
  }
};

static void nusTxNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                                uint8_t *pData,
                                size_t length,
                                bool isNotify)
{
  Serial.print("Notify callback for TX characteristic ");
  Serial.print(" of data length ");
  Serial.println(length);
  Serial.print("data: ");
  Serial.println((char *)pData);

  if (strncmp((char *)pData, "PONG", 4) == 0)
  {
    Serial.println("Ping successful");
  }
}

bool connectToServer()
{
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient *pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(myDevice); // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  Serial.println(" - Connected to server");
  pClient->setMTU(517); // set client to request maximum MTU from server (default is 23 otherwise)

  // Obtain a reference to the service we are after in the remote BLE server.
  pNusService = pClient->getService(NRF_NUS);
  if (pNusService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(NRF_NUS.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  nus.pNusRXCharacteristic = pNusService->getCharacteristic(NRF_NUS_RX_CHAR);
  if (nus.pNusRXCharacteristic == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(NRF_NUS_RX_CHAR.toString().c_str());
    pClient->disconnect();
    return false;
  }

  nus.pNusTXCharacteristic = pNusService->getCharacteristic(NRF_NUS_TX_CHAR);
  if (nus.pNusTXCharacteristic == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(NRF_NUS_TX_CHAR.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristics");

  if (nus.pNusTXCharacteristic->canNotify())
  {
    nus.pNusTXCharacteristic->registerForNotify(nusTxNotifyCallback);
  }
  connected = true;
  return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.getName().c_str() == TARGET_NAME)
    {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  }   // onResult
};    // MyAdvertisedDeviceCallbacks

void process_serial(char * pDataString, uint16_t len)
{
  char responseString[100];
  memset(responseString, 100, 0);

  if (strncmp((char *)receivedString, "PING", 4) == 0)
  {
    Serial.println("PONG");
  }

  if (strncmp((char *)receivedString, "VAR", 4) == 0) // set variable to selected value
  {
    Serial.println("PONG");
  }


}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
} // End of setup.

// This is the Arduino main loop function.
void loop()
{
  if (doConnect == true)
  {
    if (connectToServer())
    {
      Serial.println("We are now connected to the BLE Server.");
    }
    else
    {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected)
  {
    // Set the characteristic's value to be the array of bytes that is actually a string.
    // pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
    // process_Connected
  }
  else if (doScan)
  {
    BLEDevice::getScan()->start(0); // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }

  while (Serial.available())
  {
    receivedString[serialRecLen] = (char)Serial.read();

    if (receivedString[serialRecLen - 1] == '\r' && receivedString[serialRecLen] == '\n')
    {
      process_serial(receivedString, serialRecLen);
      serialRecLen = 0;
    }
    else
    {
      serialRecLen++;
      if (serialRecLen >= 1022)
      {
        serialRecLen = 0;
      }
    }
  }
}