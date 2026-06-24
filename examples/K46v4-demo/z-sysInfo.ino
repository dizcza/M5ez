/*
 * 
 * This is a "z-sketch". It means you can run this sketch on its own, or use it as a sub-sketch of some bigger program
 * See the M5ez user manual under z-sketches at https://github.com/M5ez/M5ez
 *
 */

#ifndef MAIN_DECLARED

#include <M5StX.h>
#include <M5ez.h>

void setup() {
  ez.begin();
  sysInfo();
}

void loop() {

}

String exit_button = "";

#else

String exit_button = "Exit";

#endif  // #ifndef MAIN_DECLARED



void sysInfo() {
  while(true) {
    String btn = ez.buttons.poll();
    if (btn == "Exit") break;
  }
}
