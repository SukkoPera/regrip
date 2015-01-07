# Regrip

Regrip is an Audio CD (CD-DA) player and ripper for the Linux desktop.

It has the ripping capabilities of cdparanoia builtin and provides output
modules for many common file formats, both lossy, such as MP3, OGG, Opus, and
lossless, such as WAV and FLAC, letting you take a disc and transform it easily
straight into your favorite format. Internet disc lookups are also supported
for retrieving track information from disc database servers.

Regrip is a fork of Grip, a once-famous CD player for the GNOME desktop:
http://sourceforge.net/projects/grip

# Requirements

To use Regrip, you must have:

* A computer with a CD/DVD-Rom drive (Surprise, surprise!)
* A modern Linux distribution with GTK+ 2.0 and several other libraries (It
   might also work under other Unix-like systems, but this has not yet been
   tested)
* A net connection (if you want to use disc database lookup)

Regrip uses several external libraries. The following is a list of libraries
required for proper operation on a recent Ubuntu system:

* `libgtk2.0`
* `libvte`
* `libcdparanoia` (https://xiph.org/paranoia/)

Support for file tagging is optional and relies on TagLib:
* `libtagc0` (http://taglib.github.io)

To produce useful output, you will also need an output module, each of which
has it own requirements:
 
* WAV/OGG/FLAC output module:
 * `libsndfile1` (http://www.mega-nerd.com/libsndfile/)
 
* AAC output module:
 * `libfaac` (http://www.audiocoding.com/faac.html)

* Opus output module (currently broken):
 * `libopus` (http://opus-codec.org)
 * `libsamplerate0` (http://www.mega-nerd.com/SRC/)


# Installation

If you obtained Regrip through the package management system of your
distribution, there is probably nothing more to do. If you want to compile
from sources, you will need to make sure that the software development packages
are available on your system. On an Ubuntu system this means:

* `build-essential`
* `cmake`
* `-dev` packages of the packages listed under *Requirements*

Then you will need to go through the following steps:

1. Unpack Regrip and cd to the extracted directory
2. `mkdir BUILD`
3. `cd BUILD`
4. `cmake ..`
5. Optionally, if you want to customize the build options: `make edit_cache`
5. `make`
6. `sudo make install`

# Running Regrip

Regrip usage is:

`regrip [options]`

  where the available options are:

    --device=DEVICE             Specify the cdrom device to use
    --scsi-device=DEVICE        Specify the generic scsi device to use
    --small                     Launch in "small" (CD-only) mode
    --local                     "Local" mode -- do not look up disc info on
                                the net
    --no-redirect               Do not do I/O redirection
    --verbose                   Run in verbose (debug) mode

# Getting More Help

For more information, see the online documentation within Regrip. It can also
be accessed locally in the source distribution. Load doc/C/grip/grip.html
into an html viewer.

To report a bug with Regrip, or to submit a patch, please use the Issue Tracker
on the Regrip GitHub page:
  https://github.com/SukkoPera/regrip/issues

---
SukkoPera <software AT sukkology DOT net>

https://github.com/SukkoPera/regrip
