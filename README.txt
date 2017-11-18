A simple, slapped-together Snake game that I uploaded mainly to help get comfortable with Git / Github.

Can you eat all the apples? Do snakes even eat apples? No? Too late!

Requirements
	NCurses

Compiling
	Linux: gcc *.c -o snek -lncurses
	Windows: * TODO *

Controls
	Arrow keys to direct the Snake.
	'q' to end the game. Ctrl+c should also kill it dead.

Known Issues
* Noticed artifacting under Linux / GNOME Terminal -- parts of the snake body intermittently being drawn at the wrong coordinates.
  After chasing ghosts for a while, simply calling clear() prior to drawing seems to resolve it. I recall clear()
  causing flickering under Windows 10 / PDCurses, but I'll leave this as is for now.

Improvement Ideas:
* Omm Nom Nom text randomly splashing the UI when an apple is eaten
* Show the arena before action starts, and allow the player to choose their first move/direction instead of defaulting to moving east
* Multiple screens / arenas
* Make the code less awful
  
  
Author: Rabbitboots
www.rabbitboots.com
admin@rabbitboots.com

