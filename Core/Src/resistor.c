/*
 * resistor.c
 *
 *  Created on: Jun 19, 2024
 *      Author: Rafael Reyes
 */

#include "main.h"
#include "stdio.h"
#include "resistor.h"
#include "i2c-lcd.h"
#include "math.h"


#define R_MUX 3.3


extern ADC_HandleTypeDef hadc1;
extern R_paramTypeDef R_config;


void resistor_measure(void);
double resistor_MUX(void);
void resistor_GPIO(void);
double r_standard(double index);


/**************************************************
Brief: Reads the ADC output and converts it into the value of the measured resistor in ohms
Return: Resistance in Ohms
***************************************************/
double resisor_value(double decade)
{

	double raw = R_config.ADC;
	double decade_resistor = decade * R_MUX;
	double ADC_ratio;
	double resistor_value;

	ADC_ratio = ((double)raw/4096); // percentage of total voltage

	//R_config.r_measured = ((decade)/ADC_ratio)-(decade); // Known resistor tied to GND

	resistor_value = (ADC_ratio * decade_resistor)/(1 - ADC_ratio); // Known resistor tied to VCC 3.3V

 	if(decade <= 1)
		resistor_value = (ADC_ratio * 10 * R_MUX)/(1 - ADC_ratio); // Uses the 10-Decade resistor for 1-Decade measurements														   // To prevent a large current draw through resistors

	return resistor_value;
}


/**************************************************
Brief: Calculates the percent error of the measured resistor value with the closest standard value in the series
Return: NONE
***************************************************/
void resistor_error(void)
{
	double error_percentage;

	error_percentage = ((R_config.r_measured - R_config.r_standard) / R_config.r_standard) * 100;

	R_config.r_percentage = error_percentage;
}


/**************************************************
Brief: Finds the nearest standard resistor value from the measured resistor
Return: NONE
***************************************************/
void resistor_match(void)
{

	double r_index;

	r_index = R_config.Eseries*(log10(R_config.r_measured/R_config.decade));


	// Compares the standard value to the measured value to compensate for rounding

	if(fabs(r_standard(ceil(r_index)) - R_config.r_measured) < fabs(r_standard(floor(r_index)) - R_config.r_measured))
		r_index = ceil(r_index);
	else
		r_index = floor(r_index);


	R_config.r_standard = r_standard(r_index);

}

/**************************************************
Brief: Sorts through measured values and determines the appropriate decade
Return: Resistor decade 10, 100, 1k, 10k, 100k, 1M
***************************************************/
double r_standard(double index)
{
	double std_value;

	std_value = round(R_config.decade * pow(10, index/R_config.Eseries) / (R_config.decade/10));
	std_value *= R_config.decade / 10;

	if(R_config.Eseries == 24)
	{
		if((int)index % 24 >= 10 && (int)index % 24 <= 16)
			std_value += R_config.decade/10;
		if((int)index == 22)
			std_value -= R_config.decade/10;
	}

	return std_value;
}


/**************************************************
Brief: Sorts through measured values and determines the appropriate decade
Return: Resistor decade 10, 100, 1k, 10k, 100k, 1M
***************************************************/
void resistor_decade(void)
{

	double decade = 1000;


	if((resisor_value(decade) >= decade) && (resisor_value(decade) < decade*10))
	{
		R_config.decade = decade;
		R_config.r_measured = resisor_value(decade);
	}

}


/**************************************************
Brief: Parses the standard resistor values to determine the resistor band colors
Return: NONE
***************************************************/
void resistor_parse(void)
{
	lcd_put_cur(0, 0);

	uint32_t band1 = (uint32_t)R_config.r_standard / (uint32_t)R_config.decade;
	uint32_t band2 = ((uint32_t)(R_config.r_standard) % ((uint32_t)R_config.decade)) / (R_config.decade/10);
	uint32_t band3 = log10(R_config.decade/10);
	uint32_t band4;

	if(band1 == 10)
	{
		band1 = 1;
		band3++;
	}

	//Brief: Get 4-band resistor colors
	resistor_band(0, band1); // First band value
	resistor_band(R_config.color_index, band2); // Second band value

	resistor_band(R_config.color_index , band3); // Decade multiplier

	for(int i = R_config.color_index; i < 13; i++)
		R_config.color_bands[i] = ' ';

	sprintf(R_config.band4, "4Band:%s", R_config.color_bands);


	band1 = (uint32_t)R_config.r_standard / (uint32_t)R_config.decade;
	band2 = ((uint32_t)(R_config.r_standard) % ((uint32_t)R_config.decade)) / (R_config.decade/10);
	band3 = ((uint32_t)(R_config.r_standard) % ((uint32_t)R_config.decade/10)) / (R_config.decade/100);
	band4 = log10(R_config.decade/100);

	if(band1 == 10)
		{
			band1 = 1;
			band4++;
		}

	//Brief: Get 5-band resistor colors
	resistor_band(0, band1); // First band value
	resistor_band(R_config.color_index, band2); // Second band value
	resistor_band(R_config.color_index, band3); // Third band value

	resistor_band(R_config.color_index , band4); // Decade multiplier

	for(int i = R_config.color_index; i < 13; i++)
			R_config.color_bands[i] = ' ';

	sprintf(R_config.band5, "5Band:%s",R_config.color_bands);
}


/**************************************************
Brief: Selects the appropriate characters for a selected color
Return: NONE
***************************************************/
void resistor_band(uint32_t band_index, uint32_t band_value)
{


	//Brief: Space columns numbers based on band_number and UI configuration
	if(band_index == 0)
		R_config.color_index = 0;


	switch(band_value)
	{
		case -2:
			R_config.color_bands[R_config.color_index] = 'S';
			R_config.color_index++;
			R_config.color_bands[R_config.color_index] = 'I';
			R_config.color_index++;
			R_config.color_bands[R_config.color_index] = ' ';
			R_config.color_index++;
			break;
		case -1:
			R_config.color_bands[R_config.color_index] = 'G';
			R_config.color_index++;
			R_config.color_bands[R_config.color_index] = 'D';
			R_config.color_index++;
			R_config.color_bands[R_config.color_index] = ' ';
			R_config.color_index++;
			break;
		case 0:
			R_config.color_bands[R_config.color_index] = 'B';
			R_config.color_index++;
			R_config.color_bands[R_config.color_index] = 'K';
			R_config.color_index++;
			R_config.color_bands[R_config.color_index] = ' ';
			R_config.color_index++;
			break;
		case 1:
			R_config.color_bands[R_config.color_index] = 'B';
			R_config.color_index++;
			R_config.color_bands[R_config.color_index] = 'R';
			R_config.color_index++;
			R_config.color_bands[R_config.color_index] = ' ';
			R_config.color_index++;
			break;
		case 2:
			R_config.color_bands[R_config.color_index] = 'R';
			R_config.color_index++;
			R_config.color_bands[R_config.color_index] = ' ';
			R_config.color_index++;
			break;
		case 3:
			R_config.color_bands[R_config.color_index] = 'O';
			R_config.color_index++;
			R_config.color_bands[R_config.color_index] = ' ';
			R_config.color_index++;
			break;
		case 4:
			R_config.color_bands[R_config.color_index] = 'Y';
			R_config.color_index++;
			R_config.color_bands[R_config.color_index] = ' ';
			R_config.color_index++;
			break;
		case 5:
			R_config.color_bands[R_config.color_index] = 'G';
			R_config.color_index++;
			R_config.color_bands[R_config.color_index] = ' ';
			R_config.color_index++;
			break;
		case 6:
			R_config.color_bands[R_config.color_index] = 'B';
			R_config.color_index++;
			R_config.color_bands[R_config.color_index] = ' ';
			R_config.color_index++;
			break;
		case 7:
			R_config.color_bands[R_config.color_index] = 'V';
			R_config.color_index++;
			R_config.color_bands[R_config.color_index] = ' ';
			R_config.color_index++;
			break;
		case 8:
			R_config.color_bands[R_config.color_index] = 'G';
			R_config.color_index++;
			R_config.color_bands[R_config.color_index] = 'Y';
			R_config.color_index++;
			R_config.color_bands[R_config.color_index] = ' ';
			R_config.color_index++;
			break;
		case 9:
			R_config.color_bands[R_config.color_index] = 'W';
			R_config.color_index++;
			R_config.color_bands[R_config.color_index] = ' ';
			R_config.color_index++;
			break;
		default:
			R_config.color_bands[R_config.color_index] = 'N';
			R_config.color_index++;
			R_config.color_bands[R_config.color_index] = 'A';
			R_config.color_index++;
			R_config.color_bands[R_config.color_index] = ' ';
			R_config.color_index++;
			break;
	}

}


/**************************************************
Brief: Executes all necessary functions for data acquisition and conversion
Return: NONE
***************************************************/
void resistor_measure(void)
{
	resistor_decade();
	resistor_match();
	resistor_error();
	resistor_parse();
}


/**************************************************
Brief: Resets all values in struct R_config
Return: NONE
***************************************************/
void resistor_flush(void)
{

	R_config.r_measured = 0;
	R_config.r_percentage = 0;
	R_config.r_standard = 0;

}
