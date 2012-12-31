//
//  $Id: main.m 2136 2009-11-16 03:27:03Z ahwulf@CRS $
//  NewPlayNet
//
//  Created by crs on 12/8/04.
//  Copyright __MyCompanyName__ 2004. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "sysmain.h"

int main(int argc, char *argv[])
{
  sysmainCommandLine(argc, argv);
  int n = NSApplicationMain(argc,  (const char **) argv);
  
  return n;
}
