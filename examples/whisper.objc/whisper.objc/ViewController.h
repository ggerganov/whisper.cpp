//
//  ViewController.h
//  whisper.objc
//
//  Created by Georgi Gerganov on 23.10.22.
//

#import <UIKit/UIKit.h>

#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioQueue.h>

#define NUM_BUFFERS 3
#define MAX_AUDIO_SEC 30
#define SAMPLE_RATE 16000

struct whisper_context;

typedef struct
{
    int ggwaveId;
    bool isCapturing;
    UILabel * labelReceived;

    AudioQueueRef queue;
    AudioStreamBasicDescription dataFormat;
    AudioQueueBufferRef buffers[NUM_BUFFERS];

    int n_samples;
    int16_t * audioBufferI16;
    float   * audioBufferF32;

    struct whisper_context * ctx;
} StateInp;

@interface ViewController : UIViewController
{
    StateInp stateInp;
}

@end
