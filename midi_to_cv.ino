
#include <MIDI.h>
#include <SPI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

#define GATE 2
#define TRIG 3
#define TRIG_INV 4
#define DAC1 8

#define NOTE_SF 47.069f // Scaling factor for converting MIDI note to DAC voltage
// DAC input go from 0 to 4095, and our note number go from 0-87
// thus the mapping is note number * (4095 / 87) 

void setup() {  

  pinMode(GATE, OUTPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(TRIG_INV, OUTPUT);
  pinMode(DAC1, OUTPUT);  

  digitalWrite(GATE,LOW);
  digitalWrite(TRIG,LOW);
  digitalWrite(TRIG_INV,HIGH);
  digitalWrite(DAC1,HIGH);

  SPI.begin();
  //Serial.end();
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(MyHandleNoteOn);
  MIDI.setHandleNoteOff(MyHandleNoteOff);
}

void loop() {
  MIDI.read();
}

void MyHandleNoteOn(byte channel, byte pitch, byte velocity) {
  // Convert pitch to mV for the DAC output
  int noteMsg = MIDI.getData1() - 21; // MIDI pitch 21 corresponds to the lowest no2te in the 88-key range (A0)
  if (noteMsg < 0 || noteMsg > 87) return; // Ensure pitch is within valid range
  unsigned int mV = (unsigned int)((float)noteMsg * NOTE_SF); // I'm assuming the 0.5 is there to round up?
  
  // Set DAC (i.e., note CV) voltage
  setVoltage(DAC1, 0, 1, mV); // Set DAC1, channel 0, 2X gain, and the calculated mV
  digitalWrite(GATE,HIGH);
  digitalWrite(TRIG,HIGH);
  digitalWrite(TRIG_INV,LOW);
  delay(1000);
  digitalWrite(TRIG,LOW);
  digitalWrite(TRIG_INV,HIGH);
}

void MyHandleNoteOff(byte channel, byte pitch, byte velocity) {
  digitalWrite(GATE,LOW);
}

void setVoltage(int dacpin, bool channel, bool gain, unsigned int mV) {
  // the command we are writing to the DAC is 16bit
  // first 4 are control (-A/B select, -, -gain, -shutdown), last 12bit is data 
  unsigned int command = channel ? 0x9000 : 0x1000;
  command |= gain ? 0x0000 : 0x2000;
  command |= (mV & 0x0FFF);
  
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
  digitalWrite(dacpin, LOW); // this tell dac we are writing
  // the data sheet says it's supposed to take 16, so I don't see why the github guy did 2 8bit transfer
  SPI.transfer16(command);
  //SPI.transfer(command >> 8);
  //SPI.transfer(command & 0xFF);
  digitalWrite(dacpin, HIGH);
  SPI.endTransaction();
}
