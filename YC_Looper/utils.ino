/*
 * utils.ino - Helpful methods that don't belong anywhere else
 * Author: Tom Hightower - May 10, 2022
 */
 
bool any_channel_recording() {
  return  Channel_A.state == LoopState::Rec ||
          Channel_B.state == LoopState::Rec ||
          Channel_C.state == LoopState::Rec;
}

String get_value_for_menuItem(MenuItem *item) {
  switch (item->valueType) {
    case Tempo:
      return String(Tempo_val);
    case TimeSig:
      char strBuf[3];
      sprintf(strBuf, "%d:%d", TimeSig_val.top, TimeSig_val.bottom);
      return String(strBuf);
    case Metronome:
      return Metronome_val ? "On" : "Off";
    case LoopLen:
      return String(LoopLen_val);
    case VolMain:
      return String(VolMain_val);
    case VolA:
      return String(VolA_val);
    case VolB:
      return String(VolB_val);
    case VolC:
      return String(VolC_val);
    default:
      break;
  }
  return "";
}
