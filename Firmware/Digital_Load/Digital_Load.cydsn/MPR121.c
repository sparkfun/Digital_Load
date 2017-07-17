#include <project.h>
#include "MPR121.h"

void mpr121Write(unsigned char address, unsigned char data)
{
  I2C_MasterSendStart(0x5a, 0);
  I2C_MasterWriteByte(address);
  I2C_MasterWriteByte(data);
  I2C_MasterSendStop();
}

uint8_t mpr121Read(uint8_t address)
{
  uint8_t data;
  I2C_MasterSendStart(0x5a, 0);
  I2C_MasterWriteByte(address);
  I2C_MasterSendRestart(0x5a,1);
  data = I2C_MasterReadByte(I2C_NAK_DATA);
  I2C_MasterSendStop();
  return data;
}

void mpr121QuickConfig(void)
{
  // Section A
  // This group controls filtering when data is > baseline.
  mpr121Write(MHD_R, 0x01);
  mpr121Write(NHD_R, 0x01);
  mpr121Write(NCL_R, 0x00);
  mpr121Write(FDL_R, 0x00);

  // Section B
  // This group controls filtering when data is < baseline.
  mpr121Write(MHD_F, 0x01);
  mpr121Write(NHD_F, 0x01);
  mpr121Write(NCL_F, 0xFF);
  mpr121Write(FDL_F, 0x02);
  
  // Section C
  // This group sets touch and release thresholds for each electrode
  mpr121Write(ELE0_T, TOU_THRESH);
  mpr121Write(ELE0_R, REL_THRESH);
  mpr121Write(ELE1_T, TOU_THRESH);
  mpr121Write(ELE1_R, REL_THRESH);
  mpr121Write(ELE2_T, TOU_THRESH);
  mpr121Write(ELE2_R, REL_THRESH);
  mpr121Write(ELE3_T, TOU_THRESH);
  mpr121Write(ELE3_R, REL_THRESH);
  mpr121Write(ELE4_T, TOU_THRESH);
  mpr121Write(ELE4_R, REL_THRESH);
  mpr121Write(ELE5_T, TOU_THRESH);
  mpr121Write(ELE5_R, REL_THRESH);
  mpr121Write(ELE6_T, TOU_THRESH);
  mpr121Write(ELE6_R, REL_THRESH);
  mpr121Write(ELE7_T, TOU_THRESH);
  mpr121Write(ELE7_R, REL_THRESH);
  mpr121Write(ELE8_T, TOU_THRESH);
  mpr121Write(ELE8_R, REL_THRESH);
  mpr121Write(ELE9_T, TOU_THRESH);
  mpr121Write(ELE9_R, REL_THRESH);
  mpr121Write(ELE10_T, TOU_THRESH);
  mpr121Write(ELE10_R, REL_THRESH);
  mpr121Write(ELE11_T, TOU_THRESH);
  mpr121Write(ELE11_R, REL_THRESH);
  
  // Section D
  // Set the Filter Configuration
  // Set ESI2
  mpr121Write(FIL_CFG, 0x04);
  
  // Section E
  // Electrode Configuration
  // Enable 6 Electrodes and set to run mode
  // Set ELE_CFG to 0x00 to return to standby mode
  mpr121Write(ELE_CFG, 0x0C);	// Enables all 12 Electrodes
  //mpr121Write(ELE_CFG, 0x06);		// Enable first 6 electrodes
  
  // Section F
  // Enable Auto Config and auto Reconfig
  /*mpr121Write(ATO_CFG0, 0x0B);
  mpr121Write(ATO_CFGU, 0xC9);	// USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V   
  mpr121Write(ATO_CFGL, 0x82);	// LSL = 0.65*USL = 0x82 @3.3V
  mpr121Write(ATO_CFGT, 0xB5);*/	// Target = 0.9*USL = 0xB5 @3.3V
}

int8_t getKeypadNumber(void)
{
  uint16_t touchStatus;
  uint8_t touchNumber=0;
  uint8_t j = 0;
  touchStatus = mpr121Read(0x01) << 8;
  touchStatus |= mpr121Read(0x00);
  for (j=0; j<12; j++)  // Check how many electrodes were pressed
  {
    if ((touchStatus & (1<<j)))
      touchNumber++;
  }
  if (touchNumber > 1) return -1;
  if (touchStatus & 1<<0) return 9;
  else if (touchStatus & 1<<1) return 6;
  else if (touchStatus & 1<<2) return 3;
  else if (touchStatus & 1<<3) return ENTER;
  else if (touchStatus & 1<<4) return 8;
  else if (touchStatus & 1<<5) return 5;
  else if (touchStatus & 1<<6) return 2;
  else if (touchStatus & 1<<7) return DECIMAL;
  else if (touchStatus & 1<<8) return 7;
  else if (touchStatus & 1<<9) return 4;
  else if (touchStatus & 1<<10) return 1;
  else if (touchStatus & 1<<11) return 0;
  else return -1;
}
