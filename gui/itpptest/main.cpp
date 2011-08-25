#include <itpp/itbase.h>
#include <itpp/signal/fastica.h>
#include <itpp/signal/sigfun.h>
#include <itpp/base/math/misc.h>
#include <itpp/itstat.h>
#include <iostream>

using namespace itpp;

using std::cout;
using std::endl;
#define TOTAL_LEADS (15)
#define INBUFSIZE (1024)
#define MAX_SAMPLES (1000)

void parse_line( char*, mat* );
int parse_table( FILE* , mat* );

int main( int32_t argc, char *argv[] )
{
  FILE *fp_in, *fp_out; 
  
  mat *samples;             // Matrix with sampled data
  mat leads_initial;        // Initial sampled leads (I,II, and V2)
  mat leads_to_reconstruct; // Leads not used to generate ICs
  mat *leads_new;            // New leads to generate the ICs for each new beat
  mat ICs_initial;          //
  mat reconst_transform;    // Transform used to reconstruct 12 leads from 3
  mat A_initial;
  mat ICs_current;
  mat A_current;
  mat leads_reconst;
    
  vec *temp;
  
  Fast_ICA *fastica_train_reconstruct;
  
  int rows, cols;
  

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
  
  // Create new matrix to store sampled data
  samples = new mat;
  
  // Read in CSV file and populate samples matrix
  parse_table(fp_in, samples);
  
  // Close input file
  fclose(fp_in);
    
  //
  // Construct leads_initial and leads_to_reconstruct matrices from csv file
  // data stored in the samples matrix
  //
  
  // Leads I(0),II(1), and V2(7)
  leads_initial.append_row( samples->get_col(0) );
  leads_initial.append_row( samples->get_col(1) );
  leads_initial.append_row( samples->get_col(7) );
  
  // Whichever leads are not used to generate ICs
  leads_to_reconstruct.append_row( samples->get_col(2) );
  leads_to_reconstruct.append_row( samples->get_col(3) );
  leads_to_reconstruct.append_row( samples->get_col(4) );
  leads_to_reconstruct.append_row( samples->get_col(5) );
  leads_to_reconstruct.append_row( samples->get_col(6) );
  leads_to_reconstruct.append_row( samples->get_col(8) );
  leads_to_reconstruct.append_row( samples->get_col(9) );
  leads_to_reconstruct.append_row( samples->get_col(10) );
  leads_to_reconstruct.append_row( samples->get_col(11) );
  leads_to_reconstruct.append_row( samples->get_col(12) );
  leads_to_reconstruct.append_row( samples->get_col(13) );
  leads_to_reconstruct.append_row( samples->get_col(14) );

  
  cout << "Training." << endl;
  
  //
  // Begin ICA training
  //  
  Fast_ICA fastica_train(leads_initial); 
  fastica_train.set_nrof_independent_components(3);
  fastica_train.set_approach(FICA_APPROACH_DEFL);
  fastica_train.separate();  
  ICs_initial = fastica_train.get_independent_components();  
  A_initial = fastica_train.get_mixing_matrix();

  //
  // Generate transform from IC to remaining leads
  //
  reconst_transform = leads_to_reconstruct * ICs_initial.T() *
                                  inv( ICs_initial * ICs_initial.T() );
                                  
  cout << "Done training." << endl;
        
  // Open input csv file
  fp_in = fopen( argv[2], "r" );
  
  if( NULL == fp_in )
  {
    printf( "Error opening input file.\r\n" );
    return 1;
  }
  
  // Open input csv file
  fp_out = fopen( argv[3], "w" );
  
  if( NULL == fp_out )
  {
    printf( "Error opening output file.\r\n" );
    return 1;
  }
  
  fclose(fp_out);
  
  // Free up memory used by the samples matrix
  delete samples;
  samples = new mat;
  
  leads_new = new mat;
  
  // Read in CSV file and populate samples matrix
  while( parse_table(fp_in, samples) )
  //parse_table(fp_in, samples);
  {  
    
    
    // Leads I(0),II(1), and V2(7)
    leads_new->append_row( samples->get_col(0) );
    leads_new->append_row( samples->get_col(1) );
    leads_new->append_row( samples->get_col(7) );
    
    cout << "Reconstructing " << leads_new->rows() << "," << leads_new->cols()  << endl;

    // Reconstruction
    fastica_train_reconstruct = new Fast_ICA(*leads_new);
    fastica_train_reconstruct->set_nrof_independent_components(3); 
    //fastica_train_reconstruct->set_approach(FICA_APPROACH_DEFL);
    fastica_train_reconstruct->set_init_guess(A_initial);
    fastica_train_reconstruct->separate();    
    
    ICs_current = fastica_train_reconstruct->get_independent_components();
    
    //A_current = fastica_train_reconstruct->get_mixing_matrix();
    
    //
    // Sort ICs
    //
    
    
    int temp_ind;
    double temp_value;
    
    mat ind_max;
    vec tmp_vector(2);
    
    int perm_sel;
    
    mat *sort_tran;
    
    vec cov_values(3);
    
    mat ICs_sorted;
    
    mat IC_perms("0 0 1 1 2 2;1 2 0 2 0 1;2 1 2 0 1 0");
    
    mat neg_trans;
    

    
    for( int i = 0; i < 6; i++)
    {
      temp = new vec(2*MAX_SAMPLES-1);
      temp->zeros();
      for( int j = 0; j < 3; j++ )
      {
        *temp +=  abs( xcorr( (ICs_initial.get_row(j)-mean(ICs_initial.get_row(j)))
          ,( ICs_current.get_row(IC_perms(j,i))-mean(ICs_current.get_row( IC_perms(j,i))))));
          
          //cout << "IC_perms("<<i<<","<<j<<")" << IC_perms(j,i) << endl;
      }
      
      temp_value = max(*temp, temp_ind);

      tmp_vector[0] = temp_value;
      tmp_vector[1] = temp_ind;            
      
      ind_max.append_row( tmp_vector );
      
      delete temp;
    }
    
    ind_max.set(0,0, 1.25 * ind_max(0,0) );
    
    max( ind_max.get_col(0), perm_sel );
    
    sort_tran = new mat(3,3);
    sort_tran->zeros();
    
    for ( int i = 0; i < 3; i++ )
    {
      sort_tran->set(IC_perms(perm_sel,i),i, 1);
      temp = new vec;
      *temp = xcorr( 
        (ICs_initial.get_row(i)-mean(ICs_initial.get_row(i))),
        (ICs_current.get_row(IC_perms(i,perm_sel))-mean(ICs_current.get_row( IC_perms(i,perm_sel)))) );
              
      cov_values[i]=(*temp)[(int)ind_max(perm_sel,1)];
      
      delete temp;
    }
    
    
    
    neg_trans = eye(3);
    
    for( int i = 0; i < 3; i++ )
    {
      if ( cov_values[i] < 0 )
      {
        neg_trans.set(i,i,-1);
      }
      else
      {
        neg_trans.set(i,i,1);
      }
    }
    
    ICs_sorted = neg_trans * (*sort_tran) * ICs_current;
    
    delete sort_tran;
    
    leads_reconst = (reconst_transform * ICs_sorted).T();    

    cout << "Writing recostructed leads to file" << endl;    

    // Open input csv file
    fp_out = fopen( argv[3], "a" );
    
    if( NULL == fp_out )
    {
      printf( "Error opening output file.\r\n" );
      return 1;
    }

    // Output reconstructed leads to file
    for ( rows = 0; rows < leads_reconst.rows(); rows++ )
    {
      for ( cols = 0; cols < leads_reconst.cols(); cols++ )
      {
        fprintf(fp_out, "%.3f,", leads_reconst.get(rows,cols) );
      }
      fprintf(fp_out, "\n" );
    }
    
    fclose(fp_out);
    
    delete fastica_train_reconstruct;
    
    // Free up memory used by the samples matrix
    delete samples;
    samples = new mat;
    
    // Free up memory used by the samples matrix
    delete leads_new;
    leads_new = new mat;
  }
  
  // Close input file
  fclose(fp_in);
  
  cout << "done" << endl;

  // Cleanup  
  delete samples;
  delete leads_new;

  return 0;

}


/*******************************************************************************
 * @fn    void parse_line ( char* csv_line, mat* output_line )
 *
 * @brief Parse line from csv file an populate vector with contents
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
 * @fn    int parse_table ( FILE* fp_csv_file, mat* p_value_table )
 *
 * @brief Read (up to MAX_SAMPLES)lines from CSV file and parse them
 * ****************************************************************************/
int parse_table ( FILE* fp_csv_file, mat* p_value_table )
{
  char csv_line[INBUFSIZE];   // Buffer for reading a line in the file
  uint16_t line_index = 0;
  

  while( NULL != fgets( csv_line, sizeof(csv_line), fp_csv_file ) )
  {  
    // Remove newline
    csv_line[(int32_t)strlen(csv_line)-1] = 0;
    
    // Parse csv line and populate array
    parse_line( csv_line, p_value_table );         
    
    // Keep track of how many lines have been read
    line_index++;
    
    // Stop reading
    if( line_index >= MAX_SAMPLES )
    {
      return 1;
    }
  }
  
  // Did not read table successfully
  return 0;
}
