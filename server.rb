require 'sinatra'
require 'shellwords'

set :bind, '0.0.0.0'

get '/' do
  <<-HTML
    <html>
    <body>
      <h1>Record Audio</h1>
      <button onclick="startRecording()">Start Recording</button>
      <button onclick="stopRecording()" disabled>Stop Recording</button>
      <audio id="audioPlayback" controls></audio>

      <script>
        let mediaRecorder;
        let audioChunks = [];

        function startRecording() {
          navigator.mediaDevices.getUserMedia({ audio: true })
            .then(stream => {
              mediaRecorder = new MediaRecorder(stream);
              mediaRecorder.ondataavailable = e => {
                audioChunks.push(e.data);
              };
              mediaRecorder.onstop = e => {
                const audioBlob = new Blob(audioChunks, { type: 'audio/wav' });
                const audioUrl = URL.createObjectURL(audioBlob);
                const audio = document.getElementById('audioPlayback');
                audio.src = audioUrl;

                // Prepare and send the POST request
                const xhr = new XMLHttpRequest();
                xhr.open("POST", "/upload", true);
                xhr.setRequestHeader("Content-Type", "audio/wav");
                xhr.send(audioBlob);
              };
              mediaRecorder.start();
              document.querySelector('button[onclick="startRecording()"]').disabled = true;
              document.querySelector('button[onclick="stopRecording()"]').disabled = false;
            })
            .catch(e => console.error(e));
        }

        function stopRecording() {
          mediaRecorder.stop();
          document.querySelector('button[onclick="startRecording()"]').disabled = false;
          document.querySelector('button[onclick="stopRecording()"]').disabled = true;
          audioChunks = [];
        }
      </script>
    </body>
    </html>
  HTML
end

post '/upload' do
  # Retrieve filename from parameters or use a default one
  filename = 'audio.wav'

  # Write the request body to an audio file
  File.open(filename, 'wb') do |file|
    request.body.rewind
    file.write(request.body.read)
  end

  # Remove existing output.wav
  `rm -f output.wav`

  # Convert to required format
  `ffmpeg -i #{filename} -ar 16000 -ac 1 -c:a pcm_s16le output.wav`

  "Audio file uploaded"
end

get '/execute' do
  `./main -f output.wav`
end
