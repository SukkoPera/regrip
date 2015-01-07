# Regrip TODO list
* Support MP3 through LAME
* Support M4A through FAAC

 > Basically AAC with a different container, but requires fiddling with bits to put in the container.

* Support Opus

 > Initial encoder is there, but is not working correctly. Yes, I hate having to fit the stream in the container.

* Make encoders configurable

 > Some GSettings stuff is there, need more and GUIs.

* Consider plugin architecture, maybe using libpeas

 > Main issue is that libpeas seems to only support GTK3 for plugin configuration GUIs.

* Switch to `libcddb` for CDDB support

* > Almost there.

* Add *MusicBrainz* support 
* Add *Discogs* support
* Use `libcdio` for CD playing functions.
* Add *AccurateRip* support
* Use DBus (maybe through GIO/GDrive) to autodetect CD drives and media insertion/ejection, instead of polling drives directly.
* Make external library of the gain analysis part
* Remove any code specific to the Solaris port

 > Not a good thing, per se, but I cannot test that, plus relaying on external library also aims at making any explicit support unnecessary.

* Switch to GtkBuilder

 > Added initial stuff, already used for multiple CDDB reults dialog.

* Reorganize GUI

 > Preferences should have their own separate dialog, as the ripping process should, not to mention program credits. A toolbar and a status bar might also be handy.

# Changes completed in Regrip
* Switch to CMake
* Use GSettings for user settings
* Remove support for cdda2wav and external cdparanoia, only support libcdparanoia
* Remove ability to rip and encode separately

 > This probably made sense when computers were slow, but since now encoding is faster than realtime I really see no need for that.

* Refactor encoding process
* Support WAV/OGG/FLAC through `libsndfile`
* Support AAC through `FAAC`
* Replace `ID3Lib` with `TagLib`. This allows to put tags in OGG and FLAC files as well as MP3s.

# Original Grip TODO list
* ~~add ability to browse for rip/encode executables~~

 > No longer necessary.

* overall progress indicator for multi-track rips

 > Seems done, but needs fixes.

* ~~abort current track from multi-track rip~~

 > Not deemed necessary. When one aborts a rip, he probably wants to stop it altogether.

* command-line arguments for batch operation
* some way to get the track name in condensed mode (perhaps a tooltip?)
* display/edit extended DiscDB info
* allow ejecting when the tray is closed, but with no disc
* slider to move within a track
* ~~add a timeout for DiscDB lookup~~

 > No longer necessary since we rely on `libcddb` now.

* check rip/encode result status and repeat if necessary
* avoid locking of drive
* update the track time properly when paused and the track is changed
* update the playtime during ripping/encoding?
* digital playing of cds (through CDDA)
* allow per-song year specification?
* better display of cdparanoia error status
* size info as tooltip on rip/encode progress bar
* remember previous position when smart-relocating on expand
* ~~support multiple cdrom drives?~~

 > Not deemed necessary.

* remember Disk Editor toggle status
* ask if the whole disc should be ripped if no track is selected

 > Done.

* ~~add an option "CD eject program"~~

 > Will be no longer necessary when we switch to `libcdio`.

* allow different file format for multi-artist cds
