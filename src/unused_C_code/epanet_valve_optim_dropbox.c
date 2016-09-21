
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __APPLE__
#include <malloc.h>
#endif
#include <math.h>
#include <float.h>                                                             //(2.00.12 - LR)
#include "hash.h"    
#include "text.h"
#include "types.h"
#include "enumstxt.h"
#include "funcs.h"
#define  EXTERN
#include "vars.h"
#include "toolkit.h"

#define MAX_VALVEID 256
#define MAX_TIMEPERIOD 72
#define MAXTANKLEVEL 100
#define myMAXITER 50
#define myOUTER_MAXITER 2000
#define DEBUG 1
#define LPS_TANKUNITS 3.6 	/* 1LPS * 3600seconds/1000 = volume in one hour in meter^3*/
struct TankStruct {
    char *TankID;	//Tank ID
    int NodeIndex;		//NodeIndex
    int TimePeriod;	//TimePeriod
    int InputLink; 	//LinkIndex of input Link. Currently assuming there is only one input link to a tank;
    float InFlow[MAX_TIMEPERIOD];	//Array of input flows at eat time.
    int OutputLink;	//LinkIndex of output Link. Currently assuming there is only one output link to a tank.
    float OutFlow[MAX_TIMEPERIOD];	//OutFlow array of size timeperiod (T).
    float OutFlow_Epanet[MAX_TIMEPERIOD];
    float TankLevels[MAX_TIMEPERIOD+1];	//All tank levels from 0 to T. Array of size(T+1).    
    float MaxTankLevel;
    float MinTankLevel;
  };

struct ValveStruct{
  char ValveID[MAX_VALVEID];	//ValveID
  int ValveLink;	//LinkIndex
  int TimePeriod;
  float ValveValues[MAX_TIMEPERIOD];	//Valvevalues. Array of size timeperiod (T).
};
  
  struct TankStruct *temp_tankstruct;
  struct ValveStruct *temp_valvestruct;
  
float epsilon = 0.1;    

void ENReadOutFlow(char *f_demand_input, struct TankStruct *tankcontrol, int timeperiod);
void ENOptimiseValve(struct TankStruct *tankcontrol, struct ValveStruct *valvecontrol);
void update_tank_level(struct TankStruct *tankcontrol);
float objective_function(struct TankStruct *tankcontrol_current,struct ValveStruct *valvecontrol_current);
void compute_flows(struct TankStruct *tankstruct, struct ValveStruct *valvestruct, int timeinstance);
void compute_gradient(struct TankStruct *tankcontrol_current, struct ValveStruct *valvecontrol_current, struct TankStruct *tankcontrol_gradient,struct ValveStruct *valvecontrol_gradient);
void update_control(struct TankStruct *tankcontrol_current, struct ValveStruct *valvecontrol_current,struct TankStruct *tankcontrol_gradient,struct ValveStruct *valvecontrol_gradient);
float norm_gradient(struct TankStruct *tankcontrol_gradient,struct ValveStruct *valvecontrol_gradient);
float max_tank_level(int tank_struct_index);
//char* getfield(char* line, int num);

  
int main(int argc, char *argv[])
{
  char * f_epanet_input,
   * f_demand_input,
   * f_epanet_Rpt = "temp.out",
   * f_blank = " ",
   * temp_char;  
   
   int 	timeperiod=0;
   int 	temp_count=0,
	temp_count2=0,
	temp_node1 = 0,
	temp_node2 = 0,
	temp_nodeIndex = 0;

    
    
 struct TankStruct *tankcontrol;
 struct ValveStruct *valvecontrol; 
  
 printf("size of TankStruct = %d, Size of Valve Struct = %d", sizeof(struct TankStruct), sizeof(struct ValveStruct));
   f_epanet_input = argv[1]; 			// first argument is EPANET input file
   f_demand_input = argv[2]; 			// second argument is Tank demand data file
   sscanf(argv[3],"%d", &timeperiod); 		//Time period to be used in optimisation.  
   
   
   ENopen(f_epanet_input, f_epanet_Rpt,f_blank); 				// Open Epanet input file
   ENsetreport("MESSAGES NO"); //suppresses warnings and erros
   
    temp_tankstruct = (struct TankStruct *)calloc(Ntanks,sizeof(struct TankStruct)); // to be used in compute_gradient
    temp_valvestruct = (struct ValveStruct *)calloc(Nvalves,sizeof(struct ValveStruct));
  
   tankcontrol = (struct TankStruct *)calloc(Ntanks,sizeof(struct TankStruct)); 	//allocate memory for tank structs.  
      
    
    ENReadOutFlow(f_demand_input, tankcontrol, timeperiod);   //Read tank demands (outflow) from file. Need to see if it can be integrated with Epanet.
   
   //Initialise tank control struct pointers
   for(temp_count = 0; temp_count < Ntanks;temp_count++)
   {      
      ENgetnodeindex(tankcontrol[temp_count].TankID, &temp_nodeIndex); //get tank nodeID 
      temp_count2 = -1;
      do{
	temp_count2++;
      }while(Tank[temp_count2].Node != temp_nodeIndex);
      tankcontrol[temp_count].MaxTankLevel = max_tank_level(temp_count2); //find max tank level
      printf("\n Tank ID = %s, Tank Max Level = %f",tankcontrol[temp_count].TankID,tankcontrol[temp_count].MaxTankLevel);
      tankcontrol[temp_count].TimePeriod = timeperiod;
      
      tankcontrol[temp_count].MinTankLevel = 0;
      tankcontrol[temp_count].NodeIndex = temp_nodeIndex;
      tankcontrol[temp_count].OutputLink = 0;
      tankcontrol[temp_count].InputLink = 0;
      for(temp_count2=0;temp_count2<timeperiod;temp_count2++)
      {
	tankcontrol[temp_count].TankLevels[temp_count2] = (tankcontrol[temp_count].MaxTankLevel)/2; //initialise tanks to half capacity
      }
      
      for (temp_count2=0;temp_count2<Nlinks;temp_count2++)
      {
	ENgetlinknodes(temp_count2, &temp_node1, &temp_node2);
	if(temp_node1 == tankcontrol[temp_count].NodeIndex) 
	{
	  tankcontrol[temp_count].OutputLink = temp_count2;	//find output link to tank 
	  if(DEBUG)
	  {
	    printf("\n Tank ID = %s, Tank Output Link ID = %s", tankcontrol[temp_count].TankID, Link[temp_count2].ID );
	  }
  
	} 
	if(temp_node2 == tankcontrol[temp_count].NodeIndex) 
	{
	  tankcontrol[temp_count].InputLink = temp_count2;//find input link to tank
	  if(DEBUG)
	  {
	    printf("\n Tank ID = %s, Tank Input Link ID = %s", tankcontrol[temp_count].TankID, Link[temp_count2].ID );
	  }
	  
	}  
      }

   }
   
   // Initialise Valve Controls
    valvecontrol = (struct ValveStruct *) calloc(Nvalves,sizeof(struct ValveStruct));
    for(temp_count =0; temp_count < Nvalves;temp_count++)
    {       
      valvecontrol[temp_count].ValveLink = Valve[temp_count+1].Link;
      ENgetlinkid(Valve[temp_count+1].Link,valvecontrol[temp_count].ValveID);     
      valvecontrol[temp_count].TimePeriod = timeperiod;  
      for(temp_count2=0;temp_count2<timeperiod;temp_count2++)
      {
	valvecontrol[temp_count].ValveValues[temp_count2] = Link[valvecontrol[temp_count].ValveLink].Kc;
      }
    }
    
    //Call Optmisation module;

    ENOptimiseValve( tankcontrol,  valvecontrol);    
    // Print final results (Valve Values)
    for(temp_count=0;temp_count<Nvalves;temp_count++)
    {
      printf("\n \n \n \n Valve ID = %s", valvecontrol[temp_count].ValveID);
      for(temp_count2 =0; temp_count2 < timeperiod;temp_count2++)
      {
	printf("\t Valve Value [%d] = %f",temp_count2,valvecontrol[temp_count].ValveValues[temp_count2]);
      }
    }
    // Print Final results (Tank Values)
    for(temp_count=0;temp_count<Ntanks;temp_count++)
    {
      printf("\n \n \n Tank ID = %s  ",tankcontrol[temp_count].TankID);
      for(temp_count2=0;temp_count2<=timeperiod;temp_count2++)
      {
	printf("\n Tank Level[%d] = %f, \t Tank Inflow = %f, \t Tank Outflow = %f, ", temp_count2, tankcontrol[temp_count].TankLevels[temp_count2],tankcontrol[temp_count].InFlow[temp_count2],(tankcontrol[temp_count].OutFlow[temp_count2]+tankcontrol[temp_count].OutFlow_Epanet[temp_count2]));
      }
    }    
    
    // free all pointer
    for(temp_count=0;temp_count<Ntanks;temp_count++)
    {
      free(tankcontrol[temp_count].TankID);
    }
    free(tankcontrol);
    free(valvecontrol);
    free(temp_tankstruct);
    free(temp_valvestruct);

   ENclose();
}




void ENReadOutFlow(char *f_demand_input, struct TankStruct *tankcontrol, int timeperiod)
{
    FILE* stream = fopen(f_demand_input, "r"); 	// Open demand file for reading
    char line[1024];				// Read line by line and store in line
    char * temp_field;				// temporary field to extract values
    int tankcount = 0;				// Initial Tank Count = 0;
    int i = 0;
    char * tok;
    char * tmp;
    
    while ((fgets(line, 1024, stream)!=NULL) && (tankcount < Ntanks))
    {
        //tankcontrol[tankcount].OutFlow = (float *)calloc(timeperiod,sizeof(float)); Already allocated in main.
        printf("\ntankcount = %d",tankcount);
        tmp = strdup(line);
	tok = strtok(tmp, ",");
	if(tok !=NULL) {tankcontrol[tankcount].TankID = tok;}
	printf("\n Tank %d: Tank ID = %s,",tankcount+1,tok);
	
	i=0;		
	while((NULL != (tok = strtok(NULL, ","))) && (i<timeperiod))
	{
	  tankcontrol[tankcount].OutFlow[i] = strtod(tok,NULL);	  
	  //printf("OutFlow %d = %f",i+1,tankcontrol[tankcount].OutFlow[i]);
	  i++;
	}
	tankcount++;
    }
    
}
  
void ENOptimiseValve(struct TankStruct *tankcontrol, struct ValveStruct *valvecontrol)
{
   int iteration_count = 0;
  float function_value_current=0;
  float function_value_previous = 0;
  
  struct TankStruct * tankcontrol_current,
		    * tankcontrol_previous,
		    * tankcontrol_gradient;
		    
  struct ValveStruct 	* valvecontrol_current,
			* valvecontrol_previous,
			* valvecontrol_gradient;
			

  tankcontrol_current = (struct TankStruct *)calloc(Ntanks,sizeof(struct TankStruct));
  tankcontrol_previous = (struct TankStruct *)calloc(Ntanks,sizeof(struct TankStruct));
  tankcontrol_gradient = (struct TankStruct *)calloc(Ntanks,sizeof(struct TankStruct));
  
  
   valvecontrol_current = (struct ValveStruct *) calloc(Nvalves,sizeof(struct ValveStruct));
   valvecontrol_previous = (struct ValveStruct *) calloc(Nvalves,sizeof(struct ValveStruct));
   valvecontrol_gradient = (struct ValveStruct *) calloc(Nvalves,sizeof(struct ValveStruct));
  
  memcpy(tankcontrol_current,tankcontrol,Ntanks*sizeof(struct TankStruct)); //Dont know of this will work.
  memcpy(tankcontrol_previous,tankcontrol,Ntanks*sizeof(struct TankStruct));
  memcpy(tankcontrol_gradient,tankcontrol,Ntanks*sizeof(struct TankStruct));
  
  memcpy(valvecontrol_current,valvecontrol,Nvalves*sizeof(struct ValveStruct));
  memcpy(valvecontrol_previous,valvecontrol,Nvalves*sizeof(struct ValveStruct));
  memcpy(valvecontrol_gradient,valvecontrol,Nvalves*sizeof(struct ValveStruct));
  
  function_value_previous = objective_function(tankcontrol_current,valvecontrol_current);
  
  while((iteration_count < myOUTER_MAXITER))
  {
    //printf("\n Iteration Count = %d Function Value = %f",iteration_count,function_value_current);
    memcpy(tankcontrol_previous,tankcontrol_current,Ntanks*sizeof(struct TankStruct));
    memcpy(valvecontrol_previous,valvecontrol_current,Nvalves*sizeof(struct ValveStruct));
    
    compute_gradient(tankcontrol_current, valvecontrol_current,tankcontrol_gradient,valvecontrol_gradient);
    update_control(tankcontrol_current, valvecontrol_current,tankcontrol_gradient,valvecontrol_gradient);
    function_value_previous = function_value_current;
    function_value_current = objective_function(tankcontrol_current,valvecontrol_current);
    iteration_count++;   
    printf("\n Iteration Count = %d Function Value = %f",iteration_count,function_value_current);
  }
  
  memcpy(tankcontrol,tankcontrol_current,Ntanks*sizeof(struct TankStruct));
  memcpy(valvecontrol,valvecontrol_current,Nvalves*sizeof(struct ValveStruct));
  
  free(tankcontrol_current);
  free(tankcontrol_previous);
  free(tankcontrol_gradient);
  free(valvecontrol_current);
  free(valvecontrol_previous);
  free(valvecontrol_gradient);
  
}

float objective_function(struct TankStruct *tankcontrol_current,struct ValveStruct *valvecontrol_current){
  float func_value = 0.0;
  int temp_count;
  int temp_count2;
  int timeperiod;
  float temp_float_var_max = 0.0,
	temp_float_var_min = 0.0;
  
  timeperiod = tankcontrol_current[0].TimePeriod;
  update_tank_level(tankcontrol_current);
  for(temp_count2 = 0; temp_count2 < Ntanks; temp_count2++)
  {     
      func_value += (tankcontrol_current[temp_count2].TankLevels[0]-tankcontrol_current[temp_count2].TankLevels[timeperiod])*(tankcontrol_current[temp_count2].TankLevels[0]-tankcontrol_current[temp_count2].TankLevels[timeperiod]); // penalise non periodicity.
    }
    
  for(temp_count = 0 ; temp_count < timeperiod; temp_count++)
  {
    for(temp_count2 = 0; temp_count2 < Ntanks; temp_count2++)
    {
      
      temp_float_var_max = tankcontrol_current[temp_count2].TankLevels[temp_count]-tankcontrol_current[temp_count2].MaxTankLevel;
      if(temp_float_var_max > 0) {func_value+=temp_float_var_max*temp_float_var_max;}
      temp_float_var_min = tankcontrol_current[temp_count2].TankLevels[temp_count]-tankcontrol_current[temp_count2].MinTankLevel;
      if(temp_float_var_min < 0) {func_value+=temp_float_var_min*temp_float_var_min;}
      
    }
  }
  return func_value;
}

void update_tank_level(struct TankStruct *tankcontrol)
{
  int temp_count;
  int temp_count2;
  int timeperiod;
  
  // Tank levels are in meter^3. Tank Capacity is in meter^3. Inflows are in LPS. 
  // Currently, usage data is assumed to be hourly. Need to convert everthing to meter^3 per hour.
  timeperiod = tankcontrol[0].TimePeriod;
  for(temp_count = 0 ; temp_count < timeperiod; temp_count++)
  {
    for(temp_count2 = 0; temp_count2 < Ntanks; temp_count2++)
    {
      tankcontrol[temp_count2].TankLevels[temp_count+1] = tankcontrol[temp_count2].TankLevels[temp_count]+(tankcontrol[temp_count2].InFlow[temp_count]-tankcontrol[temp_count2].OutFlow[temp_count]-tankcontrol[temp_count2].OutFlow_Epanet[temp_count])*LPS_TANKUNITS;
    }
  }
}

void compute_flows(struct TankStruct *tankstruct, struct ValveStruct *valvestruct, int timeinstance)
{
  int timeperiod;
  int temp_count;
  long t, tstep;
  
  timeperiod = tankstruct[0].TimePeriod;
  timeinstance = timeinstance%timeperiod;
  
  // Call epanet for function computation
  ENopenH();
  for(temp_count=0;temp_count<Nvalves;temp_count++)
  {
    ENsetlinkvalue(valvestruct[temp_count].ValveLink,EN_INITSETTING,valvestruct[temp_count].ValveValues[timeinstance]);
  }
  ENinitH(10);
  do{
         ENrunH(&t);
         ENnextH(&tstep);
      }
      while (tstep > 0);
   
  
  for(temp_count=0;temp_count<Ntanks;temp_count++)
  {
    ENgetlinkvalue(tankstruct[temp_count].InputLink,EN_FLOW, &(tankstruct[temp_count].InFlow[timeinstance]));
    ENgetlinkvalue(tankstruct[temp_count].OutputLink,EN_FLOW, &(tankstruct[temp_count].OutFlow_Epanet[timeinstance]));
  }  
  
  ENcloseH(); 
}

void compute_gradient(struct TankStruct *tankcontrol_current, struct ValveStruct *valvecontrol_current, struct TankStruct *tankcontrol_gradient,struct ValveStruct *valvecontrol_gradient)
{
  int temp_count;
  int temp_count2;
  int timeperiod;
  
  float function_value_1,
	function_value_2;
  
  timeperiod = tankcontrol_current->TimePeriod;

  
  memcpy(temp_tankstruct,tankcontrol_current,Ntanks*sizeof(struct TankStruct));
  memcpy(temp_valvestruct,valvecontrol_current,Nvalves*sizeof(struct ValveStruct));
  
  //compute greadient for valves
  for(temp_count =0;temp_count <Nvalves;temp_count++)
  {
    for(temp_count2=0;temp_count2<timeperiod;temp_count2++)
    {
      temp_valvestruct[temp_count].ValveValues[temp_count2] = valvecontrol_current[temp_count].ValveValues[temp_count2]+epsilon;
      compute_flows(temp_tankstruct,temp_valvestruct,temp_count2);
      function_value_1 = objective_function(temp_tankstruct,temp_valvestruct);
      
      temp_valvestruct[temp_count].ValveValues[temp_count2] = (valvecontrol_current[temp_count].ValveValues[temp_count2]-epsilon); //just to take positive values, (x+\x\)/2.
      if(temp_valvestruct[temp_count].ValveValues[temp_count2] < 0)
      {temp_valvestruct[temp_count].ValveValues[temp_count2] = 0;}
      compute_flows(temp_tankstruct,temp_valvestruct,temp_count2);
      function_value_2 = objective_function(temp_tankstruct,temp_valvestruct);
      
      valvecontrol_gradient[temp_count].ValveValues[temp_count2] = (function_value_1-function_value_2)/(2*epsilon); //gradient computation
      
      temp_valvestruct[temp_count].ValveValues[temp_count2] = valvecontrol_current[temp_count].ValveValues[temp_count2]; //make the value back to original.
    }
  }
  
  //reinitialise temp structures. This step need not be required as nothing is changed in the above computation.
  memcpy(temp_tankstruct,tankcontrol_current,Ntanks*sizeof(struct TankStruct));
  memcpy(temp_valvestruct,valvecontrol_current,Nvalves*sizeof(struct ValveStruct));
  //compute gradient for tanks
  for(temp_count =0;temp_count <Ntanks;temp_count++)
    {    
      temp_tankstruct[temp_count].TankLevels[0] = tankcontrol_current[temp_count].TankLevels[0]+epsilon;     
      function_value_1 = objective_function(temp_tankstruct,temp_valvestruct);
      
      temp_tankstruct[temp_count].TankLevels[0] = tankcontrol_current[temp_count].TankLevels[0]-epsilon;      
      function_value_2 = objective_function(temp_tankstruct,temp_valvestruct);
	
      tankcontrol_gradient[temp_count].TankLevels[0] = (function_value_1-function_value_2)/(2*epsilon);
      temp_tankstruct[temp_count].TankLevels[0] = tankcontrol_current[temp_count].TankLevels[0]; //not necessary in gradient compuation as function values are separable in the tanklevels. Nevertheless to allay any fears.
    }
  
  
  
}

void update_control(struct TankStruct *tankcontrol_current, struct ValveStruct *valvecontrol_current,struct TankStruct *tankcontrol_gradient,struct ValveStruct *valvecontrol_gradient)
{
  int temp_count;
  int temp_count2;
  int timeperiod;
  float alpha_mult,
	rho =0.63,
	c = 0.1,
	temp_valve_value = 0,
	iteration_count = 0,
	norm_grad_var=0.0;
  
  float function_value_current,
	function_value_next = 0;
  
  timeperiod = tankcontrol_current->TimePeriod;
    
  memcpy(temp_tankstruct,tankcontrol_current,Ntanks*sizeof(struct TankStruct));
  memcpy(temp_valvestruct,valvecontrol_current,Nvalves*sizeof(struct ValveStruct));
  
  function_value_current = objective_function(tankcontrol_current,valvecontrol_current);
  function_value_next = function_value_current;
  
  // TankValues are not updated.
  alpha_mult = 1;
  norm_grad_var = norm_gradient(tankcontrol_gradient,valvecontrol_gradient);
  printf(" Gradient Norm = %f", norm_grad_var);
  //((function_value_next-function_value_current)> -c *alpha_mult* norm_grad_var) && (iteration_count < myMAXITER)
  while(((function_value_next-function_value_current)>= -c *alpha_mult* norm_grad_var) && (iteration_count < myMAXITER))
  { //printf("\n Line Search Iter = %f",alpha_mult);
    
    for(temp_count2=0;temp_count2<timeperiod;temp_count2++)    
    {
      for(temp_count =0;temp_count <Nvalves;temp_count++)
      {
	temp_valve_value = valvecontrol_current[temp_count].ValveValues[temp_count2]-valvecontrol_gradient[temp_count].ValveValues[temp_count2]*alpha_mult;	
	
	if(temp_valve_value > 0) {temp_valvestruct[temp_count].ValveValues[temp_count2] = temp_valve_value;}
	else{ temp_valvestruct[temp_count].ValveValues[temp_count2] = 0;}
	
      }
      compute_flows(temp_tankstruct,temp_valvestruct,temp_count2);
    }
    
    for(temp_count =0;temp_count <Ntanks;temp_count++)
      {
	temp_tankstruct[temp_count].TankLevels[0] = tankcontrol_current[temp_count].TankLevels[0]-tankcontrol_gradient[temp_count].TankLevels[0]*alpha_mult;			
      }
    //printf("\n alpha mult %f",alpha_mult);    
    function_value_next = objective_function(temp_tankstruct,temp_valvestruct);
    
    alpha_mult*=rho;
    iteration_count++;
  }
  printf(" alpha mult %f",alpha_mult);
 memcpy(tankcontrol_current, temp_tankstruct,Ntanks*sizeof(struct TankStruct));
 memcpy(valvecontrol_current,temp_valvestruct,Nvalves*sizeof(struct ValveStruct));
}

float norm_gradient(struct TankStruct *tankcontrol_gradient,struct ValveStruct *valvecontrol_gradient)
{
  int temp_count =0,
      temp_count2 = 0,
      timeperiod;
  float temp_gradient_norm = 0.0;
      
      timeperiod = tankcontrol_gradient->TimePeriod;
      for(temp_count=0;temp_count<Nvalves;temp_count++)
      {
	for(temp_count2=0;temp_count2<timeperiod;temp_count2++)
	{
	  temp_gradient_norm += valvecontrol_gradient[temp_count].ValveValues[temp_count2]*valvecontrol_gradient[temp_count].ValveValues[temp_count2];
	}	
      }
      for(temp_count2=0;temp_count2<Ntanks;temp_count2++)
	{
	  temp_gradient_norm += tankcontrol_gradient[temp_count2].TankLevels[0]*tankcontrol_gradient[temp_count2].TankLevels[0];
	}
  return temp_gradient_norm;
}

float max_tank_level(int tank_struct_index)
{
  float area = 0.0,
	tank_max_level,
	tank_min_level,
	tank_level_LH,
	feet3_meter3_coversion = pow(12*2.54/100,3);
	
	area = Tank[tank_struct_index].A;
	tank_max_level = Tank[tank_struct_index].Hmax;
	tank_min_level = Tank[tank_struct_index].Hmin;		
	tank_level_LH = area*(tank_max_level-tank_min_level);	// This is in feet^3. COnvert to meter^3
	tank_level_LH *= feet3_meter3_coversion;
	
	
	printf("\n Tank ID = %d, Tank Area = %f, Tank Max Level = %f, Tank Min Level = %f, Tank Max Capacity = %f, Max Volume = %f, Min Volume = %f ",Tank[tank_struct_index].Node, area, tank_max_level, tank_min_level, tank_level_LH, Tank[tank_struct_index].Vmax, tank_level_LH, Tank[tank_struct_index].Vmin );
	return tank_level_LH;
}

