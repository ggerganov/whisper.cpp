package whisper

import (
	"io"
	"time"
)

///////////////////////////////////////////////////////////////////////////////
// TYPES

// SegmentCallback is the callback function for processing segments in real
// time. It is called during the Process function
type SegmentCallback func(Segment)

// Model is the interface to a whisper model. Create a new model with the
// function whisper.New(string)
type Model interface {
	io.Closer

	// Return a new speech-to-text context.
	NewContext() (Context, error)

	// Return all languages supported.
	Languages() []string
}

// Context is the speach recognition context.
type Context interface {
	SetLanguage(string) error // Set the language to use for speech recognition.
	Language() string         // Get language
	SetSpeedup(bool)          // Set speedup flag

	// Process mono audio data and return any errors.
	// If defined, newly generated segments are passed to the
	// callback function during processing.
	Process([]float32, SegmentCallback) error

	// After process is called, return segments until the end of the stream
	// is reached, when io.EOF is returned.
	NextSegment() (Segment, error)
}

// Segment is the text result of a speech recognition.
type Segment struct {
	// Segment Number
	Num int

	// Time beginning and end timestamps for the segment.
	Start, End time.Duration

	// The text of the segment.
	Text string

	// The tokens of the segment.
	Tokens []Token
}

// Token is a text or special token
type Token struct {
	Id   int
	Text string
	P    float32
}
