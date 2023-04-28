/*****************************************************************************
 * lifepar_v2.c
 * Second iteration of parallelized and optimized implementation
 ****************************************************************************/
#include "life.h"
#include "util.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <unistd.h>

/*****************************************************************************
 * Helper function definitions
 ****************************************************************************/
/**
 * Swapping the two boards only involves swapping pointers, not
 * copying values.
 */
#define SWAP_BOARDS( b1, b2 )  do { \
  char* temp = b1; \
  b1 = b2; \
  b2 = temp; \
} while(0)

#define BOARD( __board, __i, __j)  ( __board[nrows*(__i) + (__j)] )

#define STATE(cell)  ( (cell >> 4) & 0x01 )
#define SPAWN(__board, __i, __j)  ( BOARD( __board, __i, __j) |= (1 << 4) )
#define KILL(__board, __i, __j)  ( BOARD( __board, __i, __j) &= ~(1 << 4) )
#define SHOULD_SPAWN(cell)  ( cell == (char) 0x03 )
#define SHOULD_KILL(cell)  ( (cell <= (char) 0x11) || (cell >= (char) 0x14) )
#define INCREMENT(__board, __i, __j)  ( BOARD( __board, __i, __j) ++ )
#define DECREMENT(__board, __i, __j)  ( BOARD( __board, __i, __j) -- )

/*****************************************************************************
 * Game of life implementation
 ****************************************************************************/
char*
parallel_game_of_life_v2 (register char* outboard, 
	      register char* inboard,
	      register const int nrows,
	      register const int ncols,
	      register const int gens_max)
{
	register const unsigned char NUM_THREADS = (unsigned char) sysconf(_SC_NPROCESSORS_ONLN)/2;
	
	register char *new_inboard = calloc(nrows * ncols, sizeof(char));
	register char *new_outboard = calloc(nrows * ncols, sizeof(char));

	omp_lock_t cell_locks[nrows * ncols];

	register unsigned short i, j;

	register unsigned short inorth;
	register unsigned short isouth;

	register unsigned short jwest;
	register unsigned short jeast;

	for (i = 0; i < nrows; i++) {
		for (j = 0; j < ncols; j++) {
			omp_init_lock(&BOARD(cell_locks, i, j));
			
			if (BOARD(inboard, i, j)) {
				inorth = mod (i-1, nrows);
				isouth = mod (i+1, nrows);
				jwest = mod (j-1, ncols);
				jeast = mod (j+1, ncols);

				INCREMENT (new_inboard, inorth, jwest);
                INCREMENT (new_inboard, inorth, j);
                INCREMENT (new_inboard, inorth, jeast);
                INCREMENT (new_inboard, i, jwest);
                INCREMENT (new_inboard, i, jeast);
                INCREMENT (new_inboard, isouth, jwest);
                INCREMENT (new_inboard, isouth, j);
                INCREMENT (new_inboard, isouth, jeast);

				SPAWN(new_inboard, i, j);
			}
		}
	}

	register unsigned char row_tiles = 8;
	register unsigned char col_tiles = NUM_THREADS/row_tiles;
	
 	register unsigned short tile_rows = nrows / row_tiles;
 	register unsigned short tile_cols = ncols / col_tiles;

	register unsigned short curgen;

	for (curgen = 0; curgen < gens_max; curgen++) {
		memcpy(new_outboard, new_inboard, nrows * ncols * sizeof(char));

		#pragma omp parallel num_threads(NUM_THREADS) proc_bind(close) private(i, j, inorth, isouth, jwest, jeast)
		{
			register unsigned char thread_id = omp_get_thread_num();

			register unsigned short row_start = thread_id / col_tiles * tile_rows;
			register unsigned short row_end = row_start + tile_rows;

			register unsigned short col_start = thread_id % col_tiles * tile_cols;
			register unsigned short col_end = col_start + tile_cols;

			register char cell;

			for (j = col_start; j < col_start + 2; j++) {
				jwest = mod (j-1, ncols);
				jeast = mod (j+1, ncols);
				for (i = row_start; i < row_end; i++) {
					cell = BOARD(new_inboard, i, j);

					if (STATE(cell)) {
						if (SHOULD_KILL(cell)) {
							inorth = mod (i-1, nrows);
							isouth = mod (i+1, nrows);
							
							omp_set_lock(&BOARD(cell_locks, inorth, jwest));
							DECREMENT (new_outboard, inorth, jwest);
							omp_unset_lock(&BOARD(cell_locks, inorth, jwest));

							omp_set_lock(&BOARD(cell_locks, inorth, j));
							DECREMENT (new_outboard, inorth, j);
							omp_unset_lock(&BOARD(cell_locks, inorth, j));

							omp_set_lock(&BOARD(cell_locks, inorth, jeast));
							DECREMENT (new_outboard, inorth, jeast);
							omp_unset_lock(&BOARD(cell_locks, inorth, jeast));

							omp_set_lock(&BOARD(cell_locks, i, jwest));
							DECREMENT (new_outboard, i, jwest);
							omp_unset_lock(&BOARD(cell_locks, i, jwest));

							omp_set_lock(&BOARD(cell_locks, i, jeast));
							DECREMENT (new_outboard, i, jeast);
							omp_unset_lock(&BOARD(cell_locks, i, jeast));

							omp_set_lock(&BOARD(cell_locks, isouth, jwest));
							DECREMENT (new_outboard, isouth, jwest);
							omp_unset_lock(&BOARD(cell_locks, isouth, jwest));

							omp_set_lock(&BOARD(cell_locks, isouth, j));
							DECREMENT (new_outboard, isouth, j);
							omp_unset_lock(&BOARD(cell_locks, isouth, j));

							omp_set_lock(&BOARD(cell_locks, isouth, jeast));
							DECREMENT (new_outboard, isouth, jeast);
							omp_unset_lock(&BOARD(cell_locks, isouth, jeast));

							omp_set_lock(&BOARD(cell_locks, i, j));
							KILL(new_outboard, i, j);
							omp_unset_lock(&BOARD(cell_locks, i, j));
						}
					} else {
						if (SHOULD_SPAWN(cell)) {
							inorth = mod (i-1, nrows);
							isouth = mod (i+1, nrows);
							
							omp_set_lock(&BOARD(cell_locks, inorth, jwest));
							INCREMENT (new_outboard, inorth, jwest);
							omp_unset_lock(&BOARD(cell_locks, inorth, jwest));

							omp_set_lock(&BOARD(cell_locks, inorth, j));
							INCREMENT (new_outboard, inorth, j);
							omp_unset_lock(&BOARD(cell_locks, inorth, j));

							omp_set_lock(&BOARD(cell_locks, inorth, jeast));
							INCREMENT (new_outboard, inorth, jeast);
							omp_unset_lock(&BOARD(cell_locks, inorth, jeast));

							omp_set_lock(&BOARD(cell_locks, i, jwest));
							INCREMENT (new_outboard, i, jwest);
							omp_unset_lock(&BOARD(cell_locks, i, jwest));

							omp_set_lock(&BOARD(cell_locks, i, jeast));
							INCREMENT (new_outboard, i, jeast);
							omp_unset_lock(&BOARD(cell_locks, i, jeast));

							omp_set_lock(&BOARD(cell_locks, isouth, jwest));
							INCREMENT (new_outboard, isouth, jwest);
							omp_unset_lock(&BOARD(cell_locks, isouth, jwest));

							omp_set_lock(&BOARD(cell_locks, isouth, j));
							INCREMENT (new_outboard, isouth, j);
							omp_unset_lock(&BOARD(cell_locks, isouth, j));

							omp_set_lock(&BOARD(cell_locks, isouth, jeast));
							INCREMENT (new_outboard, isouth, jeast);
							omp_unset_lock(&BOARD(cell_locks, isouth, jeast));

							omp_set_lock(&BOARD(cell_locks, i, j));
							SPAWN(new_outboard, i, j);
							omp_unset_lock(&BOARD(cell_locks, i, j));
						}
					}
				}
			}

			for (j = col_start + 2; j < col_end - 2; j++) {
				jwest = mod (j-1, ncols);
				jeast = mod (j+1, ncols);
				for (i = row_start; i < row_start + 2; i++) {
					cell = BOARD(new_inboard, i, j);

					if (STATE(cell)) {
						if (SHOULD_KILL(cell)) {
							inorth = mod (i-1, nrows);
							isouth = mod (i+1, nrows);
							
							omp_set_lock(&BOARD(cell_locks, inorth, jwest));
							DECREMENT (new_outboard, inorth, jwest);
							omp_unset_lock(&BOARD(cell_locks, inorth, jwest));

							omp_set_lock(&BOARD(cell_locks, inorth, j));
							DECREMENT (new_outboard, inorth, j);
							omp_unset_lock(&BOARD(cell_locks, inorth, j));

							omp_set_lock(&BOARD(cell_locks, inorth, jeast));
							DECREMENT (new_outboard, inorth, jeast);
							omp_unset_lock(&BOARD(cell_locks, inorth, jeast));

							omp_set_lock(&BOARD(cell_locks, i, jwest));
							DECREMENT (new_outboard, i, jwest);
							omp_unset_lock(&BOARD(cell_locks, i, jwest));

							omp_set_lock(&BOARD(cell_locks, i, jeast));
							DECREMENT (new_outboard, i, jeast);
							omp_unset_lock(&BOARD(cell_locks, i, jeast));

							omp_set_lock(&BOARD(cell_locks, isouth, jwest));
							DECREMENT (new_outboard, isouth, jwest);
							omp_unset_lock(&BOARD(cell_locks, isouth, jwest));

							omp_set_lock(&BOARD(cell_locks, isouth, j));
							DECREMENT (new_outboard, isouth, j);
							omp_unset_lock(&BOARD(cell_locks, isouth, j));

							omp_set_lock(&BOARD(cell_locks, isouth, jeast));
							DECREMENT (new_outboard, isouth, jeast);
							omp_unset_lock(&BOARD(cell_locks, isouth, jeast));

							omp_set_lock(&BOARD(cell_locks, i, j));
							KILL(new_outboard, i, j);
							omp_unset_lock(&BOARD(cell_locks, i, j));
						}
					} else {
						if (SHOULD_SPAWN(cell)) {
							inorth = mod (i-1, nrows);
							isouth = mod (i+1, nrows);
							
							omp_set_lock(&BOARD(cell_locks, inorth, jwest));
							INCREMENT (new_outboard, inorth, jwest);
							omp_unset_lock(&BOARD(cell_locks, inorth, jwest));

							omp_set_lock(&BOARD(cell_locks, inorth, j));
							INCREMENT (new_outboard, inorth, j);
							omp_unset_lock(&BOARD(cell_locks, inorth, j));

							omp_set_lock(&BOARD(cell_locks, inorth, jeast));
							INCREMENT (new_outboard, inorth, jeast);
							omp_unset_lock(&BOARD(cell_locks, inorth, jeast));

							omp_set_lock(&BOARD(cell_locks, i, jwest));
							INCREMENT (new_outboard, i, jwest);
							omp_unset_lock(&BOARD(cell_locks, i, jwest));

							omp_set_lock(&BOARD(cell_locks, i, jeast));
							INCREMENT (new_outboard, i, jeast);
							omp_unset_lock(&BOARD(cell_locks, i, jeast));

							omp_set_lock(&BOARD(cell_locks, isouth, jwest));
							INCREMENT (new_outboard, isouth, jwest);
							omp_unset_lock(&BOARD(cell_locks, isouth, jwest));

							omp_set_lock(&BOARD(cell_locks, isouth, j));
							INCREMENT (new_outboard, isouth, j);
							omp_unset_lock(&BOARD(cell_locks, isouth, j));

							omp_set_lock(&BOARD(cell_locks, isouth, jeast));
							INCREMENT (new_outboard, isouth, jeast);
							omp_unset_lock(&BOARD(cell_locks, isouth, jeast));

							omp_set_lock(&BOARD(cell_locks, i, j));
							SPAWN(new_outboard, i, j);
							omp_unset_lock(&BOARD(cell_locks, i, j));
						}
					}
				}
				for (i = row_end - 2; i < row_end; i++) {
					cell = BOARD(new_inboard, i, j);
					
					if (STATE(cell)) {
						if (SHOULD_KILL(cell)) {
							inorth = mod (i-1, nrows);
							isouth = mod (i+1, nrows);
							
							omp_set_lock(&BOARD(cell_locks, inorth, jwest));
							DECREMENT (new_outboard, inorth, jwest);
							omp_unset_lock(&BOARD(cell_locks, inorth, jwest));

							omp_set_lock(&BOARD(cell_locks, inorth, j));
							DECREMENT (new_outboard, inorth, j);
							omp_unset_lock(&BOARD(cell_locks, inorth, j));

							omp_set_lock(&BOARD(cell_locks, inorth, jeast));
							DECREMENT (new_outboard, inorth, jeast);
							omp_unset_lock(&BOARD(cell_locks, inorth, jeast));

							omp_set_lock(&BOARD(cell_locks, i, jwest));
							DECREMENT (new_outboard, i, jwest);
							omp_unset_lock(&BOARD(cell_locks, i, jwest));

							omp_set_lock(&BOARD(cell_locks, i, jeast));
							DECREMENT (new_outboard, i, jeast);
							omp_unset_lock(&BOARD(cell_locks, i, jeast));

							omp_set_lock(&BOARD(cell_locks, isouth, jwest));
							DECREMENT (new_outboard, isouth, jwest);
							omp_unset_lock(&BOARD(cell_locks, isouth, jwest));

							omp_set_lock(&BOARD(cell_locks, isouth, j));
							DECREMENT (new_outboard, isouth, j);
							omp_unset_lock(&BOARD(cell_locks, isouth, j));

							omp_set_lock(&BOARD(cell_locks, isouth, jeast));
							DECREMENT (new_outboard, isouth, jeast);
							omp_unset_lock(&BOARD(cell_locks, isouth, jeast));

							omp_set_lock(&BOARD(cell_locks, i, j));
							KILL(new_outboard, i, j);
							omp_unset_lock(&BOARD(cell_locks, i, j));
						}
					} else {
						if (SHOULD_SPAWN(cell)) {
							inorth = mod (i-1, nrows);
							isouth = mod (i+1, nrows);
							
							omp_set_lock(&BOARD(cell_locks, inorth, jwest));
							INCREMENT (new_outboard, inorth, jwest);
							omp_unset_lock(&BOARD(cell_locks, inorth, jwest));

							omp_set_lock(&BOARD(cell_locks, inorth, j));
							INCREMENT (new_outboard, inorth, j);
							omp_unset_lock(&BOARD(cell_locks, inorth, j));

							omp_set_lock(&BOARD(cell_locks, inorth, jeast));
							INCREMENT (new_outboard, inorth, jeast);
							omp_unset_lock(&BOARD(cell_locks, inorth, jeast));

							omp_set_lock(&BOARD(cell_locks, i, jwest));
							INCREMENT (new_outboard, i, jwest);
							omp_unset_lock(&BOARD(cell_locks, i, jwest));

							omp_set_lock(&BOARD(cell_locks, i, jeast));
							INCREMENT (new_outboard, i, jeast);
							omp_unset_lock(&BOARD(cell_locks, i, jeast));

							omp_set_lock(&BOARD(cell_locks, isouth, jwest));
							INCREMENT (new_outboard, isouth, jwest);
							omp_unset_lock(&BOARD(cell_locks, isouth, jwest));

							omp_set_lock(&BOARD(cell_locks, isouth, j));
							INCREMENT (new_outboard, isouth, j);
							omp_unset_lock(&BOARD(cell_locks, isouth, j));

							omp_set_lock(&BOARD(cell_locks, isouth, jeast));
							INCREMENT (new_outboard, isouth, jeast);
							omp_unset_lock(&BOARD(cell_locks, isouth, jeast));

							omp_set_lock(&BOARD(cell_locks, i, j));
							SPAWN(new_outboard, i, j);
							omp_unset_lock(&BOARD(cell_locks, i, j));
						}
					}
				}
			}

			for (j = col_end - 2; j < col_end; j++) {
				jwest = mod (j-1, ncols);
				jeast = mod (j+1, ncols);
				for (i = row_start; i < row_end; i++) {
					cell = BOARD(new_inboard, i, j);
					
					if (STATE(cell)) {
						if (SHOULD_KILL(cell)) {
							inorth = mod (i-1, nrows);
							isouth = mod (i+1, nrows);
							
							omp_set_lock(&BOARD(cell_locks, inorth, jwest));
							DECREMENT (new_outboard, inorth, jwest);
							omp_unset_lock(&BOARD(cell_locks, inorth, jwest));

							omp_set_lock(&BOARD(cell_locks, inorth, j));
							DECREMENT (new_outboard, inorth, j);
							omp_unset_lock(&BOARD(cell_locks, inorth, j));

							omp_set_lock(&BOARD(cell_locks, inorth, jeast));
							DECREMENT (new_outboard, inorth, jeast);
							omp_unset_lock(&BOARD(cell_locks, inorth, jeast));

							omp_set_lock(&BOARD(cell_locks, i, jwest));
							DECREMENT (new_outboard, i, jwest);
							omp_unset_lock(&BOARD(cell_locks, i, jwest));

							omp_set_lock(&BOARD(cell_locks, i, jeast));
							DECREMENT (new_outboard, i, jeast);
							omp_unset_lock(&BOARD(cell_locks, i, jeast));

							omp_set_lock(&BOARD(cell_locks, isouth, jwest));
							DECREMENT (new_outboard, isouth, jwest);
							omp_unset_lock(&BOARD(cell_locks, isouth, jwest));

							omp_set_lock(&BOARD(cell_locks, isouth, j));
							DECREMENT (new_outboard, isouth, j);
							omp_unset_lock(&BOARD(cell_locks, isouth, j));

							omp_set_lock(&BOARD(cell_locks, isouth, jeast));
							DECREMENT (new_outboard, isouth, jeast);
							omp_unset_lock(&BOARD(cell_locks, isouth, jeast));

							omp_set_lock(&BOARD(cell_locks, i, j));
							KILL(new_outboard, i, j);
							omp_unset_lock(&BOARD(cell_locks, i, j));
						}
					} else {
						if (SHOULD_SPAWN(cell)) {
							inorth = mod (i-1, nrows);
							isouth = mod (i+1, nrows);
							
							omp_set_lock(&BOARD(cell_locks, inorth, jwest));
							INCREMENT (new_outboard, inorth, jwest);
							omp_unset_lock(&BOARD(cell_locks, inorth, jwest));

							omp_set_lock(&BOARD(cell_locks, inorth, j));
							INCREMENT (new_outboard, inorth, j);
							omp_unset_lock(&BOARD(cell_locks, inorth, j));

							omp_set_lock(&BOARD(cell_locks, inorth, jeast));
							INCREMENT (new_outboard, inorth, jeast);
							omp_unset_lock(&BOARD(cell_locks, inorth, jeast));

							omp_set_lock(&BOARD(cell_locks, i, jwest));
							INCREMENT (new_outboard, i, jwest);
							omp_unset_lock(&BOARD(cell_locks, i, jwest));

							omp_set_lock(&BOARD(cell_locks, i, jeast));
							INCREMENT (new_outboard, i, jeast);
							omp_unset_lock(&BOARD(cell_locks, i, jeast));

							omp_set_lock(&BOARD(cell_locks, isouth, jwest));
							INCREMENT (new_outboard, isouth, jwest);
							omp_unset_lock(&BOARD(cell_locks, isouth, jwest));

							omp_set_lock(&BOARD(cell_locks, isouth, j));
							INCREMENT (new_outboard, isouth, j);
							omp_unset_lock(&BOARD(cell_locks, isouth, j));

							omp_set_lock(&BOARD(cell_locks, isouth, jeast));
							INCREMENT (new_outboard, isouth, jeast);
							omp_unset_lock(&BOARD(cell_locks, isouth, jeast));

							omp_set_lock(&BOARD(cell_locks, i, j));
							SPAWN(new_outboard, i, j);
							omp_unset_lock(&BOARD(cell_locks, i, j));
						}
					}
				}
			}

			for (i = row_start + 2; i < row_end - 2; i++) {
				inorth = mod (i-1, nrows);
				isouth = mod (i+1, nrows);
				
				for (j = col_start + 2; j < col_end - 2; j++) {
					cell = BOARD(new_inboard, i, j);
					
					if (STATE(cell)) {
						if (SHOULD_KILL(cell)) {
							jwest = mod (j-1, ncols);
							jeast = mod (j+1, ncols);
							
							DECREMENT (new_outboard, inorth, jwest);

							DECREMENT (new_outboard, inorth, j);

							DECREMENT (new_outboard, inorth, jeast);

							DECREMENT (new_outboard, i, jwest);

							DECREMENT (new_outboard, i, jeast);

							DECREMENT (new_outboard, isouth, jwest);

							DECREMENT (new_outboard, isouth, j);

							DECREMENT (new_outboard, isouth, jeast);

							KILL(new_outboard, i, j);
						}
					} else {
						if (SHOULD_SPAWN(cell)) {
							jwest = mod (j-1, ncols);
							jeast = mod (j+1, ncols);
							
							INCREMENT (new_outboard, inorth, jwest);

							INCREMENT (new_outboard, inorth, j);

							INCREMENT (new_outboard, inorth, jeast);

							INCREMENT (new_outboard, i, jwest);

							INCREMENT (new_outboard, i, jeast);

							INCREMENT (new_outboard, isouth, jwest);

							INCREMENT (new_outboard, isouth, j);

							INCREMENT (new_outboard, isouth, jeast);

							SPAWN(new_outboard, i, j);
						}
					}
				}
			}
		}

		SWAP_BOARDS( new_outboard, new_inboard );
	}

	for (i = 0; i < nrows; i++) {
		for (j = 0; j < ncols; j++) {
			BOARD(inboard, i, j) = STATE(BOARD(new_inboard, i, j));
		}
	}

	return inboard;
}
