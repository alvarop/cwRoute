/** @file main.c
*
* @brief Read CSV file with RSSI table and generate links with it
*
* @author Alvaro Prieto
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "dijkstra.h"

#include "main.h"

#define INBUFSIZE (512)
#define MAX_DEVICES (6)

void parse_line( char*, energy_t* );
uint8_t parse_table( FILE* , energy_t p_rssi_table[][MAX_DEVICES+1] );
void clean_table( energy_t p_rssi_table[][MAX_DEVICES+1] );
void add_links_from_table( energy_t p_rssi_table[][MAX_DEVICES+1] );

double dbm_to_watt( double power );
double watt_to_dbm( double power );

energy_t target_rssi;

int32_t main ( int32_t argc, char *argv[] )
{  
  FILE *fp_in, *fp_out;      
  energy_t rssi_table[MAX_DEVICES+1][MAX_DEVICES+1];
  uint32_t index = 0;
  char node_id_string[3];
  uint8_t node_index;
  
  target_rssi = dbm_to_watt(-75l);
  
  // Make sure the filename is included
  if ( argc < 3 )
  {
    printf( "Usage: %s infile outfile\r\n", argv[0] );
    return 1;
  }
  
  // Open input csv file
  fp_in = fopen( argv[1], "r" );
  
  if( NULL == fp_in )
  {
    printf( "Error opening input file.\r\n" );
    return 1;
  }
  
  // Open output csv file
  fp_out = fopen( argv[2], "w" );
  
  if( NULL == fp_out )
  {
    printf( "Error opening input file.\r\n" );
    return 1;
  }
  
  // Add nodes 
  for( node_index = 0; node_index < (MAX_DEVICES + 1); node_index++ )
  {
    sprintf( node_id_string, "%d", node_index );
    add_labeled_node( node_index, 0, node_id_string );
  }
  
  initialize_node_energy( 0 );
  
  print_node_energy( 0, fp_out );
  
  //parse_table( fp_in, rssi_table ); 
  while( parse_table( fp_in, rssi_table ) )  
  {    
    
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
    
    printf("Round %d\n", index);
    print_node_energy( 0, fp_out );
    
    //generate_graph( 0, index );
    
    
    
    index++;
  }

  fclose( fp_out );  
  fclose( fp_in );
  
  return 0;
}

/*******************************************************************************
 * @fn    void parse_line ( char* csv_line, int8_t* rssi_line )
 *
 * @brief Parse line from csv file an populate array row with contents
 * ****************************************************************************/
void parse_line ( char* csv_line, energy_t* rssi_line )
{
  char *p_item;
  uint16_t item_index = 0;
  
  p_item = strtok( csv_line, "," );
  while( NULL != p_item )
  {    
    // Convert string to RSSI value and store in rssi_line
    rssi_line[item_index] = rssi_values[(uint8_t)strtol( p_item, NULL, 10 )];
                            
    item_index++;
    p_item = strtok( NULL, "," );
  }
}

/*******************************************************************************
 * @fn    uint8_t parse_table ( FILE* fp_csv_file, 
 *                                      int8_t p_rssi_table[][MAX_DEVICES+1] )
 *
 * @brief Read lines from CSV file and parse them until an empty line is found
 * ****************************************************************************/
uint8_t parse_table ( FILE* fp_csv_file, energy_t p_rssi_table[][MAX_DEVICES+1] )
{
  char csv_line[INBUFSIZE];   // Buffer for reading a line in the file
  uint16_t line_index = 0;

  while( NULL != fgets( csv_line, sizeof(csv_line), fp_csv_file ) )
  {
    // Detect empty line
    if( csv_line[0] == '\n' )
    {
      // Read table successfully
      return 1;
    }
    else
    {
      // Remove newline
      csv_line[(int32_t)strlen(csv_line)-1] = 0;
      
      // Parse csv line and populate array
      parse_line( csv_line, p_rssi_table[line_index] );            
    }
    
    if( line_index > MAX_DEVICES )
    {
      // Don't want to overflow the array. Return error.
      return 0;
    }
    
    line_index++;
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

