#include <Encoder.h>
#include <Audio.h>
#include <Wire.h>               // SCL pin 19, SDA pin 18
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Audio Setup
AudioInputI2S         i2s_in;
AudioOutputI2S        i2s_out;
AudioRecordQueue      record_queue;
AudioPlaySdRaw        playRaw1;
AudioPlaySdRaw        playRaw2;
AudioPlaySdRaw        playRaw3;
AudioSynthWaveform    firstBeatWaveform;
AudioSynthWaveform    otherBeatWaveform;
AudioMixer4           audio_mixer;
AudioMixer4           waveform_mixer;
AudioConnection       patchCord1(i2s_in, 0, record_queue, 0);
AudioConnection       patchCord2(playRaw1, 0, audio_mixer, 0);
AudioConnection       patchCord3(playRaw2, 0, audio_mixer, 1);
AudioConnection       patchCord4(playRaw3, 0, audio_mixer, 2);
AudioConnection       patchCord5(firstBeatWaveform, 0, waveform_mixer, 0);
AudioConnection       patchCord6(otherBeatWaveform, 0, waveform_mixer, 0);
AudioConnection       patchCord7(waveform_mixer, 0, audio_mixer, 3);
AudioConnection       patchCord8(audio_mixer, 0, i2s_out, 0);
AudioControlSGTL5000  sgtl5000_1;

const int lineInput = AUDIO_INPUT_LINEIN;

// SD interface (Teensy 4.x Audio board)
#define SDCARD_CS_PIN     10
#define SDCARD_MOSI_PIN   7
#define SDCARD_SCK_PIN    14

File record_file;

const char * fileList[3] = {
  "CHA.RAW", "CHB.RAW", "CHC.RAW"
};

enum RecordingChannel {
  NotRecording = 0,
  ChA,
  ChB,
  ChC
};

RecordingChannel recordingChannel;

// Timing
IntervalTimer metronomeInterval;
IntervalTimer loopInterval;

volatile uint8_t currentBeat = 0;

// Menu Setup
enum SelectionZone {
  None = 0,
  Menu1,
  Menu2,
  Menu3,
  Confirm,
  Cancel,
  Save1,
  Save2,
  Save3,
  Save4,
  SaveConfirm,
  TimeSig1,
  TimeSig2,
  TimeSigConfirm
};

enum PageType {
  Menu = 0,
  DynamicMenu,
  ValToggle,
  ValTimeSig,
  ValNumeric,
  ValText,
  ValConfirm
};

enum LoopState {
  Empty = 0,
  Rec,
  Play,
  Pause,
  PreRec
};

struct LoopChannel {
  volatile LoopState state;
  uint8_t id;
  char name[10];
  uint8_t *vol;
  uint8_t *led;
  AudioPlaySdRaw *playRaw;
};

struct TimeSignature {
  uint8_t top;
  uint8_t bottom;
};

TimeSignature defaultTimeSig = {4, 4};

// SetupValues
uint8_t Tempo_val = 120;
TimeSignature TimeSig_val = defaultTimeSig;
bool Metronome_val = false;
uint8_t LoopLen_val = 4;
uint8_t VolMain_val = 75;
uint8_t VolA_val = 100;
uint8_t VolB_val = 100;
uint8_t VolC_val = 100;
String SaveName_val = "0000";

struct LoopChannel *channels[3];

enum ValueType {
  NoValue = 0,
  Tempo,
  TimeSig,
  Metronome,
  LoopLen,
  VolMain,
  VolA,
  VolB,
  VolC,
  ConfirmVal,
  SaveName
};

struct Page;

struct MenuItem {
  String name;
  Page *next;
  ValueType valueType;
  unsigned int maxValue;
};

struct Page {
  MenuItem *items [3];
  PageType type;
  Page *back;
  Page *prev;
  Page *next;
  SelectionZone defaultZone;
};

Page MainPage1;
Page MainPage2;
Page SetupPage1;
Page SetupPage2;
Page MixingPage1;
Page MixingPage2;
Page LoadListPage;
Page LoadActionPage;
Page SaveAsPage;
Page ConfirmPage;
Page NumericPage;
Page TogglePage;
Page TimeSigPage;

Page *EditBackPage = &MainPage1;

MenuItem main_setup  = {"Setup",         &SetupPage1,   ValueType::NoValue, 0};
MenuItem main_mixing = {"Mixing",        &MixingPage1,  ValueType::NoValue, 0};
MenuItem main_load   = {"Load",          &LoadListPage, ValueType::NoValue, 0};
MenuItem main_save   = {"Save As",       &SaveAsPage,   ValueType::NoValue, 0};
MenuItem main_clear  = {"Clear Session", &ConfirmPage,  ValueType::NoValue, 0};

MenuItem setup_tempo     = {"Tempo",       &NumericPage, ValueType::Tempo,     180};
MenuItem setup_timeSig   = {"Time Sig",    &TimeSigPage, ValueType::TimeSig,   0};
MenuItem setup_metronome = {"Metronome",   &TogglePage,  ValueType::Metronome, 0};
MenuItem setup_loopLen   = {"Loop Length", &NumericPage, ValueType::LoopLen,   32};

MenuItem mixing_volMain = {"Main Volume", &NumericPage, ValueType::VolMain, 100};
MenuItem mixing_volA    = {"Ch A Vol",    &NumericPage, ValueType::VolA,    100};
MenuItem mixing_volB    = {"Ch B Vol",    &NumericPage, ValueType::VolB,    100};
MenuItem mixing_volC    = {"Ch C Vol",    &NumericPage, ValueType::VolC,    100};

MenuItem load_load   = {"Load",   &MainPage1,   ValueType::NoValue, 0};
MenuItem load_delete = {"Delete", &ConfirmPage, ValueType::NoValue, 0};

uint8_t RE_CLK   = 0;
uint8_t RE_DT    = 1;
uint8_t RE_SW    = 2;

uint8_t LED_CH_A = 3;
uint8_t LED_CH_B = 4;
uint8_t LED_CH_C = 5;
uint8_t LED_PWR  = 16;
uint8_t LED_TMPO = 17;

uint8_t PB_CH_A  = 24; // Most buttons need ~10-15ms bounce val
uint8_t PB_CH_B  = 26;
uint8_t PB_CH_C  = 28;
uint8_t PB_BACK  = 30; // Probably needs a higher bounce value (50-100?) TODO: switch back to pin 30 for final

int OLED_ADDR = 0x78;

Encoder knob(RE_CLK, RE_DT);
Adafruit_SSD1306 display(128, 64, &Wire, -1, 1000000);  // 1MHz I2C clock

LoopChannel Channel_A;
LoopChannel Channel_B;
LoopChannel Channel_C;

// Button creation
Bounce enterButton = Bounce(RE_SW,   10);
Bounce backButton  = Bounce(PB_BACK, 50);
Bounce chA_Button  = Bounce(PB_CH_A, 10);
Bounce chB_Button  = Bounce(PB_CH_B, 10);
Bounce chC_Button  = Bounce(PB_CH_C, 10);

void setup() {
  Serial.begin(9600);
  Serial.println("YC Looper Test");
  init_display();
  init_leds();
  init_pbs();
  init_audio_shield();
  init_menus();
}

long knobPosition = -999;
bool screenNeedsUpdate = true;
bool ledNeedsUpdate = true;
bool freshStart = true;
bool textEdit = false;
SelectionZone selectionZone = SelectionZone::Menu1;
uint8_t *editValue = &Tempo_val;
uint8_t maxEditValue = 100;
String editValueName = "";
Page *currentPage = &MainPage1;

// MAIN LOOP
void loop() {
  if (any_channel_recording()) try_record();

  check_encoder();
  check_pbs();

  if (screenNeedsUpdate) {
    update_display();
  }

  if (ledNeedsUpdate) {
    update_leds();
  }
}

bool any_channel_recording() {
  return  Channel_A.state == LoopState::Rec ||
          Channel_B.state == LoopState::Rec ||
          Channel_C.state == LoopState::Rec;
}

void save_project() {
  //TODO
  SaveName_val = "0000";
  display_go_back();
}

void load_projects() {
  //TODO
}

void update_leds() {
  for (int i = 0; i < 3; i++) {
    switch (channels[i]->state) {
      case Empty:
        digitalWrite(*channels[i]->led, LOW); // OFF
        break;
      case PreRec:
        digitalWrite(*channels[i]->led, HIGH); // TODO: fast blink
        break;
      case Rec:
        digitalWrite(*channels[i]->led, HIGH); // TODO: blink with tempo
        break;
      case Play:
        digitalWrite(*channels[i]->led, HIGH); // ON
        break;
      case Pause:
        digitalWrite(*channels[i]->led, LOW); // TODO: blink once per bar
        break;
    }
  }
}

void playFile(LoopChannel channel) {
  channel.playRaw->play(fileList[channel.id]);
}

void init_leds() {
  pinMode(LED_CH_A, OUTPUT);
  pinMode(LED_CH_B, OUTPUT);
  pinMode(LED_CH_C, OUTPUT);
  pinMode(LED_PWR,  OUTPUT);
  pinMode(LED_TMPO, OUTPUT);
}

void init_audio_shield() {
  // Audio Setup
  AudioMemory(120);
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(lineInput);
  sgtl5000_1.volume(VolMain_val / 100);
  firstBeatWaveform.frequency(880);
  otherBeatWaveform.frequency(440);
  firstBeatWaveform.amplitude(0.0);
  otherBeatWaveform.amplitude(0.0);
  firstBeatWaveform.begin(WAVEFORM_SINE);
  otherBeatWaveform.begin(WAVEFORM_SINE);

  // SD Setup
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(1000);
    }
  }
}

void init_menus() {
  Channel_A = (struct LoopChannel) {
    LoopState::Empty, 0, "Channel A", &VolA_val, &LED_CH_A, &playRaw1
  };
  Channel_B = (struct LoopChannel) {
    LoopState::Empty, 1, "Channel B", &VolB_val, &LED_CH_B, &playRaw2
  };
  Channel_C = (struct LoopChannel) {
    LoopState::Empty, 2, "Channel C", &VolC_val, &LED_CH_C, &playRaw3
  };

  MainPage1      = (struct Page) { {&main_setup, &main_mixing, &main_load}, PageType::Menu, nullptr, nullptr, &MainPage2, SelectionZone::Menu1
  };
  MainPage2      = (struct Page) { {&main_save, &main_clear, nullptr}, PageType::Menu, nullptr, &MainPage1, nullptr, SelectionZone::Menu1
  };
  SetupPage1     = (struct Page) { {&setup_tempo, &setup_timeSig, &setup_metronome}, PageType::Menu, &MainPage1, nullptr, &SetupPage2, SelectionZone::Menu1
  };
  SetupPage2     = (struct Page) { {&setup_loopLen, nullptr, nullptr}, PageType::Menu, &MainPage1, &SetupPage1, nullptr, SelectionZone::Menu1
  };
  MixingPage1    = (struct Page) { {&mixing_volMain, &mixing_volA, &mixing_volB}, PageType::Menu, &MainPage1, nullptr, &MixingPage2, SelectionZone::Menu1
  };
  MixingPage2    = (struct Page) { {&mixing_volC, nullptr, nullptr}, PageType::Menu, &MainPage1, &MixingPage1, nullptr, SelectionZone::Menu1
  };
  LoadListPage   = (struct Page) { {nullptr, nullptr, nullptr},        PageType::DynamicMenu, &MainPage1,    nullptr, nullptr, SelectionZone::Menu1
  };
  LoadActionPage = (struct Page) { {&load_load, &load_delete, nullptr}, PageType::Menu,       &LoadListPage, nullptr, nullptr, SelectionZone::Menu1
  };
  SaveAsPage     = (struct Page) { {nullptr, nullptr, nullptr},         PageType::ValText,    &MainPage1,    nullptr, nullptr, SelectionZone::Save1
  };
  ConfirmPage    = (struct Page) { {nullptr, nullptr, nullptr},         PageType::ValConfirm,  EditBackPage, nullptr, nullptr, SelectionZone::Confirm
  };
  NumericPage    = (struct Page) { {nullptr, nullptr, nullptr},         PageType::ValNumeric,  EditBackPage, nullptr, nullptr, SelectionZone::None
  };
  TogglePage     = (struct Page) { {nullptr, nullptr, nullptr},         PageType::ValToggle,   EditBackPage, nullptr, nullptr, SelectionZone::Confirm
  };
  TimeSigPage    = (struct Page) { {nullptr, nullptr, nullptr},         PageType::ValTimeSig,  EditBackPage, nullptr, nullptr, SelectionZone::TimeSig1
  };

  channels[0] = &Channel_A;
  channels[1] = &Channel_B;
  channels[2] = &Channel_C;
  
  recordingChannel = RecordingChannel::NotRecording;
}
