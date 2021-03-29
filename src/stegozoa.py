import os



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
message = 'A' * 10000

encoderPipe.write(message)
encoderPipe.flush()
encoderPipe.close()



os.remove(encoderPipePath)
os.remove(decoderPipePath)
