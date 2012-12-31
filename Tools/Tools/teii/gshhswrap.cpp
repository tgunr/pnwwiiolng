#include <math.h>
#include <stdio.h>
#include <memory>
#include <list>
#include <algorithm>
#include <vector>

#include "gdal_priv.h"
#include "ogrsf_frmts.h"

#define OGE_USE_SEAWATER 15

/************************************************************************************************
	main
*************************************************************************************************/

int main(int argc, char** argv)
{
	GDALAllRegister();
	OGRRegisterAll();

	// src
	OGRDataSource* src = OGRSFDriverRegistrar::Open(argv[1]);
	OGRLayer* srclayer = src->GetLayer(0);
    OGRFeatureDefn* featuredef = srclayer->GetLayerDefn();
    
    // dst
	OGRSFDriverRegistrar *registrar = OGRSFDriverRegistrar::GetRegistrar();
	OGRSFDriver          *driver = NULL;
	for(int i = 0; i < registrar->GetDriverCount() && driver == NULL;  i++)
	{
	    if( EQUAL(registrar->GetDriver(i)->GetName(),"ESRI Shapefile") )
	    {
	        driver = registrar->GetDriver(i);
	        break;
	    }
	}
	OGRDataSource* dst = driver->CreateDataSource( argv[2], NULL);
	OGRLayer* dstlayer = dst->CreateLayer(argv[2], NULL,
                                          (OGRwkbGeometryType)featuredef->GetGeomType(), NULL);
	
	// loop
	srclayer->ResetReading();
	OGRFeature* srcfeature = srclayer->GetNextFeature();

	dstlayer->StartTransaction();
	int j = 0;
	for(; srcfeature; srcfeature = srclayer->GetNextFeature(),j++)
	{	
		OGRFeature* dstfeature = srcfeature->Clone();
		OGRLineString* ogrline = (OGRLineString*)(dstfeature->StealGeometry());
		
		int n = ogrline->getNumPoints();
		printf("%d %d\n", j, n);

		for(int i = 0; i < n; i++)
		{
			double lon = ogrline->getX(i);			
			double lat = ogrline->getY(i);
			if(lon >= 180)
			{
			 	ogrline->setPoint(i,lon-360, lat);
			 }
		}

		dstfeature->SetGeometryDirectly(ogrline);
		dstlayer->CreateFeature(dstfeature);
		
		OGRFeature::DestroyFeature(srcfeature);
		OGRFeature::DestroyFeature(dstfeature);
	}
	dstlayer->CommitTransaction();

	delete src;
	delete dst;
}
