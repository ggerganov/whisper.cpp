# wchess

Voice-controlled chess using Whisper

Online demo: https://whisper.ggerganov.com/wchess/

https://github.com/ggerganov/whisper.cpp/assets/1991296/c2b2f03c-9684-49f3-8106-357d2d4e67fa

## Command-line tool

```bash
mkdir build && cd build
cmake -DWHISPER_SDL2=1 ..
make -j

./bin/wchess -m ../models/ggml-base.en.bin

Move: start

a b c d e f g h
r n b q k b n r 8
p p p p p p p p 7
. * . * . * . * 6
* . * . * . * . 5
. * . * . * . * 4
* . * . * . * . 3
P P P P P P P P 2
R N B Q K B N R 1

White's turn
[(l)isten/(p)ause/(q)uit]: 
```

## TODO

- Improve web-browser audio capture - sometimes it does not record the voice properly
- Add support for more languages by making the generated grammar string multi-lingual
- Fix bugs in the chess moves logic

PRs welcome!
