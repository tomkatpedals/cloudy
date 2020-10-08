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
// User interface.

#include "ui.h"

#include "clouds/cv_scaler.h"
#include "clouds/drivers/gate_input.h"
#include "clouds/dsp/granular_processor.h"
#include "clouds/meter.h"
#include "stmlib/system/system_clock.h"

namespace clouds {

const int32_t  kLongPressDuration = 1000;
const size_t   kNumPresetLeds     = 4;
const uint32_t kHoldOffDuration   = 100;  // UI ticks

using namespace stmlib;

void Ui::Splash(uint32_t clock) {
  uint8_t index = ((clock >> 8) + 1) & 3;
  uint8_t fade  = (clock >> 2);
  fade          = fade <= 127 ? (fade << 1) : 255 - (fade << 1);
  leds_.set_intensity(3 - index, fade);
}

void Ui::Init(Settings* settings, CvScaler* cv_scaler, GranularProcessor* processor, Meter* meter) {
  settings_  = settings;
  cv_scaler_ = cv_scaler;
  leds_.Init();
  switches_.Init();

  processor_ = processor;
  meter_     = meter;
  mode_      = UI_MODE_SPLASH;

  const State& state = settings_->state();

  // Sanitize saved settings.
  processor_->set_quality(state.quality & 3);
  processor_->set_playback_mode(
    static_cast<PlaybackMode>(state.playback_mode % PLAYBACK_MODE_LAST));
  if (switches_[SWITCH_WRITE]->pressed_immediate()) {
    mode_ = UI_MODE_CALIBRATION_1;
    switches_[SWITCH_WRITE]->reset();  // ignore release
  }
}

void Ui::SaveState() {
  State* state         = settings_->mutable_state();
  state->quality       = processor_->quality();
  state->playback_mode = processor_->playback_mode();
  settings_->Save();
}

void Ui::resetFlaggedSwitches(uint16_t flags) {
  for (size_t i = 0; i < kNumSwitches; i++) {
    if (flags & (1 << i)) {
      switches_[i]->reset();
    }
  }
}

void Ui::setFlaggedSwitchState(uint16_t flags, enum SwitchState state) {
  for (size_t i = 0; i < kNumSwitches; i++) {
    if (flags & (1 << i)) {
      switches_[i]->set_state(SwitchLongPressed);
    }
  }
}

void Ui::Poll() {
  system_clock.Tick();
  switches_.Scan();

  // Identify if we have a combo press first
  uint16_t combo_flags = 0;
  uint16_t combo_count = 0;
  uint16_t combo_id    = 0;
  for (uint8_t i = 0; i < kNumSwitches; ++i) {
    if (switches_[i]->state() != SwitchReleased) {
      combo_flags |= (1 << i);
      combo_count++;
    }
  }
  combo_id = combo_flags << fls(kNumSwitches - 1);

  for (uint8_t i = 0; i < kNumSwitches; ++i) {
    Switch* s = switches_[i];

    int32_t pressed_time = system_clock.milliseconds() - s->press_time();
    switch (s->state()) {
      case SwitchPressed:
        if (s->just_released()) {
          if (combo_count > 1) {
            resetFlaggedSwitches(combo_flags);
            queue_.AddEvent(CONTROL_SWITCH, combo_id, s->state());
          } else {
            s->reset();
            queue_.AddEvent(CONTROL_SWITCH, i, s->state());
          }
        } else if (pressed_time > kLongPressDuration) {
          if (combo_count > 1) {
            setFlaggedSwitchState(combo_flags, SwitchLongPressed);
            queue_.AddEvent(CONTROL_SWITCH, combo_id, s->state());
          } else {
            s->set_state(SwitchLongPressed);
            queue_.AddEvent(CONTROL_SWITCH, i, s->state());
          }
        }
        break;
      case SwitchLongPressed:
        if (s->just_released()) {
          s->reset();  // don't queue, redundant event
        }
        break;
      default:
        if (s->just_pressed()) {
          s->capture_press();  // don't queue, nothing in the queue cares
        }
    }
  }

  PaintLeds();
}

void Ui::PaintLeds() {
  leds_.Clear();
  uint32_t clock     = system_clock.milliseconds();
  bool     blink     = (clock & 0xFF) > 64;
  bool     flash     = (clock & 0x1FF) < 16;
  bool     slowflash = (clock & 0x3FF) < 16;
  uint8_t  fade      = __builtin_abs(2 * ((clock >> 1) - 128));
  fade               = static_cast<uint16_t>(fade) * fade >> 8;
  uint8_t slowfade   = __builtin_abs(2 * ((clock >> 2) - 128));
  slowfade           = static_cast<uint16_t>(slowfade) * slowfade >> 8;

  leds_.set_enabled(!processor_->bypass());

  switch (mode_) {
    case UI_MODE_SPLASH:
      Splash(clock);
      break;

    case UI_MODE_VU_METER:
      if (processor_->bypass()) {
        leds_.PaintBar(0);
      } else {
        leds_.PaintBar(lut_db[meter_->peak() >> 7]);
      }
      break;

    case UI_MODE_QUALITY:
      leds_.set_status(processor_->quality(), 255, 0);
      break;

    case UI_MODE_PLAYBACK_MODE:
      if (blink) {
        for (int i = 0; i < 4; i++)
          leds_.set_status(i, 0, 0);
      } else if (processor_->playback_mode() < 4) {
        leds_.set_status(processor_->playback_mode(), 128 + (fade >> 1), 255 - (fade >> 1));
      } else {
        for (int i = 0; i < 4; i++)
          leds_.set_status(i, 128 + (fade >> 1), 255 - (fade >> 1));
        leds_.set_status(processor_->playback_mode() & 3, 0, 0);
      }

      break;

    case UI_MODE_LOAD:
      VisualizeLoadLocation(slowfade, slowflash);
      break;

    case UI_MODE_SAVE:
      VisualizeSaveLocation(slowfade, slowflash);
      break;

    case UI_MODE_SAVING:
      leds_.set_status(load_save_location_, 255, 0);
      break;

    case UI_MODE_CALIBRATION_1:
      leds_.set_status(0, blink ? 255 : 0, blink ? 255 : 0);
      leds_.set_status(1, blink ? 255 : 0, blink ? 255 : 0);
      leds_.set_status(2, 0, 0);
      leds_.set_status(3, 0, 0);
      break;

    case UI_MODE_CALIBRATION_2:
      leds_.set_status(0, blink ? 255 : 0, blink ? 255 : 0);
      leds_.set_status(1, blink ? 255 : 0, blink ? 255 : 0);
      leds_.set_status(2, blink ? 255 : 0, blink ? 255 : 0);
      leds_.set_status(3, blink ? 255 : 0, blink ? 255 : 0);
      break;

    case UI_MODE_PANIC:
      leds_.set_status(0, 255, 0);
      leds_.set_status(1, 255, 0);
      leds_.set_status(2, 255, 0);
      leds_.set_status(3, 255, 0);
      break;

    default:
      break;
  }

  bool freeze = processor_->frozen();
  if (processor_->reversed()) {
    freeze ^= flash;
  }
  leds_.set_freeze(freeze);

  leds_.Write();
}

void Ui::FlushEvents() {
  queue_.Flush();
}

void Ui::CalibrateC1() {
  cv_scaler_->CalibrateC1();
  cv_scaler_->CalibrateOffsets();
  mode_ = UI_MODE_CALIBRATION_2;
}

void Ui::CalibrateC3() {
  bool success = cv_scaler_->CalibrateC3();
  if (success) {
    settings_->Save();
    mode_ = UI_MODE_VU_METER;
  } else {
    mode_ = UI_MODE_PANIC;
  }
}

void Ui::OnSwitchEvent(const Event& e) {
  // Currently we care about LongPressed and Released only
  if (e.data == SwitchPressed) {
    return;
  }

  switch (mode_) {
    case UI_MODE_CALIBRATION_1:
      switch (e.control_id) {
        case SWITCH_WRITE:
          CalibrateC1();
          break;
        default:
          mode_ = UI_MODE_VU_METER;
      }
      break;

    case UI_MODE_CALIBRATION_2:
      switch (e.control_id) {
        case SWITCH_WRITE:
          CalibrateC3();
          break;
        default:
          mode_ = UI_MODE_VU_METER;
      }
      break;

    case UI_MODE_LOAD:
      switch (e.control_id) {
        case SWITCH_FREEZE:
          // fallthrough
        case SWITCH_MODE:
          switch (e.data) {
            case SwitchLongPressed:
              LoadPreset();
              mode_ = UI_MODE_VU_METER;
              break;
            case SwitchReleased:
              IncrementLoadSaveLocation();
              break;
            default:
              break;  // Do nothing
          }
          break;
        default:
          mode_ = UI_MODE_VU_METER;
      }
      break;

    case UI_MODE_PLAYBACK_MODE:
      switch (e.control_id) {
        case SWITCH_MODE:
          DecrementPlaybackMode();
          break;
        case SWITCH_WRITE:
          IncrementPlaybackMode();
          break;
        default:
          mode_ = UI_MODE_VU_METER;
      }
      break;

    case UI_MODE_QUALITY:
      switch (e.control_id) {
        case SWITCH_MODE:
          switch (e.data) {
            case SwitchReleased:
              IncrementQuality();
              break;
            default:
              break;
          }
          break;
        default:
          mode_ = UI_MODE_VU_METER;
      }
      break;

    case UI_MODE_SAVE:
      switch (e.control_id) {
        case SWITCH_WRITE:
          switch (e.data) {
            case SwitchLongPressed:
              mode_ = UI_MODE_SAVING;
              SavePreset();
              mode_ = UI_MODE_VU_METER;
              break;
            case SwitchReleased:
              IncrementLoadSaveLocation();
              break;
            default:
              break;  // do nothing
          }
          break;
        default:
          mode_ = UI_MODE_VU_METER;
      }
      break;

    case UI_MODE_VU_METER:
      switch (e.control_id) {
        case SWITCH_MODE:
          switch (e.data) {
            case SwitchLongPressed:
              mode_ = UI_MODE_LOAD;
              break;
            case SwitchReleased:
              mode_ = UI_MODE_QUALITY;
              break;
            default:
              break;  // Do nothing
          }
          break;
        case SWITCH_WRITE:
          switch (e.data) {
            case SwitchLongPressed:
              mode_ = UI_MODE_SAVE;
              break;
            case SwitchReleased:
              mode_ = UI_MODE_PLAYBACK_MODE;
              break;
            default:
              break;  // do nothing
          }
          break;
        case SWITCH_BYPASS:
          switch (e.data) {
            case SwitchLongPressed:
              processor_->set_inf_reverb(true);
              break;
            case SwitchReleased:
              processor_->ToggleBypass();
              break;
            default:
              break;
          }
          break;
        case SWITCH_FREEZE:
          switch (e.data) {
            case SwitchLongPressed:
              processor_->ToggleReverse();
              break;
            case SwitchReleased:
              processor_->ToggleFreeze();
              break;
            default:
              break;
          }
          break;
        case SWITCH_COMBO_FREEZE_BYPASS:
          switch (e.data) {
            case SwitchLongPressed:
              mode_ = UI_MODE_LOAD;
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }
      break;

    case UI_MODE_PANIC:
      // fallthrough
    case UI_MODE_SAVING:
      // fallthrough
    case UI_MODE_SPLASH:
      // fallthrough
    default:
      break;
  }
}  // namespace clouds

void Ui::LoadPreset(void) {
  processor_->LoadPreset(settings_->ConstPreset(load_save_bank_, load_save_location_));
  IncrementLoadSaveLocation();
}

void Ui::DoEvents() {
  while (queue_.available()) {
    Event e = queue_.PullEvent();
    if (e.control_type == CONTROL_SWITCH) {
      OnSwitchEvent(e);
    }
  }

  if (queue_.idle_time() > 1000 && mode_ == UI_MODE_PANIC) {
    queue_.Touch();
    mode_ = UI_MODE_VU_METER;
  }

  if (queue_.idle_time() > 3000 &&
      (mode_ == UI_MODE_QUALITY || mode_ == UI_MODE_PLAYBACK_MODE || mode_ == UI_MODE_SPLASH)) {
    queue_.Touch();
    mode_ = UI_MODE_VU_METER;
  }

  if (queue_.idle_time() > 6000) {
    queue_.Touch();
    if (mode_ == UI_MODE_SAVE || mode_ == UI_MODE_LOAD) {
      mode_ = UI_MODE_VU_METER;
    }
  }

  if (processor_->inf_reverb() && !switches_[SWITCH_BYPASS]->pressed()) {
    processor_->set_inf_reverb(false);
  }
}

uint8_t Ui::HandleFactoryTestingRequest(uint8_t command) {
  uint8_t argument = command & 0x1f;
  command          = command >> 5;
  uint8_t reply    = 0;
  switch (command) {
    case FACTORY_TESTING_READ_POT:
    case FACTORY_TESTING_READ_CV:
      reply = cv_scaler_->adc_value(argument);
      break;

    case FACTORY_TESTING_READ_GATE:
      if (argument <= 2) {
        return switches_[argument]->pressed();
      } else {
        return cv_scaler_->gate(argument - 3);
      }
      break;

    case FACTORY_TESTING_SET_BYPASS:
      processor_->set_bypass(argument);
      break;

    case FACTORY_TESTING_CALIBRATE:
      if (argument == 0) {
        mode_ = UI_MODE_CALIBRATION_1;
      } else if (argument == 1) {
        CalibrateC1();
      } else {
        CalibrateC3();
        SaveState();
      }
      break;
  }
  return reply;
}

void Ui::SavePreset(void) {
  processor_->set_silence(true);
  processor_->ExportPreset(settings_->Preset(load_save_bank_, load_save_location_));
  settings_->SavePresets();
  processor_->set_silence(false);
  IncrementLoadSaveLocation();
}

void Ui::IncrementLoadSaveLocation(void) {
  settings_->IncrementPresetLocation(load_save_bank_, load_save_location_);
}

void Ui::DecrementPlaybackMode(void) {
  uint8_t mode =
    processor_->playback_mode() == 0 ? PLAYBACK_MODE_LAST - 1 : processor_->playback_mode() - 1;
  processor_->set_playback_mode(static_cast<PlaybackMode>(mode));
  SaveState();
}

void Ui::IncrementPlaybackMode(void) {
  uint8_t mode = (processor_->playback_mode() + 1) % PLAYBACK_MODE_LAST;
  processor_->set_playback_mode(static_cast<PlaybackMode>(mode));
  SaveState();
}

void Ui::IncrementQuality(void) {
  processor_->set_quality((processor_->quality() + 1) & 3);
  SaveState();
}

void Ui::VisualizeLoadLocation(uint8_t fade, bool flash) {
  uint8_t red   = (load_save_bank_ & 1) ? 0 : 255;
  uint8_t white = (load_save_bank_ & 3) ? 255 : 0;
  for (size_t i = 0; i < kNumPresetLeds; i++) {
    leds_.set_status(i, fade & red, fade & white);
  }
  leds_.set_status(load_save_location_, flash ? red : 0, flash ? white : 0);
}

void Ui::VisualizeSaveLocation(uint8_t fade, bool flash) {
  uint8_t red   = (load_save_bank_ & 1) ? 0 : 255;
  uint8_t white = (load_save_bank_ & 3) ? 255 : 0;
  for (size_t i = 0; i < kNumPresetLeds; i++) {
    leds_.set_status(i, flash ? red : 0, flash ? white : 0);
  }
  leds_.set_status(load_save_location_, fade & red, fade & white);
}

}  // namespace clouds
