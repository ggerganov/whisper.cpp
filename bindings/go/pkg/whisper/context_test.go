package whisper_test

import (
	"os"
	"testing"

	// Packages
	whisper "github.com/ggerganov/whisper.cpp/bindings/go/pkg/whisper"
	assert "github.com/stretchr/testify/assert"
)

const (
	ModelPath  = "../../models/ggml-tiny.bin"
	SamplePath = "../../samples/jfk.wav"
)

func Test_Whisper_000(t *testing.T) {
	assert := assert.New(t)
	if _, err := os.Stat(ModelPath); os.IsNotExist(err) {
		t.Skip("Skipping test, model not found:", ModelPath)
	}
	if _, err := os.Stat(SamplePath); os.IsNotExist(err) {
		t.Skip("Skipping test, sample not found:", SamplePath)
	}

	// Load model
	model, err := whisper.New(ModelPath)
	assert.NoError(err)
	assert.NotNil(model)
	assert.NoError(model.Close())

	t.Log("languages=", model.Languages())
}

func Test_Whisper_001(t *testing.T) {
	assert := assert.New(t)
	if _, err := os.Stat(ModelPath); os.IsNotExist(err) {
		t.Skip("Skipping test, model not found:", ModelPath)
	}
	if _, err := os.Stat(SamplePath); os.IsNotExist(err) {
		t.Skip("Skipping test, sample not found:", SamplePath)
	}

	// Load model
	model, err := whisper.New(ModelPath)
	assert.NoError(err)
	assert.NotNil(model)
	defer model.Close()

	// Get context for decoding
	ctx, err := model.NewContext()
	assert.NoError(err)
	assert.NotNil(ctx)

}
