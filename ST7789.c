///////////////////////////////////////////////////////////////////////////
////                                                                   ////
////                              ST7789_.c                             ////
////                                                                   ////
////               ST7789_ display driver for CCS C compiler            ////
////                                                                   ////
///////////////////////////////////////////////////////////////////////////
////                                                                   ////
////               This is a free software with NO WARRANTY.           ////
////                     https://simple-circuit.com/                   ////
////                                                                   ////
///////////////////////////////////////////////////////////////////////////
/**************************************************************************
  This is a library for several Adafruit displays based on ST77* drivers.

  Works with the Adafruit 1.8" TFT Breakout w/SD card
    ----> http://www.adafruit.com/products/358
  The 1.8" TFT shield
    ----> https://www.adafruit.com/product/802
  The 1.44" TFT breakout
    ----> https://www.adafruit.com/product/2088
  as well as Adafruit raw 1.8" TFT display
    ----> http://www.adafruit.com/products/618

  Check out the links above for our tutorials and wiring diagrams.
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional).

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 **************************************************************************/

#include <stdint.h>
#ifndef bool
#define bool int1
#endif

#define ST_CMD_DELAY      0x80    // special signifier for command lists

#define ST7789_TFTWIDTH    240
#define ST7789_TFTHEIGHT    240

#define ST7789_240x240_XSTART 0
#define ST7789_240x240_YSTART 80

#define ST7789_NOP     0x00
#define ST7789_SWRESET 0x01
#define ST7789_RDDID   0x04
#define ST7789_RDDST   0x09

#define ST7789_SLPIN   0x10
#define ST7789_SLPOUT  0x11
#define ST7789_PTLON   0x12
#define ST7789_NORON   0x13

#define ST7789_INVOFF  0x20
#define ST7789_INVON   0x21
#define ST7789_DISPOFF 0x28
#define ST7789_DISPON  0x29
#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_RAMWR   0x2C
#define ST7789_RAMRD   0x2E

#define ST7789_PTLAR   0x30
#define ST7789_COLMOD  0x3A
#define ST7789_MADCTL  0x36

#define ST7789_MADCTL_MY  0x80
#define ST7789_MADCTL_MX  0x40
#define ST7789_MADCTL_MV  0x20
#define ST7789_MADCTL_ML  0x10
#define ST7789_MADCTL_RGB 0x00

#define ST7789_RDID1   0xDA
#define ST7789_RDID2   0xDB
#define ST7789_RDID3   0xDC
#define ST7789_RDID4   0xDD

// Color definitions
#define   ST7789_BLACK   0x0000
#define   ST7789_BLUE    0x001F
#define   ST7789_RED     0xF800
#define   ST7789_GREEN   0x07E0
#define   ST7789_CYAN    0x07FF
#define   ST7789_MAGENTA 0xF81F
#define   ST7789_YELLOW  0xFFE0
#define   ST7789_WHITE   0xFFFF
//!
//!#ifndef spi_write2
//!#define spi_write2(x) SPI_XFER(ST7789_, x)
//!#endif


// SCREEN INITIALIZATION ***************************************************

// Rather than a bazillion writecommand() and writedata() calls, screen
// initialization commands and arguments are organized in these tables
// stored in PROGMEM.  The table may look bulky, but that's mostly the
// formatting -- storage-wise this is hundreds of bytes more compact
// than the equivalent code.  Companion function follows.
uint8_t  _colstart, _rowstart, _xstart, _ystart,rotation; 
int16 _height=240,_width = 240;
rom uint8_t
  cmd_240x240[] = {                       // Initialization commands for 7789 screens
    10,                                   // 9 commands in list:
    ST7789_SWRESET,   ST_CMD_DELAY,        // 1: Software reset, no args, w/delay
      150,                                 // 150 ms delay
    ST7789_SLPOUT ,   ST_CMD_DELAY,        // 2: Out of sleep mode, no args, w/delay
      255,                                // 255 = 500 ms delay
    ST7789_COLMOD , 1+ST_CMD_DELAY,        // 3: Set color mode, 1 arg + delay:
      0x55,                               // 16-bit color
      10,                                 // 10 ms delay
    ST7789_MADCTL , 1,                 // 4: Memory access ctrl (directions), 1 arg:
      0x00,                               // Row addr/col addr, bottom to top refresh
    ST7789_CASET  , 4,                 // 5: Column addr set, 4 args, no delay:
      0x00, ST7789_240x240_XSTART,          // XSTART = 0
     (ST7789_TFTWIDTH+ST7789_240x240_XSTART) >> 8,
     (ST7789_TFTWIDTH+ST7789_240x240_XSTART) & 0xFF,   // XEND = 240
    ST7789_RASET  , 4,                 // 6: Row addr set, 4 args, no delay:
      0x00, ST7789_240x240_YSTART,          // YSTART = 0
      (ST7789_TFTHEIGHT+ST7789_240x240_YSTART) >> 8,
     (ST7789_TFTHEIGHT+ST7789_240x240_YSTART) & 0xFF,   // YEND = 240
    ST7789_INVON ,   ST_CMD_DELAY,        // 7: Inversion ON
      10,
    ST7789_NORON  ,   ST_CMD_DELAY,        // 8: Normal display on, no args, w/delay
      10,                                 // 10 ms delay
    ST7789_DISPON ,   ST_CMD_DELAY,        // 9: Main screen turn on, no args, w/delay
    255 };                  

//*************************** User Functions ***************************//
void tft_init(void);

void drawPixel(uint8_t x, uint8_t y, uint16_t color);
void drawHLine(uint8_t x, uint8_t y, uint8_t w, uint16_t color);
void drawVLine(uint8_t x, uint8_t y, uint8_t h, uint16_t color);
void fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);
void fillScreen(uint16_t color);
void setRotation(uint8_t m);
void invertDisplay(bool i);
void pushColor(uint16_t color);

//************************* Non User Functions *************************//
void startWrite(void);
void endWrite(void);
void displayInit(rom uint8_t *addr);
void writeCommand(uint8_t cmd);
void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/**************************************************************************/
/*!
    @brief  Call before issuing command(s) or data to display. Performs
            chip-select (if required). Required
            for all display types; not an SPI-specific function.
*/
/**************************************************************************/
void startWrite(void) {
  #ifdef TFT_CS
    output_low(TFT_CS);
  #endif
}

/**************************************************************************/
/*!
    @brief  Call after issuing command(s) or data to display. Performs
            chip-deselect (if required). Required
            for all display types; not an SPI-specific function.
*/
/**************************************************************************/
void endWrite(void) {
  #ifdef TFT_CS
    output_high(TFT_CS);
  #endif
}

/**************************************************************************/
/*!
    @brief  Write a single command byte to the display. Chip-select and
            transaction must have been previously set -- this ONLY sets
            the device to COMMAND mode, issues the byte and then restores
            DATA mode. There is no corresponding explicit writeData()
            function -- just use spi_write2().
    @param  cmd  8-bit command to write.
*/
/**************************************************************************/
void writeCommand(uint8_t cmd) {
  output_low(TFT_DC);
  spi_write2(cmd);
  output_high(TFT_DC);
}

/**************************************************************************/
/*!
    @brief  Companion code to the initiliazation tables. Reads and issues
            a series of LCD commands stored in ROM byte array.
    @param  addr  Flash memory array with commands and data to send
*/
/**************************************************************************/
void displayInit(rom uint8_t *addr){
  uint8_t  numCommands, numArgs;
  uint16_t ms;
  startWrite();

  numCommands = *addr++;   // Number of commands to follow
  
  while(numCommands--) {                 // For each command...

    writeCommand(*addr++); // Read, issue command
    numArgs  = *addr++;    // Number of args to follow
    ms       = numArgs & ST_CMD_DELAY;   // If hibit set, delay follows args
    numArgs &= ~ST_CMD_DELAY;            // Mask out delay bit
    while(numArgs--) {                   // For each argument...
      spi_write2(*addr++);   // Read, issue argument
    }

    if(ms) {
      ms = *addr++; // Read post-command delay time (ms)
      if(ms == 255) ms = 500;     // If 255, delay for 500 ms
      delay_ms(ms);
    }
  }
  endWrite();
}

/**************************************************************************/
/*!
    @brief  Initialization code for ST7789_ display
*/
/**************************************************************************/
void tft_init(void) {
_ystart = _xstart = 0;
  _colstart  = _rowstart = 0; // May be overridden in init func

  #ifdef TFT_RST
    output_high(TFT_RST);
    output_drive(TFT_RST);
    delay_ms(100);
    output_low(TFT_RST);
    delay_ms(100);
    output_high(TFT_RST);
    delay_ms(200);
  #endif

  #ifdef TFT_CS
    output_high(TFT_CS);
    output_drive(TFT_CS);
  #endif

  output_drive(TFT_DC);

  displayInit(cmd_240x240);

  _colstart = ST7789_240x240_XSTART;
  _rowstart = ST7789_240x240_YSTART;
  _height = 240;
  _width = 240;
  setRotation(2);
}

/**************************************************************************/
/*!
  @brief  SPI displays set an address window rectangle for blitting pixels
  @param  x  Top left corner x coordinate
  @param  y  Top left corner x coordinate
  @param  w  Width of window
  @param  h  Height of window
*/
/**************************************************************************/
void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1){
  uint16_t x_start = x0 + _xstart, x_end = x1 + _xstart;
  uint16_t y_start = y0 + _ystart, y_end = y1 + _ystart;

  writeCommand(ST7789_CASET); // Column addr set
  spi_write2(x_start >> 8);
  spi_write2(x_start & 0xFF);     // XSTART 
  spi_write2(x_end >> 8);
  spi_write2(x_end & 0xFF);     // XEND

  writeCommand(ST7789_RASET); // Row addr set
  spi_write2(y_start >> 8);
  spi_write2(y_start & 0xFF);     // YSTART
  spi_write2(y_end >> 8);
  spi_write2(y_end & 0xFF);     // YEND

  writeCommand(ST7789_RAMWR); // write to RAM
}

/**************************************************************************/
/*!
    @brief  Set origin of (0,0) and orientation of TFT display
    @param  m  The index for rotation, from 0-3 inclusive
*/
/**************************************************************************/
void setRotation(uint8_t m) {
  uint8_t madctl = 0;

   rotation = m & 3; // can't be higher than 3

  switch (rotation) {
  case 0:
     madctl=(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);

     _xstart = _colstart;
     _ystart = _rowstart;
     break;
   case 1:
     madctl=(ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);

     _ystart = _colstart;
     _xstart = _rowstart;
     break;
  case 2:
     madctl=(ST7789_MADCTL_RGB);
 
     _xstart = _colstart;
     _ystart = _rowstart;
     break;

   case 3:
     madctl=(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);

     _ystart = _colstart;
     _xstart = _rowstart;
     break;
  }
  startWrite();
  writeCommand(ST7789_MADCTL);
  spi_write2(madctl);
  endWrite();
}

void drawPixel(uint8_t x, uint8_t y, uint16_t color) {
 if(!((x < 0) ||(x >= _width) || (y < 0) || (y >= _height)))
 {
    startWrite();
    setAddrWindow(x,y,x+1,y+1);
    spi_write2(color >> 8);
    spi_write2(color & 0xFF);
    endWrite();
  }
}

/**************************************************************************/
/*!
   @brief    Draw a perfectly horizontal line (this is often optimized in a subclass!)
    @param    x   Left-most x coordinate
    @param    y   Left-most y coordinate
    @param    w   Width in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void drawHLine(uint8_t x, uint8_t y, uint8_t w, uint16_t color) {
  if( (x < _width) && (y < _height) && w) {   
    if((x+w-1) >= _width)  w = _width-x;
 
  uint8_t hi = color >> 8, lo = color;
    startWrite();
     setAddrWindow(x, y, x+w-1, y);
    while (w--) {
      spi_write2(hi);
      spi_write2(lo);
    }
    endWrite();
  }
}

/**************************************************************************/
/*!
   @brief    Draw a perfectly vertical line (this is often optimized in a subclass!)
    @param    x   Top-most x coordinate
    @param    y   Top-most y coordinate
    @param    h   Height in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void drawVLine(uint8_t x, uint8_t y, uint8_t h, uint16_t color) {
  if( (x < _width) && (y < _height) && h) {  
    if((x >= _width) || (y >= _height)) return;
  if((y+h-1) >= _height) h = _height-y;
  uint8_t hi = color >> 8, lo = color;
    startWrite();
    setAddrWindow(x, y, x, y+h-1);
    while (h--) {
      spi_write2(hi);
      spi_write2(lo);
    }
    endWrite();
  }
}

/**************************************************************************/
/*!
   @brief    Fill a rectangle completely with one color. Update in subclasses if desired!
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color) {
  if(w && h) {                            // Nonzero width and height?  
    uint8_t hi = color >> 8, lo = color;
if((x >= _width) || (y >= _height)) return;
  if((x + w - 1) >= _width)  w = _width  - x;
  if((y + h - 1) >= _height) h = _height - y;

  
    startWrite();
setAddrWindow(x, y, x+w-1, y+h-1);
    uint16_t px = (uint16_t)w * h;
    while (px--) {
      spi_write2(hi);
      spi_write2(lo);
    }
    endWrite();
  }
}

/**************************************************************************/
/*!
   @brief    Fill the screen completely with one color. Update in subclasses if desired!
    @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void fillScreen(uint16_t color) {
    fillRect(0, 0, _width, _height, color);
}

/**************************************************************************/
/*!
    @brief  Invert the colors of the display (if supported by hardware).
            Self-contained, no transaction setup required.
    @param  i  true = inverted display, false = normal display.
*/
/**************************************************************************/
void invertDisplay(bool i) {
    startWrite();
    writeCommand(i ? ST7789_INVON : ST7789_INVOFF);
    endWrite();
}

/*!
    @brief  Essentially writePixel() with a transaction around it. I don't
            think this is in use by any of our code anymore (believe it was
            for some older BMP-reading examples), but is kept here in case
            any user code relies on it. Consider it DEPRECATED.
    @param  color  16-bit pixel color in '565' RGB format.
*/
void pushColor(uint16_t color) {
    uint8_t hi = color >> 8, lo = color;
    startWrite();
    spi_write2(hi);
    spi_write2(lo);
    endWrite();
}

// end of code.

