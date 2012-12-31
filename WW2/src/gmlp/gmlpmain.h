#pragma once
//	Main game loop functions and high level event system for WW2OL
//	Copyright 1999 Playnet Inc, All rights reserved
//
//	  Originating Programmer: Jonathan Hoof

extern LOGICAL gmlpMainInit(void); // initializes the main game loop
extern LOGICAL gmlpMainUpdate(void); // updates the game
extern LOGICAL gmlpMainKill(void); // cleans up after game loop shutdown
extern UINT32 gmlpHandleFieldOfView(void);
extern void gmlpProcessInputs(void); // check for keyboard/mouse etc inputs.
