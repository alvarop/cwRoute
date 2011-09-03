/** @file routing.c
*
* @brief Get rssi table and produce routing and power tables
*
* @author Alvaro Prieto
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "dijkstra.h"

#include "routing.h"

#define INBUFSIZE (512)

void clean_table( energy_t p_rssi_table[][MAX_DEVICES+1] );
void add_links_from_table( energy_t p_rssi_table[][MAX_DEVICES+1] );
void print_rssi_table();
double dbm_to_watt( double power );
double watt_to_dbm( double power );

energy_t target_rssi;
energy_t rssi_table[MAX_DEVICES+1][MAX_DEVICES+1];

void routing_initialize()
{    
  char node_id_string[3];
  uint8_t node_index;
  
  target_rssi = dbm_to_watt(-75l);
  
  // Add nodes 
  for( node_index = 0; node_index < (MAX_DEVICES + 1); node_index++ )
  {
    sprintf( node_id_string, "%d", node_index );
    add_labeled_node( node_index, 0, node_id_string );
  }
  
  pthread_mutex_init( &mutex_route, NULL );
  
  // Start mutex locked
  pthread_mutex_lock ( &mutex_route );
  
  initialize_node_energy( 0 );
  
  //print_node_energy( 0, fp_out );
  
  printf("Round 0\n");
  
  return;
}

void *compute_routes_thread()
{
  static uint32_t index;
  uint8_t node_index;
  // loop forever
  for (;;)
  {
    // Block until next table is ready
    pthread_mutex_lock ( &mutex_route );
    
    
    
    // Assuming rssi_table has been updated
    clean_table( rssi_table );       

    add_links_from_table( rssi_table );
    
    // Run dijkstra's algorithm with 0 being the access point
    dijkstra( 0 );
    
    // Display shortest paths and update energies
    for( node_index = 1; node_index < (MAX_DEVICES + 1); node_index++ )
    {
      compute_shortest_path( node_index );
      print_shortest_path( node_index );
    }
    
    //print_all_links();
    
    // Compute engergy mean and subtract from each individual mean
    compute_mean_energy( 0 );    
  
    
    //print_node_energy( 0, fp_out );
    
    //generate_graph( 0, index );
    
    index++;
    printf("\nRound %d\n", index);

  }

  return NULL;
}

/*******************************************************************************
 * @fn    uint8_t parse_table ( int8_t p_rssi_table[][MAX_DEVICES+1] )
 *
 * @brief Get RSSI table, convert, store and print it
 * ****************************************************************************/
uint8_t parse_table ( uint8_t p_rssi_table[][MAX_DEVICES+1] )
{
  uint8_t row_index;
  uint8_t col_index;
  
  // Convert table to rssi values from raw data and copy to local array
  for( row_index = 0; row_index <= MAX_DEVICES; row_index++ )
  {     
    for( col_index = 0; col_index <= MAX_DEVICES; col_index++ )
    {
      rssi_table[row_index][col_index] =
                               rssi_values[p_rssi_table[row_index][col_index]];      
    }
  }  
  
  // Print table if needed
  
  print_rssi_table();
  
  // Did not read table successfully
  return 0;
}

/*******************************************************************************
 * @fn    void clean_table( int8_t rssi_table[][MAX_DEVICES+1] )
 *
 * @brief Compare the RSSI from two links and only keep the best value
 * ****************************************************************************/
void clean_table( energy_t rssi_table[][MAX_DEVICES+1] )
{
  uint16_t col_index, row_index;
  
  for( row_index = 0; row_index < ( MAX_DEVICES+1 ); row_index++ )
  {
    for( col_index = row_index; col_index < ( MAX_DEVICES+1 ); col_index++ )
    {
      if( rssi_table[row_index][col_index] < rssi_table[col_index][row_index] )
      {
        rssi_table[row_index][col_index] = rssi_table[col_index][row_index];
      }
      rssi_table[col_index][row_index] = 128;
    }

  }
  
}

/*******************************************************************************
 * @fn    void add_links_from_table( int8_t rssi_table[][MAX_DEVICES+1] )
 *
 * @brief Generate dijkstra links from cleaned up table
 * ****************************************************************************/
void add_links_from_table( energy_t rssi_table[][MAX_DEVICES+1] )
{
  uint16_t col_index, row_index;
  energy_t link_power;
  energy_t tx_power;
  energy_t alpha;
  
  // Constant tx power for now, will change later
  tx_power = dbm_to_watt(1.5l);
  
  for( row_index = 0; row_index < ( MAX_DEVICES ); row_index++ )
  {
    for( col_index = row_index + 1; col_index < ( MAX_DEVICES+1 ); col_index++ )
    {
      // Compute the minimum power required to meet this link with 'target_rssi'
      // alpha is the channel attenuation, that is received/transmitted power
      alpha = dbm_to_watt( rssi_table[row_index][col_index] ) / tx_power;
      
      // Transmit power required is the target rssi / channel attenuation
      link_power = target_rssi / alpha;
      
      // Add link
      add_link( row_index, col_index, link_power );
    }
  }
}

void print_rssi_table()
{
  uint8_t row_index;
  uint8_t col_index;
  
  printf("   ");
  
  for( col_index = 0; col_index <= MAX_DEVICES; col_index++ )
  {
    printf(" %3d   ", col_index );
  }
  
  printf("\r\n");
  
  // Print packet in hex
  for( row_index = 0; row_index <= MAX_DEVICES; row_index++ )
  { 
    printf("%2d ", row_index );
    
    for( col_index = 0; col_index < MAX_DEVICES; col_index++ )
    {
      printf("%06.1f ", rssi_table[row_index][col_index] );
      //fprintf( main_fp, "%d,", rssi_table[row_index][col_index] );
    }
    
    printf("%06.1f ", rssi_table[row_index][col_index] );
    //fprintf( main_fp, "%d", rssi_table[row_index][col_index] );
    
    printf("\r\n");
    //fprintf( main_fp, "\n" );
  }    

  printf("\r\n");
  //fprintf( main_fp, "\n" );
}

/*******************************************************************************
 * @fn    double dbm_to_watt( double power )
 *
 * @brief Convert power from dBm to Watts
 * ****************************************************************************/
double dbm_to_watt( double power )
{
  return pow(10, power/10l)/1000l;
}

/*******************************************************************************
 * @fn    double watt_to_dbm( double power )
 *
 * @brief Convert power from Watts to dBm
 * ****************************************************************************/
double watt_to_dbm( double power )
{
  return 10l * log10( 1000l * power );
}

