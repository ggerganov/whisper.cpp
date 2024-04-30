import * as path from 'path'

const { whisper } = require(path.join(
  __dirname,
  "../../build/Release/whisper-addon"
));

function whisperAsync(params: WhisperParams, callback?: (index: number) => any) {
  return new Promise<Array<WhisperResultItem>>((resolve, reject) => {
    whisper(params, (error: Error, result: {res: Array<WhisperResultItem>, index: number}) => {
      if(error) {
        return reject(error);
      } else if(result.res){
        return resolve(result.res);
      }
      callback?.(result.index)
    })
  })
}

const whisperParams = {
  language: "en",
  model: path.join(__dirname, "../../models/ggml-base.en.bin"),
  fname_inp: ["../../samples/jfk.wav"],
  use_gpu: true,
} satisfies WhisperParams;

const args = process.argv.slice(2);
const params = Object.fromEntries(
  args.reduce((pre, item) => {
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

whisperAsync(whisperParams).then((res) => {
  console.log(`Result from whisper: ${res}`);
});
