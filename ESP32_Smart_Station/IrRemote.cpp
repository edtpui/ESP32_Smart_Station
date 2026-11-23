#include "IrRemote.h"
#include "Config.h"

namespace {
// NEC timing, measured from one falling edge (start of a 38 kHz burst) to the
// next. Nominal values: frame start 13.5 ms, repeat code 11.25 ms, "0" bit
// 1.125 ms, "1" bit 2.25 ms.
constexpr uint32_t START_MIN_US  = 12500, START_MAX_US  = 14500;
constexpr uint32_t REPEAT_MIN_US = 10500, REPEAT_MAX_US = 12499;
constexpr uint32_t ZERO_MIN_US   = 900,   ZERO_MAX_US   = 1400;
constexpr uint32_t ONE_MIN_US    = 1900,  ONE_MAX_US    = 2600;
constexpr uint32_t HELD_GAP_MS   = 250;   // repeat codes arrive every 108 ms while a key is held
}  // namespace

void IrRemote::begin(int pin) {
  pin_ = pin;
  pinMode(pin_, INPUT_PULLUP);   // the receiver idles HIGH; the pull-up keeps it quiet if unplugged
  attachInterruptArg(digitalPinToInterrupt(pin_), &IrRemote::onEdge, this, FALLING);
}

void IRAM_ATTR IrRemote::onEdge(void* arg) {
  IrRemote* self = static_cast<IrRemote*>(arg);
  const uint32_t nowUs = micros();
  const uint32_t gap = nowUs - self->lastEdgeUs_;
  self->lastEdgeUs_ = nowUs;

  if (gap >= START_MIN_US && gap <= START_MAX_US) {
    self->shift_ = 0;
    self->bits_ = 0;
    self->receiving_ = true;
    return;
  }
  if (gap >= REPEAT_MIN_US && gap <= REPEAT_MAX_US) {
    self->receiving_ = false;
    portENTER_CRITICAL_ISR(&self->mux_);
    self->repeatPending_ = true;
    portEXIT_CRITICAL_ISR(&self->mux_);
    return;
  }
  if (!self->receiving_) return;

  if (gap >= ONE_MIN_US && gap <= ONE_MAX_US) {
    self->shift_ = self->shift_ | (1UL << self->bits_);   // bits arrive least significant first
  } else if (gap < ZERO_MIN_US || gap > ZERO_MAX_US) {
    self->receiving_ = false;                             // not a valid bit: drop the frame
    return;
  }
  self->bits_ = self->bits_ + 1;
  if (self->bits_ == 32) {
    self->receiving_ = false;
    portENTER_CRITICAL_ISR(&self->mux_);
    self->frame_ = self->shift_;
    self->framePending_ = true;
    self->repeatPending_ = false;
    portEXIT_CRITICAL_ISR(&self->mux_);
  }
}

void IrRemote::update(uint32_t nowMs) {
  if (pin_ < 0) return;

  portENTER_CRITICAL(&mux_);
  const bool gotFrame = framePending_;
  const bool gotRepeat = repeatPending_;
  const uint32_t frame = frame_;
  framePending_ = false;
  repeatPending_ = false;
  portEXIT_CRITICAL(&mux_);

  if (gotFrame) {
    // Frame layout: address, address check, command, inverted command.
    const uint8_t command = (frame >> 16) & 0xFF;
    const uint8_t inverted = (frame >> 24) & 0xFF;
    if ((command ^ inverted) != 0xFF) return;   // corrupted, ignore it

    lastCommand_ = command;
    everReceived_ = true;
    lastKeyMs_ = nowMs;
    lastSignalMs_ = nowMs;
    nextRepeatMs_ = nowMs + IR_REPEAT_DELAY_MS;
    keyCount_++;
    keyWaiting_ = true;
    keyIsRepeat_ = false;
    return;
  }

  if (gotRepeat && everReceived_ && nowMs - lastSignalMs_ <= HELD_GAP_MS) {
    lastSignalMs_ = nowMs;
    if (static_cast<int32_t>(nowMs - nextRepeatMs_) >= 0) {
      nextRepeatMs_ = nowMs + IR_REPEAT_RATE_MS;
      keyWaiting_ = true;
      keyIsRepeat_ = true;
    }
  }
}

bool IrRemote::nextKey(uint8_t& command, bool& repeat) {
  if (!keyWaiting_) return false;
  keyWaiting_ = false;
  command = lastCommand_;
  repeat = keyIsRepeat_;
  return true;
}
