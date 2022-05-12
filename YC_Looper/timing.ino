/*
 * timing.ino - Event handlers for IntervalTimers
 * Author: Tom Hightower - May 9, 2022
 */

void handleMetronomeInterval() {
  Serial.print("Metronome fire at beat: ");
  Serial.println(currentBeat);
  if (currentBeat == TimeSig_val.top - 1) {
    currentBeat = 0;
    if (timerState == TimerState::OnlyMet) {
      begin_timer(false);
      handleLoopInterval();
      timerState = TimerState::Both;
    }
  } else {
    currentBeat++;
  }
  if (Metronome_val) triggerMet = true;
}

void handleLoopInterval() {
  for (int i = 0; i < 3; i++) {
    switch (channels[i]->state) {
      case PreRec:
        channels[i]->state = LoopState::Rec;
        break;
      case Play:
        if (!channels[i]->playRaw->isPlaying()) {
          startPlaying(i);
        }
        break;
      case Rec:
        channels[i]->state = LoopState::Play;
        break;
      default:
        break;
    }
  }
}

void change_channel_state_safe(int channel, LoopState newState) {
  noInterrupts();
  channels[channel]->state = newState;
  interrupts();
}

void stop_timers() {
  metronomeInterval.end();
  loopInterval.end();
}

void begin_timer(bool met) {
  unsigned long beat = 60000000 / Tempo_val;
  Serial.print("Tempo: ");
  Serial.print(Tempo_val);
  Serial.print(", Beat: ");
  Serial.print(60/Tempo_val);
  Serial.print(", converted is: ");
  Serial.println(beat);
  if (met && timerState == TimerState::NoTimer) {
    Serial.println("beginning timer for: metronome");
    metronomeInterval.begin(handleMetronomeInterval, beat);
  } else if (met) {
    Serial.println("beginning timer for: metronome 2");
    metronomeInterval.end();
    metronomeInterval.begin(handleMetronomeInterval, beat);
  } else if (timerState != TimerState::Both) {
    loopInterval.begin(handleLoopInterval, beat * TimeSig_val.top * LoopLen_val);
  } else {
    loopInterval.end();
    loopInterval.begin(handleLoopInterval, beat * TimeSig_val.top * LoopLen_val);
  }
}

void init_timers() {
  metronomeInterval.priority(96);
  loopInterval.priority(94);  
}
