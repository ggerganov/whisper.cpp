const path = require('path');
const { whisper } = require(path.join(__dirname, '../../build/Release/whisper-addon'));

const whisperParams = {
    language: 'en',
    model: path.join(__dirname, '../../models/ggml-base.en.bin'),
    fname_inp: '',
};

const arguments = process.argv.slice(2);
const params = Object.fromEntries(
    arguments.reduce((pre, item) => {
        if (item.startsWith("--")) {
            return [...pre, item.slice(2).split("=")];
        }
        return pre;
    }, []),
);

for (const key in params) {
    if (whisperParams.hasOwnProperty(key)) {
        whisperParams[key] = params[key];
    }
}

console.log('whisperParams =', whisperParams);
console.log(whisper(whisperParams));
