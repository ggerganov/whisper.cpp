#include "ggml-backend-impl.h"
#include "ggml-backend.h"
#include "ggml-impl.h"
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <windows.h>
#elif defined(__APPLE__)
#    include <mach-o/dyld.h>
#    include <dlfcn.h>
#else
#    include <dlfcn.h>
#    include <unistd.h>
#endif

// Backend registry
#ifdef GGML_USE_CPU
#include "ggml-cpu.h"
#endif

#ifdef GGML_USE_CUDA
#include "ggml-cuda.h"
#endif

#ifdef GGML_USE_METAL
#include "ggml-metal.h"
#endif

#ifdef GGML_USE_SYCL
#include "ggml-sycl.h"
#endif

#ifdef GGML_USE_VULKAN
#include "ggml-vulkan.h"
#endif

#ifdef GGML_USE_BLAS
#include "ggml-blas.h"
#endif

#ifdef GGML_USE_RPC
#include "ggml-rpc.h"
#endif

#ifdef GGML_USE_AMX
#  include "ggml-amx.h"
#endif

#ifdef GGML_USE_CANN
#include "ggml-cann.h"
#endif

#ifdef GGML_USE_KOMPUTE
#include "ggml-kompute.h"
#endif

struct ggml_backend_reg_entry {
    ggml_backend_reg_t reg;
    void * handle;
};

struct ggml_backend_registry {
    std::vector<ggml_backend_reg_entry> backends;
    std::vector<ggml_backend_dev_t> devices;

    ggml_backend_registry() {
#ifdef GGML_USE_CUDA
        register_backend(ggml_backend_cuda_reg());
#endif
#ifdef GGML_USE_METAL
        register_backend(ggml_backend_metal_reg());
#endif
#ifdef GGML_USE_SYCL
        register_backend(ggml_backend_sycl_reg());
#endif
#ifdef GGML_USE_VULKAN
        register_backend(ggml_backend_vk_reg());
#endif
#ifdef GGML_USE_CANN
        register_backend(ggml_backend_cann_reg());
#endif
#ifdef GGML_USE_BLAS
        register_backend(ggml_backend_blas_reg());
#endif
#ifdef GGML_USE_RPC
        register_backend(ggml_backend_rpc_reg());
#endif
#ifdef GGML_USE_AMX
        register_backend(ggml_backend_amx_reg());
#endif
#ifdef GGML_USE_KOMPUTE
        register_backend(ggml_backend_kompute_reg());
#endif
#ifdef GGML_USE_CPU
        register_backend(ggml_backend_cpu_reg());
#endif
    }

    ~ggml_backend_registry() {
        while (!backends.empty()) {
            // use silent since the log system may have been destroyed at this point
            unload_backend(backends.back().reg, true);
        }
    }

    void register_backend(ggml_backend_reg_t reg, void * handle = nullptr) {
        if (!reg) {
            return;
        }

#ifndef NDEBUG
        GGML_LOG_DEBUG("%s: registered backend %s (%zu devices)\n",
            __func__, ggml_backend_reg_name(reg), ggml_backend_reg_dev_count(reg));
#endif
        backends.push_back({ reg, handle });
        for (size_t i = 0; i < ggml_backend_reg_dev_count(reg); i++) {
            register_device(ggml_backend_reg_dev_get(reg, i));
        }
    }

    void register_device(ggml_backend_dev_t device) {
#ifndef NDEBUG
        GGML_LOG_DEBUG("%s: registered device %s (%s)\n", __func__, ggml_backend_dev_name(device), ggml_backend_dev_description(device));
#endif
        devices.push_back(device);
    }

    ggml_backend_reg_t load_backend(const char * path, bool silent) {
#ifdef _WIN32
        // suppress error dialogs for missing DLLs
        DWORD old_mode = SetErrorMode(SEM_FAILCRITICALERRORS);
        SetErrorMode(old_mode | SEM_FAILCRITICALERRORS);

        HMODULE handle = LoadLibraryA(path);

        if (!handle) {
            if (!silent) {
                GGML_LOG_ERROR("%s: failed to load %s: %lu\n", __func__, path, GetLastError());
            }
            SetErrorMode(old_mode);
            return nullptr;
        }

        ggml_backend_init_t backend_init = (ggml_backend_init_t) GetProcAddress(handle, "ggml_backend_init");

        SetErrorMode(old_mode);

        if (!backend_init) {
            if (!silent) {
                GGML_LOG_ERROR("%s: failed to find ggml_backend_init in %s: %lu\n", __func__, path, GetLastError());
            }
            FreeLibrary(handle);
            return nullptr;
        }
#else
        void * handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);

        if (!handle) {
            if (!silent) {
                GGML_LOG_ERROR("%s: failed to load %s: %s\n", __func__, path, dlerror());
            }
            return nullptr;
        }

        auto * backend_init = (ggml_backend_init_t) dlsym(handle, "ggml_backend_init");

        if (!backend_init) {
            if (!silent) {
                GGML_LOG_ERROR("%s: failed to find ggml_backend_init in %s: %s\n", __func__, path, dlerror());
            }
            dlclose(handle);
            return nullptr;
        }
#endif
        ggml_backend_reg_t reg = backend_init();

        if (!reg || reg->api_version != GGML_BACKEND_API_VERSION) {
            if (!silent) {
                if (!reg) {
                    GGML_LOG_ERROR("%s: failed to initialize backend from %s: ggml_backend_init returned NULL\n", __func__, path);
                } else {
                    GGML_LOG_ERROR("%s: failed to initialize backend from %s: incompatible API version (backend: %d, current: %d)\n",
                                   __func__, path, reg->api_version, GGML_BACKEND_API_VERSION);
                }
            }
#ifdef _WIN32
            FreeLibrary(handle);
#else
            dlclose(handle);
#endif
            return nullptr;
        }

        GGML_LOG_INFO("%s: loaded %s backend from %s\n", __func__, ggml_backend_reg_name(reg), path);
        register_backend(reg, handle);
        return reg;
    }

    void unload_backend(ggml_backend_reg_t reg, bool silent) {
        auto it = std::find_if(backends.begin(), backends.end(),
                                [reg](ggml_backend_reg_entry entry) { return entry.reg == reg; });

        if (it == backends.end()) {
            if (!silent) {
                GGML_LOG_ERROR("%s: backend not found\n", __func__);
            }
            return;
        }

        if (!silent) {
            GGML_LOG_DEBUG("%s: unloading %s backend\n", __func__, ggml_backend_reg_name(reg));
        }

        // remove devices
        devices.erase(
            std::remove_if(devices.begin(), devices.end(),
                            [reg](ggml_backend_dev_t dev) { return ggml_backend_dev_backend_reg(dev) == reg; }),
            devices.end());

        // unload library
        if (it->handle) {
#ifdef _WIN32
            FreeLibrary((HMODULE) it->handle);
#else
            dlclose(it->handle);
#endif
        }

        // remove backend
        backends.erase(it);
    }
};

static ggml_backend_registry & get_reg() {
    static ggml_backend_registry reg;
    return reg;
}

// Internal API
void ggml_backend_register(ggml_backend_reg_t reg) {
    get_reg().register_backend(reg);
}

void ggml_backend_device_register(ggml_backend_dev_t device) {
    get_reg().register_device(device);
}

// Backend (reg) enumeration
static bool striequals(const char * a, const char * b) {
    for (; *a && *b; a++, b++) {
        if (std::tolower(*a) != std::tolower(*b)) {
            return false;
        }
    }
    return *a == *b;
}

size_t ggml_backend_reg_count() {
    return get_reg().backends.size();
}

ggml_backend_reg_t ggml_backend_reg_get(size_t index) {
    GGML_ASSERT(index < ggml_backend_reg_count());
    return get_reg().backends[index].reg;
}

ggml_backend_reg_t ggml_backend_reg_by_name(const char * name) {
    for (size_t i = 0; i < ggml_backend_reg_count(); i++) {
        ggml_backend_reg_t reg = ggml_backend_reg_get(i);
        if (striequals(ggml_backend_reg_name(reg), name)) {
            return reg;
        }
    }
    return nullptr;
}

// Device enumeration
size_t ggml_backend_dev_count() {
    return get_reg().devices.size();
}

ggml_backend_dev_t ggml_backend_dev_get(size_t index) {
    GGML_ASSERT(index < ggml_backend_dev_count());
    return get_reg().devices[index];
}

ggml_backend_dev_t ggml_backend_dev_by_name(const char * name) {
    for (size_t i = 0; i < ggml_backend_dev_count(); i++) {
        ggml_backend_dev_t dev = ggml_backend_dev_get(i);
        if (striequals(ggml_backend_dev_name(dev), name)) {
            return dev;
        }
    }
    return nullptr;
}

ggml_backend_dev_t ggml_backend_dev_by_type(enum ggml_backend_dev_type type) {
    for (size_t i = 0; i < ggml_backend_dev_count(); i++) {
        ggml_backend_dev_t dev = ggml_backend_dev_get(i);
        if (ggml_backend_dev_type(dev) == type) {
            return dev;
        }
    }
    return nullptr;
}

// Convenience functions
ggml_backend_t ggml_backend_init_by_name(const char * name, const char * params) {
    ggml_backend_dev_t dev = ggml_backend_dev_by_name(name);
    if (!dev) {
        return nullptr;
    }
    return ggml_backend_dev_init(dev, params);
}

ggml_backend_t ggml_backend_init_by_type(enum ggml_backend_dev_type type, const char * params) {
    ggml_backend_dev_t dev = ggml_backend_dev_by_type(type);
    if (!dev) {
        return nullptr;
    }
    return ggml_backend_dev_init(dev, params);
}

ggml_backend_t ggml_backend_init_best(void) {
    ggml_backend_dev_t dev = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_GPU);
    if (!dev) {
        dev = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_CPU);
    }
    if (!dev) {
        return nullptr;
    }
    return ggml_backend_dev_init(dev, nullptr);
}

// Dynamic loading
ggml_backend_reg_t ggml_backend_load(const char * path) {
    return get_reg().load_backend(path, false);
}

void ggml_backend_unload(ggml_backend_reg_t reg) {
    get_reg().unload_backend(reg, true);
}

void ggml_backend_load_all() {
    std::vector<std::string> search_prefix;

    // add the executable directory to the search path
    // FIXME: this is convenient for development, but it should probably be disabled in production

#if defined(__APPLE__)
    // get executable path
    std::vector<char> path;
    uint32_t size;
    while (true) {
        size = path.size();
        if (_NSGetExecutablePath(path.data(), &size) == 0) {
            break;
        }
        path.resize(size);
    }
    std::string base_path(path.data(), size);
    // remove executable name
    auto last_slash = base_path.find_last_of('/');
    if (last_slash != std::string::npos) {
        base_path = base_path.substr(0, last_slash);
    }
    search_prefix.push_back(base_path + "/");
#elif defined(__linux__)
    std::string base_path = ".";
    std::vector<char> path(1024);
    while (true) {
        // get executable path
        ssize_t len = readlink("/proc/self/exe", path.data(), path.size());
        if (len == -1) {
            break;
        }
        if (len < (ssize_t) path.size()) {
            base_path = std::string(path.data(), len);
            // remove executable name
            auto last_slash = base_path.find_last_of('/');
            if (last_slash != std::string::npos) {
                base_path = base_path.substr(0, last_slash);
            }
            break;
        }
        path.resize(path.size() * 2);
    }

    search_prefix.push_back(base_path + "/");
#endif

    auto & reg = get_reg();

    auto try_load = [&](const std::string & name) {
        std::string os_name;
#ifdef _WIN32
        os_name = "ggml-" + name + ".dll";
#else
        os_name = "libggml-" + name + ".so";
#endif
        if (reg.load_backend(os_name.c_str(), true)) {
            return;
        }
        for (const auto & prefix : search_prefix) {
            if (reg.load_backend((prefix + os_name).c_str(), true)) {
                return;
            }
        }
    };

    try_load("amx");
    try_load("blas");
    try_load("cann");
    try_load("cuda");
    try_load("hip");
    try_load("kompute");
    try_load("metal");
    try_load("rpc");
    try_load("sycl");
    try_load("vulkan");
    try_load("musa");
    try_load("cpu");
}
