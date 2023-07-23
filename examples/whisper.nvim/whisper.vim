" The current whisper ecosystem shows mighty powerful potential, but seems to
" lack the required structure to make a speech to text plugin frictionless.
" The most direct path forward will be to have a standalone library interfaced
" with vim's libcall
" Libcall only allows for a single argument(a string or number) and a single
" output (always a string).
" This... honestly fits well for common interactions as follows
"  init(modelname) -> (null string for success or error message)
"  unload(ignored) -> (null string for success or error message)
"   likely never needed
"  processCommand(newline separated commands string) -> commands
"   Should have support for consecutive commands
"  stream(maybe sentinel?) -> processed text.
"
" Support for streaming responses is desired, but care is needed to support
" backtracking when more refined output is available.
" Perhaps the greatest element of difficulty, speech input should be buffered
" and it should be possible to 'rewind' input to mask latency and pivot off
" modal changes. (If a command sends the editor to insert mode, stt should no
" longer be limited by the command syntax)
"
" For now though, a simple proof of concept shall suffice.
if !exists("g:whisper_dir")
   let g:whisper_dir = expand($WHISPER_CPP_HOME)
   if g:whisper_dir == ""
      echoerr "Please provide a path to the whisper.cpp repo in either the $WHISPER_CPP_HOME environment variable, or g:whisper_dir"
   endif
endif
if !exists("g:whisper_stream_path")
   if executable("stream")
      " A version of stream already exists in the path and should be used
      let g:whisper_stream_path = "stream"
   else
      let g:whisper_stream_path = g:whisper_dir .. "stream"
      if !filereadable(g:whisper_stream_path)
         echoerr "Was not able to locate a stream executable at: " .. g:whisper_stream_path
         throw "Executable not found"
      endif
   endif
endif
if !exists("g:whisper_model_path")
   " TODO: allow paths relative the repo dir
   let g:whisper_model_path = g:whisper_dir .. "models/ggml-base.en.bin"
   if !filereadable(g:whisper_model_path)
      echoerr "Could not find model at: " .. g:whisper_model_path
      throw "Model not found"
   endif
endif
let s:streaming_command = [g:whisper_stream_path,"-m",g:whisper_model_path,"-t","8","--step","0","--length","5000","-vth","0.6"]

let s:listening = v:false
let s:cursor_pos = getpos(".")
let s:cursor_pos[0] = bufnr("%")
let s:loaded = v:false
func s:callbackHandler(channel, msg)
   " Large risk of breaking if msg isn't line buffered
   " TODO: investigate sound_playfile as an indicator that listening has started?
   if a:msg == "[Start speaking]"
      let s:loaded = v:true
      if s:listening
         echo "Loading complete. Now listening"
      else
         echo "Loading complete. Listening has not been started"
      endif
   endif
   if s:listening
      let l:msg_lines = split(a:msg,"\n")
      let l:new_text = ""
      for l:line in l:msg_lines
         " This is sloppy, but will suffice until library is written
         if l:line[0] == '['
            let l:new_text = l:new_text .. l:line[28:-1] .. ' '
         endif
      endfor
         let l:buffer_line = getbufoneline(s:cursor_pos[0],s:cursor_pos[1])
      if len(l:buffer_line) == 0
         " As a special case, an empty line is instead set to the text
         let l:new_line = l:new_text
         let s:cursor_pos[2] = len(l:new_text)
      else
         " Append text after the cursor
         let l:new_line = strpart(l:buffer_line,0,s:cursor_pos[2]) .. l:new_text
         let l:new_line = l:new_line .. strpart(l:buffer_line,s:cursor_pos[2])
         let s:cursor_pos[2] = s:cursor_pos[2]+len(l:new_text)
      endif
      call setbufline(s:cursor_pos[0],s:cursor_pos[1],l:new_line)
   endif
endfunction

function! whisper#startListening()
   let s:cursor_pos = getpos(".")
   let s:cursor_pos[0] = bufnr("%")
   let s:listening = v:true
endfunction
function! whisper#stopListening()
   let s:listening = v:false
endfunction
function! whisper#toggleListening()
   let s:cursor_pos = getpos(".")
   let s:cursor_pos[0] = bufnr("%")
   let s:listening = !s:listening
   if s:loaded
      if s:listening
         echo "Now listening"
      else
         echo "No longer listening"
      endif
   endif
endfunction

" Note this includes stderr at present. It's still filtered and helps debugging
let s:whisper_job = job_start(s:streaming_command, {"callback": "s:callbackHandler"})
" TODO: Check lifetime. If the script is resourced, is the existing
" s:whisper_job dropped and therefore killed?
if job_status(s:whisper_job) == "fail"
   echoerr "Failed to start whisper job"
endif
