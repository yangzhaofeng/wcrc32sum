# wcrc32sum
This program was original uploaded by iceeLyne at [hydrogenaudio](https://hydrogenaud.io/index.php/topic,102787.0.html). I reupload it to github in case the original website is inaccessible.

You can compile it with `gcc -o wcrc32sum -std=c99 -O2 wcrc32sum.c`

The program computes CRC32 ckecksums of the PCM audio data in RIFF WAV files, for validating files against the EAC logs.

Examples for this program:
```
wcrc32sum foo.wav bar.wav
wcrc32sum -r foo.wav bar.wav
wcrc32sum -n foo.wav -c0 bar.wav
wcrc32sum --help
```
pipe usage
```
flac -c -d foo.flac | wcrc32sum -n
```
