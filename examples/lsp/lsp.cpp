// command-line parameters
struct whisper_params {
    int32_t n_threads  = std::min(4, (int32_t) std::thread::hardware_concurrency());
    int32_t prompt_ms  = 5000;
    int32_t command_ms = 8000;
    int32_t capture_id = -1;
    int32_t max_tokens = 32;
    int32_t audio_ctx  = 0;

    float vad_thold    = 0.6f;
    float freq_thold   = 100.0f;

    bool speed_up      = false;
    bool translate     = false;
    bool print_special = false;
    bool print_energy  = false;
    bool no_timestamps = true;

    std::string language  = "en";
    std::string model     = "models/ggml-base.en.bin";
    std::string fname_out;
    std::string commands;
    std::string prompt;
};
struct command {
    std::vector<whisper_token> tokens;
    std::string plaintext;
};
struct commandset {
   std::vector<struct command> commands;
   std::vector<whisper_token> prompt_tokens;
   //TODO: Store longest command?
   //Multi-token commands should have probabilities of subsequent logits
   //given that the prior logit is correct.
   //In this case, all commands must be iterated.
   //This however, is likely highly involved as different tokens
   //almost certainly have different spoken lengths
   //It would also have performance implications equivalent to a beam search
};

void whisper_print_usage(int argc, char ** argv, const whisper_params & params);

bool whisper_params_parse(int argc, char ** argv, whisper_params & params) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            whisper_print_usage(argc, argv, params);
            exit(0);
        }
        else if (arg == "-t"   || arg == "--threads")       { params.n_threads     = std::stoi(argv[++i]); }
        else if (arg == "-pms" || arg == "--prompt-ms")     { params.prompt_ms     = std::stoi(argv[++i]); }
        else if (arg == "-cms" || arg == "--command-ms")    { params.command_ms    = std::stoi(argv[++i]); }
        else if (arg == "-c"   || arg == "--capture")       { params.capture_id    = std::stoi(argv[++i]); }
        else if (arg == "-mt"  || arg == "--max-tokens")    { params.max_tokens    = std::stoi(argv[++i]); }
        else if (arg == "-ac"  || arg == "--audio-ctx")     { params.audio_ctx     = std::stoi(argv[++i]); }
        else if (arg == "-vth" || arg == "--vad-thold")     { params.vad_thold     = std::stof(argv[++i]); }
        else if (arg == "-fth" || arg == "--freq-thold")    { params.freq_thold    = std::stof(argv[++i]); }
        else if (arg == "-su"  || arg == "--speed-up")      { params.speed_up      = true; }
        else if (arg == "-tr"  || arg == "--translate")     { params.translate     = true; }
        else if (arg == "-ps"  || arg == "--print-special") { params.print_special = true; }
        else if (arg == "-pe"  || arg == "--print-energy")  { params.print_energy  = true; }
        else if (arg == "-l"   || arg == "--language")      { params.language      = argv[++i]; }
        else if (arg == "-m"   || arg == "--model")         { params.model         = argv[++i]; }
        //else if (arg == "-f"   || arg == "--file")          { params.fname_out     = argv[++i]; }
        //else if (arg == "-cmd" || arg == "--commands")      { params.commands      = argv[++i]; }
        //else if (arg == "-p"   || arg == "--prompt")        { params.prompt        = argv[++i]; }
        else {
            fprintf(stderr, "error: unknown argument: %s\n", arg.c_str());
            whisper_print_usage(argc, argv, params);
            exit(0);
        }
    }

    return true;
}

void whisper_print_usage(int /*argc*/, char ** argv, const whisper_params & params) {
    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s [options]\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  -h,         --help           [default] show this help message and exit\n");
    fprintf(stderr, "  -t N,       --threads N      [%-7d] number of threads to use during computation\n", params.n_threads);
    fprintf(stderr, "  -pms N,     --prompt-ms N    [%-7d] prompt duration in milliseconds\n",             params.prompt_ms);
    fprintf(stderr, "  -cms N,     --command-ms N   [%-7d] command duration in milliseconds\n",            params.command_ms);
    fprintf(stderr, "  -c ID,      --capture ID     [%-7d] capture device ID\n",                           params.capture_id);
    fprintf(stderr, "  -mt N,      --max-tokens N   [%-7d] maximum number of tokens per audio chunk\n",    params.max_tokens);
    fprintf(stderr, "  -ac N,      --audio-ctx N    [%-7d] audio context size (0 - all)\n",                params.audio_ctx);
    fprintf(stderr, "  -vth N,     --vad-thold N    [%-7.2f] voice activity detection threshold\n",        params.vad_thold);
    fprintf(stderr, "  -fth N,     --freq-thold N   [%-7.2f] high-pass frequency cutoff\n",                params.freq_thold);
    fprintf(stderr, "  -su,        --speed-up       [%-7s] speed up audio by x2 (reduced accuracy)\n",     params.speed_up ? "true" : "false");
    fprintf(stderr, "  -tr,        --translate      [%-7s] translate from source language to english\n",   params.translate ? "true" : "false");
    fprintf(stderr, "  -ps,        --print-special  [%-7s] print special tokens\n",                        params.print_special ? "true" : "false");
    fprintf(stderr, "  -pe,        --print-energy   [%-7s] print sound energy (for debugging)\n",          params.print_energy ? "true" : "false");
    fprintf(stderr, "  -l LANG,    --language LANG  [%-7s] spoken language\n",                             params.language.c_str());
    fprintf(stderr, "  -m FNAME,   --model FNAME    [%-7s] model path\n",                                  params.model.c_str());
    //fprintf(stderr, "  -f FNAME,   --file FNAME     [%-7s] text output file name\n",                       params.fname_out.c_str());
    //fprintf(stderr, "  -cmd FNAME, --commands FNAME [%-7s] text file with allowed commands\n",             params.commands.c_str());
    //fprintf(stderr, "  -p,         --prompt         [%-7s] the required activation prompt\n",              params.prompt.c_str());
    fprintf(stderr, "\n");
}
json unguided_transcription(struct whisper_context * ctx, audio_async &audio, const whisper_params &params) {

}

// command-list mode
// guide the transcription to match the most likely command from a provided list
json guided_transcription(struct whisper_context * ctx, audio_async &audio, const whisper_params &params, struct command_set cs) {
    std::vector<float> pcmf32_cur;

    //Constant delays should not be avoided. It may be necessary to do standalone
    //timekeeping and sleep to keep a maximum poll rate.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //TODO: verify correctness here. Audio that has been processed before should
    //not be processed again, but this may show the most viable avenue for brute
    //forcing
    audio.get(2000, pcmf32_cur);
    //TODO: move to while not with delay and new audio.get
    if (::vad_simple(pcmf32_cur, WHISPER_SAMPLE_RATE, 1000, params.vad_thold, params.freq_thold, params.print_energy)) {
       fprintf(stdout, "%s: Speech detected! Processing ...\n", __func__);

       const auto t_start = std::chrono::high_resolution_clock::now();

       whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

       wparams.print_progress   = false;
       wparams.print_special    = params.print_special;
       wparams.print_realtime   = false;
       //TODO: should always be false
       wparams.print_timestamps = !params.no_timestamps;
       wparams.translate        = params.translate;
       wparams.no_context       = true;
       wparams.single_segment   = true;
       wparams.max_tokens       = 1;
       wparams.language         = params.language.c_str();
       wparams.n_threads        = params.n_threads;

       wparams.audio_ctx        = params.audio_ctx;
       wparams.speed_up         = params.speed_up;

       //TODO: Set up command sets/precompute prompts
       wparams.prompt_tokens    = cs.prompt_tokens.data();
       wparams.prompt_n_tokens  = cs.prompt_tokens.size();
       //TODO: properly expose as option
       wparams.suppress_non_speech_tokens = true;

       // run the transformer and a single decoding pass
       if (whisper_full(ctx, wparams, pcmf32_cur.data(), pcmf32_cur.size()) != 0) {
          fprintf(stderr, "%s: ERROR: whisper_full() failed\n", __func__);
          break;
       }

       // estimate command probability
       // NOTE: not optimal
       {
          const auto * logits = whisper_get_logits(ctx);

          std::vector<float> probs(whisper_n_vocab(ctx), 0.0f);

          // compute probs from logits via softmax
          {
             float max = -1e9;
             for (int i = 0; i < (int) probs.size(); ++i) {
                max = std::max(max, logits[i]);
             }

             float sum = 0.0f;
             for (int i = 0; i < (int) probs.size(); ++i) {
                probs[i] = expf(logits[i] - max);
                sum += probs[i];
             }

             for (int i = 0; i < (int) probs.size(); ++i) {
                probs[i] /= sum;
             }
          }

          std::vector<std::pair<float, int>> probs_id;

          double psum = 0.0;
          for (int i = 0; i < (int) allowed_commands.size(); ++i) {
             probs_id.emplace_back(probs[allowed_tokens[i][0]], i);
             for (int j = 1; j < (int) allowed_tokens[i].size(); ++j) {
                probs_id.back().first += probs[allowed_tokens[i][j]];
             }
             probs_id.back().first /= allowed_tokens[i].size();
             psum += probs_id.back().first;
          }

          //TODO: Either expose probabilities, or stop normalizing
          // normalize
          for (auto & p : probs_id) {
             p.first /= psum;
          }

          // sort descending
          {
             using pair_type = decltype(probs_id)::value_type;
             std::sort(probs_id.begin(), probs_id.end(), [](const pair_type & a, const pair_type & b) {
                   return a.first > b.first;
                   });
          }

          //TODO: prevent reprocessing audio
          //An amount of audio should be grabbed equal to current_time - processed timestamp
          //If this delta is not enough for evaluation, block execution until it is.
          //If this amount is too much (circular buffer overrun)...
          //warn user. Skip forward half the audio buffer size to allow contiguous
          //realtime processing until next overrun?
          //
          //Separate of this, there should be some indicator in the client message
          //regarding it's timeliness...
          //If listening was toggled from disabled to enabled,
          //the state is equivalent to that of overrun
          //
          //Add a timestamp to response field.
          //If client sends a request with latency dependent on a prior request,
          //that request should mirror the same timestamp.
          //If the latency is not dependent on a prior request, this field should be left blank
          //and is calculated as current time minus a .1ish second buffer
          //(potentially changeable as launch command line arg)
          //
          //In either case, request completion should block until a processable
          //amount of audio has elapsed which ALSO satisfies the VAD check
          //(Eventually, this server should still poll for client inputs
          //   to allow a request blocking on audio data to be canceled.)
          //N.B. whisper.cpp will refuse to process anything under 1s length
          audio.clear();
       }

       return json{
          "result": probs_id[0].second;
       }
}

json register_commandset(struct whisper_context * ctx, json params, std::vector<struct commandset> commandset_list) {
   struct commandset cs;

   int max_len = 0;
    std::string  k_prompt = " select one from the available words: ";
   for (std::string s : params.begin()) {
      //The existing command implementation uses a nested for loop to tokenize single characters
      //I fail to see the purpose of this when ' a' has a wholly different pronunciation than the start of ' apple'
      //TODO: Investigate reason for whisper_tokenize. Requires more arguments, a copy, and bounds checking is after token list exists in memory.
      const auto tokens = tokenize(ctx->vocab, " " + s);
      //might be a cleaver way to avoid copy, but I think there's a
      //type mismatch between whisper token and whisper_vocab::id
      struct command command;
      command.plaintext = s;
      command.tokens(tokens);
      max_len = std::max(max_len, tokens.size());
      cs.commands.push_back(commands);
      k_prompt += s;
   }
   k_prompt = k_prompt.substr(0,k_prompt.length()-2) + ". Selected word:";
   cs.prompt_tokens(tokenize(ctx->vocab, k_prompt));

   //prepare response
   int i = commandset_list.size();
   commandset_list.push_back(cs);
   return i;//may need manual cast
}
json seek(struct whisper_context * ctx, audio_aysnc &audio, json params) {
   //whisper_state has the pertinent offsets, but there also seem to be a large
   //number of scratch buffers that would prevent rewinding context in a manner similar to llama
   //I'll give this a another pass once everything else is implemented,
   //but for now, it's unsupported
   throw json{
      {"code", -32601},
      {"message", "Seeking is not yet supported."}
   };
}
json parse_job(const json &body, struct whisper_context * ctx, audio_async &audio, const whisper_params &params) {
   std::vector<struct commandset> commandset_list
   //Closely following https://www.jsonrpc.org/specification
   String version = body.at("jsonrpc");
   if (version != "2.0") {
      //unsupported version
   }
   String method = body.at("method");
   json params = body.value("params", json{});
   json id = body.at("id");
   json res;
   try {
      if (method == "unguided")                { res = unguided(ctx, audio, params); }
      else if (method == "guided")             { res = guided_transcription(ctx, audio, params); }
      else if (method == "seek")               { res = seek(ctx, audio, params); }
      else if (method == "registerCommandset") { res = register_commandset(ctx, params, commandset_list); }

      return json{
         {"jsonrpc", "2.0"},
         {"result", res},
         {"id", id}
      };
   } catch(json ex) {
      return json {
         {"jsonrpc", "2.0"},
         {"error", ex},
         {"id", id}
      };
   }
}

void process_loop(struct whisper_context * ctx, audio_async &audio, const whisper_params &params) {
   std::dequeue<json> jobqueue;
   while (true) {
      //For eventual cancellation support, shouldn't block if job exists
      if (cin.rdbuf().in_avail()>22 || job_queue.size() == 0) {
         int content_length;
         scanf("Content-Length: %d", &content_length);
         //scanf leaves the new lines intact
         std::cin.ignore(2);
         if (std::cin.peek() != 13) {
            //Content-Type. jsonrpc necessitates utf8.
            std::cin.ignore(200,10);
         }
         std::cin.ignore(2);
         //A message is being sent and blocking is acceptable
         char * content = new char[length];
         std::cin.read(content, content_length);
         json job = json::parse(content);
         //TODO: Some messages(cancellation) should skip queue here
         if (job.is_array()) {
            //response must also be batched. Will implement later
            //for (subjob : job.begin())
         } else {
            job_queue.push_back(job);
         }
      }
      assert(job_queue.size() > 0);
      job = jobqueue.front();
      json resp = parse_job(job, ctx, audio);
      if (resp != "unfinished") {
         jobqueue.pop_front();
         //send response
         string_t data = resp.dump(-1, ' ', false, json::error_handler_t::replace);
         fprintf(stdout, "Content-Length: %d\r\n\r\n%s", data.length, data);
      }
   }
}

int main(int argc, char ** argv) {
    whisper_params params;
    if (whisper_params_parse(argc, argv, params) == false) {
        return 1;
    }

    if (whisper_lang_id(params.language.c_str()) == -1) {
        fprintf(stderr, "error: unknown language '%s'\n", params.language.c_str());
        whisper_print_usage(argc, argv, params);
        exit(0);
    }

    // whisper init
    struct whisper_context * ctx = whisper_init_from_file(params.model.c_str());
    // init audio

    audio_async audio(30*1000);
    if (!audio.init(params.capture_id, WHISPER_SAMPLE_RATE)) {
        fprintf(stderr, "%s: audio.init() failed!\n", __func__);
        return 1;
    }

    audio.resume();

    // TODO: Investigate why this is required. An extra second of startup latency is not great
    // wait for 1 second to avoid any buffered noise
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    audio.clear();

    int  ret_val = 0;

    // TODO: consider some sort of indicator to designate loading has finished?
    // Potentially better for the client to just start with a non-blocking message (register commands)

    audio.pause();

    whisper_print_timings(ctx);
    whisper_free(ctx);

    return ret_val;
}
