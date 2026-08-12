/**
 * @file  sys_nv.h
 * @brief Lightweight system NVM for boot_app (power-loss retain).
 *
 * Owns persisted settings under the `sys` component. First consumer: CDC log
 * (global enable + per-module mask). Expand the blob carefully with version bumps.
 */
#ifndef __SYS_NV_H__
#define __SYS_NV_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 2 MB flash: sector immediately before partition-flag (0x1FF000).
 * Keep this outside the firmware image and clear of PartFlag.
 */
#ifndef SYS_NV_FLASH_ADDR
#define SYS_NV_FLASH_ADDR   0x001FE000UL
#endif
#define SYS_NV_FLASH_SIZE   0x1000UL

#define SYS_NV_MAGIC        0x42535953u  /* 'BSYS' */
#define SYS_NV_VERSION      0x0001u

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint8_t  log_global;     /* 0/1 — CdcDbg global */
    uint8_t  log_mod_mask;   /* bit i = module i enabled */
    uint8_t  reserved[14];
    uint32_t crc32;          /* checksum of bytes before this field */
} SysNv_t;

/**
 * Load NVM from flash into RAM only — do NOT enable CDC log output yet.
 * Call SysNv_ApplyLogDeferred() after USB CDC is up (and preferably DTR).
 */
int SysNv_Init(void);

/** Persist current runtime settings (log, …) to flash. Returns 0 on success. */
int SysNv_Save(void);

/** Pull log settings from CdcDbg into RAM blob and save. */
int SysNv_SaveLog(void);

/** Apply RAM blob log fields to CdcDbg immediately (no flash I/O). */
void SysNv_ApplyLog(void);

/**
 * Apply persisted log settings once CDC is ready.
 * Safe to call repeatedly from the USB task; no-ops after first apply.
 */
void SysNv_ApplyLogDeferred(void);

const SysNv_t *SysNv_Get(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYS_NV_H__ */
