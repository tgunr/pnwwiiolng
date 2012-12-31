#pragma once

// launches the default web browser
// returns true if it successfully
// opened the browser
// if local == true, we mangle the
// path in a OS dependent way as
// needed
extern LOGICAL syslaunchurl(const char* url, LOGICAL local = 0);
