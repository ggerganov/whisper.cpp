#import "coreml/whisper-encoder.h"
#import "coreml/whisper-encoder-impl.h"

#import <CoreML/CoreML.h>

#include <stdlib.h>

#if __cplusplus
extern "C" {
#endif

struct whisper_coreml_context {
    const void * data;
};

struct whisper_coreml_context * whisper_coreml_init(const char * path_model) {
    NSString * path_model_str = [[NSString alloc] initWithUTF8String:path_model];

    NSURL * url_model = [NSURL fileURLWithPath: path_model_str];

    const void * data = CFBridgingRetain([[CoremlEncoder alloc] initWithContentsOfURL:url_model error:nil]);

    if (data == NULL) {
        return NULL;
    }

    whisper_coreml_context * ctx = new whisper_coreml_context;

    ctx->data = data;

    return ctx;
}

void whisper_coreml_free(struct whisper_coreml_context * ctx) {
    CFRelease(ctx->data);
    delete ctx;
}

void whisper_coreml_encode(
        const whisper_coreml_context * ctx,
                               float * mel,
                               float * out) {
    MLMultiArray * inMultiArray = [
        [MLMultiArray alloc] initWithDataPointer: mel
                                           shape: @[@1, @80, @3000]
                                        dataType: MLMultiArrayDataTypeFloat32
                                         strides: @[@(240000), @(3000), @1]
                                     deallocator: nil
                                           error: nil
    ];

    CoremlEncoderOutput * outCoreML = [(__bridge id) ctx->data predictionFromMelSegment:inMultiArray error:nil];

    MLMultiArray * outMA = outCoreML.output;

    memcpy(out, outMA.dataPointer, outMA.count * sizeof(float));
}

#if __cplusplus
}
#endif
