#include "kernel_operator.h"

using namespace AscendC;
#ifdef ASCEND_310P // 310P not support f32->8bit quantization
    extern "C" __global__ __aicore__ void ascendc_quantize_f32_q8_0(
        GM_ADDR input_gm, GM_ADDR output_gm, GM_ADDR input_ne_gm,
        GM_ADDR input_nb_gm, GM_ADDR output_ne_gm) {
        // let following test cases can continue run, here just print error information. Of Cource the test case that call this operator is failed.
        printf("Ascend310P not support f32->8bit quantization.\n");
    }
#else

#define BUFFER_NUM 2
#define QK8_0 32

class QUANTIZE_F32_Q8_0 {
   public:
    __aicore__ inline QUANTIZE_F32_Q8_0() {}
    __aicore__ inline void init(GM_ADDR input, GM_ADDR output,
                                int64_t *input_ne_ub, size_t *input_nb_ub,
                                int64_t *output_ne_ub) {
        int64_t op_block_num = GetBlockNum();
        int64_t op_block_idx = GetBlockIdx();

        for (int i = 0; i < 4; i++) {
            input_ne[i] = input_ne_ub[i];
            input_stride[i] = input_nb_ub[i] / input_nb_ub[0];

            output_ne[i] = output_ne_ub[i];
        }

        output_stride[0] = 1;
        for (int i = 1; i < 4; i++) {
            output_stride[i] = output_stride[i - 1] * output_ne[i - 1];
        }

        scale_ne = input_ne;
        scale_stride[0] = 1;
        scale_stride[1] = input_ne[0] / QK8_0;
        for (int i = 2; i < 4; i++) {
            scale_stride[i] = scale_stride[i - 1] * scale_ne[i - 1];
        }

        // split input tensor by rows.
        uint64_t nr = input_ne[1] * input_ne[2] * input_ne[3];
        dr = nr / op_block_num;

        uint64_t tails = nr % op_block_num;
        if (op_block_idx < tails) {
            dr += 1;
            ir = dr * op_block_idx;
        } else {
            ir = dr * op_block_idx + tails;
        }

        group_size_in_row = scale_stride[1];
        int64_t output_size = output_ne[0] * output_ne[1] * output_ne[2] *
                              output_ne[3] * sizeof(uint8_t);

        input_gm.SetGlobalBuffer((__gm__ float *)input);
        output_gm.SetGlobalBuffer((__gm__ int8_t *)output);
        scale_gm.SetGlobalBuffer((__gm__ half *)(output + output_size +
                                                 ir * group_size_in_row *
                                                 sizeof(half)));

        pipe.InitBuffer(input_queue, BUFFER_NUM, QK8_0 * sizeof(float));
        pipe.InitBuffer(output_queue, BUFFER_NUM, QK8_0 * sizeof(int8_t));
        pipe.InitBuffer(work_queue, 1, 32);
        pipe.InitBuffer(max_queue, 1, 32);
        pipe.InitBuffer(abs_queue, 1, QK8_0 * sizeof(float));
        pipe.InitBuffer(cast_queue, 1, QK8_0 * sizeof(half));
        pipe.InitBuffer(scale_queue, 1, 32);
    }

    __aicore__ inline void copy_in(uint32_t offset) {
        LocalTensor<float> input_local = input_queue.AllocTensor<float>();
        DataCopy(input_local, input_gm[offset], QK8_0);
        input_queue.EnQue(input_local);
    }

    __aicore__ inline void copy_out(uint32_t offset) {
        LocalTensor<int8_t> output_local = output_queue.DeQue<int8_t>();
        DataCopy(output_gm[offset], output_local, QK8_0);
        output_queue.FreeTensor(output_local);
    }

    __aicore__ inline half calculate_group(int64_t row, int64_t group) {
        const int64_t i3 = row / (input_ne[1] * input_ne[2]);
        const int64_t i2 = (row - i3 * input_ne[1] * input_ne[2]) / input_ne[1];
        const int64_t i1 =
            row - i3 * input_ne[1] * input_ne[2] - i2 * input_ne[1];

        const int64_t input_offset = i1 * input_stride[1] +
                                     i2 * input_stride[2] +
                                     i3 * input_stride[3] + QK8_0 * group;

        const int64_t output_offset = i1 * output_stride[1] +
                                      i2 * output_stride[2] +
                                      i3 * output_stride[3] + QK8_0 * group;

        copy_in(input_offset);
        LocalTensor<float> input_local = input_queue.DeQue<float>();
        LocalTensor<int8_t> output_local = output_queue.AllocTensor<int8_t>();
        LocalTensor<float> work_local = work_queue.AllocTensor<float>();
        LocalTensor<float> abs_local = abs_queue.AllocTensor<float>();
        LocalTensor<float> max_local = max_queue.AllocTensor<float>();
        LocalTensor<half> cast_local = cast_queue.AllocTensor<half>();

        Abs(abs_local, input_local, QK8_0);
        ReduceMax(max_local, abs_local, work_local, QK8_0);
        pipe_barrier(PIPE_ALL);
        float d = max_local.GetValue(0);
        d = d / ((1 << 7) - 1);
        if (d != 0) {
            Muls(input_local, input_local, 1.0f / d, QK8_0);
        }

        Cast(input_local, input_local, RoundMode::CAST_ROUND, QK8_0);
        Cast(cast_local, input_local, RoundMode::CAST_ROUND, QK8_0);
        Cast(output_local, cast_local, RoundMode::CAST_ROUND, QK8_0);
        output_queue.EnQue(output_local);
        copy_out(output_offset);

        input_queue.FreeTensor(input_local);
        work_queue.FreeTensor(work_local);
        abs_queue.FreeTensor(abs_local);
        max_queue.FreeTensor(max_local);
        cast_queue.FreeTensor(cast_local);

        return (half)d;
    }

    __aicore__ inline void calculate() {
        LocalTensor<half> scale_local = scale_queue.AllocTensor<half>();
        uint32_t scale_local_offset = 0;
        uint32_t scale_global_offset = 0;
        for (int64_t i = ir; i < ir + dr; i++) {
            for (int64_t j = 0; j < group_size_in_row; j++) {
                half scale = calculate_group(i, j);
                scale_local.SetValue(scale_local_offset++, scale);
                if (scale_local_offset == 16) {
                    scale_local_offset = 0;
                    // TODO: OPTIMIZE ME
                    pipe_barrier(PIPE_ALL);
                    DataCopy(scale_gm[scale_global_offset], scale_local, 16);
                    pipe_barrier(PIPE_ALL);
                    scale_global_offset += 16;
                }
            }
        }

        if (scale_local_offset != 0) {
            pipe_barrier(PIPE_ALL);
            DataCopyExtParams dataCopyParams;
            dataCopyParams.blockCount = 1;
            dataCopyParams.blockLen = scale_local_offset * sizeof(half);
            DataCopyPad(scale_gm[scale_global_offset], scale_local,
                        dataCopyParams);
            pipe_barrier(PIPE_ALL);
        }
    }

   private:
    int64_t input_ne[4];
    size_t input_stride[4];

    int64_t *scale_ne;
    size_t scale_stride[4];

    int64_t output_ne[4];
    size_t output_stride[4];

    int64_t group_size_in_row;

    int64_t ir;
    int64_t dr;

    TPipe pipe;
    GlobalTensor<float> input_gm;
    GlobalTensor<half> scale_gm;
    GlobalTensor<int8_t> output_gm;
    TQue<QuePosition::VECIN, BUFFER_NUM> input_queue;
    TQue<QuePosition::VECOUT, BUFFER_NUM> output_queue;
    TQue<QuePosition::VECIN, 1> work_queue;
    TQue<QuePosition::VECOUT, 1> max_queue;
    TQue<QuePosition::VECIN, 1> abs_queue;
    TQue<QuePosition::VECIN, 1> cast_queue;
    TQue<QuePosition::VECOUT, 1> scale_queue;
};

template <typename T>
__aicore__ inline void copy_to_ub(GM_ADDR gm, T *ub, size_t size) {
    auto gm_ptr = (__gm__ uint8_t *)gm;
    auto ub_ptr = (uint8_t *)(ub);
    for (int32_t i = 0; i < size; ++i, ++ub_ptr, ++gm_ptr) {
        *ub_ptr = *gm_ptr;
    }
}

extern "C" __global__ __aicore__ void ascendc_quantize_f32_q8_0(
    GM_ADDR input_gm, GM_ADDR output_gm, GM_ADDR input_ne_gm,
    GM_ADDR input_nb_gm, GM_ADDR output_ne_gm) {
    int64_t input_ne_ub[4];
    size_t input_nb_ub[4];
    int64_t output_ne_ub[4];

    copy_to_ub(input_ne_gm, input_ne_ub, 32);
    copy_to_ub(input_nb_gm, input_nb_ub, 32);
    copy_to_ub(output_ne_gm, output_ne_ub, 32);

    QUANTIZE_F32_Q8_0 op;
    op.init(input_gm, output_gm, input_ne_ub, input_nb_ub, output_ne_ub);
    op.calculate();
}

#endif // #ifdef ASCEND_310P
