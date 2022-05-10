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
AudioMixer4           audio_mixer;
AudioConnection       patchCord1(i2s_in, 0, record_queue, 0);
AudioConnection       patchCord2(playRaw1, 0, audio_mixer, 0);
AudioConnection       patchCord3(playRaw2, 0, audio_mixer, 1);
AudioConnection       patchCord4(playRaw3, 0, audio_mixer, 2);
AudioConnection       patchCord5(audio_mixer, 0, i2s_out, 0);
AudioControlSGTL5000  sgtl5000_1;

const int lineInput = AUDIO_INPUT_LINEIN;

// SD interface (Teensy 4.x Audio board)
#define SDCARD_CS_PIN     10
#define SDCARD_MOSI_PIN   7
#define SDCARD_SCK_PIN    14

File record_file;

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
  PreRec,
  Rec,
  Play,
  Pause
};

struct LoopChannel {
  LoopState state;
  uint8_t id;
  char name[10];
  uint8_t *vol;
  uint8_t *led;
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

void loop() {
  check_encoder();
  
  check_pbs();
  
  if(screenNeedsUpdate) {
    update_display();
  }

  if (ledNeedsUpdate) {
    update_leds();
  }
}

void update_display() {
  display.clearDisplay();
  if (freshStart) {
    display_splash_screen();
    display.display();
    delay(500);
    knobPosition = (long)(knob.read() / 4);
    selectionZone = currentPage->defaultZone;
    freshStart = false;
  } else {
    display_main_bg();
    switch(currentPage->type) {
      case Menu:
        display_menu_bg();
        display_menu_items();
        break;
      case ValConfirm:
        display_confirm_page();
        break;
      case ValToggle:
        display_toggle_page();
        break;
      case ValTimeSig:
        display_timeSig_page();
        break;
      case ValNumeric:
        display_numeric_page();
        break;
      case ValText:
        display_save_as();
        break;
      default:
        break;
    }
    display_selectArea();
    display.display();
    screenNeedsUpdate = false;
  }
}

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

void display_numeric_page() {
  display_draw_topArc(64, 52, 34, SSD1306_WHITE);
  display.setTextSize(3);
  if (*editValue < 10) {
    display.setCursor(58, 28);
  } else if (*editValue < 100) {
    display.setCursor(48, 28);
  } else {
    display.setCursor(38, 28);
  }
  display.print(*editValue);
  display.setTextSize(1);
  display.setCursor(32,54);
  display.print(editValueName);
}

void display_toggle_page() {
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(12,24);
  display.print(editValueName);
  display.drawRect(16, 48, 32, 16, SSD1306_WHITE); // On
  display.drawRect(80, 48, 32, 16, SSD1306_WHITE); // Off
  display.setTextSize(1);
  display.setCursor(22,52);
  display.print("On");
  display.setCursor(88, 52);
  display.print("Off");
}

void display_timeSig_page() {
  display.setTextSize(3);
  display.setCursor(58, 28);
  display.print("/");
  display.setCursor(38, 28);
  display.print(TimeSig_val.top);
  display.setCursor(78, 28);
  display.print(TimeSig_val.bottom);
  display.setTextSize(1);
  display.setCursor(26,54);
  display.print("Time Signature");
}

void display_confirm_page() {
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(18,24);
  display.print("Confirm?");
  display.drawRect(16, 48, 32, 16, SSD1306_WHITE); // Confirm
  display.drawRect(80, 48, 32, 16, SSD1306_WHITE); // Cancel
  display.setTextSize(1);
  display.setCursor(22,52);
  display.print("yes");
  display.setCursor(88, 52);
  display.print("no");
}

void display_save_as() {
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  for (int i = 0; i < 4; i++) {
    display.drawRect(12 + (20 * i), 28, 16, 24, SSD1306_WHITE); // Character Box
    display.setCursor(15 + (20 * i), 33);
    display.print(SaveName_val[i]);
  }
  display.drawRect(92, 32, 28, 16, SSD1306_WHITE); // Confirm Save
  display.setTextSize(1);
  display.setCursor(94, 36);
  display.print("save");
}

void display_menu_items() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  for (int i = 0; i < 3; i++) {
    if (currentPage->items[i]) {
      display.setCursor(5, 20 + (16 * i));
      display.print(currentPage->items[i]->name);
      display.setCursor(100, 20 + (16 * i));
      display.print(get_value_for_menuItem(currentPage->items[i]));
    }
  }
}

void display_splash_screen() {
  display.setTextSize(4);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("YC");
  display.setCursor(0,32);
  display.print("LOOP");
  display.setTextSize(2);
  display.setCursor(72, 12);
  display.print("v1.0");
}

void display_main_bg() {
  display.drawRect(1,0,display.width()-2, 16, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(6,4);
  display.print("YC Looper v1.0");
}

void display_menu_bg() {
  display.drawFastHLine(2, 32, 122, SSD1306_WHITE);
  display.drawFastHLine(2, 48, 122, SSD1306_WHITE);
}

void display_val_bg() {
}

void display_draw_topArc(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    
    display.drawPixel(x0 + x, y0 - y, color);
    display.drawPixel(x0 + y, y0 - x, color);
    display.drawPixel(x0 - y, y0 - x, color);
    display.drawPixel(x0 - x, y0 - y, color);
  }
}

void display_selectArea() {
  int selIdx = selectionZone - 6;
  switch(selectionZone) {
    case None:
      break;
    case Menu1:
      display.fillRect(0, 16, 127, 16, SSD1306_INVERSE);
      break;
    case Menu2:
      display.fillRect(0, 32, 127, 16, SSD1306_INVERSE);
      break;
    case Menu3:
      display.fillRect(0, 48, 127, 16, SSD1306_INVERSE);
      break;
    case Confirm:
      display.fillRect(16, 48, 32, 16, SSD1306_INVERSE);
      break;
    case Cancel:
      display.fillRect(80, 48, 32, 16, SSD1306_INVERSE);
      break;
    case Save1:
    case Save2:
    case Save3:
    case Save4:
      if (textEdit) {
        display.drawFastHLine(12 + (selIdx * 20), 56, 16, SSD1306_WHITE); //underline
      } else {
        display.fillRect(12 + (selIdx * 20), 28, 16, 24, SSD1306_INVERSE);
      }
      break;
    case SaveConfirm:
      display.fillRect(92, 32, 28, 16, SSD1306_INVERSE);
      break;
    case TimeSig1:
      if (textEdit) {
        display.drawFastHLine(36, 50, 18, SSD1306_WHITE);
      } else {
        display.fillRect(36, 26, 18, 24, SSD1306_INVERSE);
      }
      break;
    case TimeSig2:
      if (textEdit) {
        display.drawFastHLine(76, 50, 18, SSD1306_WHITE);
      } else {
        display.fillRect(76, 26, 18, 24, SSD1306_INVERSE);
      }
      break;
    default:
      break;
  }
  
}

void display_go_back() {
  if (currentPage->back != nullptr) {
    currentPage = currentPage->back;
    selectionZone = currentPage->defaultZone;
    screenNeedsUpdate = true;
  }
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
    switch(channels[i]->state) {
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

void 

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

void init_display() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  delay(100);
  display.clearDisplay();
  display_splash_screen();
  display.display();
}

void init_leds() {
  pinMode(LED_CH_A, OUTPUT);
  pinMode(LED_CH_B, OUTPUT);
  pinMode(LED_CH_C, OUTPUT);
  pinMode(LED_PWR,  OUTPUT);
  pinMode(LED_TMPO, OUTPUT);
}

void init_pbs() {
  pinMode(PB_CH_A, INPUT_PULLUP);
  pinMode(PB_CH_B, INPUT_PULLUP);
  pinMode(PB_CH_C, INPUT_PULLUP);
  pinMode(PB_BACK, INPUT_PULLUP);
  pinMode(RE_SW,   INPUT_PULLUP);
}

void init_audio_shield() {
  // Audio Setup
  AudioMemory(60);
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(lineInput);
  sgtl5000_1.volume(VolMain_val/100);

  // SD Setup
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    while(1) {
      Serial.println("Unable to access the SD card");
      delay(1000);
    }
  }
}

void init_menus() {
  Channel_A = (struct LoopChannel){LoopState::Empty, 0, "Channel A", &VolA_val, &LED_CH_A};
  Channel_B = (struct LoopChannel){LoopState::Empty, 1, "Channel B", &VolB_val, &LED_CH_B};
  Channel_C = (struct LoopChannel){LoopState::Empty, 2, "Channel C", &VolC_val, &LED_CH_C};
  
  MainPage1      = (struct Page){{&main_setup, &main_mixing, &main_load}, PageType::Menu, nullptr, nullptr, &MainPage2, SelectionZone::Menu1};
  MainPage2      = (struct Page){{&main_save, &main_clear, nullptr}, PageType::Menu, nullptr, &MainPage1, nullptr, SelectionZone::Menu1};
  SetupPage1     = (struct Page){{&setup_tempo, &setup_timeSig, &setup_metronome}, PageType::Menu, &MainPage1, nullptr, &SetupPage2, SelectionZone::Menu1};
  SetupPage2     = (struct Page){{&setup_loopLen, nullptr, nullptr}, PageType::Menu, &MainPage1, &SetupPage1, nullptr, SelectionZone::Menu1};
  MixingPage1    = (struct Page){{&mixing_volMain, &mixing_volA, &mixing_volB}, PageType::Menu, &MainPage1, nullptr, &MixingPage2, SelectionZone::Menu1};
  MixingPage2    = (struct Page){{&mixing_volC, nullptr, nullptr}, PageType::Menu, &MainPage1, &MixingPage1, nullptr, SelectionZone::Menu1};
  LoadListPage   = (struct Page){{nullptr, nullptr, nullptr},        PageType::DynamicMenu, &MainPage1,    nullptr, nullptr, SelectionZone::Menu1};
  LoadActionPage = (struct Page){{&load_load, &load_delete, nullptr}, PageType::Menu,       &LoadListPage, nullptr, nullptr, SelectionZone::Menu1};
  SaveAsPage     = (struct Page){{nullptr, nullptr, nullptr},         PageType::ValText,    &MainPage1,    nullptr, nullptr, SelectionZone::Save1};
  ConfirmPage    = (struct Page){{nullptr, nullptr, nullptr},         PageType::ValConfirm,  EditBackPage, nullptr, nullptr, SelectionZone::Confirm};
  NumericPage    = (struct Page){{nullptr, nullptr, nullptr},         PageType::ValNumeric,  EditBackPage, nullptr, nullptr, SelectionZone::None};
  TogglePage     = (struct Page){{nullptr, nullptr, nullptr},         PageType::ValToggle,   EditBackPage, nullptr, nullptr, SelectionZone::Confirm};
  TimeSigPage    = (struct Page){{nullptr, nullptr, nullptr},         PageType::ValTimeSig,  EditBackPage, nullptr, nullptr, SelectionZone::TimeSig1};

  channels[0] = &Channel_A;
  channels[1] = &Channel_B;
  channels[2] = &Channel_C;
}
