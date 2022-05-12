/*
 * controls.ino - Methods for reading the rotary encoder and pushbuttons
 * Author: Tom Hightower - May 9, 2022
 */

void check_encoder() {
  long newKnobPos = (long)(knob.read() / 4);
  if (newKnobPos != knobPosition) {
    bool knobRight = newKnobPos < knobPosition;
    switch (currentPage->type) {
      case Menu:
        if (knobRight && selectionZone < 3) {
          selectionZone = static_cast<SelectionZone>(selectionZone + 1);
        } else if (knobRight && currentPage->next != nullptr) {
          currentPage = currentPage->next;
          selectionZone = currentPage->defaultZone;
        } else if (!knobRight && selectionZone > 1) {
          selectionZone = static_cast<SelectionZone>(selectionZone - 1);
        } else if (!knobRight && currentPage->prev != nullptr) {
          currentPage = currentPage->prev;
          selectionZone = SelectionZone::Menu3;
        } else {
          break;
        }
        screenNeedsUpdate = true;
        break;
      case ValToggle:
        if (knobRight) {
          selectionZone = SelectionZone::Cancel;
          Metronome_val = false;
        } else {
          selectionZone = SelectionZone::Confirm;
          Metronome_val = true;
        }
        screenNeedsUpdate = true;
        break;
      case ValTimeSig:
        if (!textEdit && knobRight) {
          selectionZone = SelectionZone::TimeSig2;
        } else if (!textEdit && !knobRight) {
          selectionZone = SelectionZone::TimeSig1;
        } else if (textEdit) {
          if (knobRight && selectionZone == SelectionZone::TimeSig1 && TimeSig_val.top < 9) {
            TimeSig_val.top++;
          } else if (knobRight && selectionZone == SelectionZone::TimeSig2 && TimeSig_val.bottom < 16) {
            TimeSig_val.bottom *= 2;
          } else if (!knobRight && selectionZone == SelectionZone::TimeSig1 && TimeSig_val.top > 1) {
            TimeSig_val.top--;
          } else if (!knobRight && selectionZone == SelectionZone::TimeSig2 && TimeSig_val.bottom > 3) {
            TimeSig_val.bottom /= 2;
          } else {
            break;
          }
        }
        screenNeedsUpdate = true;
        break;
      case ValNumeric:
        if (knobRight && *editValue < maxEditValue) {
          (*editValue)++;
        } else if (!knobRight && *editValue > 0) {
          (*editValue)--;
        }
        screenNeedsUpdate = true;
        break;
      case ValText:
        if (!textEdit && knobRight && selectionZone < 10) {
          selectionZone = static_cast<SelectionZone>(selectionZone + 1);
        } else if (!textEdit && !knobRight && selectionZone > 6) {
          selectionZone = static_cast<SelectionZone>(selectionZone - 1);
        } else if (textEdit) {
          int editIdx = selectionZone - 6;
          if (knobRight) {
            SaveName_val[editIdx]++;
          } else if (!knobRight) {
            SaveName_val[editIdx]--;
          }
        } else {
          break;
        }
        screenNeedsUpdate = true;
        break;
      case ValConfirm:
        if (knobRight) {
          selectionZone = SelectionZone::Cancel;
        } else {
          selectionZone = SelectionZone::Confirm;
        }
        screenNeedsUpdate = true;
        break;
      default:
        break;
    }
    knobPosition = newKnobPos;
    Serial.print("Knob = ");
    Serial.print(knobPosition);
    Serial.println();
  }
}

unsigned long pressTimeA = 0;
unsigned long pressTimeB = 0;
unsigned long pressTimeC = 0;
unsigned long releaseTimeA = 0;
unsigned long releaseTimeB = 0;
unsigned long releaseTimeC = 0;

void check_pbs() {
  if (chA_Button.update()) {
    if (chA_Button.fallingEdge()) {
      channel_pb_press(0, &pressTimeA);
    }
    if (chA_Button.risingEdge()) {
      channel_pb_release(0, &pressTimeA, &releaseTimeA);
    }
  }
  if (chB_Button.update()) {
    if (chB_Button.fallingEdge()) {
      channel_pb_press(1, &pressTimeB);
    }
    if (chB_Button.risingEdge()) {
      channel_pb_release(1, &pressTimeB, &releaseTimeB);
    }
  }
  if (chC_Button.update()) {
    if (chC_Button.fallingEdge()) {
      channel_pb_press(2, &pressTimeC);
    }
    if (chC_Button.risingEdge()) {
      channel_pb_release(2, &pressTimeC, &releaseTimeC);
    }
  }
  if (backButton.update()) {
    if (backButton.fallingEdge()) {
      display_go_back();
    }
  }
  if (enterButton.update()) {
    if (enterButton.fallingEdge()) {
      switch (currentPage->type) {
        case Menu:
          if (currentPage->items[selectionZone - 1] != nullptr) {
            switch (currentPage->items[selectionZone - 1]->valueType) {
              case Tempo:
                editValue = &Tempo_val;
                break;
              case LoopLen:
                editValue = &LoopLen_val;
                break;
              case VolMain:
                editValue = &VolMain_val;
                break;
              case VolA:
                editValue = &VolA_val;
                break;
              case VolB:
                editValue = &VolB_val;
                break;
              case VolC:
                editValue = &VolC_val;
                break;
              default:
                break;
            }
            maxEditValue = currentPage->items[selectionZone - 1]->maxValue;
            editValueName = currentPage->items[selectionZone - 1]->name;
            currentPage = currentPage->items[selectionZone - 1]->next;
            selectionZone = currentPage->defaultZone;
            screenNeedsUpdate = true;
          }
          break;
        case ValToggle:
          display_go_back();
          break;
        case ValTimeSig:
        case ValText:
          if (selectionZone == SelectionZone::SaveConfirm) {
            save_project();
          } else {
            textEdit = !textEdit;
          }
          screenNeedsUpdate = true;
          break;
        default:
          break;
      }
    }
  }
}

void channel_pb_press(int channel, unsigned long *pressTime) {
  *pressTime = millis();
  switch(channels[channel]->state) {
    case Empty:
      change_channel_state_safe(channel, LoopState::PreRec);
      break;
    case Rec:
    case Play:
      change_channel_state_safe(channel, static_cast<LoopState>(channels[channel]->state + 1));
      break;
    case Pause:
      change_channel_state_safe(channel, LoopState::Play);
      break;
    default:
      break;
  }
  Serial.print("Button pressed: ");
  Serial.println(channel);
  if (channels[channel]->state == LoopState::PreRec && timerState == TimerState::NoTimer) {
    begin_timer(true);
    noInterrupts();
    timerState = TimerState::OnlyMet;
    interrupts();
  }
}

void channel_pb_release(int channel, unsigned long *pressTime, unsigned long *releaseTime) {
  *releaseTime = millis();
  if ((*releaseTime - *pressTime) > 2000) {
    change_channel_state_safe(channel, LoopState::Empty);
  }
}

void init_pbs() {
  pinMode(PB_CH_A, INPUT_PULLUP);
  pinMode(PB_CH_B, INPUT_PULLUP);
  pinMode(PB_CH_C, INPUT_PULLUP);
  pinMode(PB_BACK, INPUT_PULLUP);
  pinMode(RE_SW, INPUT_PULLUP);
}
