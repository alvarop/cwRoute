/** @file dijkstra.c
*
* @brief Node and supporting function definitions for routing algorithm
*
* @author Alvaro Prieto
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dijkstra.h"

static nodes_t nodes;
static links_t links;

energy_t difference( energy_t number_a, energy_t number_b )
{
  if ( number_a > number_b )
  {     
    return number_a - number_b;
  }
  else
  {
    return number_b - number_a;
  }  
}

//
// Add new node to nodes list
//
uint8_t add_node( uint8_t node_id, uint8_t is_relay )
{
  node_t* new_node;
  if( nodes.current_nodes < MAX_NODES )
  {
    new_node = &nodes.nodes[nodes.current_nodes];
    new_node->id = node_id;   
    new_node->energy = 0;
    new_node->visited = 0;
    new_node->is_relay = is_relay;
    
    nodes.current_nodes++;   
    
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
  
  if( links.current_links < MAX_LINKS )
  {
    new_link = &links.links[links.current_links];
    new_link->links_power = link_power; 
    new_link->source = source;
    new_link->destination = destination;
    
    new_link->current_cost = link_power;  // Should be zero, but currently used
                                          // for testing dijksta's algorithm
    new_link->used = 0;
        
    links.current_links++;   
    
    return 0;
  }
  else
  {
    // Error, ran out of space in link list
    return 1;
  }
}

//
// Returns pointer to node with matching node_id
// If node is not found, returns NULL pointer
//
node_t* find_node( uint8_t node_id )
{
  uint8_t node_index;
  
  for( node_index = 0; node_index < nodes.current_nodes; node_index++ )
  {
    if( nodes.nodes[node_index].id == node_id )
    {
      return &nodes.nodes[node_index];    
    }  
  }
  
  return NULL;
}

//
// Return 1 if all nodes are marked as visited
//
uint8_t all_nodes_visited()
{
  uint8_t nodes_visited = 1;
  uint8_t node_index;
  
    
  for( node_index = 0; node_index < nodes.current_nodes; node_index++ )
  {
    nodes_visited &= nodes.nodes[node_index].visited;
  }  
  
  return nodes_visited;
}

//
// Find the node with the smallest distance that has not been visited
//
node_t* node_with_smallest_distance( )
{
  uint8_t node_index;
  uint8_t best_node = 0;
  energy_t min_distance=MAX_DISTANCE;
  
  for( node_index = 0; node_index < nodes.current_nodes; node_index++ )
  {
    // If the current node has not been visited and has a smaller distance
    // than the current minimum, select it as the new minimum
    if ( ( nodes.nodes[node_index].distance < min_distance ) &&
         ( 0 == nodes.nodes[node_index].visited ) )
    {
      best_node = node_index;
      min_distance = nodes.nodes[node_index].distance;
    }
  }  
  
  return &nodes.nodes[best_node];
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
#ifdef DEBUG_ON
  printf("Initialize nodes.\n"); // DEBUG
#endif
  
  for( node_index = 0; node_index < nodes.current_nodes; node_index++ )
  {
    nodes.nodes[node_index].distance = MAX_DISTANCE;
    nodes.nodes[node_index].p_previous = &nodes.nodes[node_index]; // point to self
  }
  
  // Get actual array index from node_id and re-use source_id variable
  p_source_node = find_node( source_id );

#ifdef DEBUG_ON  
  print_node_name( p_source_node->id );
  printf( " is the source.\n" ); //DEBUG
#endif
  
  if( NULL == p_source_node )
  {
    printf("Error: Node %d not found!\n", source_id );
    exit(1);
  }
  
  // Start with distance of 0, since it is the starting point
  p_source_node->distance = 0;

#ifdef DEBUG_ON
  printf("Start main loop.\n"); // DEBUG
#endif

  while( !all_nodes_visited() )
  {
    p_source_node = node_with_smallest_distance();

#ifdef DEBUG_ON    
    print_node_name( p_source_node->id );
    printf( " has current smallest distance.\n" ); // DEBUG
#endif
    
    if ( MAX_DISTANCE == p_source_node->distance )
    {
      // No more nodes are accessible
#ifdef DEBUG_ON
      printf("No more nodes are accessible!\n"); // DEBUG
#endif
      break;      
    }
  
    p_source_node->visited = 1;
    
    for( link_index = 0; link_index < links.current_links; link_index++ )
    {
      p_destination_node = NULL;
      p_current_link = &links.links[link_index];
      
      // If this link has the source node, use other node as destination
      if( p_current_link->destination == p_source_node->id )
      {
        p_destination_node = find_node( p_current_link->source );
      } 
      else if( p_current_link->source == p_source_node->id )
      {
        p_destination_node = find_node( p_current_link->destination );
      }
      
      // Make sure there is a destination, otherwise do nothing
      if ( NULL != p_destination_node )
      {
#ifdef DEBUG_ON
        printf("  Found link "); // DEBUG
        print_link( p_current_link ); // DEBUG
        printf("\n"); // DEBUG
#endif      
        // Compute the possible distance for destination if current link is used
        possible_distance = p_source_node->distance +
                              p_current_link->current_cost;
        
        // If possible distance is smaller than current one, update destination
        if ( possible_distance < p_destination_node->distance )
        {
#ifdef DEBUG_ON
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
  
  return 0;//hops?;
}

//
// Display shortest path from dijkstra's source to destination with node_id
// NOTE: MUST be run AFTER dijkstra() function
//
void print_shortest_path( uint8_t node_id )
{
  node_t* p_node;
  
  p_node = find_node( node_id );
  
  print_node_name( p_node->id );
  while( p_node->p_previous != p_node )
  {
    p_node = p_node->p_previous;
    printf("->");
    print_node_name( p_node->id ); 
  };
  
  printf("\n");
  
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
    
    printf("Added Node %s\n", node_info[node_info_index].label);

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
}

void print_all_links()
{
  uint8_t link_index;
  
  for( link_index = 0; link_index < links.current_links; link_index++ )
  {
    print_link( &links.links[link_index] );

    //printf( " %0.2f", links.links[link_index].links_power );
  
    printf("\n");
  }

}


#endif

