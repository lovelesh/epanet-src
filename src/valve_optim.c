
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

// Decalration of Global Variables 
  /* Initialize file pointers to NULL */
   FILE InFile_valve_demand = NULL;
   FILE RptFile_valve_demand = NULL;
   FILE OutFile_valve_demand = NULL;
   FILE HydFile_valve_demand = NULL;
   
// Declare File name copies
   char *InpFname_valve_demand;
   char *Rpt1Fname_valve_demand;
   char *OutFname_valve_demand;   
   char Outflag_valve_demand;


double eval ( double *L, double *C, double *W, int T, int K );
/*
----------------------------------------------------------------
   Functions for opening files 
----------------------------------------------------------------
*/


int   openfiles_valve_demand(char *f1, char *f2);
/*----------------------------------------------------------------
                                 /* End of openfiles */

				 
int main(char *f_epanet_input, char *f_outflow_input, char* f_final_result)
{
   int 	T,
	num_tank,
	num_valve,
	i,
	t,
	p;
	
 double level_tank[T+1][num_tank],
	valve_par[T][num_valve],
	inflow[T][num_tank],
	outflow[T][num_tank],
	epsilon=0.1;
		
 double fun_value[num_tank+num_valve*T+1],
	capacity_tank[num_tank],
	weight[T+1],
	grad_fun[num_tank+T*num_valve];
	
   openfiles_valve_demand( *f_outflow_input, *f_demand_report);
   num_tanks = Ntanks;
   num_valve = Nvalves;
   
	
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
			level_tank[0][i]+=epsilon;
		else if ( i<num_tank+num_valve*T )
			valve_par[ ( i-num_tank ) /num_valve][ ( i-num_tank ) %num_valve]+=epsilon;
/* Now we need to call ENSolveH to get new function values.
 */			ENinitH(11);
//call function with this modified input variable which output the inflow. now use the inflow to calculate the level_tank for all t.
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

/*
----------------------------------------------------------------
   Functions for opening files 
----------------------------------------------------------------
*/


int openfiles_valve_demand(char* f1, char* f2)
/*----------------------------------------------------------------
**  Input:   f1 = pointer to name of input file                  
**           f2 = pointer to name of report file                 
**           f3 = pointer to name of binary output file          
**  Output:  none
**  Returns: error code                                  
**  Purpose: opens input & report files                          
**----------------------------------------------------------------
*/
{

   
/* Save file names */
   strncpy(InpFname_valve_demand,f1,MAXFNAME);
   strncpy(Rpt1Fname_valve_demand,f2,MAXFNAME);



/* Check that file names are not identical */
   if (strcomp(f1,f2))
   {
      writecon(FMT04);
      return(301);
   }

/* Attempt to open input and report files */
   if ((InFile_valve_demand = fopen(f1,"rt")) == NULL)
   {
      writecon(FMT05);
      writecon(f1);
      return(302);
   }
   if (strlen(f2) == 0) RptFile = stdout;
   else if ((RptFile_valve_demand = fopen(f2,"wt")) == NULL)
   {
      writecon(FMT06);
      return(303);
   }

   return(0);
}                                       /* End of openfiles */
