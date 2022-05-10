
void check_encoder() {
  long newKnobPos = (long)(knob.read() / 4);
  if (newKnobPos != knobPosition) {
    bool knobRight = newKnobPos < knobPosition;
    switch(currentPage->type) {
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
      pressTimeA = millis();
      if (Channel_A.state < 4) {
        Channel_A.state = static_cast<LoopState>(Channel_A.state + 1);
      } else {
        Channel_A.state = LoopState::Play;
      }
    }
    if (chA_Button.risingEdge()) {
      releaseTimeA = millis();
      if ((releaseTimeA - pressTimeA) > 2000) {
        Channel_A.state = LoopState::Empty;
      }
    }
  }
  if (chB_Button.update()) {
    if (chB_Button.fallingEdge()) {
      pressTimeB = millis();
      if (Channel_B.state < 4) {
        Channel_B.state = static_cast<LoopState>(Channel_B.state + 1);
      } else {
        Channel_B.state = LoopState::Play;
      }
    }
    if (chB_Button.risingEdge()) {
      releaseTimeB = millis();
      if ((releaseTimeB - pressTimeB) > 2000) {
        Channel_B.state = LoopState::Empty;
      }
    }
  }
  if (chC_Button.update()) {
    if (chC_Button.fallingEdge()) {
      pressTimeC = millis();
      if (Channel_C.state < 4) {
        Channel_C.state = static_cast<LoopState>(Channel_C.state + 1);
      } else {
        Channel_C.state = LoopState::Play;
      }
    }
    if (chC_Button.risingEdge()) {
      releaseTimeC = millis();
      if ((releaseTimeC - pressTimeC) > 2000) {
        Channel_C.state = LoopState::Empty;
      }
    }
  }
  if (backButton.update()) {
    if (backButton.fallingEdge()) {
      display_go_back();
    }
  }
  if (enterButton.update()) {
    if (enterButton.fallingEdge()) {
      switch(currentPage->type) {
        case Menu:
          if (currentPage->items[selectionZone-1] != nullptr) {
            switch(currentPage->items[selectionZone-1]->valueType) {
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
            maxEditValue = currentPage->items[selectionZone-1]->maxValue;
            editValueName = currentPage->items[selectionZone-1]->name;
            currentPage = currentPage->items[selectionZone-1]->next;
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

void init_pbs() {
  pinMode(PB_CH_A, INPUT_PULLUP);
  pinMode(PB_CH_B, INPUT_PULLUP);
  pinMode(PB_CH_C, INPUT_PULLUP);
  pinMode(PB_BACK, INPUT_PULLUP);
  pinMode(RE_SW,   INPUT_PULLUP);
}
