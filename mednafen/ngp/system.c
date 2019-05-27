#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "flash.h"
#include "system.h"
#include "rom.h"

#include "../general.h"

uint_fast8_t system_comms_read(uint8_t* buffer)
{
   return 0;
}

uint_fast8_t system_comms_poll(uint8_t* buffer)
{
   return 0;
}

void system_comms_write(uint8_t data)
{
}

uint_fast8_t system_io_flash_read(uint8_t* buffer, uint32_t bufferLength)
{
	FILE* fp;
	fp = fopen("./game.sav", "rb");
	
	if (!fp) return 0;
	
	fread(&buffer, sizeof(uint8_t), bufferLength, fp);
	fclose(fp);
	
	return 1;
}

uint_fast8_t system_io_flash_write(uint8_t *buffer, uint32_t bufferLength)
{
	FILE* fp;
	fp = fopen("./game.sav", "wb");
	if (!fp) return 0;
	
	fwrite(&buffer, sizeof(uint8_t), bufferLength, fp);
	fclose(fp);
	
	return 1;
}

void NGP_FLASHSaveState(uint_fast8_t load, FILE* fp)
{
	int32_t FlashLength = 0;
	uint8_t *flashdata = NULL;
	
	/* Load state */
	if (load == 1)
	{
		fread(&FlashLength, sizeof(uint8_t), sizeof(FlashLength), fp);
	}
	/* Save State */
	else
	{
		flashdata = make_flash_commit(&FlashLength);
		fwrite(&FlashLength, sizeof(uint8_t), sizeof(FlashLength), fp);
	}
	
	if (!FlashLength) // No flash data to save, OR no flash data to load.
	{
		if(flashdata) free(flashdata);
		return;
	}
	
	/* Load state */
	if (load == 1)
	{
		flashdata = (uint8_t *)malloc(FlashLength);
		fread(&flashdata, sizeof(uint8_t), sizeof(flashdata), fp);
		
		memcpy(ngpc_rom.data, ngpc_rom.orig_data, ngpc_rom.length);
		do_flash_read(flashdata);
	}
	/* Save State */
	else
	{
		fwrite(&flashdata, sizeof(uint8_t), sizeof(flashdata), fp);
	}
	
	free(flashdata);
}
