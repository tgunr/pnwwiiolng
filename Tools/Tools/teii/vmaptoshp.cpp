#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "ogrsf_frmts.h"
#include "cpl_conv.h"
#include "cpl_string.h"

static void ReportOnLayer( OGRLayer * poLayer, const char *pszWHERE, 
                           OGRGeometry *poSpatialFilter );

int main( int nArgc, char ** papszArgv )

{

    OGRDataSource       *poDS;
    OGRSFDriver         *poDriver;

    OGRRegisterAll();
//	poDS = OGRSFDriverRegistrar::Open( "gltp:/vrf/Users/rickb/Development/terrain2/vector/v1099/v1099/noamer/lib_099" , 0, &poDriver );
//    poDS = OGRSFDriverRegistrar::Open( "gltp:/vrf//Users/rickb/Development/terrain2/vector/v0eur/vmaplv0/eurnasia" , 0, &poDriver );
// poDS = OGRSFDriverRegistrar::Open( "gltp:/vrf/Volumes/RICKS-HD/terrain2/vector/v0soa/vmaplv0/soamafr" , 0, &poDriver );
 poDS = OGRSFDriverRegistrar::Open( "gltp:/vrf/Volumes/RICKS-HD/terrain2/vector/v0sas/vmaplv0/sasaus" , 0, &poDriver );

	for( int iLayer = 0; iLayer < poDS->GetLayerCount(); iLayer++ )
	{
		OGRLayer        *poLayer = poDS->GetLayer(iLayer);
		ReportOnLayer( poLayer, NULL, NULL);
	}

    return 0;
}

static void ReportOnLayer( OGRLayer * poLayer, const char *pszWHERE, 
                           OGRGeometry *poSpatialFilter )

{
    OGRFeatureDefn      *poDefn = poLayer->GetLayerDefn();

 	char* feature = strdup(poDefn->GetName());
	feature = strtok(feature, "@");
	char* name = strtok(NULL, "(");
	char call[256];
	snprintf(call, 255, "ogr2ogr '%s_%s.shp' gltp:/vrf/Volumes/RICKS-HD/terrain2/vector/v0sas/vmaplv0/sasaus '%s'", name, feature, poDefn->GetName());
	printf("%s\n", call);
	system(call);
}