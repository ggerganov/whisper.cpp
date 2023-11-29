#include "WChess.h"
#include "Chessboard.h"
#include "grammar-parser.h"
#include "common.h"
#include <thread>

WChess::WChess(whisper_context * ctx,
        const whisper_full_params & wparams,
        callbacks cb,
        settings s)
        : m_ctx(ctx)
        , m_wparams(wparams)
        , m_cb(cb)
        , m_settings(s)
        , m_board(new Chessboard())
{}

WChess::~WChess() = default;

void WChess::set_status(const std::string& msg) const {
    if (m_cb.set_status) (*m_cb.set_status)(msg);
}

void WChess::set_moves(const std::string& moves) const {
    if (m_cb.set_moves) (*m_cb.set_moves)(moves);
}

bool WChess::check_running() const {
    if (m_cb.check_running) return (*m_cb.check_running)();
    return false;
}

void WChess::clear_audio() const {
    if (m_cb.clear_audio) (*m_cb.clear_audio)();
}

void WChess::get_audio(int ms, std::vector<float>& pcmf32) const {
    if (m_cb.get_audio) (*m_cb.get_audio)(ms, pcmf32);
}

std::string WChess::stringify_board() const {
    return m_board->stringifyBoard();
}

void WChess::run() {
    set_status("loading data ...");

    bool have_prompt  = true;
    bool ask_prompt   = false;

    float logprob_min0 = 0.0f;
    float logprob_min  = 0.0f;

    float logprob_sum0 = 0.0f;
    float logprob_sum  = 0.0f;

    int n_tokens0 = 0;
    int n_tokens  = 0;

    std::vector<float> pcmf32_cur;
    std::vector<float> pcmf32_prompt;

    std::string prompt = "";
    float prompt_prop = 0.0f;

    while (check_running()) {
        // delay
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        int64_t t_ms = 0;

        {
            get_audio(m_settings.vad_ms, pcmf32_cur);

            if (!pcmf32_cur.empty()) {
                fprintf(stdout, "%s: Processing ...\n", __func__);
                set_status("Processing ...");

                {
                    if (!pcmf32_prompt.empty()) pcmf32_cur.insert(pcmf32_cur.begin(), pcmf32_prompt.begin(), pcmf32_prompt.end());

                    if (WHISPER_SAMPLE_RATE > pcmf32_cur.size()) pcmf32_cur.insert(pcmf32_cur.begin(), WHISPER_SAMPLE_RATE - pcmf32_cur.size(), 0.0f);

                    std::string rules = m_board->getRules();
                    fprintf(stdout, "%s: grammar rules:\n'%s'\n", __func__, rules.c_str());

                    auto grammar_parsed = grammar_parser::parse(rules.c_str());
                    auto grammar_rules = grammar_parsed.c_rules();

                    m_wparams.grammar_rules   = grammar_rules.data();
                    m_wparams.n_grammar_rules = grammar_rules.size();

                    m_wparams.i_start_rule    = grammar_parsed.symbol_ids.at("move");
                    auto txt = ::trim(transcribe(pcmf32_cur, logprob_min, logprob_sum, n_tokens, t_ms));

                    const float p = 100.0f * std::exp(logprob_min);

                    fprintf(stdout, "%s: heard '%s'\n", __func__, txt.c_str());

                    // find the prompt in the text
                    float best_sim = 0.0f;
                    size_t best_len = 0;
                    if (!prompt.empty()) {
                        auto pos = txt.find_first_of('.');

                        const auto header = txt.substr(0, pos);

                        const float sim = similarity(prompt, header);

                        //fprintf(stderr, "%s: prompt = '%s', sim = %f\n", __func__, prompt.c_str(), sim);

                        if (sim > best_sim) {
                            best_sim = sim;
                            best_len = pos + 1;
                        }
                    }

                    fprintf(stdout, "%s:   DEBUG: txt = '%s', prob = %.2f%%\n", __func__, txt.c_str(), p);
                    std::string command = ::trim(txt.substr(best_len));

                    fprintf(stdout, "%s: Command '%s%s%s', (t = %d ms)\n", __func__, "\033[1m", command.c_str(), "\033[0m", (int) t_ms);
                    fprintf(stdout, "\n");

                    {
                        char txt[1024];
                        snprintf(txt, sizeof(txt), "Command '%s', (t = %d ms)", command.c_str(), (int) t_ms);
                        set_status(txt);
                    }
                    if (!command.empty()) {
                        auto move = m_board->process(command);
                        if (!move.empty()) {
                            set_moves(std::move(move));
                        }
                    }
                }

                clear_audio();
            }
        }
    }
}

std::string WChess::transcribe(
                const std::vector<float> & pcmf32,
                float & logprob_min,
                float & logprob_sum,
                int & n_tokens,
                int64_t & t_ms) {
    const auto t_start = std::chrono::high_resolution_clock::now();

    logprob_min = 0.0f;
    logprob_sum = 0.0f;
    n_tokens    = 0;
    t_ms = 0;

    if (whisper_full(m_ctx, m_wparams, pcmf32.data(), pcmf32.size()) != 0) {
        return {};
    }

    std::string result;

    const int n_segments = whisper_full_n_segments(m_ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char * text = whisper_full_get_segment_text(m_ctx, i);

        result += text;

        const int n = whisper_full_n_tokens(m_ctx, i);
        for (int j = 0; j < n; ++j) {
            const auto token = whisper_full_get_token_data(m_ctx, i, j);

            if(token.plog > 0.0f) return {};
            logprob_min = std::min(logprob_min, token.plog);
            logprob_sum += token.plog;
            ++n_tokens;
        }
    }

    const auto t_end = std::chrono::high_resolution_clock::now();
    t_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();

    return result;
}
