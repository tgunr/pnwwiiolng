#pragma once
#define HOSTV_PRODUCT	1		// Product number
#define HOSTV_VERSION	35		// Version number
#define HOSTV_PATCH		0		// Patch number
#define HOSTV_BUILD		1		// Build number
#define HOST_VERSION_NO(Prod_, Ver_, Patch_, Build_) ((Prod_<<24)|(Ver_<<16)|(Patch_<<8)|(Build_<<0))
#define HOST_VERSION (HOST_VERSION_NO(HOSTV_PRODUCT, HOSTV_VERSION, HOSTV_PATCH, HOSTV_BUILD))

