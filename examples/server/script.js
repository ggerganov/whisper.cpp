import http from "k6/http";
import { check } from "k6";
import { FormData } from "https://jslib.k6.io/formdata/0.0.2/index.js";

// Load the binary audio file once to reuse across all virtual users
const audioFile = open("../../samples/jfk.wav", "b"); // 11s audio sample

export const options = {
  scenarios: {
    constant_load: {
      executor: "constant-arrival-rate",
      rate: 4, // 20 requests per second
      timeUnit: "1s", // Defines the time unit for the arrival rate
      duration: "1m", // Total duration of the test (adjust as needed)
      preAllocatedVUs: 4, // Number of VUs to preallocate
      maxVUs: 4, // Maximum number of VUs to allow
    },
  },
};

export default function () {
  // Initialize FormData for multipart/form-data requests
  const fd = new FormData();

  // Append the binary audio file correctly
  fd.append("file", {
    data: audioFile, // Pass the binary string directly without conversion
    filename: "jfk.wav",
    content_type: "audio/wav",
  });

  // Append additional form fields as needed
  fd.append("temperature", "0.0");
  fd.append("temperature_inc", "0.2");
  fd.append("response_format", "json");

  // Perform the HTTP POST request with appropriate headers
  const res = http.post("http://127.0.0.1:8080/inference", fd.body(), {
    headers: { "Content-Type": "multipart/form-data; boundary=" + fd.boundary },
    timeout: "30s", // Set a timeout to prevent hanging requests (adjust as needed)
  });

  // Validate the response status
  check(res, {
    "status is 200": (r) => r.status === 200,
  });
}