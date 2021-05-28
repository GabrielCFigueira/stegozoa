# Stegozoa

In both VMs:

Stegozoa: `python3 src/stegozoaClient.py <peerId>`

Video camera: `sudo ffmpeg -nostats -re -i SharedFolder/some_video.mp4 -r 30 -vcodec rawvideo -pix_fmt yuv420p -threads 0 -f v4l2 /dev/video0`

Chromium: `DISPLAY=:0.0 chromium_builds/regular_build/chrome --no-sandbox https://whereby.com/<chatroomId> > output.log`

Warning: You may need to access the graphical interface of the VM on the first time launching chromium with that chatroom link. If the website supports it (whereby does), enable automatic entrance in the video room, so that next time this isn't necessary.

# Code

In `src/stegozoa_hooks` you will find the code that is bundled with Chromium, which implements the embedding and extraction functions. The set of files present in `libvpx_patches` replaces key libvpx files in order to call this code. The stegozoa library (`src/libstegozoa.py`) communicates with these hooks through named pipes, exposing a set of function calls in order to support communication: initialize(), send() and receive().
