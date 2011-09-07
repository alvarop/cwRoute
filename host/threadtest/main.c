/** @file main.c
*
* @brief Threaded packet handler
*
* @author Alvaro Prieto
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include "serial.h"
#include "routing.h"
#include "main.h"

#ifndef MAX_DEVICES
#define MAX_DEVICES (3)
#warning MAX_DEVICES not defined, defaulting to 3
#endif

void send_serial_message( uint8_t* packet_buffer, int16_t buffer_size );
void *graph_thread();

pthread_t serial_thread;
pthread_t routing_thread;
pthread_t graphing_thread;

pthread_mutex_t mutex_graph;

// Routing and power tables (all in one array)
static volatile uint8_t rp_tables[MAX_DEVICES * 2];

// Table storing all device routes
static volatile uint8_t *routing_table = &rp_tables[0];

// Table storing all device transmit powers
static volatile uint8_t *power_table = &rp_tables[MAX_DEVICES];

int main( int argc, char *argv[] )
{
  int32_t rc;

  // Handle interrupt events to make sure files are closed before exiting
  (void) signal( SIGINT, sigint_handler );

  // Make sure input is correct
  if( argc < 3 )
  {
    printf("Usage: %s port baudrate [graph (0,1)]\n", argv[0]);
    return 0;
  }

  printf("Threaded Serial Terminal\r\n");

  // Convert string serial number to integer and open port
  if( serial_open( atoi( argv[1] ), atoi( argv[2] ), &process_packet ) )
  {
    printf("Error opening serial port.\r\n");
    exit(-1);
  }

  // Initialize routing an power tables
  memset( (uint8_t*)power_table, 0xff, MAX_DEVICES );
  memset( (uint8_t*)routing_table, ( MAX_DEVICES + 1 ), MAX_DEVICES );

  if ( routing_initialize() )
  {
    printf("Error initializing routes.\n");
    exit(-1);
  }

  rc = pthread_create( &serial_thread, NULL, serial_read_thread, NULL );

  if (rc)
  {
    printf("Error creating serial thread\n");
    exit(-1);
  }

  rc = pthread_create( &routing_thread, NULL, compute_routes_thread,
                                                        (void*) routing_table );

  if (rc)
  {
    printf("Error creating routing thread\n");
    exit(-1);
  }

  rc = pthread_create( &graphing_thread, NULL, graph_thread, NULL );

  if (rc)
  {
    printf("Error creating serial thread\n");
    exit(-1);
  }

  pthread_mutex_init( &mutex_graph, NULL );

  // Start mutex locked
  pthread_mutex_lock ( &mutex_graph );

  for(;;)
  {
    //uint8_t index;

    // Wait until routing is done
    pthread_mutex_lock ( &mutex_route_done );

    // Send new routes to AP
    send_serial_message( (uint8_t *)rp_tables, sizeof(rp_tables) );

    // Only graph when asked to
    if ( argv[3][0] == '1')
    {
      // Start generating the graph
      pthread_mutex_unlock ( &mutex_graph );
    }

    // Print routes and powers
    /*
    for( index = 0; index < MAX_DEVICES; index++ )
    {
      printf( "%d->%d ", index+1, routing_table[index] );
    }

    printf("\n");

    for( index = 0; index < MAX_DEVICES; index++ )
    {
      printf( "%02X ", power_table[index] );
    }
    printf("\n");*/
  }


  return 0;
}

/*******************************************************************************
 * @fn     void process_packet( uint8_t* buffer, uint32_t size )
 * @brief  Process incoming serial packet
 * ****************************************************************************/
void process_packet( uint8_t* buffer, uint32_t size )
{
  uint8_t rssi_table[MAX_DEVICES+1][MAX_DEVICES+1];

  if( size < sizeof(rssi_table) )
  {
    printf( "Received packet smaller than RSSI table\r\n" );
    return;
  }

  memcpy( rssi_table, buffer, sizeof(rssi_table) );

  parse_table( rssi_table );

  // Let the routing algorithm run
  pthread_mutex_unlock ( &mutex_route_start );

  return;
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

    pthread_cancel( serial_thread );
    pthread_cancel( routing_thread );
    pthread_cancel( graphing_thread );

    routing_finalize();

    // Close the serial port
    serial_close();

    printf("\nExiting...\n");
    exit(sig);
}

