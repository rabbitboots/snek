/* Snek */
#define SNEK_VERSION "v1.01"

// C Standard Library
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

// Curses
#include "curses.h"

// Game source -- adapted from Fog / Floors N' Doors.

#include "error_handler.h"  // Error handling / debug logging and bad terminations.
                            // Use errLog() to log messages, and errQuit() to abandon ship.
#include "curses_wrapper.h" // Curses helper functions

// Graphics
#include "draw.h"           // Some additional Curses helper functions.

/* Bounds checking */
int inBounds( int x, int y ) {
	if( x >= 0 && x <= VIEWPORT_W - 1 && y >= 0 && y <= VIEWPORT_H - 1 ) {
		return 1;
	}
	else {
		return 0;
	}
}

int getCell( int * brd, int x, int y ) {
	int kind;
	if( inBounds( x, y ) ) {
		kind = brd[ x * VIEWPORT_H + y ];
	}
	else {
		kind = -1;
	}
	return kind;	
}

void putCell( int * brd, int kind, int x, int y ) {
	if( inBounds( x, y ) ) {
		brd[x * VIEWPORT_H + y] = kind;
	}
	return;
}

void putCellRandom( int *brd, int kind, int under ) {
	int x, y;
	int space = 0;
	
	for( x = 0; x < VIEWPORT_W; x++ ) {
		for( y = 0; y < VIEWPORT_H; y++ ) {
			if( getCell(brd, x, y) == under ) {
				space = 1;
			}
		}
	}
	while( space ) {
		x = 1 + rand() % VIEWPORT_W;
		y = 1 + rand() % VIEWPORT_H;

		if( getCell( brd, x, y ) == under ) {
			putCell( brd, kind, x, y );
			break;
		}
	}
	return;
}


int main( int argc, char *argv[] ) {

	// Print version
	if( argc > 1 && strncmp( argv[1], "--version", 10 ) == 0 ) {
		printf( "Snek " SNEK_VERSION "\n");
		exit(1);
	}

    /*  -- System Init.  */

    VIEWPORT_W = 23;
    VIEWPORT_H = 23;
    VIEWPORT_X = 0;
    VIEWPORT_Y = 0;

    // Debug logging
    errorHandlerInit( &error_handler, 0 );
    errLog( "    ** Logging new session **");

    // Seed randomizer.  Call only once.  Consider getting a more robust randomizer in the future.
	// This randomizes based on the current second -- it's easy to start multiple instances 
	// with the same seed randomizer.
    srand(time(NULL));

    // Set a fixed seed for debugging purposes.
    //srand(4);

    // Curses startup wrapper.
    if( init_curses() != 0) {
        errLog( "Failed to initialize Curses. Exiting." );
        exit(1);
    }

    /* Title Screen */

    #define SHOW_TITLE 1

    if( SHOW_TITLE ) {
        mvprintw(1, 2, "Oh No It's\n\n  Snek\n\n  " SNEK_VERSION "\n\n  Build date: " __DATE__ ", " __TIME__ "\n\n  www.rabbitboots.com\n\n  \n  An 80x25 terminal is assumed.\n\n  Press any key to start!");
        refresh();
        getch();
        clear();
    }

	/* Main loop. */
    // Loop control vars
    bool keep_going = true;
    bool first_tick = true;


	// Some vars for Snek.
	int player_input = 0;		// Player keyboard input
	int px = VIEWPORT_W / 2;	// Player XY
	int py = VIEWPORT_H / 2;
	int plen = 5;				// Snek Tail Length
	int n_apples = 0;			// Apples eaten

	// Direction constants
	#define DIR_EAST 1
	#define DIR_WEST 2
	#define DIR_NORTH 3
	#define DIR_SOUTH 4
	int pdir = DIR_EAST;

	// Cell IDs
	int nothing = 0;
	// Note: any cell larger than 'nothng' and smaller than 'wall' is considered part of the Snek's body.
	int wall = VIEWPORT_W * VIEWPORT_H;
	int apple = wall + 1;
	

	// Board init
	int * board;
	board = malloc( sizeof(int) * (VIEWPORT_W * VIEWPORT_H) );
	if( !board ) {
		errQuit( "FATAL: main.c: malloc() failed on board * pointer." );
	}
	{
		int x, y;
		for( x = 0; x < VIEWPORT_W; x++ ) {
			for( y = 0; y < VIEWPORT_H; y++ ) {
				if( x == 0 || x == VIEWPORT_W - 1 || y == 0 || y == VIEWPORT_H - 1 ) {
					putCell( board, wall, x, y );	
				}
				else {
					putCell( board, nothing, x, y );
				}
			}
		}
	}
	putCellRandom( board, apple, nothing );

	// Main loop
    while(keep_going) {
        // Makes Curses "time out" if no key has been pressed in N tenths of a second.
        halfdelay(2);

        // Do some things differently on the very first tick.
        if(!first_tick) {
	        // -- Input processing
			player_input = getch();

			// Note: disallow player from turning 180 degrees (north to south, east to west)
			switch (player_input) {

			case KEY_LEFT:
				if( pdir != DIR_EAST ) {
					pdir = DIR_WEST;
					}
				break;
			case KEY_RIGHT:
				if( pdir != DIR_WEST ) {
					pdir = DIR_EAST;
				}
				break;
			case KEY_UP:
				if( pdir != DIR_SOUTH ) {
					pdir = DIR_NORTH;
				}
				break;
			case KEY_DOWN:
				if( pdir != DIR_NORTH ) {
					pdir = DIR_SOUTH;
				}
				break;
			default:
				break;
			}
		}

        // Quitting
        if( player_input == 'q' ) {
            keep_going = false;
			continue;
        }

		// Game logic
		
		// Game Over Condition
		int under = getCell( board, px, py );

		if( under > nothing && under != apple ) {
			colorSet( COLOR_WHITE, COLOR_RED, 1, 1 );
			mvprintw(0, 0, " * S N E K   O V E R * " );
			colorSet( COLOR_WHITE, COLOR_BLACK, 1, 0 );
			mvprintw(3, 0, "'q' to quit" );
			halfdelay(0);
			continue;
		}

		// Snek meets apple
		if( under == apple ) {
			plen++;
			n_apples++;
			putCellRandom( board, apple, nothing );
		}
		
		// Clip Snek tail, but only if it hasn't just eaten an apple.
		else {
			int x, y;
			for( x = 0; x < VIEWPORT_W; x++ ) {
				for( y = 0; y < VIEWPORT_H; y++ ) {
					int this = getCell( board, x, y );
					if( this > nothing && this < wall ) {
						putCell( board, this - 1, x, y );
					}
				}
			}
		}

		// Write Snek body to buffer.  Overwrites apples
		if( plen > 0 ) {
			putCell( board, plen, px, py );
		}

		// Move Snek head
		if( pdir == DIR_EAST ) {
			px++;
		}
		if( pdir == DIR_WEST ) {
			px--;
		}
		if( pdir == DIR_SOUTH ) {
			py++;
		}
		if( pdir == DIR_NORTH ) {
			py--;
		}
		/*  Clear will wipe the Curses window, but it can cause noticeable tearing artifacts 
		    in the Windows 10 console.  It may look OK on other platforms or terminals, though.
            TODO: Drop in a config option to use clear() every on every loop tick if desired.
			Update: Now I'm experiencing artifacts when clear() is not used. Leaving it on for now.	*/
        clear();


		/* Draw background */

		{
		int x, y;
		for( x = 0; x < VIEWPORT_W; x++ ) {
			for( y = 0; y < VIEWPORT_H; y++ ) {
				int this = getCell( board, x, y );
				int glyph = 'X';
				// Cell is...
				// a) Wall
				if( this == wall ) {
					colorSet( COLOR_BLUE, COLOR_BLACK, 1, 0 );
					glyph = '#';
				}

				// b) Apple
				else if( this == apple ) {
					colorSet( COLOR_RED, COLOR_BLACK, 1, 0 );
					glyph = '@';
				}

				// c) Snek body
				else if( (this > nothing) && (this < wall) ) {
					colorSet( COLOR_GREEN, COLOR_BLACK, 0, 0 );
					if( this % 2 == 1 ) { 
						glyph = 'S';
					}
					else if( this % 2 == 0 ) {
						glyph = 's';
					}
				}

				// d) empty
				else {
					colorSet( COLOR_BLUE, COLOR_BLACK, 0, 0 );
					glyph = ' ';
				}
				mvaddch(y, x, glyph);
			}
		}
		}
		/* Draw the Snek head */

		colorSet( COLOR_GREEN, COLOR_BLACK, 1, 0 );
		mvaddch( py, px, 'S' );

		/* Draw UI elements */
		colorSet( COLOR_WHITE, COLOR_BLACK, 1, 0 );
		mvprintw( 4, 42, "                    " );
		mvprintw( 4, 42, "Apples: %d", n_apples );

        // Curses display update.
        refresh();

        first_tick = 0;

		// -- End of main loop.
    }


    /* -- Shutting down */

    /* Close allocations and pointers. */

	free( board );

    /* Close error handler */
    errLog("    **  Shutting down.  **\n");
    errorHandlerShutdown( &error_handler );

    /* Close Curses */
    endwin();

    return 0;
}
