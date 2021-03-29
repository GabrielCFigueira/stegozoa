import os







decoderPipe = "/tmp/stegozoa_decoder_pipe"
encoderPipe = "/tmp/stegozoa_encoder_pipe"

try:
    os.mkfifo(encoderPipe)
except OSError as oe: 
    raise ValueError(oe)

try:
    os.mkfifo(decoderPipe)
except OSError as oe: 
    raise ValueError(oe)
