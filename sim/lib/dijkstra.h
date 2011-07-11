/** @file dijkstra.h
*
* @brief Node and supporting function definitions for routing algorithm
*
* @author Alvaro Prieto
*/
#ifndef _NODES_H
#define _NODES_H

#define MAX_NODES (10)
#define MAX_LINKS (30)
#define MAX_DISTANCE 1e99

// Use type definition since actual datatype might change
// (don't want to use floating point on the microcontroller...)
typedef double energy_t ;

typedef struct node_s node_t;
typedef struct link_s link_t;

struct node_s
{
  energy_t distance;    // Used by dijkstra's algorithm
  energy_t energy;      // Accumulated energy
  node_t* p_previous;   // Pointer to revious node
  uint8_t id;           // Node id
  uint8_t visited;      //
  uint8_t is_relay;     // Is this a relay node?
};

struct link_s
{
  energy_t links_power;   // Constant (how much power does the link require)
  energy_t current_cost;  // Relative to mean
  uint8_t source;         // 
  uint8_t destination;    //
  uint8_t used; 
};

typedef struct
{
  uint8_t current_nodes;
  node_t nodes[MAX_NODES];
} nodes_t;

typedef struct
{
  uint8_t current_links;
  link_t links[MAX_LINKS];
} links_t;

energy_t difference( energy_t, energy_t );

uint8_t add_node( uint8_t, uint8_t );
uint8_t add_link( uint8_t, uint8_t, energy_t );
uint8_t dijkstra( uint8_t );
void print_shortest_path( uint8_t );

#ifdef DEBUG_ON
uint8_t add_labeled_node( uint8_t, uint8_t, char* );
void cleanup_node_labels();
void print_node_name( uint8_t  );
void print_link( link_t* link );
void print_all_links();
#endif

#endif /* _NODES_H */\
