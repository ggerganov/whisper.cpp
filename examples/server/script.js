import http from "k6/http";
import { check } from "k6";
import { FormData } from "https://jslib.k6.io/formdata/0.0.2/index.js";

const audioFile = open("../../samples/jfk.wav", "b"); // 11s audio sample

export const options = {
  scenarios: {
    ramping_load: {
      executor: "ramping-arrival-rate",
      startRate: 1,
      timeUnit: "1s",
      preAllocatedVUs: 4, // Pre-allocate 4 VUs
      maxVUs: 4,
      stages: [
        { target: 4, duration: "10s" }, // Start with 4 requests per second for warm-up
        { target: 1, duration: "50s" }, // Sustain 1 request per second for 50s with 4 VUs
      ],
    },
  },
};

export default function () {
  const fd = new FormData();

  fd.append("file", {
    data: new Uint8Array(audioFile).buffer,
    filename: "jfk.wav",
    content_type: "audio/wav",
  });

  fd.append("temperature", "0.0");
  fd.append("temperature_inc", "0.2");
  fd.append("response_format", "json");

  const res = http.post("http://127.0.0.1:8080/inference", fd.body(), {
    headers: { "Content-Type": "multipart/form-data; boundary=" + fd.boundary },
  });

  check(res, {
    "status is 200": (r) => r.status === 200,
  });
}
