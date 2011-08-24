#include <itpp/itbase.h>
#include <itpp/signal/fastica.h>
#include <iostream>

using namespace itpp;

using std::cout;
using std::endl;

#define TOTAL_LEADS (12)
#define INBUFSIZE (1024)
#define MAX_SAMPLES (1000)

void parse_line( char*, double* );
uint8_t parse_table( FILE* , double p_value_table[][MAX_SAMPLES] );

int main( int32_t argc, char *argv[] )
{
  FILE *fp_in; 
  double samples[TOTAL_LEADS][MAX_SAMPLES];
  
  // Make sure the filename is included
  if ( argc < 2 )
  {
    printf( "Usage: %s infile\r\n", argv[0] );
    return 1;
  }
  
  // Open input csv file
  fp_in = fopen( argv[1], "r" );
  
  if( NULL == fp_in )
  {
    printf( "Error opening input file.\r\n" );
    return 1;
  }
  
  parse_table(fp_in, samples);
  
  fclose(fp_in);

#if 0  
  mat leads_to_reconstruct; // Whichever leads are not used to generate ICs
  mat leads_initial; // just the 3 leads (I(1),II(2), and V2(8))
  mat leads_new; // New leads used to generate the ICs for each new beat
  
  // Training  
  Fast_ICA fastica(leads_initial); 
  fastica.set_nrof_independent_components(3);
  fastica.set_approach(FICA_APPROACH_DEFL);
  fastica.separate();
    
  mat ICs_initial = fastica.get_independent_components();
  mat A_initial = fastica.get_mixing_matrix();

  // Generate transform from IC to remaining leads
  mat reconst_transform = leads_to_reconstruct * ICs_initial.T() * 
                                  inv(ICs_initial * ICs_initial.T() );

  // Reconstruction
  Fast_ICA fastica2(leads_new);
  fastica2.set_nrof_independent_components(3);
  fastica2.set_approach(FICA_APPROACH_DEFL);
  fastica2.set_init_guess(A_initial);
  fastica2.separate();
  
  mat ICs_current = fastica2.get_independent_components();
  mat A_current = fastica2.get_mixing_matrix();
  mat leads_reconst = reconst_transform * ICs_current;
#endif

  return 0;

}


/*******************************************************************************
 * @fn    void parse_line ( char* csv_line, int8_t* output_line )
 *
 * @brief Parse line from csv file an populate array row with contents
 * ****************************************************************************/
void parse_line ( char* csv_line, double* output_line )
{
  char *p_item;
  uint16_t item_index = 0;
  
  p_item = strtok( csv_line, "," );
  while( NULL != p_item )
  {    
    // Convert string to RSSI value and store in output_line
    //output_line[item_index] = strtod( p_item, NULL );
    printf("%g", strtod( p_item, NULL ));
    item_index++;
    p_item = strtok( NULL, "," );
    
    if( NULL != p_item )
    {
      printf(",");
    }
  }
}

/*******************************************************************************
 * @fn    uint8_t parse_table ( FILE* fp_csv_file, 
 *                                      int8_t p_value_table[][MAX_DEVICES+1] )
 *
 * @brief Read lines from CSV file and parse them until an empty line is found
 * ****************************************************************************/
uint8_t parse_table ( FILE* fp_csv_file, double p_value_table[][MAX_SAMPLES] )
{
  char csv_line[INBUFSIZE];   // Buffer for reading a line in the file
  uint16_t line_index = 0;

  while( NULL != fgets( csv_line, sizeof(csv_line), fp_csv_file ) )
  {
    // Remove newline
    csv_line[(int32_t)strlen(csv_line)-1] = 0;
    
    // Parse csv line and populate array
    parse_line( csv_line, p_value_table[line_index] ); 
    
    printf("\r\n");           
    
    if( line_index > MAX_SAMPLES )
    {
      // Don't want to overflow the array. Return error.
      printf("OVERFLOW\n");
      return 0;
    }
    
    line_index++;
  }
  
  // Did not read table successfully
  return 0;
}
