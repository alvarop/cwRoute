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

#include "dijkstra.h"

#define INBUFSIZE (512)
#define MAX_DEVICES (6)

void parse_line( char*, energy_t* );
uint8_t parse_table( FILE* , energy_t p_rssi_table[][MAX_DEVICES+1] );
void clean_table( energy_t p_rssi_table[][MAX_DEVICES+1] );
void add_links_from_table( energy_t p_rssi_table[][MAX_DEVICES+1] );

static const energy_t rssi_values[256] = {
-72.0,-71.5,-71.0,-70.5,-70.0,-69.5,-69.0,-68.5,-68.0,-67.5,-67.0,
-66.5,-66.0,-65.5,-65.0,-64.5,-64.0,-63.5,-63.0,-62.5,-62.0,
-61.5,-61.0,-60.5,-60.0,-59.5,-59.0,-58.5,-58.0,-57.5,-57.0,
-56.5,-56.0,-55.5,-55.0,-54.5,-54.0,-53.5,-53.0,-52.5,-52.0,
-51.5,-51.0,-50.5,-50.0,-49.5,-49.0,-48.5,-48.0,-47.5,-47.0,
-46.5,-46.0,-45.5,-45.0,-44.5,-44.0,-43.5,-43.0,-42.5,-42.0,
-41.5,-41.0,-40.5,-40.0,-39.5,-39.0,-38.5,-38.0,-37.5,-37.0,
-36.5,-36.0,-35.5,-35.0,-34.5,-34.0,-33.5,-33.0,-32.5,-32.0,
-31.5,-31.0,-30.5,-30.0,-29.5,-29.0,-28.5,-28.0,-27.5,-27.0,
-26.5,-26.0,-25.5,-25.0,-24.5,-24.0,-23.5,-23.0,-22.5,-22.0,
-21.5,-21.0,-20.5,-20.0,-19.5,-19.0,-18.5,-18.0,-17.5,-17.0,
-16.5,-16.0,-15.5,-15.0,-14.5,-14.0,-13.5,-13.0,-12.5,-12.0,
-11.5,-11.0,-10.5,-10.0,-9.5,-9.0,-8.5,-136.0,-135.5,-135.0,
-134.5,-134.0,-133.5,-133.0,-132.5,-132.0,-131.5,-131.0,-130.5,-130.0,
-129.5,-129.0,-128.5,-128.0,-127.5,-127.0,-126.5,-126.0,-125.5,-125.0,
-124.5,-124.0,-123.5,-123.0,-122.5,-122.0,-121.5,-121.0,-120.5,-120.0,
-119.5,-119.0,-118.5,-118.0,-117.5,-117.0,-116.5,-116.0,-115.5,-115.0,
-114.5,-114.0,-113.5,-113.0,-112.5,-112.0,-111.5,-111.0,-110.5,-110.0,
-109.5,-109.0,-108.5,-108.0,-107.5,-107.0,-106.5,-106.0,-105.5,-105.0,
-104.5,-104.0,-103.5,-103.0,-102.5,-102.0,-101.5,-101.0,-100.5,-100.0,
-99.5,-99.0,-98.5,-98.0,-97.5,-97.0,-96.5,-96.0,-95.5,-95.0,
-94.5,-94.0,-93.5,-93.0,-92.5,-92.0,-91.5,-91.0,-90.5,-90.0,
-89.5,-89.0,-88.5,-88.0,-87.5,-87.0,-86.5,-86.0,-85.5,-85.0,
-84.5,-84.0,-83.5,-83.0,-82.5,-82.0,-81.5,-81.0,-80.5,-80.0,
-79.5,-79.0,-78.5,-78.0,-77.5,-77.0,-76.5,-76.0,-75.5,-75.0,
-74.5,-74.0,-73.5,-73.0,-72.5, };

int32_t main ( int32_t argc, char *argv[] )
{  
  FILE *fp_in, *fp_out;      
  energy_t rssi_table[MAX_DEVICES+1][MAX_DEVICES+1];
  uint32_t index = 0;
  char node_id_string[3];
  uint8_t node_index;
  
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
    
#ifndef REGULAR_DIJKSTRA
    // Calculate link costs before running dijkstra's algorithm
    calculate_link_costs();
#endif

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
  
  for( row_index = 0; row_index < ( MAX_DEVICES ); row_index++ )
  {
    for( col_index = row_index + 1; col_index < ( MAX_DEVICES+1 ); col_index++ )
    {
      printf("%d->%d[%g]\n", row_index, col_index, 
                                  rssi_table[row_index][col_index]);
      // Add link
      add_link( row_index, col_index, rssi_table[row_index][col_index] );
    }
  }
}

