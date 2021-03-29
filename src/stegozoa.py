import os
import time



decoderPipePath = "/tmp/stegozoa_decoder_pipe"
encoderPipePath = "/tmp/stegozoa_encoder_pipe"

try:
    os.mkfifo(encoderPipePath)
except Exception as oe: 
    raise ValueError(oe)

try:
    os.mkfifo(decoderPipePath)
except Exception as oe: 
    raise ValueError(oe)




encoderPipe = open(encoderPipePath, 'w')
message = 'Hello' * 1000

encoderPipe.write(message)
encoderPipe.flush()

time.sleep(10)
encoderPipe.close()



os.remove(encoderPipePath)
os.remove(decoderPipePath)
