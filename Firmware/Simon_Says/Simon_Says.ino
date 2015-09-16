/*
 Started: 6-19-2007
 Spark Fun Electronics
 Nathan Seidle
 
 Simon Says is a memory game. Start the game by pressing one of the four buttons. When a button lights up, 
 press the button, repeating the sequence. The sequence will get longer and longer. The game is won after 
 13 rounds.
 
 This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
 
 Simon Says game originally written in C for the PIC16F88.
 Ported for the ATmega168, then ATmega328, then Arduino 1.0.
 Fixes and cleanup by Joshua Neal <joshua[at]trochotron.com>
 
 Generates random sequence, plays music, and displays button lights.
 
 Simon tones from Wikipedia
 - A (red, upper left) - 440Hz - 2.272ms - 1.136ms pulse
 - a (green, upper right, an octave higher than A) - 880Hz - 1.136ms,
 0.568ms pulse
 - D (blue, lower left, a perfect fourth higher than the upper left) 
 587.33Hz - 1.702ms - 0.851ms pulse
 - G (yellow, lower right, a perfect fourth higher than the lower left) - 
 784Hz - 1.276ms - 0.638ms pulse
 
 The tones are close, but probably off a bit, but they sound all right.
 
 The old version of SparkFun simon used an ATmega8. An ATmega8 ships
 with a default internal 1MHz oscillator.  You will need to set the
 internal fuses to operate at the correct external 16MHz oscillator.
 
 Original Fuses:
 avrdude -p atmega8 -P lpt1 -c stk200 -U lfuse:w:0xE1:m -U hfuse:w:0xD9:m
 
 Command to set to fuses to use external 16MHz: 
 avrdude -p atmega8 -P lpt1 -c stk200 -U lfuse:w:0xEE:m -U hfuse:w:0xC9:m
 
 The current version of Simon uses the ATmega328. The external osciallator
 was removed to reduce component count.  This version of simon relies on the
 internal default 1MHz osciallator. Do not set the external fuses.
 */

#include "hardware_versions.h"
#include "pitches.h" // Used for the MODE_BEEGEES, for playing the melody on the buzzer!

// Define game parameters
#define ROUNDS_TO_WIN      13 //Number of rounds to succesfully remember before you win. 13 is do-able.
#define ENTRY_TIME_LIMIT   3000 //Amount of time to press a button before game times out. 3000ms = 3 sec

#define MODE_MEMORY  0
#define MODE_BATTLE  1
#define MODE_BEEGEES 2

// Game state variables
byte gameMode = MODE_MEMORY; //By default, let's play the memory game
byte gameBoard[32]; //Contains the combination of buttons as we advance
byte gameRound = 0; //Counts the number of succesful rounds the player has made it through

void setup()
{
  //Setup hardware inputs/outputs. These pins are defined in the hardware_versions header file

  //Enable pull ups on inputs
  pinMode(BUTTON_RED, INPUT_PULLUP);
  pinMode(BUTTON_GREEN, INPUT_PULLUP);
  pinMode(BUTTON_BLUE, INPUT_PULLUP);
  pinMode(BUTTON_YELLOW, INPUT_PULLUP);

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);

  pinMode(BUZZER1, OUTPUT);
  pinMode(BUZZER2, OUTPUT);

  play_greeting(); // After setup is complete, say hello to the world
}

void loop()
{
  byte button = attractMode(); // Blink lights while waiting for user to press a button

  // Indicate the start of game play
  setLEDs(CHOICE_RED | CHOICE_GREEN | CHOICE_BLUE | CHOICE_YELLOW); // Turn all LEDs on
  delay(1000);
  setLEDs(CHOICE_OFF); // Turn off LEDs
  delay(250);

  switch (button) {
  case CHOICE_RED:
    // Play memory game and handle result
    if (play_memory() == true) 
      play_march(); // Player won, play winner tones
    else 
      play_loser(); // Player lost, play loser tones
    break;

  case CHOICE_GREEN:
    play_battle(); // Play game until someone loses

    play_loser(); // Player lost, play loser tones
    break;
    
  case CHOICE_BLUE:
    play_getlucky();
    break;
    
  case CHOICE_YELLOW:
    play_beegees();
    break;
  }
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//The following functions are related to game play only

// Play the regular memory game
// Returns 0 if player loses, or 1 if player wins
boolean play_memory(void)
{
  randomSeed(millis()); // Seed the random generator with random amount of millis()

  gameRound = 0; // Reset the game to the beginning

  while (gameRound < ROUNDS_TO_WIN) 
  {
    add_to_moves(); // Add a button to the current moves, then play them back

    playMoves(); // Play back the current game board

    // Then require the player to repeat the sequence.
    for (byte currentMove = 0 ; currentMove < gameRound ; currentMove++)
    {
      byte choice = wait_for_button(); // See what button the user presses

      if (choice == 0) return false; // If wait timed out, player loses

      if (choice != gameBoard[currentMove]) return false; // If the choice is incorect, player loses
    }

    delay(1000); // Player was correct, delay before playing moves
  }

  return true; // Player made it through all the rounds to win!
}

// Play the special 2 player battle mode
// A player begins by pressing a button then handing it to the other player
// That player repeats the button and adds one, then passes back.
// This function returns when someone loses
boolean play_battle(void)
{
  gameRound = 0; // Reset the game frame back to one frame
  
  while (1) // Loop until someone fails 
  {
    byte newButton = wait_for_button(); // Wait for user to input next move
    gameBoard[gameRound++] = newButton; // Add this new button to the game array

    // Then require the player to repeat the sequence.
    for (byte currentMove = 0 ; currentMove < gameRound ; currentMove++)
    {
      byte choice = wait_for_button();

      if (choice == 0) return false; // If wait timed out, player loses.

      if (choice != gameBoard[currentMove]) return false; // If the choice is incorect, player loses.
    }

    delay(100); // Give the user an extra 100ms to hand the game to the other player
  }

  return true; // We should never get here
}

// Plays the current contents of the game moves
void playMoves(void)
{
  for (byte currentMove = 0 ; currentMove < gameRound ; currentMove++) 
  {
    toner(gameBoard[currentMove], 150);

    // Wait some amount of time between button playback
    // Shorten this to make game harder
    delay(150); // 150 works well. 75 gets fast.
  }
}

// Adds a new random button to the game sequence, by sampling the timer
void add_to_moves(void)
{
  byte newButton = random(0, 4); //min (included), max (exluded)

  // We have to convert this number, 0 to 3, to CHOICEs
  if(newButton == 0) newButton = CHOICE_RED;
  else if(newButton == 1) newButton = CHOICE_GREEN;
  else if(newButton == 2) newButton = CHOICE_BLUE;
  else if(newButton == 3) newButton = CHOICE_YELLOW;

  gameBoard[gameRound++] = newButton; // Add this new button to the game array
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//The following functions control the hardware

// Lights a given LEDs
// Pass in a byte that is made up from CHOICE_RED, CHOICE_YELLOW, etc
void setLEDs(byte leds)
{
  if ((leds & CHOICE_RED) != 0)
    digitalWrite(LED_RED, HIGH);
  else
    digitalWrite(LED_RED, LOW);

  if ((leds & CHOICE_GREEN) != 0)
    digitalWrite(LED_GREEN, HIGH);
  else
    digitalWrite(LED_GREEN, LOW);

  if ((leds & CHOICE_BLUE) != 0)
    digitalWrite(LED_BLUE, HIGH);
  else
    digitalWrite(LED_BLUE, LOW);

  if ((leds & CHOICE_YELLOW) != 0)
    digitalWrite(LED_YELLOW, HIGH);
  else
    digitalWrite(LED_YELLOW, LOW);
}

// Wait for a button to be pressed. 
// Returns one of LED colors (LED_RED, etc.) if successful, 0 if timed out
byte wait_for_button(void)
{
  long startTime = millis(); // Remember the time we started the this loop

  while ( (millis() - startTime) < ENTRY_TIME_LIMIT) // Loop until too much time has passed
  {
    byte button = checkButton();

    if (button != CHOICE_NONE)
    { 
      toner(button, 150); // Play the button the user just pressed
      
      while(checkButton() != CHOICE_NONE) ;  // Now let's wait for user to release button
      
      delay(10); // This helps with debouncing and accidental double taps

      return button;
    }

  }

  return CHOICE_NONE; // If we get here, we've timed out!
}

// Returns a '1' bit in the position corresponding to CHOICE_RED, CHOICE_GREEN, etc.
byte checkButton(void)
{
  byte choice = 0;

  if (digitalRead(BUTTON_RED) == 0) choice |= CHOICE_RED; 
  else if (digitalRead(BUTTON_GREEN) == 0) choice |= CHOICE_GREEN; 
  else if (digitalRead(BUTTON_BLUE) == 0) choice |= CHOICE_BLUE; 
  else if (digitalRead(BUTTON_YELLOW) == 0) choice |= CHOICE_YELLOW;
  
  return choice;
}

// Light an LED and play tone
// Red, upper left:     440Hz - 2.272ms - 1.136ms pulse
// Green, upper right:  880Hz - 1.136ms - 0.568ms pulse
// Blue, lower left:    587.33Hz - 1.702ms - 0.851ms pulse
// Yellow, lower right: 784Hz - 1.276ms - 0.638ms pulse
void toner(byte which, int buzz_length_ms)
{
  setLEDs(which); //Turn on a given LED

  //Play the sound associated with the given LED
  switch(which) 
  {
  case CHOICE_RED:
    playNote(4, 0, buzz_length_ms);
    break;
  case CHOICE_GREEN:
    playNote(4, 1, buzz_length_ms);
    break;
  case CHOICE_BLUE:
    playNote(4, 3, buzz_length_ms);
    break;
  case CHOICE_YELLOW:
    playNote(4, 7, buzz_length_ms);
    break;
  }

  setLEDs(CHOICE_OFF); // Turn off all LEDs
}

// Toggle buzzer every buzz_delay_us, for a duration of buzz_length_ms.
void buzz_sound(int buzz_length_ms, unsigned int freq)
{
  tone(BUZZER2, freq, buzz_length_ms);
  delay(buzz_length_ms);
}

// Play the winner sound and lights
void play_greeting(void)
{
  setLEDs(CHOICE_GREEN | CHOICE_BLUE);
  greeting_sound();
  setLEDs(CHOICE_RED | CHOICE_YELLOW);
  greeting_sound();
  setLEDs(CHOICE_GREEN | CHOICE_BLUE);
  greeting_sound();
  setLEDs(CHOICE_RED | CHOICE_YELLOW);
  greeting_sound();
}

// Play the winner sound
// This is just a unique (annoying) sound we came up with, there is no magic to it
void greeting_sound(void)
{
  // Toggle the buzzer at various speeds
  for (byte x = 250 ; x > 70 ; x--)
  {
    for (byte y = 0 ; y < 3 ; y++)
    {
      digitalWrite(BUZZER2, HIGH);
      delayMicroseconds(x);

      digitalWrite(BUZZER2, LOW);
      delayMicroseconds(x);
    }
  }
}

// Play the loser sound/lights
void play_loser(void)
{
  setLEDs(CHOICE_RED | CHOICE_GREEN);
  buzz_sound(255, 220);

  setLEDs(CHOICE_BLUE | CHOICE_YELLOW);
  buzz_sound(255, 220);

  setLEDs(CHOICE_RED | CHOICE_GREEN);
  buzz_sound(255, 220);

  setLEDs(CHOICE_BLUE | CHOICE_YELLOW);
  buzz_sound(255, 220);
}

// Show an "attract mode" display while waiting for user to press button.
byte attractMode(void)
{
  byte led = 0;
  byte button = 0;

  for (; button == 0;) {
    setLEDs(1 << led);
    delay(100);
    button = checkButton();
    led = (led + 1) % 4;
  }
  
  return button;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// The following functions are related to Beegees Easter Egg only

const unsigned int freqs[] = {
  3520, // A
  3729, // A#
  3951, // B
  2093, // C
  2217, // C#
  2349, // D
  2489, // D#
  2637, // E
  2794, // F
  2960, // F#
  3136, // G
  3322, // G#
};

const int scale[] = {
  0, 2, 3, 5, 7, 8, 10
};

void
playNote(int octave, int note, int duration)
{
  unsigned int freq;
  int i;

  if (note >= 0) {
    freq = freqs[note];
    for (i = octave; i < 7; i += 1) {
      freq /= 2;
    }
    
    tone(BUZZER2, freq, duration);
    setLEDs(note + 1);
  }
  delay(duration);
  setLEDs(0);
}

void
play(int bpm, char *tune)
{
  unsigned int baseDuration = 60000 / bpm;
  int duration = baseDuration;
  int baseOctave = 4;
  int octave = baseOctave;
  
  int note = -2;

  char *p = tune;
  
  digitalWrite(BUZZER1, LOW); // setup the "BUZZER1" side of the buzzer to stay low, while we play the tone on the other pin.
  for (; *p; p += 1) {
    boolean playNow = false;
    
    switch (*p) {
    case '>':
      octave = baseOctave + 1;
      break;
    case '<':
      octave = baseOctave - 1;
      break;
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
      playNow = true;
      note = scale[*p - 'A'];
      break;
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
      playNow = true;
      octave += 1;
      note = scale[*p - 'a'];
      break;
    case '_':
      playNow = true;
      note = -1;
      break;
    }
    
    // Check for sharps or flats
    switch (*(p+1)) {
    case '#':
    case '+':
      note += 1;
      if (note == 12) {
        octave += 1;
        note = 0;
      }
      p += 1;
      break;
    case '-':
      note -= 1;
      if (note == -1) {
        octave -= 1;
        note = 11;
      }
      p += 1;
      break;
    }
    
    // Check for octave
    switch (*(p+1)) {
    case ',':
      octave -= 1;
      p += 1;
      break;
    case '\'':
      octave += 1;
      p += 1;
      break;
    }
    
    // Check for duration
    switch (*(p+1)) {
    case '2':
      duration *= 2;
      p += 1;
      break;
    case '4':
      duration *= 4;
      p += 1;
      break;
    case '.':
      duration += duration / 2;
      p += 1;
      break;
    }
    
    if (playNow) {
      playNote(octave, note, duration);
      note = -1;
      octave = baseOctave;
      duration = baseDuration;
    }
    
    if (checkButton()) {
      noTone(BUZZER2);
      return;
    }
  }
  if (note >= 0) {
    playNote(octave, note, duration);
  }
  noTone(BUZZER2);
}

// Do nothing but play bad beegees music
// This function is activated when user holds bottom right button during power up
void play_beegees()
{
  while(checkButton() == CHOICE_NONE) {
    play(104 * 4, "E-F_a-__E-2__C_B-,CE-_B-,C_E-__B-,_C_E-_F_a-_");
  }
}

void lplay(unsigned int tempo, char *tune, int count)
{
  int i;
  
  for (i = 0; i < count; i += 1) {
    play(tempo * 3, tune);
  }
}


void play_march()
{
  const int tempo = 80;
  
  lplay(tempo * 4, "GB-,D", 6);
  lplay(tempo * 4, "GB-,D", 6);
  lplay(tempo * 4, "GB-,D", 6);
  lplay(tempo * 4, "E-G-,B-,", 5);
  play(tempo * 4, "B-");
  lplay(tempo * 4, "GB-,D", 6);
  lplay(tempo * 4, "E-G-,B-,", 5);
  play(tempo * 4, "B-");
  lplay(tempo * 4, "GB-,D", 6);
  lplay(tempo * 4, "GB-,D", 6);
  
  lplay(tempo * 4, "dGB-", 6);
  lplay(tempo * 4, "dGB-", 6);
  lplay(tempo * 4, "dGB-", 6);
  lplay(tempo * 4, "e-G-B-", 5);
  play(tempo * 4, "B-");
  lplay(tempo * 4, "G-B-,F", 6);
  lplay(tempo * 4, "E-G-,B-,", 5);
  play(tempo * 4, "B-");
  lplay(tempo * 4, "GB-,D", 6);
  lplay(tempo * 4, "GB-,D", 6);
  
  return;
}

void play_getlucky()
{
  while (checkButton() == CHOICE_NONE) {
    play(116 * 4, ("_______B__B_A_F+_"
                   "_______d__d_c+_A_"
                   "_______f+__f+_e_c+_"
                   "_______e__e_d_B_"));
  }
}
