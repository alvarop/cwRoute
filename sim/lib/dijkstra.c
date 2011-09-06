/** @file dijkstra.c
*
* @brief Node and supporting function definitions for routing algorithm
*
* @author Alvaro Prieto
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "dijkstra.h"

static nodes_t s_nodes;
static links_t s_links;

static energy_t s_mean_energy;

uint32_t current_round;

node_t* find_node( uint8_t );
node_t* node_with_smallest_distance( );
link_t* find_link( uint8_t, uint8_t );

//
// Add new node to nodes list
//
uint8_t add_node( uint8_t node_id, uint8_t is_relay )
{
  node_t* new_node;
  if( s_nodes.current_nodes < MAX_NODES )
  {
    new_node = &s_nodes.nodes[s_nodes.current_nodes];
    new_node->id = node_id;
    new_node->energy = 0;
    new_node->visited = 0;
    new_node->is_relay = is_relay;

    s_nodes.current_nodes++;

    if( is_relay )
    {
      s_nodes.current_relays++;
    }

    return 0;
  }
  else
  {
    // Error, ran out of space in node list
    return 1;
  }
}

//
// Add new link to links list
//
uint8_t add_link( uint8_t source, uint8_t destination, energy_t link_power )
{
  link_t* new_link;

  //TODO check if source and destination actually exist!

  if( s_links.current_links < MAX_LINKS )
  {

    // If the same link already exists, just update it
    new_link = find_link(source, destination);

    if( new_link == NULL  )
    {
      // Add new link
      new_link = &s_links.links[s_links.current_links];
      s_links.current_links++;
    }

    new_link->links_power = link_power;
    new_link->source = source;
    new_link->destination = destination;

    // Disable link if the power required is too high
    if( link_power > MAX_LINK_POWER * 100 )
    {
      new_link->active = 0;
    }
    else
    {
      new_link->active = 1;
    }

    return 0;
  }
  else
  {
    // Error, ran out of space in link list
    return 1;
  }
}

//
// Initialize mean energy from direct links to AP
//
energy_t initialize_node_energy( uint8_t source_id )
{
  node_t* p_node;
  uint8_t link_index;

  current_round = 1;

  for( link_index = 0; link_index < s_links.current_links; link_index++ )
  {
    if( s_links.links[link_index].source == source_id )
    {
      p_node = find_node( s_links.links[link_index].destination );

#ifdef DEBUG_ON
  //print_node_name( p_node->id );
#endif
      if ( !p_node->is_relay )
      {
        p_node->energy = s_links.links[link_index].links_power;
        s_mean_energy += p_node->energy;
#ifdef DEBUG_ON
  //printf("(%g)", p_node->energy);
#endif
      }
#ifdef DEBUG_ON
  //printf("\n");
#endif
    }
  }

  // Compute actual mean (subtracting 1 to account for source)
  s_mean_energy /= (s_nodes.current_nodes - 1 - s_nodes.current_relays);

#ifdef DEBUG_ON
  //printf("Mean Energy: %g\n", s_mean_energy);
#endif

  return s_mean_energy;
}

//
// Re-calculate mean energy
//
energy_t compute_mean_energy( uint8_t source_id )
{
  uint8_t node_index;

  s_mean_energy = 0;

  // Add up all node energies
  for( node_index = 0; node_index < s_nodes.current_nodes; node_index++ )
  {
    if( s_nodes.nodes[node_index].id != source_id )
    {
      s_mean_energy += s_nodes.nodes[node_index].energy;
    }
  }

  // Compute mean
  s_mean_energy /= (s_nodes.current_nodes - 1);

  // Subtract mean energy from node energy to 'equalize' and avoid overflows
/*  for( node_index = 0; node_index < s_nodes.current_nodes; node_index++ )
  {
    if( s_nodes.nodes[node_index].id != source_id )
    {
      s_nodes.nodes[node_index].energy -= s_mean_energy;
    }
  }  */

  return s_mean_energy;
}

//
// Returns pointer to node with matching node_id
// If node is not found, returns NULL pointer
//
node_t* find_node( uint8_t node_id )
{
  uint8_t node_index;

  for( node_index = 0; node_index < s_nodes.current_nodes; node_index++ )
  {
    if( s_nodes.nodes[node_index].id == node_id )
    {
      return &s_nodes.nodes[node_index];
    }
  }

  return NULL;
}

//
// Returns pointer to link with matching source and destination ids
// If link is not found, returns NULL pointer
//
link_t* find_link( uint8_t source_id, uint8_t destination_id )
{
  uint8_t link_index;

  for( link_index = 0; link_index < s_links.current_links; link_index++ )
  {
    if( ( s_links.links[link_index].source == source_id &&
        s_links.links[link_index].destination == destination_id )
        || ( s_links.links[link_index].destination == source_id &&
        s_links.links[link_index].source == destination_id ) )
    {
      return &s_links.links[link_index];
    }
  }

  // If this happens, expect a segfault
  //printf("NULL LINK!\n");

  return NULL;
}

//
// Find the node with the smallest distance that has not been visited
//
node_t* node_with_smallest_distance( )
{
  uint8_t node_index;
  uint8_t best_node = 0;
  energy_t min_distance=MAX_DISTANCE;

  for( node_index = 0; node_index < s_nodes.current_nodes; node_index++ )
  {
    //
    // If the current node has not been visited and has a smaller distance
    // than the current minimum, select it as the new minimum
    //
    if ( ( s_nodes.nodes[node_index].distance < min_distance ) &&
         ( 0 == s_nodes.nodes[node_index].visited ) )
    {
      best_node = node_index;
      min_distance = s_nodes.nodes[node_index].distance;
    }
  }

  //
  // If the minimum distance did not change, there were no more nodes available
  //
  if( min_distance == MAX_DISTANCE )
  {
    return NULL;
  }
  else
  {
    return &s_nodes.nodes[best_node];
  }

}

//
// Run Dijkstra's algorithm
//
uint8_t dijkstra( uint8_t source_id )
{
  uint8_t node_index;
  uint8_t link_index;

  node_t* p_source_node;
  node_t* p_destination_node;
  link_t* p_current_link;

  energy_t possible_distance;
  energy_t current_cost;

  // Update round count
  current_round += 1;

#ifdef DEBUG_D_ON
  //printf("Initialize s_nodes.\n"); // DEBUG
#endif

  //
  // Initialize node distance to 'infinity' and sets self as 'previous node'
  //
  for( node_index = 0; node_index < s_nodes.current_nodes; node_index++ )
  {
    s_nodes.nodes[node_index].distance = MAX_DISTANCE;
    s_nodes.nodes[node_index].p_previous = &s_nodes.nodes[node_index];
    s_nodes.nodes[node_index].visited = 0;
  }

  //
  // Get actual array index from node_id and re-use source_id variable
  //
  p_source_node = find_node( source_id );

#ifdef DEBUG_D_ON
  print_node_name( p_source_node->id );
  printf( " is the source.\n" ); //DEBUG
#endif

  if( NULL == p_source_node )
  {
    printf("Error: Node %d not found!\n", source_id );
    exit(1);
  }

  //
  // Start with distance of 0, since it is the starting point
  //
  p_source_node->distance = 0;


#ifdef DEBUG_D_ON
  printf("Start main loop.\n"); // DEBUG
#endif

  p_source_node = node_with_smallest_distance();

  //
  // Loop until all the (linked) nodes have been visited
  //
  while( NULL != p_source_node  )
  {

#ifdef DEBUG_D_ON
    print_node_name( p_source_node->id );
    printf( " has current smallest (non-visited) distance.\n" ); // DEBUG
#endif

    if ( MAX_DISTANCE == p_source_node->distance )
    {
    // No more nodes are accessible
#ifdef DEBUG_ON
      printf("No more nodes are accessible!\n"); // DEBUG
#endif
      break;
    }

    //
    // Mark node as already visited
    //
    p_source_node->visited = 1;

    //
    // Check for all links leaving this node
    //
    for( link_index = 0; link_index < s_links.current_links; link_index++ )
    {
      p_destination_node = NULL;
      p_current_link = &s_links.links[link_index];

      // Make sure link is active before using
      if ( p_current_link->active )
      {
        // If this link has the source node, use other node as destination
        if( p_current_link->destination == p_source_node->id )
        {
          p_destination_node = find_node( p_current_link->source );
        }
        else if( p_current_link->source == p_source_node->id )
        {
          p_destination_node = find_node( p_current_link->destination );
        }
      }

      //
      // Make sure there is a destination, otherwise do nothing
      //
      if ( NULL != p_destination_node )
      {

#ifdef DEBUG_D_ON
        printf("  Found link "); // DEBUG
        print_link( p_current_link ); // DEBUG
        printf("\n"); // DEBUG
#endif

        // Make sure we don't go backwards
        if( ! p_destination_node->visited )
        {
          // Calculate the current cost of the link
          current_cost = p_source_node->energy + p_current_link->links_power;
          current_cost /= current_round;
          current_cost += s_mean_energy /( current_round - 1 );

#ifdef REGULAR_DIJKSTRA
          current_cost = p_current_link->links_power;
#endif

          //
          // Compute the possible distance for destination if current link is used
          //
          possible_distance = p_source_node->distance + current_cost;

          //
          // If possible distance is smaller than current one, update destination
          //
          if ( possible_distance < p_destination_node->distance )
          {
#ifdef DEBUG_D_ON
            printf("    Path through this link is better for ");// DEBUG
            print_node_name(p_destination_node->id); // DEBUG
            printf(". Updating...\n");// DEBUG
#endif

            p_destination_node->distance = possible_distance;
            p_destination_node->p_previous = p_source_node;
          }
        }
      }
    }

    //
    // Find the next source node (returns NULL and exits loop if done)
    //
    p_source_node = node_with_smallest_distance();
  }

  return 0;
}

//
// Update accumulated energy of the nodes.
// NOTE: MUST be run AFTER dijkstra() function
//
void compute_shortest_path( uint8_t node_id )
{
  node_t* p_node;
  energy_t tmp_link_power;

  p_node = find_node( node_id );

  if ( p_node->p_previous == p_node )
  {
    // No path to node
  }
  else
  {
    while( p_node->p_previous != p_node )
    {
      tmp_link_power =
          find_link( p_node->p_previous->id, p_node->id )->links_power;

      // If the computed link power is greater than the maximum, set it to the
      // maximum. Since the devices can't transmit at a higher power, no extra
      // energy will be spent.
      if ( tmp_link_power > MAX_LINK_POWER )
      {
        tmp_link_power = MAX_LINK_POWER;
      }

      p_node->energy += tmp_link_power;

      p_node = p_node->p_previous;
    }
  }
}

//
// Store routes in an array and link powers in another
// If the link does not exist, fill in dummy values
//
void compute_rp_tables( uint8_t* route_table, energy_t* link_powers )
{
  uint8_t node_id;
  node_t* p_node;

  for( node_id = 1; node_id < s_nodes.current_nodes; node_id++ )
  {
    p_node = find_node( node_id );

    // If node is not connected, set route to broadcast
    if ( p_node->id == p_node->p_previous->id )
    {
      route_table[node_id-1] = 0; // Broadcast
      link_powers[node_id-1] = MAX_LINK_POWER;

    }
    else
    {
      route_table[node_id-1] = p_node->p_previous->id;
      link_powers[node_id-1] =
              find_link( p_node->p_previous->id, p_node->id )->links_power;
    }
  }

  return;
}

#ifdef DEBUG_ON
//
// Debugging functions
//

struct node_info_s
{
  uint8_t id;
  char* label;
};

static struct node_info_s node_info[MAX_NODES];
static uint8_t node_info_index = 0;

//
// Display shortest path from dijkstra's source to destination with node_id
// NOTE: MUST be run AFTER dijkstra() function
//
void print_shortest_path( uint8_t node_id )
{
  node_t* p_node;

  p_node = find_node( node_id );

  if ( p_node->p_previous == p_node )
  {

    printf("No path to node ");
    print_node_name( p_node->id );
  }
  else
  {
    print_node_name( p_node->id );
    while( p_node->p_previous != p_node )
    {
      printf("-(%g)",
              find_link( p_node->p_previous->id, p_node->id )->links_power);
      p_node = p_node->p_previous;
      printf("->");
      print_node_name( p_node->id );

    }
  }

 printf("\n");

}

//
// Same as add_node but with the option to add a node label for debugging
//
uint8_t add_labeled_node( uint8_t node_id, uint8_t is_relay, char* label )
{
  uint8_t return_value;

  return_value = add_node( node_id, is_relay );

  if ( !return_value )
  {
    // Add label
    node_info[node_info_index].id = node_id;
    node_info[node_info_index].label = malloc( strlen(label) + 1 );
    memcpy( node_info[node_info_index].label, label, strlen(label) + 1 );

    //printf("Added Node %s\n", node_info[node_info_index].label);

    node_info_index++;
  }


  // Add node
  return return_value;
}

//
// Free memory allocated by add_labeled_node
//
void cleanup_node_labels()
{
  while(node_info_index > 0)
  {
    node_info_index--;
    free(node_info[node_info_index].label);
  }
}

void print_node_name( uint8_t node_id )
{
  uint8_t node_index;

  for( node_index = 0; node_index < node_info_index; node_index++ )
  {
    if( node_info[node_index].id == node_id )
    {
      printf( "%s", node_info[node_index].label );
    }
  }

  return;
}

void print_link( link_t* link )
{
    print_node_name( link->source );
    printf("-");
    print_node_name( link->destination );
    if(link->active)
    {
      printf(" A");
    }
    else
    {
      printf(" x");
    }
}

void print_all_links()
{
  uint8_t link_index;

  printf("LINKS:\n");

  for( link_index = 0; link_index < s_links.current_links; link_index++ )
  {
    print_link( &s_links.links[link_index] );

    printf( " %g\n", s_links.links[link_index].links_power );


  }
  printf("\n");

}

void print_node_energy( uint8_t source_id, FILE* fp_out )
{
  uint8_t node_index;

  //printf("MEAN  ");

  for( node_index = 0; node_index < s_nodes.current_nodes; node_index++ )
  {
    if( s_nodes.nodes[node_index].id != source_id )
    {
      //print_node_name( s_nodes.nodes[node_index].id );
      //printf("    ");
    }
  }

  //printf("\n");

  fprintf( fp_out, "%g,", s_mean_energy );

  for( node_index = 0; node_index < s_nodes.current_nodes; node_index++ )
  {
    if( s_nodes.nodes[node_index].id != source_id )
    {
      fprintf(fp_out, "%g,", s_nodes.nodes[node_index].energy );
    }
  }

  fprintf(fp_out, "\n");

}

void print_all_nodes( uint8_t source_id )
{
  uint8_t node_index;

  printf("MEAN,");

  for( node_index = 0; node_index < s_nodes.current_nodes; node_index++ )
  {
    if( s_nodes.nodes[node_index].id != source_id )
    {
      print_node_name( s_nodes.nodes[node_index].id );
      printf(",");
    }
  }
  printf("\n");

}

//
// Generate GraphViz file and render image of network
//
void generate_graph( uint8_t source_id, uint32_t file_number )
{
  uint8_t node_index;
  uint8_t link_index;
  FILE* f_graph;
  char command[100];

  f_graph = fopen("images/graph.gv", "w" );

  if ( f_graph != NULL )
  {
    fprintf( f_graph,  "digraph network" );

    fprintf( f_graph,  " {\n" );

    // Configuration stuff

    fprintf( f_graph, "edge [len=3]\n");
    fprintf( f_graph, "nodesep=0.25\n");
    fprintf( f_graph, "node[shape = doublecircle]; ");

    // print_node_name to file
      for( node_index = 0; node_index < node_info_index; node_index++ )
      {
        if( node_info[node_index].id == source_id )
        {
          fprintf( f_graph, "%s", node_info[node_index].label );
        }
      }

    fprintf( f_graph, ";\n");

    fprintf( f_graph, "node[shape = circle];\n");

    // All connections
    for( link_index = 0; link_index < s_links.current_links; link_index++ )
    {

      // print_node_name to file
      for( node_index = 0; node_index < node_info_index; node_index++ )
      {
        if( node_info[node_index].id == s_links.links[link_index].destination )
        {
          fprintf( f_graph, "%s", node_info[node_index].label );
        }
      }

      fprintf( f_graph,  " -> " );

      // print_node_name to file
      for( node_index = 0; node_index < node_info_index; node_index++ )
      {
        if( node_info[node_index].id == s_links.links[link_index].source )
        {
          fprintf( f_graph, "%s", node_info[node_index].label );
        }
      }

      fprintf( f_graph, "[ label=\"");
      fprintf( f_graph,  "%g", s_links.links[link_index].links_power );
      fprintf( f_graph, "\" ]");
      fprintf( f_graph,  ";\n" );
    }

    // print_node_name to file
    for( node_index = 0; node_index < node_info_index; node_index++ )
    {
      fprintf( f_graph, "%s [label=\"%s\\n(%g)\"];", node_info[node_index].label,
        node_info[node_index].label,
        find_node(node_info[node_index].id)->energy );
    }

    fprintf( f_graph,  "}\n" );

    fclose( f_graph );

    // Generate svg of graph using GraphViz 'neato'
    //system("neato -Tsvg -ograph.svg graph.gv");
    //system("dot -Tsvg -ograph.svg graph.gv");
    //sprintf( command, "neato -Tjpg -oimages/graph%04d.jpg images/graph.gv", file_number);
    sprintf( command, "dot -Tjpg -oimages/graph%04d.jpg images/graph.gv", file_number);
    system( command );

  }
}

#endif
