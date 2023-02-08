const arguments = process.argv.slice(2);
const params = Object.fromEntries(
  arguments.reduce((pre, item) => {
    if (item.startsWith("--")) {
      return [...pre, item.slice(2).split("=")];
    }
    return pre;
  }, [])
);

const { whisper } = require(params.whisper);

const whisperParams = {
  language: "en",
  model: "",
  fname_inp: "",
};

for (const key in params) {
  if (whisperParams.hasOwnProperty(key)) {
    whisperParams[key] = params[key];
  }
}

console.log("whisperParams =", whisperParams);
console.log(whisper(whisperParams));
