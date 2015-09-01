/*
 * Epanet search algorithm for obtaining valve/pumping schedules
 * Author : Lovelesh
 * Reference : Nidhin, EPANET
 * 
 * 
 * |-------|    |--------|     |--------|
 * | QUEUE |--->| ENGINE |---->| OUTPUT |
 * |-------|    |--------|     |--------|
 *     ^            |              |
 *     |            V              |
 *     |     |-----------|         |
 *     |-----| MINIMISER |<--------|
 *           |-----------|
 * 
 * Queue: ENReadOutFlow();
 *        Initiate_valve();
 * 	  Queueing_Engine();
 *        Job_Handler();
 * 
 * Engine: update_tank_level();
 *         compute_flows();
 *         
 * Output: display_output();
 * 
 * Minimiser: ENOptimiseValve();
 *            objective_function();
 *            compute_gradient();
 *            update_control();
 *            norm_gradient();
 *            random_direction();
 * 
 * Queue schedules the jobs and loads into the Priority Queue. 
 * 
 * Engine takes in these jobs to start the simulation.
 * There is an external clock that determines the simulation time.
 * After every simulation, tank and valve structures are updated.
 * 
 * Output displayes the tank and valve structures in readable format.
 * 
 * Minimiser takes these tank and valve settings from engine and output,
 * calculates the objective function and finds the gradient that minimises the gradient.
 * The nedded changes are then updated as jobs in the Priority Queue and the cycle continues.
 * 
*/
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
#include <time.h>

#include "hash.h"
#include "text.h"
#include "types.h"
#include "enumstxt.h"
#include "funcs.h"
#define  EXTERN
#include "vars.h"
#include "toolkit.h"

// Macro declarations

#define MAX_VALVEID 256
#define MAX_TIMEPERIOD 72
#define MAXTANKLEVEL 100
#define myMAXITER 30
#define myOUTER_MAXITER 200000
#define DEBUG 1
#define LPS_TANKUNITS 3.6 	/* 1LPS * 3600seconds/1000 = volume in one hour in meter^3*/
#define MAX_VALVEVALUE 350
#define FILENAME_JOBS "joblist.txt"
#define SIZE 2000						/* Size of Queue */

const int PRINT_INTERVAL_FUNCTIONVALUE = 100;
const int PRINT_INTERVAL_VALVEVALUES = 1000;

// Struct Declarations

typedef struct PRQ
{
	char keyword[20];
	int hours;
	int minutes;
	char id[20];
	float value;
}PriorityQ;
 
PriorityQ PQ[SIZE];

struct TankStruct {
	char *TankID;	//Tank ID
	int NodeIndex;	//NodeIndex
	int TimePeriod;	//TimePeriod
	int InputLink; 	//LinkIndex of input Link. Currently assuming there is only one input link to a tank;
	float InFlow[MAX_TIMEPERIOD];	//Array of input flows at each time.
	int OutputLink;	//LinkIndex of output Link. Currently assuming there is only one output link to a tank.
	float OutFlow[MAX_TIMEPERIOD];	//OutFlow array of size timeperiod (T).
	float OutFlow_Epanet[MAX_TIMEPERIOD];
	float TankLevels[MAX_TIMEPERIOD+1];	//All tank levels from 0 to T. Array of size(T+1).
	float MaxTankLevel;
	float MinTankLevel;
};

struct ValveStruct {
	char ValveID[MAX_VALVEID];	//Valve ID
	int ValveLink;	//LinkIndex
	int TimePeriod;
	float ValveValues[MAX_TIMEPERIOD];	//Valvevalues. Array of size timeperiod (T).
};

// Function declarations

void ENReadOutFlow(char *f_demand_input, struct TankStruct *tankcontrol, int timeperiod);
void ENOptimiseValve(struct TankStruct *tankcontrol, struct ValveStruct *valvecontrol);
void update_tank_level(struct TankStruct *tankcontrol);
double objective_function(struct TankStruct *tankcontrol_current,struct ValveStruct *valvecontrol_current);
void compute_flows(struct TankStruct *tankstruct, struct ValveStruct *valvestruct, int simulation_time);
void compute_gradient(struct TankStruct *tankcontrol_current, struct ValveStruct *valvecontrol_current, struct TankStruct *tankcontrol_gradient,struct ValveStruct *valvecontrol_gradient);
void update_control(struct TankStruct *tankcontrol_current, struct ValveStruct *valvecontrol_current,struct TankStruct *tankcontrol_gradient,struct ValveStruct *valvecontrol_gradient,int main_iteration_count);
float norm_gradient(struct TankStruct *tankcontrol_gradient,struct ValveStruct *valvecontrol_gradient);
float max_tank_level(int tank_struct_index);
void random_direction(float *random_vector, int dimension);
float tank_level_meter3_to_Level(float tank_volume, int tank_struct_index);
void Initialise_Valve_Values(char *f_valve_init, struct ValveStruct *valvecontrol, int timeperiod);
void display_output(struct TankStruct *tankcontrol, struct TankStruct *tankcontrol_gradient, struct ValveStruct *valvecontrol, struct ValveStruct *valvecontrol_gradient);
int Queueing_Engine(char *f_job_input);
PriorityQ Qpop();
int Qfull();
int Qempty();
void Qdisplay();
void Qpush(char *keyword, int hours, int minutes, char *id, float value);
int Job_Handler(struct TankStruct *tankcontrol, struct ValveStruct *valvecontrol);
void Job_Scheduler(struct TankStruct *tankcontrol, struct ValveStruct *valvecontrol);

// Global Variable Declarations

struct TankStruct *temp_tankstruct;
struct ValveStruct *temp_valvestruct;
int f=0,r=-1;       	//For Priority Queue
float epsilon = 0.05;  //Arbitary value taken
int iteration_count = 1;
int elapsed_time = 0; // Starting clock is considered at 00:00 hrs
int hourly_scheduler = 2;      // Determines the job to be scheduled in hours; if 1 = hourly jobs, if 2 = jobs scheduled for 2 hours

// Main function starts here

int main(int argc, char *argv[])
{

	char * f_epanet_input,
	     * f_demand_input,
		 * f_job_input,
	     * f_epanet_Rpt = "temp.out",
	       * f_valve_init,
	       * f_blank = " ",
	         * temp_char;

	int 	timeperiod = 0;
	int 	temp_count = 0,
	        temp_count2 = 0,
	        temp_node1 = 0,
	        temp_node2 = 0,
	        temp_nodeIndex = 0,
	        open_valve_init_file_flag = 0;
	int simulation_time = 0; 
	int *tank_struct_index; //a temporary array to store the corresponding index of a tank in Tank struct of epanet.
	struct TankStruct *tankcontrol, *tankcontrol_gradient;
	struct ValveStruct *valvecontrol, *valvecontrol_gradient;

	printf("size of TankStruct = %d, Size of Valve Struct = %d", sizeof(struct TankStruct), sizeof(struct ValveStruct));
	f_epanet_input = argv[1]; 			// first argument is EPANET input file
	f_demand_input = argv[2]; 			// second argument is Tank demand data file
	f_job_input = argv[3];				// third argument is the joblist file
	sscanf(argv[4],"%d", &timeperiod); 		//Time period to be used in optimisation.
	if(argc>5) {
		open_valve_init_file_flag = 1;
		f_valve_init = argv[5];
	}

	ENopen(f_epanet_input, f_epanet_Rpt,f_blank); 				// Open Epanet input file
	ENsetreport("MESSAGES NO"); //suppresses warnings and erros
	ENsolveH(); //A dummy run of Epanet to populate all internal states.

	//Tank and Valve initialisation
	temp_tankstruct = (struct TankStruct *)calloc(Ntanks,sizeof(struct TankStruct)); // to be used in compute_gradient
	temp_valvestruct = (struct ValveStruct *)calloc(Nvalves,sizeof(struct ValveStruct));
	tank_struct_index = (int *)calloc(Ntanks,sizeof(int));

	tankcontrol = (struct TankStruct *)calloc(Ntanks,sizeof(struct TankStruct)); 	//allocate memory for tank structs.
	valvecontrol = (struct ValveStruct *) calloc(Nvalves,sizeof(struct ValveStruct)); //allocate memory for valve structs.


	ENReadOutFlow(f_demand_input, tankcontrol, timeperiod);   //Read tank demands (outflow) from file. Need to see if it can be integrated with Epanet.

	//Initialise tank control struct pointers
	for(temp_count = 0; temp_count < Ntanks; temp_count++) {
		ENgetnodeindex(tankcontrol[temp_count].TankID, &temp_nodeIndex); //get tank nodeID
		temp_count2 = 0;
		while(Tank[temp_count2].Node != temp_nodeIndex) {
			temp_count2++;
		}
		tank_struct_index[temp_count]=temp_count2;
		tankcontrol[temp_count].MaxTankLevel = max_tank_level(temp_count2); //find max tank level
		printf("\n Tank ID = %s, Tank Max Level (meter^3) = %f",tankcontrol[temp_count].TankID,tankcontrol[temp_count].MaxTankLevel);
		tankcontrol[temp_count].TimePeriod = timeperiod;
		tankcontrol[temp_count].MinTankLevel = 0;
		tankcontrol[temp_count].NodeIndex = temp_nodeIndex;
		tankcontrol[temp_count].OutputLink = 0;
		tankcontrol[temp_count].InputLink = 0;

		for(temp_count2=0; temp_count2<timeperiod; temp_count2++) {
			tankcontrol[temp_count].TankLevels[temp_count2] = (tankcontrol[temp_count].MaxTankLevel)/2; //initialise tanks to half capacity
			// Just arbitiarily taken
		}

		for (temp_count2=0; temp_count2<Nlinks; temp_count2++) {
			ENgetlinknodes(temp_count2, &temp_node1, &temp_node2);
			if(temp_node1 == tankcontrol[temp_count].NodeIndex) {
				tankcontrol[temp_count].OutputLink = temp_count2;	//find output link to tank
				if(DEBUG) {
					printf("\n Tank ID = %s, Tank Output Link ID = %s", tankcontrol[temp_count].TankID, Link[temp_count2].ID );
				}
			}
			if(temp_node2 == tankcontrol[temp_count].NodeIndex) {
				tankcontrol[temp_count].InputLink = temp_count2;     //find input link to tank
				if(DEBUG) {
					printf("\n Tank ID = %s, Tank Input Link ID = %s", tankcontrol[temp_count].TankID, Link[temp_count2].ID );
				}
			}
		}

	}

	// Initialise Valve Controls
	if(open_valve_init_file_flag == 1) {
		Initialise_Valve_Values(f_valve_init, valvecontrol, timeperiod);
	} else {

		for(temp_count =0; temp_count < Nvalves; temp_count++) {
			valvecontrol[temp_count].ValveLink = Valve[temp_count+1].Link;
			ENgetlinkid(Valve[temp_count+1].Link,valvecontrol[temp_count].ValveID);
			valvecontrol[temp_count].TimePeriod = timeperiod;
			printf("\n Valve Id = %s \t ",valvecontrol[temp_count].ValveID);
			for(temp_count2=0; temp_count2<timeperiod; temp_count2++) {
				valvecontrol[temp_count].ValveValues[temp_count2] = Link[valvecontrol[temp_count].ValveLink].Kc;
				// printf("\n Valve Value[%d] = %f \t ",temp_count2, valvecontrol[temp_count].ValveValues[temp_count2]);
			}

			printf(" Initial Valve Value = %f",valvecontrol[temp_count].ValveValues[0]);
		}
	}
	// Randomise random number seed.
	srand(time(NULL));

	// Initialise the priority Queue
	Queueing_Engine(f_job_input);
	//Qpush("abc", 0,25, "xyzzy", 125.1225);
	//Qdisplay();
	tankcontrol_gradient = (struct TankStruct *)calloc(Ntanks,sizeof(struct TankStruct));
	valvecontrol_gradient = (struct ValveStruct *)calloc(Nvalves,sizeof(struct ValveStruct));
	
	
	simulation_time = Job_Handler(tankcontrol, valvecontrol);
	compute_flows(tankcontrol, valvecontrol, simulation_time);
	update_tank_level(tankcontrol);
	memcpy(tankcontrol_gradient,tankcontrol,Ntanks*sizeof(struct TankStruct));
	memcpy(valvecontrol_gradient,valvecontrol,Nvalves*sizeof(struct ValveStruct));
	display_output(tankcontrol, tankcontrol_gradient, valvecontrol, valvecontrol_gradient);

	
	

	//Call Optmisation module;
	//ENOptimiseValve( tankcontrol,  valvecontrol);

	// free all pointer
	for(temp_count=0; temp_count<Ntanks; temp_count++) {
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
	/*
	 * Reads the outflow demand for each tank for times 0 to timeperiod-1.
	 * f_demand_input 	: file name of file containing outflow demand data. Each tank data should be in one line, and each field separated by a comma. First field should be the tank ID.
	 * tankcontrol	: Pointer to the TankStruct array.
	 * timeperiod		: Optimisation time period in hours.
	*/
	FILE* stream = fopen(f_demand_input, "r"); 	// Open demand file for reading
	char line[1024];				// Read line by line and store in line
	char * temp_field;				// temporary field to extract values
	int tankcount = 0;				// Initial Tank Count = 0;
	int i = 0;
	char * tok;
	char * tmp;

	while ((fgets(line, 1024, stream)!=NULL) && (tankcount < Ntanks)) {
		//tankcontrol[tankcount].OutFlow = (float *)calloc(timeperiod,sizeof(float)); Already allocated in main.
		printf("\ntankcount = %d",tankcount);
		tmp = strdup(line);
		tok = strtok(tmp, ",");
		if(tok !=NULL) {
			tankcontrol[tankcount].TankID = tok;
		}
		printf("\n Tank %d: Tank ID = %s,",tankcount+1,tok);

		i=0;
		while((NULL != (tok = strtok(NULL, ","))) && (i<timeperiod)) {
			tankcontrol[tankcount].OutFlow[i] = strtod(tok,NULL);
			//printf("OutFlow %d = %f",i+1,tankcontrol[tankcount].OutFlow[i]);
			i++;
		}
		tankcount++;
	}
	fclose(stream);

}

void ENOptimiseValve(struct TankStruct *tankcontrol, struct ValveStruct *valvecontrol)
{
	/*
	 * Main optimsation module.
	 */
	char * f_blank = " ";
	int 	temp_count = 0,
	        temp_count2 = 0;
	double  function_value_current=0,
	        function_value_previous = 0;

	struct TankStruct * tankcontrol_current,
			* tankcontrol_previous,
			* tankcontrol_gradient;

	struct ValveStruct 	* valvecontrol_current,
			* valvecontrol_previous,
			* valvecontrol_gradient;

	iteration_count = 1;
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

	while((iteration_count < myOUTER_MAXITER)) {

		//printf("\n Iteration Count = %d Function Value = %f",iteration_count,function_value_current);
		memcpy(tankcontrol_previous,tankcontrol_current,Ntanks*sizeof(struct TankStruct));
		memcpy(valvecontrol_previous,valvecontrol_current,Nvalves*sizeof(struct ValveStruct));

		compute_gradient(tankcontrol_current, valvecontrol_current,tankcontrol_gradient,valvecontrol_gradient);
		update_control(tankcontrol_current, valvecontrol_current,tankcontrol_gradient,valvecontrol_gradient, iteration_count);
		function_value_previous = function_value_current;
		function_value_current = objective_function(tankcontrol_current,valvecontrol_current);
		iteration_count++;

		if((((iteration_count%PRINT_INTERVAL_FUNCTIONVALUE)==0))) {
			printf("\n Iteration Count = %d\tFunction Value = %f \t Percentage Difference = %f",iteration_count,function_value_current,(function_value_current-function_value_previous)/function_value_previous);
		}

		// Print intermediate valve and tank values
		if(((iteration_count%PRINT_INTERVAL_VALVEVALUES) == 1)||(iteration_count == 1)) {
			
			printf("\nIteration Count = %d\n",iteration_count);
			// Schedule jobs
			Job_Scheduler(tankcontrol_current, valvecontrol_current);
			
			// Handle the jobs from the Queue
			int time = Job_Handler(tankcontrol, valvecontrol);
			compute_flows(tankcontrol, valvecontrol, 1);
			update_tank_level(tankcontrol);
			
			// Print final results (Tank and Valve Values)
			//memcpy(tankcontrol,tankcontrol_current,Ntanks*sizeof(struct TankStruct));
			//memcpy(valvecontrol,valvecontrol_current,Nvalves*sizeof(struct ValveStruct));
			display_output(tankcontrol, tankcontrol_gradient, valvecontrol, valvecontrol_gradient);
			
		}
	}

	memcpy(tankcontrol,tankcontrol_current,Ntanks*sizeof(struct TankStruct));
	memcpy(valvecontrol,valvecontrol_current,Nvalves*sizeof(struct ValveStruct));

	// free all structures
	free(tankcontrol_current);
	free(tankcontrol_previous);
	free(tankcontrol_gradient);
	free(valvecontrol_current);
	free(valvecontrol_previous);
	free(valvecontrol_gradient);

}

void Initialise_Valve_Values(char *f_valve_init, struct ValveStruct *valvecontrol, int timeperiod)
{
	/*
	 * Reads the initial valve setting for times 0 to timeperiod-1.
	 * f_valve_init 	: file name of file containing valve  data. Each tank data should be in one line, and each field separated by a comma. First field should be the tank ID.
	 * valvecontrol	: Pointer to the ValveStruct array.
	 * timeperiod		: Optimisation time period in hours.
	*/
	FILE* stream = fopen(f_valve_init, "r"); 	// Open demand file for reading
	char line[1024];				// Read line by line and store in line
	char * temp_field;				// temporary field to extract values
	int valvecount = 0;				// Initial Tank Count = 0;
	int i = 0;
	char * tok;
	char * tmp;
	int temp_count = 0,
	    temp_count2 = 0;
	char temp_valveid[MAX_VALVEID];

	while ((fgets(line, 1024, stream)!=NULL) && (valvecount < Nvalves)) {
		valvecontrol[valvecount].TimePeriod = timeperiod;
		printf("\nvalvecount = %d",valvecount);
		tmp = strdup(line);
		tok = strtok(tmp, ",");
		if(tok !=NULL) {
			strcpy(valvecontrol[valvecount].ValveID,tok);
		}
		printf("\n \nValve %d: Valve ID = %s,",valvecount+1,valvecontrol[valvecount].ValveID);
		i=0;
		while((NULL != (tok = strtok(NULL, ","))) && (i<timeperiod)) {
			valvecontrol[valvecount].ValveValues[i] = strtod(tok,NULL);
			printf("\n ValveValue %d = %f",i+1,valvecontrol[valvecount].ValveValues[i]);
			i++;
		}
		valvecount++;
	}

	for(temp_count = 0; temp_count<Nvalves; temp_count++) {
		for(temp_count2 = 0; temp_count2<Nvalves; temp_count2++) {
			ENgetlinkid(Valve[temp_count2+1].Link,temp_valveid);
			if(strcmp(temp_valveid,valvecontrol[temp_count].ValveID)==0) {
				valvecontrol[temp_count].ValveLink = Valve[temp_count2+1].Link;
				printf("\nValveID = %s, Valve Link = %d",valvecontrol[temp_count].ValveID,valvecontrol[temp_count].ValveLink);
				continue;
			}
		}
	}

}


double objective_function(struct TankStruct *tankcontrol_current,struct ValveStruct *valvecontrol_current)
{
	double func_value = 0.0;
	int temp_count;
	int temp_count2;
	int timeperiod;
	float temp_float_var_max = 0.0,
	      temp_float_var_min = 0.0,
	      temp_tank_level_difference = 0.0,
	      random_number=0.0;

	timeperiod = tankcontrol_current[0].TimePeriod;
	update_tank_level(tankcontrol_current);

	//periodicity Computation and Penalty
	for(temp_count2 = 0; temp_count2 < Ntanks; temp_count2++) {
		// A hack to ignore the main tank in optimisation
		//if(strcmp(tankcontrol_current[temp_count2].TankID,"34")==0) continue;

		// A new approach to take into account tank overflow and emptiness.
		temp_tank_level_difference = 0.0;


		//penalise only if the tank level at time T is less than the initial.

		temp_float_var_max = tankcontrol_current[temp_count2].TankLevels[timeperiod]-tankcontrol_current[temp_count2].TankLevels[0]; // penalise non periodicity.
		func_value += 24*pow(temp_float_var_max,2)/tankcontrol_current[temp_count2].MaxTankLevel;

	
	}

	//Tank Levels at each time and Penalty Computation
	for(temp_count = 0 ; temp_count <= timeperiod; temp_count++) {
		for(temp_count2 = 0; temp_count2 < Ntanks; temp_count2++) {
			//if(strcmp(tankcontrol_current[temp_count2].TankID,"34")==0) continue;
			temp_float_var_max = (2*tankcontrol_current[temp_count2].TankLevels[temp_count]-(tankcontrol_current[temp_count2].MaxTankLevel+tankcontrol_current[temp_count2].MinTankLevel));
			func_value+=pow(temp_float_var_max,2)/tankcontrol_current[temp_count2].MaxTankLevel;
		}
	}

	// If valve value is more than the max valve value, penalise objective value.
	for(temp_count = 0 ; temp_count <Nvalves; temp_count++) {
		for(temp_count2 = 0; temp_count2 < timeperiod; temp_count2++) {
			temp_float_var_max = (valvecontrol_current[temp_count].ValveValues[temp_count2]-MAX_VALVEVALUE);
			if(temp_float_var_max > 0) {
				func_value+=temp_float_var_max*temp_float_var_max;
			}
			func_value+=temp_float_var_max/MAX_VALVEVALUE;
		}
	}
	func_value = func_value/1000;
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
	for(temp_count = 0 ; temp_count < timeperiod; temp_count++) {
		for(temp_count2 = 0; temp_count2 < Ntanks; temp_count2++) {
			tankcontrol[temp_count2].TankLevels[temp_count+1] = tankcontrol[temp_count2].TankLevels[temp_count]+(tankcontrol[temp_count2].InFlow[temp_count]-tankcontrol[temp_count2].OutFlow[temp_count]-tankcontrol[temp_count2].OutFlow_Epanet[temp_count])*LPS_TANKUNITS; //TankLevel is water volume in meter^3.
		}
	}
}

void compute_flows(struct TankStruct *tankstruct, struct ValveStruct *valvestruct, int simulation_time)
{
	int timeperiod;
	int temp_count;
	long t, tstep;

	timeperiod = tankstruct[0].TimePeriod;
	simulation_time = simulation_time%timeperiod;

	// Call epanet for computation
	ENopenH();
	for(temp_count=0; temp_count<Nvalves; temp_count++) {
		ENsetlinkvalue(valvestruct[temp_count].ValveLink,EN_INITSETTING,valvestruct[temp_count].ValveValues[simulation_time]);
	}
	ENinitH(10);
	do {
		ENrunH(&t);
		ENnextH(&tstep);
	} while (tstep > 0);


	for(temp_count=0; temp_count<Ntanks; temp_count++) {
		ENgetlinkvalue(tankstruct[temp_count].InputLink,EN_FLOW, &(tankstruct[temp_count].InFlow[simulation_time]));
		ENgetlinkvalue(tankstruct[temp_count].OutputLink,EN_FLOW, &(tankstruct[temp_count].OutFlow_Epanet[simulation_time]));
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

	//compute gradient for valves
	for(temp_count =0; temp_count <Nvalves; temp_count++) {
		for(temp_count2=0; temp_count2<timeperiod; temp_count2++) {
			temp_valvestruct[temp_count].ValveValues[temp_count2] = valvecontrol_current[temp_count].ValveValues[temp_count2]+epsilon;
			compute_flows(temp_tankstruct,temp_valvestruct,temp_count2);
			function_value_1 = objective_function(temp_tankstruct,temp_valvestruct);

			temp_valvestruct[temp_count].ValveValues[temp_count2] = (valvecontrol_current[temp_count].ValveValues[temp_count2]-epsilon);
			if(temp_valvestruct[temp_count].ValveValues[temp_count2] < 0) { // valve value cannot be negative
				temp_valvestruct[temp_count].ValveValues[temp_count2] = 0;
			}
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
	for(temp_count =0; temp_count <Ntanks; temp_count++) {
		temp_tankstruct[temp_count].TankLevels[0] = tankcontrol_current[temp_count].TankLevels[0]+epsilon;
		function_value_1 = objective_function(temp_tankstruct,temp_valvestruct);

		temp_tankstruct[temp_count].TankLevels[0] = tankcontrol_current[temp_count].TankLevels[0]-epsilon;
		function_value_2 = objective_function(temp_tankstruct,temp_valvestruct);

		tankcontrol_gradient[temp_count].TankLevels[0] = (function_value_1-function_value_2)/(2*epsilon);
		temp_tankstruct[temp_count].TankLevels[0] = tankcontrol_current[temp_count].TankLevels[0]; //not necessary in gradient compuation as function values are separable in the tanklevels. Nevertheless to allay any fears.
	}
}

void update_control(struct TankStruct *tankcontrol_current, struct ValveStruct *valvecontrol_current,struct TankStruct *tankcontrol_gradient,struct ValveStruct *valvecontrol_gradient, int main_iteration_count)
{
	int temp_count;
	int temp_count2;
	int timeperiod,
	    iteration_count = 0;
	double alpha_mult = 1,
	       rho = 0.6,
	       c = 0.001,
	       temp_valve_value = 0,
	       norm_grad_var=0.0,
	       beta = 0.6;
	float	random_number = 0.0;


	double  function_value_current = 0.0,
	        function_value_next = 0.0;

	timeperiod = tankcontrol_current->TimePeriod;

	if(main_iteration_count==1) {
		printf("\n Simulation Parameters\n rho = %f. c = %f, beta = %f",rho,c,beta);
	}
	memcpy(temp_tankstruct,tankcontrol_current,Ntanks*sizeof(struct TankStruct));
	memcpy(temp_valvestruct,valvecontrol_current,Nvalves*sizeof(struct ValveStruct));

	function_value_current = objective_function(tankcontrol_current,valvecontrol_current);
	function_value_next = function_value_current;

	// TankValues are not updated.
	alpha_mult = 1;
	norm_grad_var = norm_gradient(tankcontrol_gradient,valvecontrol_gradient);

	//((function_value_next-function_value_current)> -c *alpha_mult* norm_grad_var) && (iteration_count < myMAXITER)
	while(((function_value_next-function_value_current)>= -c *alpha_mult* norm_grad_var) && (iteration_count < myMAXITER)) {
		//printf("\n Line Search Iter = %f",alpha_mult);

		for(temp_count2=0; temp_count2<timeperiod; temp_count2++) {
			for(temp_count =0; temp_count <Nvalves; temp_count++) {
				//random_direction(&random_number,1);
				temp_valve_value = valvecontrol_current[temp_count].ValveValues[temp_count2]-(valvecontrol_gradient[temp_count].ValveValues[temp_count2])*alpha_mult;
				if(temp_valve_value > 0) {
					temp_valvestruct[temp_count].ValveValues[temp_count2] = temp_valve_value;
				} else {
					temp_valvestruct[temp_count].ValveValues[temp_count2] = 0;
				}
			}
			compute_flows(temp_tankstruct,temp_valvestruct,temp_count2);
		}

		for(temp_count =0; temp_count <Ntanks; temp_count++) {
			// random_direction(&random_number,1);
			temp_tankstruct[temp_count].TankLevels[0] = tankcontrol_current[temp_count].TankLevels[0]-(tankcontrol_gradient[temp_count].TankLevels[0])*alpha_mult;
		}
		//printf("\n alpha mult %f",alpha_mult);
		function_value_next = objective_function(temp_tankstruct,temp_valvestruct);

		alpha_mult*=rho;
		iteration_count++;
	}

	// Add noise
	for(temp_count2=0; temp_count2<timeperiod; temp_count2++) {
		for(temp_count =0; temp_count <Nvalves; temp_count++) {
			random_direction(&random_number,1);
			temp_valve_value = valvecontrol_current[temp_count].ValveValues[temp_count2]-valvecontrol_gradient[temp_count].ValveValues[temp_count2]*(1+random_number*pow((main_iteration_count/100)+1,-0.6))*alpha_mult;

			if(temp_valve_value > 0) {
				temp_valvestruct[temp_count].ValveValues[temp_count2] = temp_valve_value;
			} else {
				temp_valvestruct[temp_count].ValveValues[temp_count2] = 0;
			}

		}
		compute_flows(temp_tankstruct,temp_valvestruct,temp_count2);
	}

	for(temp_count =0; temp_count <Ntanks; temp_count++) {
		random_direction(&random_number,1);
		temp_tankstruct[temp_count].TankLevels[0] = tankcontrol_current[temp_count].TankLevels[0]-(tankcontrol_gradient[temp_count].TankLevels[0])*(1+random_number*pow((main_iteration_count/100)+1,-beta))*alpha_mult;
	}


	if(((main_iteration_count%PRINT_INTERVAL_FUNCTIONVALUE)==0)) {
		printf(" Gradient Norm = %f alpha mult = %f",norm_grad_var,alpha_mult);
	}
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
	for(temp_count=0; temp_count<Nvalves; temp_count++) {
		for(temp_count2=0; temp_count2<timeperiod; temp_count2++) {
			temp_gradient_norm += valvecontrol_gradient[temp_count].ValveValues[temp_count2]*valvecontrol_gradient[temp_count].ValveValues[temp_count2];
		}
	}
	for(temp_count2=0; temp_count2<Ntanks; temp_count2++) {
		temp_gradient_norm += tankcontrol_gradient[temp_count2].TankLevels[0]*tankcontrol_gradient[temp_count2].TankLevels[0];
	}
	return temp_gradient_norm;
}

float max_tank_level(int tank_struct_index)
{
	float area = 0.0,
	      tank_max_Vol,
	      tank_min_Vol,
	      tank_level_LH,
	      feet3_meter3_coversion = pow(12*2.54/100,3);


	tank_max_Vol = Tank[tank_struct_index].Vmax;
	tank_min_Vol = Tank[tank_struct_index].Vmin;
	tank_level_LH = tank_max_Vol-tank_min_Vol;	// This is in feet^3. COnvert to meter^3
	tank_level_LH *= feet3_meter3_coversion;

	//printf("\n Tank Volume %f, %f", tank_level_LH, (Tank[tank_struct_index].Vmax-Tank[tank_struct_index].Vmin)*feet3_meter3_coversion);

	return tank_level_LH;
}

float tank_level_meter3_to_Level(float tank_volume, int tank_struct_index)
{
	float tank_area = 0.0,
	      tank_level_LH,
	      feet2_meter2_coversion = pow(12*2.54/100,2);

	tank_area = Tank[tank_struct_index].A;
	tank_level_LH = tank_volume/(tank_area*feet2_meter2_coversion);	// This is in feet^3. Convert to meter^3
	return tank_level_LH;
}

void random_direction(float *random_vector, int dimension)
{
	const int rand_max = 500;
	while(dimension > 0) {
		random_vector[dimension-1] = rand()%(2*rand_max);
		random_vector[dimension-1] = (random_vector[dimension-1]-rand_max)/rand_max;
		dimension--;
	}
	return ;
}

void display_output(struct TankStruct *tankcontrol_current, struct TankStruct *tankcontrol_gradient, struct ValveStruct *valvecontrol_current, struct ValveStruct *valvecontrol_gradient)
{
	int temp_count;
	int temp_count2;
	int timeperiod = tankcontrol_current[0].TimePeriod;
	
	// Display all the Valve Settings and Tank Levels till elapsed time
	// Elapsed time is the time till the valve or demand settings are made
	printf("\n==============================================================================================\n");
	
	//Printing final Valve values
	for(temp_count = 0; temp_count< Nvalves; temp_count++){
		printf("\n\n\n\n Valve ID = %s",valvecontrol_current[temp_count].ValveID);
		for(temp_count2 = 0; temp_count2 < timeperiod; temp_count2 += hourly_scheduler){
			printf("\n Valve Value [%d] = %f \t Gradient = %f",
				   temp_count2,valvecontrol_current[temp_count].ValveValues[temp_count2],
					valvecontrol_gradient[temp_count].ValveValues[temp_count2]);
		}
	}
	
	//Printing final tank levels
	for(temp_count = 0; temp_count< Ntanks; temp_count++){
		printf("\n\n\n\n Tank ID = %s",tankcontrol_current[temp_count].TankID);
		for(temp_count2 = 0; temp_count2 < timeperiod; temp_count2 += hourly_scheduler){
			printf("\n Tank Level [%d] = %f\%, \t Tank Inflow = %f, \t Tank Outflow = %f",
				   temp_count2,
			       (tankcontrol_current[temp_count].TankLevels[temp_count2])*100/tankcontrol_current[temp_count].MaxTankLevel,
			       tankcontrol_current[temp_count].InFlow[temp_count2],
			       (tankcontrol_current[temp_count].OutFlow[temp_count2] + tankcontrol_current[temp_count].OutFlow_Epanet[temp_count2]));
		}
	}
	printf("\n==============================================================================================\n");
}

int Queueing_Engine(char *f_job_input)
{
	/*
	 * Reads the jobs in the file for each tank demands 
	 * and valve settings for times 0 to timeperiod-1.
	 * f_job_input 	: file name of file containing job demand data. 
	 * Typical file content: demand|12|15|Bademakan|56.254
	 * 						 valve|1|35|68|118
	 * 						 tank|0|0|Bademaken|150.24
	 */
	 FILE* stream = fopen(f_job_input, "rw"); // Open job file for reading
	 char keyword[25];
	 unsigned short int hours, minutes;
	 char id[25]; 
	 float value;
	 char *tok;
	 char line[100];
	 PriorityQ p;
	 printf("\n==============================================================================================\n");
	 printf("\nChecking for jobs from the file\n");
	 
	 if (stream == NULL){
		printf("Cannot open the file\n");
		printf("\n==============================================================================================\n");
		return 0;
	 }
	 
	 // Reads one line at a time
	 while(fgets(line,100,stream) != NULL){
		tok = strtok(line,"|");
		
		// Seperate the keyword
		printf("%s ",tok);
		strcpy(keyword,tok);
		
		// Separate the hours
		tok = strtok(NULL ,"|");
		printf("%s ",tok);
		hours = atoi(tok);
		
		// Separate the minutes
		tok = strtok(NULL ,"|");
		printf("%s ",tok);
		minutes = atoi(tok);
		
		// Separate the ID
		tok = strtok(NULL ,"|");
		printf("%s ",tok);
		strcpy(id,tok);
		
		// Separate the value
		tok = strtok(NULL ,"|");
		printf("%s \n",tok);
		value = atof(tok);
		
		// Pushing data into Queue
		Qpush(keyword,hours,minutes,id,value);
	}
	fclose(stream);
	//remove(f_job_input);
	printf("\n==============================================================================================\n");
	return 1;
}

void Qpush(char *keyword, int hours, int minutes, char *id, float value)
{
 int i;       /* Function for Insert operation */
 if( Qfull()) {printf("\n\n Overflow!!!!\n\n");}
 else
 {
  i=r;
  ++r;
  while(PQ[i].hours >= hours && i >= 0) /* Find location for new elem */
  {
	if(PQ[i].hours == hours && PQ[i].minutes < minutes)
	{	
		break;
	}
	PQ[i+1]=PQ[i];
	i--;
	  
  }
  strcpy(PQ[i+1].keyword,keyword);
  PQ[i+1].hours=hours;
  PQ[i+1].minutes=minutes;
  strcpy(PQ[i+1].id,id);
  PQ[i+1].value=value;
 }
}
 
PriorityQ Qpop()
{                      /* Function for Delete operation */
 PriorityQ p;
 if(Qempty())
	{ 
		printf("\n\nUnderflow!!!!\n\n");
		p.hours=-1;p.minutes=-1;
		return(p); 
	}
 else
 {
  p=PQ[f];
  f=f+1;
  return(p);
 }
}
 
int Qfull()
{                     /* Function to Check Queue Full */
 if(r==SIZE-1) return 1;
 return 0;
}
 
int Qempty()
{                    /* Function to Check Queue Empty */
 if(f > r) return 1;
 return 0;
}

void Qdisplay()
{                  /* Function to display status of Queue */
 int i;
 if(Qempty()) printf(" \n Empty Queue\n");
 else
 {
  printf("Front->");
  for(i=f;i<=r;i++)
   printf("[%s,%d,%d,%s,%f] ",PQ[i].keyword,PQ[i].hours,PQ[i].minutes,PQ[i].id,PQ[i].value);
  printf("<-Rear\n");
 }
}


int Job_Handler(struct TankStruct *tankcontrol, struct ValveStruct *valvecontrol)
{
	char keyword[25];
	unsigned short int hours, minutes;
	char id[25]; 
	float value;
	int temp_count2;
	int temp_count;
	int simulation_time;
	int time;
	int flag = 1;
	PriorityQ p;
	
	// Handling Jobs
	while(!Qempty()){
		p=Qpop(); //Popping the first element from the queue
		/*
		if(flag == 1){
			time = p.hours;
			flag = 0;
		}
		
		if(time == p.hours){
		*/
		if(!strncmp(p.keyword,"demand",6)){
				printf("[%s,%d,%d,%s,%f] \n",p.keyword,p.hours,p.minutes,
						p.id,p.value);
				for(temp_count2 = 0; temp_count2 < Ntanks; temp_count2++){
					if(!strncmp(tankcontrol[temp_count2].TankID,p.id,15)){
						for(temp_count = p.hours; temp_count < tankcontrol[temp_count2].TimePeriod; temp_count++){
							tankcontrol[temp_count2].OutFlow[temp_count] = p.value;
						}
						break;
					}
					
				}
					
			}
			else if(!strncmp(p.keyword,"valve",5)){
				printf("[%s,%d,%d,%s,%f] \n",p.keyword,p.hours,p.minutes,
						p.id,p.value);
				for(temp_count2 = 0; temp_count2 < Nvalves; temp_count2++){
					if(!strncmp(valvecontrol[temp_count2].ValveID,p.id,2)){
						for(temp_count = p.hours; temp_count < valvecontrol[temp_count2].TimePeriod; temp_count++){
							valvecontrol[temp_count2].ValveValues[temp_count] = p.value;
						}
						break;
					}
				}
			}
			else if(!strncmp(p.keyword,"tank",4)){
				printf("[%s,%d,%d,%s,%f] \n",p.keyword,p.hours,p.minutes,
						p.id,p.value);
				for(temp_count2 = 0; temp_count2 < Ntanks; temp_count2++){
					if(!strncmp(tankcontrol[temp_count2].TankID,p.id,15)){
						tankcontrol[temp_count2].TankLevels[0] = p.value;
						break;
					}
					
				}
			}
		/*
		}
		 To be used when simulating for particular time
		else{
			Qpush(p.keyword,p.hours,p.minutes,p.id,p.value);
			break;
		}
		*/
	}
	
	//Qdisplay();
	// Calculate the simulation time. 
	// sim_time = job(seconds) - clock(seconds)
	//simulation_time = ((p.hours * 60) + p.minutes) - elapsed_time; 
	//simulation_time = time - elapsed_time; // If only hourly manipulations are possible
	
	// shift the clock to the job time
	//elapsed_time = ((p.hours * 60) + p.minutes);
	//elapsed_time = time;
	//return(simulation_time);
	return 1;
}

void Job_Scheduler(struct TankStruct *tankcontrol, struct ValveStruct *valvecontrol)
{
	int temp_count2;
	int temp_count;
			
	PriorityQ p;
	
	// Scheduling Jobs

	// Initial Tank Settings being pushed to priority queue
	for(temp_count2 = 0; temp_count2 < Ntanks; temp_count2++){
		Qpush("tank",0,0,tankcontrol[temp_count2].TankID,tankcontrol[temp_count2].TankLevels[0]);
		printf("[%s,%d,%d,%s,%f] \n","tank",0,0,tankcontrol[temp_count2].TankID,tankcontrol[temp_count2].TankLevels[0]);
	}
	
	// Valve settings being pushed to Priority Queue
	for(temp_count2 = 0; temp_count2 < Nvalves; temp_count2++){
		for(temp_count = 0; temp_count < valvecontrol[temp_count2].TimePeriod; temp_count += hourly_scheduler){
			Qpush("valve",temp_count,0,valvecontrol[temp_count2].ValveID,valvecontrol[temp_count2].ValveValues[temp_count]);
			printf("[%s,%d,%d,%s,%f] \n","valve",temp_count,0,valvecontrol[temp_count2].ValveID,valvecontrol[temp_count2].ValveValues[temp_count]);
		}
	}
	// Check the queue
	Qdisplay();	
	
	// shift the clock to the first job time
	//elapsed_time = ((p.hours * 60) + p.minutes);
  	p = Qpop();
	elapsed_time = p.hours;
	Qpush(p.keyword, p.hours, p.minutes, p.id, p.value);
}
