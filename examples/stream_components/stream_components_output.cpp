#include "stream_components_output.h"

using namespace stream_components;

// -- utility methods --

//  500 -> 00:05.000
// 6000 -> 01:00.000
std::string to_timestamp(int64_t t, bool comma = false) {
    int64_t msec = t * 10;
    int64_t hr = msec / (1000 * 60 * 60);
    msec = msec - hr * (1000 * 60 * 60);
    int64_t min = msec / (1000 * 60);
    msec = msec - min * (1000 * 60);
    int64_t sec = msec / 1000;
    msec = msec - sec * 1000;

    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d%s%03d", (int) hr, (int) min, (int) sec, comma ? "," : ".", (int) msec);

    return std::string(buf);
}

char *escape_double_quotes_and_backslashes(const char *str) {
    if (str == NULL) {
        return NULL;
    }

    size_t escaped_length = strlen(str) + 1;

    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '"' || str[i] == '\\') {
            escaped_length++;
        }
    }

    char *escaped = (char *)calloc(escaped_length, 1); // pre-zeroed
    if (escaped == NULL) {
        return NULL;
    }

    size_t pos = 0;
    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '"' || str[i] == '\\') {
            escaped[pos++] = '\\';
        }
        escaped[pos++] = str[i];
    }

    // no need to set zero due to calloc() being used prior

    return escaped;
}


// -- WhisperEncoderJSON --

void WhisperEncoderJSON::start_arr(const char *name) {
    doindent();
    fout << "\"" << name << "\": [\n";
    indent++;
}

void WhisperEncoderJSON::end_arr(bool end) {
    indent--;
    doindent();
    fout << (end ? "]\n" : "},\n");
}

void WhisperEncoderJSON::start_obj(const char *name) {
    doindent();
    if (name) {
        fout << "\"" << name << "\": {\n";
    } else {
        fout << "{\n";
    }
    indent++;
}

void WhisperEncoderJSON::end_obj(bool end) {
    indent--;
    doindent();
    fout << (end ? "}\n" : "},\n");
}

void WhisperEncoderJSON::start_value(const char *name) {
    doindent();
    fout << "\"" << name << "\": ";
}

void WhisperEncoderJSON::value_s(const char *name, const char *val, bool end) {
    start_value(name);
    char * val_escaped = escape_double_quotes_and_backslashes(val);
    fout << "\"" << val_escaped << (end ? "\"\n" : "\",\n");
    free(val_escaped);
}

void WhisperEncoderJSON::end_value(bool end) {
    fout << (end ? "\n" : ",\n");
}

void WhisperEncoderJSON::value_i(const char *name, const int64_t val, bool end) {
    start_value(name);
    fout << val;
    end_value(end);
}

void WhisperEncoderJSON::value_b(const char *name, const bool val, bool end) {
    start_value(name);
    fout << (val ? "true" : "false");
    end_value(end);
}

void WhisperEncoderJSON::value_f(const char *name, const float val, bool end) {
    start_value(name);
    fout << val;
    end_value(end);
}

void WhisperEncoderJSON::doindent() {
    for (int i = 0; i < indent; i++) fout << "\t";
}


// -- WhisperOutput -- 

WhisperOutput::WhisperOutput(
    struct whisper_context * ctx, 
    const server_params & params):
        ctx(ctx),
        params(params)
{

}

void WhisperOutput::encode_server(
    WhisperEncoder & encoder, 
    const server_params & params,
    struct whisper_context * ctx)
{
    encoder.reset();

    encoder.start_obj(nullptr);
        encoder.value_s("systeminfo", whisper_print_system_info(), false);
        encoder.start_obj("model");
            encoder.value_s("type", whisper_model_type_readable(ctx), false);
            encoder.value_b("multilingual", whisper_is_multilingual(ctx), false);
            encoder.value_i("vocab", whisper_model_n_vocab(ctx), false);
            encoder.start_obj("audio");
                encoder.value_i("ctx", whisper_model_n_audio_ctx(ctx), false);
                encoder.value_i("state", whisper_model_n_audio_state(ctx), false);
                encoder.value_i("head", whisper_model_n_audio_head(ctx), false);
                encoder.value_i("layer", whisper_model_n_audio_layer(ctx), true);
            encoder.end_obj(false);
            encoder.start_obj("text");
                encoder.value_i("ctx", whisper_model_n_text_ctx(ctx), false);
                encoder.value_i("state", whisper_model_n_text_state(ctx), false);
                encoder.value_i("head", whisper_model_n_text_head(ctx), false);
                encoder.value_i("layer", whisper_model_n_text_layer(ctx), true);
            encoder.end_obj(false);
            encoder.value_i("mels", whisper_model_n_mels(ctx), false);
            encoder.value_i("ftype", whisper_model_ftype(ctx), true);
        encoder.end_obj(false);
        encoder.start_obj("params");
            encoder.value_s("model", params.model.c_str(), false);
            encoder.value_s("language", params.language.c_str(), false);
            encoder.value_b("translate", params.translate, true);
        encoder.end_obj(false);
    encoder.end_obj(true);
}

void WhisperOutput::encode_transcription(WhisperEncoder & encoder) const
{
    encoder.reset();

    encoder.start_obj(nullptr);
        encoder.start_arr("transcription");

            const int n_segments = whisper_full_n_segments(ctx);
            for (int i = 0; i < n_segments; ++i) {

                const char * text = whisper_full_get_segment_text(ctx, i);

                const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
                const int64_t t1 = whisper_full_get_segment_t1(ctx, i);

                encoder.start_obj(nullptr);

                    // These don't seem to be useful...
                    encoder.start_obj("timestamps");
                        encoder.value_s("from", to_timestamp(t0, true).c_str(), false);
                        encoder.value_s("to", to_timestamp(t1, true).c_str(), true);
                    encoder.end_obj(false);
                    encoder.start_obj("offsets");
                        encoder.value_i("from", t0 * 10, false);
                        encoder.value_i("to", t1 * 10, true);
                    encoder.end_obj(false);

                    for (int j = 0; j < whisper_full_n_tokens(ctx, i); ++j) {

                        whisper_token_data token = whisper_full_get_token_data(ctx, i, j);

                        const char * text = whisper_full_get_token_text(ctx, i, j);
                        const float  p    = whisper_full_get_token_p   (ctx, i, j);
                        encoder.start_obj("token");
                            encoder.value_s("text", text, false);
                            encoder.value_i("id", token.id, false);
                            encoder.value_f("confidence", p, false);
                            encoder.value_i("t0", token.t0, false);
                            encoder.value_i("t1", token.t1, true);
                        encoder.end_obj(false);
                    }
                    encoder.value_s("text", text, !params.diarize && !params.tinydiarize);

                    // if (params.diarize && pcmf32s.size() == 2) {
                    //     encoder.value_s("speaker", estimate_diarization_speaker(pcmf32s, t0, t1, true).c_str(), true);
                    // }

                    if (params.tinydiarize) {
                        encoder.value_b("speaker_turn_next", whisper_full_get_segment_speaker_turn_next(ctx, i), true);
                    }
                encoder.end_obj(i == (n_segments - 1));
            }

        encoder.end_arr(true);
    encoder.end_obj(true);
}

void WhisperOutput::server_to_json(std::ostream & os, const server_params & params, struct whisper_context * ctx)
{
    WhisperEncoderJSON encoder(os);
    WhisperOutput::encode_server(encoder, params, ctx);
}

void WhisperOutput::transcription_to_json(std::ostream & os) const
{
    WhisperEncoderJSON encoder(os);
    encode_transcription(encoder);
}
