/*
 * This project is for a waveform generator.
 * Code is developed by Jamil Fakhouri.
 * Use this code at your leisure, feel free to redistribute and reuse.
 */

// libraries
#include <LiquidCrystal_I2C.h>
#include <SPI.h>

// objects
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

// variables
#define ButtonDelay 250   // millisecond
#define freqUp 2          // change frequency up
#define freqDn 3          // change frequency down  
#define freqRT 5          // move frequency cursor right button
#define freqLT 4          // move frequency cursor left button
#define waveButton 6      // wave type change button
#define outputButton 7    // output on off button
#define ONOFFRelay 8      // ON OFF relay pin
#define sqrRelay 10       // square wave relay usign 100k ohm resistor
#define FSYNC 9           // SS pin for SPI with AD9833
#define SCLK 13           // data clock pin for SPI with AD9833
#define SDATA 11          // data pin (MOSI) for SPI with AD9833
#define refFreq 25e6      // reference frequency defined by master clock frequency oscillator (MCLK)
#define RESET_CMD 0x100   // reset command for AD9833
#define LOAD_DATA_CMD 0x2100  // allows complete word to be loaded
#define SIN 0x2000
#define TRI 0x2002
#define SQR 0x2028

long freqDigit = 1;
int cursorRow = 0;
int cursorCol = 0;
int waveType = 0;     // 0 = sine, 1 = triangle, 2 = square
int freqDig[12];
long freq = 0;
int output = 0;

void setup() {
  Serial.begin(9600);     // initialize serial communication
  SPI.begin();            // initialize SPI communication
  SPI.setDataMode(SPI_MODE2); // set SPI mode to mode 2

  lcd.begin(16, 2); // 16x2 lcd
  lcd.clear();      // clear screen
  setCursorPos(0, 1);
  lcd.print("Typ:");
  lcd.print("SIN");
  setCursorPos(9, 1);
  lcd.print("Out:");
  lcd.print("OFF");
  lcd.home();       // sets cursor to 0, 0
  lcd.print("Freq: 000000 Hz");
  setCursorPos(11, 0);
  lcd.blink();

  // setting pin modes
  pinMode(freqUp, INPUT_PULLUP);
  pinMode(freqDn, INPUT_PULLUP);
  pinMode(freqRT, INPUT_PULLUP);
  pinMode(freqLT, INPUT_PULLUP);
  pinMode(waveButton, INPUT_PULLUP);
  pinMode(outputButton, INPUT_PULLUP);
  //pinMode(outputLED, OUTPUT);
  pinMode(ONOFFRelay, OUTPUT);
  pinMode(sqrRelay, OUTPUT);
  pinMode(FSYNC, OUTPUT);

  digitalWrite(ONOFFRelay, HIGH);
  digitalWrite(sqrRelay, HIGH);

  AD9833Reset();
}

// the loop just waits for an input and then redirects the program flow
// the appropriate function.
void loop() {
  if (digitalRead(freqRT) == 0) {
    moveFreqRight();
    delay(ButtonDelay);
  }

  if (digitalRead(freqLT) == 0) {
    moveFreqLeft();
    delay(ButtonDelay);
  }

  if (digitalRead(freqUp) == 0) {
    changeFreqUp();
    delay(ButtonDelay);
  }

  if (digitalRead(freqDn) == 0) {
    changeFreqDown();
    delay(ButtonDelay);
  }

  if (digitalRead(waveButton) == 0) {
    changeWaveType();
    delay(250);
  }
  
  if (digitalRead(outputButton) == 0) {
    changeOutput();
    delay(ButtonDelay);
  }
}

// sets the curcsor one position to the right
// when the right button is pressed
void moveFreqRight() {
  if (cursorCol < 11) {
    cursorCol++;
    setCursorPos(cursorCol, cursorRow);
    Serial.print("Column is now @: ");
    Serial.println(cursorCol);
  }
}

// sets the curcsor one position to the left
// when the left button is pressed
void moveFreqLeft() {
  if (cursorCol > 6) {
    cursorCol--;
    setCursorPos(cursorCol, cursorRow);
    Serial.print("Column is now @: ");
    Serial.println(cursorCol);
  }
}

// increase the frequency number by 1, 10, 100, 1000, 10000, and 100000
// depending on the display's cursor position
void changeFreqUp() {
  if (cursorCol == 11) {
    if (freqDig[11] < 9) {
      freqDig[11]++;
      freq++;
      updateFreq(11);
    }
  } else if (cursorCol == 10) {
    if (freqDig[10] < 9) {
      freqDig[10]++;
      freq += 10;
      updateFreq(10);
    }
  } else if (cursorCol == 9) {
    if (freqDig[9] < 9) {
      freqDig[9]++;
      freq += 100;
      updateFreq(9);
    }
  } else if (cursorCol == 8) {
    if (freqDig[8] < 9) {
      freqDig[8]++;
      freq += 1000;
      updateFreq(8);
    }
  } else if (cursorCol == 7) {
    if (freqDig[7] < 9) {
      freqDig[7]++;
      freq += 10000;
      updateFreq(7);
    }
  } else if (cursorCol == 6) {
    if (freqDig[6] < 9) {
      freqDig[6]++;
      freq += 100000;
      updateFreq(6);
    }
  }
  Serial.print("Frequency = ");
  Serial.println(freq);
  if (output == 1) {
    AD9833SetFrequency();
  }
}

void changeFreqDown() {
  if (cursorCol == 11) {
    if (freqDig[11] > 0) {
      freqDig[11]--;
      freq--;
      updateFreq(11);
    }
  } else if (cursorCol == 10) {
    if (freqDig[10] > 0) {
      freqDig[10]--;
      freq -= 10;
      updateFreq(10);
    }
  } else if (cursorCol == 9) {
    if (freqDig[9] > 0) {
      freqDig[9]--;
      freq -= 100;
      updateFreq(9);
    }
  } else if (cursorCol == 8) {
    if (freqDig[8] > 0) {
      freqDig[8]--;
      freq -= 1000;
      updateFreq(8);
    }
  } else if (cursorCol == 7) {
    if (freqDig[7] > 0) {
      freqDig[7]--;
      freq -= 10000;
      updateFreq(7);
    }
  } else if (cursorCol == 6) {
    if (freqDig[6] > 0) {
      freqDig[6]--;
      freq -= 100000;
      updateFreq(6);
    }
  }
  Serial.print("Frequency = ");
  Serial.println(freq);
  if (output == 1) {
    AD9833SetFrequency();
  }
}

// update the display with the newly chosed frequency value
void updateFreq(int base) {
  // placing zeros
  for (int i = 6; i < 12; i++) {  // print neccessary zeros
    setCursorPos(i, 0);
    lcd.print(freqDig[i]);
  }
  
  setCursorPos(base, cursorRow);  // set cursor where it was
}

// circulte between the possible waveforms
void changeWaveType() {
  if (waveType == 0) {  // if sine
    waveType = 1;       // change to triangle
    digitalWrite(sqrRelay, HIGH);
  } else if (waveType == 1) { // if triangle
    waveType = 2;       // change to square
    digitalWrite(sqrRelay, LOW);
  } else if (waveType == 2) { // if square
    waveType = 0;
    digitalWrite(sqrRelay, HIGH);
  }
  if (output == 1) {
    AD9833SetWaveType();
  }
  updateWave();
}

// update display with the newly selected waveform
void updateWave() {
  Serial.print("Wave type set to: ");
  int currentCol = cursorCol;
  setCursorPos(4, 1);

  if (waveType == 0) {  // if sine
    lcd.print("SIN ");
    Serial.println("Sine");
  } else if (waveType == 1) { // if triangle
    lcd.print("TRI ");
    Serial.println("Triangle");
  } else if (waveType == 2) { // if square
    lcd.print("SQR ");
    Serial.println("Square");
  }
  // set cursor to where it was
  setCursorPos(currentCol, 0);
}

// change output state
void changeOutput() {
  if (output == 0) {  // if off
    output = 1;       // turn on
    digitalWrite(ONOFFRelay, LOW);
    AD9833SetFrequency();
  } else {            // if on
    output = 0;       // turn off
    digitalWrite(ONOFFRelay, HIGH);
    AD9833Reset();
  }

  updateOutput(); // update display
}

// update display with newly selected output state
void updateOutput() {
  Serial.print("Output: ");
  int currentCol = cursorCol;
  setCursorPos(13, 1);

  if (output == 0) {
    lcd.print("OFF");
    Serial.println("OFF");
  } else {
    lcd.print("ON ");
    Serial.println("ON");
  }
  // set cursor to where it was
  setCursorPos(currentCol, 0);
}

// this function sets the position of the cursor
// based on user input
void setCursorPos(int column, int row) {
  if (column >= 0 && column < 16) {
    cursorCol = column;
  } else {
    Serial.println("Invalid column position");
  }

  if (row >= 0 && row < 2) {
    cursorRow = row;
  } else {
    Serial.println("Invalid row position");
  }

  lcd.setCursor(column, row);
}

// reset wave generator chip
void AD9833Reset() {
  registerWrite(RESET_CMD);
  delay(10);
}

// set frequency of wave generator chip
void AD9833SetFrequency() {
  // calculate frequency value to send
  long frequency = (freq * pow(2, 28)) / refFreq;

  // split value into two 14 bit words
  int16_t freqMSB = (int16_t)((frequency & 0xFFFC000) >> 14);
  int16_t freqLSB = (int16_t)(frequency & 0x3FFF);

  // set control D15 ande D14 to 0 and 1, respectively, for frequency register 0
  freqMSB |= 0x4000;
  freqLSB |= 0x4000;

  registerWrite(LOAD_DATA_CMD);
  registerWrite(freqLSB);
  registerWrite(freqMSB);
  AD9833SetWaveType();
}

// set waveform type of wave generator chip
void AD9833SetWaveType() {
  if (waveType == 0) {  // if sine
    registerWrite(SIN);
  } else if (waveType == 1) { // if triangle
    registerWrite(TRI);
  } else if (waveType == 2) { // if square
    registerWrite(SQR);
  }
}

// write commaned to wave generator chip
void registerWrite(int cmd) {
  digitalWrite(FSYNC, LOW);     // set FSYNC low to start ending data
  delayMicroseconds(100);
  SPI.transfer(highByte(cmd));  // send MSBs
  SPI.transfer(lowByte(cmd));   // send LSBs
  digitalWrite(FSYNC, HIGH);    // set FSYNC high to end sending data
}
