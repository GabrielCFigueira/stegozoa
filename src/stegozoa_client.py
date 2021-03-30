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
message = 'Hello'

encoderPipe.write(message)
encoderPipe.flush()

decoderPipe = open(decoderPipePath, 'r')

response = decoderPipe.read(5)
print(response)

if response == 'World':
    message = '!'
else:
    message = 'NOOO0'


encoderPipe.write(message)
encoderPipe.flush()


encoderPipe.close()
decoderPipe.close()

time.sleep(20)



os.remove(encoderPipePath)
os.remove(decoderPipePath)
