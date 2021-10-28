X.A.V.A.
====================

[![Build](https://github.com/nikp123/xava/actions/workflows/build.yaml/badge.svg)](https://github.com/nikp123/xava/actions/workflows/build.yaml)

**X**11 **A**udio **V**isualizer for **A**LSA

also supports audio input from WASAPI (Windows), PortAudio, Pulseaudio, SHMEM, FIFO (MPD) and sndio.

[![Demo video](https://i.imgur.com/9fvqV2N.png)](http://www.youtube.com/watch?v=OkYrHAqu3fc)

a fork of [Karl Stavestrand's](mailto:karl@stavestrand.no) [C.A.V.A.](https://github.com/karlstav/cava) by [Nikola Pavlica](mailto:pavlica.nikola@gmail.com)

- [What it is](#what-it-is)
- [Getting started](#getting-started)
  - [Windows](#windows)
  - [macOS](#macos)
  - [Linux](#linux)
    - [Arch](#arch)
    - [Void](#void)
  - [Installing manually](#installing-manually)
    - [Build requirements](#build-requirements)
    - [Compilation](#compilation)
    - [Uninstalling](#uninstalling)
- [Updating](#updating)
- [Usage](#usage)
  - [Controls](#controls)
- [Troubleshooting](#troubleshooting)
  - [Latency notes](#latency-notes)
  - [The visualizer isn't opening](#the-visualizer-isnt-opening)
  - [The visualizer does nothing](#the-visualizer-does-nothing)
  - [How do I start the visualizer on system startup](#how-do-i-start-the-visualizer-on-system-startup)
- [Configuration](#configuration)
  - [Equalizer](#equalizer)
  - [Capturing audio](#capturing-audio)
    - [From WASAPI (Windows)](#from-wasapi-windows-only-super-easy)
    - [From Pulseaudio monitor source](#from-pulseaudio-monitor-source-easy-as-well-unsupported-on-macos-and-windows)
    - [From PortAudio](#from-portaudio)
    - [From ALSA-loopback device (Tricky)](#from-alsa-loopback-device-tricky-unsupported-on-macos-and-windows)
    - [From mpd's fifo output](#from-mpds-fifo-output)
    - [sndio](#sndio)
    - [squeezelite](#squeezelite)
  - [Output modes](#output-modes)
  - [Basic window options](#basic-window-options)
  - [Window position](#window-position)
  - [Advanced window options](#advanced-window-options)
    - [Transparency](#transparency)
    - [Advanced window behaviour](#advanced-window-behaviour)
    - [Fullscreening the visualizer without changing the viewport](#fullscreening-the-visualizer-without-changing-the-viewport)
  - [OpenGL](#opengl)
    - [Shaderpacks](#shaderpacks)
    - [Resolution scaling](#resolution-scaling)
  - [Vsync](#vsync)
  - [Bars](#bars)
  - [Colors and gradients](#colors-and-gradients)
  - [Accent colors](#accent-colors)
  - [Additional options](#additional-options)
- [Contribution](#contribution)


What it is
----------

X.A.V.A. is a bar spectrum audio visualizer in a graphical window
 (X11, Wayland (wlroots and xdg shell), Windows, SDL2).

This program is not intended for scientific use. It's written
 to look responsive and aesthetic when used to visualize music. 


Getting started
---------------

### Windows

There should be an installer in the Releases page of this repository. Just
 download, install and run it. Non-release builds are unstable, so expect
 breakage.
 
Alternatively, you can download the latest "unstable" release from:

[Here for 32-bit x86 Windows](https://nightly.link/nikp123/xava/workflows/build.yaml/unstable/xava-unstable-installer-i686.exe.zip)

[Here for 64-bit x86 Windows](https://nightly.link/nikp123/xava/workflows/build.yaml/unstable/xava-unstable-installer-x86_64.exe.zip)

The configuration file is located in ``%appdata%\xava\config.cfg``

Technical info/notes:

Please use minGW for compilation. I haven't tested MSVC, however the code has a
bit of GNU specific features so it may not work.

### macOS

You have to compile this yourself, get the dependencies installed via brew (look
 below) and compile it using the method described above.

Once you've done that, install [Background Music](https://github.com/kyleneideck/BackgroundMusic)
 and get it running on your Mac machine. Then, install the latest
 [XQuartz](https://www.xquartz.org/releases/index.html) build.
 Now what you need to do is add the following into the ``[input]``
 section of the config file:
```
method = portaudio
source = "Background Music"
```
And now, within "Background Music" change the audio source of
 XAVA to the speaker output that you want recorded.

You are free to use either ``ncurses``, ``x11_sw`` or ``sdl2_sw`` as the output
methods.

### Linux

#### Arch

XAVA is availble in [AUR](https://aur.archlinux.org/packages/xava-git/).

    pacaur -S xava-git

#### Void

XAVA is available in the Void repos:

    xbps-install xava


### Installing manually

#### Build requirements

Only FFTW is actually required for XAVA to compile. However, other features may
or may not depend on other dependencies. As such, they have to be installed in
order for those same features to be enabled.

The following represents installing dependencies for all possible features on
major Linux distributions or package managers:

Debian/Raspbian:

    apt-get install make libfftw3-dev libasound2-dev libpulse-dev libx11-dev libsdl2-dev libportaudio-dev cmake git wayland-protocols

On Ubuntu 20.04+:

    apt-get install make libfftw3-dev libasound2-dev libpulse-dev libx11-dev libsdl2-dev libportaudio2 cmake git wayland-protocols

Meanwhile on previous Ubuntu releases swap ``libportaudio2`` with ``libportaudio-dev``

ArchLinux:

    pacman -S make base-devel fftw alsa-lib pulseaudio libx11 sdl2 portaudio cmake git wayland-protocols

openSUSE:

    zypper install make alsa-devel fftw3-devel libX11-devel libSDL2-devel portaudio-devel cmake git wayland-protocols

Fedora:

    dnf install make alsa-lib-devel fftw3-devel xorg-x11-devel SDL2-devel pulseaudio-libs-devel portaudio-devel cmake git wayland-protocols

MSYS2 (Windows):

    pacman -S make mingw-w64-i686-gcc mingw-w64-i686-fftw mingw-w64-i686-SDL2 mingw-w64-i686-portaudio cmake git pkg-config

Brew (macOS):

    brew install make portaudio fftw cmake gcc git pkg-config sdl2

For compilation you can (interchangably) use either clang or GCC.


#### Compliation

    mkdir build (if it doesn't already exist)
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release ( or Debug if you're that type ;) )
    make -j$(nproc) (or whatever number)

Windows users, just download the installer from the Releases page
 on this repository.

Install `xava` to default `/usr`:

    sudo make install

Or you can change `$PREFIX`, for example:
```
    cmake .. -DCMAKE_INSTALL_PREFIX:PATH=/usr -G"Unix Makefiles"
```
NOTE: Changing ``$PREFIX`` will probably break the system linker, **use with
 caution**.

If you're building from source and installing via ``make install``, please do
**not** delete the build files as you won't be able to uninstall xava if you do
(CMake limitation).

#### Uninstalling

    cd $where_is_xava_located/build
    xargs rm < install_manifest.txt

All installers/distro specific installation sources might be out of date.


Updating
--------

If you happen to be updating this program, just keep in mind that breaking
changes DO occur sometimes. In most cases, errors that show up might look like
issue #43. In most cases, these can be fixed with a simple config reset.

To do that just copy the new shader and config files from ```example_files```
dir in this repository. Sorry if this has inconvenienced you, my code can be
quite experimental sometimes....


Usage
-----

After you've installed it, to use this visualizer you'll have to be playing
something on your system (be it a movie, voice chat or song, doesn't matter) and
launch it. If everything succeeded, you'll see an audio visualization of whatever
audio is going through your system. If not, please refer to [Capturing audio](#capturing-audio)
section of this README.

In order to launch the application (in most cases), you'll be able to start it
via a start menu entry or a desktop shortcut. If not, you can still choose to
start it as a command-line executable:

	Usage : xava [options]
	Visualize audio input in terminal. 

	Options:
		-p	path to config file
		-v	print version

Exit by pressing Q, ESC or by closing the window ;)

### Controls

| Key | Description |
| --- | ----------- |
| <kbd>up</kbd> / <kbd>down</kbd>| increase/decrease sensitivity |
| <kbd>left</kbd> / <kbd>right</kbd>| increase/decrease bar width |
| <kbd>a</kbd> / <kbd>s</kbd> | increase/decrease bar spacing |
| <kbd>f</kbd> | toggle fullscreen |
| <kbd>c</kbd> / <kbd>b</kbd>| change forground/background color |
| <kbd>r</kbd> | restart visualizer |
| <kbd>q</kbd> or <kbd>ESC</kbd>| quit the program |


Troubleshooting
---------------

### Latency notes

If you see latency issues, ie. visualizer not reacting on time,
 try turning off demanding graphical effects and/or shrinking the window,
 or just lower the ``fft_size`` and ``size`` (within ``[input]``), as
 increasing capture time creates a delay.

If your audio device has a huge buffer, you might experience that xava
 is actually ahead than the audio you hear. This reduces the experience
 of the visualization. To fix this, try decreasing the buffer settings
 in your audio playing software. Or in case of MPD, just quickly pause
 and resume playback (yes, this is a bug with MPD's pulse implementation).

### The visualizer isn't opening

There's something seriously wrong with the visualizers configuration, please
check that the configuration is okay first and then file a bug if it's still not
working.

### The visualizer does nothing

The audio configuration might be incorrect, please refer to the [Capturing audio](#capturing-audio)
section of this README.

### How do I start the visualizer on system startup

On Windows create a shortcut of the ``xava.exe`` 
(which is in the install directory) and copy it to
``C:\Users\you\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup\``

On Linux/BSD (or any X11 compatible OS) just copy the
 ``assets/linux/xava.desktop`` file to ``~/.config/autostart/``. 
Unless you're on a tiling WM, in which case why are you
 even reading this.


Configuration
-------------

<b>
Important note:

Starting from 0.7.0.0, the configuration layout has been SIGNIFICANTLY CHANGED.
Please check ``examples_files/config`` before filing a bug. This has been done
to ease further development.
</b>

If you're running Windows and you used the installer, you'll find a
``Configure XAVA`` shortcut in your Start Menu. Use it to open the XAVA
configuration.

By default a configuration file is located in `$XDG_CONFIG_HOME/xava/config`, 
`$HOME/.config/xava/config` or on Windows `%APPDATA%\xava\config`

The configurations are seperated into different categories such as ``[general]``
or ``[window]`` which are used to configure how certain parts of the visualizer
should function. 

For example: ``[filter]`` refers to options that control the behavior of the
audio **filtering**/conversion to bars system, whilst ``[output]`` controls how
the output result should be drawn.

By default the configurations are commented by a semi-colon ``;`` in front of
the option. You'll need to remove this semi-colon for the configuration changes
to be effective. 


### Equalizer

To change the amplitude of certain frequency ranges, XAVA features an equalizer
in the ``[eq]`` section of the config file to do so.

The equalizer works by setting an amplitude value to a incremental
which refers to the part of the frequency spectrum.

**Examples on how the equalizer works:**

    [eq]
    1=0
    2=1
    3=0
    4=1
    5=0

![3_138](https://cloud.githubusercontent.com/assets/6376571/8670183/a54a851e-29e8-11e5-9eff-346bf6ed91e0.png)

In this example the frequency spectrum is divided in 5 parts. 
You may be able to see that the 1st, 3rd and 5th parts have been totally disabled and
you can see the result of it on the screenshot above.

    [eq]
    1=2
    2=2
    3=1
    4=1
    5=0.5

![3_139](https://cloud.githubusercontent.com/assets/6376571/8670181/9db0ef50-29e8-11e5-81bc-3e2bb9892da0.png)

And in this example you can see that the lower parts (1 and 2) 
have been amplified while 5 is being lowered.


### Capturing audio

The idea is that on most systems this should "just work" out of the box. If not,
keep reading.

The ``[input]`` section of the config file controls where and how XAVA captures
it's "audio input".

The ``method`` option describes which system should be used to capture the audio
samples for the visualizer to process and ``source`` tells that "capture source"
where to capture that audio information from. FXP. your microphone or seperate
audio input source.

#### From WASAPI (Windows only, super easy)

If you are on Windows, just use this.
 Don't even bother dealing with other options. 

It's enabled by default if it detects compilation for Windows and if it's
 disabled go and set ``method`` to ``wasapi`` in the ``[input]``section of the
 config file.

If you want to record loopback audio (that is "what you hear")
 (default) set ``source = loopback`` in the ``[input]`` section
 of the config file. Otherwise you can set anything else for
 it to record from the default microphone.

#### From Pulseaudio monitor source (Easy as well, unsupported on macOS and Windows)

First make sure you have installed pulseaudio dev files and
 that xava has been built with pulseaudio support (it should
 be automatically if the dev files are found).

If you're lucky all you have to do is to set this line in the
 config file under the ``[input]`` section ``method = pulseaudio``.
 
If nothing happens you might have to use a different source
 than the default. The default might also be your microphone.
 Look at the config file for help. 

#### From portaudio

First make sure you have portaudio dev files and that xava was
 built with portaudio support. Since portaudio combines the
 functionality of all audio systems, it should be the most compatible.
However, it is NOT "out of the box" because it requires some configuring.

1. To enable just set ``method`` to  ``portaudio`` in the ``[input]`` section of
 your config. Afterwards change the portaudio input device, by setting ``source``
 to ``list`` temporarily and by running xava again.

* It should list all of the available audio devices on your system
 (even that aren't input devices, watch out for that).

2. Once you find the device you want to record from,
 take it's number and set ``soruce`` to equal that number.

#### From ALSA-loopback device (Tricky, unsupported on macOS and Windows)

Set ``method`` to ``alsa`` in ``[output]`` section the config file.

ALSA can be difficult because there is no native way to grab audio
 from an output. If you want to capture audio straight fom the output
 (not just mic or line-in), you must create an ALSA loopback interface,
 then output the audio simultaneously to both the loopback and your
 normal interface.

To create a loopback interface simply run:

`sudo modprobe snd_aloop`

Hopefully your `aplay -l` should now contain a loopback interface.

To make it presistent across boot add the line `snd-aloop` to
 "/etc/modules". To keep it form beeing loaded as the first
 soundcard add the line `options snd-aloop index=1`to
 "/etc/modprobe.d/alsa-base.conf", this will load it at '1'.
 You can replace '1' with whatever makes most sense in your audio setup.

Playing the audio through your Loopback interface makes it
 possible for xava to to capture it, but there will be no sound
 in your speakers. In order to play audio on the loopback interface
 and your actual interface you must make use of the ALSA multi channel.

Look at the included example file `example_files/etc/asound.conf`
 on how to use  the multi channel. I was able to make this work
 on my laptop (an Asus UX31 running Ubuntu), but I had no luck
 with the ALSA method on my Rasberry PI (Rasbian) with an USB DAC.
 The PulseAudio method however works perfectly on my PI. 

Read more about the ALSA method [here](http://stackoverflow.com/questions/12984089/capture-playback-on-play-only-sound-card-with-alsa).

If you are having problems with the alsa method on Rasberry PI,
 try enabling `mmap` by adding the following line to `/boot/config.txt` and reboot:

```
dtoverlay=i2s-mmap
```

#### From mpd's fifo output

Add these lines in mpd:

    audio_output {
        type                    "fifo"
        name                    "my_fifo"
        path                    "/tmp/mpd.fifo"
        format                  "44100:16:2"
    }

Uncomment and change input method to `fifo` in the config file.

The path of the fifo can be specified with the `source` parameter.

I had some trouble with sync (the visualizer was ahead of the sound).
 Reducing the ALSA buffer in mpd fixed it:

    audio_output {
            type            "alsa"
            name            "My ALSA"
            buffer_time     "50000"   # (50ms); default is 500000 microseconds (0.5s)
    }

#### sndio

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

#### squeezelite

[squeezelite](https://en.wikipedia.org/wiki/Squeezelite) is one of several
 software clients available for the Logitech Media Server. Squeezelite can
 export it's audio data as shared memory, which is what this input module uses.
Configure XAVA with the `-DSHMEM=ON` (cmake) option, then adapt your config:
```
method = shmem
source = /squeezelite-AA:BB:CC:DD:EE:FF
```
where `AA:BB:CC:DD:EE:FF` is squeezelite's MAC address (check the LMS
 Web GUI (Settings>Information) if unsure).

Note: squeezelite must be started with the `-v` flag to enable visualizer support.


### Output modes

Since 0.7.0.0, there has been a lot of changes to the output system. Namely, by
default all "supported" modes use OpenGL (or EGL) by default. This means, that
for the visualizer to run properly your system must provide these (if you have
a graphics card, you're most likely fine). In case your system does NOT have GL
support, you can still use "unsupported" modes that don't have the full "feature
set". This is enabled by adding an additional ``-DBUILD_LEGACY_OUTPUTS:BOOL=ON``.

By default, the "supported" modes are: ``x11``, ``x11_egl`` (the difference being
that the ``x11`` is just plain desktop OpenGL), ``sdl2`` (uses desktop OpenGL),
``wayland`` (which uses EGL) and finally ``win`` which uses OpenGL on Windows.

And the "unsupported" ones are: ``x11_sw``, ``x11_sw_stars``, ``sdl2_sw``,
``wayland_sw``, ``ncurses``. However, beware that some feature might not work as
intended or at all with these modes, **hence the name**.

In order to change this mode you'll have to open the visualizer configuration and
within the ``[output]`` section, change the ``method`` to one which you desire.


### Basic window options

You can find these in the ``[window]`` section of the configuration file:

Window size:

	width = *number of pixels*
	height = *number of pixels*

Toggle fullscreen:

	fullscreen = *true or false*

Toggle window border:

	border = *true or false*


### Window position

Move the window on startup to a specific part of the screen:

	alignment = 'top_left', 'top', 'top_right'. 'left', 'center', 'right', 'bottom_left', 'bottom', 'bottom_right' and by default 'none'

In addition to window aligment you can adjust the window position further with:

	x_padding = *specify offset in pixels*

	y_padding = *specify offset in pixels*


### Advanced window options

The following features enable options that come with recent OSes
and may introduce problems on older software/hardware (fxp. Windows XP),
 but in return they may look really nice if you configure them properly.

#### Transparency

You can enable transparent windows:
     
	transparency = *true or false*

And with transparency comes the ability to change the opacity of the background and the bars:

	foreground_opacity = *range from 0.0 to 1.0*
	background_opacity = *range from 0.0 to 1.0*

#### Advanced window behaviour

Force the window to be behind any window (may not work):
    
	keep_below = *true or false*

Set window properties (useful for scripts relying on window class information):

	set_win_props = *true or false*

This changes the X11 window type so that the compositor doesn't
apply shadows and blur onto the window. However, this is not
guaranteed to work, plus additional breakage may occur. 

Make the window not react on mouse input (it just lets the click
 go through the window):
    
	interactable = 0

Pro-tip: You can still gain control of the window by clicking
 it's taskbar icon or by alt-tabbing into it.

#### Fullscreening the visualizer without changing the viewport

In case you want to make the visualizer window fullscreen, but not the visualizer
with it, you can do so by enabling the ``hold_size`` option in the ``[window]``
section of the configuration and enabling ``fullscreen``. Then use the same
options from [window position](#window-position) to change how the visualizer is
places within the window.


### OpenGL

The ``opengl = true/false`` is **deprecated** as of 0.7.0.0.

If you're running a "supported" mode, you're already using OpenGL. No need to
worry.

#### Shaderpacks

Shader packs have been introduced since ``0.7.0.0`` and allow for the user to
customize the visualizer to their liking (provided they know how to write GLSL
shaders). This funcionality works similarly to how it's implemented in Minecraft
Optifine hence the name.

In the ``[gl]`` section of the config file, you'll find that there are two
"shaderpack" options: ``pre_shaderpack`` and ``post_shaderpack``. As the name
suggests, the ``pre`` shaderpack is run before the visualizer is rendered and
the ``post`` shaderpack is additional effects that are applied **after** the
visualizer has been rendered. This customizability allows for the visualizer to
be significantly modified to look unique. By default, you can look which
"shaderpacks" are include by going in the ``example_files/gl/shaders`` then
``pre`` or ``post`` and seeing which folders are in those directories.

If you think you can write shaders on your own, you can write your own by
creating a folder at ``config directory/gl/shaders/(pre or post)/your_shader_name``
and looking at the shaders in the ``example_files`` folder for reference.

#### Resolution scaling

In case that some of the shaders create aliasing effects [as seen here](https://www.definition.net/define/anti-aliasing)
you can additionally enable resolution scaling to render more pixels on screen
then that of which are visible. The option is accessible at ``resolution_scale``
in the ``[gl]`` section of the config file and also **only works on "supported"
modes**.


### VSync

VSync is enabled by default on XAVA.

You can change it's behaviour or just outright disable it.

To do that open the configuration file and in the ``general`` section you will
find:

	vsync = 1 (1 means its enabled)
	vsync = -1 (enable variable refresh rate)
	vsync = 0 (disable Vsync)
	vsync = 2 (custom framerate, the number is a divisor of the refresh rate of your display)


### Bars

In XAVA you can customize the look of the visualizer.

In the ``[general]`` section you'll find two options that let  you change the
size of the bars and the space between them:

	bar_width = *width in pixels*
	bar_spacing = *width in pixels*

To limit the number of bars displayed on the window, set the following in the
``[general]`` category:

	bars = *insert number here*


### Colors and gradients

In the ``color`` section of the config file you can find:

	background = *by name or by hex value*
	foreground = *same as above*

Additionally, you can specify ``default`` if you want to use your system accent
or Xresources color instead. Supported in ``win`` and ``x11`` output modes only.

But if you want to have gradients on the bars instead of static colors, you can
enable them by changing:

	gradient_count = *number of colors higher than 1*
	
	gradient_color_1 = *same rule as colors*
	gradient_color_2 = *same rule as colors*
	...


### Additional options

XAVA still has plenty to offer, just look up all of the options in the config
file. The config file also serves as documentation, so that you won't get stuck
in configuration.


Contribution
------------

Please read CONTRIBUTING.md before opening a pull request.

Thanks to:
* [karlstav](https://github.com/karlstav)
* [CelestialWalrus](https://github.com/CelestialWalrus)
* [anko](https://github.com/anko)
* [livibetter](https://github.com/livibetter)
* [dpayne](https://github.com/dpayne)

for major contributions in the early development of this project.

