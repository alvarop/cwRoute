/** @file main.c
*
* @brief Read CSV file with RSSI table and simulate routing algorithm
*
* @author Alvaro Prieto
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include "routing.h"
#include "dijkstra.h"
#include "main.h"

#define INBUFSIZE (4096)

#ifndef MAX_DEVICES
#define MAX_DEVICES (3)
#warning MAX_DEVICES not defined, defaulting to 3
#endif

void read_energy_line( char*, energy_t* );
uint8_t read_power_line ( FILE* , energy_t* );
uint8_t read_table( FILE* , energy_t p_rssi_table[][MAX_DEVICES+1] );
void *graph_thread();
void sigint_handler( int32_t sig );

pthread_t routing_thread;
pthread_t graphing_thread;

pthread_mutex_t mutex_graph;

// Routing and power tables (all in one array)
static volatile uint8_t rp_tables[MAX_DEVICES * 2];

// Table storing all device routes
static volatile uint8_t *routing_table = &rp_tables[0];

// Table storing all device transmit powers
static volatile uint8_t *power_table = &rp_tables[MAX_DEVICES];

int32_t main ( int32_t argc, char *argv[] )
{
  FILE *fp_rssi;
  FILE *fp_powers;
  int32_t rc;
  uint8_t node_index;
  energy_t rssi_table[MAX_DEVICES+1][MAX_DEVICES+1];
  energy_t previous_powers[MAX_DEVICES];
  uint32_t sample_limit = 10000;
  
  // Handle interrupt events to make sure files are closed before exiting
  (void) signal( SIGINT, sigint_handler );

  // Make sure the filename is included
  if ( argc < 4 )
  {
    printf( "Usage: %s rssi.csv powers.csv C(0.0-1.0) [graph (0,1)]\r\n"
                                                                    , argv[0] );
    return 1;
  }
  
  // Open input rssi csv file
  fp_rssi = fopen( argv[1], "r" );

  if( NULL == fp_rssi )
  {
    printf( "Error opening rssi input file.\r\n" );
    return 1;
  }
  
  // Open input rssi csv file
  fp_powers = fopen( argv[2], "r" );

  if( NULL == fp_powers )
  {
    printf( "Error opening tx powers input file.\r\n" );
    return 1;
  }
  
  // Initialize routing an power tables
  memset( (uint8_t*)power_table, 0xff, MAX_DEVICES );
  memset( (uint8_t*)routing_table, ( MAX_DEVICES + 1 ), MAX_DEVICES );
  
  // Initialize previous_powers table
  for( node_index = 0; node_index < MAX_DEVICES; node_index++ )
  {

    // Initialize previous power to maximum
    previous_powers[node_index] =
                    power_values[sizeof(power_values)/sizeof(energy_t) - 1];
  }
  
  if ( routing_initialize( (energy_t)strtod( argv[3], NULL ) ) )
  {
    printf("Error initializing routes.\n");
    exit(-1);
  }

  rc = pthread_create( &routing_thread, NULL, compute_routes_thread,
                                                        (void*) routing_table );

  if (rc)
  {
    printf("Error creating routing thread\n");
    exit(-1);
  }
  
  // Create mutex
  pthread_mutex_init( &mutex_graph, NULL );

  // Start mutex locked
  pthread_mutex_lock ( &mutex_graph );

  rc = pthread_create( &graphing_thread, NULL, graph_thread, NULL );

  if (rc)
  {
    printf("Error creating serial thread\n");
    exit(-1);
  }  

  // Lock this before starting
  pthread_mutex_lock ( &mutex_route_done );

  while( read_table( fp_rssi, rssi_table ) && sample_limit-- )
  {

    parse_table_d( rssi_table, previous_powers );
  
    // Let the routing algorithm run
    pthread_mutex_unlock ( &mutex_route_start );
    
    // Wait until routing is done
    pthread_mutex_lock ( &mutex_route_done );

    // Only graph when asked to
    if ( argv[4][0] == '1')
    {
      // Start generating the graph
      pthread_mutex_unlock ( &mutex_graph );
    }
    
    // Read previous powers
    if( !read_power_line ( fp_powers, previous_powers ) )
    {
      printf("Error reading from power file!");
    }

    sample_limit--;
  }

  fclose( fp_powers );
  fclose( fp_rssi );

  return 0;
}

/*******************************************************************************
 * @fn    void read_energy_line ( char* csv_line, int8_t* rssi_line )
 *
 * @brief Parse line from csv file an populate array row with contents
 * ****************************************************************************/
void read_energy_line ( char* csv_line, energy_t* rssi_line )
{
  char *p_item;
  uint16_t item_index = 0;

  p_item = strtok( csv_line, "," );
  while( NULL != p_item )
  {
    // Convert string to RSSI value and store in rssi_line
    rssi_line[item_index] = (energy_t)strtod( p_item, NULL );

    item_index++;
    p_item = strtok( NULL, "," );
  }
}

/*******************************************************************************
 * @fn    uint8_t read_power_line ( FILE* fp_powers, energy_t* power_line )
 *
 * @brief Parse line from csv file an populate array row with contents
 * ****************************************************************************/
uint8_t read_power_line ( FILE* fp_powers, energy_t* power_line )
{
  char csv_line[INBUFSIZE];   // Buffer for reading a line in the file
  char *p_item;
  uint16_t item_index = 0;
  printf("Reading power line\n");
  if ( NULL != fgets( csv_line, sizeof(csv_line), fp_powers ) )
  {
    p_item = strtok( csv_line, "," );
    while( ( NULL != p_item ) && ( item_index < MAX_DEVICES ) )
    {
      p_item = strtok( NULL, "," );
    
      // Convert string to RSSI value and store in rssi_line
      power_line[item_index] = (energy_t)strtod( p_item, NULL );
      printf( "%g ", power_line[item_index] );
      item_index++;
      
    }
    printf("\n");
    // Read table successfully
    return 1;
  }
  
  // Did NOT read table successfully
  return 0;
}

/*******************************************************************************
 * @fn    uint8_t read_table ( FILE* fp_csv_file,
 *                                      int8_t p_rssi_table[][MAX_DEVICES+1] )
 *
 * @brief Read lines from CSV file and parse them until an empty line is found
 * ****************************************************************************/
uint8_t read_table ( FILE* fp_csv_file, energy_t p_rssi_table[][MAX_DEVICES+1] )
{
  char csv_line[INBUFSIZE];   // Buffer for reading a line in the file
  uint16_t line_index = 0;
  printf("Reading table\n");
  while( NULL != fgets( csv_line, sizeof(csv_line), fp_csv_file ) )
  {
    // Detect empty line
    if( csv_line[0] == '\n' )
    {
      printf("Read table\n");
      // Read table successfully
      return 1;
    }
    else
    {
      // Remove newline
      csv_line[(int32_t)strlen(csv_line)-1] = 0;

      // Parse csv line and populate array
      read_energy_line( csv_line, p_rssi_table[line_index] );
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
 * @fn     void *graph_thread()
 * @brief  Run as thread. Generates graph from routing table
 * ****************************************************************************/
void *graph_thread()
{
  uint8_t link_index;
  FILE* f_graph;
  char command[100];
  char filename[100];
  static uint32_t frame = 0;

  for(;;)
  {
    // Block until next table is ready
    pthread_mutex_lock ( &mutex_graph );

    sprintf( filename, "images/graph%05d.gv", frame );

    printf("%s\n", filename);

    f_graph = fopen(filename, "w" );

    if ( f_graph != NULL )
    {
      fprintf( f_graph,  "digraph network" );

      fprintf( f_graph,  " {\n" );

      // Configuration stuff

      fprintf( f_graph, "edge [len=3]\n");
      fprintf( f_graph, "nodesep=0.25\n");
      fprintf( f_graph, "node[shape = doublecircle]; %d;\n",
                                                        ( MAX_DEVICES + 1 ) );

      fprintf( f_graph, "node[shape = circle];\n");

      // All connections
      for( link_index = 0; link_index < MAX_DEVICES; link_index++ )
      {
        if ( routing_table[link_index] != 0 )
        {
          fprintf( f_graph, "%d -> %d", ( link_index + 1 ),
                                                    routing_table[link_index] );
          fprintf( f_graph, "[ label=\"");
          fprintf( f_graph,  "%5.1f dBm",
                      get_power_from_setting( power_table[link_index] ) );
          fprintf( f_graph, "\" ]");
          fprintf( f_graph,  ";\n" );
        }
      }

      // print_node_name to file
      for( link_index = 0; link_index < MAX_DEVICES; link_index++ )
      {
          fprintf( f_graph, "%d [label=\"%d\"];\n", ( link_index + 1 ),
                                                          ( link_index + 1 ) );
      }

      fprintf( f_graph, "%d [label=\"AP\"];\n", ( MAX_DEVICES + 1 ) );

      fprintf( f_graph,  "}\n" );

      fclose( f_graph );

      // Generate svg of graph using GraphViz 'neato'
      //system("neato -Tsvg -ograph.svg graph.gv");
      //system("dot -Tsvg -ograph.svg graph.gv");
      //sprintf( command, "neato -Tjpg -oimages/graph%04d.jpg images/graph.gv", file_number);
      sprintf( command, "dot -Tjpg -Gsize=4,4! -Gratio=fill -oimages/graph%05d.jpg %s", frame, filename );
      system( command );

      // Animate with ffmpeg -r 10 -b 500000 -i graph%05d.jpg ./animation.mp4

      frame++;
    }
  }
  return NULL;
}

/*******************************************************************************
 * @fn     void sigint_handler( int32_t sig )
 * @brief  Handle interrupt event (SIGINT) so program exits cleanly
 * ****************************************************************************/
void sigint_handler( int32_t sig )
{

    pthread_cancel( routing_thread );
    pthread_cancel( graphing_thread );

    routing_finalize();

    printf("\nExiting...\n");
    exit(sig);
}

