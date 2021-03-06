#pragma once

#include "srsvm/platform-detection.h"

#if defined(SRSVM_BUILD_TARGET_LINUX)

#ifndef PREFIX
#error "PREFIX not defined"
#endif

#define SRSVM_LIBEXEC_DIR PREFIX "/libexec/srsvm"
#define SRSVM_LIB_DIR PREFIX "/lib/srsvm"

#define SRSVM_USER_HOME_LIBEXEC_DIR ".local/libexec/srsvm"
#define SRSVM_USER_HOME_LIB_DIR ".local/lib/srsvm"

#endif

#define SRSVM_MOD_PATH_ENV_NAME "SRSVM_MOD_PATH"