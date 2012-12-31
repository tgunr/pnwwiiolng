#pragma once
// Created: Tue Jun 13 15:11:40 2000
//
// The ticket generator header file
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

//This function returns valid "ticket", or one time password
extern UINT32 authGetOTP(void);
