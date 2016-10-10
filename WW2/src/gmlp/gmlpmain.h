#pragma once
//	Main game loop functions and high level event system for WW2OL
//	Copyright 1999 Playnet Inc, All rights reserved
//
//	  Originating Programmer: Jonathan Hoof

LOGICAL gmlpMainInit(); // initializes the main game loop
LOGICAL gmlpMainUpdate(); // updates the game
LOGICAL gmlpMainKill(); // cleans up after game loop shutdown
UINT32 gmlpHandleFieldOfView();
void gmlpProcessInputs(); // check for keyboard/mouse etc inputs.
