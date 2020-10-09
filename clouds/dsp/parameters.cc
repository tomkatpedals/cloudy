// Copyright 2020 Daniel Collins
//
// Author: Daniel Collins
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
// Parameters of the granular effect.

#include "parameters.h"

#define TOUCH_RANGE 0.01f  // within +/-1% is close enough to pickup

namespace clouds {

void Parameter::init(void) {
  current_value_ = &control_value_;
}

void Parameter::init(float v) {
  current_value_ = &control_value_;
  control_value_ = v;
}

void Parameter::update(float control_value) {
  control_value_ = control_value;
  if (current_value_ == &loaded_value_) {
    bool touch = (__builtin_fabsf(control_value - loaded_value_) < TOUCH_RANGE);
    if (touch) {
      sync();
    }
  }
}

void Parameter::load(float loaded_value) {
  loaded_value_  = loaded_value;
  current_value_ = &loaded_value_;
}

}  // namespace clouds
