const path = require("path");
const { whisper } = require(path.join(
  __dirname,
  "../../build/Release/whisper-addon"
));
const { promisify } = require("util");

const whisperAsync = promisify(whisper);

async function runWhisper(whisperParams) {
  await whisperAsync(whisperParams);
}

const whisperParams = {
  language: "en",
  model: path.join(__dirname, "../../models/ggml-base.en.bin"),
  fname_inp: "../../samples/jfk.wav",
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
runWhisper(whisperParams).then((result) => {
  console.log(`This is the whisper result ${result}`);
});
