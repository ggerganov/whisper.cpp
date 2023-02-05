const path = require('path');
const { whisper } = require(path.join(__dirname, '../../../build/Release/whisper-addon'));

const whisperParamsMock = {
    language: 'en',
    model: path.join(__dirname, '../../../models/ggml-base.en.bin'),
    fname_inp: path.join(__dirname, '../../../samples/jfk.wav'),
};

describe("Run whisper.node", () => {

    test("it should receive a non-empty value", () => {
        expect(whisper(whisperParamsMock).length).toBeGreaterThan(0);
    });
});
