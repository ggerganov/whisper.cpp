#include <stdio.h>

#include <dlfcn.h>

#include <cuda.h>


typedef CUresult (*cuDeviceGet_pt)(CUdevice *device, int ordinal);
typedef CUresult (*cuDeviceGetAttribute_pt)(int *pi, CUdevice_attribute attrib, CUdevice dev);
typedef CUresult (*cuGetErrorString_pt)(CUresult error, const char **pStr);
typedef CUresult (*cuMemGetAllocationGranularity_pt)(size_t *granularity, const CUmemAllocationProp *prop, CUmemAllocationGranularity_flags option);
typedef CUresult (*cuMemCreate_pt)(CUmemGenericAllocationHandle *handle, size_t size, const CUmemAllocationProp *prop, unsigned long long flags);
typedef CUresult (*cuMemAddressReserve_pt)(CUdeviceptr *ptr, size_t size, size_t alignment, CUdeviceptr addr, unsigned long long flags);
typedef CUresult (*cuMemAddressFree_pt)(CUdeviceptr ptr, size_t size);
typedef CUresult (*cuMemMap_pt)(CUdeviceptr ptr, size_t size, size_t offset, CUmemGenericAllocationHandle handle, unsigned long long flags);
typedef CUresult (*cuMemUnmap_pt)(CUdeviceptr ptr, size_t size);
typedef CUresult (*cuMemRelease_pt)(CUmemGenericAllocationHandle handle);
typedef CUresult (*cuMemSetAccess_pt)(CUdeviceptr ptr, size_t size, const CUmemAccessDesc *desc, size_t count);


cuDeviceGet_pt _cuDeviceGet = NULL;
cuDeviceGetAttribute_pt _cuDeviceGetAttribute = NULL;
cuGetErrorString_pt _cuGetErrorString = NULL;
cuMemGetAllocationGranularity_pt _cuMemGetAllocationGranularity = NULL;
cuMemCreate_pt _cuMemCreate = NULL;
cuMemAddressReserve_pt _cuMemAddressReserve = NULL;
cuMemAddressFree_pt _cuMemAddressFree = NULL;
cuMemMap_pt _cuMemMap = NULL;
cuMemUnmap_pt _cuMemUnmap = NULL;
cuMemRelease_pt _cuMemRelease = NULL;
cuMemSetAccess_pt _cuMemSetAccess = NULL;


int load_libcuda(void) {

    static void * libcuda = NULL;

    if (libcuda == (void*)1)
        return 0;

    if (libcuda != NULL)
        return 1;

    libcuda = dlopen("libcuda.so", RTLD_NOW);

    if (libcuda == NULL) {
        libcuda = dlopen("libcuda.so.1", RTLD_NOW);
    }

    if (libcuda != NULL) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        _cuDeviceGet = (cuDeviceGet_pt)dlsym(libcuda, "cuDeviceGet");
        _cuDeviceGetAttribute = (cuDeviceGetAttribute_pt)dlsym(libcuda, "cuDeviceGetAttribute");
        _cuGetErrorString = (cuGetErrorString_pt)dlsym(libcuda, "cuGetErrorString");
        _cuMemGetAllocationGranularity = (cuMemGetAllocationGranularity_pt)dlsym(libcuda, "cuMemGetAllocationGranularity");
        _cuMemCreate = (cuMemCreate_pt)dlsym(libcuda, "cuMemCreate");
        _cuMemAddressReserve = (cuMemAddressReserve_pt)dlsym(libcuda, "cuMemAddressReserve");
        _cuMemAddressFree = (cuMemAddressFree_pt)dlsym(libcuda, "cuMemAddressFree");
        _cuMemMap = (cuMemMap_pt)dlsym(libcuda, "cuMemMap");
        _cuMemUnmap = (cuMemUnmap_pt)dlsym(libcuda, "cuMemUnmap");
        _cuMemRelease = (cuMemRelease_pt)dlsym(libcuda, "cuMemRelease");
        _cuMemSetAccess = (cuMemSetAccess_pt)dlsym(libcuda, "cuMemSetAccess");
#pragma GCC diagnostic pop

        return 1;
    }

    fprintf(stderr, "error: failed to load libcuda.so: %s\n", dlerror());

    libcuda = (void*)1;   // tried and failed
    return 0;
}


CUresult CUDAAPI cuDeviceGet(CUdevice *device, int ordinal) {
    if (_cuDeviceGet == NULL && !load_libcuda())
        return CUDA_ERROR_SHARED_OBJECT_INIT_FAILED;
    if (_cuDeviceGet == NULL)
        return CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND;
    return _cuDeviceGet(device, ordinal);
}

CUresult CUDAAPI cuDeviceGetAttribute(int *pi, CUdevice_attribute attrib, CUdevice dev) {
    if (_cuDeviceGetAttribute == NULL && !load_libcuda())
        return CUDA_ERROR_SHARED_OBJECT_INIT_FAILED;
    if (_cuDeviceGetAttribute == NULL)
        return CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND;
    return _cuDeviceGetAttribute(pi, attrib, dev);
}

CUresult CUDAAPI cuGetErrorString(CUresult error, const char **pStr) {
    if (_cuGetErrorString == NULL && !load_libcuda())
        return CUDA_ERROR_SHARED_OBJECT_INIT_FAILED;
    if (_cuGetErrorString == NULL)
        return CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND;
    return _cuGetErrorString(error, pStr);
}

CUresult CUDAAPI cuMemGetAllocationGranularity(size_t *granularity, const CUmemAllocationProp *prop, CUmemAllocationGranularity_flags option) {
    if (_cuMemGetAllocationGranularity == NULL && !load_libcuda())
        return CUDA_ERROR_SHARED_OBJECT_INIT_FAILED;
    if (_cuMemGetAllocationGranularity == NULL)
        return CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND;
    return _cuMemGetAllocationGranularity(granularity, prop, option);
}

CUresult CUDAAPI cuMemCreate(CUmemGenericAllocationHandle *handle, size_t size, const CUmemAllocationProp *prop, unsigned long long flags) {
    if (_cuMemCreate == NULL && !load_libcuda())
        return CUDA_ERROR_SHARED_OBJECT_INIT_FAILED;
    if (_cuMemCreate == NULL)
        return CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND;
    return _cuMemCreate(handle, size, prop, flags);
}

CUresult CUDAAPI cuMemAddressReserve(CUdeviceptr *ptr, size_t size, size_t alignment, CUdeviceptr addr, unsigned long long flags) {
    if (_cuMemAddressReserve == NULL && !load_libcuda())
        return CUDA_ERROR_SHARED_OBJECT_INIT_FAILED;
    if (_cuMemAddressReserve == NULL)
        return CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND;
    return _cuMemAddressReserve(ptr, size, alignment, addr, flags);
}

CUresult CUDAAPI cuMemAddressFree(CUdeviceptr ptr, size_t size) {
    if (_cuMemAddressFree == NULL && !load_libcuda())
        return CUDA_ERROR_SHARED_OBJECT_INIT_FAILED;
    if (_cuMemAddressFree == NULL)
        return CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND;
    return _cuMemAddressFree(ptr, size);
}

CUresult CUDAAPI cuMemMap(CUdeviceptr ptr, size_t size, size_t offset, CUmemGenericAllocationHandle handle, unsigned long long flags) {
    if (_cuMemMap == NULL && !load_libcuda())
        return CUDA_ERROR_SHARED_OBJECT_INIT_FAILED;
    if (_cuMemMap == NULL)
        return CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND;
    return _cuMemMap(ptr, size, offset, handle, flags);
}

CUresult CUDAAPI cuMemUnmap(CUdeviceptr ptr, size_t size) {
    if (_cuMemUnmap == NULL && !load_libcuda())
        return CUDA_ERROR_SHARED_OBJECT_INIT_FAILED;
    if (_cuMemUnmap == NULL)
        return CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND;
    return _cuMemUnmap(ptr, size);
}

CUresult CUDAAPI cuMemRelease(CUmemGenericAllocationHandle handle) {
    if (_cuMemRelease == NULL && !load_libcuda())
        return CUDA_ERROR_SHARED_OBJECT_INIT_FAILED;
    if (_cuMemRelease == NULL)
        return CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND;
    return _cuMemRelease(handle);
}

CUresult CUDAAPI cuMemSetAccess(CUdeviceptr ptr, size_t size, const CUmemAccessDesc *desc, size_t count) {
    if (_cuMemSetAccess == NULL && !load_libcuda())
        return CUDA_ERROR_SHARED_OBJECT_INIT_FAILED;
    if (_cuMemSetAccess == NULL)
        return CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND;
    return _cuMemSetAccess(ptr, size, desc, count);
}
