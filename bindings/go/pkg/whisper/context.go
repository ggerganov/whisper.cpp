package whisper

import (
	// Bindings
	"io"
	"strings"
	"time"

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
// STRINGIFY

func (context *context) String() string {
	str := "<whisper.context"
	return str + ">"
}

///////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS

// Process new sample data and return any errors
func (context *context) Process(data []float32) error {
	// Process data
	if context.model.ctx == nil {
		return ErrInternalAppError
	} else if ret := context.model.ctx.Whisper_full(context.params, data, nil, nil); ret != 0 {
		return ErrProcessingFailed
	}

	// Return success
	return nil
}

// Return the next segment of tokens
func (context *context) NextSegment() (Segment, error) {
	result := Segment{}
	if context.model.ctx == nil {
		return result, ErrInternalAppError
	}
	if context.n >= context.model.ctx.Whisper_full_n_segments() {
		return result, io.EOF
	}

	// Populate result
	result.Text = strings.TrimSpace(context.model.ctx.Whisper_full_get_segment_text(context.n))
	result.Start = time.Duration(context.model.ctx.Whisper_full_get_segment_t0(context.n)) * time.Millisecond
	result.End = time.Duration(context.model.ctx.Whisper_full_get_segment_t1(context.n)) * time.Millisecond

	// Increment the cursor
	context.n++

	// Return success
	return result, nil
}
