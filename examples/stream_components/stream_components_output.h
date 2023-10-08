#ifndef WHISPER_STREAM_COMPONENTS_OUTPUT_H
#define WHISPER_STREAM_COMPONENTS_OUTPUT_H

#include <memory>
#include <iostream>
#include "whisper.h"
#include "stream_components_params.h"

namespace stream_components {

class WhisperEncoder {
public:
    virtual void reset() = 0;
    virtual void start_arr(const char *name) = 0;
    virtual void end_arr(bool end) = 0;
    virtual void start_obj(const char *name) = 0;
    virtual void end_obj(bool end) = 0;
    virtual void start_value(const char *name) = 0;
    virtual void value_s(const char *name, const char *val, bool end) = 0;
    virtual void end_value(bool end) = 0;
    virtual void value_i(const char *name, const int64_t val, bool end) = 0;
    virtual void value_b(const char *name, const bool val, bool end) = 0;
    virtual void value_f(const char *name, const float val, bool end) = 0;
};

class WhisperEncoderJSON : public WhisperEncoder {
public:
    WhisperEncoderJSON(std::ostream & os): fout(os) {}

    void reset() override { indent = 0; }

    void start_arr(const char *name) override;
    void end_arr(bool end) override;
    void start_obj(const char *name) override;
    void end_obj(bool end) override;
    void start_value(const char *name) override;
    void value_s(const char *name, const char *val, bool end) override;
    void end_value(bool end) override;
    void value_i(const char *name, const int64_t val, bool end) override;
    void value_b(const char *name, const bool val, bool end) override;
    void value_f(const char *name, const float val, bool end) override;

protected:
    std::ostream & fout;
    int indent = 0;

    void doindent();
};

class WhisperOutput {
public:
    WhisperOutput(
        struct whisper_context * ctx, 
        const server_params & params);

    static void encode_server(WhisperEncoder & encoder, const server_params & params, struct whisper_context * ctx);
    void encode_transcription(WhisperEncoder & encoder) const;

    static void server_to_json(std::ostream & os, const server_params & params, struct whisper_context * ctx);
    void transcription_to_json(std::ostream & os) const;

protected:
    struct whisper_context * ctx;
    const server_params params;
};

using WhisperOutputPtr = std::shared_ptr<WhisperOutput>;

} // namespace stream_components

#endif // WHISPER_STREAM_COMPONENTS_OUTPUT_H
