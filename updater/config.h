#pragma once

// parameters can be retrieved from Keil C51 definition files (*.opt, *.gpt) inside UV4 folder
#define CHIP_TYPE			2
#define CHIP_FLASH_SIZE		32768
#define CHIP_SECTOR_SIZE	1024

#if CHIP_TYPE == 4
#define CHIP_FLASH_SIZE_MAX 1048576
#elif CHIP_TYPE == 7
#define CHIP_FLASH_SIZE_MAX 131072
#else
#define CHIP_FLASH_SIZE_MAX 65536
#endif

#if CHIP_FLASH_SIZE > CHIP_FLASH_SIZE_MAX
#error Chip flash size is not valid for this chip type
#endif
