/*
 * leds.ino - LED output and initialization
 * Author: Tom Hightower - May 10, 2022
 */
 
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

void init_leds() {
  pinMode(LED_CH_A, OUTPUT);
  pinMode(LED_CH_B, OUTPUT);
  pinMode(LED_CH_C, OUTPUT);
  pinMode(LED_PWR,  OUTPUT);
  pinMode(LED_TMPO, OUTPUT);
}
