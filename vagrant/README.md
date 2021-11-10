# StegoRTC

## Stegozoa

1 - Launch the VMs in the stegozoa folder by running `vagrant up`: cloning the chromium repo may take several hours, and requires more that 4GB of RAM (at least 8?).

2 - Compile chromium by running the following command in the scripts folder `ansible-playbook compile.yml --extra-vars "build=regular_build"`, where build should be the name of the macros file to use.

3 - Set resolution and camera configs by running `ansible-playbook setup.yml`

Stegozoa is now ready to go!
