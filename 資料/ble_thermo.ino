#include <bluefruit.h>

BLEDis bledis;
BLEHidAdafruit blehid;

bool hasKeyPressed = false;

char strKey2[] = "tamanegi";
char strKey3[] = "\n";
uint iIndexKey = 0;                   // 配列の出力インデックス
uint iSizeKey = 0;                    // 文字列サイズの保存

// +onndokei start
#include <Adafruit_TinyUSB.h> // for Serial
#include <Servo.h>
Servo servo1;
// +onndokei end

// deep sleep start
#include <Arduino.h>
#include <nrf.h>
const uint32_t sleepTimeMs = 9600000; // deep sleep time
// deep sleep start

// save start 
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

using namespace Adafruit_LittleFS_Namespace;

#define FILENAME    "/adafruit.txt"
#define CONTENTS    "Modified contents"

char strKey4[] = "-99.0,-99.1,-99.2,-99.3,-99.4,-99.5,-99.6,-99.7,-99.8,-99.9";
char strKey5[64];
char strKey6[] = ",";

int i,j;

File file(InternalFS);
// save end

void setup()
{
  // +onndokei start
  Serial.begin(9600);//115200kamo
  //servo1.attach(9);
  //pinMode(9, OUTPUT);
  // +onndokei end

  // ble start
  Bluefruit.begin();
  Bluefruit.setTxPower(4);

  bledis.setManufacturer("Adafruit Industries");
  bledis.setModel("Bluefruit Feather 52");
  bledis.begin();

  blehid.begin();

  blehid.setKeyboardLedCallback(set_keyboard_led);

  startAdv();
  // ble end

  // deep sleep setup start
  NRF_RTC2->PRESCALER = 0;                          // No prescaling (16MHz clock)
  NRF_RTC2->CC[0] = sleepTimeMs * 32.768;           // Compare value (32.768kHz clock)
  NRF_RTC2->EVTENSET = RTC_EVTENSET_COMPARE0_Msk;   // Enable COMPARE0 event
  NRF_RTC2->INTENSET = RTC_INTENSET_COMPARE0_Msk;   // Enable COMPARE0 interrupt
  NVIC_EnableIRQ(RTC2_IRQn);                        // Enable RTC2 interrupt
  NRF_RTC2->TASKS_START = 1;                        // Start RTC2
  // deep sleep setup end

  //割込みの無効化
  NVIC_DisableIRQ(UART0_IRQn); // Disable UART0 interrupt
  NVIC_DisableIRQ(SPI0_TWI0_IRQn); // Disable SPI0_TWI0 interrupt
}

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_KEYBOARD);

  // Include BLE HID service
  Bluefruit.Advertising.addService(blehid);

  // There is enough room for the dev name in the advertising packet
  Bluefruit.Advertising.addName();

  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   *
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

void loop()
{
  
  // ディープスリープモードからの復帰後の処理
  if (hasKeyPressed)
  {
    servo1.attach(9);
    pinMode(9, OUTPUT);
    // save start
    InternalFS.begin();
  
    file.open(FILENAME, FILE_O_READ);

    uint32_t readlen;
    char buffer[64] = {0};
    i=0;
    j=0;
    readlen = file.read(buffer, sizeof(buffer));
    buffer[readlen] = 0;
    //Serial.println(buffer);
    file.close(); // ファイルを閉じる
    // save end
    
    // +onndokei start
    delay(1000); // センサの値が安定するまで待つ(いらんかも)
    float readValue = analogRead(0); // A0: 温度センサ（10kΩ プルアップ）
    readValue = (321 - readValue) / 10;//接触いいなら321
    snprintf(strKey4, 6, "%.1f", readValue);
    while(i != 9){
      strKey5[j]=buffer[j];
      if(buffer[j]=='.'){
        i++;
      }
      j++;
    }
    strKey5[j]=buffer[j];
    strKey5[j+1]= '\0';
    strcat(strKey4, strKey6);
    strcat(strKey4, strKey5);
    strcat(strKey4, strKey3);

    InternalFS.remove("/adafruit.txt");

    file = File(InternalFS); // fileオブジェクトを再作成

    if (file.open(FILENAME, FILE_O_WRITE)) // Change FILE_O_READ to FILE_O_WRITE
    {
      file.write(strKey4, strlen(strKey4));
      file.close();
    }
    
    int servoValue = readValue*4.5;
    
    if (servoValue <= 180 && servoValue >= 0)
    {
      servo1.write(servoValue);
    }
    else if (servoValue < 0)
    {
      servo1.write(0);
    }
    else
    {
      servo1.write(180);
    }
    servo1.detach();
  // +onndokei end

    for (int i = 0; i < strlen(strKey4); i++)
    {
      blehid.keyPress(strKey4[i]);
      blehid.keyRelease();
    }
    hasKeyPressed = false; // フラグをリセット

  }


  // deep sleep loop start
  // ディープスリープモードへの遷移
  __WFE();
  __SEV();
  __WFE();
  // deep sleep loop end
  // ディープスリープモードからの復帰後の処理
}

/**
 * Callback invoked when received Set LED from central.
 * Must be set previously with setKeyboardLedCallback()
 *
 * The LED bit map is as follows: (also defined by KEYBOARD_LED_* )
 *    Kana (4) | Compose (3) | ScrollLock (2) | CapsLock (1) | Numlock (0)
 */
void set_keyboard_led(uint16_t conn_handle, uint8_t led_bitmap)
{
  (void)conn_handle;

  // light up Red Led if any bits is set
  //if (led_bitmap)
  //{
  //  ledOn(LED_RED);
  //}
  //else
  //{
  //  ledOff(LED_RED);
  //}
}

extern "C"
{
  void RTC2_IRQHandler()
  {
    if (NRF_RTC2->EVENTS_COMPARE[0] && (NRF_RTC2->INTENSET & RTC_INTENSET_COMPARE0_Msk))
    {
      NRF_RTC2->EVENTS_COMPARE[0] = 0;
      NRF_RTC2->TASKS_CLEAR = 1;

      // ディープスリープモードからの復帰処理
      // (例: フラグをセットして復帰後の処理を行う)
      hasKeyPressed = true;
    }
  }
}
