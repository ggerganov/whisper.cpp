import * as path from 'path'
import { describe, it, expect } from 'vitest'

const { whisper } = require(path.join(
  __dirname,
  "../../../build/Release/whisper-addon"
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

const whisperParamsMock = {
  language: "en",
  model: path.join(__dirname, "../../../models/ggml-base.en.bin"),
  fname_inp: [path.join(__dirname, "../../../samples/jfk.wav")],
  use_gpu: true,
} satisfies WhisperParams;

describe("Run whisper.node", () => {
    it("it should receive a non-empty value", async () => {
        const result = await whisperAsync(whisperParamsMock, (index) => {
          console.log({index})
        })

        expect(result.length).toBeGreaterThan(0);
    }, 100000);
});

