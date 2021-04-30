# Task 1.4

- Records audio and saves it as rec.wav on the sdcard.
- Reads rec.wav. from the sdcard and sends it via tcp.
- server.py receives bytes via tcp.
- Saves bytes into .flac file in the local directory.

# Usage

Run `$ idf.py menuconfig` and navigate to the task\_1\_4 submenu.

Configure host, ssid and password. All other configurations should work by default.

Start server with

`$ python3 server.py`.

Restart LyraT Board by pressing the `[RST]`.

Press `[REC]` to start recording.

Press `[REC]` again to stop recording.

After this the LyraT board will try to connect to the server via tcp.
