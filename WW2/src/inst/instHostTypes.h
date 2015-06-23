#pragma once
// Created: Thu Oct 23 13:54:32 CDT 2008
//
// instancing host typedefs
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

typedef std::map<Instance::Descriptor, Instance::HostInstance*> HOST_INSTANCES;
extern HOST_INSTANCES g_hostInstances;

typedef std::map<instanceid_t, Instance::HostInstance*> INSTANCE_UIDS;
extern INSTANCE_UIDS g_hostInstanceUIDs;
