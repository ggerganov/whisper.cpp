#include "kernel_operator.h"

// optimize me. Use template to avoid copy code.
using namespace AscendC;

#define BUFFER_NUM 2

class GET_ROW_F16 {
   public:
    __aicore__ inline GET_ROW_F16() {}
    __aicore__ inline void init(GM_ADDR input, GM_ADDR indices, GM_ADDR output,
                                int64_t *input_ne_ub, size_t *input_nb_ub,
                                int64_t *indices_ne_ub, size_t *indices_nb_ub,
                                int64_t *output_ne_ub, size_t *output_nb_ub) {
        // TODO, use template for F16/f32
        int64_t op_block_num = GetBlockNum();
        op_block_idx = GetBlockIdx();

        for (int i = 0; i < 4; i++) {
            input_ne[i] = input_ne_ub[i];
            input_stride[i] = input_nb_ub[i] / input_nb_ub[0];

            indices_ne[i] = indices_ne_ub[i];
            indices_stride[i] = indices_nb_ub[i] / indices_nb_ub[0];

            output_ne[i] = output_ne_ub[i];
            output_stride[i] = output_nb_ub[i] / output_nb_ub[0];
        }

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

        input_gm.SetGlobalBuffer((__gm__ half *)input);
        indices_gm.SetGlobalBuffer((__gm__ int32_t *)indices);
        output_gm.SetGlobalBuffer((__gm__ float *)output);

        uint64_t input_local_buffer_size = ((input_ne[0] * sizeof(half) + 31)
                                             & ~31);
        uint64_t output_local_buffer_size = ((input_ne[0] * sizeof(float) + 31)
                                              & ~31);

        local_buffer_elems = input_local_buffer_size / sizeof(half);

        // TODO, consider long row that can't put in UB.
        // All data should asign to 32. It's ok because all data is align to 32.
        pipe.InitBuffer(input_queue, BUFFER_NUM, input_local_buffer_size);
        pipe.InitBuffer(output_queue, BUFFER_NUM, output_local_buffer_size);
    }

    __aicore__ inline void copy_in(uint32_t offset, size_t len) {
        size_t origin_len = len;
        LocalTensor<half> input_local = input_queue.AllocTensor<half>();
        const size_t elem_per_block = 32 / sizeof(half);
        size_t tail = len % elem_per_block;
        len = len & ~(elem_per_block - 1);
        if(tail != 0) {
            len += elem_per_block;
        }
        DataCopy(input_local, input_gm[offset], len);
        input_queue.EnQue(input_local);
    }

    __aicore__ inline void copy_out(uint32_t offset, size_t len) {
        LocalTensor<float> output_local = output_queue.DeQue<float>();
        const size_t elem_per_block = 32 / sizeof(float);
        size_t tail = len % elem_per_block;
        len = len & ~(elem_per_block - 1);
        if (len > 0) {
            DataCopy(output_gm[offset], output_local, len);
        }

        if(tail != 0) {
#ifdef ASCEND_310P
            for (size_t i = tail; i < elem_per_block; i++) {
                output_local[len + i].SetValue(0, 0);
            }
            SetAtomicAdd<float>();
            DataCopy(output_gm[offset + len], output_local[len], elem_per_block);
            SetAtomicNone();
#else
            DataCopyExtParams dataCopyParams;
            dataCopyParams.blockCount = 1;
            dataCopyParams.blockLen = tail * sizeof(float);
            DataCopyPad(output_gm[offset + len], output_local[len],
                        dataCopyParams);
#endif
        }
        output_queue.FreeTensor(output_local);
    }

    __aicore__ inline void calculate_row(int64_t idx) {
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
                                     indices_ne2_idx * input_stride[3];

        const int64_t output_offset = indices_ne0_idx * output_stride[1] +
                                      indices_ne1_idx * output_stride[2] +
                                      indices_ne2_idx * output_stride[3];

        copy_in(input_offset, input_ne[0]);
        LocalTensor<half> input_local = input_queue.DeQue<half>();
        LocalTensor<float> output_local = output_queue.AllocTensor<float>();

        Cast(output_local, input_local, RoundMode::CAST_NONE,
             local_buffer_elems);
        output_queue.EnQue(output_local);
        copy_out(output_offset, input_ne[0]);

        input_queue.FreeTensor(input_local);
    }

    __aicore__ inline void calculate() {
        for (int64_t i = ir; i < ir + dr; i++) {
            calculate_row(i);
        }
    }

   private:
    int64_t input_ne[4];
    size_t input_stride[4];

    int64_t indices_ne[4];
    size_t indices_stride[4];

    int64_t output_ne[4];
    size_t output_stride[4];

    size_t local_buffer_elems;

    int64_t ir;
    int64_t dr;

    TPipe pipe;
    GlobalTensor<half> input_gm;
    GlobalTensor<int32_t> indices_gm;
    GlobalTensor<float> output_gm;
    TQue<QuePosition::VECIN, BUFFER_NUM> input_queue;
    TQue<QuePosition::VECOUT, BUFFER_NUM> output_queue;
    int64_t op_block_idx;
};

template <typename T>
__aicore__ inline void copy_to_ub(GM_ADDR gm, T *ub, size_t size) {
    auto gm_ptr = (__gm__ uint8_t *)gm;
    auto ub_ptr = (uint8_t *)(ub);
    for (int32_t i = 0; i < size; ++i, ++ub_ptr, ++gm_ptr) {
        *ub_ptr = *gm_ptr;
    }
}

extern "C" __global__ __aicore__ void ascendc_get_row_f16(
    GM_ADDR input_gm, GM_ADDR indices_gm, GM_ADDR output_gm,
    GM_ADDR input_ne_gm, GM_ADDR input_nb_gm, GM_ADDR indices_ne_gm,
    GM_ADDR indices_nb_gm, GM_ADDR output_ne_gm, GM_ADDR output_nb_gm) {
    int64_t input_ne_ub[4];
    size_t input_nb_ub[4];
    int64_t indices_ne_ub[4];
    size_t indices_nb_ub[4];
    int64_t output_ne_ub[4];
    size_t output_nb_ub[4];

    copy_to_ub(input_ne_gm, input_ne_ub, 32);
    copy_to_ub(input_nb_gm, input_nb_ub, 32);
    copy_to_ub(indices_ne_gm, indices_ne_ub, 32);
    copy_to_ub(indices_nb_gm, indices_nb_ub, 32);
    copy_to_ub(output_ne_gm, output_ne_ub, 32);
    copy_to_ub(output_nb_gm, output_nb_ub, 32);

    GET_ROW_F16 op;
    op.init(input_gm, indices_gm, output_gm, input_ne_ub, input_nb_ub,
            indices_ne_ub, indices_nb_ub, output_ne_ub, output_nb_ub);
    op.calculate();
}
