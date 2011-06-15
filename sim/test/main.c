/** @file main.c
*
* @brief cwRoute Testing file
*
* @author Alvaro Prieto
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include "nodes.h"

#define MAX_NODES (4)
#define TOTAL_RELAYS (1)
#define MAX_LINKS (5)

#define AP (0)
#define S1 (1)
#define S2 (2)
#define R1 (3)

node_t nodes[MAX_NODES]; // S1, S2, R1, AP
link_t links[MAX_LINKS]; // S1->R, S1->AP, S2->R, S2->AP, R1->AP

double mean_energy = 0;

void sigint_handler( );

double difference( double number_a, double number_b )
{
  if ( number_a > number_b )
  {     
    return number_a - number_b;
  }
  else
  {
    return number_b - number_a;
  }  
}

void print_node_name( uint8_t node_id )
{
  switch(node_id) 
    {
      case AP:
      {
        printf("AP");
        break;
      }
      case S1:
      {
        printf("S1");
        break;
      }
      case S2:
      {
        printf("S2");
        break;
      }
      case R1:
      {
        printf("R1");
        break;
      }
      default:
      {
        break;
      }  
    }
    
    return;
}

void print_node_energy( )
{
  uint8_t node_index;
  
  printf("MEAN  ");
  
  for( node_index = 1; node_index < MAX_NODES; node_index++ )
  {
    print_node_name( nodes[node_index].id );
    printf("    ");      
  }
    
  printf("\n");
  
  printf("%0.3f ", mean_energy );
  
  for( node_index = 1; node_index < MAX_NODES; node_index++ )
  {    
    printf("%0.3f ", nodes[node_index].energy );      
  }
  
  printf("\n");
  
}

int main( int argc, char *argv[] )
{   

  uint8_t node_index;  
  uint8_t link_index;
 
  // Handle interrupt events to make sure files are closed before exiting
  (void) signal( SIGINT, sigint_handler );

  // Test structure consists of two nodes and a relay.
  
  // Initialize nodes and links
  for( node_index = 0; node_index < MAX_NODES; node_index++ )
  {
    nodes[node_index].distance = 1e99;
    nodes[node_index].energy = 0;
    nodes[node_index].visited = 0;
    nodes[node_index].id = node_index;
    nodes[node_index].previous = node_index;
    nodes[node_index].is_relay = 0;   
    
  }
  
  nodes[R1].is_relay = 1;

  for( link_index = 0; link_index < MAX_LINKS; link_index++ )
  {
    links[link_index].current_cost = 0; 
  }
  
  // Create S1->R
  links[0].links_power = 2.0;
  links[0].source = S1;
  links[0].destination = R1;

  // Create S1->AP
  links[1].links_power = 1.0;
  links[1].source = S1;
  links[1].destination = AP;
  
  // Create S2->R
  links[2].links_power = 0.5;
  links[2].source = S2;
  links[2].destination = R1;

  // Create S2->AP
  links[3].links_power = 2.0;
  links[3].source = S2;
  links[3].destination = AP;  
  
  // Create R1->AP
  links[4].links_power = 1.0;
  links[4].source = R1;
  links[4].destination = AP; 

  
  // Display all connections
  for( link_index = 0; link_index < MAX_LINKS; link_index++ )
  {
    print_node_name( links[link_index].source );
    printf("-");
    print_node_name( links[link_index].destination );
    printf(" %0.2f", links[link_index].links_power);
    printf("\n");
    
    // Initialization uses energy from direct links to AP
    if( links[link_index].destination == AP
      && !nodes[links[link_index].source].is_relay )
    {
      nodes[links[link_index].source].energy = links[link_index].links_power;
      mean_energy += links[link_index].links_power;
    }
  }
  
  // Compute the initial mean
  mean_energy /= ( MAX_NODES - 1 - TOTAL_RELAYS );
  
  print_node_energy( );
  
  // Start algorithm here
  
  // Calculate link cost
  for( link_index = 0; link_index < MAX_LINKS; link_index++ )
  {
    links[link_index].current_cost = nodes[links[link_index].source].energy;
    links[link_index].current_cost = 
      difference( links[link_index].current_cost, mean_energy );
      
  }
  
  for( link_index = 0; link_index < MAX_LINKS; link_index++ )
  {
    print_node_name( links[link_index].source );
    printf("-");
    print_node_name( links[link_index].destination );
    printf(" %0.2f", links[link_index].current_cost);
    printf("\n");
  }
  
  
  // Run dijkstra's algorithm for S1  
  for( node_index = 0; node_index < MAX_NODES; node_index++ )
  {
    
    
  }
  
  // Run dijkstra's algorithm for S2

  return 0;
}



/*!
  @brief Handle interrupt event (SIGINT) so program exits cleanly
*/
void sigint_handler( int32_t sig ) 
{
    printf("\nExiting...\n");
    exit(sig);
}

