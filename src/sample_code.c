
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

				 
int main(int argc, char *argv[])
{
   char * f_epanet_input;
   char * f_epanet_output = "temp.out";
   char blank[] = "";
   
   int temp_count;
   int temp2_count;
   int epsilon = 0.1;
   
   char *temp_LinkID;
   double *temp_valve_diameter;
   double *temp_valve_flow;
   
  int errorcode;
  int errcode;
  long t, tstep;
  int temp_code;
  float temp_setting;
  
  
  f_epanet_input = argv[1];
  if(argc > 2) f_epanet_output = argv[2];
  
  printf("\n Input File %s \n Output File = %s",f_epanet_input, f_epanet_output);	
  errorcode = ENopen(f_epanet_input, f_epanet_output, blank);
  
  // Print Network Parameters
  printf("\nNumber of Tanks = %d\t Number of Valves = %d\n",Ntanks,Nvalves);
  
  for(temp_count = 1; temp_count <= Ntanks; temp_count++)
  {
    printf("\n %d Node Index = %d Node ID = %s Tank Elev = %d ",temp_count, Tank[temp_count].Node, Node[Tank[temp_count].Node].ID, Node[Tank[temp_count].Node].El );
    // printf("%f %f %f :",Tank[temp_count].H0, Tank[temp_count].Hmin, Tank[temp_count].Hmax );
    for(temp2_count = 1; temp2_count <= Nlinks;temp2_count++)
    {
      if(Link[temp2_count].N1 == Tank[temp_count].Node) {printf(" Output_Type = %c, Output_ID = %s", Link[temp2_count].Type,  Link[temp2_count].ID  );}
      if(Link[temp2_count].N2 == Tank[temp_count].Node) {printf(" Input_Type = %c,  Input_ID = %s", Link[temp2_count].Type,  Link[temp2_count].ID  );}
    }
  }
  
            for(temp_count = 1; temp_count <= Nvalves; temp_count++)
      {
	//Link[Valve[temp_count].Link].Kc += -0.1;
	printf("\nValve Id = %d, Valve Setting = %e", Valve[temp_count].Link, Link[Valve[temp_count].Link].Kc);
	
      }

    printf("\n Number of Controls %d", Ncontrols);
  for(temp_count = 1;temp_count <= Ncontrols;temp_count++)
  {
    printf("\n Control Node %d Control Link %d Control Setting %d Control Type %f",Control[temp_count].Link, Control[temp_count].Node, Control[temp_count].Type, Control[temp_count].Setting);
  }
  
  ENsolveH();
  
   for(temp_count = 1; temp_count <= Nlinks; temp_count++)
      {
	    ENgetlinktype(temp_count,&temp_code);
	    if(temp_code > 3)
	    {
	      ENgetlinkvalue(temp_count, 12, &temp_setting);
	      printf("\n Link index = %d, Link Type = %d, Valve Setting = %f", temp_count, temp_code, temp_setting);
	      ENgetlinkvalue(temp_count, 8, &temp_setting);
	      printf(" Valve Flow %f", temp_setting);
	      ENgetlinkvalue(temp_count, 11, &temp_setting);
	      printf(" Valve Status %f", temp_setting);
	    }
	   
	
      }
  
  
  
  
  printf("\n After Computation");
  /*
    for(temp_count = 1; temp_count <= Nvalves; temp_count++)
  {
    printf("\n Link index: %d, Link ID %s, Valve Daimeter = %f, Valve Type = %d, Valve Setting  = %f, Valve Flow = %f ", Valve[temp_count].Link,Link[Valve[temp_count].Link].ID, Link[Valve[temp_count].Link].Diam, Link[Valve[temp_count].Link].Type, Link[Valve[temp_count].Link].Kc, Q[Valve[temp_count].Link]*Ucf[FLOW]);
  }
  */
     for(temp_count = 1; temp_count <= Nlinks; temp_count++)
  {
    printf("\n Link index: %d, Link Flow = %f ", temp_count, Q[temp_count]*Ucf[FLOW]);
      
  }
  
  
  printf("\n\n\n\n Hydraulics Solver using ENopenH. UCF[FLOW] = %f", Ucf[FLOW]);
  
  /* Open hydraulics solver */     
   for(temp2_count = 1; temp2_count <=5;temp2_count++)
   {
     printf("\n \n \n Simulation Iteration %d", temp2_count);
     ENopenH();
         for(temp_count = 1; temp_count <= Nvalves; temp_count++)
      {
//	Link[Valve[temp_count].Link].Kc += 0.1;
//	printf("\n Valve Id = %d, Valve Setting = %e", Valve[temp_count].Link, Link[Valve[temp_count].Link].Kc);
 	ENgetlinkvalue(Valve[temp_count].Link, 5, &temp_setting);
 	ENsetlinkvalue(Valve[temp_count].Link, 5, temp_setting-0.1);
 	printf("\n Old Valve Setting = %f",temp_setting);
 	ENgetlinkvalue(Valve[temp_count].Link, 5, &temp_setting);
 	printf(" New Valve Setting = %f", temp_setting);
	printf("\n Valve Id = %d, Valve Setting = %e", Valve[temp_count].Link, Link[Valve[temp_count].Link].Kc);

      }

   ENinitH(10);

      
   for(temp_count = 1; temp_count <= Nlinks; temp_count++)
      {
	    ENgetlinktype(temp_count,&temp_code);
	    if(temp_code > 3)
	    {
	      ENgetlinkvalue(temp_count, 12, &temp_setting);
	      printf("\n Link index = %d, Link Type = %d, Valve Setting = %f", temp_count, temp_code, temp_setting);
	    }
	   
	
      }
   /* Analyze each hydraulic period */
   
//           for(temp_count = 1; temp_count <= Nvalves; temp_count++)
//       {
// 	//Link[Valve[temp_count].Link].Kc += -0.1;
// 	printf("\nValve Id = %d, Valve Setting = %e", Valve[temp_count].Link, Link[Valve[temp_count].Link].Kc);
// 	
//       }
   
    do{
         ENrunH(&t);
         ENnextH(&tstep);
      }
      while (tstep > 0);
     ENcloseH();
       
     
     for(temp_count = 1; temp_count <= Nlinks; temp_count++)
      {
	    ENgetlinktype(temp_count,&temp_code);
	    if(temp_code > 3)
	    {
	      ENgetlinkvalue(temp_count, 8, &temp_setting);
	      printf("\n Link index = %d, Link Type = %d, Valve Flow = %f", temp_count, temp_code, temp_setting);
	    }
	   
	
      }
      
      
//    printf("\n \n \n \nAfter Alteration in valve settings");
//    for(temp_count = 1; temp_count <= Nlinks; temp_count++)
//   {
//     printf("\n Link index: %d, Linkd ID = %s, Link Flow = %f ", temp_count, Link[temp_count].ID ,Q[temp_count]*Ucf[FLOW]);
//       
//   } 

    


 ENsolveH();
 printf("\n Solution using ENsolveH");
     for(temp_count = 1; temp_count <= Nlinks; temp_count++)
      {
	    ENgetlinktype(temp_count,&temp_code);
	    if(temp_code > 3)
	    {
	      ENgetlinkvalue(temp_count, 8, &temp_setting);
	      printf("\n Link index = %d, Link Type = %d, Valve Flow = %f", temp_count, temp_code, temp_setting);
	    }
	   
	
      }
//    
} 
  
  
 /*
 // ENsolveQ();
//  ENreport();
  printf("\n \n \n \nAfter Alteration in valve settings");
    for(temp_count = 1; temp_count <= Nvalves; temp_count++)
  {
    printf("\n Link index: %d, Link ID %s, Valve Daimeter = %f, Valve Type = %d, Valve Setting  = %f, Valve Flow = %f ", Valve[temp_count].Link,Link[Valve[temp_count].Link].ID, Link[Valve[temp_count].Link].Diam, Link[Valve[temp_count].Link].Type, Link[Valve[temp_count].Link].Kc, Q[Valve[temp_count].Link]*Ucf[FLOW]);
  }
  */
 
 
  ENclose();

}                                     
