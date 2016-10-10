#pragma once

void strtAddErrorListEntry(int errorNo, const char* objectName, const char* entryName, const char* entryData);
void strtDataCantFindBuilding(const char* linkName, const char* role, FULL_OBJ_ID fid);
void strtDataErrorParentFacility(const char* buildingName, const char* filename, SINT32 scx, SINT32 scy, UINT16 bid, UINT16 bno, UINT16 fid);
UINT32 strtReportDataErrors(UINT32 level);
