/*
 * menus.ino - Methods for navigation and page handling
 * Author: Tom Hightower - May 10, 2022
 */

 
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

  main_setup      = (struct MenuItem) {"Setup",         &SetupPage1,   ValueType::NoValue, 0};
  main_mixing     = (struct MenuItem) {"Mixing",        &MixingPage1,  ValueType::NoValue, 0};
  main_load       = (struct MenuItem) {"Load",          &LoadListPage, ValueType::NoValue, 0};
  main_save       = (struct MenuItem) {"Save As",       &SaveAsPage,   ValueType::NoValue, 0};
  main_clear      = (struct MenuItem) {"Clear Session", &ConfirmPage,  ValueType::NoValue, 0};
  
  setup_tempo     = (struct MenuItem) {"Tempo",       &NumericPage, ValueType::Tempo,     180};
  setup_timeSig   = (struct MenuItem) {"Time Sig",    &TimeSigPage, ValueType::TimeSig,   0};
  setup_metronome = (struct MenuItem) {"Metronome",   &TogglePage,  ValueType::Metronome, 0};
  setup_loopLen   = (struct MenuItem) {"Loop Length", &NumericPage, ValueType::LoopLen,   32};
  
  mixing_volMain  = (struct MenuItem) {"Main Volume", &NumericPage, ValueType::VolMain, 100};
  mixing_volA     = (struct MenuItem) {"Ch A Vol",    &NumericPage, ValueType::VolA,    100};
  mixing_volB     = (struct MenuItem) {"Ch B Vol",    &NumericPage, ValueType::VolB,    100};
  mixing_volC     = (struct MenuItem) {"Ch C Vol",    &NumericPage, ValueType::VolC,    100};
  
  load_load       = (struct MenuItem) {"Load",   &MainPage1,   ValueType::NoValue, 0};
  load_delete     = (struct MenuItem) {"Delete", &ConfirmPage, ValueType::NoValue, 0};

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
