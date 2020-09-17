// Copyright 2014 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// Driver for the front panel switches.

#ifndef CLOUDS_DRIVERS_SWITCHES_H_
#define CLOUDS_DRIVERS_SWITCHES_H_

#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
#include "stmlib/stmlib.h"
#include "stmlib/system/system_clock.h"

namespace clouds {

enum SwitchIndex {
  SWITCH_MODE = 0U,
  SWITCH_WRITE,
  SWITCH_FREEZE,
  SWITCH_BYPASS,
  kNumSwitches,
};

enum SwitchState {
  SwitchReleased = 0U,
  SwitchPressed,
  SwitchLongPressed,
  SwitchVLongPressed,
};

class Switch {
 public:
  void Init(GPIO_TypeDef* g, uint16_t p);

  inline bool released(void) const {
    return (debounce_buffer_ == 0x7f);
  }

  inline bool just_pressed(void) const {
    return (debounce_buffer_ == 0x80);
  }

  inline bool pressed(void) const {
    return (debounce_buffer_ == 0x00);
  }

  inline bool pressed_immediate(void) const {
    return (!GPIO_ReadInputDataBit(gpio_, pin_));
  }

  inline void scan(void) {
    debounce_buffer_ = (debounce_buffer_ << 1) | GPIO_ReadInputDataBit(gpio_, pin_);
  }

  inline void capture_press(void) {
    press_time_ = stmlib::system_clock.milliseconds();
    state_      = SwitchPressed;
  }

  inline uint32_t press_time(void) {
    return press_time_;
  }

  inline void reset(void) {
    press_time_ = 0;
    state_      = SwitchReleased;
  }

  inline void set_state(enum SwitchState state) {
    state_ = state;
  }

  inline enum SwitchState state(void) {
    return state_;
  }

 private:
  GPIO_TypeDef*    gpio_;
  uint16_t         pin_;
  uint8_t          debounce_buffer_;
  enum SwitchState state_;
  uint32_t         press_time_;
};

class Switches {
 public:
  Switches() {}
  ~Switches() {}

  void Init();
  void Scan();

  inline Switch* operator[](size_t index) {
    return (&switches_[index]);
  }

 private:
  struct Switch switches_[kNumSwitches];

  DISALLOW_COPY_AND_ASSIGN(Switches);
};

}  // namespace clouds

#endif  // CLOUDS_DRIVERS_SWITCHES_H_
