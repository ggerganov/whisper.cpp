# stream_components

Basically the same functionality as stream, but componentized-out. The microphone streamer is a component, the whisper invocation is a component, the output is another.

This will make it easy to make servers and nodes of various forms: local microphone to local whisper to file, remote microphone to local whisper to websocket, etc.