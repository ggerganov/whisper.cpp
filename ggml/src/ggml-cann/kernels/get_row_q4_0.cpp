#include "kernel_operator.h"

// optimize me. Use template to avoid copy code.
using namespace AscendC;

#define BUFFER_NUM 2

#define QK4_0 32

class GET_ROW_Q4_0 {
   public:
    __aicore__ inline GET_ROW_Q4_0() {}
    __aicore__ inline void init(GM_ADDR input, GM_ADDR indices, GM_ADDR output,
                                int64_t *input_ne_ub, int64_t *indices_ne_ub,
                                size_t *indices_nb_ub, int64_t *output_ne_ub,
                                size_t *output_nb_ub) {
        int64_t op_block_num = GetBlockNum();
        int64_t op_block_idx = GetBlockIdx();

        for (int i = 0; i < 4; i++) {
            input_ne[i] = input_ne_ub[i];
            indices_ne[i] = indices_ne_ub[i];
            indices_stride[i] = indices_nb_ub[i] / indices_nb_ub[0];
            scale_ne[i] = input_ne_ub[i];
            output_ne[i] = output_ne_ub[i];
            output_stride[i] = output_nb_ub[i] / output_nb_ub[0];
        }

        // one scale for a group.
        scale_ne[0] /= QK4_0;

        input_stride[0] = 1;
        scale_stride[0] = 1;
        output_stride[0] = 1;
        for (int i = 1; i < 4; i++) {
            input_stride[i] = input_stride[i - 1] * input_ne[i - 1];
            scale_stride[i] = scale_stride[i - 1] * scale_ne[i - 1];
        }

        group_size_in_row = input_ne[0] / QK4_0;
        int64_t scale_offset = input_ne[0] * input_ne[1] * input_ne[2] *
                               input_ne[3] / 2;

        // Indices has two dims. n_elements = all rows should get.
        // dr, all rows should this thread get.
        uint64_t n_elements =
            indices_ne[0] * indices_ne[1] * indices_ne[2] * indices_ne[3];
        dr = n_elements / op_block_num;

        uint64_t tails = n_elements % op_block_num;
        if (op_block_idx < tails) {
            dr += 1;
            ir = dr * op_block_idx;
        } else {
            ir = dr * op_block_idx + tails;
        }

        input_gm.SetGlobalBuffer((__gm__ int4b_t *)input);
        scale_gm.SetGlobalBuffer((__gm__ half *)(input + scale_offset));
        indices_gm.SetGlobalBuffer((__gm__ int32_t *)indices);
        output_gm.SetGlobalBuffer((__gm__ float *)output);

        pipe.InitBuffer(input_queue, BUFFER_NUM, QK4_0 * sizeof(int4b_t));
        pipe.InitBuffer(cast_queue, BUFFER_NUM, QK4_0 * sizeof(half));
        pipe.InitBuffer(output_queue, BUFFER_NUM, QK4_0 * sizeof(float));
    }

    __aicore__ inline void copy_in(uint32_t offset) {
        LocalTensor<int4b_t> input_local = input_queue.AllocTensor<int4b_t>();
        // 32 * sizeof(int4b_t) = 16, which is not aligned to 32, why no error?
        DataCopy(input_local, input_gm[offset], QK4_0);
        input_queue.EnQue(input_local);
    }

    __aicore__ inline void copy_out(uint32_t offset) {
        LocalTensor<float> output_local = output_queue.DeQue<float>();
        DataCopy(output_gm[offset], output_local, QK4_0);
        output_queue.FreeTensor(output_local);
    }

    __aicore__ inline void calculate_group(int64_t idx, int64_t group) {
        const int64_t indices_ne2_idx = idx / (indices_ne[0] * indices_ne[1]);
        const int64_t indices_ne1_idx =
            (idx - indices_ne2_idx * indices_ne[0] * indices_ne[1]) /
            indices_ne[0];
        const int64_t indices_ne0_idx =
            (idx - indices_ne2_idx * indices_ne[0] * indices_ne[1] -
             indices_ne1_idx * indices_ne[0]);

        const int64_t indices_offset = indices_ne0_idx * indices_stride[0] +
                                       indices_ne1_idx * indices_stride[1] +
                                       indices_ne2_idx * indices_stride[2];
        const int32_t selected_row_idx = indices_gm.GetValue(indices_offset);

        const int64_t input_offset = selected_row_idx * input_stride[1] +
                                     indices_ne1_idx * input_stride[2] +
                                     indices_ne2_idx * input_stride[3] +
                                     group * QK4_0;
        const int64_t scale_offset = selected_row_idx * scale_stride[1] +
                                     indices_ne1_idx * scale_stride[2] +
                                     indices_ne2_idx * scale_stride[3] + group;
        const int64_t output_offset = indices_ne0_idx * output_stride[1] +
                                      indices_ne1_idx * output_stride[2] +
                                      indices_ne2_idx * output_stride[3] +
                                      group * QK4_0;

        copy_in(input_offset);
        LocalTensor<int4b_t> input_local = input_queue.DeQue<int4b_t>();
        LocalTensor<half> cast_local = cast_queue.AllocTensor<half>();
        LocalTensor<float> output_local = output_queue.AllocTensor<float>();

        // TODO: cast more data to speed up.
#ifdef ASCEND_310P
        // TODO: 310P support quantification
#else
        Cast(cast_local, input_local, RoundMode::CAST_NONE, QK4_0);
        Cast(output_local, cast_local, RoundMode::CAST_NONE, QK4_0);
#endif
        // Only mul need compile by group.
        half scale = scale_gm.GetValue(scale_offset);

        Muls(output_local, output_local, (float)scale, QK4_0);

        input_queue.FreeTensor(input_local);
        cast_queue.FreeTensor(cast_local);
        output_queue.EnQue(output_local);

        copy_out(output_offset);
    }

    __aicore__ inline void calculate() {
        for (int64_t i = ir; i < ir + dr; i++) {
            for (int64_t j = 0; j < group_size_in_row; j++) {
                calculate_group(i, j);
            }
        }
    }

   private:
    int64_t input_ne[4];
    size_t input_stride[4];

    int64_t scale_ne[4];
    size_t scale_stride[4];

    int64_t indices_ne[4];
    size_t indices_stride[4];

    int64_t output_ne[4];
    size_t output_stride[4];

    int64_t ir;
    int64_t dr;

    int64_t group_size_in_row;

    TPipe pipe;
    GlobalTensor<int4b_t> input_gm;
    GlobalTensor<half> scale_gm;
    GlobalTensor<int32_t> indices_gm;
    GlobalTensor<float> output_gm;
    TQue<QuePosition::VECIN, BUFFER_NUM> input_queue;
    TQue<QuePosition::VECOUT, BUFFER_NUM> output_queue;
    TQue<QuePosition::VECIN, BUFFER_NUM> cast_queue;
};

template <typename T>
__aicore__ inline void copy_to_ub(GM_ADDR gm, T *ub, size_t size) {
    auto gm_ptr = (__gm__ uint8_t *)gm;
    auto ub_ptr = (uint8_t *)(ub);
    for (int32_t i = 0; i < size; ++i, ++ub_ptr, ++gm_ptr) {
        *ub_ptr = *gm_ptr;
    }
}

extern "C" __global__ __aicore__ void ascendc_get_row_q4_0(
    GM_ADDR input_gm, GM_ADDR indices_gm, GM_ADDR output_gm,
    GM_ADDR input_ne_gm, GM_ADDR indices_ne_gm, GM_ADDR indices_nb_gm,
    GM_ADDR output_ne_gm, GM_ADDR output_nb_gm) {
    int64_t input_ne_ub[4];
    int64_t indices_ne_ub[4];
    size_t indices_nb_ub[4];
    int64_t output_ne_ub[4];
    size_t output_nb_ub[4];

    copy_to_ub(input_ne_gm, input_ne_ub, 32);
    copy_to_ub(indices_ne_gm, indices_ne_ub, 32);
    copy_to_ub(indices_nb_gm, indices_nb_ub, 32);
    copy_to_ub(output_ne_gm, output_ne_ub, 32);
    copy_to_ub(output_nb_gm, output_nb_ub, 32);

    GET_ROW_Q4_0 op;
    op.init(input_gm, indices_gm, output_gm, input_ne_ub, indices_ne_ub,
            indices_nb_ub, output_ne_ub, output_nb_ub);
    op.calculate();
}
