package whisper_test

import (
	"os"
	"testing"
	"time"

	// Packages
	whisper "github.com/ggerganov/whisper.cpp/bindings/go"
	"github.com/go-audio/wav"
	assert "github.com/stretchr/testify/assert"
)

const (
	ModelPath  = "models/ggml-small.en.bin"
	SamplePath = "samples/jfk.wav"
)

func Test_Whisper_000(t *testing.T) {
	assert := assert.New(t)
	if _, err := os.Stat(ModelPath); os.IsNotExist(err) {
		t.Skip("Skipping test, model not found:", ModelPath)
	}
	ctx := whisper.Whisper_init(ModelPath)
	assert.NotNil(ctx)
	ctx.Whisper_free()
}

func Test_Whisper_001(t *testing.T) {
	assert := assert.New(t)
	if _, err := os.Stat(ModelPath); os.IsNotExist(err) {
		t.Skip("Skipping test, model not found:", ModelPath)
	}
	if _, err := os.Stat(SamplePath); os.IsNotExist(err) {
		t.Skip("Skipping test, sample not found:", SamplePath)
	}

	// Open samples
	fh, err := os.Open(SamplePath)
	assert.NoError(err)
	defer fh.Close()

	// Read samples
	d := wav.NewDecoder(fh)
	buf, err := d.FullPCMBuffer()
	assert.NoError(err)
	buf2 := buf.AsFloat32Buffer()

	// Run whisper
	ctx := whisper.Whisper_init(ModelPath)
	assert.NotNil(ctx)
	defer ctx.Whisper_free()
	ret := ctx.Whisper_full(ctx.Whisper_full_default_params(whisper.SAMPLING_GREEDY), buf2.Data, nil, nil)
	assert.Equal(0, ret)

	// Print out tokens
	num_segments := ctx.Whisper_full_n_segments()
	assert.GreaterOrEqual(num_segments, 1)
	for i := 0; i < num_segments; i++ {
		str := ctx.Whisper_full_get_segment_text(i)
		assert.NotEmpty(str)
		t0 := time.Duration(ctx.Whisper_full_get_segment_t0(i)) * time.Millisecond
		t1 := time.Duration(ctx.Whisper_full_get_segment_t1(i)) * time.Millisecond
		t.Logf("[%6s->%-6s] %q", t0, t1, str)
	}
}
