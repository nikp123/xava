X.A.V.A.
====================

**X**11 **A**udio **V**isualizer for **A**LSA

also supports audio input from Portaudio, Pulseaudio, MPD and sndio.

a fork of [Karl Stavestrand's](mailto:karl@stavestrand.no) [C.A.V.A.](https://github.com/karlstav/cava) by [Nikola Pavlica](mailto:pavlica.nikola@gmail.com)

Changes in 0.6:
* Autosens of low values (dynamic range)
* Removed config parameter 'overshoot'
* Removed config parameter 'mode'
* New config parameter 'waves'
* Changed config parameter 'style' to 'channels' (was either 'mono' or 'stereo' anyway)
* Parameters 'integral' and 'gravity' are now in percentage

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](https://github.com/thlorenz/doctoc)*

- [What it is](#what-it-is)
- [Build requirements](#build-requirements)
- [Getting started](#getting-started)
  - [Installing manually](#installing-manually)
  - [Uninstalling](#uninstalling)
  - [Arch](#arch)
  - [Windows](#windows)
- [Capturing audio](#capturing-audio)
  - [From PortAudio (easy)](#from-portaudio-easy)
  - [From Pulseaudio monitor source](#from-pulseaudio-monitor-source-easy-as-well-unsupported-on-macos-and-windows)
  - [From ALSA-loopback device (Tricky)](#from-alsa-loopback-device-tricky-unsupported-on-macos-and-windows) )
  - [From mpd's fifo output](#from-mpds-fifo-output)
  - [sndio](#sndio)
  - [squeezelite](#squeezelite)
- [Latency notes](#latency-notes)
- [Usage](#usage)
  - [Controls](#controls)
- [Configuration](#configuration)
  - [Output modes](#output-modes)
  - [OpenGL](#opengl)
  - [Window options](#window-options)
  - [Shadow](#shadow)
  - [Additional features](#additional-features)
- [Contribution](#contribution)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->


What it is
----------

X.A.V.A. is a bar spectrum audio visualizer in a graphical window (X11, WINAPI, SDL2).

This program is not intended for scientific use. It's written to look responsive and aesthetic when used to visualize music. 


Build requirements
------------------
* [FFTW](http://www.fftw.org/)
* [Xlib/X11 dev files](http://x.org/)
* [SDL2 dev files](https://libsdl.org/)
* [ALSA dev files](http://alsa-project.org/)
* [Pulseaudio dev files](http://freedesktop.org/software/pulseaudio/doxygen/)
* [Portaudio dev files](http://www.portaudio.com/)

Only FFTW is actually required for XAVA to compile.

All the requirements can be installed easily in all major distros:

Debian/Raspbian:

    apt-get install libfftw3-dev libasound2-dev libpulse-dev libx11-dev libsdl2-dev libportaudio-dev cmake

ArchLinux:

    pacman -S base-devel fftw alsa-lib iniparser pulseaudio libx11 sdl2 portaudio cmake

openSUSE:

    zypper install alsa-devel fftw3-devel libX11-devel libSDL2-devel portaudio-devel cmake

Fedora:

    dnf install alsa-lib-devel fftw3-devel xorg-x11-devel SDL2-devel pulseaudio-libs-devel portaudio-devel cmake

Cygwin dependencies (64bit ONLY):

    gcc-core w32api-headers libfftw3-devel libportaudio-devel portaudio cmake


Iniparser is also required, but if it is not already installed, it will clone the [repository](https://github.com/ndevilla/iniparser).

For compilation you will also need a C modern compiler, 'make', `cmake` and `git`.


Getting started
---------------

    mkdir build (if it doesn't already exist)
    cd build
    cmake ..
    make -j4

### Installing manually

Install `xava` to default `/usr/local`:

    make install

Or you can change `PREFIX`, for example:

    cmake .. -DCMAKE_INSTALL_PREFIX:PATH=/usr

### Uninstalling

    xargs rm < install_manifest.txt

### Arch

> NOTE: Currently unavailable.

XAVA-G is availble in [AUR](https://aur.archlinux.org/packages/xava-gui-git/).

    pacaur -S xava-gui-git

### VoidLinux

> NOTE: Currently unavailable.

XAVA-G is avaible in the Void repos:
	
	xbps-install xava-gui

### Windows

There should be an installer in the Releases page of this repository.

Please use minGW for compilation. You can use `-DCMAKE_C_COMIPLER=/usr/bin/gcc.exe` and `-DCMAKE_CXX_COMPILER=/usr/bin/g++.exe` in the cmake command for cross-compilation.

#### Additional info

This version doesn't include compatibility for the raw mode (since they depend on UNIX functionality to work).

To get raw functionality, compile this package with cygwin. But it still probably won't work.


All distro specific instalation sources might be out of date.


Capturing audio
---------------

### From portaudio (easy)

First make sure you have portaudio dev files and that xava was built with portaudio support (it should have done so automatically if it found the dev files).

Since portaudio combines the functionality of all audio systems, it should work "out of the box".

To enable just uncomment:
    
    method = portaudio

in the `[input]` section of your config.

To change the portaudio input device, set `source = list` temporarely and run xava again.

It should list all of the available devices (even that aren't input devices, watch out for that).

Once you find the device you want to record from, take it's number and set `soruce = *that number*`


#### If the host is running Pulseaudio

Once XAVA is running, on pulseaudio you should change it's input to the device you want to use.


#### If the host is Windows

On Windows, you should have 'Stereo Mix' as a option on the Recording tab in Audio settings.

If you don't, install better drivers or get a better sound card.

Once you figured that out, continue below.


### From Pulseaudio monitor source (Easy as well, unsupported on macOS and Windows)

First make sure you have installed pulseaudio dev files and that xava has been built with pulseaudio support (it should be automatically if the dev files are found).

If you're lucky all you have to do is to uncomment this line in the config file under input:

    method = pulse
 
If nothing happens you might have to use a different source than the default. The default might also be your microphone. Look at the config file for help. 


### From ALSA-loopback device (Tricky, unsupported on macOS and Windows)

Set

    method = alsa

in the config file.

ALSA can be difficult because there is no native way to grab audio from an output. If you want to capture audio straight fom the output (not just mic or line-in), you must create an ALSA loopback interface, then output the audio simultaneously to both the loopback and your normal interface.

To create a loopback interface simply run:

`sudo modprobe snd_aloop`

Hopefully your `aplay -l` should now contain a loopback interface.

To make it presistent across boot add the line `snd-aloop` to "/etc/modules". To keep it form beeing loaded as the first soundcard add the line `options snd-aloop index=1` to "/etc/modprobe.d/alsa-base.conf", this will load it at '1'. You can replace '1' with whatever makes most sense in your audio setup.

Playing the audio through your Loopback interface makes it possible for xava to to capture it, but there will be no sound in your speakers. In order to play audio on the loopback interface and your actual interface you must make use of the ALSA multi channel.

Look at the included example file `example_files/etc/asound.conf` on how to use the multi channel. I was able to make this work on my laptop (an Asus UX31 running Ubuntu), but I had no luck with the ALSA method on my Rasberry PI (Rasbian) with an USB DAC. The PulseAudio method however works perfectly on my PI. 

Read more about the ALSA method [here](http://stackoverflow.com/questions/12984089/capture-playback-on-play-only-sound-card-with-alsa).

If you are having problems with the alsa method on Rasberry PI, try enabling `mmap` by adding the following line to `/boot/config.txt` and reboot:

```
dtoverlay=i2s-mmap
```

### From mpd's fifo output

Add these lines in mpd:

    audio_output {
        type                    "fifo"
        name                    "my_fifo"
        path                    "/tmp/mpd.fifo"
        format                  "44100:16:2"
    }

Uncomment and change input method to `fifo` in the config file.

The path of the fifo can be specified with the `source` parameter.

I had some trouble with sync (the visualizer was ahead of the sound). Reducing the ALSA buffer in mpd fixed it:

    audio_output {
            type            "alsa"
            name            "My ALSA"
            buffer_time     "50000"   # (50ms); default is 500000 microseconds (0.5s)
    }

### sndio

sndio is the audio framework used on OpenBSD, but it's also available on
FreeBSD and Linux. So far this is only tested on FreeBSD.

To test it
```bash
# Start sndiod with a monitor sub-device
$ sndiod -dd -s default -m mon -s monitor

# Set the AUDIODEVICE environment variable to override the default
# sndio device and run xava
$ AUDIODEVICE=snd/0.monitor xava
```

### squeezelite
[squeezelite](https://en.wikipedia.org/wiki/Squeezelite) is one of several software clients available for the Logitech Media Server. Squeezelite can export it's audio data as shared memory, which is what this input module uses.
Configure C.A.V.A. with the `-DSHMEM=ON` option, then adapt your config:
```
method = shmem
source = /squeezelite-AA:BB:CC:DD:EE:FF
```
where `AA:BB:CC:DD:EE:FF` is squeezelite's MAC address (check the LMS Web GUI (Settings>Information) if unsure).
Note: squeezelite must be started with the `-v` flag to enable visualizer support.

Latency notes
-------------

If you see latency issues, ie. visualizer not reacting on time, try turning off demanding graphical effect and/or shrinking the window, or just lower the ``fft_size``.

If your audio device has a huge buffer, you might experience that xava is actually faster than the audio you hear. This reduces the experience of the visualization. To fix this, try decreasing the buffer settings in your audio playing software.

Usage
-----

    Usage : xava [options]
    Visualize audio input in terminal. 

    Options:
    	    -p          path to config file
    	    -v          print version



Exit by pressing Q, ESC or by closing the window ;)

### Controls

| Key | Description |
| --- | ----------- |
| <kbd>up</kbd> / <kbd>down</kbd>| increase/decrease sensitivity |
| <kbd>left</kbd> / <kbd>right</kbd>| increase/decrease bar width |
| <kbd>a</kbd> / <kbd>s</kbd> | increase/decrease bar spacing |
| <kbd>f</kbd> | toggle fullscreen (only in window modes, besides win32) |
| <kbd>c</kbd> / <kbd>b</kbd>| change forground/background color |
| <kbd>r</kbd> | Reload configuration |
| <kbd>q</kbd> or <kbd>ESC</kbd>| Quit C.A.V.A. |

Configuration
-------------

As of version 0.4.0 all options are done in the config file, no more command-line arguments!

By default a configuration file is located in `$XDG_CONFIG_HOME/xava/config`, `$HOME/.config/xava/config` or in case of Windows in `%APPDATA%\xava\config`

**Examples on how the equalizer works:**

    [eq]
    1=0
    2=1
    3=0
    4=1
    5=0

![3_138](https://cloud.githubusercontent.com/assets/6376571/8670183/a54a851e-29e8-11e5-9eff-346bf6ed91e0.png)

    [eq]
    1=2
    2=2
    3=1
    4=1
    5=0.5

![3_139](https://cloud.githubusercontent.com/assets/6376571/8670181/9db0ef50-29e8-11e5-81bc-3e2bb9892da0.png)

### Output modes

XAVA supports outputing as a X11 window, SDL2 and WINAPI.

### OpenGL

To run XAVA in OpenGL:
      
	opengl = true

WARNING: OpenGL isn't supported under SDL2.

### Window options

Toggle fullscreen:
     
	fullscreen = 1 or 0

WARNING: On Windows it isn't supported.


Toggle window border:
    
	border = 1 or 0

WARNING: On Windows the border is always disabled.


Change bar width/height (units are in pixels rather than characters):
    
	bar_width = (width in pixels)
    
	bar_spacing = (width in pixels)

NOTE: It's located in the ```window``` category.


Move the window on startup to a specific part of the screen:

	alignment = 'top_left', 'top', 'top_right'. 'left', 'center', 'right', 'bottom_left', 'bottom', 'bottom_right' and by default 'none'


In addition to window aligment you can adjust the window position further with:
    
	x_padding = (specify value)
    
	y_padding = (specify value)


You can enable transparent windows:
     
	transparency = 1

WARNING: SDL2 doesn't have transparency.


Force the window to be behind any window (works only under Xlib):
    
	keep_below = 1

Set window properties (Window Class):

	set_win_props = 1

This helps with removing blur and shadows from behind the window, but also removes the ability to interact with the window.

Make window click-proof (as in the window is totally ignored when clicked and clicks behind it):
    
    interactable = 0

Pro-tip: You can still gain control of the window by clicking it's taskbar icon.

### Shadow

You can change the following options:
    
    size = (in pixels)

and
    
    color = '#aarrggbb'

You need to enable transparency for the shadows to work.


### Additional features

Setting foreground color to `default` will cause in Xlib to average out the color in the desktop. 

On Windows it will grab the accent color (aka. your theme) instead.

To enable this you just have to change:
    
    foreground = 'default'
    
Set foreground opacity:

    foreground_opacity = (from 0.0 to 1.0)

You need OpenGL and transparency support in order for it to work.


Contribution
------------

Please read CONTRIBUTING.md before opening a pull request.

Thanks to:
* [CelestialWalrus](https://github.com/CelestialWalrus)
* [anko](https://github.com/anko)
* [livibetter](https://github.com/livibetter)

for major contributions in the early development of this project.

Also thanks to [dpayne](https://github.com/dpayne/) for figuring out how to find the pulseaudio default sink name.

