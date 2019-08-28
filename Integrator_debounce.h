/*
 * Integrator_debounce.h
 *
 * Created: 30.07.2014 23:36:40
 *  Author: idopshik
 
 ������ ������� ���
 */ 


#ifndef INTEGRATOR_DEBOUNCE_H_
#define INTEGRATOR_DEBOUNCE_H_


//////���������� ��� �������������� ��������
#define DEBOUNCE_TIME       0.3     ////��� ��� ���������� ��� ��������
#define SAMPLE_FREQUENCY    3000
#define MAXIMUM         (DEBOUNCE_TIME * SAMPLE_FREQUENCY)
 
 char f_integrator (unsigned char input); 
  char f_integrator_1 (unsigned char input_1);  
  
 
/* These are the variables used */
///unsigned int input;       /* 0 or 1 depending on the input signal */ ������� � ���������, ��� �� ����� ����� ����.
unsigned int integrator;  /* Will range from 0 to the specified MAXIMUM */
unsigned char output = 1 ;      /* Cleaned-up version of the input signal */

 unsigned int integrator_1;  /* Will range from 0 to the specified MAXIMUM */
 unsigned char output_1 = 1 ;      /* Cleaned-up version of the input signal */



char f_integrator (unsigned char input)
{
	

	if (input == 0)
		{
		  if (integrator > 0)integrator--;
		}
	else if (integrator < MAXIMUM) integrator++;
    
	
	if (integrator == 0) output = 0;
    
	else if (integrator >= MAXIMUM)
	  {
		output = 1;
		integrator = MAXIMUM;  // defensive code if integrator got corrupted 
	  }
	  return output;
}	  
 
///////////////////////////////////////����� ����//////////////////////


 
char f_integrator_1 (unsigned char input_1)
{
	

	if (input_1 == 0)
	{
		if (integrator_1 > 0)integrator_1--;
	}
	else if (integrator_1 < MAXIMUM) integrator_1++;
	
	
	if (integrator_1 == 0) output_1 = 0;
	
	else if (integrator_1 >= MAXIMUM)
	{
		output_1 = 1;
		integrator_1 = MAXIMUM;  // defensive code if integrator got corrupted
	}
	return output_1;
}
#endif /* INTEGRATOR_DEBOUNCE_H_ */