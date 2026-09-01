/**
 * @file shell_io_cdc.h
 * @brief boot_app USB CDC transport adapter for Banux Shell.
 */
#ifndef __SHELL_IO_CDC_H__
#define __SHELL_IO_CDC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bg_shell.h"

const ShellIO_t *ShellIO_CDC_Get(void);

#ifdef __cplusplus
}
#endif

#endif /* __SHELL_IO_CDC_H__ */
