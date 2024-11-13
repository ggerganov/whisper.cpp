require_relative "helper"

class TestModel < TestBase
  def test_model
    whisper = Whisper::Context.new(MODEL)
    assert_instance_of Whisper::Model, whisper.model
  end

  def test_attributes
    whisper = Whisper::Context.new(MODEL)
    model = whisper.model

    assert_equal 51864, model.n_vocab
    assert_equal 1500, model.n_audio_ctx
    assert_equal 512, model.n_audio_state
    assert_equal 8, model.n_audio_head
    assert_equal 6, model.n_audio_layer
    assert_equal 448, model.n_text_ctx
    assert_equal 512, model.n_text_state
    assert_equal 8, model.n_text_head
    assert_equal 6, model.n_text_layer
    assert_equal 80, model.n_mels
    assert_equal 1, model.ftype
    assert_equal "base", model.type
  end

  def test_gc
    model = Whisper::Context.new(MODEL).model
    GC.start

    assert_equal 51864, model.n_vocab
    assert_equal 1500, model.n_audio_ctx
    assert_equal 512, model.n_audio_state
    assert_equal 8, model.n_audio_head
    assert_equal 6, model.n_audio_layer
    assert_equal 448, model.n_text_ctx
    assert_equal 512, model.n_text_state
    assert_equal 8, model.n_text_head
    assert_equal 6, model.n_text_layer
    assert_equal 80, model.n_mels
    assert_equal 1, model.ftype
    assert_equal "base", model.type
  end
end
