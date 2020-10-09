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
// Calibration settings.

#include "clouds/cv_scaler.h"

#include <algorithm>
#include <cmath>

#include "stmlib/dsp/dsp.h"

#include "clouds/resources.h"

namespace clouds {

using namespace std;

/* static */
CvInput CvScaler::inputs_[ADC_CHANNEL_LAST] = {
  // Polarity,       Offset,    FltCoeff, Value
  // ADC_POSITION_POTENTIOMETER_CV,
  { kPolarityInvert, kNoOffset, 0.05f, 0.0f },
  // ADC_DENSITY_POTENTIOMETER_CV,
  { kPolarityInvert, kNoOffset, 0.01f, 0.0f },
  // ADC_SIZE_POTENTIOMETER,
  { kPolarityNormal, kNoOffset, 0.01f, 0.0f },
  // ADC_FEEDBACK_POTENTIOMETER,
  { kPolarityNormal, kNoOffset, 0.05f, 0.0f },
  // ADC_PITCH_POTENTIOMETER,
  { kPolarityNormal, kNoOffset, 0.01f, 0.0f },
  // ADC_V_OCT_CV,
  { kPolarityNormal, kNoOffset, 1.00f, 0.0f },
  // ADC_DRYWET_POTENTIOMETER,
  { kPolarityNormal, kNoOffset, 0.05f, 0.0f },
  // ADC_SPREAD_POTENTIOMETER,
  { kPolarityNormal, kNoOffset, 0.05f, 0.0f },
  // ADC_TEXTURE_POTENTIOMETER,
  { kPolarityNormal, kNoOffset, 0.01f, 0.0f },
  // ADC_REVERB_POTENTIOMETER,
  { kPolarityNormal, kNoOffset, 0.05f, 0.0f }
};

void CvScaler::Init(CalibrationData* calibration_data) {
  adc_.Init();
  gate_input_.Init();
  calibration_data_ = calibration_data;
  note_             = 0.0f;

  fill(&previous_trigger_[0], &previous_trigger_[kAdcLatency], false);
  fill(&previous_gate_[0], &previous_gate_[kAdcLatency], false);
}

void CvScaler::Read(Parameters* parameters) {
  for (size_t i = 0; i < ADC_CHANNEL_LAST; ++i) {
    CvInput* input = &inputs_[i];

    float value = adc_.float_value(i);
    if (input->polarity == kPolarityInvert) {
      value = 1.0f - value;
    }
    if (input->offset == kApplyOffset) {
      value -= calibration_data_->offset[i];
    }
    input->value += input->filter_coefficient * (value - input->value);
  }

  parameters->position.update(value(ADC_POSITION_POTENTIOMETER_CV));
  parameters->texture.update(saturate(value(ADC_TEXTURE_POTENTIOMETER), 0.0f, 1.0f));
  parameters->density.update(saturate(value(ADC_DENSITY_POTENTIOMETER_CV), 0.0f, 1.0f));
  parameters->size.update(saturate(value(ADC_SIZE_POTENTIOMETER), 0.0f, 1.0f));
  parameters->dry_wet.update(expanded_value(ADC_DRYWET_POTENTIOMETER));
  parameters->reverb.update(expanded_value(ADC_REVERB_POTENTIOMETER));
  parameters->feedback.update(expanded_value(ADC_FEEDBACK_POTENTIOMETER));
  parameters->stereo_spread.update(expanded_value(ADC_SPREAD_POTENTIOMETER));
  float pitch = stmlib::Interpolate(lut_quantized_pitch, value(ADC_PITCH_POTENTIOMETER), 1024.0f);
  parameters->pitch.update(saturate(pitch, -48.0f, 48.0f));

  gate_input_.Read();

  parameters->trigger = previous_trigger_[0];  // added from here
  parameters->gate    = previous_gate_[0];
  for (int i = 0; i < kAdcLatency - 1; ++i) {
    previous_trigger_[i] = previous_trigger_[i + 1];
    previous_gate_[i]    = previous_gate_[i + 1];
  }
  previous_trigger_[kAdcLatency - 1] = gate_input_.trigger_rising_edge();
  previous_gate_[kAdcLatency - 1]    = gate_input_.gate();  // to here

  adc_.Convert();
}

}  // namespace clouds
