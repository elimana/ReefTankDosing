void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  display.print(":");
  if(digits < 10)
    display.print('0');
  display.print(digits);
}

void digitalClockDisplay() {
  // digital clock display of the time
  display.print(hour());
  printDigits(minute());
  printDigits(second());
  //display.print(" ");
  //display.print(dayStr(weekday())); 
}

void digitalDateDisplay() {
  /*display.print(day());
  display.print("/");
  display.print(month());
  display.print("/");
  display.print(year());
  display.print(" ");*/
  display.print(dayShortStr(weekday()));
}
