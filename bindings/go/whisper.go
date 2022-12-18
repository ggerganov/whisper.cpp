package whisper

import "unsafe"

///////////////////////////////////////////////////////////////////////////////
// CGO

/*
#cgo CFLAGS: -I${SRCDIR}/../..
#cgo LDFLAGS: -L${SRCDIR}/build -lwhisper -lm -lstdc++
#cgo darwin LDFLAGS: -framework Accelerate
#include <whisper.h>
#include <stdlib.h>

extern void callNewSegment(void* user_data, int new);
extern bool callEncoderBegin(void* user_data);

// Text segment callback
// Called on every newly generated text segment
// Use the whisper_full_...() functions to obtain the text segments
static void whisper_new_segment_cb(struct whisper_context* ctx, int n_new, void* user_data) {
    if(user_data != NULL && ctx != NULL) {
        callNewSegment(user_data, n_new);
    }
}

// Encoder begin callback
// If not NULL, called before the encoder starts
// If it returns false, the computation is aborted
static bool whisper_encoder_begin_cb(struct whisper_context* ctx, void* user_data) {
    if(user_data != NULL && ctx != NULL) {
        return callEncoderBegin(user_data);
    }
    return false;
}

// Get default parameters and set callbacks
static struct whisper_full_params whisper_full_default_params_cb(struct whisper_context* ctx, enum whisper_sampling_strategy strategy) {
	struct whisper_full_params params = whisper_full_default_params(strategy);
	params.new_segment_callback = whisper_new_segment_cb;
	params.new_segment_callback_user_data = (void*)(ctx);
	params.encoder_begin_callback = whisper_encoder_begin_cb;
	params.encoder_begin_callback_user_data = (void*)(ctx);
	return params;
}
*/
import "C"

///////////////////////////////////////////////////////////////////////////////
// TYPES

type (
	Context          C.struct_whisper_context
	Token            C.whisper_token
	TokenData        C.struct_whisper_token_data
	SamplingStrategy C.enum_whisper_sampling_strategy
	Params           C.struct_whisper_full_params
)

///////////////////////////////////////////////////////////////////////////////
// GLOBALS

const (
	SAMPLING_GREEDY      SamplingStrategy = C.WHISPER_SAMPLING_GREEDY
	SAMPLING_BEAM_SEARCH SamplingStrategy = C.WHISPER_SAMPLING_BEAM_SEARCH
)

const (
	SampleRate = C.WHISPER_SAMPLE_RATE                 // Expected sample rate, samples per second
	SampleBits = uint16(unsafe.Sizeof(C.float(0))) * 8 // Sample size in bits
	NumFFT     = C.WHISPER_N_FFT
	NumMEL     = C.WHISPER_N_MEL
	HopLength  = C.WHISPER_HOP_LENGTH
	ChunkSize  = C.WHISPER_CHUNK_SIZE
)

///////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS

// Allocates all memory needed for the model and loads the model from the given file.
// Returns NULL on failure.
func Whisper_init(path string) *Context {
	cPath := C.CString(path)
	defer C.free(unsafe.Pointer(cPath))
	if ctx := C.whisper_init(cPath); ctx != nil {
		return (*Context)(ctx)
	} else {
		return nil
	}
}

// Frees all memory allocated by the model.
func (ctx *Context) Whisper_free() {
	C.whisper_free((*C.struct_whisper_context)(ctx))
}

// Convert RAW PCM audio to log mel spectrogram.
// The resulting spectrogram is stored inside the provided whisper context.
// Returns 0 on success
func (ctx *Context) Whisper_pcm_to_mel(data []float32, threads int) int {
	return int(C.whisper_pcm_to_mel((*C.struct_whisper_context)(ctx), (*C.float)(&data[0]), C.int(len(data)), C.int(threads)))
}

// This can be used to set a custom log mel spectrogram inside the provided whisper context.
// Use this instead of whisper_pcm_to_mel() if you want to provide your own log mel spectrogram.
// n_mel must be 80
// Returns 0 on success
func (ctx *Context) Whisper_set_mel(data []float32, n_mel int) int {
	return int(C.whisper_set_mel((*C.struct_whisper_context)(ctx), (*C.float)(&data[0]), C.int(len(data)), C.int(n_mel)))
}

// Run the Whisper encoder on the log mel spectrogram stored inside the provided whisper context.
// Make sure to call whisper_pcm_to_mel() or whisper_set_mel() first.
// offset can be used to specify the offset of the first frame in the spectrogram.
// Returns 0 on success
func (ctx *Context) Whisper_encode(offset, threads int) int {
	return int(C.whisper_encode((*C.struct_whisper_context)(ctx), C.int(offset), C.int(threads)))
}

// Run the Whisper decoder to obtain the logits and probabilities for the next token.
// Make sure to call whisper_encode() first.
// tokens + n_tokens is the provided context for the decoder.
// n_past is the number of tokens to use from previous decoder calls.
// Returns 0 on success
func (ctx *Context) Whisper_decode(tokens []Token, past, threads int) int {
	return int(C.whisper_decode((*C.struct_whisper_context)(ctx), (*C.whisper_token)(&tokens[0]), C.int(len(tokens)), C.int(past), C.int(threads)))
}

// whisper_sample_best() returns the token with the highest probability
func (ctx *Context) Whisper_sample_best() TokenData {
	return TokenData(C.whisper_sample_best((*C.struct_whisper_context)(ctx)))
}

// whisper_sample_timestamp() returns the most probable timestamp token
func (ctx *Context) Whisper_sample_timestamp(is_initial bool) TokenData {
	return TokenData(C.whisper_sample_timestamp((*C.struct_whisper_context)(ctx), C.bool(is_initial)))
}

// Return the id of the specified language, returns -1 if not found
func (ctx *Context) Whisper_lang_id(lang string) int {
	return int(C.whisper_lang_id(C.CString(lang)))
}

func (ctx *Context) Whisper_n_len() int {
	return int(C.whisper_n_len((*C.struct_whisper_context)(ctx)))
}

func (ctx *Context) Whisper_n_vocab() int {
	return int(C.whisper_n_vocab((*C.struct_whisper_context)(ctx)))
}

func (ctx *Context) Whisper_n_text_ctx() int {
	return int(C.whisper_n_text_ctx((*C.struct_whisper_context)(ctx)))
}

func (ctx *Context) Whisper_is_multilingual() int {
	return int(C.whisper_is_multilingual((*C.struct_whisper_context)(ctx)))
}

// The probabilities for the next token
//func (ctx *Whisper_context) Whisper_get_probs() []float32 {
//	return (*[1 << 30]float32)(unsafe.Pointer(C.whisper_get_probs((*C.struct_whisper_context)(ctx))))[:ctx.Whisper_n_vocab()]
//}

// Token Id -> String. Uses the vocabulary in the provided context
func (ctx *Context) Whisper_token_to_str(token Token) string {
	return C.GoString(C.whisper_token_to_str((*C.struct_whisper_context)(ctx), C.whisper_token(token)))
}

// Special tokens
func (ctx *Context) Whisper_token_eot() Token {
	return Token(C.whisper_token_eot((*C.struct_whisper_context)(ctx)))
}

// Special tokens
func (ctx *Context) Whisper_token_sot() Token {
	return Token(C.whisper_token_sot((*C.struct_whisper_context)(ctx)))
}

// Special tokens
func (ctx *Context) Whisper_token_prev() Token {
	return Token(C.whisper_token_prev((*C.struct_whisper_context)(ctx)))
}

// Special tokens
func (ctx *Context) Whisper_token_solm() Token {
	return Token(C.whisper_token_solm((*C.struct_whisper_context)(ctx)))
}

// Special tokens
func (ctx *Context) Whisper_token_not() Token {
	return Token(C.whisper_token_not((*C.struct_whisper_context)(ctx)))
}

// Special tokens
func (ctx *Context) Whisper_token_beg() Token {
	return Token(C.whisper_token_beg((*C.struct_whisper_context)(ctx)))
}

// Task tokens
func Whisper_token_translate() Token {
	return Token(C.whisper_token_translate())
}

// Task tokens
func Whisper_token_transcribe() Token {
	return Token(C.whisper_token_transcribe())
}

// Performance information
func (ctx *Context) Whisper_print_timings() {
	C.whisper_print_timings((*C.struct_whisper_context)(ctx))
}

// Performance information
func (ctx *Context) Whisper_reset_timings() {
	C.whisper_reset_timings((*C.struct_whisper_context)(ctx))
}

// Print system information
func Whisper_print_system_info() string {
	return C.GoString(C.whisper_print_system_info())
}

// Return default parameters for a strategy
func (ctx *Context) Whisper_full_default_params(strategy SamplingStrategy) Params {
	// Get default parameters
	return Params(C.whisper_full_default_params_cb((*C.struct_whisper_context)(ctx), C.enum_whisper_sampling_strategy(strategy)))
}

// Run the entire model: PCM -> log mel spectrogram -> encoder -> decoder -> text
// Uses the specified decoding strategy to obtain the text.
func (ctx *Context) Whisper_full(params Params, samples []float32, encoderBeginCallback func() bool, newSegmentCallback func(int)) int {
	registerEncoderBeginCallback(ctx, encoderBeginCallback)
	registerNewSegmentCallback(ctx, newSegmentCallback)
	result := int(C.whisper_full((*C.struct_whisper_context)(ctx), (C.struct_whisper_full_params)(params), (*C.float)(&samples[0]), C.int(len(samples))))
	registerEncoderBeginCallback(ctx, nil)
	registerNewSegmentCallback(ctx, nil)
	return result
}

// Split the input audio in chunks and process each chunk separately using whisper_full()
// It seems this approach can offer some speedup in some cases.
// However, the transcription accuracy can be worse at the beginning and end of each chunk.
func (ctx *Context) Whisper_full_parallel(params Params, samples []float32, processors int, encoderBeginCallback func() bool, newSegmentCallback func(int)) int {
	registerEncoderBeginCallback(ctx, encoderBeginCallback)
	registerNewSegmentCallback(ctx, newSegmentCallback)
	result := int(C.whisper_full_parallel((*C.struct_whisper_context)(ctx), (C.struct_whisper_full_params)(params), (*C.float)(&samples[0]), C.int(len(samples)), C.int(processors)))
	registerEncoderBeginCallback(ctx, nil)
	registerNewSegmentCallback(ctx, nil)
	return result
}

// Number of generated text segments.
// A segment can be a few words, a sentence, or even a paragraph.
func (ctx *Context) Whisper_full_n_segments() int {
	return int(C.whisper_full_n_segments((*C.struct_whisper_context)(ctx)))
}

// Get the start and end time of the specified segment.
func (ctx *Context) Whisper_full_get_segment_t0(segment int) int64 {
	return int64(C.whisper_full_get_segment_t0((*C.struct_whisper_context)(ctx), C.int(segment)))
}

// Get the start and end time of the specified segment.
func (ctx *Context) Whisper_full_get_segment_t1(segment int) int64 {
	return int64(C.whisper_full_get_segment_t1((*C.struct_whisper_context)(ctx), C.int(segment)))
}

// Get the text of the specified segment.
func (ctx *Context) Whisper_full_get_segment_text(segment int) string {
	return C.GoString(C.whisper_full_get_segment_text((*C.struct_whisper_context)(ctx), C.int(segment)))
}

// Get number of tokens in the specified segment.
func (ctx *Context) Whisper_full_n_tokens(segment int) int {
	return int(C.whisper_full_n_tokens((*C.struct_whisper_context)(ctx), C.int(segment)))
}

// Get the token text of the specified token index in the specified segment.
func (ctx *Context) Whisper_full_get_token_text(segment int, token int) string {
	return C.GoString(C.whisper_full_get_token_text((*C.struct_whisper_context)(ctx), C.int(segment), C.int(token)))
}

// Get the token of the specified token index in the specified segment.
func (ctx *Context) Whisper_full_get_token_id(segment int, token int) Token {
	return Token(C.whisper_full_get_token_id((*C.struct_whisper_context)(ctx), C.int(segment), C.int(token)))
}

// Get token data for the specified token in the specified segment.
// This contains probabilities, timestamps, etc.
func (ctx *Context) whisper_full_get_token_data(segment int, token int) TokenData {
	return TokenData(C.whisper_full_get_token_data((*C.struct_whisper_context)(ctx), C.int(segment), C.int(token)))
}

// Get the probability of the specified token in the specified segment.
func (ctx *Context) Whisper_full_get_token_p(segment int, token int) float32 {
	return float32(C.whisper_full_get_token_p((*C.struct_whisper_context)(ctx), C.int(segment), C.int(token)))
}

///////////////////////////////////////////////////////////////////////////////
// CALLBACKS

var (
	cbNewSegment   = make(map[unsafe.Pointer]func(int))
	cbEncoderBegin = make(map[unsafe.Pointer]func() bool)
)

func registerNewSegmentCallback(ctx *Context, fn func(int)) {
	if fn == nil {
		delete(cbNewSegment, unsafe.Pointer(ctx))
	} else {
		cbNewSegment[unsafe.Pointer(ctx)] = fn
	}
}

func registerEncoderBeginCallback(ctx *Context, fn func() bool) {
	if fn == nil {
		delete(cbEncoderBegin, unsafe.Pointer(ctx))
	} else {
		cbEncoderBegin[unsafe.Pointer(ctx)] = fn
	}
}

//export callNewSegment
func callNewSegment(user_data unsafe.Pointer, new C.int) {
	if fn, ok := cbNewSegment[user_data]; ok {
		fn(int(new))
	}
}

//export callEncoderBegin
func callEncoderBegin(user_data unsafe.Pointer) C.bool {
	if fn, ok := cbEncoderBegin[user_data]; ok {
		if fn() {
			return C.bool(true)
		} else {
			return C.bool(false)
		}
	}
	return true
}
