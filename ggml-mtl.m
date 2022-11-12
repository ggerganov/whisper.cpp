#import "ggml-mtl.h"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

#define GGML_MTL_MAX_BUFFERS 256

// global static storage for Metal buffers
// TODO: move this into a dynamic context
static id<MTLBuffer> g_buffers[GGML_MTL_MAX_BUFFERS];

// global MTL context
// TODO: move this into a dynamic context
static id<MTLDevice>       g_device;
static id<MTLCommandQueue> g_command_queue;

struct ggml_mtl_context * ggml_mtl_init() {
    // TODO: implement properly
    //       for now, init the global MTL context and MTL buffers
    g_device = MTLCreateSystemDefaultDevice();

    g_command_queue = [g_device newCommandQueue];
    if (g_command_queue == nil)
    {
        NSLog(@"Failed to find the command queue.");
        return nil;
    }

    return nil;
}

// search for unallocated buffer slot and use it
struct ggml_mtl_object ggml_mtl_alloc(size_t size) {
    // TODO: temporarily making sure that the buffers are nil at the start
    static bool first = true;
    if (first) {
        for (int i = 0; i < GGML_MTL_MAX_BUFFERS; ++i) {
            assert(g_buffers[i] == nil);
        }
        first = false;
    }

    struct ggml_mtl_object obj = { -1, nil };

    for (int i = 0; i < GGML_MTL_MAX_BUFFERS; i++) {
        if (g_buffers[i] == nil) {
            g_buffers[i] = [g_device newBufferWithLength:size options:MTLResourceStorageModeManaged];

            // lunk the MTL buffer to the ggml object
            obj.id = i;
            obj.data = [g_buffers[i] contents];

            break;
        }
    }

    return obj;
}

struct params_mul_mat_vec {
    int  N; // rows
    int  M; // cols
};

// multiply matrix with a vector using MPSMatrixVectorMultiplication
void ggml_mtl_mul_mat_vec_f16(
        struct ggml_mtl_context * ctx,
        struct ggml_mtl_object    src0,
        const __fp16            * src1,
        float                   * dst,
        int                       nrows,
        int                       ncols) {
    (void) ctx; // unused

    // Create a command buffer to hold commands.
    id<MTLCommandBuffer> commandBuffer = [g_command_queue commandBuffer];
    assert(commandBuffer != nil);

    // make managed device buffer to store src1
    id<MTLBuffer> src1_buffer = [g_device newBufferWithBytes:src1 length:ncols*sizeof(__fp16) options:MTLResourceStorageModeManaged];
    id<MTLBuffer> dst_buffer  = [g_device newBufferWithLength:nrows*sizeof(float) options:MTLResourceStorageModeManaged];

    // MPSMatrixDescriptor
    MPSMatrixDescriptor *src0_desc = [MPSMatrixDescriptor matrixDescriptorWithRows:nrows columns:ncols rowBytes:ncols*sizeof(__fp16) dataType:MPSDataTypeFloat16];
    MPSVectorDescriptor *src1_desc = [MPSVectorDescriptor vectorDescriptorWithLength:ncols dataType:MPSDataTypeFloat16];
    MPSVectorDescriptor *dst_desc  = [MPSVectorDescriptor vectorDescriptorWithLength:nrows dataType:MPSDataTypeFloat32];

    // MPSMatrix
    MPSMatrix *src0_mat = [[MPSMatrix alloc] initWithBuffer:g_buffers[src0.id] descriptor:src0_desc];
    MPSVector *src1_vec = [[MPSVector alloc] initWithBuffer:src1_buffer        descriptor:src1_desc];
    MPSVector *dst_vec  = [[MPSVector alloc] initWithBuffer:dst_buffer         descriptor:dst_desc];

    // MPSMatrixVectorMultiplication
    MPSMatrixVectorMultiplication *mul_mat_vec = [[MPSMatrixVectorMultiplication alloc] initWithDevice:g_device transpose:NO rows:nrows columns:ncols alpha:1.0 beta:0.0];

    // encode
    [mul_mat_vec encodeToCommandBuffer:commandBuffer
                           inputMatrix:src0_mat
                           inputVector:src1_vec
                          resultVector:dst_vec];

    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];

    // copy GPU result to CPU
    memcpy(dst, [dst_buffer contents], nrows*sizeof(float));
}

// multiply matrix with a matrix using MPSMatrixMultiplication
void ggml_mtl_mul_mat_f16(
        struct ggml_mtl_context * ctx,
        struct ggml_mtl_object    src0,
        const __fp16            * src1,
        float                   * dst,
        int                       nrows0,
        int                       nrows1,
        int                       ncols) {
    (void) ctx; // unused

    // Create a command buffer to hold commands.
    id<MTLCommandBuffer> commandBuffer = [g_command_queue commandBuffer];
    assert(commandBuffer != nil);

    // make managed device buffer to store src1
    id<MTLBuffer> src1_buffer = [g_device newBufferWithBytes:src1 length:ncols*nrows1*sizeof(__fp16) options:MTLResourceStorageModeManaged];
    id<MTLBuffer> dst_buffer  = [g_device newBufferWithLength:nrows0*nrows1*sizeof(float) options:MTLResourceStorageModeManaged];

    // MPSMatrixDescriptor
    MPSMatrixDescriptor *src0_desc = [MPSMatrixDescriptor matrixDescriptorWithRows:nrows0 columns:ncols  rowBytes:ncols*sizeof(__fp16) dataType:MPSDataTypeFloat16];
    MPSMatrixDescriptor *src1_desc = [MPSMatrixDescriptor matrixDescriptorWithRows:nrows1 columns:ncols  rowBytes:ncols*sizeof(__fp16) dataType:MPSDataTypeFloat16];
    MPSMatrixDescriptor *dst_desc  = [MPSMatrixDescriptor matrixDescriptorWithRows:nrows1 columns:nrows0 rowBytes:nrows0*sizeof(float) dataType:MPSDataTypeFloat32];

    // MPSMatrix
    MPSMatrix *src0_mat = [[MPSMatrix alloc] initWithBuffer:g_buffers[src0.id] descriptor:src0_desc];
    MPSMatrix *src1_mat = [[MPSMatrix alloc] initWithBuffer:src1_buffer        descriptor:src1_desc];
    MPSMatrix *dst_mat  = [[MPSMatrix alloc] initWithBuffer:dst_buffer         descriptor:dst_desc];

    //// MPSMatrixMultiplication z = x * yT
    //MPSMatrixMultiplication *mul_mat = [[MPSMatrixMultiplication alloc] initWithDevice:g_device transposeLeft:NO transposeRight:YES resultRows:nrows resultColumns:nrows interiorColumns:ncols alpha:1.0 beta:0.0];

    //// encode
    //[mul_mat encodeToCommandBuffer:commandBuffer
    //                   leftMatrix:src0_mat
    //                  rightMatrix:src1_mat
    //                 resultMatrix:dst_mat];

    // MPSMatrixMultiplication zT = xT * y
    MPSMatrixMultiplication *mul_mat = [[MPSMatrixMultiplication alloc] initWithDevice:g_device transposeLeft:NO transposeRight:YES resultRows:nrows1 resultColumns:nrows0 interiorColumns:ncols alpha:1.0 beta:0.0];

    // encode
    [mul_mat encodeToCommandBuffer:commandBuffer
                       leftMatrix:src1_mat
                      rightMatrix:src0_mat
                     resultMatrix:dst_mat];

    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];

    // copy GPU result to CPU
    memcpy(dst, [dst_buffer contents], nrows0*nrows1*sizeof(float));
}
