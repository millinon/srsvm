#pragma once

#include <pthread.h>

#define SRSVM_MODULE_FILE_EXTENSION ".svmmod"

#if defined(SRSVM_INTERNAL)
#define SRSVM_EXPORT __attribute__((visibility("default"))) 
#else
#define SRSVM_EXPORT extern
#endif

#define SRSVM_PROGRAM_SUPPORT_SHEBANG
#define SRSVM_MAX_PATH_LEN PATH_MAX

#define srsvm_strncpy(dst,src,len) strncpy(dst,src,len)

typedef pthread_mutex_t srsvm_lock;

typedef pthread_t srsvm_thread_native_handle;

typedef void* srsvm_native_module_handle;
