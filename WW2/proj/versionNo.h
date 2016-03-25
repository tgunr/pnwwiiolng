#pragma once

////////// These lines can be automated as long as you don't change the format
#define PN_WW2_PRODUCT_NO	1
#define PN_WW2_VERSION_NO	35
#define PN_WW2_PATCH_NO		0
#define PN_WW2_BUILD_NO		9
////////// End of don't-reformat section

// Because cpp(1) is painful
#define __SIFY(x) #x
#define _SIFY(x) __SIFY(x)
#define SIFY(x) _SIFY(x)

#define FILEVER				PN_WW2_PRODUCT_NO ## , ## PN_WW2_VERSION_NO ## , ## PN_WW2_PATCH_NO ## , ## PN_WW2_BUILD_NO
#define PRODUCTVER			PN_WW2_PRODUCT_NO ## , ## PN_WW2_VERSION_NO ## , ## PN_WW2_PATCH_NO
#define STRFILEVER			SIFY(PN_WW2_PRODUCT_NO) "." SIFY(PN_WW2_VERSION_NO) "." SIFY(PN_WW2_PATCH_NO) "." SIFY(PN_WW2_BUILD_NO)
#define STRPRODUCTVER		SIFY(PN_WW2_PRODUCT_NO) "." SIFY(PN_WW2_VERSION_NO) "." SIFY(PN_WW2_PATCH_NO)
#define	STRVER				STRFILEVER
#define TEULVERMIN			"1.4.0.0"

// BETA_RELEASE ENABLE_FLOATERS is now defined in project configuration settings

// *NOTE 1* The version information held here should always
// represent the source code version level.

// *NOTE 2* The resource compiler does not propagate the
// project preprocessor definitions so bracketing data with
// an #ifdef in this file is not effective for resource info.

// *NOTE 3* Leaving these #defines at the top of the file
// will allow us to use a macro for updates; if desired

// Position description 
// first - major version
// second - minor version
// third - beta/test phase
// forth - build number
