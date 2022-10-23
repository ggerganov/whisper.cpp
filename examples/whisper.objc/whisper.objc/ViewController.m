//
//  ViewController.m
//  whisper.objc
//
//  Created by Georgi Gerganov on 23.10.22.
//

#import "ViewController.h"

#import "whisper.h"

#define NUM_BYTES_PER_BUFFER 16*1024

// callback used to process captured audio
void AudioInputCallback(void * inUserData,
                        AudioQueueRef inAQ,
                        AudioQueueBufferRef inBuffer,
                        const AudioTimeStamp * inStartTime,
                        UInt32 inNumberPacketDescriptions,
                        const AudioStreamPacketDescription * inPacketDescs);

@interface ViewController ()

@property (weak, nonatomic) IBOutlet UILabel *labelStatusInp;
@property (weak, nonatomic) IBOutlet UIButton *buttonToggleCapture;
@property (weak, nonatomic) IBOutlet UIButton *buttonTranscribe;
@property (weak, nonatomic) IBOutlet UITextView *textviewResult;

@end

@implementation ViewController

- (void)setupAudioFormat:(AudioStreamBasicDescription*)format
{
    format->mSampleRate       = 16000;
    format->mFormatID         = kAudioFormatLinearPCM;
    format->mFramesPerPacket  = 1;
    format->mChannelsPerFrame = 1;
    format->mBytesPerFrame    = 2;
    format->mBytesPerPacket   = 2;
    format->mBitsPerChannel   = 16;
    format->mReserved         = 0;
    format->mFormatFlags      = kLinearPCMFormatFlagIsSignedInteger;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    // whisper.cpp initialization
    {
        // load the model
        NSString *modelPath = [[NSBundle mainBundle] pathForResource:@"ggml-base.en" ofType:@"bin"];

        // check if the model exists
        if (![[NSFileManager defaultManager] fileExistsAtPath:modelPath]) {
            NSLog(@"Model file not found");
            return;
        }

        NSLog(@"Loading model from %@", modelPath);

        // create ggml context
        stateInp.ctx = whisper_init([modelPath UTF8String]);

        // check if the model was loaded successfully
        if (stateInp.ctx == NULL) {
            NSLog(@"Failed to load model");
            return;
        }
    }

    // initialize audio format and buffers
    {
        [self setupAudioFormat:&stateInp.dataFormat];

        stateInp.n_samples = 0;
        stateInp.audioBufferI16 = malloc(MAX_AUDIO_SEC*SAMPLE_RATE*sizeof(int16_t));
        stateInp.audioBufferF32 = malloc(MAX_AUDIO_SEC*SAMPLE_RATE*sizeof(float));
    }
}

-(IBAction) stopCapturing {
    NSLog(@"Stop capturing");

    _labelStatusInp.text = @"Status: Idle";

    [_buttonToggleCapture setTitle:@"Start capturing" forState:UIControlStateNormal];
    [_buttonToggleCapture setBackgroundColor:[UIColor grayColor]];

    stateInp.isCapturing = false;

    AudioQueueStop(stateInp.queue, true);
    for (int i = 0; i < NUM_BUFFERS; i++) {
        AudioQueueFreeBuffer(stateInp.queue, stateInp.buffers[i]);
    }

    AudioQueueDispose(stateInp.queue, true);
}

- (IBAction)toggleCapture:(id)sender {
    if (stateInp.isCapturing) {
        // stop capturing
        [self stopCapturing];

        return;
    }

    // initiate audio capturing
    NSLog(@"Start capturing");

    stateInp.n_samples = 0;

    OSStatus status = AudioQueueNewInput(&stateInp.dataFormat,
                                         AudioInputCallback,
                                         &stateInp,
                                         CFRunLoopGetCurrent(),
                                         kCFRunLoopCommonModes,
                                         0,
                                         &stateInp.queue);

    if (status == 0) {
        for (int i = 0; i < NUM_BUFFERS; i++) {
            AudioQueueAllocateBuffer(stateInp.queue, NUM_BYTES_PER_BUFFER, &stateInp.buffers[i]);
            AudioQueueEnqueueBuffer (stateInp.queue, stateInp.buffers[i], 0, NULL);
        }

        stateInp.isCapturing = true;
        status = AudioQueueStart(stateInp.queue, NULL);
        if (status == 0) {
            _labelStatusInp.text = @"Status: Capturing";
            [sender setTitle:@"Stop Capturing" forState:UIControlStateNormal];
            [_buttonToggleCapture setBackgroundColor:[UIColor redColor]];
        }
    }

    if (status != 0) {
        [self stopCapturing];
    }
}

- (IBAction)onTranscribePrepare:(id)sender {
    _textviewResult.text = @"Processing - please wait ...";

    if (stateInp.isCapturing) {
        // stop capturing
        [self stopCapturing];

        return;
    }
}

- (IBAction)onTranscribe:(id)sender {
    NSLog(@"Processing %d samples", stateInp.n_samples);

    // process captured audio
    // convert I16 to F32
    for (int i = 0; i < stateInp.n_samples; i++) {
        stateInp.audioBufferF32[i] = (float)stateInp.audioBufferI16[i] / 32768.0f;
    }

    // run the model
    struct whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

    params.print_realtime       = true;
    params.print_progress       = false;
    params.print_timestamps     = true;
    params.print_special_tokens = false;
    params.translate            = false;
    params.language             = "en";
    params.n_threads            = 4;
    params.offset_ms            = 0;

    CFTimeInterval startTime = CACurrentMediaTime();

    if (whisper_full(stateInp.ctx, params, stateInp.audioBufferF32, stateInp.n_samples) != 0) {
        NSLog(@"Failed to run the model");
        _textviewResult.text = @"Failed to run the model";

        return;
    }

    CFTimeInterval endTime = CACurrentMediaTime();

    // clear the text in the textview
    _textviewResult.text = @"";

    int n_segments = whisper_full_n_segments(stateInp.ctx);
    for (int i = 0; i < n_segments; i++) {
        const char * text_cur = whisper_full_get_segment_text(stateInp.ctx, i);

        // append the text to the textview
        _textviewResult.text = [_textviewResult.text stringByAppendingString:[NSString stringWithUTF8String:text_cur]];
    }

    // internal model timing
    whisper_print_timings(stateInp.ctx);

    NSLog(@"\nProcessing time: %5.3f", endTime - startTime);

    _textviewResult.text = [_textviewResult.text stringByAppendingString:[NSString stringWithFormat:@"\n\n[processing time: %5.3f s]", endTime - startTime]];
}

//
// Callback implmentation
//

void AudioInputCallback(void * inUserData,
                        AudioQueueRef inAQ,
                        AudioQueueBufferRef inBuffer,
                        const AudioTimeStamp * inStartTime,
                        UInt32 inNumberPacketDescriptions,
                        const AudioStreamPacketDescription * inPacketDescs)
{
    StateInp * stateInp = (StateInp*)inUserData;

    if (!stateInp->isCapturing) {
        NSLog(@"Not capturing, ignoring audio");
        return;
    }

    const int n = inBuffer->mAudioDataByteSize / 2;

    NSLog(@"Captured %d new samples", n);

    if (stateInp->n_samples + n > MAX_AUDIO_SEC*SAMPLE_RATE) {
        NSLog(@"Too much audio data, ignoring");
        return;
    }

    for (int i = 0; i < n; i++) {
        stateInp->audioBufferI16[stateInp->n_samples + i] = ((short*)inBuffer->mAudioData)[i];
    }

    stateInp->n_samples += n;

    // put the buffer back in the queue
    AudioQueueEnqueueBuffer(stateInp->queue, inBuffer, 0, NULL);
}

@end
