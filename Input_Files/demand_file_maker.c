#include<stdio.h>

#define PATTERNTIMESTEP 24 

void main()
{
	int i;
	float demand[48],actual_demand,pattern[]={0.2,0.2,0.2,0.2,0.4,1,
						  2,2.5,2.5,2,1,1,
						  0.8,0.8,1,0.6,0.8,1,
						  1.5,1.5,1,0.8,0.6,0.4};
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
