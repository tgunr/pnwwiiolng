#pragma once

////////// These lines can be automated as long as you don't change the format
#define PN_WW2_PRODUCT_NO 1
#define PN_WW2_VERSION_NO 35
#define PN_WW2_PATCH_NO   6
#define PN_WW2_BUILD_NO   2

// Used in sysreg.cpp for retrieving info from the registry.
#define PN_WW2_INSTALL_ID Auth::Apps::LIVE_APP_ID
////////// End of don't-reformat section

// Because cpp(1) is painful
#define __SIFY(x) #x
#define _SIFY(x) __SIFY(x)
#define SIFY(x) _SIFY(x)

#define STRVER              SIFY(PN_WW2_PRODUCT_NO) "." SIFY(PN_WW2_VERSION_NO) "." SIFY(PN_WW2_PATCH_NO) "." SIFY(PN_WW2_BUILD_NO)

#define PRODUCTVER			PN_WW2_PRODUCT_NO ## , ## PN_WW2_VERSION_NO ## , ## PN_WW2_PATCH_NO
#define STRPRODUCTVER		SIFY(PN_WW2_PRODUCT_NO) "." SIFY(PN_WW2_VERSION_NO) "." SIFY(PN_WW2_PATCH_NO)
