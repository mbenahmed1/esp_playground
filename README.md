# ESP32 playground

<image src="https://docs.espressif.com/projects/esp-adf/en/latest/_images/esp32-lyrat-v4.2-side.jpg" width="300" />

In this repository I tried out some of the examples from the LyraT documentation.
Since the LyraT board is an audio development board, all tasks are audio focused.

The tutorials and guides can be found here:
https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/
https://docs.espressif.com/projects/esp-adf/en/latest/get-started/

## Task 1.2
In this task the LyraT board records audio via it's internal microphones and saves it as .wav file on the sdcard.

## Task 1.3
Here the LyraT board first records audio and saves it as .wav and afterwards sends it via http to a remote server.

## Task 1.4
In this taks the recorded audio is saved as .wav and send via tcp to a server. The server then converts the bytes to .flac file, which is smaller than pure wav.
