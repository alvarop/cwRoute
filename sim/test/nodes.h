/** @file nodes.h
*
* @brief Node and supporting function definitions for routing algorithm
*
* @author Alvaro Prieto
*/
#ifndef _NODES_H
#define _NODES_H

#include <stdio.h>
#include <stdint.h>

typedef struct 
{
  double distance;  // Used by dijkstra's algorithm
  double energy;    // Accumulated energy
  uint8_t id;       // Node id
  uint8_t previous; // Previous node
  uint8_t visited;  //
  uint8_t is_relay; // Is this a relay(1) or sensor node(0)
} node_t;

typedef struct 
{
  double links_power;   // Constant (how much power does the link require)
  double current_cost;  // Relative to mean
  uint8_t source;       // 
  uint8_t destination;  // 
} link_t;

#endif /* _NODES_H */\

