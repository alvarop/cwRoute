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

#include "routing.h"

#define INBUFSIZE (512)

void clean_table( energy_t p_rssi_table[][MAX_DEVICES+1] );
void add_links_from_table( energy_t p_rssi_table[][MAX_DEVICES+1] );
void compute_required_powers( energy_t*, uint8_t* );
void print_rssi_table();
energy_t get_power_from_setting( uint8_t setting );
uint8_t find_closest_power( energy_t power );
double dbm_to_watt( double power );
double watt_to_dbm( double power );

energy_t target_rssi;
energy_t rssi_table[MAX_DEVICES+1][MAX_DEVICES+1];
energy_t link_power_table[MAX_DEVICES+1][MAX_DEVICES+1];


static energy_t previous_powers[MAX_DEVICES];  

#define AP_NODE_ID (MAX_DEVICES+1)

void routing_initialize()
{    
  char node_id_string[3];
  uint8_t node_index;
  
  target_rssi = dbm_to_watt(-75l);
  
  // Add nodes 
  sprintf( node_id_string, "AP" );
  add_labeled_node( AP_NODE_ID, 0, node_id_string );
  
  for( node_index = 0; node_index < MAX_DEVICES; node_index++ )
  {
    sprintf( node_id_string, "%d", ( node_index + 1 ) );
    add_labeled_node( ( node_index + 1 ), 0, node_id_string );
    
    // Initialize previous power to maximum
    previous_powers[node_index] = 
                    power_values[sizeof(power_values)/sizeof(energy_t) - 1];
  }
  
  pthread_mutex_init( &mutex_route_start, NULL );
  pthread_mutex_init( &mutex_route_done, NULL );
  
  // Start mutex locked
  pthread_mutex_lock ( &mutex_route_start );
  
  initialize_node_energy( AP_NODE_ID );
  
  //print_node_energy( 0, fp_out );
  
  printf("Round 0\n");
  
  return;
}

void *compute_routes_thread( void *rp_tables )
{
  static uint32_t index;
  uint8_t *route_table = &((uint8_t*)rp_tables)[0];
  uint8_t *power_table = &((uint8_t*)rp_tables)[MAX_DEVICES];
  energy_t link_powers[MAX_DEVICES];
  
  uint8_t node_index;
  for( node_index = 0; node_index < MAX_DEVICES; node_index++ )
    {
     
      printf("%5.1f ", previous_powers[node_index] );
    }
    printf("<--\n");
  
  // loop forever
  for (;;)
  {
    // Block until next table is ready
    pthread_mutex_lock ( &mutex_route_start );
            
    // Assuming rssi_table has been updated
    clean_table( rssi_table );       

    add_links_from_table( rssi_table );
    
    // Run dijkstra's algorithm with 0 being the access point
    dijkstra( AP_NODE_ID );
    
    // Display shortest paths and update energies
    for( node_index = 1; node_index < (MAX_DEVICES + 1); node_index++ )
    {
      compute_shortest_path( node_index );
      print_shortest_path( node_index );
    }
    
    // Compute routing table
    compute_rp_tables( route_table, link_powers );
    
    
    // Compute power table
    compute_required_powers( link_powers, power_table );
    
    for( node_index = 0; node_index < MAX_DEVICES; node_index++ )
    {
      //printf("%02X ", power_table[node_index] );
      printf("%g(0x%02X) ", previous_powers[node_index], power_table[node_index] );
    }
    printf("\n");
    
    //print_all_links();
    
    // Compute engergy mean and subtract from each individual mean
    compute_mean_energy( AP_NODE_ID );    
     
    //print_node_energy( 0, fp_out );
    
    //generate_graph( 0, index );
    
    index++;
    printf("\nRound %d\n", index);
    
    // Block until next table is ready
    pthread_mutex_unlock ( &mutex_route_done );

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
  energy_t tx_power;
  energy_t alpha;
  
  // Convert table to rssi values from raw data and copy to local array
  for( row_index = 0; row_index <= MAX_DEVICES; row_index++ )
  {     
    for( col_index = 0; col_index <= MAX_DEVICES; col_index++ )
    {
      rssi_table[row_index][col_index] =
                               rssi_values[p_rssi_table[row_index][col_index]];
                               
      if( 0 == col_index )
      {
        // AP always transmits with max power
        tx_power = dbm_to_watt( get_power_from_setting( 0xff ) );
      }
      else
      {
        // Use previous transmit settings
        tx_power = dbm_to_watt( previous_powers[col_index + 1] );
      } 
      
      //tx_power = dbm_to_watt( 1.5l );
      
      // Compute the minimum power required to meet this link with 'target_rssi'
      // alpha is the channel attenuation, that is received/transmitted power
      alpha = dbm_to_watt( rssi_table[row_index][col_index] ) / tx_power;
      
      // Transmit power required is the target rssi / channel attenuation
      link_power_table[row_index][col_index] = target_rssi / alpha;
                                     
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
      if( link_power_table[row_index][col_index] <
                                      link_power_table[col_index][row_index] )
      {
        link_power_table[row_index][col_index] = 
                                        link_power_table[col_index][row_index];
      }

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
  uint16_t col_index, row_index, source, destination;
 
  
  for( row_index = 0; row_index < ( MAX_DEVICES ); row_index++ )
  {
    
    for( col_index = row_index + 1; col_index < ( MAX_DEVICES+1 ); col_index++ )
    {

      source = row_index;      
      destination = col_index;
      
      // Make sure the access point has the correct address
      if( 0 == source )
      {
        source = AP_NODE_ID;
      }    
      if( 0 == destination )
      {
        destination = AP_NODE_ID;
      }
      
      // Add link
      add_link( source, destination, link_power_table[row_index][col_index] );
    }
  }
}

void compute_required_powers( energy_t* link_powers, uint8_t* power_table )
{
  uint8_t node_index;

  
  for( node_index = 0; node_index < MAX_DEVICES; node_index++ )
  {
    
    // Store required power in power table
    power_table[node_index] = 
                find_closest_power( watt_to_dbm( link_powers[node_index] ) );
    
    // Save current required power to be used as tx_power next round
    previous_powers[node_index] = 
                            get_power_from_setting( power_table[node_index] );
      
  
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


energy_t get_power_from_setting( uint8_t setting )
{ 
  uint8_t index;
  for( index = 0; index < sizeof(power_settings); index++ )
  {
    if( setting == power_settings[index] )
    {
      return power_values[index];
    }
  }
  
  printf("Power not found!\n");
  
  // In case the power isn't found, default to maximum
  return power_values[( sizeof(power_values)/sizeof(energy_t) - 1 )];
}

/*******************************************************************************
 * @fn    uint8_t find_closest_power( double power )
 *
 * @brief Get a desired tx power in RSSI and returns radio register setting that
 *        matches as close as possible.
 * ****************************************************************************/
uint8_t find_closest_power( energy_t power )
{
  uint8_t min_index = 0;
  uint8_t index;
  energy_t min_value = 1e99;
  energy_t difference;

  // The current power calculation method is off, here's an attempt to 
  // compensate for that
  //power += POWER_CALIBRATION_FACTOR;

  for( index = 0; index < (sizeof(power_values)/sizeof(energy_t)); index++ )
  {
    
    difference = fabs( power - power_values[index] );
      
    if ( difference < min_value )
    {
      min_value = difference;
      min_index = index;
    }

  }
  
  //printf("%+0.1f", power_values[min_index]);
  
  return power_settings[min_index];
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

