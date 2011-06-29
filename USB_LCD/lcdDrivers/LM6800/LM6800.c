#include "LM6800.h"

static void LM6800ComputePixelConfigData(uint8_t x, uint8_t y, struct LM6800PixelConfigData *data) __attribute__((always_inline));

void LM6800Init(void)
{
	LM6800_CONTROL_DDR |=
		(1 << LM6800_CS) |
		(1 << LM6800_CSA) |
		(1 << LM6800_CSB) |
		(1 << LM6800_CSC) |
		(1 << LM6800_RESET) |
		(1 << LM6800_RS) |
		(1 << LM6800_RW);

	//set the default state of the ports
	LM6800_CONTROL_PORT = 0x00;
	LM6800_DOUT = 0x00;

	//Apparently setting CSC high disables access to the LCD
	LM6800_CONTROL_PORT |= (1 << LM6800_CSC);

	//reset LCD
	LM6800_CONTROL_PORT |= (1 << LM6800_RESET);
	_delay_ms(1);
	LM6800_CONTROL_PORT &= ~(1 << LM6800_RESET);
	_delay_ms(1);
	LM6800_CONTROL_PORT |= (1 << LM6800_RESET);
	_delay_ms(1);

	//Turn on the display for all chips
	LM6800Write(0,0x3f, LM6800_COMMAND);
	LM6800Write(1,0x3f, LM6800_COMMAND);
	LM6800Write(2,0x3f, LM6800_COMMAND);
	LM6800Write(3,0x3f, LM6800_COMMAND);
	

	//set start line = 0 for all chips
	LM6800Write(0,0xc0, LM6800_COMMAND);
	LM6800Write(1,0xc0, LM6800_COMMAND);
	LM6800Write(2,0xc0, LM6800_COMMAND);
	LM6800Write(3,0xc0, LM6800_COMMAND);

}

static void LM6800ComputePixelConfigData(uint8_t x, uint8_t y, struct LM6800PixelConfigData *data)
{
   //figure out which page and chip we want
   data->page = (y >> 3) & 7;
   data->chip = (x >> 6) & 3;

   //figure out the pixel on each page/chip
   data->column = (x & 63);
   data->pixely = (y & 7);
}


void LM6800ClearPixel(uint8_t x, uint8_t y)
{
	struct LM6800PixelConfigData piConData;
	LM6800ComputePixelConfigData(x, y, &piConData);

	//Set the colum of the chip we want
	LM6800Write(piConData.chip, (1 << 6) | piConData.column, LM6800_COMMAND);
	//set the page we want for that chip
	LM6800Write(piConData.chip, (23 << 3) | piConData.page, LM6800_COMMAND);

	//dummy read
	LM6800Read(piConData.chip);
	uint8_t columnVal = LM6800Read(piConData.chip);

	//set the pixel on the column
	columnVal &= ~(1 << piConData.pixely);

	//Set the colum of the chip we want
	LM6800Write(piConData.chip, (1 << 6) | piConData.column, LM6800_COMMAND);
	//write the column back
	LM6800Write(piConData.chip, columnVal, LM6800_RAM);
}



void LM6800SetPixel(uint8_t x, uint8_t y)
{
	struct LM6800PixelConfigData piConData;
	LM6800ComputePixelConfigData(x, y, &piConData);

	//Set the colum of the chip we want
	LM6800Write(piConData.chip, (1 << 6) | piConData.column, LM6800_COMMAND);
	//set the page we want for that chip
	LM6800Write(piConData.chip, (23 << 3) | piConData.page, LM6800_COMMAND);

	//dummy read
	LM6800Read(piConData.chip);
	uint8_t columnVal = LM6800Read(piConData.chip);

	//set the pixel on the column
	columnVal |= (1 << piConData.pixely);

	//Set the colum of the chip we want
	LM6800Write(piConData.chip, (1 << 6) | piConData.column, LM6800_COMMAND);
	//write the column back
	LM6800Write(piConData.chip, columnVal, LM6800_RAM);
}


uint8_t LM6800GetColumn(uint8_t x, uint8_t y)
{
	struct LM6800PixelConfigData piConData;
	LM6800ComputePixelConfigData(x, y, &piConData);

	//Set the colum of the chip we want
	LM6800Write(piConData.chip, (1 << 6) | piConData.column, LM6800_COMMAND);
	//set the page we want for that chip
	LM6800Write(piConData.chip, (23 << 3) | piConData.page, LM6800_COMMAND);

	LM6800Read(piConData.chip);
	uint8_t columnVal = LM6800Read(piConData.chip);
	return columnVal;
}



uint8_t LM6800Read(uint8_t chip)
{
	uint8_t data;
	LM6800SelectChip(chip);

	while(LM6800ReadStatus(chip) && 0x80);

	//set the data port to input
	LM6800_DDR = 0x00;
	//set the port to 0
	LM6800_DOUT = 0x00;

	//set read mode
	LM6800_CONTROL_PORT |= (1 << LM6800_RS) | (1 << LM6800_RW);

	LM6800_CONFIG_DELAY;

	LM6800_CONTROL_PORT |= (1 << LM6800_CS);
	LM6800_DATA_DELAY;
	data = LM6800_DIN;
	LM6800_CONTROL_PORT &= ~(1 << LM6800_CS);

	return data;
}

uint8_t LM6800ReadStatus(uint8_t chip)
{
	uint8_t data;
	LM6800SelectChip(chip);

	//set the data port to input
	LM6800_DDR = 0x00;
	//set the port to 0
	LM6800_DOUT = 0x00;

	//enter mode to read status
	LM6800_CONTROL_PORT |= (1 << LM6800_RW);
	LM6800_CONTROL_PORT &= ~(1 << LM6800_RS);

	LM6800_CONFIG_DELAY;

	LM6800_CONTROL_PORT |= (1 << LM6800_CS);
	LM6800_DATA_DELAY;
	//we may need another nop here
	data = LM6800_DIN;
	LM6800_CONTROL_PORT &= ~(1 << LM6800_CS);

	return data;
}


void LM6800Write(uint8_t chip, uint8_t data, enum LM6800_WRITE_MODE writeMode)
{
	LM6800SelectChip(chip);

	//Wait for the status
	while(LM6800ReadStatus(chip) & 0x80);

	//set data port to output
	LM6800_DDR = 0xFF;
	//set the data on the port
	LM6800_DOUT = data;

	if(writeMode == LM6800_RAM)
	{
		//select display ram
		LM6800_CONTROL_PORT |= (1 << LM6800_RS);
	}
	else if(writeMode == LM6800_COMMAND)
	{
		//select display command
		LM6800_CONTROL_PORT &= ~(1 << LM6800_RS);
	}

	//select write mode
	LM6800_CONTROL_PORT &= ~(1 << LM6800_RW);

	LM6800_CONFIG_DELAY;

	//Raise the control select pin (sometimes called E)
	LM6800_CONTROL_PORT |= (1 << LM6800_CS);
	LM6800_DATA_DELAY;
	LM6800_CONTROL_PORT &= ~(1 << LM6800_CS);
}


void LM6800WriteBlock(uint8_t chip, uint8_t page, char *data)
{
	//Set our column to the beginning
	LM6800Write(chip, (1 << 6), LM6800_COMMAND);
	//set the page we want for that chip
	LM6800Write(chip, (23 << 3) | page, LM6800_COMMAND);

	//Dump a block of data to the page
	for(uint8_t col = 0; col < LM6800_COLUMNS_PER_PAGE; col ++)
	{
		LM6800Write(chip, data[col], LM6800_RAM);
	}
}

void LM6800SelectChip(uint8_t chip)
{
	switch(chip)
	{
		case 0:
			LM6800_CONTROL_PORT &= ~((1 << LM6800_CSA) | (1 << LM6800_CSB) | (1 << LM6800_CSC));
			break;
		case 1:
			LM6800_CONTROL_PORT &= ~((1 << LM6800_CSA) | (1 << LM6800_CSB) | (1 << LM6800_CSC));
			LM6800_CONTROL_PORT |= (1 << LM6800_CSA);
			break;
		case 2:
			LM6800_CONTROL_PORT &= ~((1 << LM6800_CSA) | (1 << LM6800_CSB) | (1 << LM6800_CSC));
			LM6800_CONTROL_PORT |= (1 << LM6800_CSB);
			break;
		case 3:
			LM6800_CONTROL_PORT &= ~((1 << LM6800_CSA) | (1 << LM6800_CSB) | (1 << LM6800_CSC));
			LM6800_CONTROL_PORT |= ((1 << LM6800_CSA) | (1 << LM6800_CSB));
			break;
		default:
			LM6800_CONTROL_PORT &= ~((1 << LM6800_CSA) | (1 << LM6800_CSB) | (1 << LM6800_CSC));
			break;
	}
}