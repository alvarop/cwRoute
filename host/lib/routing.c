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

uint8_t find_closest_power( energy_t power );
double dbm_to_watt( double power );
double watt_to_dbm( double power );

energy_t target_rssi;
energy_t rssi_table[MAX_DEVICES+1][MAX_DEVICES+1];
energy_t link_power_table[MAX_DEVICES+1][MAX_DEVICES+1];
energy_t link_powers[MAX_DEVICES];
static energy_t previous_powers[MAX_DEVICES];
static energy_t previous_powers_debug[MAX_DEVICES];
uint8_t route_table_debug[MAX_DEVICES];

energy_t c_factor;

FILE *fp_energies, *fp_routes, *fp_powers, *fp_rssi, *fp_debug;

#define AP_NODE_ID (MAX_DEVICES+1)

/*******************************************************************************
 * @fn    uint8_t routing_initialize( energy_t dijkstra_c_factor )
 *
 * @brief Open all debugging files and initialize routing system
 * ****************************************************************************/
uint8_t routing_initialize( energy_t dijkstra_c_factor )
{
  char node_id_string[3];
  uint8_t node_index;

  // Open output csv files
  fp_energies = fopen( "./logs/energies.csv", "w" );
  if( NULL == fp_energies )
  {
    printf( "Error opening energies file.\r\n" );
    return 1;
  }

  fp_routes = fopen( "./logs/routes.csv", "w" );
  if( NULL == fp_routes )
  {
    printf( "Error opening routes file.\r\n" );
    return 1;
  }

  fp_powers = fopen( "./logs/powers.csv", "w" );
  if( NULL == fp_powers )
  {
    printf( "Error opening powers file.\r\n" );
    return 1;
  }

  fp_rssi = fopen( "./logs/rssi.csv", "w" );
  if( NULL == fp_rssi )
  {
    printf( "Error opening rssi file.\r\n" );
    return 1;
  }

  fp_debug = fopen( "./logs/debug.csv", "w" );
  if( NULL == fp_debug )
  {
    printf( "Error opening debug file.\r\n" );
    return 1;
  }

  // Store c_factor for later use
  c_factor = dijkstra_c_factor;

  target_rssi = dbm_to_watt(-60l);

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

  return 0;
}

/*******************************************************************************
 * @fn    void routing_finalize()
 *
 * @brief Call when finished. Close all files.
 * ****************************************************************************/
void routing_finalize()
{
  fclose( fp_energies );
  fclose( fp_routes );
  fclose( fp_powers );
  fclose( fp_rssi );
  fclose( fp_debug );
}

/*******************************************************************************
 * @fn    void *compute_routes_thread( void *rp_tables )
 *
 * @brief Thread that takes care of routing
 * ****************************************************************************/
void *compute_routes_thread( void *rp_tables )
{
  static uint32_t index;
  uint8_t node_index;
  uint8_t *route_table = &((uint8_t*)rp_tables)[0];
  uint8_t *power_table = &((uint8_t*)rp_tables)[MAX_DEVICES];
  
  
  // loop forever
  for (;;)
  {
    // Block until next table is ready
    pthread_mutex_lock ( &mutex_route_start );

    // Assuming rssi_table has been updated
    clean_table( rssi_table );

    add_links_from_table( rssi_table );

    // Run dijkstra's algorithm with 0 being the access point
    dijkstra( AP_NODE_ID, c_factor );

    // Display shortest paths and update energies
    for( node_index = 1; node_index < (MAX_DEVICES + 1); node_index++ )
    {
      compute_shortest_path( node_index );
      print_shortest_path( node_index );
    }

    // Compute routing table
    compute_rp_tables( route_table, link_powers );

    // Debug
    memcpy( previous_powers_debug, previous_powers, sizeof(previous_powers) );
    memcpy( route_table_debug, route_table, sizeof(route_table_debug) );

    // Compute power table
    compute_required_powers( link_powers, power_table );

    print_rssi_table();

    print_node_energy( AP_NODE_ID, fp_energies );

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
 * @brief Get RSSI table, convert, store and print it(gets uint8_t rssi array)
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
      // If RSSI is the minimum (-136.0), make it much lower so that the maximum
      // transmit power is used.
      if( p_rssi_table[row_index][col_index] == 0x80 )
      {
        rssi_table[row_index][col_index] = -999.0;
      }
      else
      {
        rssi_table[row_index][col_index] =
                               rssi_values[p_rssi_table[row_index][col_index]];
      }

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

      //tx_power = dbm_to_watt( 1.5l ); // uncomment to override

      // Compute the minimum power required to meet this link with 'target_rssi'
      // alpha is the channel attenuation, that is received/transmitted power
      alpha = dbm_to_watt( rssi_table[row_index][col_index] ) / tx_power;

      // Transmit power required is the target rssi / channel attenuation
      link_power_table[row_index][col_index] = target_rssi / alpha;

    }
  }

  // Did not read table successfully
  return 0;
}

/*******************************************************************************
 * @fn    uint8_t parse_table_d ( energy_t p_rssi_table[][MAX_DEVICES+1]
 *                                            energy_t *p_previous_powers )
 *
 * @brief Get RSSI table, convert, store and print it 
 * (gets energy_t rssi array AND energy_t previous tx power array)
 * ****************************************************************************/
uint8_t parse_table_d ( energy_t p_rssi_table[][MAX_DEVICES+1], 
                                      energy_t *p_previous_powers )
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
      // Copy rssi table
      rssi_table[row_index][col_index] = p_rssi_table[row_index][col_index];
      
      if( 0 == col_index )
      {
        // AP always transmits with max power
        tx_power = dbm_to_watt( get_power_from_setting( 0xff ) );
      }
      else
      {
        // Use previous transmit settings
        tx_power = dbm_to_watt( p_previous_powers[col_index + 1] );
      }

      //tx_power = dbm_to_watt( 1.5l ); // uncomment for no power control

      // Compute the minimum power required to meet this link with 'target_rssi'
      // alpha is the channel attenuation, that is received/transmitted power
      alpha = dbm_to_watt( rssi_table[row_index][col_index] ) / tx_power;

      // Transmit power required is the target rssi / channel attenuation
      link_power_table[row_index][col_index] = target_rssi / alpha;

    }
  }

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
      if( link_power_table[row_index][col_index] >
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

/*******************************************************************************
 * @fn    void compute_required_powers( energy_t* p_link_powers,
 *                                                      uint8_t* power_table )
 *
 * @brief Compute required power to meet the selected links
 * ****************************************************************************/
void compute_required_powers( energy_t* p_link_powers, uint8_t* power_table )
{
  uint8_t node_index;


  for( node_index = 0; node_index < MAX_DEVICES; node_index++ )
  {

    // Store required power in power table
    power_table[node_index] =
                find_closest_power( watt_to_dbm( p_link_powers[node_index] ) );

    // Save current required power to be used as tx_power next round
    previous_powers[node_index] =
                            //get_power_from_setting( 0xff );  // no power control
                            get_power_from_setting( power_table[node_index] );

  }
}

/*******************************************************************************
 * @fn    void print_rssi_table()
 *
 * @brief Save current data to files
 * ****************************************************************************/
void print_rssi_table()
{
  uint8_t row_index;
  uint8_t col_index;

  //printf("   ");

  for( col_index = 0; col_index <= MAX_DEVICES; col_index++ )
  {
    //printf(" %3d   ", col_index );
  }

  //printf("\r\n");

  // Print packet in hex
  for( row_index = 0; row_index <= MAX_DEVICES; row_index++ )
  {
    //printf("%2d ", row_index );

    // RSSI table
    for( col_index = 0; col_index < MAX_DEVICES; col_index++ )
    {
      //printf("%06.1f ", rssi_table[row_index][col_index] );
      fprintf( fp_debug, "%g,", rssi_table[row_index][col_index] );
      fprintf( fp_rssi, "%g,", rssi_table[row_index][col_index] );
    }
    //printf("%06.1f ", rssi_table[row_index][col_index] );
    fprintf( fp_debug, "%g,", rssi_table[row_index][col_index] );
    fprintf( fp_rssi, "%g\n", rssi_table[row_index][col_index] );

    if( row_index > 0 )
    {
      //printf("%g ", previous_powers_debug[row_index-1] );
      fprintf( fp_debug, "%g,", ( previous_powers_debug[row_index-1] ) );

      //printf("%g ", link_powers[row_index-1] );
      fprintf( fp_debug, "%g,", watt_to_dbm( link_powers[row_index-1] ) );
      fprintf( fp_powers, "%g,", watt_to_dbm( link_powers[row_index-1] ) );

      //printf("%d ", route_table_debug[row_index-1] );
      fprintf( fp_debug, "%d,", route_table_debug[row_index-1] );
      fprintf( fp_routes, "%d,", route_table_debug[row_index-1] );

    }
    else
    {
      //printf("%g ", 1.5 ); // AP always transmits max power
      fprintf( fp_debug, "%g,", ( 1.5 ) );

      //printf("%g ", 1.5 ); // AP always transmits max power
      fprintf( fp_debug, "%g,", ( 1.5 ) );
      fprintf( fp_powers, "%g,", 1.5 );

      //printf("0 ");
      fprintf( fp_debug, "0," );
      fprintf( fp_routes, "0," );

    }


    for( col_index = 0; col_index < MAX_DEVICES; col_index++ )
    {
      //printf("%g ", link_power_table[row_index][col_index] );
      fprintf( fp_debug, "%g,", watt_to_dbm( link_power_table[row_index][col_index] ) );
    }

    //printf("%g ", link_power_table[row_index][col_index] );
    fprintf( fp_debug, "%g,", watt_to_dbm( link_power_table[row_index][col_index] ) );

    //printf("\r\n");
    fprintf( fp_debug, "\n" );
  }

  //printf("\r\n");
  fprintf( fp_debug, "\n" );
  fprintf( fp_rssi, "\n" );
  fprintf( fp_routes, "\n" );
  fprintf( fp_powers, "\n" );
}

/*******************************************************************************
 * @fn    energy_t get_power_from_setting( uint8_t setting )
 *
 * @brief Provide cc2500 and function returns tx power in dBm
 * ****************************************************************************/
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

