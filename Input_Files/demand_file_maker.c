#include<stdio.h>

#define PATTERNTIMESTEP 24 

void main()
{
	int i;
	float demand[48], actual_demand;
	/* CPHEEO pattern
	float pattern[] = {0.2,0.2,0.2,0.2,0.4,1,
			   2,2.5,2.5,2,1,1,
		           0.8,0.8,1,0.6,0.8,1,
			   1.5,1.5,1,0.8,0.6,0.4};
	*/
	float pattern[] = {0.3,0.3,0.35,0.31,0.30,0.83,
			   1.57,2.16,2.39,2.16,1.50,1.19,
                           1.07,1.02,0.95,0.92,0.93,0.94,
                           1.01,0.99,1.00,0.93,0.52,0.35};
	/*printf("Enter the demand pattern\n");
	for(i=0; i<PATTERNTIMESTEP; i++)
	{
		scanf("%f",&pattern[i]);
	}*/
	printf("Pattern is:");
	for(i=0;i<PATTERNTIMESTEP; i++)
	{
		printf("%2.1f,",pattern[i]);
	}	
	printf("\nEnter actual Demand:");
	scanf("%f",&actual_demand);
	
	printf("Calculating Demands\n");
	for( i=0;i<PATTERNTIMESTEP; i++)
	{
		demand[i]=pattern[i]*actual_demand;
		printf("%4.3f,",demand[i]);
	}
	printf("\n");
}
