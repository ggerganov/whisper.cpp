package whisper

import (
	"fmt"
	"os"

	// Bindings
	whisper "github.com/ggerganov/whisper.cpp/bindings/go"
)

///////////////////////////////////////////////////////////////////////////////
// TYPES

type model struct {
	path string
	ctx  *whisper.Context
}

// Make sure model adheres to the interface
var _ Model = (*model)(nil)

///////////////////////////////////////////////////////////////////////////////
// LIFECYCLE

func New(path string) (*model, error) {
	model := new(model)
	if _, err := os.Stat(path); err != nil {
		return nil, err
	} else if ctx := whisper.Whisper_init(path); ctx == nil {
		return nil, ErrUnableToLoadModel
	} else {
		model.ctx = ctx
		model.path = path
	}

	// Return success
	return model, nil
}

func (model *model) Close() error {
	if model.ctx != nil {
		model.ctx.Whisper_free()
	}

	// Release resources
	model.ctx = nil

	// Return success
	return nil
}

///////////////////////////////////////////////////////////////////////////////
// STRINGIFY

func (model *model) String() string {
	str := "<whisper.model"
	if model.ctx != nil {
		str += fmt.Sprintf(" model=%q", model.path)
	}
	return str + ">"
}

///////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS

func (model *model) NewContext() (Context, error) {
	if model.ctx == nil {
		return nil, ErrInternalAppError
	}

	// Create new context
	params := model.ctx.Whisper_full_default_params(whisper.SAMPLING_GREEDY)
	params.SetTranslate(false)
	params.SetPrintSpecial(false)
	params.SetPrintProgress(false)
	params.SetPrintRealtime(false)
	params.SetPrintTimestamps(false)
	params.SetSpeedup(false)

	// Return new context
	return NewContext(model, params)
}
