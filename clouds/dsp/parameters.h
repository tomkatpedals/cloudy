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
// Parameters of the granular effect.

#ifndef CLOUDS_DSP_PARAMETERS_H_
#define CLOUDS_DSP_PARAMETERS_H_

#include "stmlib/stmlib.h"

namespace clouds {

enum PotMode {
  kPotModeJump = 0U,
  kPotModePickup,
};

class Parameter {
 public:
  Parameter(void) {
    init();
  }

  void init(void);
  void init(float);
  void update(float control_value);
  void load(float active_value);

  inline float value(void) const {
    return *current_value_;
  }

  inline void sync(void) {
    current_value_ = &control_value_;
  }

 private:
  float* current_value_;
  float  loaded_value_;
  float  control_value_;
};

struct Parameters {
  Parameter position;
  Parameter size;
  Parameter pitch;
  Parameter density;
  Parameter texture;
  Parameter dry_wet;
  Parameter stereo_spread;
  Parameter feedback;
  Parameter reverb;

  bool freeze;
  bool trigger;
  bool gate;

  struct Granular {
    float overlap;
    float window_shape;
    float stereo_spread;
    bool  use_deterministic_seed;
    bool  reverse;
  } granular;

  struct Spectral {
    float quantization;
    float refresh_rate;
    float phase_randomization;
    float warp;
  } spectral;
};

}  // namespace clouds

#endif  // CLOUDS_DSP_PARAMETERS_H_
