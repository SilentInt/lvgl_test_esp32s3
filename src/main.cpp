#include "disp_touch/disp_touch.hpp"

void setup()
{
  Serial.begin(115200);
  init_display_touch();
}

void loop()
{
  lv_timer_handler(); /* let the GUI do its work */
  delay(5);
}
