#pragma once
// Created: Mon May 14 14:47:03 CDT 2007
//
// 64-bit game permission masks
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

//tolua_begin

// 64-bit permission masks.
// Note: A client's overall permission mask changes as the
// player moves around the game, depending on side, persona,
// etc.
#define POST_MISSIONS			0x0000000000000001ULL // 1
#define BREVET_POSTING			0x0000000000000002ULL // 2
#define SQUAD_MISSIONS			0x0000000000000004ULL // 3
#define SQUAD_CO				0x0000000000000008ULL // 4
#define IS_SPAWNED				0x0000000000000010ULL // 5
#define CROSS_TALK				0x0000000000000020ULL // 6
#define CGI_MASK				0x0000000000000040ULL // 7
#define MISSION_LEADER			0x0000000000000080ULL // 8
#define BRIGADE_LEADER			0x0000000000000100ULL // 9
#define CAN_CAPTURE				0x0000000000000200ULL // 10
#define VEHICLE_LEVEL4_MASK		0x0000000000000400ULL // 11
#define VEHICLE_LEVEL5_MASK		0x0000000000000800ULL // 12
#define VEHICLE_LEVEL6_MASK		0x0000000000001000ULL // 13
#define VEHICLE_LEVEL7_MASK		0x0000000000002000ULL // 14
#define	SUBSCRIPTION_ACTIVE		0x0000000000004000ULL // 15  User can use auth bypass
#define VEHICLE_LEVEL1_MASK		0x0000000000008000ULL // 16
#define VEHICLE_LEVEL2_MASK		0x0000000000010000ULL // 17
#define VEHICLE_LEVEL3_MASK		0x0000000000020000ULL // 18
#define ACTIVE_OFFICER			0x0000000000040000ULL // 19
#define UNMETERED_ACCESS		0x0000000000080000ULL // 20  Does not use minutely balance
#define FREE_SIDE_SWITCH		0x0000000000100000ULL // 21
#define IS_MATURE				0x0000000000200000ULL // 22
#define USE_BINOCS				0x0000000000400000ULL // 23	Grant use of binoculars
#define CAN_OIC					0x0000000000800000ULL // 24	Allowed to take OIC position.
#define ALLIED_COMMAND_STAFF	0x0000000001000000ULL // 25
#define AXIS_COMMAND_STAFF		0x0000000002000000ULL // 26
#define VEHICLE_LEVEL8_MASK		0x0000000004000000ULL // 27
#define VEHICLE_LEVEL9_MASK		0x0000000008000000ULL // 28
#define ALL_SPAWN				0x0000000010000000ULL // 29
#define ALLIED_MOTD				0x0000000020000000ULL // 30
#define AXIS_MOTD				0x0000000040000000ULL // 31
#define GAME_MANAGER			0x0000000080000000ULL // 32  Can use GM commands
#define CAN_BE_EJECTED			0x0000000100000000ULL // 33  Can be ejected for speedhacking
#define GAME_TRAINER			0x0000000200000000ULL // 34  Player is a trainer/instructor
#define	IS_BUILDER				0x0000000400000000ULL // 35	Member of the builder program
#define	CAN_USE_PPO_TOOL		0x0000000800000000ULL // 36	 Can use PPO tool
#define	UNMASKED_VEHICLES		0x0000001000000000ULL // 37	Bypass any vehicle masks
#define	NO_ADS					0x0000002000000000ULL // 38	Tell the client to show upsells
#define	FULL_TRAINING			0x0000004000000000ULL // 39	Can use advanced training

// 1.34 product permission flags
#define	PRODUCT01				0x0000008000000000ULL // 40	product permission 1
#define	PRODUCT02				0x0000010000000000ULL // 41	product permission 2
#define	PRODUCT03				0x0000020000000000ULL // 42	product permission 3
#define	PRODUCT04				0x0000040000000000ULL // 43	product permission 4
#define	PRODUCT05				0x0000080000000000ULL // 44	product permission 5
#define	PRODUCT06				0x0000100000000000ULL // 45	product permission 6
#define	PRODUCT07				0x0000200000000000ULL // 46	product permission 7
#define	PRODUCT08				0x0000400000000000ULL // 47	product permission 8
#define	PRODUCT09				0x0000800000000000ULL // 48	product permission 9
#define	PRODUCT10				0x0001000000000000ULL // 49	product permission 10
#define	PRODUCT11				0x0002000000000000ULL // 50	product permission 11
#define	PRODUCT12				0x0004000000000000ULL // 51	product permission 12
#define	PRODUCT13				0x0008000000000000ULL // 52	product permission 13
#define	PRODUCT14				0x0010000000000000ULL // 53	product permission 14
#define	PRODUCT15				0x0020000000000000ULL // 54	product permission 15
#define	PRODUCT16				0x0040000000000000ULL // 55	product permission 16
#define	PRODUCT17				0x0080000000000000ULL // 56	product permission 17
#define	PRODUCT18				0x0100000000000000ULL // 57	product permission 18
#define	PRODUCT19				0x0200000000000000ULL // 58	product permission 19
#define	PRODUCT20				0x0400000000000000ULL // 59	product permission 20

// ^-^- Insert new permissions above here -^-^
#define IS_PRODUCER				0x1000000000000000ULL // 61	Producer -- use checksums
#define DARK_GLASSES			0x2000000000000000ULL // 62  Dark glasses account
#define IS_PLAYNET				0x4000000000000000ULL // 63  Playnet account

//tolua_end
