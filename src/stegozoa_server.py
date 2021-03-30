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



decoderPipe = open(decoderPipePath, 'r')

request = decoderPipe.read(5)
print(request)

encoderPipe = open(encoderPipePath, 'w')
message = 'World' 

encoderPipe.write(message)
encoderPipe.flush()

response = decoderPipe.read(1)
print(response)

encoderPipe.close()
decoderPipe.close()


os.remove(encoderPipePath)
os.remove(decoderPipePath)
