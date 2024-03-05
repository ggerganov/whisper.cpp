const path = require("path");
const { whisper } = require(path.join(
  __dirname,
  "../../../build/Release/whisper-addon"
));

function whisperAsync(params, callback) {
  return new Promise((resolve, reject) => {
    whisper(params, (error, result) => {
      if(error) {
        return reject(error);
      } else if(result.res){
        return resolve(result.res);
      }
      callback(result.index)
    })
  })
}

const whisperParamsMock = {
  language: "en",
  model: path.join(__dirname, "../../../models/ggml-base.en.bin"),
  fname_inp: [path.join(__dirname, "../../../samples/jfk.wav")],
  use_gpu: true,
};

describe("Run whisper.node", () => {
    test("it should receive a non-empty value", async () => {
        const result = await whisperAsync(whisperParamsMock, (index) => {
          console.log({index})
        })

        expect(result.length).toBeGreaterThan(0);
    }, 100000);
});

