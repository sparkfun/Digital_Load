/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "MPR121.h"
#include "digital_load.h"

#define KP  0.15
#define KI  0.10
#define KD  0

// systemTimer is incremented in the tickISR, which occurs once a millisecond.
//  It's the timebase for the entire firmware, upon which all other timing
//  is based.
volatile int32 systemTimer = 0;
CY_ISR_PROTO(tickISR);

CY_ISR_PROTO(keypadISR);
volatile bool keyPressOccurred = false; 

int main(void)
{
  CyGlobalIntEnable; /* Enable global interrupts. */
  
  // ARM devices have an internal system tick which can be used as a timebase
  // without having to tie up additional system resources. Here we set it up to
  // point to our system tick ISR and configure it to occur once every 64000
  // clock ticks, which is once per millisecond on our 64MHz processor.
  CyIntSetSysVector((SysTick_IRQn + 16), tickISR);
  SysTick_Config(64000);
  
  // Create our timing "tick" variables. These are used to record when the last
  // iteration of an action in the main loop happened and to trigger the next
  // iteration when some number of clock ticks have passed.
  int32_t _20HzTick = 0;
  int32_t _1kHzTick = 0;
  
  Gate_Drive_Tune_Start();
  Offset_Start();
  Offset_Gain_Start();
  GDT_Buffer_Start();
  O_Buffer_Start();
  LCD_Start();
  LCD_DisplayOn();
  I2C_Start();
  Keypad_Interrupt_StartEx(keypadISR);
  Source_ADC_Start();
  I_Source_ADC_Start();
  CyDelay(100);
  mpr121QuickConfig();
  LCD_PrintString("Hello, world");
  
  uint8_t mode = DISPLAY;
  float iLimit = 1;
  float error = 0.0;
  float lastError = 0.0;
  float integral = 0.0;
  float derivative = 0.0;
  float vSource = 0.0;
  int16_t vSourceRaw = 0;
  float iSource = 0.0;
  float setPoint = 0.0;
  int32_t iSourceRaw = 0;
  int8_t keypadValue = -1;
  uint16_t grossSetPoint = 64;
  uint16_t fineSetPoint = 0;
  
  Offset_SetValue(grossSetPoint);
  Gate_Drive_Tune_SetValue(fineSetPoint);
  
  for(;;)
  {
    char buff[16];
    
    if (systemTimer - 1 > _1kHzTick)
    {
      _1kHzTick = systemTimer;
      I_Source_ADC_StartConvert();
      I_Source_ADC_IsEndConversion(I_Source_ADC_WAIT_FOR_RESULT);
      iSourceRaw = I_Source_ADC_GetResult32();
      iSource = 10 * I_Source_ADC_CountsTo_Volts(iSourceRaw);
      lastError = error;
      error = iLimit - iSource;
      integral = integral + error;
      derivative = error - lastError;
      setPoint = (KP * error) + (KI * integral) + (KD * derivative);
      
      // setPoint is a floating point voltage. We need to convert that
      //  into an integer that can be fed into our DACs.
      // First, find our grossSetPoint. This is a largish voltage that
      //  represents a coarse 0-4V offset in 16mV steps.
      grossSetPoint = (int)(setPoint / 0.016);
      // We want to limit our gross offset to 255, since it's an 8-bit
      //  DAC output.
      if (grossSetPoint > 255) grossSetPoint = 255;
      // Now, find the fineSetPoint. This is a 4mV step 8-bit DAC which
      //  allows us to tune the set point a little more finely.
      fineSetPoint = (int)((setPoint - grossSetPoint*0.016)/0.004);
      // Finally, one last check: if the source voltage is below 0.3V,
      //  we want to make both of our output values ZERO.
      Source_ADC_StartConvert();
      Source_ADC_IsEndConversion(Source_ADC_WAIT_FOR_RESULT);
      vSourceRaw = Source_ADC_GetResult16();
      vSource = 10*Source_ADC_CountsTo_Volts(vSourceRaw);
      if (vSource < 0.3)
      {
        error = 0;
        lastError = 0;
        integral = 0;
        derivative = 0;
        grossSetPoint = 0;
        fineSetPoint = 0;
      }
      Offset_SetValue(grossSetPoint);
      Gate_Drive_Tune_SetValue(fineSetPoint);
    } // if (systemTimer - 50 > _20HzTick)
    if (systemTimer - 200 > _20HzTick)
    {
      _20HzTick = systemTimer;
      Source_ADC_StartConvert();
      Source_ADC_IsEndConversion(Source_ADC_WAIT_FOR_RESULT);
      vSourceRaw = Source_ADC_GetResult16();
      vSource = 10*Source_ADC_CountsTo_Volts(vSourceRaw);
      
      if (keyPressOccurred == true)
      {
        keyPressOccurred = false;
        keypadValue = getKeypadNumber();
      } // if (keyPressOccurred == true)
      if (mode == DISPLAY)
      {
        LCD_ClearDisplay();
        LCD_PrintString("Press 1 to set I");
        LCD_Position(1,0);
        sprintf(buff, "I Lim: %.3f", iLimit);
        LCD_PrintString(buff);
        LCD_Position(2,0);
        sprintf(buff, "V Source: %.3f", vSource);
        LCD_PrintString(buff);
        LCD_Position(3,0);
        sprintf(buff, "I Source: %.3f", iSource);
        LCD_PrintString(buff);
        if (keypadValue == 1) 
        {
          mode = SET;
          sprintf(buff, "0");
          keypadValue = -1;
        }
      } // if (mode == DISPLAY)
      if (mode == SET)
      {
        LCD_ClearDisplay();
        LCD_PrintString("Set current in A");
        LCD_Position(1,0);
        LCD_PrintString(buff);
        if (keypadValue == ENTER)
        {
          mode = DISPLAY;
          float temp = atoff(buff);
          if (temp) iLimit = temp;
        }
        if (keypadValue != -1)
        {
          if (keypadValue == 0) strcat(buff, "0");
          else if (keypadValue == 1) strcat(buff, "1");
          else if (keypadValue == 2) strcat(buff, "2");
          else if (keypadValue == 3) strcat(buff, "3");
          else if (keypadValue == 4) strcat(buff, "4");
          else if (keypadValue == 5) strcat(buff, "5");
          else if (keypadValue == 6) strcat(buff, "6");
          else if (keypadValue == 7) strcat(buff, "7");
          else if (keypadValue == 8) strcat(buff, "8");
          else if (keypadValue == 9) strcat(buff, "9");
          else if (keypadValue == 10) strcat(buff, ".");
          keypadValue = -1;
        }
      } // if (mode == SET)
    } // if (systemTimer - 200 > _5HzTick)
    
  }
  return 0;
}

CY_ISR(keypadISR)
{
  keyPressOccurred = true;
  Keypad_Interrupt_ClearPending();
  keypadIRQ_ClearInterrupt();
}

CY_ISR(tickISR)
{
  systemTimer++;
}

/* [] END OF FILE */
