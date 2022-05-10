
void try_record() {
  for (int i = 0; i < 3; i++) {
    switch(channels[i]->state) {
      case Rec:
        if (recordingChannel != i+1 && recordingChannel != RecordingChannel::NotRecording) {
          channels[i]->state = LoopState::Empty;
        } else if (recordingChannel == RecordingChannel::NotRecording) {
          startRecording(i);
          recordingChannel = static_cast<RecordingChannel>(i+1);
        } else {
          continueRecording();
        }
        goto found_rec;
        break;
      case Play:
        if (recordingChannel == i+1) {
          stopRecording();
          recordingChannel = RecordingChannel::NotRecording;
        }
        break;
      default:
        break;
    }
  }
  found_rec:;
}

void startRecording(int channel) {
  if (SD.exists(fileList[channel])) {
    SD.remove(fileList[channel]);
  }
  record_file = SD.open(fileList[channel]);
  if (record_file) {
    record_queue.begin();
  }
}

void continueRecording() {
  if (record_queue.available() >= 2) {
    byte buffer[512];
    memcpy(buffer, record_queue.readBuffer(), 256);
    record_queue.freeBuffer();
    memcpy(buffer + 256, record_queue.readBuffer(), 256);
    record_queue.freeBuffer();
    record_file.write(buffer, 512);
  }
}

void stopRecording() {
  record_queue.end();
  while (record_queue.available() > 0) {
    record_file.write((byte*)record_queue.readBuffer(), 256);
    record_queue.freeBuffer();
  }
  record_file.close();
}
