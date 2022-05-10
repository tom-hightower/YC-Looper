
void handleMetronomeInterval(void) {
  if (currentBeat == 0) {
    AudioNoInterrupts();
    firstBeatWaveform.amplitude(1.0);
    delay(50);
    firstBeatWaveform.amplitude(0.0);
    AudioInterrupts();
    currentBeat++;
  } else {
    AudioNoInterrupts();
    otherBeatWaveform.amplitude(1.0);
    delay(50);
    otherBeatWaveform.amplitude(0.0);
    AudioInterrupts();
    if (currentBeat == TimeSig_val.top) {
      currentBeat = 0;
    } else {
      currentBeat++;
    }
  }
}

void handleLoopInterval(void) {
  for (int i = 0; i < 3; i++) {
    switch(channels[i]->state) {
      case PreRec:
        channels[i]->state = LoopState::Rec;
        break;
      case Play:
        channels[i]->playRaw->stop();
        channels[i]->playRaw->play(fileList[i]);
        break;
      case Rec:
        channels[i]->state = LoopState::Play;
        break;
      default:
        break;
    }
  }
}
