# Task 1.3

- Records audio and saves it as rec.wav on the sdcard.
- Reads rec.wav. from the sdcard and sends it via http.
- server.py receives bytes via http.
- Saves bytes into .wav file in the local directory.

# Usage

Run `$ idf.py set-target esp32`.

Run `$ idf.py menuconfig` and navigate to the task\_1\_3 submenu.

Configure server URL, ssid and password. All other configurations should work by default.


Start server with

`$ python2 server.py`.

Restart LyraT Board by pressing the `[RST]`.

Press `[REC]` to start recording.

Press `[REC]` again to stop recording.

After this the LyraT board will try to connect to the server via http.
