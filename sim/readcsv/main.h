/** @file main.h
*
* @brief Main definitions and data types
*
* @author Alvaro Prieto
*/
#ifndef _MAIN_H
#define _MAIN_H



// Lookup table for converting cc2500 rssi value to received power in dBm
static const double rssi_values[256] = {
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

// Register settings for powers in power_values array
static const uint8_t power_settings[] = { 
0x00, 0x40, 0x50, 0x60, 0x70, 0x80, 0x44, 0x90, 0x41, 0x42, 
0xC0, 0x48, 0x43, 0x54, 0xA0, 0xB0, 0x84, 0x74, 0x51, 0x52, 
0x53, 0x58, 0x5C, 0xE0, 0x61, 0x81, 0x82, 0x72, 0x88, 0x83, 
0x8C, 0x94, 0x6C, 0x45, 0xC4, 0x46, 0x47, 0xA4, 0xB4, 0x49, 
0x91, 0x4E, 0x92, 0xC1, 0xC2, 0x93, 0x4B, 0x4F, 0xC3, 0xC8, 
0xD4, 0xCC, 0xA1, 0xA2, 0xB2, 0xA3, 0xB3, 0xA8, 0xB8, 0x55, 
0x56, 0x85, 0x86, 0xD2, 0x87, 0xD3, 0xD8, 0xE1, 0x89, 0x8D, 
0x8A, 0x8E, 0xE3, 0xF3, 0x65, 0x8B, 0x66, 0x59, 0x76, 0x5A, 
0x5D, 0xE8, 0x5E, 0xEC, 0xFC, 0x67, 0x77, 0x5B, 0xC5, 0x5F, 
0xC6, 0xC7, 0x95, 0x96, 0xC9, 0xCA, 0xCE, 0x97, 0xCB, 0xCF, 
0xB5, 0x6A, 0x6D, 0x99, 0x9D, 0x9A, 0x9E, 0xD5, 0x7E, 0xD6, 
0x9B, 0x9F, 0xD7, 0x7B, 0x7F, 0xE5, 0xE6, 0xE7, 0xD9, 0xDA, 
0xDE, 0xDB, 0xDF, 0xAA, 0xAE, 0xBE, 0xAB, 0xAF, 0xBB, 0xBF, 
0xE9, 0xF9, 0xEA, 0xFA, 0xFD, 0xEE, 0xFE, 0xEB, 0xEF, 0xFF };

// Transmit power (in dBm) for register settings in power_settings array
static const double power_values[] = { 
-65.4, -33.9, -31.1, -29.5, -29.4, -29.3, -28.4, -26.4, -26.3, -26.2, 
-25.9, -25.8, -25.6, -25.4, -24.9, -24.8, -23.7, -23.6, -23.4, -23.2, 
-22.7, -22.5, -22.2, -21.9, -21.8, -21.7, -21.6, -21.5, -21.1, -21.0, 
-20.8, -20.6, -20.5, -20.4, -20.3, -20.2, -19.7, -19.0, -18.9, -18.8, 
-18.7, -18.6, -18.5, -18.3, -18.2, -18.0, -17.9, -17.8, -17.7, -17.5, 
-17.4, -17.3, -17.1, -17.0, -16.9, -16.5, -16.4, -16.0, -15.9, -15.8, 
-15.7, -15.6, -15.5, -15.4, -15.0, -14.8, -14.3, -14.2, -14.1, -14.0, 
-13.9, -13.8, -13.5, -13.5, -13.4, -13.3, -13.2, -13.1, -13.0, -12.9, 
-12.9, -12.8, -12.7, -12.5, -12.4, -12.3, -12.2, -12.1, -12.0, -11.9, 
-11.8, -11.3, -10.9, -10.7, -10.4, -10.3, -10.2, -10.1, -9.80, -9.70, 
-8.50, -8.40, -8.30, -8.00, -7.90, -7.80, -7.70, -7.60, -7.50, -7.40, 
-7.20, -7.00, -6.80, -6.50, -6.10, -5.70, -5.50, -4.70, -4.60, -4.40, 
-4.30, -3.80, -3.70, -3.50, -3.10, -2.70, -2.30, -1.90, -1.80, -1.30, 
-0.80, -0.60, -0.40, -0.20, -0.10, +0.00, +0.30, +0.70, +1.10, +1.50 };

#endif /*_MAIN_H */
