#include <Arduino.h>
#include "BLEDevice.h"
#include "nrf_nus.h"

#define TARGET_NAME "BLE_SRVR"

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static boolean scanCompleted = false;
static boolean tryToPing = false;
static BLEAdvertisedDevice *myDevice;
BLERemoteService *pNusService;
BLEScan *pBLEScan;
static nus_t nus;
static uint8_t allowedToConnect = 0;
char receivedString[1024];
uint16_t serialRecLen = 0;
uint16_t testVar = 0;
uint32_t currMillis, prevMillis;
const uint8_t notificationOff[] = {0x0, 0x0};
const uint8_t notificationOn[] = {0x1, 0x0};

SemaphoreHandle_t distanceVarMutex;

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

// Funkcija za lovljenje dogodkov na BLE povezavi
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
    tryToPing = false;
  }
};
// Funkcija za lovljenje dogodkov na TX značilosti in sprejem podatkov.
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

// Funkcija za vzpostavljanje povezave z BLE strežnikom
bool connectToServer()
{
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient *pClient = BLEDevice::createClient(); // Ustvarimo kazalec na spremenljivko tipa BLEClient
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback()); // Dodelimo funkcijo za lovljenje dogodkov BLE povezave s strežnikom

  // Povežimo se na BLE strežnik
  pClient->connect(myDevice);
  Serial.println(" - Connected to server");
  pClient->setMTU(517); // Nastavi maksimalno velikost paketa

  // Poizvedi če na BLE strežniku obstaja NUS storitev
  pNusService = pClient->getService(NRF_NUS);
  if (pNusService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(NRF_NUS.toString().c_str());
    pClient->disconnect(); // Če storitve ni, naj prekine povezavo
    return false;
  }
  Serial.println(" - Found our service");

  // Poišči značilnosti v naši storitvi RX
  nus.pNusRXCharacteristic = pNusService->getCharacteristic(NRF_NUS_RX_CHAR);
  if (nus.pNusRXCharacteristic == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(NRF_NUS_RX_CHAR.toString().c_str());
    pClient->disconnect(); // Če značilnosti ni, naj prekine povezavo
    return false;
  }
  // Poišči značilnosti v naši storitvi TX
  nus.pNusTXCharacteristic = pNusService->getCharacteristic(NRF_NUS_TX_CHAR);
  if (nus.pNusTXCharacteristic == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(NRF_NUS_TX_CHAR.toString().c_str());
    pClient->disconnect(); // Če značilnosti ni, naj prekine povezavo
    return false;
  }
  Serial.println(" - Found our characteristics");

  if (nus.pNusTXCharacteristic->canNotify())
  {
    nus.pNusTXCharacteristic->registerForNotify(nusTxNotifyCallback);
    Serial.println("Can notify activated, trying to register notify");
  }
  connected = true; // Povezava uspešno vzpostavljena
  return true;
}

// Funkcija vrne dogodke iz skenerja BLE naprav
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // Če je ime najdene BLE naprave enako imenu iskane naprave, vzpostavi povezavo
    if (strncmp(advertisedDevice.getName().c_str(), TARGET_NAME, strlen(TARGET_NAME)) == 0)
    {
      Serial.println("Found correct device, starting to connect");
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    }
  }
};

// Obdelava serijskih komand
void process_serial(char *pDataString, uint16_t len)
{
  char responseString[100];
  memset(responseString, 100, 0);

  if (strncmp((char *)receivedString, "PING", 4) == 0)
  {
    Serial.println("PONG");
  }

  if (strncmp((char *)receivedString, "VAR ", 4) == 0) // set variable to selected value
  {
    char varStr[10];
    strcpy(varStr, receivedString + 4);
    xSemaphoreTake(distanceVarMutex, portMAX_DELAY);
    testVar = strtol(varStr, NULL, 10);
    xSemaphoreGive(distanceVarMutex);
    Serial.printf("Var is %d\r\n", testVar);
  }

  if (strncmp((char *)receivedString, "TARGET", 7) == 0)
  {
    Serial.println("Setting target name to:");
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init(""); // Inicializiraj BLE

  distanceVarMutex = xSemaphoreCreateMutex();

  // Nastavi skeniranje za BLE napravami
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks()); // Nastavi lovljenje dogodkov za skeniranje
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(500, false);
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
    tryToPing = true;
    // Set the characteristic's value to be the array of bytes that is actually a string.
    // pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
    // process_Connected
  }
  else if (doScan)
  {
    BLEDevice::getScan()->start(0); // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }

  // Glej serijski vhod za komande
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

  if (currMillis - prevMillis > 5000)
  {
    if (scanCompleted && connected == 0)
    {
      Serial.println("Havent found device in this scan, starting again!");
    }

    if (tryToPing)
    {
      Serial.println("Pinging");
      char buf[20];
      sprintf(buf, "PING\r\n");
      nus.pNusRXCharacteristic->writeValue(buf, false);
    }
    prevMillis = currMillis;
    Serial.println("Timer tripped");
  }

  currMillis = millis();
}