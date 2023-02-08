rm -rf ./build
rm -rf ./examples/addon.node/whisper

npx cmake-js compile -T whisper-addon

mkdir ./examples/addon.node/whisper
cp ./build/Release/* ./examples/addon.node/whisper

rm -rf ./build

cd ./examples/addon.node
node ./index.js --model=../../models/tiny.en.bin --fname_inp=../../samples/jfk.wav

# pkg index.js --target node16-macos-arm64

# rm ./whisper/addon.node

# ./index --model=../../models/tiny.en.bin --fname_inp=../../samples/jfk.wav

# cd ..

# ./addon.node/index --model=../models/tiny.en.bin --fname_inp=../samples/jfk.wav

# cp ./index ../../index

# cd ../..

# ./index --model=./models/tiny.en.bin --fname_inp=./samples/jfk.wav