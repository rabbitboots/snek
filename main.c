/* Snek */
#define SNEK_VERSION "v1.02"

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

#include "board.h"			// Load and display boards from an ascii art editor

/* Cell IDs */
#define CELL_EMPTY 0
#define CELL_WALL 1
#define CELL_APPLE 2
#define CELL_SNAKE 100 // 100 + length count
#define CELL_SNAKE_TO_EMPTY (CELL_SNAKE-1)

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

int putCellRandom( int *brd, int kind, int under ) {
	int x, y;
	int r;
	int space = 0;
	
	/* Determine if there is space to put something */
	for( x = 0; x < VIEWPORT_W; x++ ) {
		for( y = 0; y < VIEWPORT_H; y++ ) {
			if( getCell(brd, x, y) == under ) {
				space++;
			}
		}
	}

	r = rand() % space;
	
	if( space > 0 ) {
		for( x = 0; x < VIEWPORT_W; x++ ) {
			for( y = 0; y < VIEWPORT_H; y++ ) {
				if( getCell(brd, x, y) == under ) {
					if( r == 0 ) {
						putCell(brd, kind, x, y);
						return 0;
					}
					else {
						r--;
					}
				}
			}
		}
	}
	return 1;
}


#define BOARDTYPE_EMPTY 0
#define BOARDTYPE_CROSS 1

void boardMake( int * brd, int b_n, int b_s, int b_e, int b_w, int board_type ) {
	int x, y;
	for( x = 0; x < VIEWPORT_W; x++ ) {
		for( y = 0; y < VIEWPORT_H; y++ ) {

			// Start with an empty cell
			putCell( brd, CELL_EMPTY, x, y );
		
			// Plot borders if requested
			if(	(b_n && y == 0 )
			  ||	(b_s && y == VIEWPORT_H - 1)
			  ||	(b_e && x == VIEWPORT_W - 1)
			  ||  	(b_w && x == 0) ) {
				putCell( brd, CELL_WALL, x, y );
			}
		
			// Cross pattern
			if( board_type == BOARDTYPE_CROSS ) {
				if( (x > 3 && x < VIEWPORT_W - 4 && y > VIEWPORT_H/ 2 - 2 && y < VIEWPORT_H/2 + 2) 
				||  (y > 3 && y < VIEWPORT_H - 4 && x > VIEWPORT_W/ 2 - 2 && x < VIEWPORT_W/2 + 2) ) {
					putCell( brd, CELL_WALL, x, y );
				}
			}
		}
	}
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

	int title_in = 0;
	int board_select = -1;

    if( SHOW_TITLE ) {
		Board * title_art = boardLoadFromFile( "snek.brd" );
		if( title_art ) {
			Coord title_offset = { 4, 2 };
			boardDraw( title_art, title_offset, false );
			getch();
			clear();
		}

		while( board_select == -1 ) {
	        mvprintw(1, 2, "Snek " SNEK_VERSION "\n\n  Build date: " __DATE__ ", " __TIME__ "\n\n  www.rabbitboots.com\n\n  \n  An 80x25 terminal is assumed.\n\n  Arrow keys to move your Snek.\n\n  Please choose an arena, or press 'q' to quit:\n\n  a) Square Board of Mundanity\n  b) Cross Board of Tight Quarters");
	        refresh();
	        title_in = getch();
    	    clear();
			if( title_in == 'q' ) {
				endwin();
				return 0;
			}

			if( title_in >= 'a' && title_in <= 'b' ) {
				if( title_in == 'a' ) {
					board_select = BOARDTYPE_EMPTY;
				}
				if( title_in == 'b' ) {
					board_select = BOARDTYPE_CROSS;
				}
			}
		}
    }

	/* Main loop. */
    // Loop control vars
    bool keep_going = true;
    bool first_tick = true;


	// Some vars for Snek.
	int player_input = 0;		// Player keyboard input
	int px, py;
	if( board_select == BOARDTYPE_EMPTY ) {
		px = VIEWPORT_W / 2;	// Player XY.  May need to be overridden depending on the board.
		py = VIEWPORT_H / 2;
	}
	if( board_select == BOARDTYPE_CROSS ) {
		px = 1;
		py = 1;
	}
	
	int plen = 5;				// Snek Tail Length
	int n_apples = 0;			// Apples eaten

	// Direction constants
	#define DIR_EAST  1
	#define DIR_WEST  2
	#define DIR_NORTH 3
	#define DIR_SOUTH 4
	int pdir = DIR_EAST;

	// Board init
	int * board;
	board = malloc( sizeof(int) * (VIEWPORT_W * VIEWPORT_H) );
	if( !board ) {
		errQuit( "FATAL: main.c: malloc() failed on board * pointer." );
	}
	
	boardMake( board, 1, 1, 1, 1, board_select );
	putCellRandom( board, CELL_APPLE, CELL_EMPTY );

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

		if( under == CELL_WALL || under >= CELL_SNAKE ) {
			colorSet( COLOR_WHITE, COLOR_RED, 1, 1 );
			mvprintw(0, 0, " * S N E K   O V E R * " );
			colorSet( COLOR_WHITE, COLOR_BLACK, 1, 0 );
			mvprintw(3, 0, "'q' to quit" );
			halfdelay(0);
			continue;
		}

		// Snek meets apple
		if( under == CELL_APPLE ) {
			plen++;
			n_apples++;
			putCellRandom( board, CELL_APPLE, CELL_EMPTY );
		}
		
		// Clip Snek tail, but only if it hasn't just eaten an apple.
		else {
			int x, y;
			for( x = 0; x < VIEWPORT_W; x++ ) {
				for( y = 0; y < VIEWPORT_H; y++ ) {
					int this = getCell( board, x, y );
					if( this >= CELL_SNAKE ) {
						putCell( board, this - 1, x, y );
					}
					if( this == CELL_SNAKE_TO_EMPTY) {
						putCell( board, CELL_EMPTY, x, y );
					}
				}
			}
		}

		// Write Snek body to buffer.  Overwrites apples
		if( plen > 0 ) {
			putCell( board, CELL_SNAKE + plen, px, py );
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
				if( this == CELL_WALL ) {
					colorSet( COLOR_BLUE, COLOR_BLACK, 1, 0 );
					glyph = '#';
				}

				// b) Apple
				else if( this == CELL_APPLE ) {
					colorSet( COLOR_RED, COLOR_BLACK, 1, 0 );
					glyph = '@';
				}

				// c) Snek body
				else if( this >= CELL_SNAKE ) {
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

    /* Deallocate pointers */

	free( board );

    /* Close error handler */
    errLog("    **  Shutting down.  **\n");
    errorHandlerShutdown( &error_handler );

    /* Close Curses */
    endwin();

    return 0;
}
