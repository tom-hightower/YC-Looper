/*
 * timing.ino - Event handlers for IntervalTimers
 * Author: Tom Hightower - May 9, 2022
 */

void handleMetronomeInterval() {
  Serial.print("met beat: ");
  Serial.print(currentBeat);
  Serial.print(", Ch A state: ");
  Serial.print(channels[0]->state);
  Serial.print(", Timer state: ");
  Serial.println(timerState);
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
  Serial.println("handle loop");
  for (int i = 0; i < 3; i++) {
    switch (channels[i]->state) {
      case PreRec:
        channels[i]->state = LoopState::Rec;
        break;
      case Play:
        startPlaying(i);
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
  if (met && timerState == TimerState::NoTimer) {
    metronomeInterval.begin(handleMetronomeInterval, beat);
  } else if (met) {
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
