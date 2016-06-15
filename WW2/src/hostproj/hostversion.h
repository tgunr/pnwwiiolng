#pragma once
#define HOSTV_PRODUCT 1
#define HOSTV_VERSION 35
#define HOSTV_PATCH   3
#define HOSTV_BUILD   0
#define HOST_VERSION_NO(Prod_, Ver_, Patch_, Build_) ((Prod_<<24)|(Ver_<<16)|(Patch_<<8)|(Build_<<0))
#define HOST_VERSION (HOST_VERSION_NO(HOSTV_PRODUCT, HOSTV_VERSION, HOSTV_PATCH, HOSTV_BUILD))

