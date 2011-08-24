#include <itpp/itbase.h>
#include <itpp/signal/fastica.h>
#include <iostream>

using namespace itpp;

using std::cout;
using std::endl;
#define TOTAL_LEADS (15)
#define INBUFSIZE (1024)
#define MAX_SAMPLES (1000)

void parse_line( char*, mat* );
uint8_t parse_table( FILE* , mat* );

int main( int32_t argc, char *argv[] )
{
  FILE *fp_in, *fp_out; 
  mat *samples;
  
  
  
  // Make sure the filename is included
  if ( argc < 4 )
  {
    printf( "Usage: %s infile1 infile2 outfile\r\n", argv[0] );
    return 1;
  }
  
  // Open input csv file
  fp_in = fopen( argv[1], "r" );
  
  if( NULL == fp_in )
  {
    printf( "Error opening input file.\r\n" );
    return 1;
  }
  
  samples = new mat;
  // Read in CSV file and populate samples matrix
  parse_table(fp_in, samples);
  
  // Close input file
  fclose(fp_in);
  
  //cout << *samples << endl;
  
  mat *leads_initial = new mat;
  mat *leads_to_reconstruct = new mat;
  
  // New leads used to generate the ICs for each new beat
  mat *leads_new = new mat;
  
  // Leads I(0),II(1), and V2(7)
  leads_initial->append_row( samples->get_col(0) );
  leads_initial->append_row( samples->get_col(1) );
  leads_initial->append_row( samples->get_col(7) );
  
 
  // Whichever leads are not used to generate ICs
  leads_to_reconstruct->append_row( samples->get_col(2) );
  leads_to_reconstruct->append_row( samples->get_col(3) );
  leads_to_reconstruct->append_row( samples->get_col(4) );
  leads_to_reconstruct->append_row( samples->get_col(5) );
  leads_to_reconstruct->append_row( samples->get_col(6) );
  leads_to_reconstruct->append_row( samples->get_col(8) );
  leads_to_reconstruct->append_row( samples->get_col(9) );
  leads_to_reconstruct->append_row( samples->get_col(10) );
  leads_to_reconstruct->append_row( samples->get_col(11) );
  leads_to_reconstruct->append_row( samples->get_col(12) );
  leads_to_reconstruct->append_row( samples->get_col(13) );
  leads_to_reconstruct->append_row( samples->get_col(14) );


  cout << "Starting fastica" << endl;
  
  // Training  
  Fast_ICA fastica(*leads_initial); 
  fastica.set_nrof_independent_components(3);
  fastica.set_approach(FICA_APPROACH_DEFL);
  fastica.separate();
  
  cout << "done separating" << endl;
  
  mat ICs_initial = fastica.get_independent_components();
  
  cout << "done getting ICs" << endl;
  
  mat A_initial = fastica.get_mixing_matrix();
  
  cout << "done getting mixing matrix" << endl;  

  // Generate transform from IC to remaining leads
  mat reconst_transform = (*leads_to_reconstruct) * ICs_initial.T() *
                                  inv( ICs_initial * ICs_initial.T() );
                                  

  delete samples;
    
  // Open input csv file
  fp_in = fopen( argv[2], "r" );
  
  if( NULL == fp_in )
  {
    printf( "Error opening input file.\r\n" );
    return 1;
  }
  
  samples = new mat;
  // Read in CSV file and populate samples matrix
  parse_table(fp_in, samples);
  
  // Close input file
  fclose(fp_in);
  
    // Leads I(0),II(1), and V2(7)
  leads_new->append_row( samples->get_col(0) );
  leads_new->append_row( samples->get_col(1) );
  leads_new->append_row( samples->get_col(7) );
  
  cout << "Reconstructing " << leads_new->rows() << "," << leads_new->cols()  << endl;

  // Reconstruction
  Fast_ICA fastica2(*leads_new);
  
  fastica2.set_nrof_independent_components(3);
 
  //fastica2.set_approach(FICA_APPROACH_DEFL);

  fastica2.set_init_guess(A_initial);

  fastica2.separate();    
  
  mat ICs_current = fastica2.get_independent_components();
  
  mat A_current = fastica2.get_mixing_matrix();
  mat leads_reconst = (reconst_transform * ICs_current).T();

  

  cout << "Writing recostructed leads to file" << endl;
  
  // Open input csv file
  fp_out = fopen( argv[3], "w" );
  
  if( NULL == fp_out )
  {
    printf( "Error opening output file.\r\n" );
    return 1;
  }   
  
  int rows, cols;    
  
  // Output reconstructed leads to file
  for ( rows = 0; rows < leads_reconst.rows(); rows++ )
  {
    for ( cols = 0; cols < leads_reconst.cols(); cols++ )
    {
      fprintf(fp_out, "%g,", leads_reconst.get(rows,cols) );
    }
    fprintf(fp_out, "\n" );
  }

  // Close output file
  fclose(fp_out);
  
  cout << "done" << endl;

  // Cleanup
  delete leads_to_reconstruct;
  delete leads_initial;
  delete leads_new; 
  
  delete samples;

  return 0;

}


/*******************************************************************************
 * @fn    void parse_line ( char* csv_line, int8_t* output_line )
 *
 * @brief Parse line from csv file an populate array row with contents
 * ****************************************************************************/
void parse_line ( char* csv_line, mat* output_line )
{
  char *p_item;
  uint16_t item_index = 0;
  vec line(TOTAL_LEADS);
  
  p_item = strtok( csv_line, "," );
  while( NULL != p_item )
  {    
    // Convert string to RSSI value and store in output_line
    line[item_index] = strtod( p_item, NULL );
    item_index++;
    p_item = strtok( NULL, "," );    
  }
  output_line->append_row( line );
  
}

/*******************************************************************************
 * @fn    uint8_t parse_table ( FILE* fp_csv_file, 
 *                                      int8_t p_value_table[][MAX_DEVICES+1] )
 *
 * @brief Read lines from CSV file and parse them until an empty line is found
 * ****************************************************************************/
uint8_t parse_table ( FILE* fp_csv_file, mat* p_value_table )
{
  char csv_line[INBUFSIZE];   // Buffer for reading a line in the file
  uint16_t line_index = 0;
  

  while( NULL != fgets( csv_line, sizeof(csv_line), fp_csv_file ) )
  {  
    // Remove newline
    csv_line[(int32_t)strlen(csv_line)-1] = 0;
    
    // Parse csv line and populate array
    parse_line( csv_line, p_value_table );         
    
    line_index++;
    
    if( line_index >= MAX_SAMPLES )
    {
      return 1;
    }
  }
  
  // Did not read table successfully
  return 0;
}
