#include "kernel_operator.h"

using namespace AscendC;
#ifdef ASCEND_310P // 310P not support float->4bit quantization
    extern "C" __global__ __aicore__ void ascendc_quantize_f32_to_q4_0(
        GM_ADDR input_gm, GM_ADDR output_gm, GM_ADDR input_ne_gm,
        GM_ADDR input_nb_gm, GM_ADDR output_ne_gm) {
        // let following test cases can continue run, here just print error information. Of Cource the test case that call this operator is failed.
        printf("Ascend310P not support f32->4bit quantization.\n");
    }

    extern "C" __global__ __aicore__ void ascendc_quantize_f16_to_q4_0(
        GM_ADDR input_gm, GM_ADDR output_gm, GM_ADDR input_ne_gm,
        GM_ADDR input_nb_gm, GM_ADDR output_ne_gm) {
        // let following test cases can continue run, here just print error information. Of Cource the test case that call this operator is failed.
        printf("Ascend310P not support f16->4bit quantization.\n");
    }
#else

#define BUFFER_NUM 2
#define Group_Size 32

template <typename SRC_T>
class QUANTIZE_FLOAT_TO_Q4_0 {
   public:
    __aicore__ inline QUANTIZE_FLOAT_TO_Q4_0() {}
    __aicore__ inline void init(GM_ADDR input, GM_ADDR output,
                                int64_t *input_ne_ub, size_t *input_nb_ub,
                                int64_t *output_ne_ub) {
        // TODO: fix test_case CPY(type_src=f16,type_dst=q4_0,ne=[256,4,4,4],
        //                         permute=[0,0,0,0]):
        // [CPY] NMSE = 0.000008343 > 0.000001000 FAIL
        int64_t op_block_num = GetBlockNum();
        int64_t op_block_idx = GetBlockIdx();

        // input stride of data elements
        for (int i = 0; i < 4; i++) {
            input_ne[i] = input_ne_ub[i];
            input_stride[i] = input_nb_ub[i] / input_nb_ub[0];
            output_ne[i] = output_ne_ub[i];
        }

        // output stride of data elements
        output_stride[0] = 1;
        for (int i = 1; i < 4; i++) {
            output_stride[i] = output_stride[i - 1] * output_ne[i - 1];
        }

        // scale saved one by one after data:. [group1_scale, group2_scale, ...]
        scale_ne = input_ne;
        scale_stride[0] = 1;
        scale_stride[1] = input_ne[0] / Group_Size;
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
        int64_t scale_offset = output_ne[0] * output_ne[1] * output_ne[2] *
                              output_ne[3] * sizeof(uint8_t) / 2;

        input_gm.SetGlobalBuffer((__gm__ SRC_T *)input);
        output_gm.SetGlobalBuffer((__gm__ int8_t *)output);
        scale_gm.SetGlobalBuffer((__gm__ half *)(output + scale_offset + ir *
                                                 group_size_in_row *
                                                 sizeof(half)));

        pipe.InitBuffer(input_queue, BUFFER_NUM, Group_Size * sizeof(SRC_T));
        pipe.InitBuffer(output_queue, BUFFER_NUM,
                            Group_Size * sizeof(int8_t) / 2);
        pipe.InitBuffer(cast_queue , 1, Group_Size * sizeof(float));
        pipe.InitBuffer(work_queue, 1, Group_Size * sizeof(float));
        pipe.InitBuffer(max_queue, 1, Group_Size * sizeof(float));
        pipe.InitBuffer(min_queue, 1, Group_Size * sizeof(float));
        pipe.InitBuffer(scale_queue, 1, Group_Size / 2 * sizeof(half));
        pipe.InitBuffer(int8_queue, 1, Group_Size * sizeof(int8_t));
        pipe.InitBuffer(half_queue, 1, Group_Size * sizeof(half));
    }

    __aicore__ inline void copy_in(uint32_t offset) {
        LocalTensor<SRC_T> input_local = input_queue.AllocTensor<SRC_T>();
        DataCopy(input_local, input_gm[offset], Group_Size);
        input_queue.EnQue(input_local);
    }

    __aicore__ inline void copy_out(uint32_t offset) {
        // reinterpretcast Group_Size(32) * int4b_t to Group_Size / 2 * int8_t,
        // and using DataCopyPad to avoid 32 bits align.
        LocalTensor<int4b_t> output_local = output_queue.DeQue<int4b_t>();
        LocalTensor<int8_t> output_int8_local =
                                    output_local.ReinterpretCast<int8_t>();

        DataCopyExtParams dataCopyParams;
        dataCopyParams.blockCount = 1;
        dataCopyParams.blockLen = Group_Size / 2  * sizeof(int8_t);
        DataCopyPad(output_gm[offset], output_int8_local, dataCopyParams);

        output_queue.FreeTensor(output_local);
    }

    __aicore__ inline void input_to_cast(LocalTensor<float> cast_local,
                                         LocalTensor<float> input_local) {
        DataCopy(cast_local, input_local, Group_Size);
    }

    __aicore__ inline void input_to_cast(LocalTensor<float> cast_local,
                                         LocalTensor<half> input_local) {
        Cast(cast_local, input_local, RoundMode::CAST_NONE, Group_Size);
    }

    __aicore__ inline half calculate_group(int64_t row, int64_t group) {
        const int64_t i3 = row / (input_ne[1] * input_ne[2]);
        const int64_t i2 = (row - i3 * input_ne[1] * input_ne[2]) / input_ne[1];
        const int64_t i1 =
            row - i3 * input_ne[1] * input_ne[2] - i2 * input_ne[1];

        const int64_t input_offset = i1 * input_stride[1] +
                                     i2 * input_stride[2] +
                                     i3 * input_stride[3] + Group_Size * group;

        // output_offset is stride for output_gm which datatype is int8_t and
        // divided by 2 is needed for int4b_t.
        const int64_t output_offset = (i1 * output_stride[1] +
                                       i2 * output_stride[2] +
                                       i3 * output_stride[3] +
                                       Group_Size * group) / 2;
        copy_in(input_offset);

        LocalTensor<SRC_T> input_local = input_queue.DeQue<SRC_T>();
        LocalTensor<int4b_t> output_local = output_queue.AllocTensor<int4b_t>();
        LocalTensor<float> cast_local = cast_queue.AllocTensor<float>();
        LocalTensor<float> work_local = work_queue.AllocTensor<float>();
        LocalTensor<float> max_local = max_queue.AllocTensor<float>();
        LocalTensor<float> min_local = min_queue.AllocTensor<float>();
        LocalTensor<int8_t> int8_local = int8_queue.AllocTensor<int8_t>();
        LocalTensor<half> half_local = half_queue.AllocTensor<half>();

        input_to_cast(cast_local, input_local);

        ReduceMax(max_local, cast_local, work_local, Group_Size);
        ReduceMin(min_local, cast_local, work_local, Group_Size);
        const float max_value = max_local.GetValue(0);
        const float min_value = min_local.GetValue(0);
        float d = max_value;
        if (min_value < 0 && (-1 * min_value) > max_value) {
            d = min_value;
        }

        d = d / (-8);
        if (d != 0) {
            Muls(cast_local, cast_local, 1.0f / d, Group_Size);
        }

        // range: [-8,8] -> [0.5,16.5] -> [0,16] -> [0,15] -> [-8,7]
        float scalar = 8.5f;
        Adds(cast_local, cast_local, scalar, Group_Size);
        Cast(cast_local, cast_local, RoundMode::CAST_FLOOR, Group_Size);
        scalar = 15.0f;
        Mins(cast_local, cast_local, scalar, Group_Size);
        scalar = -8.0f;
        Adds(cast_local, cast_local, scalar, Group_Size);

        // float->half->int4b
        Cast(half_local, cast_local, RoundMode::CAST_NONE, Group_Size);
        Cast(output_local, half_local, RoundMode::CAST_NONE, Group_Size);

        output_queue.EnQue(output_local);
        copy_out(output_offset);

        input_queue.FreeTensor(input_local);
        work_queue.FreeTensor(work_local);
        max_queue.FreeTensor(max_local);
        min_queue.FreeTensor(min_local);
        int8_queue.FreeTensor(int8_local);
        half_queue.FreeTensor(half_local);
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
                // Copy Group_Size/2 length data each time.
                if (scale_local_offset == Group_Size / 2) {
                    scale_local_offset = 0;
                    // TODO: OPTIMIZE ME
                    pipe_barrier(PIPE_ALL);
                    DataCopy(scale_gm[scale_global_offset], scale_local,
                                      Group_Size / 2);
                    pipe_barrier(PIPE_ALL);
                    scale_global_offset += Group_Size / 2;
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
        scale_queue.FreeTensor(scale_local);
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
    GlobalTensor<SRC_T> input_gm;
    GlobalTensor<half> scale_gm;
    GlobalTensor<int8_t> output_gm;
    TQue<QuePosition::VECIN, BUFFER_NUM> input_queue;
    TQue<QuePosition::VECOUT, BUFFER_NUM> output_queue;
    TQue<QuePosition::VECIN, BUFFER_NUM> work_queue;
    TQue<QuePosition::VECOUT, BUFFER_NUM> max_queue;
    TQue<QuePosition::VECOUT, BUFFER_NUM> min_queue;
    TQue<QuePosition::VECOUT, BUFFER_NUM> scale_queue;
    TQue<QuePosition::VECOUT, BUFFER_NUM> cast_queue;
    TQue<QuePosition::VECOUT, BUFFER_NUM> int8_queue;
    TQue<QuePosition::VECOUT, BUFFER_NUM> half_queue;
};

template <typename T>
__aicore__ inline void copy_to_ub(GM_ADDR gm, T *ub, size_t size) {
    auto gm_ptr = (__gm__ uint8_t *)gm;
    auto ub_ptr = (uint8_t *)(ub);
    for (int32_t i = 0; i < size; ++i, ++ub_ptr, ++gm_ptr) {
        *ub_ptr = *gm_ptr;
    }
}

extern "C" __global__ __aicore__ void ascendc_quantize_f16_to_q4_0(
    GM_ADDR input_gm, GM_ADDR output_gm, GM_ADDR input_ne_gm,
    GM_ADDR input_nb_gm, GM_ADDR output_ne_gm) {
    int64_t input_ne_ub[4];
    size_t input_nb_ub[4];
    int64_t output_ne_ub[4];

    copy_to_ub(input_ne_gm, input_ne_ub, 32);
    copy_to_ub(input_nb_gm, input_nb_ub, 32);
    copy_to_ub(output_ne_gm, output_ne_ub, 32);

    QUANTIZE_FLOAT_TO_Q4_0<half> op;
    op.init(input_gm, output_gm, input_ne_ub, input_nb_ub, output_ne_ub);
    op.calculate();
}

extern "C" __global__ __aicore__ void ascendc_quantize_f32_to_q4_0(
    GM_ADDR input_gm, GM_ADDR output_gm, GM_ADDR input_ne_gm,
    GM_ADDR input_nb_gm, GM_ADDR output_ne_gm) {
    int64_t input_ne_ub[4];
    size_t input_nb_ub[4];
    int64_t output_ne_ub[4];

    copy_to_ub(input_ne_gm, input_ne_ub, 32);
    copy_to_ub(input_nb_gm, input_nb_ub, 32);
    copy_to_ub(output_ne_gm, output_ne_ub, 32);

    QUANTIZE_FLOAT_TO_Q4_0<float> op;
    op.init(input_gm, output_gm, input_ne_ub, input_nb_ub, output_ne_ub);
    op.calculate();
}

#endif // #ifdef ASCEND_310P
