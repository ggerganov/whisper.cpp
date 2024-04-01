const path = require("path");
const { whisper } = require(path.join(
  __dirname,
  "../../build/Release/whisper-addon"
));
const { promisify } = require("util");

const whisperAsync = promisify(whisper);

const whisperParams = {
  language: "en",
  model: path.join(__dirname, "../../models/ggml-base.en.bin"),
  fname_inp: path.join(__dirname, "../../samples/jfk.wav"),
  use_gpu: true,
  no_timestamps: true,
  no_prints: true,
  comma_in_time: false,
  translate: true
};

const arguments = process.argv.slice(2);
const params = Object.fromEntries(
  arguments.reduce((pre, item) => {
    if (item.startsWith("--")) {
      return [...pre, item.slice(2).split("=")];
    }
    return pre;
  }, [])
);

for (const key in params) {
  if (whisperParams.hasOwnProperty(key)) {
    whisperParams[key] = params[key];
  }
}

console.log("whisperParams =", whisperParams);

whisperAsync(whisperParams).then((result) => {
  console.log();
  console.log(result);
});
