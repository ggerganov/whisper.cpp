if !exists("g:whisper_dir")
   let g:whisper_dir = expand($WHISPER_CPP_HOME)
   if g:whisper_dir == ""
      echoerr "Please provide a path to the whisper.cpp repo in either the $WHISPER_CPP_HOME environment variable, or g:whisper_dir"
   endif
endif
if !exists("g:whisper_lsp_path")
      let g:whisper_lsp_path = g:whisper_dir .. "lsp"
      if !filereadable(g:whisper_lsp_path)
         echoerr "Was not able to locate a lsp executable at: " .. g:whisper_lsp_path
         throw "Executable not found"
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
let s:output_buffer = bufnr("whisper_log", v:true)
call setbufvar(s:output_buffer,"&buftype","nofile")
let s:lsp_command = [g:whisper_lsp_path,"-m",g:whisper_model_path]
"let s:lsp_command = [g:whisper_lsp_path,"-m","/home/austin/whisper.cpp/models/ggml-tiny.en.bin", "-ac", "128"]

let s:commands_text = ["up", "down", "left", "right", "from", "till", "back", "word", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "change", "insert", "append", "delete", "go", "yank", "undo", "open", "paste", "repeat", "save", "run", "quotation", "exit"]
let s:commands_ex = ["k", "j", "h", "l", "f", "t", "b", "w", "1", "2", "3", "4", "5", "6", "7", "8", "9", "o", "c", "i", "a", "d", "g", "y", "u", "o", "p", ".", "save", "run", '"', "exit"]
"let s:commands_text = ["up", "down", "left", "right"]
"let s:commands_ex = ["k","j","h","l"]

" If a command does not include a whole actionable step, attempting to execute
" it discards the remainder of things. There is likely a simpler solution,
" but it can be made functional now by storing a backbuffer until actionable
let s:command_backlog = ""
func s:commandCallback(channel, msg)
   if a:msg.result.command_text == "exit"
      call appendbufline(s:output_buffer, "$", "Ending command processing")
      return
   endif
   let l:do_execute = v:false
   let l:command = s:commands_ex[a:msg.result.command_index]
   let s:command_backlog = s:command_backlog .. l:command
   if index(["k","j","h","l","u","o",".","b","w","p"],l:command) != -1
      let l:do_execute = v:true
   endif
   if (index(["y","d","c","g"], l:command) != -1) && (l:command == s:command_backlog[-1:-1])
      let l:do_execute = v:true
   endif
   if l:do_execute
      exe "normal" s:command_backlog
      let s:command_backlog = ""
   endif
   call whisper#requestCommands({"timestamp": a:msg.result.timestamp})
endfunction

func s:logCallback(channel, msg)
   call appendbufline(s:output_buffer,"$",a:msg)
endfunction

"requestCommands([params_dict])
func whisper#requestCommands(...)
   let l:req = {"method": "guided", "params": {}}
   if a:0 > 0
      call extend(l:req.params, a:1)
   endif
   let resp = ch_sendexpr(g:lsp_job, l:req, {"callback": "s:commandCallback"})
endfunction

"TODO update to instead send strings to a callback.
"This is likely the cleanest way to allow this single method to also serve
"search (/) and ex(:) inputs
fun s:transcriptionCallback(channel, msg)
   call appendbufline(s:output_buffer,"$",a:msg)
   let l:pos = getcurpos()
   let l:tr = a:msg.result.transcription

   let l:ex_ind = match(tolower(l:tr),"exit", len(l:tr)-6)
   "The worst case I've observed so far is " Exit.", which is 6 characters
   if l:ex_ind != -1
      call appendbufline(s:output_buffer, "$", "Ending unguided transcription")
      exe "normal a" .. strpart(l:tr,0,l:ex_ind-1)
   else
      exe "normal a" .. l:tr
      let req = {"method": "unguided", "params": {"timestamp": a:msg.result.timestamp, "no_context": v:false}}
      let resp = ch_sendexpr(g:lsp_job, req, {"callback": "s:transcriptionCallback"})
   endif
endfunction

"doTranscription([params_dict])
func whisper#doTranscription(...)
   let l:req = {"method": "unguided", "params": {}}
   if a:0 > 0
      call extend(l:req.params, a:1)
   endif
   let resp = ch_sendexpr(g:lsp_job, l:req, {"callback": "s:transcriptionCallback"})
endfunction

func g:Lsp_echo(channel, msg)
   let req = {"method": "echo", "params": {"dummy": "dummy"}}
endfunction

func s:registerCommandset(commandlist)
   let req = {"method": "registerCommandset"}
   let req.params = a:commandlist
   let resp = ch_sendexpr(g:lsp_job, req, {"callback": "s:logCallback"})
endfunction

func whisper#registerCommandset(commandlist)
   call s:registerCommandset(a:commandlist)
endfunction


" TODO: Check lifetime. If the script is resourced, is the existing
" s:wlsp_job dropped and therefore killed?
" This seems to not be the case and I've had to deal with zombie processes
" that survive exiting vim, even though this should not be the case
"let s:lsp_opts = {"in_mode": "lsp", "out_mode": "lsp", "err_mode": "nl", "out_cb": function('LspOutCallback'), "err_cb": function("LspErrCallback"), "exit_cb": function("LspExitCallback")}
let s:lsp_opts = {"in_mode": "lsp", "out_mode": "lsp", "err_mode": "nl", "err_io": "buffer", "err_buf": s:output_buffer}
if !exists("g:lsp_job")
   let g:lsp_job = job_start(s:lsp_command, s:lsp_opts)
   if job_status(g:lsp_job) == "fail"
      echoerr "Failed to start whisper job"
   endif
   call s:registerCommandset(s:commands_text)
endif
