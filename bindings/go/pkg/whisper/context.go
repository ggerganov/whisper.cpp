package whisper

import (
	"io"
	"strings"
	"time"

	// Bindings
	whisper "github.com/ggerganov/whisper.cpp/bindings/go"
)

///////////////////////////////////////////////////////////////////////////////
// TYPES

type context struct {
	n      int
	model  *model
	params whisper.Params
}

// Make sure context adheres to the interface
var _ Context = (*context)(nil)

///////////////////////////////////////////////////////////////////////////////
// LIFECYCLE

func NewContext(model *model, params whisper.Params) (Context, error) {
	context := new(context)
	context.model = model
	context.params = params

	// Return success
	return context, nil
}

///////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS

// Set the language to use for speech recognition.
func (context *context) SetLanguage(lang string) error {
	if context.model.ctx == nil {
		return ErrInternalAppError
	}
	if id := context.model.ctx.Whisper_lang_id(lang); id < 0 {
		return ErrUnsupportedLanguage
	} else if err := context.params.SetLanguage(id); err != nil {
		return err
	}
	// Return success
	return nil
}

// Get language
func (context *context) Language() string {
	return whisper.Whisper_lang_str(context.params.Language())
}

// Set speedup flag
func (context *context) SetSpeedup(v bool) {
	context.params.SetSpeedup(v)
}

// Process new sample data and return any errors
func (context *context) Process(data []float32, cb SegmentCallback) error {
	if context.model.ctx == nil {
		return ErrInternalAppError
	}
	// If the callback is defined then we force on single_segment mode
	if cb != nil {
		context.params.SetSingleSegment(true)
	}

	// We don't do parallel processing at the moment
	processors := 0
	if processors > 1 {
		if err := context.model.ctx.Whisper_full_parallel(context.params, data, processors, nil, func(new int) {
			if cb != nil {
				num_segments := context.model.ctx.Whisper_full_n_segments()
				s0 := num_segments - new
				for i := s0; i < num_segments; i++ {
					cb(toSegment(context.model.ctx, i))
				}
			}
		}); err != nil {
			return err
		}
	} else if err := context.model.ctx.Whisper_full(context.params, data, nil, func(new int) {
		if cb != nil {
			num_segments := context.model.ctx.Whisper_full_n_segments()
			s0 := num_segments - new
			for i := s0; i < num_segments; i++ {
				cb(toSegment(context.model.ctx, i))
			}
		}
	}); err != nil {
		return err
	}

	// Return success
	return nil
}

// Return the next segment of tokens
func (context *context) NextSegment() (Segment, error) {
	if context.model.ctx == nil {
		return Segment{}, ErrInternalAppError
	}
	if context.n >= context.model.ctx.Whisper_full_n_segments() {
		return Segment{}, io.EOF
	}

	// Populate result
	result := toSegment(context.model.ctx, context.n)

	// Increment the cursor
	context.n++

	// Return success
	return result, nil
}

///////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS

func toSegment(ctx *whisper.Context, n int) Segment {
	return Segment{
		Num:    n,
		Text:   strings.TrimSpace(ctx.Whisper_full_get_segment_text(n)),
		Start:  time.Duration(ctx.Whisper_full_get_segment_t0(n)) * time.Millisecond * 10,
		End:    time.Duration(ctx.Whisper_full_get_segment_t1(n)) * time.Millisecond * 10,
		Tokens: toTokens(ctx, n),
	}
}

func toTokens(ctx *whisper.Context, n int) []Token {
	result := make([]Token, ctx.Whisper_full_n_tokens(n))
	for i := 0; i < len(result); i++ {
		result[i] = Token{
			Id:   int(ctx.Whisper_full_get_token_id(n, i)),
			Text: strings.TrimSpace(ctx.Whisper_full_get_token_text(n, i)),
			P:    ctx.Whisper_full_get_token_p(n, i),
		}
	}
	return result
}
