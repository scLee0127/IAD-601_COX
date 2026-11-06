#ifndef __MYFLASH_H__
#define __MYFLASH_H__

#include "stm32f0xx_hal.h"

// #define FLASH_BASE_ADDR         ((uint32_t)0x08000000U)
// #define FLASH_SIZE_BYTES        ((uint32_t)(64 * 1024U))  // 64KB
// #define FLASH_PAGE_SIZE         ((uint32_t)0x400U)        // 1KB
// #define FLASH_PAGE_COUNT        ((uint32_t)64U)

// #define FLASH_PAGE_ADDR(n)      (FLASH_BASE_ADDR + (uint32_t)(n) * FLASH_PAGE_SIZE)

// #define FLASH_USER_PAGE_INDEX       (63U)
// #define FLASH_USER_START_ADDR       FLASH_PAGE_ADDR(FLASH_USER_PAGE_INDEX)
// #define FLASH_USER_END_ADDR         (FLASH_USER_START_ADDR + FLASH_PAGE_SIZE)  // 끝 + 1 주소

// #define FLASH_USER_PAGES_COUNT      (4U)
// #define FLASH_USER_START_PAGE_INDEX (FLASH_PAGE_COUNT - FLASH_USER_PAGES_COUNT)  // 64 - 4 = 60

// #define FLASH_USER_START_ADDR       FLASH_PAGE_ADDR(FLASH_USER_START_PAGE_INDEX) // Page 60 시작
// #define FLASH_USER_END_ADDR         (FLASH_BASE_ADDR + FLASH_SIZE_BYTES) 

#define FLASH_USER_START_ADDR   ((uint32_t)0x0800FC00U)
#define FLASH_USER_PAGE_SIZE    ((uint32_t)0x400U)
#define FLASH_USER_END_ADDR     ((uint32_t)0x08010000U)

HAL_StatusTypeDef Flash_EraseUserPage(void);
HAL_StatusTypeDef Flash_WriteHalfWords(uint32_t startAddr, const uint16_t *pData, uint32_t halfwordCount);
void Flash_ReadHalfWords(uint32_t startAddr, uint16_t *pData, uint32_t halfwordCount);
#endif // __MYFLASH_H__