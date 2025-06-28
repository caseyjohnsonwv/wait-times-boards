To flash a new wait times board:

1. Set DIP switches 5,6,7 to ON & all others to OFF.
2. Upload `get_queue_times.ino`.
3. Set DIP switches 3,4 to ON & all others to OFF.
4. Upload `display_queue_times.ino`.
5. Set DIP switches 1,2 to ON & all others to OFF.
6. Connect the shield screen & reset the Arduino.

---

To reset an existing wait times board:

1. Reset the Arduino.
2. Wait for the startup "Reset" URL to display.
3. Navigate to the given URL from any device on the same network as the Arduino.
