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
#define S3 (4)
#define R2 (5)

#define R (0)
#define S (1)
#define T (2)
#define U (3)
#define V (4)
#define W (5)
#define X (6)
#define Y (7)
#define Z (8)

#define TOTAL_ITERATIONS 1000

void sigint_handler( );

int32_t main( int argc, char *argv[] )
{    
  uint32_t loop_counter;
  // Handle interrupt events to make sure files are closed before exiting
  (void) signal( SIGINT, sigint_handler );

#ifdef TEST_1
  // Test structure consists of two nodes and a relay.
  // S1, S2, R1, AP
  add_labeled_node( AP, 0, "AP" );
  add_labeled_node( S1, 0, "S1" );
  add_labeled_node( S2, 0, "S2" );
  add_labeled_node( R1, 1, "R1" );

  // S1->R, S1->AP, S2->R, S2->AP, R1->AP
  add_link( R1, S1, 2.0 );
  add_link( AP, S1, 1.0 );
  add_link( R1, S2, 0.5 );
  add_link( AP, S2, 2.0 );
  add_link( AP, R1, 1.0 ); 
  
  //printf("\nAdded links:\n");
  
  // Display all connections
  //print_all_links();
  
  //printf("\nRunning dijkstra's algorithm.\n");
  
  // Initialize algorithm
  initialize_node_energy( AP );

  print_node_energy( AP );

  for( loop_counter = 0; loop_counter < TOTAL_ITERATIONS; loop_counter++ )
  {

#ifndef REGULAR_DIJKSTRA
    // Calculate link costs before running dijkstra's algorithm
    calculate_link_costs();
#endif

    // Find least expensive route from (source) to AP    
    dijkstra( AP );
  
    //print_all_links();
  
    print_shortest_path( S1 );
    print_shortest_path( S2 );
    
    compute_mean_energy( AP );
    
    print_node_energy( AP );
  }
  
#endif


#ifdef TEST_2
  // Test structure consists of two nodes and a relay.
  // S1, S2, R1, AP
  add_labeled_node( AP, 0, "AP" );
  add_labeled_node( S1, 0, "S1" );
  add_labeled_node( R1, 1, "R1" );
  add_labeled_node( S2, 0, "S2" );
  add_labeled_node( R2, 1, "R2" );
  add_labeled_node( S3, 0, "S3" );

  // S1->R, S1->AP, S2->R, S2->AP, R1->AP  
  add_link( S1, AP, 8.0 );
  add_link( S1, R1, 4.0 );
  add_link( S1, R2, 7.0 );
  
  add_link( S2, AP, 5.0 );
  add_link( S2, R1, 2.0 );
  add_link( S2, R2, 4.0 );
  
  add_link( S3, AP, 10.0 );
  add_link( S3, R1, 5.0 );
  add_link( S3, R2, 5.0 );
  
  add_link( R1, AP, 3.0 ); 
  add_link( R2, AP, 1.0 ); 
  
  add_link( R1, R2, 1.0 ); 
  
  
  printf("\nAdded links:\n");
  
  // Display all connections
  print_all_links();
  
  printf("\nRunning dijkstra's algorithm.\n");
  
  // Find least expensive route from (source) to AP    
  dijkstra( AP );

  print_shortest_path( S1 );
  print_shortest_path( S2 );
  print_shortest_path( S3 );
#endif


#ifdef TEST_3
  // Test structure consists of two nodes and a relay.
  // S1, S2, R1, AP
  add_labeled_node( S, 0, "S" );
  add_labeled_node( T, 0, "T" );
  add_labeled_node( U, 0, "U" );
  add_labeled_node( V, 0, "V" );
  add_labeled_node( W, 0, "W" );
  add_labeled_node( X, 0, "X" );
  add_labeled_node( Y, 0, "Y" );
  add_labeled_node( Z, 0, "Z" );

  // S1->R, S1->AP, S2->R, S2->AP, R1->AP

  add_link( U, S, 4 );
  add_link( U, W, 3 );  
  add_link( V, U, 3 );
  add_link( V, W, 4 );  
  add_link( X, V, 3 );
  add_link( X, W, 6 );
  add_link( T, S, 1 );
  add_link( T, V, 4 );
  add_link( T, U, 2 );
  add_link( Y, T, 7 );
  add_link( Y, V, 1 );
  add_link( Y, X, 6 );
  add_link( Z, T, 5 );
  add_link( Z, Y, 12 );
  
  
  printf("\nAdded links:\n");
  
  // Display all connections
  print_all_links();
  
  printf("\nRunning dijkstra's algorithm.\n");
  
  // Find least expensive route from (source) to AP    
  dijkstra( Z );

  print_shortest_path( S );
  print_shortest_path( T );
  print_shortest_path( U );
  print_shortest_path( V );
  print_shortest_path( W );
  print_shortest_path( X );
  print_shortest_path( Y );
#endif


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

