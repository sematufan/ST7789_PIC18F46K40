#include <18F46K40.h> 

#use delay(internal=32000000) 
#FUSES PUT                    //Power Up Timer
#FUSES PROTECT                //Code not protected from reading
#FUSES NODEBUG                //No Debug mode for ICD
#FUSES BROWNOUT               //No brownout reset
#FUSES NOLVP                  //No low voltage prgming, B3(PIC16) or B5(PIC18) used for I/O
#FUSES NOCPD                  //No EE protection
#FUSES NOWRT 
#FUSES NOMCLR                 //Program memory not write protected
#FUSES NOWDT



#BYTE       TRISA       =0xF88
#BYTE       TRISB       =0xF89
#BYTE       TRISC       =0xF8A
#BYTE       TRISD       =0xF8B
#BYTE       TRISE       =0xF8C

#byte       PORTA       =0xF8D
#byte       PORTB       =0xF8E
#byte       PORTC       =0xF8F
#byte       PORTD       =0xF90
#byte       PORTE       =0xF91



#Bit  LED                = PORTC.7


#Bit  btnVoltage1Plus    = PORTA.0
#Bit  btnVoltage2Plus    = PORTA.1
#Bit  btnVoltage3Plus    = PORTA.2
#Bit  btnVoltage4Plus    = PORTA.3

#Bit  btnVoltage1Minus   = PORTC.4
#Bit  btnVoltage2Minus   = PORTD.3
#Bit  btnVoltage3Minus   = PORTD.2
#Bit  btnVoltage4Minus   = PORTD.6

#Bit  btnEmergencyStart      = PORTD.5
#Bit  btnEmergencyStop       = PORTD.4


#Bit  btnVoltage1Start   = PORTA.6
#Bit  btnVoltage2Start   = PORTE.2
#Bit  btnVoltage3Start   = PORTA.7
#Bit  btnVoltage4Start   = PORTC.0

#Bit  btnUp              = PORTE.0
#Bit  btnDown            = PORTA.5
#Bit  btnLeft            = PORTA.4
#Bit  btnRight           = PORTE.1



#pin_select SCK2=PIN_B1
#pin_select SDI2=PIN_B3
#pin_select SDO2=PIN_B2
#define TFT_RST   PIN_B4     // reset pin, optional
#define TFT_DC    PIN_B5     // data/command pin  
//#use spi(DI=PIN_B3, DO=PIN_B2, CLK=PIN_B1,BAUD = 16000000, BITS = 8,MODE =2,STREAM = ST7789)
//#use SPI(SPI2, MODE = 2, BITS =8, STREAM = ST7789)
//#use SPI2(DI=PIN_B3, DO=PIN_B2, CLK=PIN_B1)
#include <ST7789.c>
#include <GFX_Library.c>

#pin_select SCK1=PIN_C3
#pin_select SDI1=PIN_C1
#pin_select SDO1=PIN_C2
#include <NRF24L01PP.c>

const char mainMenu[4][20]= {
    "OUTPUT 1",
    "OUTPUT 2",
    "OUTPUT 3",
    "OUTPUT 4"
};

int16   lastTimeVoltage1Start=0;
int16   lastTimeVoltage2Start=0;
int16   lastTimeVoltage3Start=0;
int16   lastTimeVoltage4Start=0;
int16   lastTimeUp=0;
int16 currentTime=0;
int16   lastTimeDown=0;
int1  upBtnState=0;
int1  downBtnState=0;
int1  setPageState = 0;
int1  emergencyState=0;
int1  voltage1StartState=0;
int1  voltage2StartState=0;
int1  voltage3StartState=0;
int1  voltage4StartState=0;
int timerCounter=0;
int sleepState=0;
int   currentRow = 0;
float voltage1 = 0.0;
float voltage2 = 0.0;
float voltage3 = 0.0;
float voltage4 = 0.0;
int eepromAddress1=25;
int eepromAddress2=35;
int eepromAddress3=45;
int eepromAddress4=55; 
int16 sendValue=0;

void mainTemplate(void);
void homePage(void);
void setPage(void);
void mainMenuList();
void eepromWrite();
void eepromRead();
void sendDataForVoltage(int16);
void spi_write16(int16 data)
{
    msb = make8(data, 1);   // Send MSB
    lsb = make8(data, 0);   // Send LSB
}
//!#define INT_IOC_C0_L2H   0x1201C210 // IOC on rising edge on C0  
//!#define INT_IOC_C0_H2L   0x2201C210 // IOC on falling edge on C0  
//!#undef  INT_IOC_C0
//!#define INT_IOC_C0       0x3201C210  // IOC on either edge on C0  
#INT_EXT
void ext_isr(void)
{
    clear_interrupt(INT_EXT);
    disable_interrupts(int_timer0);
    timerCounter=0;
    restart_wdt();
    if(sleepState==1)
    {
       sleepState=0;
       startWrite();
       writeCommand(ST7789_SLPOUT);
       endWrite();
       endWrite();
    }
    else
    {
          //-------------------------BEGIN EMERGENCY  PRESS BUTTON --
          if  (btnEmergencyStop==1 && btnEmergencyStart==0) 
          {
            emergencyState=1;  
             display_fillRect(12, 52, 220, 143, ST7789_BLACK);
               delay_ms(1000);
            display_setTextColor(ST7789_RED);
            display_setCursor(60, 90);
            display_print("ACIL STOP");
            display_setCursor(90, 130);
            display_print("ACIK");
            sendDataForVoltage(33000);
            voltage1StartState=0;
            voltage2StartState=0;
            voltage3StartState=0;
            voltage4StartState=0;
            upBtnState=0;
            downBtnState=0;
          }       
      
          if  (btnEmergencyStart==1 && btnEmergencyStop==0 ) 
          {
            emergencyState=0;
              display_fillRect(12, 52, 220, 143, ST7789_BLACK);
            
            display_setTextColor(ST7789_GREEN);
            display_setCursor(60, 90);
            display_print("ACIL STOP");
            display_setCursor(90, 130);
            display_print("KAPALI");
            sendDataForVoltage(30000);
            delay_ms(500);
            homePage();  
          }
         //-------------------------END EMERGENCY PRESS BUTTON ------
         
         
         if(  emergencyState!=1)
        {
        
          //-------------------------BEGIN VOLTAGE1 PRESS BUTTON PLUS----
          while (btnVoltage1Plus==1)
          {
             restart_wdt(); 
             currentRow=0;
             setPageState=0;
             while (btnVoltage1Plus==1)
             {
                restart_wdt();
                if (voltage4 < 5) voltage1 += 0.10;
                sendValue=((voltage1*4096)/5);
                sendDataForVoltage(sendValue);
                setPage();
             }
                 eepromWrite();
                 delay_ms(500);
                 homePage();  
          }
          //-------------------------END VOLTAGE1 PRESS BUTTON PLUS----
          //-------------------------BEGIN VOLTAGE2 PRESS BUTTON PLUS----
          while(btnVoltage2Plus==1)
          {
             restart_wdt();
             currentRow=1;
             setPageState=0;
             while (btnVoltage2Plus==1)
             {
                restart_wdt();
                if (voltage4 < 5)  voltage2 += 0.10;
                sendValue=((voltage2*4096)/5)+4096;
                sendDataForVoltage(sendValue);
                setPage();
             }
             eepromWrite();
             delay_ms(500);
             homePage();
          }
          //-------------------------END VOLTAGE2 PRESS BUTTON PLUS----
          //-------------------------BEGIN VOLTAGE3 PRESS BUTTON PLUS----
          while (btnVoltage3Plus==1)
          {  
             restart_wdt();
             currentRow=2;
             setPageState=0;
             while (btnVoltage3Plus==1)
             {
                restart_wdt();
                if (voltage4 < 5) voltage3 += 0.10;
                sendValue=((voltage3*4096)/5)+8192;
                sendDataForVoltage(sendValue);
                setPage();
             }
             eepromWrite();
             delay_ms(500);
             homePage();
          }
          //-------------------------END VOLTAGE3 PRESS BUTTON PLUS----
          //-------------------------BEGIN VOLTAGE4 PRESS BUTTON PLUS----
          while (btnVoltage4Plus==1)
          {
              currentRow=3;
              setPageState=0;
               restart_wdt();
              while (btnVoltage4Plus==1)
             {
                 restart_wdt();
                 if (voltage4 < 5) voltage4 += 0.10;
                 sendValue=((voltage4*4096)/5)+12288;
                 sendDataForVoltage(sendValue);
                 setPage();
             }
             eepromWrite();
             delay_ms(500);
             homePage(); 
          }
          //-------------------------END VOLTAGE4 PRESS BUTTON MINUS---
          //-------------------------BEGIN VOLTAGE1 PRESS BUTTON MINUS-
          while (btnVoltage1Minus==1)
          {
             currentRow=0;
             setPageState=0;
             restart_wdt();
             while (btnVoltage1Minus==1)
             {
                 restart_wdt();
                 if (voltage1 >= 0.10) voltage1 -= 0.10;
                 sendValue=((voltage1*4096)/5);
                 sendDataForVoltage(sendValue);
                 setPage();
             }
             eepromWrite();
             delay_ms(500);
             homePage(); 
          }
          //-------------------------END VOLTAGE1 PRESS BUTTON MINUS----
          //-------------------------BEGIN VOLTAGE2 PRESS BUTTON MINUS--
          while (btnVoltage2Minus==1) 
             {
               currentRow=1;
               setPageState=0;
               restart_wdt();
               while (btnVoltage2Minus==1) 
               {
                  restart_wdt();
                  if (voltage2 >= 0.10) voltage2 -= 0.10;
                  sendValue=((voltage2*4096)/5);
                  sendValue+=4096;
                  sendDataForVoltage(sendValue);
                  setPage();
               }
               eepromWrite();
               delay_ms(500);
               homePage(); 
             }
          //-------------------------END VOLTAGE2 PRESS BUTTON MINUS----
          //-------------------------BEGIN VOLTAGE3 PRESS BUTTON MINUS--
          while  (btnVoltage3Minus==1) 
          {
              currentRow=2;
              setPageState=0;
              restart_wdt();
              while  (btnVoltage3Minus==1) 
              {
                 restart_wdt();
                 if (voltage3 >= 0.10) voltage3 -= 0.10;
                 sendValue=((voltage3*4096)/5)+8192;
                 sendDataForVoltage(sendValue);
                 setPage();
              }
              eepromWrite();
              delay_ms(500);
              homePage();   
          }
          //-------------------------END VOLTAGE3 PRESS BUTTON MINUS----
          //-------------------------BEGIN VOLTAGE4 PRESS BUTTON MINUS--
          while  (btnVoltage4Minus==1) 
          {
              currentRow=3;
              setPageState=0;
              restart_wdt();
              while  (btnVoltage4Minus==1) 
             {
                 restart_wdt();
                 if (voltage4 >= 0.10) voltage4 -= 0.10;
                 sendValue=((voltage4*4096)/5)+12288;
                 sendDataForVoltage(sendValue);
                 setPage();
             }
             eepromWrite();
             delay_ms(500);
             homePage();  
          }
          //-------------------------END VOLTAGE4 PRESS BUTTON PLUS----
          
          
          
          
          
          
          
      //-------------------------BEGIN VOLTAGE1 START BUTTON ------  
          if  (btnVoltage1Start==1) 
          {
             if(currentTime>lastTimeVoltage1Start+1)
                lastTimeVoltage1Start=currentTime;
              voltage1StartState=!voltage1StartState;
              if(voltage1StartState==0)sendDataForVoltage(41000);//Stop
              if(voltage1StartState==1) sendDataForVoltage(40000);//start
             }
          }
      //-------------------------END VOLTAGE1  START BUTTON -------
      //-------------------------BEGIN VOLTAGE2 START BUTTON ------  
          if  (btnVoltage2Start==1) 
          {
            if(currentTime>lastTimeVoltage2Start+1)
             { 
                lastTimeVoltage2Start=currentTime;
                voltage2StartState=!voltage2StartState;
                if(voltage2StartState==0)sendDataForVoltage(43000);//Stop
                if(voltage2StartState==1) sendDataForVoltage(42000);//start
             }
          }
             
      //-------------------------END VOLTAGE2 START BUTTON --------
      //-------------------------BEGIN VOLTAGE3 START BUTTON ------  
          if  (btnVoltage3Start==1) 
          {
             if(currentTime>lastTimeVoltage3Start+1)
             { 
              lastTimeVoltage3Start=currentTime;
              voltage3StartState=!voltage3StartState;
              if(voltage3StartState==0)sendDataForVoltage(45000);
              if(voltage3StartState==1) sendDataForVoltage(44000);
             }
          }
      //-------------------------END VVOLTAGE3 START BUTTON--------  
      //-------------------------BEGIN VOLTAGE4 START BUTTON-------
           
             if(btnVoltage4Start==1)
          {
             if(currentTime>lastTimeVoltage4Start+2)
             { 

                lastTimeVoltage4Start=currentTime;                 
                voltage4StartState=!voltage4StartState;
                if(voltage4StartState==0)sendDataForVoltage(47000); // Stop  
                if(voltage4StartState==1) sendDataForVoltage(46000);//Start       
                
             }
      
          }
      //-------------------------END VOLTAGE4 START BUTTON---------
      
      
      
      
      //-------------------------BEGIN UP  BUTTON-------
         if(btnUp==1) 
          {
              if(downBtnState==1)
              {         
                  sendDataForVoltage(56000);
                  downBtnState=0;
              }
              else
              {
                    if(currentTime>lastTimeDown+2)
                    {
                      lastTimeDown=currentTime;
                      lastTimeUp=currentTime;
                      upBtnState=!upBtnState;
                      if(upBtnState==1)sendDataForVoltage(48000);
                      else sendDataForVoltage(49000);
                      
                    }

              }
          }
      //-------------------------END UP  BUTTON---------
      
      //-------------------------BEGIN DOWN  BUTTON-------
          if (btnDown==1) 
          {
            
            if(upBtnState==1)
              {         
                  sendDataForVoltage(56000);
                  upBtnState=0;
              }
              else
              {
                if(currentTime>lastTimeUp+2)
                {
                    
                    lastTimeDown=currentTime;
                    lastTimeUp=currentTime;
                    downBtnState=!downBtnState;
                    if(downBtnState==1) sendDataForVoltage(50000);
                    else sendDataForVoltage(51000);
             
                }
              }
          }
      //-------------------------END DOWN  BUTTON---------
      
      
      //-------------------------BEGIN LEFT  BUTTON-------
          while  (btnLeft==1) 
          {    restart_wdt();
               while  (btnLeft==1) sendDataForVoltage(52000);
               sendDataForVoltage(53000); 
          }
      //-------------------------END LEFT  BUTTON---------
      
      //-------------------------BEGIN RIGHT  BUTTON-------
            while  (btnRight==1) 
          {
             restart_wdt();
             while  (btnRight==1) sendDataForVoltage(54000);
             sendDataForVoltage(55000);
          }
      //-------------------------END RIGHT  BUTTON---------
             
       }
  

    enable_interrupts(int_timer0);
    delay_ms(100);
}

#int_timer0
void timer0_Int()
{
   set_timer0(0);
    timerCounter++;
    currentTime++;
    if(currentTime==65535)currentTime=0;
    else currentTime++;
    if(timerCounter==10) //20sn 
    {
      LED=0;
      sleepState=1;
      startWrite();
      writeCommand(ST7789_SLPIN);
      endWrite();
      disable_interrupts(int_timer0);
      sleep();
    }
}
void main()
{

    setup_oscillator(OSC_HFINTRC_32MHZ|OSC_HFINTRC_ENABLED);
    setup_adc_ports(NO_ANALOGS);
    setup_adc(ADC_OFF);
    setup_spi(SPI_SS_DISABLED);
    setup_timer_0(RTCC_INTERNAL|RTCC_DIV_128);
    setup_timer_1(T1_DISABLED);
    setup_timer_2(T2_DISABLED, 0, 1);
    setup_timer_3(T3_DISABLED);
    setup_ccp1(CCP_OFF);
    setup_spi(SPI_MASTER | SPI_L_TO_H | SPI_XMIT_L_TO_H | SPI_CLK_DIV_4);
    setup_spi2(SPI_MASTER | SPI_H_TO_L | SPI_CLK_DIV_4);
    
    
    enable_interrupts(int_timer0);
    clear_interrupt(int_timer0);
    clear_interrupt(INT_EXT);                      // Clear external interrupt flag bit
    enable_interrupts(INT_EXT);                // Enable external interrupt
    enable_interrupts(GLOBAL);  

    TRISA = (0B11111111);
    TRISB = (0B00000000);
    TRISC = (0B00010001);
    TRISD = (0B01101100);
    TRISE = (0B00000111);
    PORTA = (0B00000000);
    PORTB = (0B00000000);
    PORTC = (0B00000000);
    PORTD = (0B00000000);
    PORTE = (0B00000000);

    eepromRead();
    tft_init();
    setRotation(1);
//!    startWrite();
//!    writeCommand(ST7789_WRCTRLD);
//!    spi_write2(0x00);
//    endWrite();

    display_fillScreen(ST7789_BLACK);
    mainTemplate();
    homePage();
    config_nrf24();
    setup_wdt(WDT_ON);
    setup_WDT(WDT_16S);
    //display_fillScreen(ST7789_BLACK);
    while (true)
    {
    
    }
}


//-------------------------BEGIN MAIN TEMPLATE (HEADER AND FOOTER DESIGN)--
void mainTemplate()
{

    display_fillRect(0, 0, 240, 130, ST7789_BLACK);
    display_fillRect(0, 0, 240, 20, ST7789_MAGENTA);
    display_fillRoundRect(0, 0, 240, 45, 10, ST7789_MAGENTA);
    display_setTextSize(4);
    display_setTextColor(ST7789_BLACK);
    display_setCursor(20, 8);
    display_print("RAYE");
    display_setTextColor(ST7789_WHITE);
    display_setTextSize(2);
    display_setCursor(120, 15);
    display_print("OTOMASYON");

    display_drawRect(10, 49, 224, 147, ST7789_MAGENTA);

    display_fillRoundRect(2, 200, 238, 40, 10, ST7789_CYAN);
    display_setCursor(10, 215);
    display_setTextSize(2);
    display_setTextColor(ST7789_BLACK);
    display_print("TEL:0(542) 673 7766");
}
//-------------------------END MAIN TEMPLATE (HEADER AND FOOTER DESIGN)----


//-------------------------BEGIN HOME PAGE (ALL VOLTAGES SHOW)-------------
void homePage()
{
     display_fillRect(12, 52, 220, 143, ST7789_BLACK);
 

    //----------------------------BEGIN OUTPUT 1------------
    display_setTextSize(2);
    display_setTextColor(ST7789_RED);
    display_setCursor(20, 62);
    display_print("OUTPUT 1=");
    printf(display_print, "  %f V", voltage1);
    display_drawLine(11, 85, 232, 85, ST7789_RED);
    //----------------------------END OUTPUT 1--------------

    //----------------------------BEGIN OUTPUT 2------------
    display_setTextColor(ST7789_YELLOW);
    display_setCursor(20, 96);
    display_print("OUTPUT 2=");
    printf(display_print, "  %f V", voltage2);
    display_drawLine(11, 121, 232, 121, ST7789_YELLOW);
    //----------------------------END OUTPUT 2--------------

    //----------------------------BEGIN OUTPUT 3------------
    display_setTextColor(ST7789_GREEN);
    display_setCursor(20, 132);
    display_print("OUTPUT 3=");
    printf(display_print, "  %f V", voltage3);
    display_drawLine(11, 157, 232, 157, ST7789_GREEN);
    //-----------------------------END OUTPUT 3--------------

    //----------------------------BEGIN OUTPUT 4------------
    display_setTextColor(ST7789_BLUE);
    display_setCursor(20, 171);
    display_print("OUTPUT 4=");
    printf(display_print, "  %f V", voltage4);
    //tft.drawLine(0,102, 160,102,ST77XX_BLUE);
    //----------------------------END OUTPUT 4------------
}
//-------------------------END HOME PAGE (ALL VOLTAGES SHOW)---------------


//-------------------------BEGIN VOLTAGE SET PAGE--------------------------
void setPage()
{
    if (setPageState == 0)
    {
        setPageState++;
        display_fillRect(12, 52, 220, 143, ST7789_BLACK);
        display_setTextColor(ST7789_GREEN);
        display_setTextSize(3);
        display_setCursor(50, 60);
        printf(display_print, "%s", mainMenu[currentRow]);
        display_drawLine(12, 90, 232, 90, ST7789_GREEN);
        display_setTextColor(ST7789_YELLOW);
        display_setCursor(20, 120);
        display_setTextSize(7);
        display_print("-");
        display_setCursor(190, 120);
        display_print("+");
    }
    display_fillRect(65, 100, 100, 70, ST7789_BLACK);
    display_setCursor(77, 130);
    display_setTextSize(4);
    display_setTextColor(ST7789_GREEN);
    if (currentRow == 0)
    {
        printf(display_print, "%f", voltage1);
    }
    if (currentRow == 1)
    {
        printf(display_print, "%f", voltage2);
    }
    if (currentRow == 2)
    {
        printf(display_print, "%f", voltage3);
    }
    if (currentRow == 3)
    {
        printf(display_print, "%f", voltage4);
    }
}
//-------------------------END VOLTAGE SET PAGE----------------------------


//-------------------------BEGIN MAIN MENU LIST----------------------------
//!void mainMenuList()
//!{
//!
//!    display_fillRect(10, 49, 224, 147, ST7789_BLACK);
//!    display_drawRect(10, 49, 224, 147, ST7789_MAGENTA);
//!    int rowYPixel = 60;
//!    char* rowValue = "                            ";
//!    char* scrolSymbol = ">>";
//!    display_setTextSize(2);
//!    display_setTextColor(ST7789_MAGENTA);
//!
//!    for (int i = 0; i < 4; i++)
//!    {
//!
//!        sprintf(rowValue, "%s", mainMenu[i]);
//!
//!        if (currentRow == i)
//!        {
//!            fillRect(12, rowYPixel - 10, 220, 35, ST7789_YELLOW);
//!            display_setTextColor(ST7789_BLACK);
//!            display_setCursor(25, rowYPixel);
//!            printf(display_print, "%s", scrolSymbol);
//!            display_setCursor(55, rowYPixel);
//!            printf(display_print, "%s", rowValue);
//!        }
//!        else
//!        {
//!            display_setTextColor(ST7789_CYAN);
//!            display_setCursor(25, rowYPixel);
//!            printf(display_print, "%s", rowValue);
//!        }
//!        rowYPixel += 35;
//!    }
//!}
//-------------------------END MAIN MENU LIST------------------------------


//-------------------------BEGIN EEPROM WRITE ALL VOTAGE CURRENT VALUE-----
void eepromWrite()
{ 
   for (int i = 0; i < 4; i++) write_eeprom(i +   eepromAddress1, *((int8*)&voltage1 + i) ) ;
   for (int i = 0; i < 4; i++) write_eeprom(i +   eepromAddress2, *((int8*)&voltage2 + i) ) ;
   for (int i = 0; i < 4; i++) write_eeprom(i +   eepromAddress3, *((int8*)&voltage3 + i) ) ;
   for (int i = 0; i < 4; i++) write_eeprom(i +   eepromAddress4, *((int8*)&voltage4 + i) ) ;
}
//-------------------------END EEPROM WRITE ALL VOTAGE CURRENT VALUE-------

//-------------------------BEGIN EEPROM WRITE ALL VOTAGE CURRENT VALUE-----
void eepromRead()
{ 
   if (read_eeprom(eepromAddress1)!=-1)
    {
      for (int i = 0; i < 4; i++) *((int8*)&voltage1 + i) = read_eeprom(i + eepromAddress1);
      for (int i = 0; i < 4; i++) *((int8*)&voltage2 + i) = read_eeprom(i + eepromAddress2);
      for (int i = 0; i < 4; i++) *((int8*)&voltage3 + i) = read_eeprom(i + eepromAddress3);
      for (int i = 0; i < 4; i++) *((int8*)&voltage4 + i) = read_eeprom(i + eepromAddress4);
    }
}
//-------------------------END EEPROM WRITE ALL VOTAGE CURRENT VALUE-------

//-------------------------BEGIN   SEND DATA FOR CHANGE VOLTAGE INFO--------

void sendDataForVoltage(int16 value)
{
       LED = 1;
       spi_write16(value);
       for(int i=0; i<50; i++) writeSendData();
       LED = 0;
       restart_wdt();
}
//-------------------------END  SEND DATA FOR CHANGE VOLTAGE INFO-----------

