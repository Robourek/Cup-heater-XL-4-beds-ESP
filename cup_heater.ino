#include <EEPROM.h>

//***********************//
#define LCD_ADDRESS 0x3C
#include <U8x8lib.h>
U8X8_SSD1306_72X40_ER_HW_I2C u8x8(/* reset=*/U8X8_PIN_NONE);  // EastRising 0.42"

//***********************//
#include "temp.h"
const char DEGREE_SYMBOL[] = { 0xB0, '\0' };
int m_iLastTemp = 0;
int m_iTargetTemp = 50;

//***********************//
#define BT_UP 10
#define BT_SET 12
#define BT_DOWN 11
#define LONG_PRESS 2000

unsigned long m_ulSetPressed = 0;
bool m_bSetup = false;
unsigned long m_ulUpDownPressed = 0;

//***********************//
#define HEAT 3
bool m_bHeat = false;
int m_iHeatPWM = 255;

//***********************//
#define PG 2
bool m_bPowerGood = false;

//-----------------------------------------------------------------//
void View(bool bSetup, bool bHeat)
{
  char chInt[20];

  u8x8.setFont(u8x8_font_8x13_1x2_r);
  u8x8.drawString(0, 0, " Teplota");

  sprintf(chInt, "%2d", bSetup ? m_iTargetTemp : m_iLastTemp);
  u8x8.setFont(u8x8_font_8x13_1x2_f);
  u8x8.drawString(0, 2, bSetup ? "[" : " ");
  u8x8.drawString(1, 2, chInt);
  u8x8.setCursor(3, 2);
  u8x8.print(DEGREE_SYMBOL);
  u8x8.print("C");
  u8x8.print(bSetup ? "]" : " ");

  u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);
  if (!m_bPowerGood)
  {
    u8x8.drawString(5, 4, "NOPD");
  }
  else if (bHeat)
  {
    sprintf(chInt, "%3d%%", (int)((m_iHeatPWM / 255.0 * 100.0) + 0.5));
    //sprintf(chInt, "%3d ", m_iHeatPWM);
    u8x8.drawString(5, 4, chInt);
  }
  else
  {
    u8x8.drawString(5, 4, "----");
  }
}

//-----------------------------------------------------------------//
void setup()
{
  Serial.begin(9600);
  m_iTargetTemp = EEPROM.read(sizeof(m_iTargetTemp));
  if (m_iTargetTemp > 99 || m_iTargetTemp < 30)
    m_iTargetTemp = 50;

  pinMode(PG, INPUT);

  pinMode(BT_UP, INPUT);
  pinMode(BT_SET, INPUT);
  pinMode(BT_DOWN, INPUT);

  pinMode(HEAT, OUTPUT);
  analogWrite(HEAT, 0);

  u8x8.begin();
  //u8x8.setPowerSave(0);
}

//-----------------------------------------------------------------//
unsigned long m_ulStart_100ms = -1;
#define PERIOD_100ms 100L

unsigned long m_ulStart_200ms = -1;
#define PERIOD_200ms 200L

//-----------------------------------------------------------------//
void loop()
{
  if (millis() >= m_ulStart_100ms + PERIOD_100ms)
  {
    m_ulStart_100ms = millis();

    if (!m_bSetup && m_ulSetPressed == 0 && !digitalRead(BT_SET))  // Press
    {
      m_ulSetPressed = millis();
      Serial.println("Press");
    }
    else if (!m_bSetup && m_ulSetPressed && digitalRead(BT_SET))  // Short Press Release
    {
      m_ulSetPressed = 0;
      m_bHeat = !m_bHeat;
      View(m_bSetup, m_bHeat);
      Serial.println("Short Press Release");
    }
    else if (!m_bSetup && m_ulSetPressed + LONG_PRESS < millis() && !digitalRead(BT_SET))  // Long Press and EnterSetup
    {
      m_bSetup = true;
      Serial.println("Long Press");
    }
    else if (m_bSetup && digitalRead(BT_SET) && m_ulSetPressed)  // Long Press Release
    {
      m_ulSetPressed = 0;
      Serial.println("Long Press Release");
    }
    else if (m_bSetup && m_ulSetPressed == 0 && !digitalRead(BT_SET))  // Next press Leave Setup
    {
      m_bSetup = false;
      EEPROM.write(sizeof(m_iTargetTemp), m_iTargetTemp);
      Serial.println("Next press Leave Setup");
    }

    if (m_bSetup == true)
    {
      if (m_ulUpDownPressed == 0 && digitalRead(BT_UP) != digitalRead(BT_DOWN))
      {
        m_ulUpDownPressed = millis();
        m_iTargetTemp += !digitalRead(BT_UP) ? 1 : -1;
      }
      else if (m_ulUpDownPressed + 800 < millis() && (digitalRead(BT_UP) != digitalRead(BT_DOWN)))
      {
        m_iTargetTemp += !digitalRead(BT_UP) ? 1 : -1;
      }
      else
      {
        m_ulUpDownPressed = 0;
      }
      if (m_iTargetTemp > 99)
        m_iTargetTemp = 99;
      else if (m_iTargetTemp < 30)
        m_iTargetTemp = 30;

      View(m_bSetup, m_bHeat);
    }

    m_bPowerGood = !digitalRead(PG);
  }

  if (millis() >= m_ulStart_200ms + PERIOD_200ms)
  {
    m_ulStart_200ms = millis();

    m_iLastTemp = getTemp();
    View(m_bSetup, m_bHeat);

    if (m_bHeat && m_bPowerGood)
    {
      if (m_iLastTemp > m_iTargetTemp)
        m_iHeatPWM = 0;
      else if (m_iLastTemp == m_iTargetTemp)
        m_iHeatPWM = 70;
      else if (m_iLastTemp + 1 >= m_iTargetTemp)
        m_iHeatPWM = 120;
      else if (m_iLastTemp + 2 >= m_iTargetTemp)
        m_iHeatPWM = 160;
      else if (m_iLastTemp + 3 >= m_iTargetTemp)
        m_iHeatPWM = 190;
      else if (m_iLastTemp + 4 >= m_iTargetTemp)
        m_iHeatPWM = 220;
      else if (m_iLastTemp + 5 >= m_iTargetTemp)
        m_iHeatPWM = 230;
      else
        m_iHeatPWM = 255;
    }
    else
    {
      m_iHeatPWM = 0;
    }
    analogWrite(HEAT, m_iHeatPWM);
  }
}