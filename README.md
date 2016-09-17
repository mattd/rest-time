# Rest Time

A Pebble app that reminds you to rest.

### What's new

- v2.1
  Keep timer running after app exists.

- v2.0
  Supports overrun.

### Usage

Simple, really.

Two clock displays: the bottom shows the current time, the top shows a timer
counting down the time until your next break. By default "work" periods are 25
minutes and "rest" periods are 2 minutes, but the duration of both intervals is
configurable (see below). When you're in work mode, the display is white text on
a black background; when you're in rest mode, the display is black text on a
white background. A double buzz tells you rest mode has started; a single buzz
tells you rest mode is over.

By default, it switches between work mode and rest mode when time is up. You can
enable "overrun" to turn off automatic switch. The top clock display shows a 
count-up timer when it's overrunning. You need to press up or down button to
start a new period. During overrun, it buzzes every minute.

The timer keeps running after app exists. It will wake up at about 15 seconds
ahead of buzz.

### Controls

#### App View

* **Up Button:** Reset the countdown timer to a rest mode start.
* **Down Button:** Reset the countdown timer to a work mode start.
* **Center Button - Single Click:** Pause the countdown timer.
* **Center Button - Double Click:** Open the settings menu.

#### Settings Menu View

* **Up/Down Buttons:** Navigate the settings.
* **Center Button:** Cycle through options on the selected setting.

### Available Settings

* **Work Interval:** Time not spent at rest. Possible values are in 5 minute
  increments from 5 to 60 minutes.
* **Rest Interval:** Time spent at rest. Possible values are in 1 minute
  increments from 1 to 10 minutes.
* **Warning Vibe:** Whether to trigger a double buzz warning vibration 10
  seconds before the next rest period begins. Possible values are "On" or "Off".
* **Overrun:** Timer overruns after time is up.