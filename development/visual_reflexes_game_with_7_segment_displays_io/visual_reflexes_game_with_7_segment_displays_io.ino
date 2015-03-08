/*
 *  Visual Reflexes Game With 7-Segment Displays & I/O.
 *
 *  Copyright (C) 2010 Efstathios Chatzikyriakidis (contact@efxa.org)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// include for portable "non-local" jumps.
#include <setjmp.h>

// include notes' frequencies.
#include "pitches.h"

// the led pin of the game.
const uint8_t ledPin = 4;

// the piezo pin of the game.
const uint8_t piezoPin = 6;

// data type definition for a user player.
typedef struct UP_T {
  uint8_t buttonPin;
  uint8_t irqNumber;
  uint8_t winnings;
  void (*handler) (void);
} UP_T;

// array for the user players.
UP_T userPlayers[] = {
  { 2, 0, 0, player0Handler },
  { 3, 1, 0, player1Handler }
};

// max number of the user players.
const uint8_t MAX_UPS = (uint8_t) (sizeof (userPlayers) / sizeof (userPlayers[0]));

// data type definition for a 74HC595 8-bit shift register.
typedef struct SR_T {
  uint8_t latchPin;
  uint8_t clockPin; // PWM pin.
  uint8_t dataPin;
} SR_T;

// array for the 74HC595 8-bit shift registers.
const SR_T shiftRegisters[MAX_UPS] = {
  { 13, 11, 12 }, { 9, 10, 8 }
};

// 7-segment display character set codes.
const uint8_t charSet[] = {
  0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x98 // 0-9.

  // ABCDEF: 0x88, 0x83, 0xC6, 0xA1, 0x86, 0x8E
};

// calculate the number of the symbols in the character set array.
const uint8_t CHARSET_SYMS = (uint8_t) (sizeof (charSet) / sizeof (charSet[0]));

// notes in the game's melody.
const uint16_t notesMelody[] = {
  NOTE_G4, NOTE_C4, NOTE_G3, NOTE_G3, NOTE_C4, NOTE_G3, NOTE_C4
};

// note durations: 4 = quarter note, 8 = eighth note, etc.
const uint8_t noteDurations[] = {
  4, 8, 8, 2, 4, 2, 4
};

// calculate the number of the notes in the melody in the array.
const uint8_t NUM_NOTES = (uint8_t) (sizeof (notesMelody) / sizeof (notesMelody[0]));

// time slot delay time period (ms).
const uint32_t timeSlotDelay = 1000;

// define a bounce time for a button.
const uint16_t BOUNCE_DURATION = 200;

// ms count to debounce a pressed button.
volatile uint32_t bounceTime = 0;

// time slot status for a user player.
volatile bool timeSlotStatus = false;

// information to restore calling environment.
jmp_buf buf;

// startup point entry (runs once).
void
setup() {
  // set the led and piezo as outputs.
  pinMode(ledPin, OUTPUT);
  pinMode(piezoPin, OUTPUT);

  // initialize all the user players.
  for (uint8_t i = 0; i < MAX_UPS; i++) {
    // get the handler pointer of the user player.
    void (*handler) (void) = userPlayers[i].handler;

    // get pin & irq of user player's interrupting button.
    uint8_t irq = userPlayers[i].irqNumber;
    uint8_t button = userPlayers[i].buttonPin;

    // set the button as output.
    pinMode(button, INPUT);

    // attach an ISR for the IRQ (for button pressing).
    attachInterrupt(irq, handler, RISING);
  }

  // initialize all 74HC595 8-bit shift registers.
  for (uint8_t i = 0; i < MAX_UPS; i++) {
    // get the control pins of the 74HC595.
    uint8_t latch = shiftRegisters[i].latchPin;
    uint8_t clock = shiftRegisters[i].clockPin;
    uint8_t data = shiftRegisters[i].dataPin;

    // set 74HC595 control pins as output.
    pinMode(latch, OUTPUT);
    pinMode(clock, OUTPUT);  
    pinMode(data, OUTPUT);  
  }

  // if the analog input pin 0 is unconnected, random analog noise
  // will cause the call to randomSeed() to produce different seed
  // numbers each time the sketch runs.
  randomSeed(analogRead(0));
}

// ISR for the Player0 IRQ (is called on button presses).
void
player0Handler() {
  // long jump flag to the main loop.
  bool jumpFlag = false;

  // it ignores presses intervals less than the bounce time.
  if (abs(millis() - bounceTime) > BOUNCE_DURATION) {
    // if the time slot is open.
    if (timeSlotStatus) {
      // increase the winnings of the user player.
      userPlayers[0].winnings++;

      // close the time slot.
      timeSlotStatus = false;
      
      // set the flag true in order to occur the jump.
      jumpFlag = true;
    }

    // set whatever bounce time in ms is appropriate.
    bounceTime = millis();

    // if the jump flag is true.
    if (jumpFlag)
      // go to the main loop and start again.
      longjmp(buf, 0);
  }
}

// ISR for the Player1 IRQ (is called on button presses).
void
player1Handler() {
  // long jump flag to the main loop.
  bool jumpFlag = false;

  // it ignores presses intervals less than the bounce time.
  if (abs(millis() - bounceTime) > BOUNCE_DURATION) {
    // if the time slot is open.
    if (timeSlotStatus) {
      // increase the winnings of the user player.
      userPlayers[1].winnings++;

      // close the time slot.
      timeSlotStatus = false;
      
      // set the flag true in order to occur the jump.
      jumpFlag = true;
    }

    // set whatever bounce time in ms is appropriate.
    bounceTime = millis();

    // if the jump flag is true.
    if (jumpFlag)
      // go to the main loop and start again.
      longjmp(buf, 0);
  }
}

// loop the main sketch.
void
loop() {
  // save the environment of the calling function.
  setjmp(buf);

  // close the time slot.
  timeSlotStatus = false;

  // dark the reflexes led.
  digitalWrite(ledPin, LOW);

  // refresh user players' 7-segment winnings displays.
  for (uint8_t i = 0; i < MAX_UPS; i++) {
    // get the control pins of the 74HC595.
    uint8_t latch = shiftRegisters[i].latchPin;
    uint8_t clock = shiftRegisters[i].clockPin;
    uint8_t data = shiftRegisters[i].dataPin;

    // send to the 74HC595 the 8-bit character according to user's winnings.
    updateSR (latch, data, clock, charSet[userPlayers[i].winnings]);
  }

  // check winning conditions for all the user players.
  for (uint8_t i = 0; i < MAX_UPS; i++) {
    // if the user player's winnings is the last symbol (user has win).
    if (userPlayers[i].winnings >= CHARSET_SYMS-1) {
      // play a winning melody.
      playMelody();

      // reset all user players' winnings.
      for (uint8_t j = 0; j < MAX_UPS; j++)
        userPlayers[j].winnings = 0;
      
      // go to the main loop and start again.
      longjmp(buf, 0);
    }
  }

  // delay random time slots.
  delay(random(1, 10) * timeSlotDelay); 

  // open the time slot.
  timeSlotStatus = true;

  // light the reflexes led.
  digitalWrite(ledPin, HIGH);

  // delay a time slot so the players can see the light.
  delay(timeSlotDelay); 
}

// sends sequence bits to a 74HC595 8-bit shift register.
void
updateSR(uint8_t latch, uint8_t data, uint8_t clock, uint8_t value) {
  // pulls the chips latch low.
  digitalWrite(latch, LOW);

  // shifts out the 8 bits to the shift register.
  shiftOut(data, clock, MSBFIRST, value);

  // pulls the latch high displaying the data.
  digitalWrite(latch, HIGH);
}

// play a melody and return immediately.
void
playMelody() {
  // iterate over the notes of the melody.
  for (uint8_t thisNote = 0; thisNote < NUM_NOTES; thisNote++) {
    // to calculate the note duration, take one second divided by the note type.
    // e.g. quarter note = 1000/4, eighth note = 1000/8, etc.
    uint16_t noteDuration = 1000 / noteDurations[thisNote];

    // play the tone.
    tone(piezoPin, notesMelody[thisNote], noteDuration);

    // to distinguish notes, set a minimum time between them.
    // the note's duration plus 30% seems to work well enough.
    uint16_t pauseBetweenNotes = noteDuration * 1.30;

    // delay some time.
    delay(pauseBetweenNotes);
  }
}
