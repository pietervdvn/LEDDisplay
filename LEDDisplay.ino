/***********************************************************************
  LED-Display for the VELO M2 PROJECT

  RED WIRE of the connect should be on the side of the USB connector of the arduino

  X is the long side
  Y is the short side


 ***********************************************************************/

#include "ht1632.h"


#define X_MAX 32
#define Y_MAX 16
#define CHIP_MAX 4 //Four HT1632Cs on one board
#define CLK_DELAY


// possible values for a pixel;
#define BLACK  0
#define GREEN  1
#define RED    2
#define ORANGE 3


#define cls          ht1632_clear


// our own copy of the "video" memory; 64 bytes for each of the 4 screen quarters;
// each 64-element array maps 2 planes:
// indexes from 0 to 31 are allocated for green plane;
// indexes from 32 to 63 are allocated for red plane;
// when a bit is 1 in both planes, it is displayed as orange (green + red);
byte ht1632_shadowram[64][4] = {0};



/*
   Set these constants to the values of the pins connected to the SureElectronics Module
*/
static const byte ht1632_data = 11;  // Data pin (pin 7 of display connector)
static const byte ht1632_wrclk = 10; // Write clock pin (pin 5 of display connector)
static const byte ht1632_cs = 8;    // Chip Select (pin 1 of display connnector)
static const byte ht1632_clk = 9; // clock pin (pin 2 of display connector)
static const byte ht1632_5v = 13;


//**************************************************************************************************
//Function Name: OutputCLK_Pulse
//Function Feature: enable CLK_74164 pin to output a clock pulse
//Input Argument: void
//Output Argument: void
//**************************************************************************************************
void OutputCLK_Pulse(void) //Output a clock pulse
{
  digitalWrite(ht1632_clk, HIGH);
  digitalWrite(ht1632_clk, LOW);
}


//**************************************************************************************************
//Function Name: OutputA_74164
//Function Feature: enable pin A of 74164 to output 0 or 1
//Input Argument: x: if x=1, 74164 outputs high. If x?1, 74164 outputs low.
//Output Argument: void
//**************************************************************************************************
void OutputA_74164(unsigned char x) //Input a digital level to 74164
{
  digitalWrite(ht1632_cs, (x == 1 ? HIGH : LOW));
}


//**************************************************************************************************
//Function Name: ChipSelect
//Function Feature: enable HT1632C
//Input Argument: select: HT1632C to be selected
// If select=0, select none.
// If s<0, select all.
//Output Argument: void
//**************************************************************************************************
void ChipSelect(int select)
{
  unsigned char tmp = 0;
  if (select < 0) //Enable all HT1632Cs
  {
    OutputA_74164(0);
    CLK_DELAY;
    for (tmp = 0; tmp < CHIP_MAX; tmp++)
    {
      OutputCLK_Pulse();
    }
  }
  else if (select == 0) //Disable all HT1632Cs
  {
    OutputA_74164(1);
    CLK_DELAY;
    for (tmp = 0; tmp < CHIP_MAX; tmp++)
    {
      OutputCLK_Pulse();
    }
  }
  else
  {
    OutputA_74164(1);
    CLK_DELAY;
    for (tmp = 0; tmp < CHIP_MAX; tmp++)
    {
      OutputCLK_Pulse();
    }
    OutputA_74164(0);
    CLK_DELAY;
    OutputCLK_Pulse();
    CLK_DELAY;
    OutputA_74164(1);
    CLK_DELAY;
    tmp = 1;
    for ( ; tmp < select; tmp++)
    {
      OutputCLK_Pulse();
    }
  }
}


/*
   ht1632_writebits
   Write bits (up to 8) to h1632 on pins ht1632_data, ht1632_wrclk
   Chip is assumed to already be chip-selected
   Bits are shifted out from MSB to LSB, with the first bit sent
   being (bits & firstbit), shifted till firsbit is zero.
*/
void ht1632_writebits (byte bits, byte firstbit)
{
  while (firstbit) {
    digitalWrite(ht1632_wrclk, LOW);
    if (bits & firstbit) {
      digitalWrite(ht1632_data, HIGH);
    }
    else {
      digitalWrite(ht1632_data, LOW);
    }
    digitalWrite(ht1632_wrclk, HIGH);
    firstbit >>= 1;
  }
}


/*
   ht1632_sendcmd
   Send a command to the ht1632 chip.
*/
static void ht1632_sendcmd (byte chipNo, byte command)
{
  ChipSelect(chipNo);
  ht1632_writebits(HT1632_ID_CMD, 1 << 2); // send 3 bits of id: COMMMAND
  ht1632_writebits(command, 1 << 7); // send the actual command
  ht1632_writebits(0, 1); 	/* one extra dont-care bit in commands. */
  ChipSelect(0);
}


/*
   ht1632_senddata
   send a nibble (4 bits) of data to a particular memory location of the
   ht1632.  The command has 3 bit ID, 7 bits of address, and 4 bits of data.
      Select 1 0 1 A6 A5 A4 A3 A2 A1 A0 D0 D1 D2 D3 Free
   Note that the address is sent MSB first, while the data is sent LSB first!
   This means that somewhere a bit reversal will have to be done to get
   zero-based addressing of words and dots within words.
*/
static void ht1632_senddata (byte chipNo, byte address, byte data)
{
  ChipSelect(chipNo);
  ht1632_writebits(HT1632_ID_WR, 1 << 2); // send ID: WRITE to RAM
  ht1632_writebits(address, 1 << 6); // Send address
  ht1632_writebits(data, 1 << 3); // send 4 bits of data
  ChipSelect(0);
}


void ht1632_setup()
{
  pinMode(ht1632_cs, OUTPUT);
  digitalWrite(ht1632_cs, HIGH); 	/* unselect (active low) */
  pinMode(ht1632_wrclk, OUTPUT);
  pinMode(ht1632_data, OUTPUT);
  pinMode(ht1632_clk, OUTPUT);

  for (int j = 1; j < 5; j++)
  {
    ht1632_sendcmd(j, HT1632_CMD_SYSDIS);  // Disable system
    ht1632_sendcmd(j, HT1632_CMD_COMS00);
    ht1632_sendcmd(j, HT1632_CMD_MSTMD); 	/* Master Mode */
    ht1632_sendcmd(j, HT1632_CMD_RCCLK);  // HT1632C
    ht1632_sendcmd(j, HT1632_CMD_SYSON); 	/* System on */
    ht1632_sendcmd(j, HT1632_CMD_LEDON); 	/* LEDs on */
  }

  for (byte i = 0; i < 96; i++)
  {
    ht1632_senddata(1, i, 0);  // clear the display!
    ht1632_senddata(2, i, 0);  // clear the display!
    ht1632_senddata(3, i, 0);  // clear the display!
    ht1632_senddata(4, i, 0);  // clear the display!
  }
  delay(1000);
}


/*
   plot a point on the display, with the upper left hand corner
   being (0,0), and the lower right hand corner being (31, 15);
   parameter "color" could have one of the 4 values:
   black (off), red, green or yellow;
*/
void ht1632_plot (byte x, byte y, byte color)
{
  if (x < 0 || x > X_MAX || y < 0 || y > Y_MAX)
    return;

  if (color != BLACK && color != GREEN && color != RED && color != ORANGE)
    return;

  byte nChip = 1 + x / 16 + (y > 7 ? 2 : 0) ;
  x = x % 16;
  y = y % 8;
  byte addr = (x << 1) + (y >> 2);
  byte bitval = 8 >> (y & 3); // compute which bit will need set
  switch (color)
  {
    case BLACK:
      // clear the bit in both planes;
      ht1632_shadowram[addr][nChip - 1] &= ~bitval;
      ht1632_senddata(nChip, addr, ht1632_shadowram[addr][nChip - 1]);
      addr = addr + 32;
      ht1632_shadowram[addr][nChip - 1] &= ~bitval;
      ht1632_senddata(nChip, addr, ht1632_shadowram[addr][nChip - 1]);
      break;
    case GREEN:
      // set the bit in the green plane and clear the bit in the red plane;
      ht1632_shadowram[addr][nChip - 1] |= bitval;
      ht1632_senddata(nChip, addr, ht1632_shadowram[addr][nChip - 1]);
      addr = addr + 32;
      ht1632_shadowram[addr][nChip - 1] &= ~bitval;
      ht1632_senddata(nChip, addr, ht1632_shadowram[addr][nChip - 1]);
      break;
    case RED:
      // clear the bit in green plane and set the bit in the red plane;
      ht1632_shadowram[addr][nChip - 1] &= ~bitval;
      ht1632_senddata(nChip, addr, ht1632_shadowram[addr][nChip - 1]);
      addr = addr + 32;
      ht1632_shadowram[addr][nChip - 1] |= bitval;
      ht1632_senddata(nChip, addr, ht1632_shadowram[addr][nChip - 1]);
      break;
    case ORANGE:
      // set the bit in both the green and red planes;
      ht1632_shadowram[addr][nChip - 1] |= bitval;
      ht1632_senddata(nChip, addr, ht1632_shadowram[addr][nChip - 1]);
      addr = addr + 32;
      ht1632_shadowram[addr][nChip - 1] |= bitval;
      ht1632_senddata(nChip, addr, ht1632_shadowram[addr][nChip - 1]);
      break;
  }
}


void plot_test (int chipNo, byte addr, byte val)
{
  ht1632_shadowram[addr][0] = val;
  // Now copy the new memory value to the display
  ht1632_senddata(chipNo, addr, ht1632_shadowram[addr][0]);
}


/*
   return the value of a pixel from the video memory (either BLACK, RED, GREEN, ORANGE);
*/
byte get_shadowram(byte x, byte y)
{
  byte nQuarter = x / 16 + (y > 7 ? 2 : 0) ;
  x = x % 16;
  y = y % 8;
  byte addr = (x << 1) + (y >> 2);
  byte bitval = 8 >> (y & 3);
  byte retVal = 0;
  byte val = (ht1632_shadowram[addr][nQuarter] & bitval) ? 1 : 0;
  val += (ht1632_shadowram[addr + 32][nQuarter] & bitval) ? 2 : 0;
  return val;
}


/*
   ht1632_clear
   clear the display, and the shadow memory, and the snapshot
   memory.  This uses the "write multiple words" capability of
   the chipset by writing all 96 words of memory without raising
   the chipselect signal.
*/
void ht1632_clear()
{
  char i;
  for (int i = 1; i < 5; i++)
  {
    ChipSelect(-1);
    ht1632_writebits(HT1632_ID_WR, 1 << 2); // send ID: WRITE to RAM
    ht1632_writebits(0, 1 << 6); // Send address
    for (i = 0; i < 96 / 2; i++) // Clear entire display
      ht1632_writebits(0, 1 << 7); // send 8 bits of data
    ChipSelect(0);

    for (int j = 0; j < 64; j++)
      ht1632_shadowram[j][i - 1] = 0;
  }
}




/*
   snapshot_shadowram; used in Game of Life;
   Copy the shadow ram into the snapshot ram (the upper bits)
   This gives us a separate copy so we can plot new data while
   still having a copy of the old data.  snapshotram is NOT
   updated by the plot functions (except "clear")
*/
void snapshot_shadowram()
{
  for (int nQuarter = 0; nQuarter < 4; nQuarter++)
  {
    for (byte i = 0; i < 64; i++)
    {
      // copy the video bits (lower 4) in the upper 4;
      byte val = ht1632_shadowram[i][nQuarter];
      val = (val & 0x0F) + (val << 4);
      ht1632_shadowram[i][nQuarter] = val;
    }
  }
}


/*
   get_snapshotram
   get a pixel value from the snapshot ram instead of the actual video memory;
   return BLACK, GREEN, RED or ORANGE;
*/
byte get_snapshotram(byte x, byte y)
{
  byte nQuarter = x / 16 + (y > 7 ? 2 : 0); // 0..3;
  x = x % 16;
  y = y % 8;
  byte addr = (x << 1) + (y >> 2);
  byte bitval = 8 >> (y & 3); // compute the required bit;

  byte greenByte = ht1632_shadowram[addr]   [nQuarter] >> 4;
  byte redByte   = ht1632_shadowram[addr + 32][nQuarter] >> 4;

  byte retVal = 0;
  byte val = (greenByte & bitval) ? 1 : 0;
  val += (redByte & bitval) ? 2 : 0;
  return val;
}



/***********************************************************************
   traditional Arduino sketch functions: setup and loop.
 ***********************************************************************/

void setup ()  // flow chart from page 17 of datasheet
{
  pinMode(ht1632_5v, OUTPUT);
  digitalWrite(ht1632_5v, HIGH); // 5V reference for the board
  ht1632_setup();
  randomSeed(0);
  cls;
}


void plot_rect(int x_start, int y_start, int x_end, int y_end, int color) {
  for (int x = x_start; x <= x_end; x++) {
    for (int y = y_start; y <= y_end; y++) {
      plot(x, y, color);
    }
  }
}

// DIRTY HACK
int x_offset = 0; 
int y_offset = 0;
void plot(int x, int y, int color){
  ht1632_plot(x + x_offset, y + y_offset, color);
}



// --------------------------- LETTERS ------------------------------








const int green_min = 5;
const int orange_min = 0;
const int red_min = 11;



void vertical_bar(int x, int w, int value) {
  w--;
  if (value == 0) {
    plot_rect(x, Y_MAX - 1, x + w, Y_MAX - 1, RED);
    plot_rect(x, 0, x + w, Y_MAX - 2, BLACK);
    return;
  }

  int color = ORANGE;
  if (value >= green_min) {
    color = GREEN;
  }
  if (value >= red_min) {
    color = RED;
  }

  plot_rect(x, 0, x + w, Y_MAX - 1 - value, BLACK);
  plot_rect(x, Y_MAX - 1 - value, x + w, Y_MAX - 1, color);

}

// Prins a char, returns the used width of the char
byte print_char(char chr, int c) {
  int w = 3;
  switch (chr) {
    case 'A':
      plot_rect(0, 1, 0, 6, c);
      plot_rect(3, 1, 3, 6, c);
      plot_rect(1, 0, 2, 0, c);
      plot_rect(1, 3, 2, 3, c);
      break;
    case 'B':
      plot_rect(0, 0, 0, 6, c);
      plot_rect(1, 0, 2, 0, c);
      plot_rect(1, 3, 2, 3, c);
      plot_rect(1, 6, 2, 6, c);
      plot_rect(3, 1, 3, 2, c);
      plot_rect(3, 4, 3, 5, c);
      break;
    case 'C':
      plot_rect(0, 1, 0, 5, c);
      plot_rect(1, 0, 2, 0, c);
      plot_rect(1, 6, 2, 6, c);
      plot(3, 1, c);
      plot(3, 5, c);
      break;
    case 'D':
      plot_rect(0, 0, 0, 6, c);
      plot_rect(1, 0, 2, 0, c);
      plot_rect(1, 6, 2, 6, c);
      plot_rect(3, 1, 3, 5, c);
      break;
    case 'E':
      plot_rect(0, 0, 0, 6, c);
      plot_rect(1, 0, 3, 0, c);
      plot_rect(1, 3, 2, 3, c);
      plot_rect(1, 6, 3, 6, c);
      break;
    case 'F':
      plot_rect(0, 0, 0, 6, c);
      plot_rect(1, 0, 3, 0, c);
      plot_rect(1, 3, 2, 3, c);
      break;
    case 'G':
      plot_rect(0, 1, 0, 5, c);
      plot_rect(1, 0, 2, 0, c);
      plot_rect(1, 6, 2, 6, c);
      plot(3, 1, c);
      plot_rect(3, 3, 3, 6, c);
      plot(2, 3, c);
      break;
    case 'H':
      plot_rect(0, 0, 0, 6, c);
      plot_rect(3, 0, 3, 6, c);
      plot_rect(1, 3, 2, 3, c);
      break;
    case 'I':
      plot_rect(0, 0, 2, 0, c);
      plot_rect(0, 6, 2, 6, c);
      plot_rect(1, 1, 1, 5, c);
      w = 2;
      break;

    case 'J':
      plot_rect(1, 0, 3, 0, c);
      plot_rect(1, 6, 2, 6, c);
      plot_rect(3, 1, 3, 5, c);
      plot(0, 5, c);
      break;

    case 'K':
      plot_rect(0, 0, 0, 6, c);
      plot_rect(3, 0, 3, 1, c);
      plot_rect(3, 5, 3, 6, c);
      plot(1, 3, c);
      plot(2, 2, c);
      plot(2, 4, c);
      break;
    case 'L':
      plot_rect(0, 0, 0, 6, c);
      plot_rect(1, 6, 3, 6, c);
      plot(3, 5, c);
      break;
    case 'M':
      plot_rect(0, 0, 0, 6, c);
      plot_rect(4, 0, 4, 6, c);
      plot(1, 1, c);
      plot(2, 2, c);
      plot(3, 1, c);
      w = 4;
      break;
    case 'N':
      plot_rect(0, 0, 0, 6, c);
      plot_rect(3, 0, 3, 6, c);
      plot(1, 1, c);
      plot(2, 2, c);
      break;
    case 'O':
      plot_rect(0, 1, 0, 5, c);
      plot_rect(3, 1, 3, 5, c);
      plot_rect(1, 0, 2, 0, c);
      plot_rect(1, 6, 2, 6, c);
      break;
    case 'P':
      plot_rect(0, 0, 0, 6, c);
      plot_rect(3, 1, 3, 2, c);
      plot_rect(1, 0, 2, 0, c);
      plot_rect(1, 3, 2, 3, c);
      break;
    case 'Q':
      plot_rect(0, 1, 0, 4, c);
      plot_rect(3, 1, 3, 4, c);
      plot_rect(1, 0, 2, 0, c);
      plot_rect(1, 5, 2, 5, c);
      plot(3,6, c);
      break;
   case 'R':
      plot_rect(0, 0, 0, 6, c);
      plot_rect(3, 1, 3, 2, c);
      plot_rect(1, 0, 2, 0, c);
      plot_rect(1, 3, 2, 3, c);
      plot_rect(3, 4, 3, 6, c);
      break;
   case 'S':
      plot_rect(0, 1, 0, 2, c);
      plot_rect(1, 0, 3, 0, c);
      plot_rect(1, 3, 2, 3, c);
      plot_rect(0, 6, 2, 6, c);
      plot_rect(3,4,3,5,c);
      break;
   case 'T':
      plot_rect(0, 0, 4, 0, c);
      plot_rect(2, 1, 2, 6, c);
      w = 4;
      break;
   case 'U':
      plot_rect(0,0,0,5,c);
      plot_rect(3,0,3,5,c);
      plot_rect(1,6,2,6,c);
      break;
    case 'V':
      plot_rect(0,0,0,3,c);
      plot_rect(4,0,4,3,c);
      plot(2,6,c);
      plot(1,5,c);
      plot(1,4,c);
      plot(3,4,c);
      plot(3,5,c);
      w = 4;
      break;
    case 'W':
      plot_rect(0,0,0,6,c);
      plot_rect(4,0,4,6,c);
      plot(1,5,c);
      plot(2,4,c);
      plot(3,5,c);
      w = 4;
      break;
    case 'X':
      plot(3,4,c);
      plot_rect(4,5,4,6,c);
      // NO BREAK HERE
    case 'Y':
      plot(2,3,c);
      plot(1,2,c);
      plot_rect(0,0,0,1,c);
      plot(3,2,c);
      plot_rect(4,0,4,1,c);
      plot(1,4,c);
      plot_rect(0,5,0,6,c);
      w = 4;
      break;
    case 'Z':
      plot_rect(0,0,3,0,c);
      plot(0,5,c);
      plot(1,4,c);
      plot(2,3,c);
      plot(3,2,c);
      plot(3,1,c);
      plot_rect(0,6,3,6,c);
      break;
    case '.':
      plot_rect(0,4,1,5,c);
      w = 1;
      break;
    case ',':
      plot_rect(0,5,1,6,c);
      w = 1;
      break;
    case '0':
      plot_rect(0, 1, 0, 5, c);
      plot_rect(3, 1, 3, 5, c);
      plot_rect(1, 0, 2, 0, c);
      plot_rect(1, 6, 2, 6, c);
      plot(1,4,c);
      plot(2,2,c);
      break;
    case '1':
      //print_char('I', c); // TODO render properly
      plot_rect(1,0,1,5,c);
      plot(0,1,c);
      plot_rect(0,6,2,6,c);
      break;

    case '2':
      plot_rect(1,0,2,0,c);
      plot(0,1,c);
      plot_rect(3,1,3,2,c);
      plot(2,3,c);
      plot(1,4,c);
      plot(0,5,c);
      plot_rect(0,6,3,6,c);
      plot(0,1,c);
      break;
      
    case '3':
      plot_rect(1,0,2,0,c);
      plot(0,1,c);
      plot_rect(3,1,3,2,c);
      plot(2,3,c);
      plot_rect(3,4,3,5,c);
      plot(0,5,c);
      plot_rect(1,6,2,6,c);
      break;
      
    case '4':
      plot_rect(0,3,2,3,c);
      plot(2,0,c);
      plot(1,1,c);
      plot(0,2,c);
      plot_rect(3,0,3,6,c);
      break;

    case '5':
      plot_rect(0,0,3,0,c);
      plot(0,1,c);
      plot_rect(0,2,2,2,c);
      plot_rect(3,3,3,5,c);
      plot(0,5,c);
      plot(0,3,c);
      plot_rect(1,6,2,6,c);
      break;            

    case '6':
      plot_rect(1,0,2,0,c);
      plot_rect(0,1,0,5,c);
      plot_rect(1,2,2,2,c);
      plot_rect(3,3,3,5,c);
      plot_rect(1,6,2,6,c);
      break;   

    case '7':
      plot_rect(0,0,3,0,c);
      plot_rect(3,1,3,2,c);
      plot(2,3,c);
      plot(1,4,c);
      plot(0,5,c);
      plot(0,6,c);     
      plot(0,1,c);
      break;

    case '8':
      plot_rect(1,0,2,0,c);
      plot_rect(0,1,0,2,c);
      plot_rect(3,1,3,2,c);
      plot_rect(1,3,2,3,c);
      plot_rect(3,4,3,5,c);
      plot_rect(0,4,0,5,c);
      plot_rect(1,6,2,6,c);
      break;
 
    case '9':
      plot_rect(1,0,2,0,c);
      plot_rect(0,1,0,3,c);
      plot_rect(3,1,3,5,c);
      plot_rect(1,4,2,4,c);
      plot_rect(1,6,2,6,c);
      break;
      
      break;
    default: plot(X_MAX-1, Y_MAX-1, RED); return 0; // will return 0
    
  }


  return w + 1;

}


int plot_char(char chr, int x, int y, int c){
  int w;
  x_offset = x;
  y_offset = y;

  w = print_char(chr, c);
  
  x_offset = 0;
  y_offset = 0;
  return w;
}

void clearScr(){
  plot_rect(0,0,X_MAX-1, Y_MAX-1, BLACK);
}


char intToChar(int i){
  return i + 48;  
}

int plot_int(int i, int x, int y, byte number_of_chars, int color){
  if(number_of_chars == 0){
    return 0;
  }
  int last = i % 10;
  byte x_offs = (number_of_chars-1) * 5;
  plot_char(intToChar(last), x + x_offs, y, color);

  plot_int(i / 10, x, y, number_of_chars-1, color);
  return 5 * number_of_chars;
}


void loop ()
{
   // cls; // clear screen
   // plot(x, y, color); put a certain pixel on (BLACK = OFF)
   // plot_rect(x_left_upper, y_left_upper, x_right_down, y_right_down, color); // fill a rectangle
   // vertical_bar(x_position, width, height); colored bard vertically (Orange - green - red)
   // plot_char('A', x, y, color); // charachters should be uppercase! x, y = upper left corner; returns the width of the character plotted
   // plot_int( number_to_display, x, y, number_of_chars, color); if to little space to display: drops higher numbers

/*
plot_char('8',0,0,GREEN);
delay(2500);
plot_char('8',0,0,BLACK);
plot_char('6',0,0,GREEN);
delay(2500);
plot_char('6',0,0,BLACK);
plot_char('9',0,0,GREEN);
delay(2500);
plot_char('9',0,0,BLACK);
*/


  // read the input on analog pin 0:
  int sensorValue = analogRead(A0);
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  float fvoltage = sensorValue * (5.0 / 1023);
  int ufvoltage = fvoltage;
  float dvoltage = fvoltage - ufvoltage;
  int voltage = dvoltage*100+0.5;

  float fwatt = fvoltage*10;
  int ufwatt = fwatt;
  float dfwatt = fwatt - ufwatt;
  
  plot_int(ufvoltage,0,0,2,GREEN);
  plot_char(',',10,0,GREEN);
  plot_int(voltage,13,0,2,GREEN);
  plot_char('V',24,0,GREEN);

  plot_int(ufwatt,0,8,3,RED);
  plot_char(',',10,8,RED);
  plot_int(dfwatt,13,8,1,RED);
  plot_char('W',24,8,RED);
  
  delay(250);
  plot_int(ufvoltage,0,0,2,BLACK);
  plot_int(voltage,13,0,2,BLACK);
  
  
  
  /*
  for(char c = '0'; c <= '9'; c ++){
    plot_char(c, 0, 0, GREEN);
    delay(250);
    plot_char(c, 0, 0, BLACK);
    
  }
  */
  





 
}
