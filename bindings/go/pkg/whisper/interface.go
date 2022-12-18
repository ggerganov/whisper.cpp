package whisper

import (
	"io"
	"time"
)

///////////////////////////////////////////////////////////////////////////////
// TYPES

// Model is the interface to a whisper model. Create a new model with the
// function whisper.New(string)
type Model interface {
	io.Closer

	NewContext() (Context, error)
}

// Context is the speach recognition context.
type Context interface {
	// Process mono audio data and return any errors.
	Process([]float32) error

	// Return segments until the end of the stream is reached, when io.EOF is
	// returned.
	NextSegment() (Segment, error)
}

type Segment struct {
	// Time beginning and end timestamps for the segment.
	Start, End time.Duration

	// The text of the segment.
	Text string
}
