/** @file main.c
*
* @brief dijkstra's algorithm test file
*
* @author Alvaro Prieto
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include "dijkstra.h"

#define AP (0)
#define S1 (1)
#define S2 (2)
#define R1 (3)

void sigint_handler( );

int32_t main( int argc, char *argv[] )
{    
  // Handle interrupt events to make sure files are closed before exiting
  (void) signal( SIGINT, sigint_handler );

  // Test structure consists of two nodes and a relay.
 
  // S1, S2, R1, AP
  add_labeled_node( AP, 0, "AP" );
  add_labeled_node( S1, 0, "S1" );
  add_labeled_node( R1, 1, "R1" );
  add_labeled_node( S2, 0, "S2" );

  // S1->R, S1->AP, S2->R, S2->AP, R1->AP
  add_link( S1, R1, 2.0 );
  add_link( S1, AP, 1.0 );
  add_link( S2, R1, 0.5 );
  add_link( S2, AP, 2.0 );
  add_link( R1, AP, 1.0 ); 
  
  printf("\nAdded links:\n");
  
  // Display all connections
  print_all_links();
  
  printf("\n Running dijkstra's algorithm.\n");
  
  // Find least expensive route from (source) to AP    
  dijkstra( AP );

  print_shortest_path( S1 );
  print_shortest_path( S2 );

#ifdef DEBUG_ON
  cleanup_node_labels();
#endif  
  
  return 0;
}

/*!
  @brief Handle interrupt event (SIGINT) so program exits cleanly
*/
void sigint_handler( int32_t sig ) 
{
#ifdef DEBUG_ON
  cleanup_node_labels();
#endif

  printf("\nExiting...\n");
  exit(sig);
}

