# alsadump

Experimentally using `LD_PRELOAD` to capture raw PCM output of single applications that use ALSA's `libasound.so`. It has the advantage that you don't need to setup a capturing environment and deal with additional audio sinks, correct timings, avoid recording silence and so on and so forth.

## Usage

Make sure that the application you want to capture is using ALSA and preload the shared library when starting, for example with `mplayer`:

	$ LD_PRELOAD=./alsadump.so mplayer some_music.mp3

When playback starts, the dump should start and write to a file named after the current timestamp. When playback stops, the file is closed.

At the moment, a new dump is created when `snd_pcm_open` or `snd_pcm_start` is called. All data passed to `snd_pcm_writei` will be written to that file. Additional `snd_pcm_start` or `snd_pcm_close` closes the file.

How exactly these functions are used depends on the application, so there might be customizations needed to get correctly split dumps.

If you don't want to simultaneously listen to the recorded sound, you can set the environment variable `DUMP_ONLY=true`, and the PCM data are no longer passed to the PCM device. This can also have an interesting impact on the application that uses synchronous mode in serving the PCM data as fast as possible, so that you don't have to capture in real-time speed.

## Converting raw to mp3

You need the format, rate and channels:

    $ ffmpeg -f s32le -ar 44100 -ac 2 -i input.raw -codec:a libmp3lame -b:a 128k output.mp3

Alternatively:

Write wav header for raw PCM data.

You need to find the correct parameters for encoding, sample rate, sample size, channels:

    $ sox -t raw -e signed-integer -r 44100 -b 32 -c 2 input.raw output.wav

Convert wav to mp3:

    $ ffmpeg -i input.wav -codec:a libmp3lame -b:a 128k output.mp3

## Sound configuration

### Disable pulseaudio

	$ systemctl --user mask pulseaudio.socket
	$ systemctl --user stop pulseaudio.service

### Default ALSA configuration

Example `/etc/asound.conf` (or alternatively `~/.asoundrc`):

```
pcm.dmixed {
    type dmix
    ipc_key 1024
    ipc_key_add_uid 0
    slave.pcm "hw:0,0"
}

pcm.dsnooped {
    type dsnoop
    ipc_key 1025
    slave.pcm "hw:0,0"
}

pcm.duplex {
    type asym
    playback.pcm "dmixed"
    capture.pcm "dsnooped"
}

# Instruct ALSA to use pcm.duplex as the default device
pcm.!default {
    type plug
    slave.pcm "duplex"
}
ctl.!default {
    type hw
    card 0
}
```