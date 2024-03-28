interface WhisperParams {
  language: string;
  model: string;
  fname_inp: string[];
  use_gpu: boolean;
}

type WhisperResultItem = [
  /** start */
  string,
  /** end */
  string,
  /** subtitle */
  string,
]
