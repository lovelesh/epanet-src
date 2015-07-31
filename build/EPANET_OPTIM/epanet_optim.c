#include <stdio.h>
#include "epanet2.h"
#include<math.h>
#define MAXTANK = 10 
int main(char *f_epanet_input, char *f_outflow_input, char* f_final_result)
{
  char blank[] = "";
  long t, tstep;
  char *f_epanet_output = "epanet_out.txt";
  FILE ouflow = NULL;
  FILE optim_solutions = NULL;
  
   int 	T, 		/* Optimisation Time Period*/
	num_tanks,  	/* Number of Tanks*/
	num_valves,  	/* Number of Valves*/
	num_nodes,	/* Number of nodes*/
	i,		 /* Count variable 1*/	
	t,		 /* Count variable 2*/	
	p,		 /* Count variable 3*/	
	tank_index[MAXTANK], /*Node Index of Each Tank*/
	valve_index[MAXVALVE], /* Link Index of Each Valve*/
	valve_link_id;	 /* Valve Link Id*/
	
 double level_tank[T+1][num_tank], 	/* Level in Tank*/
	valve_par[T][num_valve],	/* Valve Parameter*/	
	inflow[T][num_tank],		/* Inflow*/
	outflow[T][num_tank],		/* Outflow*/
	epsilon=0.1;			/* Gradient pertubation*/
		
 double fun_value[num_tank+num_valve*T+1],	
	capacity_tank[num_tank],
	weight[T+1],
	grad_fun[num_tank+T*num_valve];

ENopenH(f_epanet_input, f_epanet_output, blank);  	/* Open epanet input file*/
ENinitH(10);						/* Initialise Hydraulics*/
ENgetcount(EN_NODECOUNT, &num_nodes);
ENgetcount(EN_TANKCOUNT, &num_tanks);			/* Get Number of Tanks*/
ENgetcount(EN_VALVECOUNT, &num_valves);			/* Get number of valves*/

/* Things to be done. Open ouflow file. And populate outflow for correct tank.*/
 //openfiles_valve_demand( *f_demand, *f_demand_report);


/* Things to be done
 1) iterate over all nodes and find tank index's using ENgetnodetype and store the index in tank_index
 2) iterate over all links and find valve index using ENgetlinktype in valve_index*/

	
//a)initialize outflow and epsilon. b)initialize  level_tank[.][0] and valve_par[.][.]., capacity, weights,num_tank, num_valve, T.
//temporary initializing begins. this part should be removed from the code.
	for ( i=0;i<=T;i++ )
	{
		for ( p=0;p<num_tank;p++ )
		{
			level_tank[i][p]=Tank[p].H0-Tank[p].Hmin; /* Initial Tank Level*/
		}
	}
	for ( i=0;i<T;i++ )
	{
		for ( p=0;p<num_valve;p++ )
		{	
			valve_link_id = Valve[p].Link;
			valve_par[i][p]=Link[valve_link_id].Kc;
		}
	}
	for ( i=0;i<T;i++ )
	{
		for ( p=0;p<num_tank;p++ )
		{
			inflow[i][p]=10;
		}
	}
	for ( i=0;i<T;i++ )
	{
		for ( p=0;p<num_tank;p++ )
		{
			outflow[i][p]=5;
		}
	}

	for ( i=0;i<num_tank;i++ )
	{
		/*Need a time variable to denote the time between valve changes. 
		 Then total inflow volume is inflow*time_period. Time period will determine 
		 the maximum capacity of tank.*/
		capacity_tank[i]=110; /* Need to calculate from Tank Volume*/
	}
	for ( i=0;i<=T;i++ )
	{
		weight[i]=1;
	}
//temporary initializing ends.this part should be removed from the code.


	for ( i=0;i<=num_tank+T*num_valve;i++ )
	{
		if ( i < num_tank )
		{
			level_tank[0][i]+=epsilon;
			// Set this tank value in epanet using 
		}
		else if ( i<num_tank+num_valve*T )
		{
		  valve_par[ ( i-num_tank ) /num_valve][ ( i-num_tank ) %num_valve]+=epsilon;
		}
		
		  /* Call EPANET to get function value*/
		  ENinitH(10);
		  do{
		      ENrunH(&t);
		      /* Retrieve hydraulic results for time t  */
		      ENnextH(&tstep);
		     }while(tstep>0);
		
		for ( t=1;t<=T;t++ )
		{
			for ( p=0;p<=num_tank-1;p++ )
			{
				level_tank[t][p]=level_tank[t-1][p]+inflow[t-1][p]-outflow[t-1][p];
			}
		}
//evaluate the objective function value


		fun_value[i]=eval ( ( double * ) level_tank, capacity_tank, weight, T, num_tank );
		printf ( "%f",fun_value[i] );
		if ( i < num_tank )
			level_tank[0][i]-=epsilon;
		else if ( i<num_tank+num_valve*T )
			valve_par[ ( i-num_tank ) /num_valve][ ( i-num_tank ) %num_valve]-=epsilon;
	}
//calculate the gradient
	for ( i=0;i<num_tank+T*num_valve;i++ )
	{
		grad_fun[i]= ( fun_value[i]-fun_value[num_tank+T*num_valve] ) /epsilon;
	}
	return 0;
}

double eval ( double *L, double *C, double *W, int T, int K )
{
	double temp_1=0,temp_2=0,phi;
	int i,t,p;
	for ( i=0;i<K;i++ )
	{
		temp_1+=fabs ( * ( L+i ) - * ( L+T*K+i ) );
		temp_1*=*W;
	}
	for ( t=1;t<=T;t++ )
	{
		for ( p=0;p<K;p++ )
		{
			if ( * ( L+t*K+p ) <0 )
				phi=fabs ( * ( L+t*K+p ) );
			else if ( * ( L+t*K+p ) >* ( C+p ) )
				phi=* ( L+t*K+p )-* ( C+p );
			else
				phi=0;
			temp_2+=phi;
		}
		temp_2*=* ( W+t );
	}

	return temp_1+temp_2;
}

