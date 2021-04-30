'''Contains a tcp server implementation.

Saves the received bytes as audio file
'''

# imports
from pydub import AudioSegment
import socket
import io
import string
import sys
import random

# tcp
BUFFERSIZE = 2048
HOST = '192.168.2.115'
PORT = 25666

# audio
FILENAME = "audio"
FORMAT = "flac"
SAMPLEWIDTH = 2
CHANNELS = 2
FRAMERATE = 44100
RANDOMSTRINLENGTH = 10


def receive_data() -> bytearray:
    '''Create TCP socket and listen on predefined host and port.
    Saving received bytes in bytearray

    return:
        bytearray       raw data bytes
    '''
    array = bytearray(b'0')
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((HOST, PORT))
        s.listen()
        print('Listening on ' + HOST + ':' + str(PORT) + ' ...')
        conn, addr = s.accept()
        counter = 0
        with conn:
            print('Connected by ' + str(addr[0]) + ':' + str(addr[1])  + ' ...')
            while True:
                data = conn.recv(BUFFERSIZE)
                print('     Receiving package ' + str(counter), end='\r')
                sys.stdout.flush()
                array += data
                counter += 1
                if not data:
                    break
    return array


def save_bytes_as_file(raw_data: bytearray):
    '''Saves the given bytearray as file.
    
    parameters:
        raw_data:       bytesarray with the raw data
    '''
    audio = AudioSegment.from_raw(io.BytesIO(raw_data), sample_width=SAMPLEWIDTH, channels=CHANNELS, frame_rate=FRAMERATE)
    random_string = ''.join(random.choices(string.ascii_lowercase + string.digits, k=RANDOMSTRINLENGTH))
    audio.export(FILENAME + '_' + random_string + '.' + FORMAT, format=FORMAT)
    
    print('Saved audio as \'' + FILENAME + '_' + random_string  + '.' + FORMAT + '\' ...')

# call functions
save_bytes_as_file(receive_data())
