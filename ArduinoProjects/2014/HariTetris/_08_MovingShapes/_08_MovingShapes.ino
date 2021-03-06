
// Towards Arduino Tetris

// v0.8 - 
// v0.7 - Finally, draw Tetris shapes!

// The 16x16 bitmap.  Each array element is a row (16 pixels on the X axis). 0,0 is bottom left of LED matrix.
int bitmap[16]; 

//-- Shift register pins --
const byte Row_RCLK = 11;   // Register Clock: Positive edge triggers shift register to output (White wire)
const byte Row_SRCLK = 12;  // Shift Register Clock: Positive edge triggers SER input into shift register (Yellow wire)
const byte Row_SER = 13;    // Serial input, read in on positive edge of SRCLK (Blue wire)

const byte Col_RCLK =5;   // Register Clock: Positive edge triggers shift register to output (White wire)
const byte Col_SRCLK = 6;  // Shift Register Clock: Positive edge triggers SER input into shift register (Yellow wire)
const byte Col_SER = 7;    // Serial input, read in on positive edge of SRCLK (Blue wire)

int twoBytes;
int cycles = 0;
byte currentRotation = 0; // 0..3 clockwise rotation
byte currentX = 6;
byte currentY = 16; // 16 instead of 15 because we'll decremented before using this variable.

//=== Tetris Shapes ===
// Each row of this array represents one 4x4 shape in all its four rotations.
// I used a spreadsheet to convert bitmap bit patterns into the hex numbers below.
int shapes[][4] = {
  { 0x0000, 0x4404, 0xE6EC, 0x444},   // 0. 
  { 0x0000, 0x0808, 0x6C6C, 0xC4C4},  // 1
  { 0x0000, 0x0404, 0xCCCC, 0x6868},  // 2
  { 0x0000, 0x40C0, 0x4E42, 0x684E},  // 3
  { 0x0000, 0x2060, 0x284E, 0x6E42},  // 4
  { 0x0000 ,0x0000, 0x6666, 0x6666},  // 5
  { 0x4040, 0x4040, 0x4F4F, 0x4040},
  { 0x0001, 0x0022, 0x0444, 0x8888},
  { 0x0000, 0x0000, 0x000, 0x8CEF}
};

void DrawShape(byte whichShape, byte whichRotation, byte xOffset, byte yOffset) {
  for (byte r=0; r<4; r++) {
    unsigned int shape = 0xF000 & (shapes[whichShape][r] << (4*whichRotation) );
    shape = shape >> (xOffset);
    byte bitmapY = 3-r+yOffset;
    //if (bitmapY>=0 && bitmapY<=15)
      bitmap[bitmapY] = bitmap[bitmapY] | shape;
  }
}

// Shows all the bit patterns for a shape
//void DumpShape(byte whichShape, byte vertOffset) {
//  for (byte r=0; r<4; r++) {
//    bitmap[3-r+vertOffset] = shapes[whichShape][r];    
//  }
//}

void UpdateBitmap() {
  int sensorValue = analogRead(A5);
// Y min-max
//  int minValue = 220; // Vertical limit
//  int maxValue = 320; // Horizontal limit
//-- X min max --
  int minValue = 335;
  int maxValue = 395;
  int xOffset = map(sensorValue, minValue, maxValue, 0,15-4);

  ClearBitmap();

//  DumpShape(7,0);
//  DumpShape(6,4);
//  DumpShape(5,8);
//  DumpShape(4,12);
//  DumpShape(3,0);
//  DumpShape(2,4);
//  DumpShape(1,8);
//  DumpShape(0,12);

  DrawShape(0,currentRotation, xOffset, --currentY);
  if (currentY==0) currentY=16;

  //if (++currentRotation>3) currentRotation=0;
}

void setup() {
  SetupShiftRegisterPins();
  SetupInterruptRoutine();
}

void SetupInterruptRoutine() {
  cli(); // Stop interrupts

  TCCR0A = 0; // 
  TCCR0B = 0;
  TCNT0 = 0; // Counter
  
  // compare match register = [ 16,000,000Hz/ (prescaler * desired interrupt frequency) ] - 1
  OCR0A = 255; //(16*10^6) / (2000*64) - 1; // OCR0A/OCR02A can be up to 255, OCR1A up to 65535
  
  TCCR0A |= (1 << WGM01); // Turn on CTC mode
  TCCR0B |= (1 << CS01) | (1 << CS00); // turn on bits for 64 prescaler
  
  TIMSK0 |= (1 << OCIE0A); // enable timer compare interrupt
  
  sei(); // allow interrupts
}

ISR(TIMER0_COMPA_vect)
{
  cycles++;
  RefreshScreen();
  
  if ((cycles % 15) == 0)
    UpdateBitmap();
}


void SetupShiftRegisterPins() {
  pinMode(Row_RCLK,OUTPUT);
  pinMode(Row_SRCLK,OUTPUT);
  pinMode(Row_SER,OUTPUT);

  pinMode(Col_RCLK,OUTPUT);
  pinMode(Col_SRCLK,OUTPUT);
  pinMode(Col_SER,OUTPUT);
}

void loop() {

}

void RefreshScreen() {
  for (byte r=0; r<16; r++) {
    // Turn off all X bits before turning on the row, so we won't get shadowing of the previous X pattern    
    twoBytes = 0;
    WriteX( highByte(twoBytes), lowByte(twoBytes) );

    // Turn on ONE row at a time
    twoBytes = 0;
    bitSet(twoBytes,r);
    twoBytes = ~twoBytes; // Rows need to be pulled to negative.  So invert before pushing bits to shift registers
    WriteY( highByte(twoBytes), lowByte(twoBytes) );

    // Now that the row is on, turn on the appropriate bit pattern for that row.
    twoBytes = bitmap[r];
    WriteX( highByte(twoBytes), lowByte(twoBytes) );

    // Let humans enjoy that row pattern for a few microseconds.
    delayMicroseconds(40); // was 400
  }
}

void WriteY(byte hiByte, byte loByte) {
  digitalWrite(Row_RCLK, LOW);
  shiftOut(Row_SER, Row_SRCLK, LSBFIRST, loByte); // When sending LSBFirst, send loByte, then HiByte
  shiftOut(Row_SER, Row_SRCLK, LSBFIRST, hiByte);
  digitalWrite(Row_RCLK, HIGH);
};

void WriteX(byte hiByte, byte loByte) {
  digitalWrite(Col_RCLK, LOW);
  shiftOut(Col_SER, Col_SRCLK, LSBFIRST, loByte);
  shiftOut(Col_SER, Col_SRCLK, LSBFIRST, hiByte); // When sending MSBFirst, send hiByte, then loByte
  digitalWrite(Col_RCLK, HIGH);
};

void ClearBitmap() {
  for (byte r=0; r<16; r++) {
    bitmap[r] = 0;
  }
}

void PlotDot(byte x, byte y, byte isOn) {
  if (isOn)
    bitSet(bitmap[y], x);
  else
    bitClear(bitmap[y], x);
}
