# Needs "pip install -U openai-whisper"
from whisper.timing import dtw_cpu
import numpy as np
from ctypes import *
import sys

# Load whisper.cpp
if len(sys.argv) != 2:
    print("Usage: python test-dtw <PATH_TO_LIBWHISPER.SO>")
wcpp = CDLL(sys.argv[1])

# Generate test data
np.random.seed(0)
IN_DINS=[(1,1), (5,5,), (50, 200), (200, 1500), (1500, 200), (200, 50), (1,250), (250, 1)]
pairs=[]
for d in IN_DINS:
    x = np.random.standard_normal((d[0], d[1])).astype('float32')
    dtw = dtw_cpu(x)
    pairs.append((x,dtw))

# Run whisper.cpp dtw
for idx, p in enumerate(pairs):
    print("Running test {}...".format(idx), file=sys.stderr, end="")

    # Prepare types
    in_size = IN_DINS[idx][0]*IN_DINS[idx][1]
    in_type = c_float * in_size
    out_type = POINTER(POINTER(c_int32))
    out_size_type = POINTER(c_size_t)

    wcpp_test_dtw = wcpp.whisper_test_dtw
    wcpp_test_dtw.argtypes = (in_type, c_size_t, c_size_t, out_type, out_size_type, out_size_type)
    wcpp_test_dtw.restype = None

    # Create args as ctypes
    in_data_py = p[0].flatten().tolist()
    in_data = in_type(*in_data_py)
    out = POINTER(c_int32)()
    out_ne0 = c_size_t()
    out_ne1 = c_size_t()

    # Call whisper_test_dtw, retrieve output
    wcpp_test_dtw(in_data, IN_DINS[idx][0], IN_DINS[idx][1], byref(out), byref(out_ne0), byref(out_ne1))
    out_np = np.empty((out_ne0.value, out_ne1.value), dtype=np.int32)
    for i in range (0, out_ne0.value):
        for j in range(0, out_ne1.value):
            out_np[i][j] = out[j + i*out_ne1.value]

    # Test
    if (np.array_equal(out_np, p[1])):
        print(" OK!", file=sys.stderr)
    else:
        print(" Failed!", file=sys.stderr)
