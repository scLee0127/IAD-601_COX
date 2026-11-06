#include "global.h"
#include "myflash.h"

/////////////////////////////////////////////////////////////////////////////////////////////
// import function prototype


/////////////////////////////////////////////////////////////////////////////////////////////
// local function prototype

/////////////////////////////////////////////////////////////////////////////////////////////
// global variable

/////////////////////////////////////////////////////////////////////////////////////////////
// public and export function implement
/* 내부 헬퍼: 주소 범위 체크 */
static HAL_StatusTypeDef FLASH_CheckRange(uint32_t startAddr, uint32_t lengthBytes)
{
    if (startAddr < FLASH_USER_START_ADDR) return HAL_ERROR;
    if ((startAddr + lengthBytes) > FLASH_USER_END_ADDR) return HAL_ERROR;
    return HAL_OK;
}

/* 1. 사용자 페이지(1KB) 전체 Erase */
HAL_StatusTypeDef Flash_EraseUserPage(void)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t pageError = 0;

    /* Flash 언락 */
    HAL_FLASH_Unlock();

    /* 에러 플래그 클리어 */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR);

    eraseInit.TypeErase   = FLASH_TYPEERASE_PAGES;
    eraseInit.PageAddress = FLASH_USER_START_ADDR;
    eraseInit.NbPages     = 1;

    status = HAL_FLASHEx_Erase(&eraseInit, &pageError);

    /* Flash 락 */
    HAL_FLASH_Lock();

    if (status != HAL_OK || pageError != 0xFFFFFFFFU)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/* 2. Half-Word(16bit) 단위로 쓰기
 *   - startAddr: FLASH_USER_START_ADDR ~ FLASH_USER_END_ADDR-2
 *   - pData: 기록할 데이터 배열
 *   - halfwordCount: Halfword 개수 (uint16_t 개수)
 */
HAL_StatusTypeDef Flash_WriteHalfWords(uint32_t startAddr, const uint16_t *pData, uint32_t halfwordCount)
{
    HAL_StatusTypeDef status = HAL_OK;
    uint32_t addr = startAddr;
    uint32_t i;

    /* 2바이트 정렬 체크 */
    if ((startAddr % 2U) != 0U)
    {
        return HAL_ERROR; // Halfword는 2바이트 정렬 필요
    }

    /* 범위 체크 */
    if (FLASH_CheckRange(startAddr, halfwordCount * 2U) != HAL_OK)
    {
        return HAL_ERROR;
    }

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR);

    for (i = 0; i < halfwordCount; i++)
    {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr, pData[i]);
        if (status != HAL_OK)
        {
            break;
        }

        /* 확인(read-after-write) – 옵션. 필요 없으면 주석 처리 */
        if (*((__IO uint16_t *)addr) != pData[i])
        {
            status = HAL_ERROR;
            break;
        }

        addr += 2U;
    }

    HAL_FLASH_Lock();

    return status;
}

/* 3. Half-Word(16bit) 단위로 읽기 */
void Flash_ReadHalfWords(uint32_t startAddr, uint16_t *pData, uint32_t halfwordCount)
{
    uint32_t addr = startAddr;
    uint32_t i;

    for (i = 0; i < halfwordCount; i++)
    {
        pData[i] = *(__IO uint16_t *)addr;
        addr += 2U;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// local function implement
