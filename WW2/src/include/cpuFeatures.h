#pragma once
//! @file cpuFeatures.h
//! @brief CPU feature encapsulation and feature set names.

//! @class CPUFeatures
//! @brief Describes a CPU and it's capabilities.
//! Based on: http://msdn.microsoft.com/en-us/library/hskdteyh(VS.80).aspx

#pragma pack(push, cpuf, 1)

struct CPUFeatures
{
	char manufacturer[16] ;				//!< Manufacturer, e.g. GenuineIntel
	char brandName[68] ;				//!< Brand name
	unsigned char stepping ;			//!< CPU stepping
	unsigned char model ;				//!< CPU model number
	unsigned char family ;				//!< CPU family number
	unsigned char processorType ;
	unsigned char extendedModel ;
	unsigned char l2Associativity ;		//!< Associativity with L2 cache
	unsigned char extendedFamily ;
	unsigned char brandIndex ;
	unsigned char cacheLineSize ;		//!< Size of cache lines
	unsigned short clflushCLS ;			//!< CLFLUSH cache line size
	unsigned short apicPhysicalID ;		//!< Physical APIC ID for CPU
	unsigned short cacheSizeK ;			//!< Size of CPU's L1 cache in K
	unsigned int featureInfo ;			//!< Binary feature mask
	unsigned int extFeatureInfo ;		//!< Additional binary feature mask
	unsigned long long speed ;			//!< CPU speed (in Hz?)
} ;

#pragma pack(pop, cpuf)
