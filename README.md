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
  - [Installing manually](#installing-manually)
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
  - [Configuring OpenGL](#configuring-opengl)
    - [Programming OpenGL shaders](#programming-opengl-shaders)
  - [Configuring Cairo](#configuring-cairo)
  - [Basic window options](#basic-window-options)
  - [Window position](#window-position)
  - [Advanced window options](#advanced-window-options)
    - [Transparency](#transparency)
    - [Advanced window behaviour](#advanced-window-behaviour)
    - [Fullscreening the visualizer without changing the viewport](#fullscreening-the-visualizer-without-changing-the-viewport)
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

You are free to use either ``ncurses`` or ``x11_cairo`` output methods.

No OpenGL, because Apple likes to be Apple.


### Linux

I recommend grabbing the AppImage, as it deals with all dependency issues upfront
and all you **really** need to to is just download and run it.

You can grab it from the Releases page or get the
[unstable](https://nightly.link/nikp123/xava/workflows/build.yaml/unstable/xava-unstable-x86_64.AppImage.zip)
version.


### Installing manually

Since there's an AppImage there's no point to this, but essentially you're
supposed to use CMake and some basic binutils, like so:

```
git clone https://github.com/nikp123/xava
cd xava
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make
sudo make install
```

And uninstalling with:
```
cat install_manifest.txt | xargs sudo rm -
```

Updating
--------

If you happen to be updating this program, just keep in mind that breaking
changes DO occur sometimes. 

In most cases, errors that show up might look like
[issue #43](https://github.com/nikp123/xava/issues/43)
which (likely) can be fixed with a simple config reset.

In order to fix it I recommend just resetting your config by deleting:

* ``%APPDATA%\xava`` on Windows
* ``$HOME/.config/xava`` on Linux

You will lose your modifications if you do this, so I'd recommend making a
backup first.

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

To-do Linux instructions: But basically just make sure that the ``xava``
executable or the AppImage one runs at startup.


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

Since 0.7.1.1 the output methods have been sorted into three categories:
 * OpenGL output methods
 * Cairo output methods
 * Unsupported output methods (basically those that don't have **good** support)

In the ``[output]`` section of the config there's an parameter named ``method``.
This "method" determines what backend system you will like XAVA to use for it's
visualizations.

For example:
 * Windows uses ``win``
 * Linux uses either ``x11`` or ``wayland``
 * Any other operating system is going to use ``sdl2``
 * And unsupported modes such as ``ncurses`` (which is a terminal display)

The philosophy is that you'd most likely want your CPU cycles spared, so you'd
use OpenGL by default. This means when you choose ``win`` (for example) that it
will use OpenGL for rendering it's graphics. 

However, if you wish to out-out, there's the CPU-drawn ``cairo`` mode. In order
to use it, you just need to append ``_cairo`` at the end of the output method's
name, like so: ``win_cairo``.

Both Cairo and OpenGL use what I call "modules" that you can mix and match to
display whatever you want in the visualizer. These can be adjusted by going into
their respective categories within the config file:
 * For OpenGL there's the ``[gl]`` category
 * For Cairo there's the ``[cairo]`` category

### Configuring OpenGL

In OpenGL you can pick ONLY ONE module to run at a time, and it changes (mostly)
how the audio data is represented to you. Do you want ``bars`` or ``bars_circle``
..etc.?

You can (optionally) apply a post-processing effect to said module via the
``post_shader`` option. Currently packed-in shaders are:
 * ``default`` - does nothing special basically
 * ``shadow`` - adds simple 2D shadows
 * ``kinetic`` - changes background opacity based on the intensity of music

#### Programming OpenGL shaders

You are free to add more if you'd like, though it requires programming knowledge.
The files are located in your config's directory > gl > shaders > the shader's
name. There you must add three files:
 * ``config.ini`` - You can figure this one out by looking at how other shaders do it.
 * ``fragment.glsl``
 * ``vertex.glsl``

And optionally:
 * ``geometry.glsl``

### Configuring Cairo

Cairo, as opposed to OpenGL, is much simpler, but that comes with graphical
effects limitations (and sometimes) bad performance. But unlike OpenGL, it is
*more* flexible. One example of it's flexibility is it's ability to add as many
modules as you'd like in any order that you'd like.

By going into the ``[cairo]`` section of the config you can add modules by
appending (or changing) the line/option:
``module_$indexnumber = module's name``
Where ``$indexnumber`` represents the order at which something is drawn and
it **must** start from 1 and increment accordingly ``2, 3`` and so forth.

Currently, there are 4 available modules you can choose from:
 * ``bars`` - draws horizontal bars as it regularely would
 * ``stars`` - draws stars on the screen that move based on the intensity of
 music
 * ``kinetic`` - changes the background opacity based on the intensity of must,
 be absolutely sure that it's at the lowest "layer" (aka 0) otherwise it WILL
 overwrite everything on the screen
 * ``media_info`` (Linux-only) - Displays song artist, title and artwork in the
 top left part of the screen.

Mix and match these for optimum effect, however, be sure to not do too much as
you can easily overwhelm your system by using this since it's CPU-based graphics.

If you want the lightest possible setup, I recommend just sticking with the default
``bars`` module.

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


### VSync

VSync is enabled by default on XAVA and works ONLY if you use OpenGL.

You can change its behavior or just outright disable it.

To do that open the configuration file and in the ``[general]`` section you will
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

