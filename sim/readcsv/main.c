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

#define INBUFSIZE (512)
#define MAX_DEVICES (6)

void parse_line( char*, int8_t* );
uint8_t parse_table( FILE* , int8_t p_rssi_table[][MAX_DEVICES+1] );
void clean_table( int8_t p_rssi_table[][MAX_DEVICES+1] );
void add_links_from_table( int8_t p_rssi_table[][MAX_DEVICES+1] );

int32_t main ( int32_t argc, char *argv[] )
{  
  FILE *fp_in;      
  int8_t rssi_table[MAX_DEVICES+1][MAX_DEVICES+1];
  
  // Make sure the filename is included
  if ( argc < 2 )
  {
    printf( "Usage: %s filename\r\n", argv[0] );
    return 1;
  }
  
  // Open csv file
  fp_in = fopen( argv[1], "r" );
  
  if( NULL == fp_in )
  {
    printf( "Error opening file.\r\n" );
    return 1;
  }
   
  while( parse_table( fp_in, rssi_table ) )
  {
    uint16_t col_index, row_index;        
      
    for( row_index = 0; row_index < ( MAX_DEVICES+1 ); row_index++ )
    {
      for( col_index = 0; col_index < ( MAX_DEVICES+1 ); col_index++ )
      {
        printf("%+4d,", rssi_table[row_index][col_index]);
      }
      printf("\n");
    }
    printf("\n");
    
    clean_table( rssi_table );
    
    for( row_index = 0; row_index < ( MAX_DEVICES+1 ); row_index++ )
    {
      for( col_index = 0; col_index < ( MAX_DEVICES+1 ); col_index++ )
      {
        printf("%+4d,", rssi_table[row_index][col_index]);
      }
      printf("\n");
    }
    printf("\n");

    add_links_from_table( rssi_table );
    
  }
  
  fclose( fp_in );
  
  return 0;
}

/*******************************************************************************
 * @fn    void parse_line ( char* csv_line, int8_t* rssi_line )
 *
 * @brief Parse line from csv file an populate array row with contents
 * ****************************************************************************/
void parse_line ( char* csv_line, int8_t* rssi_line )
{
  char *p_item;
  uint16_t item_index = 0;
  
  p_item = strtok( csv_line, "," );
  while( NULL != p_item )
  {    
    // Convert string to RSSI value and store in rssi_line
    rssi_line[item_index] = (int8_t)strtol( p_item, NULL, 10 );
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
uint8_t parse_table ( FILE* fp_csv_file, int8_t p_rssi_table[][MAX_DEVICES+1] )
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
void clean_table( int8_t rssi_table[][MAX_DEVICES+1] )
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
      rssi_table[col_index][row_index] = -128;
    }

  }
  
}

/*******************************************************************************
 * @fn    void add_links_from_table( int8_t rssi_table[][MAX_DEVICES+1] )
 *
 * @brief Generate dijkstra links from cleaned up table
 * ****************************************************************************/
void add_links_from_table( int8_t rssi_table[][MAX_DEVICES+1] )
{
  uint16_t col_index, row_index;
  
  for( row_index = 0; row_index < ( MAX_DEVICES ); row_index++ )
  {
    for( col_index = row_index + 1; col_index < ( MAX_DEVICES+1 ); col_index++ )
    {
      printf("%d->%d[%d]\n", row_index, col_index, 
                                  rssi_table[row_index][col_index]);
    }
  }
}

