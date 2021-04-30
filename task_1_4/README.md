# Task 1.4

- Records audio and saves it as rec.wav on the sdcard.
- Reads rec.wav. from the sdcard and sends it via tcp.
- server.py receives bytes via tcp.
- Saves bytes into .flac file in the local directory.

# Usage

Run `$ idf.py set-target esp32`.

Run `$ idf.py menuconfig` and navigate to the task\_1\_4 submenu.

Configure host, ssid and password. All other configurations should work by default.

Set HOST in `server.py` according to the ip address of the machine you are running the server on.
This has to be the same address as configured in menuconfig of the LyraT board.

Run `$ idf.py -p /dev/ttyUSB0 flash monitor` to compile and flash onto the LyraT board.

Start server with

`$ python3 server.py`.

Restart LyraT Board by pressing the `[RST]`.

Press `[REC]` to start recording.

Press `[REC]` again to stop recording.

After this the LyraT board will try to connect to the server via tcp.
