/*****************************************************************************
 * lifepar_v1.c
 * First iteration of parallelized and optimized implementation
 ****************************************************************************/
#include "life.h"
#include "util.h"
#include <omp.h>
#include <unistd.h>
#include <stdio.h>

/**
 * Swapping the two boards only involves swapping pointers, not
 * copying values.
 */
#define SWAP_BOARDS( b1, b2 )  do { \
  char* temp = b1; \
  b1 = b2; \
  b2 = temp; \
} while(0)

#define BOARD( __board, __i, __j )  (__board[LDA*(__i) + (__j)])

/*****************************************************************************
 * Helper function definitions
 ****************************************************************************/

/*****************************************************************************
 * Game of life implementation
 ****************************************************************************/
char*
parallel_game_of_life_v1 (
	register char* outboard, 
	register char* inboard,
	register const int nrows,
	register const int ncols,
	register const int gens_max
) {
  	register const unsigned char NUM_THREADS = (unsigned char) sysconf(_SC_NPROCESSORS_ONLN)/2;
		
	/* HINT: in the parallel decomposition, LDA may not be equal to
       nrows! */
    register const unsigned short LDA = nrows;
    register unsigned short curgen;//, i, j;

	register unsigned char row_tiles = 2;
	register unsigned char col_tiles = NUM_THREADS/row_tiles;
	
	register unsigned short tile_rows = nrows / row_tiles;
	register unsigned short tile_cols = ncols / col_tiles;

    for (curgen = 0; curgen < gens_max; curgen++)
    {
        /* HINT: you'll be parallelizing these loop(s) by doing a
           geometric decomposition of the output */
		#pragma omp parallel num_threads(NUM_THREADS)
		{
			register unsigned char thread_id = omp_get_thread_num();

			register unsigned short row_start = thread_id / col_tiles * tile_rows;
			register unsigned short row_end = row_start + tile_rows;

			register unsigned short col_start = thread_id % col_tiles * tile_cols;
			register unsigned short col_end = col_start + tile_cols;

			register unsigned short i, j;

			register unsigned short inorth;
			register unsigned short isouth;

			register unsigned short jwest;
			register unsigned short jeast;

			register unsigned char neighbor_count;

			// register char i_nw;
			// register char i_n;
			// register char i_ne;
			register unsigned char i_w;
			register unsigned char i_c;
			register unsigned char i_e;
			register unsigned char i_sw;
			register unsigned char i_s;
			register unsigned char i_se;

			register unsigned char j_nw;
			register unsigned char j_n;
			register unsigned char j_ne;
			register unsigned char j_w;
			register unsigned char j_c;
			register unsigned char j_e;
			register unsigned char j_sw;
			register unsigned char j_s;
			register unsigned char j_se;

			i = row_start;
			j = col_start;
				
			inorth = mod (i-1, nrows);
			isouth = mod (i+1, nrows);

			jwest = mod (j-1, ncols);
			jeast = mod (j+1, ncols);

			// i_nw = j_nw = BOARD (inboard, inorth, jwest); 
			// i_n = j_n = BOARD (inboard, inorth, j);
			// i_ne = j_ne = BOARD (inboard, inorth, jeast);
			j_nw = BOARD (inboard, inorth, jwest); 
			j_n = BOARD (inboard, inorth, j);
			j_ne = BOARD (inboard, inorth, jeast);
			i_w = j_w = BOARD (inboard, i, jwest);
			i_c = j_c = BOARD (inboard, i, j);
			i_e = j_e = BOARD (inboard, i, jeast);
			i_sw = j_sw = BOARD (inboard, isouth, jwest);
			i_s = j_s = BOARD (inboard, isouth, j);
			i_se = j_se = BOARD (inboard, isouth, jeast);

			neighbor_count = j_nw + j_n + j_ne + j_w + j_e + j_sw + j_s + j_se;

			BOARD(outboard, i, j) = alivep (neighbor_count, j_c);

			j++;
			
			for (; j < col_end; j++) {
				jwest = mod (j-1, ncols);
				jeast = mod (j+1, ncols);

				j_nw = j_n;
				j_n = j_ne;
				j_ne = BOARD (inboard, inorth, jeast);
				j_w = j_c;
				j_c = j_e;
				j_e = BOARD (inboard, i, jeast);
				j_sw = j_s;
				j_s = j_se;
				j_se = BOARD (inboard, isouth, jeast);

				neighbor_count = j_nw + j_n + j_ne + j_w + j_e + j_sw + j_s + j_se;

				BOARD(outboard, i, j) = alivep (neighbor_count, j_c);
			}

			i++;

			for (; i < row_end; i++) {
				j = col_start;
				
				inorth = mod (i-1, nrows);
				isouth = mod (i+1, nrows);

				jwest = mod (j-1, ncols);
				jeast = mod (j+1, ncols);

				// i_nw = j_nw = i_w;
				// i_n = j_n = i_c;
				// i_ne = j_ne = i_e;
				j_nw = i_w;
				j_n = i_c;
				j_ne = i_e;
				i_w = j_w = i_sw;
				i_c = j_c = i_s;
				i_e = j_e = i_se;
				i_sw = j_sw = BOARD (inboard, isouth, jwest);
				i_s = j_s = BOARD (inboard, isouth, j);
				i_se = j_se = BOARD (inboard, isouth, jeast);

				neighbor_count = j_nw + j_n + j_ne + j_w + j_e + j_sw + j_s + j_se;

				BOARD(outboard, i, j) = alivep (neighbor_count, j_c);

				j++;
				
				for (; j < col_end; j++) {
					jwest = mod (j-1, ncols);
					jeast = mod (j+1, ncols);

					j_nw = j_n;
					j_n = j_ne;
					j_ne = BOARD (inboard, inorth, jeast);
					j_w = j_c;
					j_c = j_e;
					j_e = BOARD (inboard, i, jeast);
					j_sw = j_s;
					j_s = j_se;
					j_se = BOARD (inboard, isouth, jeast);

					neighbor_count = j_nw + j_n + j_ne + j_w + j_e + j_sw + j_s + j_se;

					BOARD(outboard, i, j) = alivep (neighbor_count, j_c);
				}
			}
		}
        
        SWAP_BOARDS( outboard, inboard );
    }
    /* 
     * We return the output board, so that we know which one contains
     * the final result (because we've been swapping boards around).
     * Just be careful when you free() the two boards, so that you don't
     * free the same one twice!!! 
     */
    return inboard;
}
