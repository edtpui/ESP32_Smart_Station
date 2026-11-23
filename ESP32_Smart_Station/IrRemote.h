#pragma once
#include <Arduino.h>

// Infrared remote receiver (38 kHz module such as the VS1838B / KY-022) with a
// built-in decoder for the NEC protocol, which the kit's 21-key remote uses.
// The pin interrupt times the gaps between pulses; update() turns finished
// frames into key presses, with auto-repeat while a key is held.
class IrRemote {
public:
  void begin(int pin);
  void update(uint32_t nowMs);                    // call every loop
  bool nextKey(uint8_t& command, bool& repeat);   // false when nothing is waiting

  bool     everReceived() const { return everReceived_; }
  uint8_t  lastCommand() const  { return lastCommand_; }
  uint32_t lastKeyMs() const    { return lastKeyMs_; }
  uint32_t keyCount() const     { return keyCount_; }

private:
  static void onEdge(void* arg);

  int pin_ = -1;

  // Written by the interrupt handler
  portMUX_TYPE      mux_ = portMUX_INITIALIZER_UNLOCKED;
  volatile uint32_t lastEdgeUs_ = 0;
  volatile uint32_t shift_ = 0;
  volatile uint8_t  bits_ = 0;
  volatile bool     receiving_ = false;
  volatile uint32_t frame_ = 0;
  volatile bool     framePending_ = false;
  volatile bool     repeatPending_ = false;

  // Main-loop state
  bool     everReceived_ = false;
  uint8_t  lastCommand_ = 0;
  uint32_t lastKeyMs_ = 0;       // last new key press
  uint32_t lastSignalMs_ = 0;    // last frame or repeat code
  uint32_t nextRepeatMs_ = 0;
  uint32_t keyCount_ = 0;
  bool     keyWaiting_ = false;
  bool     keyIsRepeat_ = false;
};
