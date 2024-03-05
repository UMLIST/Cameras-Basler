# Cameras-Basler
Contains source code for collecting video with Basler cameras as well as documentation. Written for Linux ARM on NVIDIA Jetson platform, but can also run on Windows, MacOS, and Linux x86.

# How to Use
1. If running source code, install pylon software (see Getting Started)
2. Clone this repository
3. Run the shell script *cam_runner.sh*. This will call the main exe file. You can specify the frames per second and total number of frames by modifying the two arguments inside the script.

## Modifying code/build process
Source code (C++) is in the *src* folder. It is built using Makefiles. It is based on provided C++ examples in the pylon documentation *Grab* and *Utility_GrabVideo* with some modifications to add timestamps/change input parameters. 
1. Make your modications to *main.cpp* (or other functions, files)
2. In the *src* folder, run *sudo make* to build the new exe file
3. You can run the code without the shell script by then running *./main (FPS) (NumFrames)* in the CLI where FPS and NumFrames are you input arguments

# Getting Started
Basler cameras use their open-source pylon software/SDK compatible with multiple operating systems. The page for Pylon can be found here:
https://www.baslerweb.com/en-us/software/pylon/. 

Follow the link to *Download pylon software* and select the download link for your operating system. For saving mp4 files, also download and install the *pylon Supplementary Package for MPEG-4* for your desired OS. This is noted in the download readme, but for Jetsons, you will also have to run a shell script called *setup-usb.sh*, which will be located in the pylon directly after installation. This installs udev-rules for permissions for basler cameras. I remember having to unplug the cameras/restart the Jetson to get it to work after. 

*For Linux ARM: Members of Duke List Private Box can get download links here. I had a weird issue where I couldn't get the file to download on the Jetsons and had to post and pull from box:

    - pylon software: https://duke.box.com/s/r6rijs3y2lwku7hfia5f3d8yekfukobo 
    - pylon supplemental package for mpeg: https://duke.box.com/s/68zr2yrt6ods7inmjs54czns8uasucpo. 

Included in the pylon software is the *pylon Viewer* which is used for setting up the cameras. It is a GUI that allows you to configure settings, take quick video, and provides documentation on functions. This is your starting point to configure the cameras. Once you have your settings, you can move to writing code with the SDK. 

The SDK has support for C/C++/.NET. Basler also maintains an open-source Python wrapper pypylon here, if that is more your cup of tea: https://www.baslerweb.com/en-us/software/pylon/pypylon/. 

# LIST Cameras
LIST has the following camera models
- 2 x daA3840-45uc (s-mount)
    - https://docs.baslerweb.com/daa3840-45uc 
- 1 x daA1920-30uc (s-mount)
    - https://docs.baslerweb.com/daa1920-30uc
- 2 x Evetar Lens E3417C IR-Cut F2.4 f2.5mm 1/1.8"
    - https://www.baslerweb.com/en-us/shop/evetar-lens-e3417c-ir-cut-f2-4-f2-5mm-1-1-8/ 

The dart R or ace2 models are probably what we should stick with. Ace2 has slightly more features, GigE options, bigger, bigger frames, slightly higher cost.

# pylon Documentation 
## Cameras
Jumping off point for cameras is here: https://docs.baslerweb.com/cameras. 

Go to Area Scan Cameras, find your camera model. Search under the left tab on the screen for things such as features specific to your camera model to find documentation, including example code and commands on how to use each feature. So helpful!

## pylon SDK
Jumping off point for software documentation is here: https://docs.baslerweb.com/software. 

Under pylon camera software suite, you will find documentation on things such as the pylon viewer. Under pylon APIs, you will find all the documentation on the API for all supported languages (C/C++/.NET), including example code. Quick link to the samples manual is here: https://docs.baslerweb.com/pylonapi/pylon-sdk-samples-manual.

# Status
As of 3/5/24:
- Saving mp4 for single USB camera
- User can specify frames per second and total number of frames to grab
    - I'm finding best performance at 25 FPS and 1080p resolution at the moment...
- Timestamps recorded for each frame

Next Steps:
- Grab initial timestamp at beginning of acqusition
- Timestamps are in number of seconds since camera turned on
    - Reformat into time relative to start of video
- Handle multiple cameras
    - We probably need to refactor to collect on a hardware trigger to do this
- Stream cameras (over UDP?) so they can be viewed during collection

# Issues
- Timestamp cannot be reset to zero each data run. There is a command to do this, but it doesn't seem to work
    - Workaround is to take the difference between each timestamp to get the delta T.
- When I add timestamps, the last frame returns an error (all other frames save OK)
    - Quick workaround is just collect N+1 number of frames
