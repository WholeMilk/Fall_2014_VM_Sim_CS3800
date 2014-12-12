/*-----------------------------------
            Includes
-----------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <vector>
#include <math.h>

/*-----------------------------------
            Defines
-----------------------------------*/
#define MAX_NUM_PAGE 512

/*-----------------------------------
        Types & Structures
-----------------------------------*/
struct program_table_struct
{
	int program_number;
	int program_size;
	int page_size;
	int page_needed;
	std::vector<int> UID;
};

struct frame_struct
{
	int page_num;
	int used_bit;
	unsigned long time_stamp;
};

/*-----------------------------------
        Enums & const string
-----------------------------------*/
enum algorithm
{
    CLK,
    LRU,
    FIFO
};

enum paging_policy
{
	DEMAND_PAGING,
    PREPAGING
};

const char* algorithm_str[] = { "Clock Based Policy", "Least Recently Used Policy", "First In First Out Policy" };
const char* paging_str[] = { "Demand Paging", "Pre-Paging" };

/*-----------------------------------
            Variables
-----------------------------------*/
//initialize program table
std::vector<program_table_struct> program_table;

//Initialize main memory table
std::vector<frame_struct> main_memory;

//file handle for program_list and program_trace
FILE *program_list_f;
FILE *program_trace_f;

//output buffer
char buf[200];

//page size
int page_size_input;

//algorithm
int algorithm_input;

//paging option
int paging_input;

//number of frames
unsigned int num_of_frames;

//program count
int program_count = 0;

//clock counter
int clock_counter = 0;

//program counter for LRU
unsigned long program_counter = 0;

//output message buffer
char output[200];

//frames per program
int frames_per_program;

//result counters
unsigned long page_fault_count = 0;

/*-----------------------------------
            Functions
-----------------------------------*/
//----- page access function prototype
int page_access( int program_num, int offset );


/*--------------------------------------
					main
--------------------------------------*/
int main( int argc, char *argv[] )
{
	if( ( argc == 7 ) && ( strcmp( argv[6], "-demo" ) == 0 ) );
	else
	{
		printf("\n=============================================\n\r" );
		printf("=         Virtual Memory Simulator          =\n\r" );
		printf("=          CS284    -   Xiao Deng           =\n\r" );
		printf("=============================================\n\r" );
		
		//printout input parameters
		// printf("> Parameter list: \n\r" );
		// for( int n=0; n<argc; n++ )
		// {
			// printf("(%d) %s \n\r", n, argv[n] );
		// }
		// printf("\n\r" );
		
		//check number of parameters
		if( ( argc <= 1 ) || ( argc > 6 ) )
		{
			printf(" Invalid arguments, use '-help' for format information\n");
			exit(1);
		}
		//help info
		if( (strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "-help") == 0) )
		{
			printf("- HELP -\n\r");
			printf(" Run this memory simulator with the following format and parameters:\n\r\n");
			printf("    memorysimulator [pl_filename] [pt_filename] [page_size] [algorithm] [paging_option]\n\r\n");
			printf("    -[pl_filename]:        complete filename for program list\n");
			printf("    -[pt_filename]:        complete filename for program trace\n");
			printf("    -[page_size]:          2, 4, 8, 16\n");
			printf("    -[algorithm]:          clock, fifo, lru\n");
			printf("    -[paging_option]:      0-Pre-paging, 1-Demand Paging\n\r\n");
			exit(0);
		}
	}
	
	if( access( argv[1], F_OK ) == -1)
	{
		printf(" Error open program list file (parameter 1), use '-help' for format information\n");
		exit(1);
	}
	
	if( access( argv[2], F_OK ) == -1)
	{
		printf(" Error open program trace file (parameter 2), use '-help' for format information\n");
		exit(1);
	}
	
    //read in page size
	page_size_input = atoi( argv[3] );
	if( ( ( page_size_input%2 != 0 ) && ( page_size_input != 1 ) ) || ( page_size_input < 1 ) )
	{
		printf(" Invalid page size (parameter 3), use '-help' for format information\n");
		exit(1);
	}
	
	if(strcmp(argv[4], "clock") == 0)
	{
		algorithm_input = CLK;
	}
	else if(strcmp(argv[4], "fifo") == 0)
	{
		algorithm_input = FIFO;
	}
	else if(strcmp(argv[4], "lru") == 0)
	{
		algorithm_input = LRU;
	}
	else
	{
		printf(" Invalid page replacement algorithm (parameter 4), use '-help' for format information\n");
		exit(1);
	}
	
	paging_input = atoi( argv[5] );
	if( ( paging_input != PREPAGING ) && ( paging_input != DEMAND_PAGING ) )
	{
		printf(" Invalid paging option (parameter 5), use '-help' for format information\n");
		exit(1);
	}
	
	//open program list file
	program_list_f = fopen( argv[1], "r" );
	int program_num_in;		//program number
	int program_size_in;	//program size
	
	int i = 0;				//program index
	int uid_i = 0;			//UID index
	
	if( argc != 7 )
	{
		printf( "[x] Processing program list file... " );
	}
	//read in and construct page table for each program
	while( !feof( program_list_f ) )
	{
		if( fscanf( program_list_f, "%d %d", &program_num_in, &program_size_in ) == 2 )
		{
			program_count++;
			program_table.push_back( program_table_struct() );
			program_table[i].program_number = program_num_in;
			program_table[i].program_size = program_size_in;
			program_table[i].page_size = page_size_input;
			program_table[i].page_needed = ( program_size_in / page_size_input ) + ( program_size_in % page_size_input );
			//program_table[i].UID.resize( program_table[i].page_needed );
            
			for( int n=0; n<program_table[i].page_needed; n++ )
			{
				program_table[i].UID.push_back( uid_i );
				uid_i++;
			}
			i++;
		}
	}
	if( argc != 7 )
	{
		printf( "   DONE!\n" );
	}
	
	//calculate some numbers
	num_of_frames = MAX_NUM_PAGE / page_size_input;
	frames_per_program = num_of_frames / program_count;
	
	//printout some numbers for debugging
	//printf( "num_of_frames = %d, frame_per_pgm = %d, program_count = %d\n", num_of_frames, frames_per_program, program_count );
	
	int frame_id = 0;			//frame id
	//time_t current;				//current time
	
	//initialize main memory
	for( int m=0; m<program_count; m++ )
	{
		for( int n=0; n<frames_per_program; n++ )
		{
			//time( &current );
			main_memory.push_back( frame_struct() );
            
            // if program size is less then allocated number of pages,
            // assign rest with -1 (empty)
            if( program_table[m].page_needed < frames_per_program )
            {
                main_memory[ frame_id ].page_num = -1;
            }
            else
            {
                main_memory[ frame_id ].page_num = program_table[m].UID[n];
            }
            
			main_memory[ frame_id ].used_bit = 1;
			main_memory[ frame_id ].time_stamp = program_counter;
			
			//print out initial main memory content for debugging
			//printf( "main[%d] = %d %d\n", frame_id, main_memory[ frame_id ].page_num, main_memory[ frame_id ].used_bit );
			
			frame_id++;
		}
	}
	
	//open program trace file
	program_trace_f = fopen( argv[2], "r" );
	//word offset
	int word_offset;
    
	if( argc != 7 )
	{
		printf( "[x] Processing program trace file..." );
	}
	//read in program trace
	while ( !feof( program_trace_f ) )
	{
		if( fscanf( program_trace_f, "%d %d", &program_num_in, &word_offset ) == 2 )
		{
			int found = page_access( program_num_in, word_offset );
		}
	}
	if( argc != 7 )
	{
		printf( "   DONE!\n" );
	}
    
    printf( "\n\r- RESULT ---------------------------------\n\r  Page Size: %d\n\r  Replacement Algorithm: %s\n\r  Paging Policy: %s\n\r  Total page access: %lu\n\r  Total Page faults: %lu\n\r------------------------------------------\n\r\n",
			page_size_input,
			algorithm_str[ algorithm_input ],
			paging_str[ paging_input ],
            program_counter,
            page_fault_count
            );
	
	fclose( program_list_f );
	fclose( program_trace_f );
	
	return 0;
}

/*--------------------------------------
		page access function
--------------------------------------*/
int page_access( int program_num, int offset )
{
	//get page offset
	int page_offset = offset / page_size_input;
	
	//get UID for page
	int access_UID = program_table[ program_num ].UID[ page_offset ];
	
	int main_mem_size = main_memory.size();
	int found = 0;
	int i = 0;
	
	//increment program counter
    program_counter++;
	
	//search for page in main memory
	while( ( i<main_mem_size ) && ( found == 0 ) )
	{
		//printf( "requested: %d, actual = %d\n", access_UID, main_memory[i].page_num );
		
		//if page is found in main memory, do things according to paging algorithm
		if( main_memory[i].page_num == access_UID )
		{
			switch( algorithm_input )
			{
				case LRU:
					//update time stamp
					main_memory[i].time_stamp = program_counter;
					break;
				case CLK:
					//update used_bit
					main_memory[i].used_bit = 1;
					break;
				case FIFO:
					break;
				default:
					break;
			}
			found = 1;
		}
		i++;
	}
    
	//if page is not found in main memory, do other things according to paging algorithm
    if( found == 0 )
    {
		long temp_time = program_counter;
		int replace_page = 0;
        
		page_fault_count++;
		
		switch( algorithm_input )
		{
			case FIFO:
			case LRU:
				i = 0;
				//find the frame with the smallest time stamp
				while( i<main_mem_size )
				{
					if( temp_time > main_memory[i].time_stamp )
					{
						temp_time = main_memory[i].time_stamp;
						replace_page = i;
					}
					i++;
				}
				main_memory[ replace_page ].page_num = access_UID;
				replace_page++;
				if( replace_page >= main_mem_size )
				{
					replace_page = 0;
				}
				break;
			case CLK:
				// rotate the clock counter to find the first frame with used_bit = 0
				// , for frames scanned which has used-bit - 1, set it to 0
				while( main_memory[ clock_counter ].used_bit == 1 )
				{
					main_memory[ clock_counter ].used_bit = 0;
					clock_counter++;
					if( clock_counter == main_mem_size)
					{
						clock_counter = 0;
					}
				}
				main_memory[ clock_counter ].page_num = access_UID;
				clock_counter++;
				if( clock_counter >= main_mem_size)
				{
					clock_counter = 0;
				}
				replace_page = clock_counter;
				break;
			default:
				break;
		}
		
		//handling pre-paging
		if( paging_input == PREPAGING )
		{
			int next_page_offset = page_offset+1;
			if( next_page_offset == program_table[ program_num ].program_size )
			{
				next_page_offset = 0;
			}
			int next_UID = program_table[ program_num ].UID[ next_page_offset ];
			
			main_memory[ replace_page ].page_num = next_UID;
			main_memory[ replace_page ].used_bit = 0;
		}
    }
	
	//printf( "\r [DEBUG] Page accesses: %lu, program_no = %d, offset = %d, access_UID = %d", program_counter, program_num, offset, access_UID );
	//printf( " ", );
	return found;
}

