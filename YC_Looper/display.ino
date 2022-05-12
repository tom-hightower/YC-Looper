/*
 * display.ino - Display update methods and drawing helpers
 * Author: Tom Hightower - May 9, 2022
 */

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
    switch (currentPage->type) {
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
  display.setCursor(32, 54);
  display.print(editValueName);
}

void display_toggle_page() {
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(12, 24);
  display.print(editValueName);
  display.drawRect(16, 48, 32, 16, SSD1306_WHITE); // On
  display.drawRect(80, 48, 32, 16, SSD1306_WHITE); // Off
  display.setTextSize(1);
  display.setCursor(22, 52);
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
  display.setCursor(26, 54);
  display.print("Time Signature");
}

void display_confirm_page() {
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(18, 24);
  display.print("Confirm?");
  display.drawRect(16, 48, 32, 16, SSD1306_WHITE); // Confirm
  display.drawRect(80, 48, 32, 16, SSD1306_WHITE); // Cancel
  display.setTextSize(1);
  display.setCursor(22, 52);
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
  display.setCursor(0, 32);
  display.print("LOOP");
  display.setTextSize(2);
  display.setCursor(72, 12);
  display.print("v1.0");
}

void display_main_bg() {
  display.drawRect(1, 0, display.width() - 2, 16, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(6, 4);
  display.print("YC Looper v1.0");
}

void display_menu_bg() {
  display.drawFastHLine(2, 32, 122, SSD1306_WHITE);
  display.drawFastHLine(2, 48, 122, SSD1306_WHITE);
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
  switch (selectionZone) {
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

void init_display() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  delay(100);
  display.clearDisplay();
  display_splash_screen();
  display.display();
}
