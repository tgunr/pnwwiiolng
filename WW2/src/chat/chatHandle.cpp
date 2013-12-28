#include <syscfg.h> // Path is project specific.
// Date: Wed Jul 26 14:24:29 2000
//
// This is the actual file containing the functions for manipulating handles
// in the chat system.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include <atomic>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <wctype.h>

#include "chat/chatHandle.h"
#include "chat/chatName.h"		// to ensure it gets instantiated.
#include "wwii/wwiiUTF.h"

Chat::Name g_chatSystem("SYSTEM");
Chat::NameW g_chatUnknown(chatGetHandle(L"UNKNOWN"));

static const int HANDLE_PLAIN_BITS = 6;
static const UINT64 HANDLE_PLAIN_MASK = (1ULL << HANDLE_PLAIN_BITS) - 1;
static const int HANDLE_WCHAR_BITS = 14;
static const UINT64 HANDLE_WCHAR_MASK = (1ULL << HANDLE_WCHAR_BITS) - 1;
static const int MAX_WIDE_HANDLE_WCHARS = 64 / HANDLE_WCHAR_BITS;
static const int MAX_WIDE_HANDLE_EXTRAS = 1;
static const int MAX_WIDE_HANDLE_LEN = MAX_WIDE_HANDLE_WCHARS + MAX_WIDE_HANDLE_EXTRAS;
static const int MAX_WIDE_HANDLE_SIZE = MAX_WIDE_HANDLE_LEN + 1;

//Handles can contain:
//	upper and lowercase letters
//	numbers as of 20010203
//	space(' '), underscore(_), dash(-), asterick(*), plus(+), and equals(=)
//Players will be allowed lowercase, underscore, and dash.

// List of the characters in their compressed positions
static const UINT32 PLAIN_CHARACTERS = 64;
static const wchar_t s_characters[256] =
{
	 0, 'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',//15
    'p','q','r','s','t','u','v','w','x','y','z','A','B','C','D','E',//31
    'F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U',//47
	'V','W','X','Y','Z','0','1','2','3','4','5','6','7','8','9','-' //63
};

// Map from character values to compressed values
static const int s_characterMap[256] = {
/*		00	01	02	03	04	05	06	07	08	09	0A	0B	0C	0D	0E	0F */
/*---/-----------------------------------------------------------------*/
/*0*/	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
/*1*/	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
/*2*/	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,  63,	0,	0,
/*3*/	53,	54,	55,	56,	57,	58,	59,	60,	61,	62,	0,	0,	0,	0,	0,	0,
/*4*/	0,	27,	28,	29,	30,	31,	32,	33,	34,	35,	36,	37,	38,	39,	40,	41,
/*5*/	42,	43,	44,	45,	46,	47,	48,	49,	50,	51,	52,	0,	0,	0,	0,	63,
/*6*/	0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	12,	13,	14,	15,
/*7*/	16,	17,	18,	19,	20,	21,	22,	23,	24,	25,	26,	0,	0,	0,	0,	0
};

static const wchar_t s_wideCharacters[65535] =
{	// The latin characters we support followed by
	// table of Chinse characters based on GB2312
	// Pulled from http://www.ansell-uebersetzungen.com/gbuni.html

	  0 , 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'
	, 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's'
	, 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C'
	, 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M'
	, 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W'
	, 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5', '6'
	, '7', '8', '9', '-', L'\u4e00', L'\u4e01', L'\u4e03', L'\u4e07', L'\u4e08', L'\u4e09'
	, L'\u4e0a', L'\u4e0b', L'\u4e0c', L'\u4e0d', L'\u4e0e', L'\u4e10', L'\u4e11', L'\u4e13', L'\u4e14', L'\u4e15'
	, L'\u4e16', L'\u4e18', L'\u4e19', L'\u4e1a', L'\u4e1b', L'\u4e1c', L'\u4e1d', L'\u4e1e', L'\u4e22', L'\u4e24'
	, L'\u4e25', L'\u4e27', L'\u4e28', L'\u4e2a', L'\u4e2b', L'\u4e2c', L'\u4e2d', L'\u4e30', L'\u4e32', L'\u4e34'
	, L'\u4e36', L'\u4e38', L'\u4e39', L'\u4e3a', L'\u4e3b', L'\u4e3d', L'\u4e3e', L'\u4e3f', L'\u4e43', L'\u4e45'
	, L'\u4e47', L'\u4e48', L'\u4e49', L'\u4e4b', L'\u4e4c', L'\u4e4d', L'\u4e4e', L'\u4e4f', L'\u4e50', L'\u4e52'
	, L'\u4e53', L'\u4e54', L'\u4e56', L'\u4e58', L'\u4e59', L'\u4e5c', L'\u4e5d', L'\u4e5e', L'\u4e5f', L'\u4e60'
	, L'\u4e61', L'\u4e66', L'\u4e69', L'\u4e70', L'\u4e71', L'\u4e73', L'\u4e7e', L'\u4e86', L'\u4e88', L'\u4e89'
	, L'\u4e8b', L'\u4e8c', L'\u4e8d', L'\u4e8e', L'\u4e8f', L'\u4e91', L'\u4e92', L'\u4e93', L'\u4e94', L'\u4e95'
	, L'\u4e98', L'\u4e9a', L'\u4e9b', L'\u4e9f', L'\u4ea0', L'\u4ea1', L'\u4ea2', L'\u4ea4', L'\u4ea5', L'\u4ea6'
	, L'\u4ea7', L'\u4ea8', L'\u4ea9', L'\u4eab', L'\u4eac', L'\u4ead', L'\u4eae', L'\u4eb2', L'\u4eb3', L'\u4eb5'
	, L'\u4eba', L'\u4ebb', L'\u4ebf', L'\u4ec0', L'\u4ec1', L'\u4ec2', L'\u4ec3', L'\u4ec4', L'\u4ec5', L'\u4ec6'
	, L'\u4ec7', L'\u4ec9', L'\u4eca', L'\u4ecb', L'\u4ecd', L'\u4ece', L'\u4ed1', L'\u4ed3', L'\u4ed4', L'\u4ed5'
	, L'\u4ed6', L'\u4ed7', L'\u4ed8', L'\u4ed9', L'\u4edd', L'\u4ede', L'\u4edf', L'\u4ee1', L'\u4ee3', L'\u4ee4'
	, L'\u4ee5', L'\u4ee8', L'\u4eea', L'\u4eeb', L'\u4eec', L'\u4ef0', L'\u4ef2', L'\u4ef3', L'\u4ef5', L'\u4ef6'
	, L'\u4ef7', L'\u4efb', L'\u4efd', L'\u4eff', L'\u4f01', L'\u4f09', L'\u4f0a', L'\u4f0d', L'\u4f0e', L'\u4f0f'
	, L'\u4f10', L'\u4f11', L'\u4f17', L'\u4f18', L'\u4f19', L'\u4f1a', L'\u4f1b', L'\u4f1e', L'\u4f1f', L'\u4f20'
	, L'\u4f22', L'\u4f24', L'\u4f25', L'\u4f26', L'\u4f27', L'\u4f2a', L'\u4f2b', L'\u4f2f', L'\u4f30', L'\u4f32'
	, L'\u4f34', L'\u4f36', L'\u4f38', L'\u4f3a', L'\u4f3c', L'\u4f3d', L'\u4f43', L'\u4f46', L'\u4f4d', L'\u4f4e'
	, L'\u4f4f', L'\u4f50', L'\u4f51', L'\u4f53', L'\u4f55', L'\u4f57', L'\u4f58', L'\u4f59', L'\u4f5a', L'\u4f5b'
	, L'\u4f5c', L'\u4f5d', L'\u4f5e', L'\u4f5f', L'\u4f60', L'\u4f63', L'\u4f64', L'\u4f65', L'\u4f67', L'\u4f69'
	, L'\u4f6c', L'\u4f6f', L'\u4f70', L'\u4f73', L'\u4f74', L'\u4f76', L'\u4f7b', L'\u4f7c', L'\u4f7e', L'\u4f7f'
	, L'\u4f83', L'\u4f84', L'\u4f88', L'\u4f89', L'\u4f8b', L'\u4f8d', L'\u4f8f', L'\u4f91', L'\u4f94', L'\u4f97'
	, L'\u4f9b', L'\u4f9d', L'\u4fa0', L'\u4fa3', L'\u4fa5', L'\u4fa6', L'\u4fa7', L'\u4fa8', L'\u4fa9', L'\u4faa'
	, L'\u4fac', L'\u4fae', L'\u4faf', L'\u4fb5', L'\u4fbf', L'\u4fc3', L'\u4fc4', L'\u4fc5', L'\u4fca', L'\u4fce'
	, L'\u4fcf', L'\u4fd0', L'\u4fd1', L'\u4fd7', L'\u4fd8', L'\u4fda', L'\u4fdc', L'\u4fdd', L'\u4fde', L'\u4fdf'
	, L'\u4fe1', L'\u4fe3', L'\u4fe6', L'\u4fe8', L'\u4fe9', L'\u4fea', L'\u4fed', L'\u4fee', L'\u4fef', L'\u4ff1'
	, L'\u4ff3', L'\u4ff8', L'\u4ffa', L'\u4ffe', L'\u500c', L'\u500d', L'\u500f', L'\u5012', L'\u5014', L'\u5018'
	, L'\u5019', L'\u501a', L'\u501c', L'\u501f', L'\u5021', L'\u5025', L'\u5026', L'\u5028', L'\u5029', L'\u502a'
	, L'\u502c', L'\u502d', L'\u502e', L'\u503a', L'\u503c', L'\u503e', L'\u5043', L'\u5047', L'\u5048', L'\u504c'
	, L'\u504e', L'\u504f', L'\u5055', L'\u505a', L'\u505c', L'\u5065', L'\u506c', L'\u5076', L'\u5077', L'\u507b'
	, L'\u507e', L'\u507f', L'\u5080', L'\u5085', L'\u5088', L'\u508d', L'\u50a3', L'\u50a5', L'\u50a7', L'\u50a8'
	, L'\u50a9', L'\u50ac', L'\u50b2', L'\u50ba', L'\u50bb', L'\u50cf', L'\u50d6', L'\u50da', L'\u50e6', L'\u50e7'
	, L'\u50ec', L'\u50ed', L'\u50ee', L'\u50f3', L'\u50f5', L'\u50fb', L'\u5106', L'\u5107', L'\u510b', L'\u5112'
	, L'\u5121', L'\u513f', L'\u5140', L'\u5141', L'\u5143', L'\u5144', L'\u5145', L'\u5146', L'\u5148', L'\u5149'
	, L'\u514b', L'\u514d', L'\u5151', L'\u5154', L'\u5155', L'\u5156', L'\u515a', L'\u515c', L'\u5162', L'\u5165'
	, L'\u5168', L'\u516b', L'\u516c', L'\u516d', L'\u516e', L'\u5170', L'\u5171', L'\u5173', L'\u5174', L'\u5175'
	, L'\u5176', L'\u5177', L'\u5178', L'\u5179', L'\u517b', L'\u517c', L'\u517d', L'\u5180', L'\u5181', L'\u5182'
	, L'\u5185', L'\u5188', L'\u5189', L'\u518c', L'\u518d', L'\u5192', L'\u5195', L'\u5196', L'\u5197', L'\u5199'
	, L'\u519b', L'\u519c', L'\u51a0', L'\u51a2', L'\u51a4', L'\u51a5', L'\u51ab', L'\u51ac', L'\u51af', L'\u51b0'
	, L'\u51b1', L'\u51b2', L'\u51b3', L'\u51b5', L'\u51b6', L'\u51b7', L'\u51bb', L'\u51bc', L'\u51bd', L'\u51c0'
	, L'\u51c4', L'\u51c6', L'\u51c7', L'\u51c9', L'\u51cb', L'\u51cc', L'\u51cf', L'\u51d1', L'\u51db', L'\u51dd'
	, L'\u51e0', L'\u51e1', L'\u51e4', L'\u51eb', L'\u51ed', L'\u51ef', L'\u51f0', L'\u51f3', L'\u51f5', L'\u51f6'
	, L'\u51f8', L'\u51f9', L'\u51fa', L'\u51fb', L'\u51fc', L'\u51fd', L'\u51ff', L'\u5200', L'\u5201', L'\u5202'
	, L'\u5203', L'\u5206', L'\u5207', L'\u5208', L'\u520a', L'\u520d', L'\u520e', L'\u5211', L'\u5212', L'\u5216'
	, L'\u5217', L'\u5218', L'\u5219', L'\u521a', L'\u521b', L'\u521d', L'\u5220', L'\u5224', L'\u5228', L'\u5229'
	, L'\u522b', L'\u522d', L'\u522e', L'\u5230', L'\u5233', L'\u5236', L'\u5237', L'\u5238', L'\u5239', L'\u523a'
	, L'\u523b', L'\u523d', L'\u523f', L'\u5240', L'\u5241', L'\u5242', L'\u5243', L'\u524a', L'\u524c', L'\u524d'
	, L'\u5250', L'\u5251', L'\u5254', L'\u5256', L'\u525c', L'\u525e', L'\u5261', L'\u5265', L'\u5267', L'\u5269'
	, L'\u526a', L'\u526f', L'\u5272', L'\u527d', L'\u527f', L'\u5281', L'\u5282', L'\u5288', L'\u5290', L'\u5293'
	, L'\u529b', L'\u529d', L'\u529e', L'\u529f', L'\u52a0', L'\u52a1', L'\u52a2', L'\u52a3', L'\u52a8', L'\u52a9'
	, L'\u52aa', L'\u52ab', L'\u52ac', L'\u52ad', L'\u52b1', L'\u52b2', L'\u52b3', L'\u52be', L'\u52bf', L'\u52c3'
	, L'\u52c7', L'\u52c9', L'\u52cb', L'\u52d0', L'\u52d2', L'\u52d6', L'\u52d8', L'\u52df', L'\u52e4', L'\u52f0'
	, L'\u52f9', L'\u52fa', L'\u52fe', L'\u52ff', L'\u5300', L'\u5305', L'\u5306', L'\u5308', L'\u530d', L'\u530f'
	, L'\u5310', L'\u5315', L'\u5316', L'\u5317', L'\u5319', L'\u531a', L'\u531d', L'\u5320', L'\u5321', L'\u5323'
	, L'\u5326', L'\u532a', L'\u532e', L'\u5339', L'\u533a', L'\u533b', L'\u533e', L'\u533f', L'\u5341', L'\u5343'
	, L'\u5345', L'\u5347', L'\u5348', L'\u5349', L'\u534a', L'\u534e', L'\u534f', L'\u5351', L'\u5352', L'\u5353'
	, L'\u5355', L'\u5356', L'\u5357', L'\u535a', L'\u535c', L'\u535e', L'\u535f', L'\u5360', L'\u5361', L'\u5362'
	, L'\u5363', L'\u5364', L'\u5366', L'\u5367', L'\u5369', L'\u536b', L'\u536e', L'\u536f', L'\u5370', L'\u5371'
	, L'\u5373', L'\u5374', L'\u5375', L'\u5377', L'\u5378', L'\u537a', L'\u537f', L'\u5382', L'\u5384', L'\u5385'
	, L'\u5386', L'\u5389', L'\u538b', L'\u538c', L'\u538d', L'\u5395', L'\u5398', L'\u539a', L'\u539d', L'\u539f'
	, L'\u53a2', L'\u53a3', L'\u53a5', L'\u53a6', L'\u53a8', L'\u53a9', L'\u53ae', L'\u53b6', L'\u53bb', L'\u53bf'
	, L'\u53c1', L'\u53c2', L'\u53c8', L'\u53c9', L'\u53ca', L'\u53cb', L'\u53cc', L'\u53cd', L'\u53d1', L'\u53d4'
	, L'\u53d6', L'\u53d7', L'\u53d8', L'\u53d9', L'\u53db', L'\u53df', L'\u53e0', L'\u53e3', L'\u53e4', L'\u53e5'
	, L'\u53e6', L'\u53e8', L'\u53e9', L'\u53ea', L'\u53eb', L'\u53ec', L'\u53ed', L'\u53ee', L'\u53ef', L'\u53f0'
	, L'\u53f1', L'\u53f2', L'\u53f3', L'\u53f5', L'\u53f6', L'\u53f7', L'\u53f8', L'\u53f9', L'\u53fb', L'\u53fc'
	, L'\u53fd', L'\u5401', L'\u5403', L'\u5404', L'\u5406', L'\u5408', L'\u5409', L'\u540a', L'\u540c', L'\u540d'
	, L'\u540e', L'\u540f', L'\u5410', L'\u5411', L'\u5412', L'\u5413', L'\u5415', L'\u5416', L'\u5417', L'\u541b'
	, L'\u541d', L'\u541e', L'\u541f', L'\u5420', L'\u5421', L'\u5423', L'\u5426', L'\u5427', L'\u5428', L'\u5429'
	, L'\u542b', L'\u542c', L'\u542d', L'\u542e', L'\u542f', L'\u5431', L'\u5432', L'\u5434', L'\u5435', L'\u5438'
	, L'\u5439', L'\u543b', L'\u543c', L'\u543e', L'\u5440', L'\u5443', L'\u5446', L'\u5448', L'\u544a', L'\u544b'
	, L'\u5450', L'\u5452', L'\u5453', L'\u5454', L'\u5455', L'\u5456', L'\u5457', L'\u5458', L'\u5459', L'\u545b'
	, L'\u545c', L'\u5462', L'\u5464', L'\u5466', L'\u5468', L'\u5471', L'\u5472', L'\u5473', L'\u5475', L'\u5476'
	, L'\u5477', L'\u5478', L'\u547b', L'\u547c', L'\u547d', L'\u5480', L'\u5482', L'\u5484', L'\u5486', L'\u548b'
	, L'\u548c', L'\u548e', L'\u548f', L'\u5490', L'\u5492', L'\u5494', L'\u5495', L'\u5496', L'\u5499', L'\u549a'
	, L'\u549b', L'\u549d', L'\u54a3', L'\u54a4', L'\u54a6', L'\u54a7', L'\u54a8', L'\u54a9', L'\u54aa', L'\u54ab'
	, L'\u54ac', L'\u54ad', L'\u54af', L'\u54b1', L'\u54b3', L'\u54b4', L'\u54b8', L'\u54bb', L'\u54bd', L'\u54bf'
	, L'\u54c0', L'\u54c1', L'\u54c2', L'\u54c4', L'\u54c6', L'\u54c7', L'\u54c8', L'\u54c9', L'\u54cc', L'\u54cd'
	, L'\u54ce', L'\u54cf', L'\u54d0', L'\u54d1', L'\u54d2', L'\u54d3', L'\u54d4', L'\u54d5', L'\u54d7', L'\u54d9'
	, L'\u54da', L'\u54dc', L'\u54dd', L'\u54de', L'\u54df', L'\u54e5', L'\u54e6', L'\u54e7', L'\u54e8', L'\u54e9'
	, L'\u54ea', L'\u54ed', L'\u54ee', L'\u54f2', L'\u54f3', L'\u54fa', L'\u54fc', L'\u54fd', L'\u54ff', L'\u5501'
	, L'\u5506', L'\u5507', L'\u5509', L'\u550f', L'\u5510', L'\u5511', L'\u5514', L'\u551b', L'\u5520', L'\u5522'
	, L'\u5523', L'\u5524', L'\u5527', L'\u552a', L'\u552c', L'\u552e', L'\u552f', L'\u5530', L'\u5531', L'\u5533'
	, L'\u5537', L'\u553c', L'\u553e', L'\u553f', L'\u5541', L'\u5543', L'\u5544', L'\u5546', L'\u5549', L'\u554a'
	, L'\u5550', L'\u5555', L'\u5556', L'\u555c', L'\u5561', L'\u5564', L'\u5565', L'\u5566', L'\u5567', L'\u556a'
	, L'\u556c', L'\u556d', L'\u556e', L'\u5575', L'\u5576', L'\u5577', L'\u5578', L'\u557b', L'\u557c', L'\u557e'
	, L'\u5580', L'\u5581', L'\u5582', L'\u5583', L'\u5584', L'\u5587', L'\u5588', L'\u5589', L'\u558a', L'\u558b'
	, L'\u558f', L'\u5591', L'\u5594', L'\u5598', L'\u5599', L'\u559c', L'\u559d', L'\u559f', L'\u55a7', L'\u55b1'
	, L'\u55b3', L'\u55b5', L'\u55b7', L'\u55b9', L'\u55bb', L'\u55bd', L'\u55be', L'\u55c4', L'\u55c5', L'\u55c9'
	, L'\u55cc', L'\u55cd', L'\u55d1', L'\u55d2', L'\u55d3', L'\u55d4', L'\u55d6', L'\u55dc', L'\u55dd', L'\u55df'
	, L'\u55e1', L'\u55e3', L'\u55e4', L'\u55e5', L'\u55e6', L'\u55e8', L'\u55ea', L'\u55eb', L'\u55ec', L'\u55ef'
	, L'\u55f2', L'\u55f3', L'\u55f5', L'\u55f7', L'\u55fd', L'\u55fe', L'\u5600', L'\u5601', L'\u5608', L'\u5609'
	, L'\u560c', L'\u560e', L'\u560f', L'\u5618', L'\u561b', L'\u561e', L'\u561f', L'\u5623', L'\u5624', L'\u5627'
	, L'\u562c', L'\u562d', L'\u5631', L'\u5632', L'\u5634', L'\u5636', L'\u5639', L'\u563b', L'\u563f', L'\u564c'
	, L'\u564d', L'\u564e', L'\u5654', L'\u5657', L'\u5658', L'\u5659', L'\u565c', L'\u5662', L'\u5664', L'\u5668'
	, L'\u5669', L'\u566a', L'\u566b', L'\u566c', L'\u5671', L'\u5676', L'\u567b', L'\u567c', L'\u5685', L'\u5686'
	, L'\u568e', L'\u568f', L'\u5693', L'\u56a3', L'\u56af', L'\u56b7', L'\u56bc', L'\u56ca', L'\u56d4', L'\u56d7'
	, L'\u56da', L'\u56db', L'\u56dd', L'\u56de', L'\u56df', L'\u56e0', L'\u56e1', L'\u56e2', L'\u56e4', L'\u56eb'
	, L'\u56ed', L'\u56f0', L'\u56f1', L'\u56f4', L'\u56f5', L'\u56f9', L'\u56fa', L'\u56fd', L'\u56fe', L'\u56ff'
	, L'\u5703', L'\u5704', L'\u5706', L'\u5708', L'\u5709', L'\u570a', L'\u571c', L'\u571f', L'\u5723', L'\u5728'
	, L'\u5729', L'\u572a', L'\u572c', L'\u572d', L'\u572e', L'\u572f', L'\u5730', L'\u5733', L'\u5739', L'\u573a'
	, L'\u573b', L'\u573e', L'\u5740', L'\u5742', L'\u5747', L'\u574a', L'\u574c', L'\u574d', L'\u574e', L'\u574f'
	, L'\u5750', L'\u5751', L'\u5757', L'\u575a', L'\u575b', L'\u575c', L'\u575d', L'\u575e', L'\u575f', L'\u5760'
	, L'\u5761', L'\u5764', L'\u5766', L'\u5768', L'\u5769', L'\u576a', L'\u576b', L'\u576d', L'\u576f', L'\u5773'
	, L'\u5776', L'\u5777', L'\u577b', L'\u577c', L'\u5782', L'\u5783', L'\u5784', L'\u5785', L'\u5786', L'\u578b'
	, L'\u578c', L'\u5792', L'\u5793', L'\u579b', L'\u57a0', L'\u57a1', L'\u57a2', L'\u57a3', L'\u57a4', L'\u57a6'
	, L'\u57a7', L'\u57a9', L'\u57ab', L'\u57ad', L'\u57ae', L'\u57b2', L'\u57b4', L'\u57b8', L'\u57c2', L'\u57c3'
	, L'\u57cb', L'\u57ce', L'\u57cf', L'\u57d2', L'\u57d4', L'\u57d5', L'\u57d8', L'\u57d9', L'\u57da', L'\u57dd'
	, L'\u57df', L'\u57e0', L'\u57e4', L'\u57ed', L'\u57ef', L'\u57f4', L'\u57f8', L'\u57f9', L'\u57fa', L'\u57fd'
	, L'\u5800', L'\u5802', L'\u5806', L'\u5807', L'\u580b', L'\u580d', L'\u5811', L'\u5815', L'\u5819', L'\u581e'
	, L'\u5820', L'\u5821', L'\u5824', L'\u582a', L'\u5830', L'\u5835', L'\u5844', L'\u584c', L'\u584d', L'\u5851'
	, L'\u5854', L'\u5858', L'\u585e', L'\u5865', L'\u586b', L'\u586c', L'\u587e', L'\u5880', L'\u5881', L'\u5883'
	, L'\u5885', L'\u5889', L'\u5892', L'\u5893', L'\u5899', L'\u589a', L'\u589e', L'\u589f', L'\u58a8', L'\u58a9'
	, L'\u58bc', L'\u58c1', L'\u58c5', L'\u58d1', L'\u58d5', L'\u58e4', L'\u58eb', L'\u58ec', L'\u58ee', L'\u58f0'
	, L'\u58f3', L'\u58f6', L'\u58f9', L'\u5902', L'\u5904', L'\u5907', L'\u590d', L'\u590f', L'\u5914', L'\u5915'
	, L'\u5916', L'\u5919', L'\u591a', L'\u591c', L'\u591f', L'\u5924', L'\u5925', L'\u5927', L'\u5929', L'\u592a'
	, L'\u592b', L'\u592d', L'\u592e', L'\u592f', L'\u5931', L'\u5934', L'\u5937', L'\u5938', L'\u5939', L'\u593a'
	, L'\u593c', L'\u5941', L'\u5942', L'\u5944', L'\u5947', L'\u5948', L'\u5949', L'\u594b', L'\u594e', L'\u594f'
	, L'\u5951', L'\u5954', L'\u5955', L'\u5956', L'\u5957', L'\u5958', L'\u595a', L'\u5960', L'\u5962', L'\u5965'
	, L'\u5973', L'\u5974', L'\u5976', L'\u5978', L'\u5979', L'\u597d', L'\u5981', L'\u5982', L'\u5983', L'\u5984'
	, L'\u5986', L'\u5987', L'\u5988', L'\u598a', L'\u598d', L'\u5992', L'\u5993', L'\u5996', L'\u5997', L'\u5999'
	, L'\u599e', L'\u59a3', L'\u59a4', L'\u59a5', L'\u59a8', L'\u59a9', L'\u59aa', L'\u59ab', L'\u59ae', L'\u59af'
	, L'\u59b2', L'\u59b9', L'\u59bb', L'\u59be', L'\u59c6', L'\u59ca', L'\u59cb', L'\u59d0', L'\u59d1', L'\u59d2'
	, L'\u59d3', L'\u59d4', L'\u59d7', L'\u59d8', L'\u59da', L'\u59dc', L'\u59dd', L'\u59e3', L'\u59e5', L'\u59e8'
	, L'\u59ec', L'\u59f9', L'\u59fb', L'\u59ff', L'\u5a01', L'\u5a03', L'\u5a04', L'\u5a05', L'\u5a06', L'\u5a07'
	, L'\u5a08', L'\u5a09', L'\u5a0c', L'\u5a11', L'\u5a13', L'\u5a18', L'\u5a1c', L'\u5a1f', L'\u5a20', L'\u5a23'
	, L'\u5a25', L'\u5a29', L'\u5a31', L'\u5a32', L'\u5a34', L'\u5a36', L'\u5a3c', L'\u5a40', L'\u5a46', L'\u5a49'
	, L'\u5a4a', L'\u5a55', L'\u5a5a', L'\u5a62', L'\u5a67', L'\u5a6a', L'\u5a74', L'\u5a75', L'\u5a76', L'\u5a77'
	, L'\u5a7a', L'\u5a7f', L'\u5a92', L'\u5a9a', L'\u5a9b', L'\u5aaa', L'\u5ab2', L'\u5ab3', L'\u5ab5', L'\u5ab8'
	, L'\u5abe', L'\u5ac1', L'\u5ac2', L'\u5ac9', L'\u5acc', L'\u5ad2', L'\u5ad4', L'\u5ad6', L'\u5ad8', L'\u5adc'
	, L'\u5ae0', L'\u5ae1', L'\u5ae3', L'\u5ae6', L'\u5ae9', L'\u5aeb', L'\u5af1', L'\u5b09', L'\u5b16', L'\u5b17'
	, L'\u5b32', L'\u5b34', L'\u5b37', L'\u5b40', L'\u5b50', L'\u5b51', L'\u5b53', L'\u5b54', L'\u5b55', L'\u5b57'
	, L'\u5b58', L'\u5b59', L'\u5b5a', L'\u5b5b', L'\u5b5c', L'\u5b5d', L'\u5b5f', L'\u5b62', L'\u5b63', L'\u5b64'
	, L'\u5b65', L'\u5b66', L'\u5b69', L'\u5b6a', L'\u5b6c', L'\u5b70', L'\u5b71', L'\u5b73', L'\u5b75', L'\u5b7a'
	, L'\u5b7d', L'\u5b80', L'\u5b81', L'\u5b83', L'\u5b84', L'\u5b85', L'\u5b87', L'\u5b88', L'\u5b89', L'\u5b8b'
	, L'\u5b8c', L'\u5b8f', L'\u5b93', L'\u5b95', L'\u5b97', L'\u5b98', L'\u5b99', L'\u5b9a', L'\u5b9b', L'\u5b9c'
	, L'\u5b9d', L'\u5b9e', L'\u5ba0', L'\u5ba1', L'\u5ba2', L'\u5ba3', L'\u5ba4', L'\u5ba5', L'\u5ba6', L'\u5baa'
	, L'\u5bab', L'\u5bb0', L'\u5bb3', L'\u5bb4', L'\u5bb5', L'\u5bb6', L'\u5bb8', L'\u5bb9', L'\u5bbd', L'\u5bbe'
	, L'\u5bbf', L'\u5bc2', L'\u5bc4', L'\u5bc5', L'\u5bc6', L'\u5bc7', L'\u5bcc', L'\u5bd0', L'\u5bd2', L'\u5bd3'
	, L'\u5bdd', L'\u5bde', L'\u5bdf', L'\u5be1', L'\u5be4', L'\u5be5', L'\u5be8', L'\u5bee', L'\u5bf0', L'\u5bf8'
	, L'\u5bf9', L'\u5bfa', L'\u5bfb', L'\u5bfc', L'\u5bff', L'\u5c01', L'\u5c04', L'\u5c06', L'\u5c09', L'\u5c0a'
	, L'\u5c0f', L'\u5c11', L'\u5c14', L'\u5c15', L'\u5c16', L'\u5c18', L'\u5c1a', L'\u5c1c', L'\u5c1d', L'\u5c22'
	, L'\u5c24', L'\u5c25', L'\u5c27', L'\u5c2c', L'\u5c31', L'\u5c34', L'\u5c38', L'\u5c39', L'\u5c3a', L'\u5c3b'
	, L'\u5c3c', L'\u5c3d', L'\u5c3e', L'\u5c3f', L'\u5c40', L'\u5c41', L'\u5c42', L'\u5c45', L'\u5c48', L'\u5c49'
	, L'\u5c4a', L'\u5c4b', L'\u5c4e', L'\u5c4f', L'\u5c50', L'\u5c51', L'\u5c55', L'\u5c59', L'\u5c5e', L'\u5c60'
	, L'\u5c61', L'\u5c63', L'\u5c65', L'\u5c66', L'\u5c6e', L'\u5c6f', L'\u5c71', L'\u5c79', L'\u5c7a', L'\u5c7f'
	, L'\u5c81', L'\u5c82', L'\u5c88', L'\u5c8c', L'\u5c8d', L'\u5c90', L'\u5c91', L'\u5c94', L'\u5c96', L'\u5c97'
	, L'\u5c98', L'\u5c99', L'\u5c9a', L'\u5c9b', L'\u5c9c', L'\u5ca2', L'\u5ca3', L'\u5ca9', L'\u5cab', L'\u5cac'
	, L'\u5cad', L'\u5cb1', L'\u5cb3', L'\u5cb5', L'\u5cb7', L'\u5cb8', L'\u5cbd', L'\u5cbf', L'\u5cc1', L'\u5cc4'
	, L'\u5ccb', L'\u5cd2', L'\u5cd9', L'\u5ce1', L'\u5ce4', L'\u5ce5', L'\u5ce6', L'\u5ce8', L'\u5cea', L'\u5ced'
	, L'\u5cf0', L'\u5cfb', L'\u5d02', L'\u5d03', L'\u5d06', L'\u5d07', L'\u5d0e', L'\u5d14', L'\u5d16', L'\u5d1b'
	, L'\u5d1e', L'\u5d24', L'\u5d26', L'\u5d27', L'\u5d29', L'\u5d2d', L'\u5d2e', L'\u5d34', L'\u5d3d', L'\u5d3e'
	, L'\u5d47', L'\u5d4a', L'\u5d4b', L'\u5d4c', L'\u5d58', L'\u5d5b', L'\u5d5d', L'\u5d69', L'\u5d6b', L'\u5d6c'
	, L'\u5d6f', L'\u5d74', L'\u5d82', L'\u5d99', L'\u5d9d', L'\u5db7', L'\u5dc5', L'\u5dcd', L'\u5ddb', L'\u5ddd'
	, L'\u5dde', L'\u5de1', L'\u5de2', L'\u5de5', L'\u5de6', L'\u5de7', L'\u5de8', L'\u5de9', L'\u5deb', L'\u5dee'
	, L'\u5def', L'\u5df1', L'\u5df2', L'\u5df3', L'\u5df4', L'\u5df7', L'\u5dfd', L'\u5dfe', L'\u5e01', L'\u5e02'
	, L'\u5e03', L'\u5e05', L'\u5e06', L'\u5e08', L'\u5e0c', L'\u5e0f', L'\u5e10', L'\u5e11', L'\u5e14', L'\u5e15'
	, L'\u5e16', L'\u5e18', L'\u5e19', L'\u5e1a', L'\u5e1b', L'\u5e1c', L'\u5e1d', L'\u5e26', L'\u5e27', L'\u5e2d'
	, L'\u5e2e', L'\u5e31', L'\u5e37', L'\u5e38', L'\u5e3b', L'\u5e3c', L'\u5e3d', L'\u5e42', L'\u5e44', L'\u5e45'
	, L'\u5e4c', L'\u5e54', L'\u5e55', L'\u5e5b', L'\u5e5e', L'\u5e61', L'\u5e62', L'\u5e72', L'\u5e73', L'\u5e74'
	, L'\u5e76', L'\u5e78', L'\u5e7a', L'\u5e7b', L'\u5e7c', L'\u5e7d', L'\u5e7f', L'\u5e80', L'\u5e84', L'\u5e86'
	, L'\u5e87', L'\u5e8a', L'\u5e8b', L'\u5e8f', L'\u5e90', L'\u5e91', L'\u5e93', L'\u5e94', L'\u5e95', L'\u5e96'
	, L'\u5e97', L'\u5e99', L'\u5e9a', L'\u5e9c', L'\u5e9e', L'\u5e9f', L'\u5ea0', L'\u5ea5', L'\u5ea6', L'\u5ea7'
	, L'\u5ead', L'\u5eb3', L'\u5eb5', L'\u5eb6', L'\u5eb7', L'\u5eb8', L'\u5eb9', L'\u5ebe', L'\u5ec9', L'\u5eca'
	, L'\u5ed1', L'\u5ed2', L'\u5ed3', L'\u5ed6', L'\u5edb', L'\u5ee8', L'\u5eea', L'\u5ef4', L'\u5ef6', L'\u5ef7'
	, L'\u5efa', L'\u5efe', L'\u5eff', L'\u5f00', L'\u5f01', L'\u5f02', L'\u5f03', L'\u5f04', L'\u5f08', L'\u5f0a'
	, L'\u5f0b', L'\u5f0f', L'\u5f11', L'\u5f13', L'\u5f15', L'\u5f17', L'\u5f18', L'\u5f1b', L'\u5f1f', L'\u5f20'
	, L'\u5f25', L'\u5f26', L'\u5f27', L'\u5f29', L'\u5f2a', L'\u5f2d', L'\u5f2f', L'\u5f31', L'\u5f39', L'\u5f3a'
	, L'\u5f3c', L'\u5f40', L'\u5f50', L'\u5f52', L'\u5f53', L'\u5f55', L'\u5f56', L'\u5f57', L'\u5f58', L'\u5f5d'
	, L'\u5f61', L'\u5f62', L'\u5f64', L'\u5f66', L'\u5f69', L'\u5f6a', L'\u5f6c', L'\u5f6d', L'\u5f70', L'\u5f71'
	, L'\u5f73', L'\u5f77', L'\u5f79', L'\u5f7b', L'\u5f7c', L'\u5f80', L'\u5f81', L'\u5f82', L'\u5f84', L'\u5f85'
	, L'\u5f87', L'\u5f88', L'\u5f89', L'\u5f8a', L'\u5f8b', L'\u5f8c', L'\u5f90', L'\u5f92', L'\u5f95', L'\u5f97'
	, L'\u5f98', L'\u5f99', L'\u5f9c', L'\u5fa1', L'\u5fa8', L'\u5faa', L'\u5fad', L'\u5fae', L'\u5fb5', L'\u5fb7'
	, L'\u5fbc', L'\u5fbd', L'\u5fc3', L'\u5fc4', L'\u5fc5', L'\u5fc6', L'\u5fc9', L'\u5fcc', L'\u5fcd', L'\u5fcf'
	, L'\u5fd0', L'\u5fd1', L'\u5fd2', L'\u5fd6', L'\u5fd7', L'\u5fd8', L'\u5fd9', L'\u5fdd', L'\u5fe0', L'\u5fe1'
	, L'\u5fe4', L'\u5fe7', L'\u5fea', L'\u5feb', L'\u5fed', L'\u5fee', L'\u5ff1', L'\u5ff5', L'\u5ff8', L'\u5ffb'
	, L'\u5ffd', L'\u5ffe', L'\u5fff', L'\u6000', L'\u6001', L'\u6002', L'\u6003', L'\u6004', L'\u6005', L'\u6006'
	, L'\u600a', L'\u600d', L'\u600e', L'\u600f', L'\u6012', L'\u6014', L'\u6015', L'\u6016', L'\u6019', L'\u601b'
	, L'\u601c', L'\u601d', L'\u6020', L'\u6021', L'\u6025', L'\u6026', L'\u6027', L'\u6028', L'\u6029', L'\u602a'
	, L'\u602b', L'\u602f', L'\u6035', L'\u603b', L'\u603c', L'\u603f', L'\u6041', L'\u6042', L'\u6043', L'\u604b'
	, L'\u604d', L'\u6050', L'\u6052', L'\u6055', L'\u6059', L'\u605a', L'\u605d', L'\u6062', L'\u6063', L'\u6064'
	, L'\u6067', L'\u6068', L'\u6069', L'\u606a', L'\u606b', L'\u606c', L'\u606d', L'\u606f', L'\u6070', L'\u6073'
	, L'\u6076', L'\u6078', L'\u6079', L'\u607a', L'\u607b', L'\u607c', L'\u607d', L'\u607f', L'\u6083', L'\u6084'
	, L'\u6089', L'\u608c', L'\u608d', L'\u6092', L'\u6094', L'\u6096', L'\u609a', L'\u609b', L'\u609d', L'\u609f'
	, L'\u60a0', L'\u60a3', L'\u60a6', L'\u60a8', L'\u60ab', L'\u60ac', L'\u60ad', L'\u60af', L'\u60b1', L'\u60b2'
	, L'\u60b4', L'\u60b8', L'\u60bb', L'\u60bc', L'\u60c5', L'\u60c6', L'\u60ca', L'\u60cb', L'\u60d1', L'\u60d5'
	, L'\u60d8', L'\u60da', L'\u60dc', L'\u60dd', L'\u60df', L'\u60e0', L'\u60e6', L'\u60e7', L'\u60e8', L'\u60e9'
	, L'\u60eb', L'\u60ec', L'\u60ed', L'\u60ee', L'\u60ef', L'\u60f0', L'\u60f3', L'\u60f4', L'\u60f6', L'\u60f9'
	, L'\u60fa', L'\u6100', L'\u6101', L'\u6106', L'\u6108', L'\u6109', L'\u610d', L'\u610e', L'\u610f', L'\u6115'
	, L'\u611a', L'\u611f', L'\u6120', L'\u6123', L'\u6124', L'\u6126', L'\u6127', L'\u612b', L'\u613f', L'\u6148'
	, L'\u614a', L'\u614c', L'\u614e', L'\u6151', L'\u6155', L'\u615d', L'\u6162', L'\u6167', L'\u6168', L'\u6170'
	, L'\u6175', L'\u6177', L'\u618b', L'\u618e', L'\u6194', L'\u619d', L'\u61a7', L'\u61a8', L'\u61a9', L'\u61ac'
	, L'\u61b7', L'\u61be', L'\u61c2', L'\u61c8', L'\u61ca', L'\u61cb', L'\u61d1', L'\u61d2', L'\u61d4', L'\u61e6'
	, L'\u61f5', L'\u61ff', L'\u6206', L'\u6208', L'\u620a', L'\u620b', L'\u620c', L'\u620d', L'\u620e', L'\u620f'
	, L'\u6210', L'\u6211', L'\u6212', L'\u6215', L'\u6216', L'\u6217', L'\u6218', L'\u621a', L'\u621b', L'\u621f'
	, L'\u6221', L'\u6222', L'\u6224', L'\u6225', L'\u622a', L'\u622c', L'\u622e', L'\u6233', L'\u6234', L'\u6237'
	, L'\u623d', L'\u623e', L'\u623f', L'\u6240', L'\u6241', L'\u6243', L'\u6247', L'\u6248', L'\u6249', L'\u624b'
	, L'\u624c', L'\u624d', L'\u624e', L'\u6251', L'\u6252', L'\u6253', L'\u6254', L'\u6258', L'\u625b', L'\u6263'
	, L'\u6266', L'\u6267', L'\u6269', L'\u626a', L'\u626b', L'\u626c', L'\u626d', L'\u626e', L'\u626f', L'\u6270'
	, L'\u6273', L'\u6276', L'\u6279', L'\u627c', L'\u627e', L'\u627f', L'\u6280', L'\u6284', L'\u6289', L'\u628a'
	, L'\u6291', L'\u6292', L'\u6293', L'\u6295', L'\u6296', L'\u6297', L'\u6298', L'\u629a', L'\u629b', L'\u629f'
	, L'\u62a0', L'\u62a1', L'\u62a2', L'\u62a4', L'\u62a5', L'\u62a8', L'\u62ab', L'\u62ac', L'\u62b1', L'\u62b5'
	, L'\u62b9', L'\u62bb', L'\u62bc', L'\u62bd', L'\u62bf', L'\u62c2', L'\u62c4', L'\u62c5', L'\u62c6', L'\u62c7'
	, L'\u62c8', L'\u62c9', L'\u62ca', L'\u62cc', L'\u62cd', L'\u62ce', L'\u62d0', L'\u62d2', L'\u62d3', L'\u62d4'
	, L'\u62d6', L'\u62d7', L'\u62d8', L'\u62d9', L'\u62da', L'\u62db', L'\u62dc', L'\u62df', L'\u62e2', L'\u62e3'
	, L'\u62e5', L'\u62e6', L'\u62e7', L'\u62e8', L'\u62e9', L'\u62ec', L'\u62ed', L'\u62ee', L'\u62ef', L'\u62f1'
	, L'\u62f3', L'\u62f4', L'\u62f6', L'\u62f7', L'\u62fc', L'\u62fd', L'\u62fe', L'\u62ff', L'\u6301', L'\u6302'
	, L'\u6307', L'\u6308', L'\u6309', L'\u630e', L'\u6311', L'\u6316', L'\u631a', L'\u631b', L'\u631d', L'\u631e'
	, L'\u631f', L'\u6320', L'\u6321', L'\u6322', L'\u6323', L'\u6324', L'\u6325', L'\u6328', L'\u632a', L'\u632b'
	, L'\u632f', L'\u6332', L'\u6339', L'\u633a', L'\u633d', L'\u6342', L'\u6343', L'\u6345', L'\u6346', L'\u6349'
	, L'\u634b', L'\u634c', L'\u634d', L'\u634e', L'\u634f', L'\u6350', L'\u6355', L'\u635e', L'\u635f', L'\u6361'
	, L'\u6362', L'\u6363', L'\u6367', L'\u6369', L'\u636d', L'\u636e', L'\u6371', L'\u6376', L'\u6377', L'\u637a'
	, L'\u637b', L'\u6380', L'\u6382', L'\u6387', L'\u6388', L'\u6389', L'\u638a', L'\u638c', L'\u638e', L'\u638f'
	, L'\u6390', L'\u6392', L'\u6396', L'\u6398', L'\u63a0', L'\u63a2', L'\u63a3', L'\u63a5', L'\u63a7', L'\u63a8'
	, L'\u63a9', L'\u63aa', L'\u63ac', L'\u63ad', L'\u63ae', L'\u63b0', L'\u63b3', L'\u63b4', L'\u63b7', L'\u63b8'
	, L'\u63ba', L'\u63bc', L'\u63be', L'\u63c4', L'\u63c6', L'\u63c9', L'\u63cd', L'\u63ce', L'\u63cf', L'\u63d0'
	, L'\u63d2', L'\u63d6', L'\u63de', L'\u63e0', L'\u63e1', L'\u63e3', L'\u63e9', L'\u63ea', L'\u63ed', L'\u63f2'
	, L'\u63f4', L'\u63f6', L'\u63f8', L'\u63fd', L'\u63ff', L'\u6400', L'\u6401', L'\u6402', L'\u6405', L'\u640b'
	, L'\u640c', L'\u640f', L'\u6410', L'\u6413', L'\u6414', L'\u641b', L'\u641c', L'\u641e', L'\u6420', L'\u6421'
	, L'\u6426', L'\u642a', L'\u642c', L'\u642d', L'\u6434', L'\u643a', L'\u643d', L'\u643f', L'\u6441', L'\u6444'
	, L'\u6445', L'\u6446', L'\u6447', L'\u6448', L'\u644a', L'\u6452', L'\u6454', L'\u6458', L'\u645e', L'\u6467'
	, L'\u6469', L'\u646d', L'\u6478', L'\u6479', L'\u647a', L'\u6482', L'\u6484', L'\u6485', L'\u6487', L'\u6491'
	, L'\u6492', L'\u6495', L'\u6496', L'\u6499', L'\u649e', L'\u64a4', L'\u64a9', L'\u64ac', L'\u64ad', L'\u64ae'
	, L'\u64b0', L'\u64b5', L'\u64b7', L'\u64b8', L'\u64ba', L'\u64bc', L'\u64c0', L'\u64c2', L'\u64c5', L'\u64cd'
	, L'\u64ce', L'\u64d0', L'\u64d2', L'\u64d7', L'\u64d8', L'\u64de', L'\u64e2', L'\u64e4', L'\u64e6', L'\u6500'
	, L'\u6509', L'\u6512', L'\u6518', L'\u6525', L'\u652b', L'\u652e', L'\u652f', L'\u6534', L'\u6535', L'\u6536'
	, L'\u6538', L'\u6539', L'\u653b', L'\u653e', L'\u653f', L'\u6545', L'\u6548', L'\u6549', L'\u654c', L'\u654f'
	, L'\u6551', L'\u6555', L'\u6556', L'\u6559', L'\u655b', L'\u655d', L'\u655e', L'\u6562', L'\u6563', L'\u6566'
	, L'\u656b', L'\u656c', L'\u6570', L'\u6572', L'\u6574', L'\u6577', L'\u6587', L'\u658b', L'\u658c', L'\u6590'
	, L'\u6591', L'\u6593', L'\u6597', L'\u6599', L'\u659b', L'\u659c', L'\u659f', L'\u65a1', L'\u65a4', L'\u65a5'
	, L'\u65a7', L'\u65a9', L'\u65ab', L'\u65ad', L'\u65af', L'\u65b0', L'\u65b9', L'\u65bc', L'\u65bd', L'\u65c1'
	, L'\u65c3', L'\u65c4', L'\u65c5', L'\u65c6', L'\u65cb', L'\u65cc', L'\u65ce', L'\u65cf', L'\u65d2', L'\u65d6'
	, L'\u65d7', L'\u65e0', L'\u65e2', L'\u65e5', L'\u65e6', L'\u65e7', L'\u65e8', L'\u65e9', L'\u65ec', L'\u65ed'
	, L'\u65ee', L'\u65ef', L'\u65f0', L'\u65f1', L'\u65f6', L'\u65f7', L'\u65fa', L'\u6600', L'\u6602', L'\u6603'
	, L'\u6606', L'\u660a', L'\u660c', L'\u660e', L'\u660f', L'\u6613', L'\u6614', L'\u6615', L'\u6619', L'\u661d'
	, L'\u661f', L'\u6620', L'\u6625', L'\u6627', L'\u6628', L'\u662d', L'\u662f', L'\u6631', L'\u6634', L'\u6635'
	, L'\u6636', L'\u663c', L'\u663e', L'\u6641', L'\u6643', L'\u664b', L'\u664c', L'\u664f', L'\u6652', L'\u6653'
	, L'\u6654', L'\u6655', L'\u6656', L'\u6657', L'\u665a', L'\u665f', L'\u6661', L'\u6664', L'\u6666', L'\u6668'
	, L'\u666e', L'\u666f', L'\u6670', L'\u6674', L'\u6676', L'\u6677', L'\u667a', L'\u667e', L'\u6682', L'\u6684'
	, L'\u6687', L'\u668c', L'\u6691', L'\u6696', L'\u6697', L'\u669d', L'\u66a7', L'\u66a8', L'\u66ae', L'\u66b4'
	, L'\u66b9', L'\u66be', L'\u66d9', L'\u66db', L'\u66dc', L'\u66dd', L'\u66e6', L'\u66e9', L'\u66f0', L'\u66f2'
	, L'\u66f3', L'\u66f4', L'\u66f7', L'\u66f9', L'\u66fc', L'\u66fe', L'\u66ff', L'\u6700', L'\u6708', L'\u6709'
	, L'\u670a', L'\u670b', L'\u670d', L'\u6710', L'\u6714', L'\u6715', L'\u6717', L'\u671b', L'\u671d', L'\u671f'
	, L'\u6726', L'\u6728', L'\u672a', L'\u672b', L'\u672c', L'\u672d', L'\u672f', L'\u6731', L'\u6734', L'\u6735'
	, L'\u673a', L'\u673d', L'\u6740', L'\u6742', L'\u6743', L'\u6746', L'\u6748', L'\u6749', L'\u674c', L'\u674e'
	, L'\u674f', L'\u6750', L'\u6751', L'\u6753', L'\u6756', L'\u675c', L'\u675e', L'\u675f', L'\u6760', L'\u6761'
	, L'\u6765', L'\u6768', L'\u6769', L'\u676a', L'\u676d', L'\u676f', L'\u6770', L'\u6772', L'\u6773', L'\u6775'
	, L'\u6777', L'\u677c', L'\u677e', L'\u677f', L'\u6781', L'\u6784', L'\u6787', L'\u6789', L'\u678b', L'\u6790'
	, L'\u6795', L'\u6797', L'\u6798', L'\u679a', L'\u679c', L'\u679d', L'\u679e', L'\u67a2', L'\u67a3', L'\u67a5'
	, L'\u67a7', L'\u67a8', L'\u67aa', L'\u67ab', L'\u67ad', L'\u67af', L'\u67b0', L'\u67b3', L'\u67b5', L'\u67b6'
	, L'\u67b7', L'\u67b8', L'\u67c1', L'\u67c3', L'\u67c4', L'\u67cf', L'\u67d0', L'\u67d1', L'\u67d2', L'\u67d3'
	, L'\u67d4', L'\u67d8', L'\u67d9', L'\u67da', L'\u67dc', L'\u67dd', L'\u67de', L'\u67e0', L'\u67e2', L'\u67e5'
	, L'\u67e9', L'\u67ec', L'\u67ef', L'\u67f0', L'\u67f1', L'\u67f3', L'\u67f4', L'\u67fd', L'\u67ff', L'\u6800'
	, L'\u6805', L'\u6807', L'\u6808', L'\u6809', L'\u680a', L'\u680b', L'\u680c', L'\u680e', L'\u680f', L'\u6811'
	, L'\u6813', L'\u6816', L'\u6817', L'\u681d', L'\u6821', L'\u6829', L'\u682a', L'\u6832', L'\u6833', L'\u6837'
	, L'\u6838', L'\u6839', L'\u683c', L'\u683d', L'\u683e', L'\u6840', L'\u6841', L'\u6842', L'\u6843', L'\u6844'
	, L'\u6845', L'\u6846', L'\u6848', L'\u6849', L'\u684a', L'\u684c', L'\u684e', L'\u6850', L'\u6851', L'\u6853'
	, L'\u6854', L'\u6855', L'\u6860', L'\u6861', L'\u6862', L'\u6863', L'\u6864', L'\u6865', L'\u6866', L'\u6867'
	, L'\u6868', L'\u6869', L'\u686b', L'\u6874', L'\u6876', L'\u6877', L'\u6881', L'\u6883', L'\u6885', L'\u6886'
	, L'\u688f', L'\u6893', L'\u6897', L'\u68a2', L'\u68a6', L'\u68a7', L'\u68a8', L'\u68ad', L'\u68af', L'\u68b0'
	, L'\u68b3', L'\u68b5', L'\u68c0', L'\u68c2', L'\u68c9', L'\u68cb', L'\u68cd', L'\u68d2', L'\u68d5', L'\u68d8'
	, L'\u68da', L'\u68e0', L'\u68e3', L'\u68ee', L'\u68f0', L'\u68f1', L'\u68f5', L'\u68f9', L'\u68fa', L'\u68fc'
	, L'\u6901', L'\u6905', L'\u690b', L'\u690d', L'\u690e', L'\u6910', L'\u6912', L'\u691f', L'\u6920', L'\u6924'
	, L'\u692d', L'\u6930', L'\u6934', L'\u6939', L'\u693d', L'\u693f', L'\u6942', L'\u6954', L'\u6957', L'\u695a'
	, L'\u695d', L'\u695e', L'\u6960', L'\u6963', L'\u6966', L'\u696b', L'\u696e', L'\u6971', L'\u6977', L'\u6978'
	, L'\u6979', L'\u697c', L'\u6980', L'\u6982', L'\u6984', L'\u6986', L'\u6987', L'\u6988', L'\u6989', L'\u698d'
	, L'\u6994', L'\u6995', L'\u6998', L'\u699b', L'\u699c', L'\u69a7', L'\u69a8', L'\u69ab', L'\u69ad', L'\u69b1'
	, L'\u69b4', L'\u69b7', L'\u69bb', L'\u69c1', L'\u69ca', L'\u69cc', L'\u69ce', L'\u69d0', L'\u69d4', L'\u69db'
	, L'\u69df', L'\u69e0', L'\u69ed', L'\u69f2', L'\u69fd', L'\u69ff', L'\u6a0a', L'\u6a17', L'\u6a18', L'\u6a1f'
	, L'\u6a21', L'\u6a28', L'\u6a2a', L'\u6a2f', L'\u6a31', L'\u6a35', L'\u6a3d', L'\u6a3e', L'\u6a44', L'\u6a47'
	, L'\u6a50', L'\u6a58', L'\u6a59', L'\u6a5b', L'\u6a61', L'\u6a65', L'\u6a71', L'\u6a79', L'\u6a7c', L'\u6a80'
	, L'\u6a84', L'\u6a8e', L'\u6a90', L'\u6a91', L'\u6a97', L'\u6aa0', L'\u6aa9', L'\u6aab', L'\u6aac', L'\u6b20'
	, L'\u6b21', L'\u6b22', L'\u6b23', L'\u6b24', L'\u6b27', L'\u6b32', L'\u6b37', L'\u6b39', L'\u6b3a', L'\u6b3e'
	, L'\u6b43', L'\u6b46', L'\u6b47', L'\u6b49', L'\u6b4c', L'\u6b59', L'\u6b62', L'\u6b63', L'\u6b64', L'\u6b65'
	, L'\u6b66', L'\u6b67', L'\u6b6a', L'\u6b79', L'\u6b7b', L'\u6b7c', L'\u6b81', L'\u6b82', L'\u6b83', L'\u6b84'
	, L'\u6b86', L'\u6b87', L'\u6b89', L'\u6b8a', L'\u6b8b', L'\u6b8d', L'\u6b92', L'\u6b93', L'\u6b96', L'\u6b9a'
	, L'\u6b9b', L'\u6ba1', L'\u6baa', L'\u6bb3', L'\u6bb4', L'\u6bb5', L'\u6bb7', L'\u6bbf', L'\u6bc1', L'\u6bc2'
	, L'\u6bc5', L'\u6bcb', L'\u6bcd', L'\u6bcf', L'\u6bd2', L'\u6bd3', L'\u6bd4', L'\u6bd5', L'\u6bd6', L'\u6bd7'
	, L'\u6bd9', L'\u6bdb', L'\u6be1', L'\u6bea', L'\u6beb', L'\u6bef', L'\u6bf3', L'\u6bf5', L'\u6bf9', L'\u6bfd'
	, L'\u6c05', L'\u6c06', L'\u6c07', L'\u6c0d', L'\u6c0f', L'\u6c10', L'\u6c11', L'\u6c13', L'\u6c14', L'\u6c15'
	, L'\u6c16', L'\u6c18', L'\u6c19', L'\u6c1a', L'\u6c1b', L'\u6c1f', L'\u6c21', L'\u6c22', L'\u6c24', L'\u6c26'
	, L'\u6c27', L'\u6c28', L'\u6c29', L'\u6c2a', L'\u6c2e', L'\u6c2f', L'\u6c30', L'\u6c32', L'\u6c34', L'\u6c35'
	, L'\u6c38', L'\u6c3d', L'\u6c40', L'\u6c41', L'\u6c42', L'\u6c46', L'\u6c47', L'\u6c49', L'\u6c4a', L'\u6c50'
	, L'\u6c54', L'\u6c55', L'\u6c57', L'\u6c5b', L'\u6c5c', L'\u6c5d', L'\u6c5e', L'\u6c5f', L'\u6c60', L'\u6c61'
	, L'\u6c64', L'\u6c68', L'\u6c69', L'\u6c6a', L'\u6c70', L'\u6c72', L'\u6c74', L'\u6c76', L'\u6c79', L'\u6c7d'
	, L'\u6c7e', L'\u6c81', L'\u6c82', L'\u6c83', L'\u6c85', L'\u6c86', L'\u6c88', L'\u6c89', L'\u6c8c', L'\u6c8f'
	, L'\u6c90', L'\u6c93', L'\u6c94', L'\u6c99', L'\u6c9b', L'\u6c9f', L'\u6ca1', L'\u6ca3', L'\u6ca4', L'\u6ca5'
	, L'\u6ca6', L'\u6ca7', L'\u6ca9', L'\u6caa', L'\u6cab', L'\u6cad', L'\u6cae', L'\u6cb1', L'\u6cb2', L'\u6cb3'
	, L'\u6cb8', L'\u6cb9', L'\u6cbb', L'\u6cbc', L'\u6cbd', L'\u6cbe', L'\u6cbf', L'\u6cc4', L'\u6cc5', L'\u6cc9'
	, L'\u6cca', L'\u6ccc', L'\u6cd0', L'\u6cd3', L'\u6cd4', L'\u6cd5', L'\u6cd6', L'\u6cd7', L'\u6cdb', L'\u6cde'
	, L'\u6ce0', L'\u6ce1', L'\u6ce2', L'\u6ce3', L'\u6ce5', L'\u6ce8', L'\u6cea', L'\u6ceb', L'\u6cee', L'\u6cef'
	, L'\u6cf0', L'\u6cf1', L'\u6cf3', L'\u6cf5', L'\u6cf6', L'\u6cf7', L'\u6cf8', L'\u6cfa', L'\u6cfb', L'\u6cfc'
	, L'\u6cfd', L'\u6cfe', L'\u6d01', L'\u6d04', L'\u6d07', L'\u6d0b', L'\u6d0c', L'\u6d0e', L'\u6d12', L'\u6d17'
	, L'\u6d19', L'\u6d1a', L'\u6d1b', L'\u6d1e', L'\u6d25', L'\u6d27', L'\u6d2a', L'\u6d2b', L'\u6d2e', L'\u6d31'
	, L'\u6d32', L'\u6d33', L'\u6d35', L'\u6d39', L'\u6d3b', L'\u6d3c', L'\u6d3d', L'\u6d3e', L'\u6d41', L'\u6d43'
	, L'\u6d45', L'\u6d46', L'\u6d47', L'\u6d48', L'\u6d4a', L'\u6d4b', L'\u6d4d', L'\u6d4e', L'\u6d4f', L'\u6d51'
	, L'\u6d52', L'\u6d53', L'\u6d54', L'\u6d59', L'\u6d5a', L'\u6d5c', L'\u6d5e', L'\u6d60', L'\u6d63', L'\u6d66'
	, L'\u6d69', L'\u6d6a', L'\u6d6e', L'\u6d6f', L'\u6d74', L'\u6d77', L'\u6d78', L'\u6d7c', L'\u6d82', L'\u6d85'
	, L'\u6d88', L'\u6d89', L'\u6d8c', L'\u6d8e', L'\u6d91', L'\u6d93', L'\u6d94', L'\u6d95', L'\u6d9b', L'\u6d9d'
	, L'\u6d9e', L'\u6d9f', L'\u6da0', L'\u6da1', L'\u6da3', L'\u6da4', L'\u6da6', L'\u6da7', L'\u6da8', L'\u6da9'
	, L'\u6daa', L'\u6dab', L'\u6dae', L'\u6daf', L'\u6db2', L'\u6db5', L'\u6db8', L'\u6dbf', L'\u6dc0', L'\u6dc4'
	, L'\u6dc5', L'\u6dc6', L'\u6dc7', L'\u6dcb', L'\u6dcc', L'\u6dd1', L'\u6dd6', L'\u6dd8', L'\u6dd9', L'\u6ddd'
	, L'\u6dde', L'\u6de0', L'\u6de1', L'\u6de4', L'\u6de6', L'\u6deb', L'\u6dec', L'\u6dee', L'\u6df1', L'\u6df3'
	, L'\u6df7', L'\u6df9', L'\u6dfb', L'\u6dfc', L'\u6e05', L'\u6e0a', L'\u6e0c', L'\u6e0d', L'\u6e0e', L'\u6e10'
	, L'\u6e11', L'\u6e14', L'\u6e16', L'\u6e17', L'\u6e1a', L'\u6e1d', L'\u6e20', L'\u6e21', L'\u6e23', L'\u6e24'
	, L'\u6e25', L'\u6e29', L'\u6e2b', L'\u6e2d', L'\u6e2f', L'\u6e32', L'\u6e34', L'\u6e38', L'\u6e3a', L'\u6e43'
	, L'\u6e44', L'\u6e4d', L'\u6e4e', L'\u6e53', L'\u6e54', L'\u6e56', L'\u6e58', L'\u6e5b', L'\u6e5f', L'\u6e6b'
	, L'\u6e6e', L'\u6e7e', L'\u6e7f', L'\u6e83', L'\u6e85', L'\u6e86', L'\u6e89', L'\u6e8f', L'\u6e90', L'\u6e98'
	, L'\u6e9c', L'\u6e9f', L'\u6ea2', L'\u6ea5', L'\u6ea7', L'\u6eaa', L'\u6eaf', L'\u6eb1', L'\u6eb2', L'\u6eb4'
	, L'\u6eb6', L'\u6eb7', L'\u6eba', L'\u6ebb', L'\u6ebd', L'\u6ec1', L'\u6ec2', L'\u6ec7', L'\u6ecb', L'\u6ecf'
	, L'\u6ed1', L'\u6ed3', L'\u6ed4', L'\u6ed5', L'\u6ed7', L'\u6eda', L'\u6ede', L'\u6edf', L'\u6ee0', L'\u6ee1'
	, L'\u6ee2', L'\u6ee4', L'\u6ee5', L'\u6ee6', L'\u6ee8', L'\u6ee9', L'\u6ef4', L'\u6ef9', L'\u6f02', L'\u6f06'
	, L'\u6f09', L'\u6f0f', L'\u6f13', L'\u6f14', L'\u6f15', L'\u6f20', L'\u6f24', L'\u6f29', L'\u6f2a', L'\u6f2b'
	, L'\u6f2d', L'\u6f2f', L'\u6f31', L'\u6f33', L'\u6f36', L'\u6f3e', L'\u6f46', L'\u6f47', L'\u6f4b', L'\u6f4d'
	, L'\u6f58', L'\u6f5c', L'\u6f5e', L'\u6f62', L'\u6f66', L'\u6f6d', L'\u6f6e', L'\u6f72', L'\u6f74', L'\u6f78'
	, L'\u6f7a', L'\u6f7c', L'\u6f84', L'\u6f88', L'\u6f89', L'\u6f8c', L'\u6f8d', L'\u6f8e', L'\u6f9c', L'\u6fa1'
	, L'\u6fa7', L'\u6fb3', L'\u6fb6', L'\u6fb9', L'\u6fc0', L'\u6fc2', L'\u6fc9', L'\u6fd1', L'\u6fd2', L'\u6fde'
	, L'\u6fe0', L'\u6fe1', L'\u6fee', L'\u6fef', L'\u7011', L'\u701a', L'\u701b', L'\u7023', L'\u7035', L'\u7039'
	, L'\u704c', L'\u704f', L'\u705e', L'\u706b', L'\u706c', L'\u706d', L'\u706f', L'\u7070', L'\u7075', L'\u7076'
	, L'\u7078', L'\u707c', L'\u707e', L'\u707f', L'\u7080', L'\u7085', L'\u7089', L'\u708a', L'\u708e', L'\u7092'
	, L'\u7094', L'\u7095', L'\u7096', L'\u7099', L'\u709c', L'\u709d', L'\u70ab', L'\u70ac', L'\u70ad', L'\u70ae'
	, L'\u70af', L'\u70b1', L'\u70b3', L'\u70b7', L'\u70b8', L'\u70b9', L'\u70bb', L'\u70bc', L'\u70bd', L'\u70c0'
	, L'\u70c1', L'\u70c2', L'\u70c3', L'\u70c8', L'\u70ca', L'\u70d8', L'\u70d9', L'\u70db', L'\u70df', L'\u70e4'
	, L'\u70e6', L'\u70e7', L'\u70e8', L'\u70e9', L'\u70eb', L'\u70ec', L'\u70ed', L'\u70ef', L'\u70f7', L'\u70f9'
	, L'\u70fd', L'\u7109', L'\u710a', L'\u7110', L'\u7113', L'\u7115', L'\u7116', L'\u7118', L'\u7119', L'\u711a'
	, L'\u7126', L'\u712f', L'\u7130', L'\u7131', L'\u7136', L'\u7145', L'\u714a', L'\u714c', L'\u714e', L'\u715c'
	, L'\u715e', L'\u7164', L'\u7166', L'\u7167', L'\u7168', L'\u716e', L'\u7172', L'\u7173', L'\u7178', L'\u717a'
	, L'\u717d', L'\u7184', L'\u718a', L'\u718f', L'\u7194', L'\u7198', L'\u7199', L'\u719f', L'\u71a0', L'\u71a8'
	, L'\u71ac', L'\u71b3', L'\u71b5', L'\u71b9', L'\u71c3', L'\u71ce', L'\u71d4', L'\u71d5', L'\u71e0', L'\u71e5'
	, L'\u71e7', L'\u71ee', L'\u71f9', L'\u7206', L'\u721d', L'\u7228', L'\u722a', L'\u722c', L'\u7230', L'\u7231'
	, L'\u7235', L'\u7236', L'\u7237', L'\u7238', L'\u7239', L'\u723b', L'\u723d', L'\u723f', L'\u7247', L'\u7248'
	, L'\u724c', L'\u724d', L'\u7252', L'\u7256', L'\u7259', L'\u725b', L'\u725d', L'\u725f', L'\u7261', L'\u7262'
	, L'\u7266', L'\u7267', L'\u7269', L'\u726e', L'\u726f', L'\u7272', L'\u7275', L'\u7279', L'\u727a', L'\u727e'
	, L'\u727f', L'\u7280', L'\u7281', L'\u7284', L'\u728a', L'\u728b', L'\u728d', L'\u728f', L'\u7292', L'\u729f'
	, L'\u72ac', L'\u72ad', L'\u72af', L'\u72b0', L'\u72b4', L'\u72b6', L'\u72b7', L'\u72b8', L'\u72b9', L'\u72c1'
	, L'\u72c2', L'\u72c3', L'\u72c4', L'\u72c8', L'\u72cd', L'\u72ce', L'\u72d0', L'\u72d2', L'\u72d7', L'\u72d9'
	, L'\u72de', L'\u72e0', L'\u72e1', L'\u72e8', L'\u72e9', L'\u72ec', L'\u72ed', L'\u72ee', L'\u72ef', L'\u72f0'
	, L'\u72f1', L'\u72f2', L'\u72f3', L'\u72f4', L'\u72f7', L'\u72f8', L'\u72fa', L'\u72fb', L'\u72fc', L'\u7301'
	, L'\u7303', L'\u730a', L'\u730e', L'\u7313', L'\u7315', L'\u7316', L'\u7317', L'\u731b', L'\u731c', L'\u731d'
	, L'\u731e', L'\u7321', L'\u7322', L'\u7325', L'\u7329', L'\u732a', L'\u732b', L'\u732c', L'\u732e', L'\u7331'
	, L'\u7334', L'\u7337', L'\u7338', L'\u7339', L'\u733e', L'\u733f', L'\u734d', L'\u7350', L'\u7352', L'\u7357'
	, L'\u7360', L'\u736c', L'\u736d', L'\u736f', L'\u737e', L'\u7384', L'\u7387', L'\u7389', L'\u738b', L'\u738e'
	, L'\u7391', L'\u7396', L'\u739b', L'\u739f', L'\u73a2', L'\u73a9', L'\u73ab', L'\u73ae', L'\u73af', L'\u73b0'
	, L'\u73b2', L'\u73b3', L'\u73b7', L'\u73ba', L'\u73bb', L'\u73c0', L'\u73c2', L'\u73c8', L'\u73c9', L'\u73ca'
	, L'\u73cd', L'\u73cf', L'\u73d0', L'\u73d1', L'\u73d9', L'\u73de', L'\u73e0', L'\u73e5', L'\u73e7', L'\u73e9'
	, L'\u73ed', L'\u73f2', L'\u7403', L'\u7405', L'\u7406', L'\u7409', L'\u740a', L'\u740f', L'\u7410', L'\u741a'
	, L'\u741b', L'\u7422', L'\u7425', L'\u7426', L'\u7428', L'\u742a', L'\u742c', L'\u742e', L'\u7430', L'\u7433'
	, L'\u7434', L'\u7435', L'\u7436', L'\u743c', L'\u7441', L'\u7455', L'\u7457', L'\u7459', L'\u745a', L'\u745b'
	, L'\u745c', L'\u745e', L'\u745f', L'\u746d', L'\u7470', L'\u7476', L'\u7477', L'\u747e', L'\u7480', L'\u7481'
	, L'\u7483', L'\u7487', L'\u748b', L'\u748e', L'\u7490', L'\u749c', L'\u749e', L'\u74a7', L'\u74a8', L'\u74a9'
	, L'\u74ba', L'\u74d2', L'\u74dc', L'\u74de', L'\u74e0', L'\u74e2', L'\u74e3', L'\u74e4', L'\u74e6', L'\u74ee'
	, L'\u74ef', L'\u74f4', L'\u74f6', L'\u74f7', L'\u74ff', L'\u7504', L'\u750d', L'\u750f', L'\u7511', L'\u7513'
	, L'\u7518', L'\u7519', L'\u751a', L'\u751c', L'\u751f', L'\u7525', L'\u7528', L'\u7529', L'\u752b', L'\u752c'
	, L'\u752d', L'\u752f', L'\u7530', L'\u7531', L'\u7532', L'\u7533', L'\u7535', L'\u7537', L'\u7538', L'\u753a'
	, L'\u753b', L'\u753e', L'\u7540', L'\u7545', L'\u7548', L'\u754b', L'\u754c', L'\u754e', L'\u754f', L'\u7554'
	, L'\u7559', L'\u755a', L'\u755b', L'\u755c', L'\u7565', L'\u7566', L'\u756a', L'\u7572', L'\u7574', L'\u7578'
	, L'\u7579', L'\u757f', L'\u7583', L'\u7586', L'\u758b', L'\u758f', L'\u7591', L'\u7592', L'\u7594', L'\u7596'
	, L'\u7597', L'\u7599', L'\u759a', L'\u759d', L'\u759f', L'\u75a0', L'\u75a1', L'\u75a3', L'\u75a4', L'\u75a5'
	, L'\u75ab', L'\u75ac', L'\u75ae', L'\u75af', L'\u75b0', L'\u75b1', L'\u75b2', L'\u75b3', L'\u75b4', L'\u75b5'
	, L'\u75b8', L'\u75b9', L'\u75bc', L'\u75bd', L'\u75be', L'\u75c2', L'\u75c3', L'\u75c4', L'\u75c5', L'\u75c7'
	, L'\u75c8', L'\u75c9', L'\u75ca', L'\u75cd', L'\u75d2', L'\u75d4', L'\u75d5', L'\u75d6', L'\u75d8', L'\u75db'
	, L'\u75de', L'\u75e2', L'\u75e3', L'\u75e4', L'\u75e6', L'\u75e7', L'\u75e8', L'\u75ea', L'\u75eb', L'\u75f0'
	, L'\u75f1', L'\u75f4', L'\u75f9', L'\u75fc', L'\u75ff', L'\u7600', L'\u7601', L'\u7603', L'\u7605', L'\u760a'
	, L'\u760c', L'\u7610', L'\u7615', L'\u7617', L'\u7618', L'\u7619', L'\u761b', L'\u761f', L'\u7620', L'\u7622'
	, L'\u7624', L'\u7625', L'\u7626', L'\u7629', L'\u762a', L'\u762b', L'\u762d', L'\u7630', L'\u7633', L'\u7634'
	, L'\u7635', L'\u7638', L'\u763c', L'\u763e', L'\u763f', L'\u7640', L'\u7643', L'\u764c', L'\u764d', L'\u7654'
	, L'\u7656', L'\u765c', L'\u765e', L'\u7663', L'\u766b', L'\u766f', L'\u7678', L'\u767b', L'\u767d', L'\u767e'
	, L'\u7682', L'\u7684', L'\u7686', L'\u7687', L'\u7688', L'\u768b', L'\u768e', L'\u7691', L'\u7693', L'\u7696'
	, L'\u7699', L'\u76a4', L'\u76ae', L'\u76b1', L'\u76b2', L'\u76b4', L'\u76bf', L'\u76c2', L'\u76c5', L'\u76c6'
	, L'\u76c8', L'\u76ca', L'\u76cd', L'\u76ce', L'\u76cf', L'\u76d0', L'\u76d1', L'\u76d2', L'\u76d4', L'\u76d6'
	, L'\u76d7', L'\u76d8', L'\u76db', L'\u76df', L'\u76e5', L'\u76ee', L'\u76ef', L'\u76f1', L'\u76f2', L'\u76f4'
	, L'\u76f8', L'\u76f9', L'\u76fc', L'\u76fe', L'\u7701', L'\u7704', L'\u7707', L'\u7708', L'\u7709', L'\u770b'
	, L'\u770d', L'\u7719', L'\u771a', L'\u771f', L'\u7720', L'\u7722', L'\u7726', L'\u7728', L'\u7729', L'\u772d'
	, L'\u772f', L'\u7735', L'\u7736', L'\u7737', L'\u7738', L'\u773a', L'\u773c', L'\u7740', L'\u7741', L'\u7743'
	, L'\u7747', L'\u7750', L'\u7751', L'\u775a', L'\u775b', L'\u7761', L'\u7762', L'\u7763', L'\u7765', L'\u7766'
	, L'\u7768', L'\u776b', L'\u776c', L'\u7779', L'\u777d', L'\u777e', L'\u777f', L'\u7780', L'\u7784', L'\u7785'
	, L'\u778c', L'\u778d', L'\u778e', L'\u7791', L'\u7792', L'\u779f', L'\u77a0', L'\u77a2', L'\u77a5', L'\u77a7'
	, L'\u77a9', L'\u77aa', L'\u77ac', L'\u77b0', L'\u77b3', L'\u77b5', L'\u77bb', L'\u77bd', L'\u77bf', L'\u77cd'
	, L'\u77d7', L'\u77db', L'\u77dc', L'\u77e2', L'\u77e3', L'\u77e5', L'\u77e7', L'\u77e9', L'\u77eb', L'\u77ec'
	, L'\u77ed', L'\u77ee', L'\u77f3', L'\u77f6', L'\u77f8', L'\u77fd', L'\u77fe', L'\u77ff', L'\u7800', L'\u7801'
	, L'\u7802', L'\u7809', L'\u780c', L'\u780d', L'\u7811', L'\u7812', L'\u7814', L'\u7816', L'\u7817', L'\u7818'
	, L'\u781a', L'\u781c', L'\u781d', L'\u781f', L'\u7823', L'\u7825', L'\u7826', L'\u7827', L'\u7829', L'\u782c'
	, L'\u782d', L'\u7830', L'\u7834', L'\u7837', L'\u7838', L'\u7839', L'\u783a', L'\u783b', L'\u783c', L'\u783e'
	, L'\u7840', L'\u7845', L'\u7847', L'\u784c', L'\u784e', L'\u7850', L'\u7852', L'\u7855', L'\u7856', L'\u7857'
	, L'\u785d', L'\u786a', L'\u786b', L'\u786c', L'\u786d', L'\u786e', L'\u7877', L'\u787c', L'\u7887', L'\u7889'
	, L'\u788c', L'\u788d', L'\u788e', L'\u7891', L'\u7893', L'\u7897', L'\u7898', L'\u789a', L'\u789b', L'\u789c'
	, L'\u789f', L'\u78a1', L'\u78a3', L'\u78a5', L'\u78a7', L'\u78b0', L'\u78b1', L'\u78b2', L'\u78b3', L'\u78b4'
	, L'\u78b9', L'\u78be', L'\u78c1', L'\u78c5', L'\u78c9', L'\u78ca', L'\u78cb', L'\u78d0', L'\u78d4', L'\u78d5'
	, L'\u78d9', L'\u78e8', L'\u78ec', L'\u78f2', L'\u78f4', L'\u78f7', L'\u78fa', L'\u7901', L'\u7905', L'\u7913'
	, L'\u791e', L'\u7924', L'\u7934', L'\u793a', L'\u793b', L'\u793c', L'\u793e', L'\u7940', L'\u7941', L'\u7946'
	, L'\u7948', L'\u7949', L'\u7953', L'\u7956', L'\u7957', L'\u795a', L'\u795b', L'\u795c', L'\u795d', L'\u795e'
	, L'\u795f', L'\u7960', L'\u7962', L'\u7965', L'\u7967', L'\u7968', L'\u796d', L'\u796f', L'\u7977', L'\u7978'
	, L'\u797a', L'\u7980', L'\u7981', L'\u7984', L'\u7985', L'\u798a', L'\u798f', L'\u799a', L'\u79a7', L'\u79b3'
	, L'\u79b9', L'\u79ba', L'\u79bb', L'\u79bd', L'\u79be', L'\u79c0', L'\u79c1', L'\u79c3', L'\u79c6', L'\u79c9'
	, L'\u79cb', L'\u79cd', L'\u79d1', L'\u79d2', L'\u79d5', L'\u79d8', L'\u79df', L'\u79e3', L'\u79e4', L'\u79e6'
	, L'\u79e7', L'\u79e9', L'\u79eb', L'\u79ed', L'\u79ef', L'\u79f0', L'\u79f8', L'\u79fb', L'\u79fd', L'\u7a00'
	, L'\u7a02', L'\u7a03', L'\u7a06', L'\u7a0b', L'\u7a0d', L'\u7a0e', L'\u7a14', L'\u7a17', L'\u7a1a', L'\u7a1e'
	, L'\u7a20', L'\u7a23', L'\u7a33', L'\u7a37', L'\u7a39', L'\u7a3b', L'\u7a3c', L'\u7a3d', L'\u7a3f', L'\u7a46'
	, L'\u7a51', L'\u7a57', L'\u7a70', L'\u7a74', L'\u7a76', L'\u7a77', L'\u7a78', L'\u7a79', L'\u7a7a', L'\u7a7f'
	, L'\u7a80', L'\u7a81', L'\u7a83', L'\u7a84', L'\u7a86', L'\u7a88', L'\u7a8d', L'\u7a91', L'\u7a92', L'\u7a95'
	, L'\u7a96', L'\u7a97', L'\u7a98', L'\u7a9c', L'\u7a9d', L'\u7a9f', L'\u7aa0', L'\u7aa5', L'\u7aa6', L'\u7aa8'
	, L'\u7aac', L'\u7aad', L'\u7ab3', L'\u7abf', L'\u7acb', L'\u7ad6', L'\u7ad9', L'\u7ade', L'\u7adf', L'\u7ae0'
	, L'\u7ae3', L'\u7ae5', L'\u7ae6', L'\u7aed', L'\u7aef', L'\u7af9', L'\u7afa', L'\u7afd', L'\u7aff', L'\u7b03'
	, L'\u7b04', L'\u7b06', L'\u7b08', L'\u7b0a', L'\u7b0b', L'\u7b0f', L'\u7b11', L'\u7b14', L'\u7b15', L'\u7b19'
	, L'\u7b1b', L'\u7b1e', L'\u7b20', L'\u7b24', L'\u7b25', L'\u7b26', L'\u7b28', L'\u7b2a', L'\u7b2b', L'\u7b2c'
	, L'\u7b2e', L'\u7b31', L'\u7b33', L'\u7b38', L'\u7b3a', L'\u7b3c', L'\u7b3e', L'\u7b45', L'\u7b47', L'\u7b49'
	, L'\u7b4b', L'\u7b4c', L'\u7b4f', L'\u7b50', L'\u7b51', L'\u7b52', L'\u7b54', L'\u7b56', L'\u7b58', L'\u7b5a'
	, L'\u7b5b', L'\u7b5d', L'\u7b60', L'\u7b62', L'\u7b6e', L'\u7b71', L'\u7b72', L'\u7b75', L'\u7b77', L'\u7b79'
	, L'\u7b7b', L'\u7b7e', L'\u7b80', L'\u7b85', L'\u7b8d', L'\u7b90', L'\u7b94', L'\u7b95', L'\u7b97', L'\u7b9c'
	, L'\u7b9d', L'\u7ba1', L'\u7ba2', L'\u7ba6', L'\u7ba7', L'\u7ba8', L'\u7ba9', L'\u7baa', L'\u7bab', L'\u7bac'
	, L'\u7bad', L'\u7bb1', L'\u7bb4', L'\u7bb8', L'\u7bc1', L'\u7bc6', L'\u7bc7', L'\u7bcc', L'\u7bd1', L'\u7bd3'
	, L'\u7bd9', L'\u7bda', L'\u7bdd', L'\u7be1', L'\u7be5', L'\u7be6', L'\u7bea', L'\u7bee', L'\u7bf1', L'\u7bf7'
	, L'\u7bfc', L'\u7bfe', L'\u7c07', L'\u7c0b', L'\u7c0c', L'\u7c0f', L'\u7c16', L'\u7c1f', L'\u7c26', L'\u7c27'
	, L'\u7c2a', L'\u7c38', L'\u7c3f', L'\u7c40', L'\u7c41', L'\u7c4d', L'\u7c73', L'\u7c74', L'\u7c7b', L'\u7c7c'
	, L'\u7c7d', L'\u7c89', L'\u7c91', L'\u7c92', L'\u7c95', L'\u7c97', L'\u7c98', L'\u7c9c', L'\u7c9d', L'\u7c9e'
	, L'\u7c9f', L'\u7ca2', L'\u7ca4', L'\u7ca5', L'\u7caa', L'\u7cae', L'\u7cb1', L'\u7cb2', L'\u7cb3', L'\u7cb9'
	, L'\u7cbc', L'\u7cbd', L'\u7cbe', L'\u7cc1', L'\u7cc5', L'\u7cc7', L'\u7cc8', L'\u7cca', L'\u7ccc', L'\u7ccd'
	, L'\u7cd5', L'\u7cd6', L'\u7cd7', L'\u7cd9', L'\u7cdc', L'\u7cdf', L'\u7ce0', L'\u7ce8', L'\u7cef', L'\u7cf8'
	, L'\u7cfb', L'\u7d0a', L'\u7d20', L'\u7d22', L'\u7d27', L'\u7d2b', L'\u7d2f', L'\u7d6e', L'\u7d77', L'\u7da6'
	, L'\u7dae', L'\u7e3b', L'\u7e41', L'\u7e47', L'\u7e82', L'\u7e9b', L'\u7e9f', L'\u7ea0', L'\u7ea1', L'\u7ea2'
	, L'\u7ea3', L'\u7ea4', L'\u7ea5', L'\u7ea6', L'\u7ea7', L'\u7ea8', L'\u7ea9', L'\u7eaa', L'\u7eab', L'\u7eac'
	, L'\u7ead', L'\u7eaf', L'\u7eb0', L'\u7eb1', L'\u7eb2', L'\u7eb3', L'\u7eb5', L'\u7eb6', L'\u7eb7', L'\u7eb8'
	, L'\u7eb9', L'\u7eba', L'\u7ebd', L'\u7ebe', L'\u7ebf', L'\u7ec0', L'\u7ec1', L'\u7ec2', L'\u7ec3', L'\u7ec4'
	, L'\u7ec5', L'\u7ec6', L'\u7ec7', L'\u7ec8', L'\u7ec9', L'\u7eca', L'\u7ecb', L'\u7ecc', L'\u7ecd', L'\u7ece'
	, L'\u7ecf', L'\u7ed0', L'\u7ed1', L'\u7ed2', L'\u7ed3', L'\u7ed4', L'\u7ed5', L'\u7ed7', L'\u7ed8', L'\u7ed9'
	, L'\u7eda', L'\u7edb', L'\u7edc', L'\u7edd', L'\u7ede', L'\u7edf', L'\u7ee0', L'\u7ee1', L'\u7ee2', L'\u7ee3'
	, L'\u7ee5', L'\u7ee6', L'\u7ee7', L'\u7ee8', L'\u7ee9', L'\u7eea', L'\u7eeb', L'\u7eed', L'\u7eee', L'\u7eef'
	, L'\u7ef0', L'\u7ef1', L'\u7ef2', L'\u7ef3', L'\u7ef4', L'\u7ef5', L'\u7ef6', L'\u7ef7', L'\u7ef8', L'\u7efa'
	, L'\u7efb', L'\u7efc', L'\u7efd', L'\u7efe', L'\u7eff', L'\u7f00', L'\u7f01', L'\u7f02', L'\u7f03', L'\u7f04'
	, L'\u7f05', L'\u7f06', L'\u7f07', L'\u7f08', L'\u7f09', L'\u7f0b', L'\u7f0c', L'\u7f0d', L'\u7f0e', L'\u7f0f'
	, L'\u7f11', L'\u7f12', L'\u7f13', L'\u7f14', L'\u7f15', L'\u7f16', L'\u7f17', L'\u7f18', L'\u7f19', L'\u7f1a'
	, L'\u7f1b', L'\u7f1c', L'\u7f1d', L'\u7f1f', L'\u7f20', L'\u7f21', L'\u7f22', L'\u7f23', L'\u7f24', L'\u7f25'
	, L'\u7f26', L'\u7f27', L'\u7f28', L'\u7f29', L'\u7f2a', L'\u7f2b', L'\u7f2c', L'\u7f2d', L'\u7f2e', L'\u7f2f'
	, L'\u7f30', L'\u7f31', L'\u7f32', L'\u7f33', L'\u7f34', L'\u7f35', L'\u7f36', L'\u7f38', L'\u7f3a', L'\u7f42'
	, L'\u7f44', L'\u7f45', L'\u7f50', L'\u7f51', L'\u7f54', L'\u7f55', L'\u7f57', L'\u7f58', L'\u7f5a', L'\u7f5f'
	, L'\u7f61', L'\u7f62', L'\u7f68', L'\u7f69', L'\u7f6a', L'\u7f6e', L'\u7f71', L'\u7f72', L'\u7f74', L'\u7f79'
	, L'\u7f7e', L'\u7f81', L'\u7f8a', L'\u7f8c', L'\u7f8e', L'\u7f94', L'\u7f9a', L'\u7f9d', L'\u7f9e', L'\u7f9f'
	, L'\u7fa1', L'\u7fa4', L'\u7fa7', L'\u7faf', L'\u7fb0', L'\u7fb2', L'\u7fb8', L'\u7fb9', L'\u7fbc', L'\u7fbd'
	, L'\u7fbf', L'\u7fc1', L'\u7fc5', L'\u7fca', L'\u7fcc', L'\u7fce', L'\u7fd4', L'\u7fd5', L'\u7fd8', L'\u7fdf'
	, L'\u7fe0', L'\u7fe1', L'\u7fe5', L'\u7fe6', L'\u7fe9', L'\u7fee', L'\u7ff0', L'\u7ff1', L'\u7ff3', L'\u7ffb'
	, L'\u7ffc', L'\u8000', L'\u8001', L'\u8003', L'\u8004', L'\u8005', L'\u8006', L'\u800b', L'\u800c', L'\u800d'
	, L'\u8010', L'\u8012', L'\u8014', L'\u8015', L'\u8016', L'\u8017', L'\u8018', L'\u8019', L'\u801c', L'\u8020'
	, L'\u8022', L'\u8025', L'\u8026', L'\u8027', L'\u8028', L'\u8029', L'\u802a', L'\u8031', L'\u8033', L'\u8035'
	, L'\u8036', L'\u8037', L'\u8038', L'\u803b', L'\u803d', L'\u803f', L'\u8042', L'\u8043', L'\u8046', L'\u804a'
	, L'\u804b', L'\u804c', L'\u804d', L'\u8052', L'\u8054', L'\u8058', L'\u805a', L'\u8069', L'\u806a', L'\u8071'
	, L'\u807f', L'\u8080', L'\u8083', L'\u8084', L'\u8086', L'\u8087', L'\u8089', L'\u808b', L'\u808c', L'\u8093'
	, L'\u8096', L'\u8098', L'\u809a', L'\u809b', L'\u809c', L'\u809d', L'\u809f', L'\u80a0', L'\u80a1', L'\u80a2'
	, L'\u80a4', L'\u80a5', L'\u80a9', L'\u80aa', L'\u80ab', L'\u80ad', L'\u80ae', L'\u80af', L'\u80b1', L'\u80b2'
	, L'\u80b4', L'\u80b7', L'\u80ba', L'\u80bc', L'\u80bd', L'\u80be', L'\u80bf', L'\u80c0', L'\u80c1', L'\u80c2'
	, L'\u80c3', L'\u80c4', L'\u80c6', L'\u80cc', L'\u80cd', L'\u80ce', L'\u80d6', L'\u80d7', L'\u80d9', L'\u80da'
	, L'\u80db', L'\u80dc', L'\u80dd', L'\u80de', L'\u80e1', L'\u80e4', L'\u80e5', L'\u80e7', L'\u80e8', L'\u80e9'
	, L'\u80ea', L'\u80eb', L'\u80ec', L'\u80ed', L'\u80ef', L'\u80f0', L'\u80f1', L'\u80f2', L'\u80f3', L'\u80f4'
	, L'\u80f6', L'\u80f8', L'\u80fa', L'\u80fc', L'\u80fd', L'\u8102', L'\u8106', L'\u8109', L'\u810a', L'\u810d'
	, L'\u810e', L'\u810f', L'\u8110', L'\u8111', L'\u8112', L'\u8113', L'\u8114', L'\u8116', L'\u8118', L'\u811a'
	, L'\u811e', L'\u812c', L'\u812f', L'\u8131', L'\u8132', L'\u8136', L'\u8138', L'\u813e', L'\u8146', L'\u8148'
	, L'\u814a', L'\u814b', L'\u814c', L'\u8150', L'\u8151', L'\u8153', L'\u8154', L'\u8155', L'\u8159', L'\u815a'
	, L'\u8160', L'\u8165', L'\u8167', L'\u8169', L'\u816d', L'\u816e', L'\u8170', L'\u8171', L'\u8174', L'\u8179'
	, L'\u817a', L'\u817b', L'\u817c', L'\u817d', L'\u817e', L'\u817f', L'\u8180', L'\u8182', L'\u8188', L'\u818a'
	, L'\u818f', L'\u8191', L'\u8198', L'\u819b', L'\u819c', L'\u819d', L'\u81a3', L'\u81a6', L'\u81a8', L'\u81aa'
	, L'\u81b3', L'\u81ba', L'\u81bb', L'\u81c0', L'\u81c1', L'\u81c2', L'\u81c3', L'\u81c6', L'\u81ca', L'\u81cc'
	, L'\u81e3', L'\u81e7', L'\u81ea', L'\u81ec', L'\u81ed', L'\u81f3', L'\u81f4', L'\u81fb', L'\u81fc', L'\u81fe'
	, L'\u8200', L'\u8201', L'\u8202', L'\u8204', L'\u8205', L'\u8206', L'\u820c', L'\u820d', L'\u8210', L'\u8212'
	, L'\u8214', L'\u821b', L'\u821c', L'\u821e', L'\u821f', L'\u8221', L'\u8222', L'\u8223', L'\u8228', L'\u822a'
	, L'\u822b', L'\u822c', L'\u822d', L'\u822f', L'\u8230', L'\u8231', L'\u8233', L'\u8234', L'\u8235', L'\u8236'
	, L'\u8237', L'\u8238', L'\u8239', L'\u823b', L'\u823e', L'\u8244', L'\u8247', L'\u8249', L'\u824b', L'\u824f'
	, L'\u8258', L'\u825a', L'\u825f', L'\u8268', L'\u826e', L'\u826f', L'\u8270', L'\u8272', L'\u8273', L'\u8274'
	, L'\u8279', L'\u827a', L'\u827d', L'\u827e', L'\u827f', L'\u8282', L'\u8284', L'\u8288', L'\u828a', L'\u828b'
	, L'\u828d', L'\u828e', L'\u828f', L'\u8291', L'\u8292', L'\u8297', L'\u8298', L'\u8299', L'\u829c', L'\u829d'
	, L'\u829f', L'\u82a1', L'\u82a4', L'\u82a5', L'\u82a6', L'\u82a8', L'\u82a9', L'\u82aa', L'\u82ab', L'\u82ac'
	, L'\u82ad', L'\u82ae', L'\u82af', L'\u82b0', L'\u82b1', L'\u82b3', L'\u82b4', L'\u82b7', L'\u82b8', L'\u82b9'
	, L'\u82bd', L'\u82be', L'\u82c1', L'\u82c4', L'\u82c7', L'\u82c8', L'\u82ca', L'\u82cb', L'\u82cc', L'\u82cd'
	, L'\u82ce', L'\u82cf', L'\u82d1', L'\u82d2', L'\u82d3', L'\u82d4', L'\u82d5', L'\u82d7', L'\u82d8', L'\u82db'
	, L'\u82dc', L'\u82de', L'\u82df', L'\u82e0', L'\u82e1', L'\u82e3', L'\u82e4', L'\u82e5', L'\u82e6', L'\u82eb'
	, L'\u82ef', L'\u82f1', L'\u82f4', L'\u82f7', L'\u82f9', L'\u82fb', L'\u8301', L'\u8302', L'\u8303', L'\u8304'
	, L'\u8305', L'\u8306', L'\u8307', L'\u8308', L'\u8309', L'\u830c', L'\u830e', L'\u830f', L'\u8311', L'\u8314'
	, L'\u8315', L'\u8317', L'\u831a', L'\u831b', L'\u831c', L'\u8327', L'\u8328', L'\u832b', L'\u832c', L'\u832d'
	, L'\u832f', L'\u8331', L'\u8333', L'\u8334', L'\u8335', L'\u8336', L'\u8338', L'\u8339', L'\u833a', L'\u833c'
	, L'\u8340', L'\u8343', L'\u8346', L'\u8347', L'\u8349', L'\u834f', L'\u8350', L'\u8351', L'\u8352', L'\u8354'
	, L'\u835a', L'\u835b', L'\u835c', L'\u835e', L'\u835f', L'\u8360', L'\u8361', L'\u8363', L'\u8364', L'\u8365'
	, L'\u8366', L'\u8367', L'\u8368', L'\u8369', L'\u836a', L'\u836b', L'\u836c', L'\u836d', L'\u836e', L'\u836f'
	, L'\u8377', L'\u8378', L'\u837b', L'\u837c', L'\u837d', L'\u8385', L'\u8386', L'\u8389', L'\u838e', L'\u8392'
	, L'\u8393', L'\u8398', L'\u839b', L'\u839c', L'\u839e', L'\u83a0', L'\u83a8', L'\u83a9', L'\u83aa', L'\u83ab'
	, L'\u83b0', L'\u83b1', L'\u83b2', L'\u83b3', L'\u83b4', L'\u83b6', L'\u83b7', L'\u83b8', L'\u83b9', L'\u83ba'
	, L'\u83bc', L'\u83bd', L'\u83c0', L'\u83c1', L'\u83c5', L'\u83c7', L'\u83ca', L'\u83cc', L'\u83cf', L'\u83d4'
	, L'\u83d6', L'\u83d8', L'\u83dc', L'\u83dd', L'\u83df', L'\u83e0', L'\u83e1', L'\u83e5', L'\u83e9', L'\u83ea'
	, L'\u83f0', L'\u83f1', L'\u83f2', L'\u83f8', L'\u83f9', L'\u83fd', L'\u8401', L'\u8403', L'\u8404', L'\u8406'
	, L'\u840b', L'\u840c', L'\u840d', L'\u840e', L'\u840f', L'\u8411', L'\u8418', L'\u841c', L'\u841d', L'\u8424'
	, L'\u8425', L'\u8426', L'\u8427', L'\u8428', L'\u8431', L'\u8438', L'\u843c', L'\u843d', L'\u8446', L'\u8451'
	, L'\u8457', L'\u8459', L'\u845a', L'\u845b', L'\u845c', L'\u8461', L'\u8463', L'\u8469', L'\u846b', L'\u846c'
	, L'\u846d', L'\u8471', L'\u8473', L'\u8475', L'\u8476', L'\u8478', L'\u847a', L'\u8482', L'\u8487', L'\u8488'
	, L'\u8489', L'\u848b', L'\u848c', L'\u848e', L'\u8497', L'\u8499', L'\u849c', L'\u84a1', L'\u84af', L'\u84b2'
	, L'\u84b4', L'\u84b8', L'\u84b9', L'\u84ba', L'\u84bd', L'\u84bf', L'\u84c1', L'\u84c4', L'\u84c9', L'\u84ca'
	, L'\u84cd', L'\u84d0', L'\u84d1', L'\u84d3', L'\u84d6', L'\u84dd', L'\u84df', L'\u84e0', L'\u84e3', L'\u84e5'
	, L'\u84e6', L'\u84ec', L'\u84f0', L'\u84fc', L'\u84ff', L'\u850c', L'\u8511', L'\u8513', L'\u8517', L'\u851a'
	, L'\u851f', L'\u8521', L'\u852b', L'\u852c', L'\u8537', L'\u8538', L'\u8539', L'\u853a', L'\u853b', L'\u853c'
	, L'\u853d', L'\u8543', L'\u8548', L'\u8549', L'\u854a', L'\u8556', L'\u8559', L'\u855e', L'\u8564', L'\u8568'
	, L'\u8572', L'\u8574', L'\u8579', L'\u857a', L'\u857b', L'\u857e', L'\u8584', L'\u8585', L'\u8587', L'\u858f'
	, L'\u859b', L'\u859c', L'\u85a4', L'\u85a8', L'\u85aa', L'\u85ae', L'\u85af', L'\u85b0', L'\u85b7', L'\u85b9'
	, L'\u85c1', L'\u85c9', L'\u85cf', L'\u85d0', L'\u85d3', L'\u85d5', L'\u85dc', L'\u85e4', L'\u85e9', L'\u85fb'
	, L'\u85ff', L'\u8605', L'\u8611', L'\u8616', L'\u8627', L'\u8629', L'\u8638', L'\u863c', L'\u864d', L'\u864e'
	, L'\u864f', L'\u8650', L'\u8651', L'\u8654', L'\u865a', L'\u865e', L'\u8662', L'\u866b', L'\u866c', L'\u866e'
	, L'\u8671', L'\u8679', L'\u867a', L'\u867b', L'\u867c', L'\u867d', L'\u867e', L'\u867f', L'\u8680', L'\u8681'
	, L'\u8682', L'\u868a', L'\u868b', L'\u868c', L'\u868d', L'\u8693', L'\u8695', L'\u869c', L'\u869d', L'\u86a3'
	, L'\u86a4', L'\u86a7', L'\u86a8', L'\u86a9', L'\u86aa', L'\u86ac', L'\u86af', L'\u86b0', L'\u86b1', L'\u86b4'
	, L'\u86b5', L'\u86b6', L'\u86ba', L'\u86c0', L'\u86c4', L'\u86c6', L'\u86c7', L'\u86c9', L'\u86ca', L'\u86cb'
	, L'\u86ce', L'\u86cf', L'\u86d0', L'\u86d1', L'\u86d4', L'\u86d8', L'\u86d9', L'\u86db', L'\u86de', L'\u86df'
	, L'\u86e4', L'\u86e9', L'\u86ed', L'\u86ee', L'\u86f0', L'\u86f1', L'\u86f2', L'\u86f3', L'\u86f4', L'\u86f8'
	, L'\u86f9', L'\u86fe', L'\u8700', L'\u8702', L'\u8703', L'\u8707', L'\u8708', L'\u8709', L'\u870a', L'\u870d'
	, L'\u8712', L'\u8713', L'\u8715', L'\u8717', L'\u8718', L'\u871a', L'\u871c', L'\u871e', L'\u8721', L'\u8722'
	, L'\u8723', L'\u8725', L'\u8729', L'\u872e', L'\u8731', L'\u8734', L'\u8737', L'\u873b', L'\u873e', L'\u873f'
	, L'\u8747', L'\u8748', L'\u8749', L'\u874c', L'\u874e', L'\u8753', L'\u8757', L'\u8759', L'\u8760', L'\u8763'
	, L'\u8764', L'\u8765', L'\u876e', L'\u8770', L'\u8774', L'\u8776', L'\u877b', L'\u877c', L'\u877d', L'\u877e'
	, L'\u8782', L'\u8783', L'\u8785', L'\u8788', L'\u878b', L'\u878d', L'\u8793', L'\u8797', L'\u879f', L'\u87a8'
	, L'\u87ab', L'\u87ac', L'\u87ad', L'\u87af', L'\u87b3', L'\u87b5', L'\u87ba', L'\u87bd', L'\u87c0', L'\u87c6'
	, L'\u87ca', L'\u87cb', L'\u87d1', L'\u87d2', L'\u87d3', L'\u87db', L'\u87e0', L'\u87e5', L'\u87ea', L'\u87ee'
	, L'\u87f9', L'\u87fe', L'\u8803', L'\u880a', L'\u8813', L'\u8815', L'\u8816', L'\u881b', L'\u8821', L'\u8822'
	, L'\u8832', L'\u8839', L'\u883c', L'\u8840', L'\u8844', L'\u8845', L'\u884c', L'\u884d', L'\u8854', L'\u8857'
	, L'\u8859', L'\u8861', L'\u8862', L'\u8863', L'\u8864', L'\u8865', L'\u8868', L'\u8869', L'\u886b', L'\u886c'
	, L'\u886e', L'\u8870', L'\u8872', L'\u8877', L'\u887d', L'\u887e', L'\u887f', L'\u8881', L'\u8882', L'\u8884'
	, L'\u8885', L'\u8888', L'\u888b', L'\u888d', L'\u8892', L'\u8896', L'\u889c', L'\u88a2', L'\u88a4', L'\u88ab'
	, L'\u88ad', L'\u88b1', L'\u88b7', L'\u88bc', L'\u88c1', L'\u88c2', L'\u88c5', L'\u88c6', L'\u88c9', L'\u88ce'
	, L'\u88d2', L'\u88d4', L'\u88d5', L'\u88d8', L'\u88d9', L'\u88df', L'\u88e2', L'\u88e3', L'\u88e4', L'\u88e5'
	, L'\u88e8', L'\u88f0', L'\u88f1', L'\u88f3', L'\u88f4', L'\u88f8', L'\u88f9', L'\u88fc', L'\u88fe', L'\u8902'
	, L'\u890a', L'\u8910', L'\u8912', L'\u8913', L'\u8919', L'\u891a', L'\u891b', L'\u8921', L'\u8925', L'\u892a'
	, L'\u892b', L'\u8930', L'\u8934', L'\u8936', L'\u8941', L'\u8944', L'\u895e', L'\u895f', L'\u8966', L'\u897b'
	, L'\u897f', L'\u8981', L'\u8983', L'\u8986', L'\u89c1', L'\u89c2', L'\u89c4', L'\u89c5', L'\u89c6', L'\u89c7'
	, L'\u89c8', L'\u89c9', L'\u89ca', L'\u89cb', L'\u89cc', L'\u89ce', L'\u89cf', L'\u89d0', L'\u89d1', L'\u89d2'
	, L'\u89d6', L'\u89da', L'\u89dc', L'\u89de', L'\u89e3', L'\u89e5', L'\u89e6', L'\u89eb', L'\u89ef', L'\u89f3'
	, L'\u8a00', L'\u8a07', L'\u8a3e', L'\u8a48', L'\u8a79', L'\u8a89', L'\u8a8a', L'\u8a93', L'\u8b07', L'\u8b26'
	, L'\u8b66', L'\u8b6c', L'\u8ba0', L'\u8ba1', L'\u8ba2', L'\u8ba3', L'\u8ba4', L'\u8ba5', L'\u8ba6', L'\u8ba7'
	, L'\u8ba8', L'\u8ba9', L'\u8baa', L'\u8bab', L'\u8bad', L'\u8bae', L'\u8baf', L'\u8bb0', L'\u8bb2', L'\u8bb3'
	, L'\u8bb4', L'\u8bb5', L'\u8bb6', L'\u8bb7', L'\u8bb8', L'\u8bb9', L'\u8bba', L'\u8bbc', L'\u8bbd', L'\u8bbe'
	, L'\u8bbf', L'\u8bc0', L'\u8bc1', L'\u8bc2', L'\u8bc3', L'\u8bc4', L'\u8bc5', L'\u8bc6', L'\u8bc8', L'\u8bc9'
	, L'\u8bca', L'\u8bcb', L'\u8bcc', L'\u8bcd', L'\u8bce', L'\u8bcf', L'\u8bd1', L'\u8bd2', L'\u8bd3', L'\u8bd4'
	, L'\u8bd5', L'\u8bd6', L'\u8bd7', L'\u8bd8', L'\u8bd9', L'\u8bda', L'\u8bdb', L'\u8bdc', L'\u8bdd', L'\u8bde'
	, L'\u8bdf', L'\u8be0', L'\u8be1', L'\u8be2', L'\u8be3', L'\u8be4', L'\u8be5', L'\u8be6', L'\u8be7', L'\u8be8'
	, L'\u8be9', L'\u8beb', L'\u8bec', L'\u8bed', L'\u8bee', L'\u8bef', L'\u8bf0', L'\u8bf1', L'\u8bf2', L'\u8bf3'
	, L'\u8bf4', L'\u8bf5', L'\u8bf6', L'\u8bf7', L'\u8bf8', L'\u8bf9', L'\u8bfa', L'\u8bfb', L'\u8bfc', L'\u8bfd'
	, L'\u8bfe', L'\u8bff', L'\u8c00', L'\u8c01', L'\u8c02', L'\u8c03', L'\u8c04', L'\u8c05', L'\u8c06', L'\u8c07'
	, L'\u8c08', L'\u8c0a', L'\u8c0b', L'\u8c0c', L'\u8c0d', L'\u8c0e', L'\u8c0f', L'\u8c10', L'\u8c11', L'\u8c12'
	, L'\u8c13', L'\u8c14', L'\u8c15', L'\u8c16', L'\u8c17', L'\u8c18', L'\u8c19', L'\u8c1a', L'\u8c1b', L'\u8c1c'
	, L'\u8c1d', L'\u8c1f', L'\u8c20', L'\u8c21', L'\u8c22', L'\u8c23', L'\u8c24', L'\u8c25', L'\u8c26', L'\u8c27'
	, L'\u8c28', L'\u8c29', L'\u8c2a', L'\u8c2b', L'\u8c2c', L'\u8c2d', L'\u8c2e', L'\u8c2f', L'\u8c30', L'\u8c31'
	, L'\u8c32', L'\u8c33', L'\u8c34', L'\u8c35', L'\u8c36', L'\u8c37', L'\u8c41', L'\u8c46', L'\u8c47', L'\u8c49'
	, L'\u8c4c', L'\u8c55', L'\u8c5a', L'\u8c61', L'\u8c62', L'\u8c6a', L'\u8c6b', L'\u8c73', L'\u8c78', L'\u8c79'
	, L'\u8c7a', L'\u8c82', L'\u8c85', L'\u8c89', L'\u8c8a', L'\u8c8c', L'\u8c94', L'\u8c98', L'\u8d1d', L'\u8d1e'
	, L'\u8d1f', L'\u8d21', L'\u8d22', L'\u8d23', L'\u8d24', L'\u8d25', L'\u8d26', L'\u8d27', L'\u8d28', L'\u8d29'
	, L'\u8d2a', L'\u8d2b', L'\u8d2c', L'\u8d2d', L'\u8d2e', L'\u8d2f', L'\u8d30', L'\u8d31', L'\u8d32', L'\u8d33'
	, L'\u8d34', L'\u8d35', L'\u8d36', L'\u8d37', L'\u8d38', L'\u8d39', L'\u8d3a', L'\u8d3b', L'\u8d3c', L'\u8d3d'
	, L'\u8d3e', L'\u8d3f', L'\u8d40', L'\u8d41', L'\u8d42', L'\u8d43', L'\u8d44', L'\u8d45', L'\u8d46', L'\u8d47'
	, L'\u8d48', L'\u8d49', L'\u8d4a', L'\u8d4b', L'\u8d4c', L'\u8d4d', L'\u8d4e', L'\u8d4f', L'\u8d50', L'\u8d53'
	, L'\u8d54', L'\u8d55', L'\u8d56', L'\u8d58', L'\u8d59', L'\u8d5a', L'\u8d5b', L'\u8d5c', L'\u8d5d', L'\u8d5e'
	, L'\u8d60', L'\u8d61', L'\u8d62', L'\u8d63', L'\u8d64', L'\u8d66', L'\u8d67', L'\u8d6b', L'\u8d6d', L'\u8d70'
	, L'\u8d73', L'\u8d74', L'\u8d75', L'\u8d76', L'\u8d77', L'\u8d81', L'\u8d84', L'\u8d85', L'\u8d8a', L'\u8d8b'
	, L'\u8d91', L'\u8d94', L'\u8d9f', L'\u8da3', L'\u8db1', L'\u8db3', L'\u8db4', L'\u8db5', L'\u8db8', L'\u8dba'
	, L'\u8dbc', L'\u8dbe', L'\u8dbf', L'\u8dc3', L'\u8dc4', L'\u8dc6', L'\u8dcb', L'\u8dcc', L'\u8dce', L'\u8dcf'
	, L'\u8dd1', L'\u8dd6', L'\u8dd7', L'\u8dda', L'\u8ddb', L'\u8ddd', L'\u8dde', L'\u8ddf', L'\u8de3', L'\u8de4'
	, L'\u8de8', L'\u8dea', L'\u8deb', L'\u8dec', L'\u8def', L'\u8df3', L'\u8df5', L'\u8df7', L'\u8df8', L'\u8df9'
	, L'\u8dfa', L'\u8dfb', L'\u8dfd', L'\u8e05', L'\u8e09', L'\u8e0a', L'\u8e0c', L'\u8e0f', L'\u8e14', L'\u8e1d'
	, L'\u8e1e', L'\u8e1f', L'\u8e22', L'\u8e23', L'\u8e29', L'\u8e2a', L'\u8e2c', L'\u8e2e', L'\u8e2f', L'\u8e31'
	, L'\u8e35', L'\u8e39', L'\u8e3a', L'\u8e3d', L'\u8e40', L'\u8e41', L'\u8e42', L'\u8e44', L'\u8e47', L'\u8e48'
	, L'\u8e49', L'\u8e4a', L'\u8e4b', L'\u8e51', L'\u8e52', L'\u8e59', L'\u8e66', L'\u8e69', L'\u8e6c', L'\u8e6d'
	, L'\u8e6f', L'\u8e70', L'\u8e72', L'\u8e74', L'\u8e76', L'\u8e7c', L'\u8e7f', L'\u8e81', L'\u8e85', L'\u8e87'
	, L'\u8e8f', L'\u8e90', L'\u8e94', L'\u8e9c', L'\u8e9e', L'\u8eab', L'\u8eac', L'\u8eaf', L'\u8eb2', L'\u8eba'
	, L'\u8ece', L'\u8f66', L'\u8f67', L'\u8f68', L'\u8f69', L'\u8f6b', L'\u8f6c', L'\u8f6d', L'\u8f6e', L'\u8f6f'
	, L'\u8f70', L'\u8f71', L'\u8f72', L'\u8f73', L'\u8f74', L'\u8f75', L'\u8f76', L'\u8f77', L'\u8f78', L'\u8f79'
	, L'\u8f7a', L'\u8f7b', L'\u8f7c', L'\u8f7d', L'\u8f7e', L'\u8f7f', L'\u8f81', L'\u8f82', L'\u8f83', L'\u8f84'
	, L'\u8f85', L'\u8f86', L'\u8f87', L'\u8f88', L'\u8f89', L'\u8f8a', L'\u8f8b', L'\u8f8d', L'\u8f8e', L'\u8f8f'
	, L'\u8f90', L'\u8f91', L'\u8f93', L'\u8f94', L'\u8f95', L'\u8f96', L'\u8f97', L'\u8f98', L'\u8f99', L'\u8f9a'
	, L'\u8f9b', L'\u8f9c', L'\u8f9e', L'\u8f9f', L'\u8fa3', L'\u8fa8', L'\u8fa9', L'\u8fab', L'\u8fb0', L'\u8fb1'
	, L'\u8fb6', L'\u8fb9', L'\u8fbd', L'\u8fbe', L'\u8fc1', L'\u8fc2', L'\u8fc4', L'\u8fc5', L'\u8fc7', L'\u8fc8'
	, L'\u8fce', L'\u8fd0', L'\u8fd1', L'\u8fd3', L'\u8fd4', L'\u8fd5', L'\u8fd8', L'\u8fd9', L'\u8fdb', L'\u8fdc'
	, L'\u8fdd', L'\u8fde', L'\u8fdf', L'\u8fe2', L'\u8fe4', L'\u8fe5', L'\u8fe6', L'\u8fe8', L'\u8fe9', L'\u8fea'
	, L'\u8feb', L'\u8fed', L'\u8fee', L'\u8ff0', L'\u8ff3', L'\u8ff7', L'\u8ff8', L'\u8ff9', L'\u8ffd', L'\u9000'
	, L'\u9001', L'\u9002', L'\u9003', L'\u9004', L'\u9005', L'\u9006', L'\u9009', L'\u900a', L'\u900b', L'\u900d'
	, L'\u900f', L'\u9010', L'\u9011', L'\u9012', L'\u9014', L'\u9016', L'\u9017', L'\u901a', L'\u901b', L'\u901d'
	, L'\u901e', L'\u901f', L'\u9020', L'\u9021', L'\u9022', L'\u9026', L'\u902d', L'\u902e', L'\u902f', L'\u9035'
	, L'\u9036', L'\u9038', L'\u903b', L'\u903c', L'\u903e', L'\u9041', L'\u9042', L'\u9044', L'\u9047', L'\u904d'
	, L'\u904f', L'\u9050', L'\u9051', L'\u9052', L'\u9053', L'\u9057', L'\u9058', L'\u905b', L'\u9062', L'\u9063'
	, L'\u9065', L'\u9068', L'\u906d', L'\u906e', L'\u9074', L'\u9075', L'\u907d', L'\u907f', L'\u9080', L'\u9082'
	, L'\u9083', L'\u9088', L'\u908b', L'\u9091', L'\u9093', L'\u9095', L'\u9097', L'\u9099', L'\u909b', L'\u909d'
	, L'\u90a1', L'\u90a2', L'\u90a3', L'\u90a6', L'\u90aa', L'\u90ac', L'\u90ae', L'\u90af', L'\u90b0', L'\u90b1'
	, L'\u90b3', L'\u90b4', L'\u90b5', L'\u90b6', L'\u90b8', L'\u90b9', L'\u90ba', L'\u90bb', L'\u90be', L'\u90c1'
	, L'\u90c4', L'\u90c5', L'\u90c7', L'\u90ca', L'\u90ce', L'\u90cf', L'\u90d0', L'\u90d1', L'\u90d3', L'\u90d7'
	, L'\u90db', L'\u90dc', L'\u90dd', L'\u90e1', L'\u90e2', L'\u90e6', L'\u90e7', L'\u90e8', L'\u90eb', L'\u90ed'
	, L'\u90ef', L'\u90f4', L'\u90f8', L'\u90fd', L'\u90fe', L'\u9102', L'\u9104', L'\u9119', L'\u911e', L'\u9122'
	, L'\u9123', L'\u912f', L'\u9131', L'\u9139', L'\u9143', L'\u9146', L'\u9149', L'\u914a', L'\u914b', L'\u914c'
	, L'\u914d', L'\u914e', L'\u914f', L'\u9150', L'\u9152', L'\u9157', L'\u915a', L'\u915d', L'\u915e', L'\u9161'
	, L'\u9162', L'\u9163', L'\u9164', L'\u9165', L'\u9169', L'\u916a', L'\u916c', L'\u916e', L'\u916f', L'\u9170'
	, L'\u9171', L'\u9172', L'\u9174', L'\u9175', L'\u9176', L'\u9177', L'\u9178', L'\u9179', L'\u917d', L'\u917e'
	, L'\u917f', L'\u9185', L'\u9187', L'\u9189', L'\u918b', L'\u918c', L'\u918d', L'\u9190', L'\u9191', L'\u9192'
	, L'\u919a', L'\u919b', L'\u91a2', L'\u91a3', L'\u91aa', L'\u91ad', L'\u91ae', L'\u91af', L'\u91b4', L'\u91b5'
	, L'\u91ba', L'\u91c7', L'\u91c9', L'\u91ca', L'\u91cc', L'\u91cd', L'\u91ce', L'\u91cf', L'\u91d1', L'\u91dc'
	, L'\u9274', L'\u928e', L'\u92ae', L'\u92c8', L'\u933e', L'\u936a', L'\u938f', L'\u93ca', L'\u93d6', L'\u943e'
	, L'\u946b', L'\u9485', L'\u9486', L'\u9487', L'\u9488', L'\u9489', L'\u948a', L'\u948b', L'\u948c', L'\u948d'
	, L'\u948e', L'\u948f', L'\u9490', L'\u9492', L'\u9493', L'\u9494', L'\u9495', L'\u9497', L'\u9499', L'\u949a'
	, L'\u949b', L'\u949c', L'\u949d', L'\u949e', L'\u949f', L'\u94a0', L'\u94a1', L'\u94a2', L'\u94a3', L'\u94a4'
	, L'\u94a5', L'\u94a6', L'\u94a7', L'\u94a8', L'\u94a9', L'\u94aa', L'\u94ab', L'\u94ac', L'\u94ad', L'\u94ae'
	, L'\u94af', L'\u94b0', L'\u94b1', L'\u94b2', L'\u94b3', L'\u94b4', L'\u94b5', L'\u94b6', L'\u94b7', L'\u94b8'
	, L'\u94b9', L'\u94ba', L'\u94bb', L'\u94bc', L'\u94bd', L'\u94be', L'\u94bf', L'\u94c0', L'\u94c1', L'\u94c2'
	, L'\u94c3', L'\u94c4', L'\u94c5', L'\u94c6', L'\u94c8', L'\u94c9', L'\u94ca', L'\u94cb', L'\u94cc', L'\u94cd'
	, L'\u94ce', L'\u94d0', L'\u94d1', L'\u94d2', L'\u94d5', L'\u94d6', L'\u94d7', L'\u94d8', L'\u94d9', L'\u94db'
	, L'\u94dc', L'\u94dd', L'\u94de', L'\u94df', L'\u94e0', L'\u94e1', L'\u94e2', L'\u94e3', L'\u94e4', L'\u94e5'
	, L'\u94e7', L'\u94e8', L'\u94e9', L'\u94ea', L'\u94eb', L'\u94ec', L'\u94ed', L'\u94ee', L'\u94ef', L'\u94f0'
	, L'\u94f1', L'\u94f2', L'\u94f3', L'\u94f4', L'\u94f5', L'\u94f6', L'\u94f7', L'\u94f8', L'\u94f9', L'\u94fa'
	, L'\u94fc', L'\u94fd', L'\u94fe', L'\u94ff', L'\u9500', L'\u9501', L'\u9502', L'\u9503', L'\u9504', L'\u9505'
	, L'\u9506', L'\u9507', L'\u9508', L'\u9509', L'\u950a', L'\u950b', L'\u950c', L'\u950d', L'\u950e', L'\u950f'
	, L'\u9510', L'\u9511', L'\u9512', L'\u9513', L'\u9514', L'\u9515', L'\u9516', L'\u9517', L'\u9518', L'\u9519'
	, L'\u951a', L'\u951b', L'\u951d', L'\u951e', L'\u951f', L'\u9521', L'\u9522', L'\u9523', L'\u9524', L'\u9525'
	, L'\u9526', L'\u9528', L'\u9529', L'\u952a', L'\u952b', L'\u952c', L'\u952d', L'\u952e', L'\u952f', L'\u9530'
	, L'\u9531', L'\u9532', L'\u9534', L'\u9535', L'\u9536', L'\u9537', L'\u9538', L'\u9539', L'\u953a', L'\u953b'
	, L'\u953c', L'\u953e', L'\u953f', L'\u9540', L'\u9541', L'\u9542', L'\u9544', L'\u9545', L'\u9546', L'\u9547'
	, L'\u9549', L'\u954a', L'\u954c', L'\u954d', L'\u954e', L'\u954f', L'\u9550', L'\u9551', L'\u9552', L'\u9553'
	, L'\u9554', L'\u9556', L'\u9557', L'\u9558', L'\u9559', L'\u955b', L'\u955c', L'\u955d', L'\u955e', L'\u955f'
	, L'\u9561', L'\u9562', L'\u9563', L'\u9564', L'\u9565', L'\u9566', L'\u9567', L'\u9568', L'\u9569', L'\u956a'
	, L'\u956b', L'\u956c', L'\u956d', L'\u956f', L'\u9570', L'\u9571', L'\u9572', L'\u9573', L'\u9576', L'\u957f'
	, L'\u95e8', L'\u95e9', L'\u95ea', L'\u95eb', L'\u95ed', L'\u95ee', L'\u95ef', L'\u95f0', L'\u95f1', L'\u95f2'
	, L'\u95f3', L'\u95f4', L'\u95f5', L'\u95f6', L'\u95f7', L'\u95f8', L'\u95f9', L'\u95fa', L'\u95fb', L'\u95fc'
	, L'\u95fd', L'\u95fe', L'\u9600', L'\u9601', L'\u9602', L'\u9603', L'\u9604', L'\u9605', L'\u9606', L'\u9608'
	, L'\u9609', L'\u960a', L'\u960b', L'\u960c', L'\u960d', L'\u960e', L'\u960f', L'\u9610', L'\u9611', L'\u9612'
	, L'\u9614', L'\u9615', L'\u9616', L'\u9617', L'\u9619', L'\u961a', L'\u961c', L'\u961d', L'\u961f', L'\u9621'
	, L'\u9622', L'\u962a', L'\u962e', L'\u9631', L'\u9632', L'\u9633', L'\u9634', L'\u9635', L'\u9636', L'\u963b'
	, L'\u963c', L'\u963d', L'\u963f', L'\u9640', L'\u9642', L'\u9644', L'\u9645', L'\u9646', L'\u9647', L'\u9648'
	, L'\u9649', L'\u964b', L'\u964c', L'\u964d', L'\u9650', L'\u9654', L'\u9655', L'\u965b', L'\u965f', L'\u9661'
	, L'\u9662', L'\u9664', L'\u9667', L'\u9668', L'\u9669', L'\u966a', L'\u966c', L'\u9672', L'\u9674', L'\u9675'
	, L'\u9676', L'\u9677', L'\u9685', L'\u9686', L'\u9688', L'\u968b', L'\u968d', L'\u968f', L'\u9690', L'\u9694'
	, L'\u9697', L'\u9698', L'\u9699', L'\u969c', L'\u96a7', L'\u96b0', L'\u96b3', L'\u96b6', L'\u96b9', L'\u96bc'
	, L'\u96bd', L'\u96be', L'\u96c0', L'\u96c1', L'\u96c4', L'\u96c5', L'\u96c6', L'\u96c7', L'\u96c9', L'\u96cc'
	, L'\u96cd', L'\u96ce', L'\u96cf', L'\u96d2', L'\u96d5', L'\u96e0', L'\u96e8', L'\u96e9', L'\u96ea', L'\u96ef'
	, L'\u96f3', L'\u96f6', L'\u96f7', L'\u96f9', L'\u96fe', L'\u9700', L'\u9701', L'\u9704', L'\u9706', L'\u9707'
	, L'\u9708', L'\u9709', L'\u970d', L'\u970e', L'\u970f', L'\u9713', L'\u9716', L'\u971c', L'\u971e', L'\u972a'
	, L'\u972d', L'\u9730', L'\u9732', L'\u9738', L'\u9739', L'\u973e', L'\u9752', L'\u9753', L'\u9756', L'\u9759'
	, L'\u975b', L'\u975e', L'\u9760', L'\u9761', L'\u9762', L'\u9765', L'\u9769', L'\u9773', L'\u9774', L'\u9776'
	, L'\u977c', L'\u9785', L'\u978b', L'\u978d', L'\u9791', L'\u9792', L'\u9794', L'\u9798', L'\u97a0', L'\u97a3'
	, L'\u97ab', L'\u97ad', L'\u97af', L'\u97b2', L'\u97b4', L'\u97e6', L'\u97e7', L'\u97e9', L'\u97ea', L'\u97eb'
	, L'\u97ec', L'\u97ed', L'\u97f3', L'\u97f5', L'\u97f6', L'\u9875', L'\u9876', L'\u9877', L'\u9878', L'\u9879'
	, L'\u987a', L'\u987b', L'\u987c', L'\u987d', L'\u987e', L'\u987f', L'\u9880', L'\u9881', L'\u9882', L'\u9883'
	, L'\u9884', L'\u9885', L'\u9886', L'\u9887', L'\u9888', L'\u9889', L'\u988a', L'\u988c', L'\u988d', L'\u988f'
	, L'\u9890', L'\u9891', L'\u9893', L'\u9894', L'\u9896', L'\u9897', L'\u9898', L'\u989a', L'\u989b', L'\u989c'
	, L'\u989d', L'\u989e', L'\u989f', L'\u98a0', L'\u98a1', L'\u98a2', L'\u98a4', L'\u98a5', L'\u98a6', L'\u98a7'
	, L'\u98ce', L'\u98d1', L'\u98d2', L'\u98d3', L'\u98d5', L'\u98d8', L'\u98d9', L'\u98da', L'\u98de', L'\u98df'
	, L'\u98e7', L'\u98e8', L'\u990d', L'\u9910', L'\u992e', L'\u9954', L'\u9955', L'\u9963', L'\u9965', L'\u9967'
	, L'\u9968', L'\u9969', L'\u996a', L'\u996b', L'\u996c', L'\u996d', L'\u996e', L'\u996f', L'\u9970', L'\u9971'
	, L'\u9972', L'\u9974', L'\u9975', L'\u9976', L'\u9977', L'\u997a', L'\u997c', L'\u997d', L'\u997f', L'\u9980'
	, L'\u9981', L'\u9984', L'\u9985', L'\u9986', L'\u9987', L'\u9988', L'\u998a', L'\u998b', L'\u998d', L'\u998f'
	, L'\u9990', L'\u9991', L'\u9992', L'\u9993', L'\u9994', L'\u9995', L'\u9996', L'\u9997', L'\u9998', L'\u9999'
	, L'\u99a5', L'\u99a8', L'\u9a6c', L'\u9a6d', L'\u9a6e', L'\u9a6f', L'\u9a70', L'\u9a71', L'\u9a73', L'\u9a74'
	, L'\u9a75', L'\u9a76', L'\u9a77', L'\u9a78', L'\u9a79', L'\u9a7a', L'\u9a7b', L'\u9a7c', L'\u9a7d', L'\u9a7e'
	, L'\u9a7f', L'\u9a80', L'\u9a81', L'\u9a82', L'\u9a84', L'\u9a85', L'\u9a86', L'\u9a87', L'\u9a88', L'\u9a8a'
	, L'\u9a8b', L'\u9a8c', L'\u9a8f', L'\u9a90', L'\u9a91', L'\u9a92', L'\u9a93', L'\u9a96', L'\u9a97', L'\u9a98'
	, L'\u9a9a', L'\u9a9b', L'\u9a9c', L'\u9a9d', L'\u9a9e', L'\u9a9f', L'\u9aa0', L'\u9aa1', L'\u9aa2', L'\u9aa3'
	, L'\u9aa4', L'\u9aa5', L'\u9aa7', L'\u9aa8', L'\u9ab0', L'\u9ab1', L'\u9ab6', L'\u9ab7', L'\u9ab8', L'\u9aba'
	, L'\u9abc', L'\u9ac0', L'\u9ac1', L'\u9ac2', L'\u9ac5', L'\u9acb', L'\u9acc', L'\u9ad1', L'\u9ad3', L'\u9ad8'
	, L'\u9adf', L'\u9ae1', L'\u9ae6', L'\u9aeb', L'\u9aed', L'\u9aef', L'\u9af9', L'\u9afb', L'\u9b03', L'\u9b08'
	, L'\u9b0f', L'\u9b13', L'\u9b1f', L'\u9b23', L'\u9b2f', L'\u9b32', L'\u9b3b', L'\u9b3c', L'\u9b41', L'\u9b42'
	, L'\u9b43', L'\u9b44', L'\u9b45', L'\u9b47', L'\u9b48', L'\u9b49', L'\u9b4d', L'\u9b4f', L'\u9b51', L'\u9b54'
	, L'\u9c7c', L'\u9c7f', L'\u9c81', L'\u9c82', L'\u9c85', L'\u9c86', L'\u9c87', L'\u9c88', L'\u9c8b', L'\u9c8d'
	, L'\u9c8e', L'\u9c90', L'\u9c91', L'\u9c92', L'\u9c94', L'\u9c95', L'\u9c9a', L'\u9c9b', L'\u9c9c', L'\u9c9e'
	, L'\u9c9f', L'\u9ca0', L'\u9ca1', L'\u9ca2', L'\u9ca3', L'\u9ca4', L'\u9ca5', L'\u9ca6', L'\u9ca7', L'\u9ca8'
	, L'\u9ca9', L'\u9cab', L'\u9cad', L'\u9cae', L'\u9cb0', L'\u9cb1', L'\u9cb2', L'\u9cb3', L'\u9cb4', L'\u9cb5'
	, L'\u9cb6', L'\u9cb7', L'\u9cb8', L'\u9cba', L'\u9cbb', L'\u9cbc', L'\u9cbd', L'\u9cc3', L'\u9cc4', L'\u9cc5'
	, L'\u9cc6', L'\u9cc7', L'\u9cca', L'\u9ccb', L'\u9ccc', L'\u9ccd', L'\u9cce', L'\u9ccf', L'\u9cd0', L'\u9cd3'
	, L'\u9cd4', L'\u9cd5', L'\u9cd6', L'\u9cd7', L'\u9cd8', L'\u9cd9', L'\u9cdc', L'\u9cdd', L'\u9cde', L'\u9cdf'
	, L'\u9ce2', L'\u9e1f', L'\u9e20', L'\u9e21', L'\u9e22', L'\u9e23', L'\u9e25', L'\u9e26', L'\u9e28', L'\u9e29'
	, L'\u9e2a', L'\u9e2b', L'\u9e2c', L'\u9e2d', L'\u9e2f', L'\u9e31', L'\u9e32', L'\u9e33', L'\u9e35', L'\u9e36'
	, L'\u9e37', L'\u9e38', L'\u9e39', L'\u9e3a', L'\u9e3d', L'\u9e3e', L'\u9e3f', L'\u9e41', L'\u9e42', L'\u9e43'
	, L'\u9e44', L'\u9e45', L'\u9e46', L'\u9e47', L'\u9e48', L'\u9e49', L'\u9e4a', L'\u9e4b', L'\u9e4c', L'\u9e4e'
	, L'\u9e4f', L'\u9e51', L'\u9e55', L'\u9e57', L'\u9e58', L'\u9e5a', L'\u9e5b', L'\u9e5c', L'\u9e5e', L'\u9e63'
	, L'\u9e64', L'\u9e66', L'\u9e67', L'\u9e68', L'\u9e69', L'\u9e6a', L'\u9e6b', L'\u9e6c', L'\u9e6d', L'\u9e70'
	, L'\u9e71', L'\u9e73', L'\u9e7e', L'\u9e7f', L'\u9e82', L'\u9e87', L'\u9e88', L'\u9e8b', L'\u9e92', L'\u9e93'
	, L'\u9e9d', L'\u9e9f', L'\u9ea6', L'\u9eb4', L'\u9eb8', L'\u9ebb', L'\u9ebd', L'\u9ebe', L'\u9ec4', L'\u9ec9'
	, L'\u9ecd', L'\u9ece', L'\u9ecf', L'\u9ed1', L'\u9ed4', L'\u9ed8', L'\u9edb', L'\u9edc', L'\u9edd', L'\u9edf'
	, L'\u9ee0', L'\u9ee2', L'\u9ee5', L'\u9ee7', L'\u9ee9', L'\u9eea', L'\u9eef', L'\u9ef9', L'\u9efb', L'\u9efc'
	, L'\u9efe', L'\u9f0b', L'\u9f0d', L'\u9f0e', L'\u9f10', L'\u9f13', L'\u9f17', L'\u9f19', L'\u9f20', L'\u9f22'
	, L'\u9f2c', L'\u9f2f', L'\u9f37', L'\u9f39', L'\u9f3b', L'\u9f3d', L'\u9f3e', L'\u9f44', L'\u9f50', L'\u9f51'
	, L'\u9f7f', L'\u9f80', L'\u9f83', L'\u9f84', L'\u9f85', L'\u9f86', L'\u9f87', L'\u9f88', L'\u9f89', L'\u9f8a'
	, L'\u9f8b', L'\u9f8c', L'\u9f99', L'\u9f9a', L'\u9f9b', L'\u9f9f', L'\u9fa0'
};

//static const wchar_t MAX_WIDE_CHAR = 40864;
static const int s_wideCharacterMap[65535] =
{
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,       63,        0,        0,       53,       54,
	      55,       56,       57,       58,       59,       60,       61,       62,        0,        0,
	       0,        0,        0,        0,        0,       27,       28,       29,       30,       31,
	      32,       33,       34,       35,       36,       37,       38,       39,       40,       41,
	      42,       43,       44,       45,       46,       47,       48,       49,       50,       51,
	      52,        0,        0,        0,        0,        0,        0,        1,        2,        3,
	       4,        5,        6,        7,        8,        9,       10,       11,       12,       13,
	      14,       15,       16,       17,       18,       19,       20,       21,       22,       23,
	      24,       25,       26,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,       64,       65,
	       0,       66,        0,        0,        0,       67,       68,       69,       70,       71,
	      72,       73,       74,        0,       75,       76,        0,       77,       78,       79,
	      80,        0,       81,       82,       83,       84,       85,       86,       87,        0,
	       0,        0,       88,        0,       89,       90,        0,       91,       92,        0,
	      93,       94,       95,       96,        0,        0,       97,        0,       98,        0,
	      99,        0,      100,        0,      101,      102,      103,      104,        0,      105,
	     106,      107,        0,        0,        0,      108,        0,      109,        0,      110,
	     111,      112,        0,      113,      114,      115,      116,      117,      118,        0,
	     119,      120,      121,        0,      122,        0,      123,      124,        0,        0,
	     125,      126,      127,      128,      129,      130,        0,        0,        0,        0,
	     131,        0,        0,      132,        0,        0,        0,        0,        0,        0,
	     133,      134,        0,      135,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,      136,        0,        0,        0,        0,        0,
	       0,        0,      137,        0,      138,      139,        0,      140,      141,      142,
	     143,      144,        0,      145,      146,      147,      148,      149,        0,        0,
	     150,        0,      151,      152,        0,        0,        0,      153,      154,      155,
	     156,        0,      157,      158,      159,      160,      161,      162,        0,      163,
	     164,      165,      166,        0,        0,        0,      167,      168,        0,      169,
	       0,        0,        0,        0,      170,      171,        0,        0,        0,      172,
	     173,      174,      175,      176,      177,      178,      179,      180,        0,      181,
	     182,      183,        0,      184,      185,        0,        0,      186,        0,      187,
	     188,      189,      190,      191,      192,      193,        0,        0,        0,      194,
	     195,      196,        0,      197,        0,      198,      199,      200,        0,        0,
	     201,        0,      202,      203,      204,        0,        0,        0,      205,        0,
	     206,      207,        0,      208,      209,      210,        0,        0,        0,      211,
	       0,      212,        0,      213,        0,      214,        0,        0,        0,        0,
	       0,        0,        0,      215,      216,        0,        0,      217,      218,      219,
	     220,      221,        0,        0,        0,        0,        0,      222,      223,      224,
	     225,      226,        0,        0,      227,      228,      229,        0,      230,        0,
	     231,      232,      233,      234,        0,        0,      235,      236,        0,        0,
	       0,      237,      238,        0,      239,        0,      240,        0,      241,        0,
	     242,        0,      243,        0,      244,      245,        0,        0,        0,        0,
	       0,      246,        0,        0,      247,        0,        0,        0,        0,        0,
	       0,      248,      249,      250,      251,      252,        0,      253,        0,      254,
	       0,      255,      256,      257,      258,      259,      260,      261,      262,      263,
	     264,        0,        0,      265,      266,      267,        0,      268,        0,      269,
	       0,        0,      270,        0,        0,      271,      272,        0,        0,      273,
	     274,        0,      275,        0,        0,        0,        0,      276,      277,        0,
	     278,      279,        0,        0,        0,      280,      281,        0,        0,        0,
	     282,      283,        0,      284,        0,      285,        0,      286,        0,      287,
	       0,        0,      288,        0,        0,      289,        0,        0,        0,      290,
	       0,      291,        0,        0,      292,        0,        0,      293,        0,      294,
	     295,      296,      297,      298,      299,        0,      300,        0,      301,      302,
	       0,        0,        0,        0,        0,      303,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,      304,        0,        0,        0,      305,
	     306,      307,        0,        0,        0,        0,      308,        0,        0,        0,
	     309,      310,      311,      312,        0,        0,        0,        0,        0,      313,
	     314,        0,      315,        0,      316,      317,      318,      319,        0,      320,
	       0,      321,        0,        0,      322,        0,      323,      324,      325,        0,
	       0,      326,      327,      328,        0,      329,        0,      330,        0,        0,
	       0,        0,      331,        0,      332,        0,        0,        0,      333,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,      334,      335,        0,      336,        0,        0,      337,        0,
	     338,        0,        0,        0,      339,      340,      341,        0,      342,        0,
	       0,      343,        0,      344,        0,        0,        0,      345,      346,        0,
	     347,      348,      349,        0,      350,      351,      352,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,      353,        0,
	     354,        0,      355,        0,        0,        0,        0,      356,        0,        0,
	       0,      357,      358,        0,        0,        0,      359,        0,      360,      361,
	       0,        0,        0,        0,        0,      362,        0,        0,        0,        0,
	     363,        0,      364,        0,        0,        0,        0,        0,        0,        0,
	       0,      365,        0,        0,        0,        0,        0,        0,      366,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,      367,      368,
	       0,        0,        0,      369,        0,        0,      370,      371,      372,        0,
	       0,        0,        0,      373,        0,        0,      374,        0,        0,        0,
	       0,      375,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,      376,        0,      377,        0,      378,      379,      380,
	       0,        0,      381,        0,        0,        0,        0,        0,      382,        0,
	       0,        0,        0,        0,        0,        0,      383,      384,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,      385,        0,        0,
	       0,        0,        0,        0,      386,        0,        0,        0,      387,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	     388,      389,        0,        0,        0,        0,      390,      391,      392,        0,
	       0,        0,        0,      393,        0,      394,        0,        0,        0,        0,
	       0,      395,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,      396,      397,        0,        0,        0,      398,        0,        0,
	       0,        0,        0,        0,      399,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,      400,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,      401,
	     402,      403,        0,      404,      405,      406,      407,        0,      408,      409,
	       0,      410,        0,      411,        0,        0,        0,      412,        0,        0,
	     413,      414,      415,        0,        0,        0,      416,        0,      417,        0,
	       0,        0,        0,        0,      418,        0,        0,      419,        0,        0,
	     420,        0,        0,      421,      422,      423,      424,        0,      425,      426,
	       0,      427,      428,      429,      430,      431,      432,      433,        0,      434,
	     435,      436,        0,        0,      437,      438,      439,        0,        0,      440,
	       0,        0,      441,      442,        0,        0,      443,      444,        0,        0,
	       0,        0,      445,        0,        0,      446,      447,      448,        0,      449,
	       0,      450,      451,        0,        0,        0,      452,        0,      453,        0,
	     454,      455,        0,        0,        0,        0,        0,      456,      457,        0,
	       0,      458,      459,      460,      461,      462,        0,      463,      464,      465,
	       0,        0,        0,      466,      467,      468,        0,        0,      469,        0,
	       0,        0,      470,        0,      471,      472,        0,      473,        0,      474,
	     475,        0,        0,      476,        0,      477,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,      478,        0,      479,        0,        0,
	     480,      481,        0,        0,      482,        0,        0,        0,        0,        0,
	       0,      483,        0,      484,        0,      485,      486,        0,        0,      487,
	       0,      488,      489,        0,      490,      491,      492,      493,      494,      495,
	       0,      496,      497,      498,      499,      500,        0,        0,      501,      502,
	     503,        0,      504,        0,        0,      505,      506,        0,        0,      507,
	     508,        0,        0,        0,      509,      510,      511,      512,      513,      514,
	       0,      515,        0,        0,      516,        0,        0,        0,      517,        0,
	       0,        0,      518,      519,        0,      520,        0,      521,      522,        0,
	     523,        0,        0,      524,        0,        0,      525,      526,      527,      528,
	     529,      530,        0,      531,        0,      532,      533,      534,      535,      536,
	       0,        0,        0,        0,        0,        0,      537,        0,      538,      539,
	       0,        0,      540,      541,        0,        0,      542,        0,      543,        0,
	       0,        0,        0,        0,      544,        0,      545,        0,        0,      546,
	       0,        0,        0,      547,        0,      548,        0,      549,      550,        0,
	       0,        0,        0,      551,        0,        0,      552,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,      553,        0,      554,
	       0,      555,      556,        0,        0,        0,        0,        0,      557,        0,
	       0,        0,        0,        0,        0,        0,      558,        0,        0,      559,
	       0,        0,        0,        0,        0,        0,        0,      560,        0,      561,
	     562,      563,      564,      565,      566,      567,        0,        0,        0,        0,
	     568,      569,      570,      571,      572,      573,        0,        0,        0,      574,
	     575,      576,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,      577,      578,        0,        0,        0,      579,        0,        0,
	       0,      580,        0,      581,        0,      582,        0,        0,        0,        0,
	     583,        0,      584,        0,        0,        0,      585,        0,      586,        0,
	       0,        0,        0,        0,        0,      587,        0,        0,        0,        0,
	     588,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,      589,        0,        0,        0,        0,        0,        0,        0,
	       0,      590,      591,        0,        0,        0,      592,      593,      594,        0,
	       0,        0,        0,      595,      596,        0,      597,        0,        0,        0,
	       0,      598,        0,      599,      600,        0,        0,        0,        0,      601,
	     602,      603,        0,      604,      605,        0,        0,      606,        0,        0,
	     607,      608,        0,      609,        0,        0,      610,        0,        0,        0,
	     611,        0,        0,        0,      612,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,      613,      614,      615,        0,        0,
	     616,      617,        0,      618,        0,      619,        0,      620,        0,      621,
	     622,      623,      624,        0,        0,        0,      625,      626,        0,      627,
	     628,      629,        0,      630,      631,      632,        0,        0,      633,        0,
	     634,        0,      635,      636,      637,      638,      639,      640,      641,        0,
	     642,      643,        0,      644,        0,      645,        0,        0,      646,      647,
	     648,      649,        0,      650,      651,      652,        0,      653,      654,        0,
	     655,        0,        0,        0,        0,      656,        0,        0,      657,        0,
	     658,      659,      660,        0,        0,      661,        0,      662,      663,      664,
	       0,        0,        0,        0,        0,        0,        0,      665,        0,        0,
	     666,        0,      667,        0,        0,      668,        0,      669,        0,        0,
	     670,      671,        0,      672,      673,        0,      674,      675,        0,        0,
	       0,        0,      676,        0,        0,        0,        0,        0,        0,        0,
	     677,        0,        0,        0,        0,      678,        0,        0,        0,      679,
	       0,      680,      681,        0,        0,        0,        0,        0,      682,      683,
	     684,      685,      686,      687,        0,        0,        0,      688,        0,        0,
	     689,        0,      690,      691,      692,      693,        0,      694,        0,        0,
	       0,      695,      696,        0,        0,      697,      698,      699,      700,        0,
	     701,      702,      703,      704,      705,      706,      707,      708,      709,      710,
	     711,      712,        0,      713,      714,      715,      716,      717,        0,      718,
	     719,      720,        0,        0,        0,      721,        0,      722,      723,        0,
	     724,        0,      725,      726,      727,        0,      728,      729,      730,      731,
	     732,      733,      734,      735,        0,      736,      737,      738,        0,        0,
	       0,      739,        0,      740,      741,      742,      743,      744,        0,      745,
	       0,        0,      746,      747,      748,      749,        0,      750,      751,      752,
	     753,      754,        0,      755,      756,        0,      757,      758,        0,        0,
	     759,      760,        0,      761,      762,        0,      763,        0,      764,        0,
	       0,      765,        0,        0,      766,        0,      767,        0,      768,      769,
	       0,        0,        0,        0,      770,        0,      771,      772,      773,      774,
	     775,      776,      777,      778,        0,      779,      780,        0,        0,        0,
	       0,        0,      781,        0,      782,        0,      783,        0,      784,        0,
	       0,        0,        0,        0,        0,        0,        0,      785,      786,      787,
	       0,      788,      789,      790,      791,        0,        0,      792,      793,      794,
	       0,        0,      795,        0,      796,        0,      797,        0,      798,        0,
	       0,        0,        0,      799,      800,        0,      801,      802,      803,        0,
	     804,        0,      805,      806,      807,        0,        0,      808,      809,      810,
	       0,      811,        0,        0,        0,        0,        0,      812,      813,        0,
	     814,      815,      816,      817,      818,      819,      820,      821,        0,      822,
	       0,      823,        0,      824,      825,        0,        0,        0,      826,        0,
	       0,      827,        0,      828,        0,      829,      830,      831,      832,        0,
	     833,        0,      834,      835,      836,      837,        0,        0,      838,      839,
	     840,      841,      842,      843,      844,      845,      846,      847,        0,      848,
	       0,      849,      850,        0,      851,      852,      853,      854,        0,        0,
	       0,        0,        0,      855,      856,      857,      858,      859,      860,        0,
	       0,      861,      862,        0,        0,        0,      863,      864,        0,        0,
	       0,        0,        0,        0,      865,        0,      866,      867,        0,      868,
	       0,      869,        0,        0,        0,        0,      870,      871,        0,      872,
	       0,        0,        0,        0,        0,      873,      874,      875,        0,        0,
	     876,        0,        0,        0,        0,        0,        0,      877,        0,        0,
	       0,        0,      878,        0,      879,      880,      881,        0,        0,      882,
	       0,        0,      883,        0,      884,        0,      885,      886,      887,      888,
	       0,      889,        0,        0,        0,      890,        0,        0,        0,        0,
	     891,        0,      892,      893,        0,      894,        0,      895,      896,        0,
	     897,        0,        0,      898,      899,        0,        0,        0,        0,        0,
	     900,        0,        0,        0,        0,      901,      902,        0,        0,        0,
	       0,        0,      903,        0,        0,        0,        0,      904,        0,        0,
	     905,      906,      907,      908,        0,        0,      909,        0,      910,      911,
	     912,        0,        0,        0,        0,        0,        0,      913,      914,      915,
	     916,        0,        0,      917,      918,        0,      919,        0,      920,      921,
	     922,      923,      924,        0,        0,      925,      926,      927,      928,      929,
	       0,        0,        0,      930,        0,      931,        0,        0,      932,        0,
	       0,        0,      933,      934,        0,        0,      935,      936,        0,      937,
	       0,        0,        0,        0,        0,        0,        0,      938,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,      939,        0,      940,
	       0,      941,        0,      942,        0,      943,        0,      944,        0,      945,
	     946,        0,        0,        0,        0,        0,      947,      948,        0,        0,
	       0,      949,        0,        0,      950,      951,        0,        0,        0,      952,
	     953,      954,      955,        0,      956,        0,        0,        0,        0,        0,
	     957,      958,        0,      959,        0,      960,        0,      961,      962,      963,
	     964,        0,      965,        0,      966,      967,      968,        0,        0,      969,
	       0,        0,      970,      971,        0,      972,        0,      973,        0,        0,
	       0,        0,        0,      974,      975,        0,      976,      977,        0,        0,
	       0,        0,        0,        0,      978,      979,        0,        0,      980,        0,
	     981,      982,        0,        0,        0,        0,        0,        0,        0,        0,
	     983,        0,        0,      984,        0,        0,      985,      986,        0,        0,
	       0,      987,      988,        0,        0,      989,        0,        0,        0,        0,
	     990,      991,        0,        0,        0,      992,      993,        0,      994,        0,
	     995,        0,        0,      996,        0,      997,        0,        0,        0,      998,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,      999,     1000,     1001,        0,        0,        0,        0,        0,
	    1002,        0,        0,     1003,     1004,     1005,        0,        0,     1006,        0,
	       0,        0,        0,        0,     1007,        0,     1008,        0,        0,        0,
	    1009,     1010,     1011,     1012,     1013,        0,        0,        0,        0,     1014,
	       0,        0,        0,        0,     1015,        0,        0,        0,        0,     1016,
	    1017,        0,        0,        0,        0,        0,        0,        0,        0,     1018,
	    1019,        0,        0,        0,        0,        0,        0,        0,     1020,     1021,
	       0,        0,        0,     1022,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     1023,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     1024,        0,        0,        0,        0,        0,        0,        0,     1025,
	       0,        0,        0,        0,     1026,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     1027,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     1028,        0,
	       0,     1029,        0,        0,     1030,     1031,        0,     1032,     1033,     1034,
	    1035,     1036,     1037,        0,     1038,        0,        0,        0,        0,        0,
	       0,     1039,        0,     1040,        0,        0,     1041,     1042,        0,        0,
	    1043,     1044,        0,        0,        0,     1045,     1046,        0,        0,     1047,
	    1048,     1049,        0,        0,        0,     1050,     1051,        0,     1052,        0,
	    1053,     1054,     1055,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	    1056,        0,        0,     1057,        0,        0,        0,     1058,        0,        0,
	       0,        0,     1059,     1060,     1061,        0,     1062,     1063,     1064,     1065,
	    1066,        0,        0,     1067,        0,        0,        0,        0,        0,     1068,
	    1069,     1070,        0,        0,     1071,        0,     1072,        0,     1073,        0,
	       0,        0,        0,     1074,        0,        0,     1075,        0,     1076,     1077,
	    1078,     1079,     1080,     1081,        0,        0,        0,        0,        0,     1082,
	       0,        0,     1083,     1084,     1085,     1086,     1087,     1088,     1089,     1090,
	       0,        0,     1091,        0,     1092,        0,     1093,     1094,     1095,     1096,
	       0,     1097,        0,     1098,        0,        0,        0,     1099,        0,        0,
	    1100,     1101,        0,        0,        0,     1102,     1103,        0,        0,        0,
	       0,        0,     1104,     1105,     1106,     1107,     1108,        0,        0,        0,
	       0,     1109,     1110,        0,        0,        0,        0,        0,     1111,     1112,
	       0,        0,        0,        0,        0,        0,        0,     1113,        0,        0,
	       0,        0,     1114,     1115,     1116,     1117,     1118,        0,     1119,     1120,
	       0,     1121,        0,     1122,        0,     1123,     1124,        0,        0,        0,
	    1125,        0,     1126,        0,        0,        0,     1127,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     1128,     1129,        0,        0,
	       0,        0,        0,        0,        0,     1130,        0,        0,     1131,     1132,
	       0,        0,     1133,        0,     1134,     1135,        0,        0,     1136,     1137,
	    1138,        0,        0,     1139,        0,     1140,     1141,        0,        0,        0,
	    1142,        0,        0,        0,        0,        0,        0,        0,        0,     1143,
	       0,     1144,        0,        0,        0,        0,     1145,        0,        0,        0,
	    1146,     1147,     1148,        0,        0,     1149,        0,        0,     1150,        0,
	    1151,        0,        0,        0,     1152,     1153,        0,        0,        0,     1154,
	       0,     1155,        0,        0,        0,     1156,        0,        0,        0,     1157,
	       0,        0,        0,     1158,        0,        0,        0,        0,     1159,        0,
	    1160,     1161,        0,        0,     1162,        0,        0,        0,        0,        0,
	    1163,        0,        0,        0,        0,        0,     1164,        0,        0,        0,
	       0,     1165,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     1166,        0,        0,        0,
	       0,        0,        0,        0,     1167,     1168,        0,        0,        0,     1169,
	       0,        0,     1170,        0,        0,        0,     1171,        0,        0,        0,
	       0,        0,     1172,        0,        0,        0,        0,        0,        0,     1173,
	       0,        0,        0,        0,        0,     1174,     1175,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,     1176,        0,     1177,     1178,        0,     1179,
	       0,     1180,        0,        0,        0,     1181,        0,        0,        0,        0,
	       0,        0,        0,        0,     1182,     1183,        0,        0,        0,        0,
	       0,     1184,     1185,        0,        0,        0,     1186,     1187,        0,        0,
	       0,        0,        0,        0,        0,        0,     1188,     1189,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     1190,        0,        0,        0,
	       0,     1191,        0,        0,        0,     1192,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,     1193,        0,        0,
	       0,     1194,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     1195,        0,        0,        0,
	       0,        0,        0,     1196,     1197,        0,     1198,        0,     1199,        0,
	       0,     1200,        0,        0,     1201,        0,        0,     1202,        0,        0,
	       0,        0,        0,        0,        0,        0,     1203,        0,     1204,        0,
	       0,     1205,        0,        0,        0,        0,        0,     1206,        0,     1207,
	       0,        0,        0,        0,     1208,     1209,     1210,        0,        0,     1211,
	    1212,        0,     1213,        0,        0,     1214,        0,        0,        0,        0,
	    1215,     1216,        0,     1217,        0,     1218,     1219,     1220,        0,     1221,
	    1222,     1223,        0,     1224,        0,        0,     1225,        0,        0,     1226,
	    1227,     1228,     1229,        0,     1230,        0,        0,        0,        0,     1231,
	    1232,        0,     1233,        0,        0,     1234,     1235,     1236,        0,     1237,
	       0,        0,     1238,     1239,        0,     1240,        0,        0,     1241,     1242,
	    1243,     1244,     1245,        0,     1246,        0,        0,        0,        0,        0,
	    1247,        0,     1248,        0,        0,     1249,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     1250,
	    1251,        0,     1252,        0,     1253,     1254,        0,        0,        0,     1255,
	       0,        0,        0,     1256,     1257,     1258,     1259,        0,     1260,     1261,
	    1262,        0,     1263,        0,        0,     1264,        0,        0,        0,        0,
	    1265,     1266,        0,        0,     1267,     1268,        0,     1269,        0,        0,
	       0,        0,     1270,        0,        0,        0,        0,     1271,     1272,     1273,
	       0,        0,     1274,     1275,     1276,     1277,        0,        0,     1278,     1279,
	       0,        0,     1280,        0,        0,        0,        0,        0,        0,     1281,
	       0,     1282,        0,        0,     1283,        0,        0,        0,        0,        0,
	       0,        0,     1284,        0,        0,        0,     1285,     1286,        0,        0,
	       0,        0,     1287,     1288,     1289,     1290,     1291,        0,        0,     1292,
	    1293,        0,     1294,        0,     1295,     1296,        0,        0,        0,        0,
	       0,     1297,        0,     1298,        0,        0,     1299,        0,        0,        0,
	    1300,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     1301,        0,     1302,        0,        0,        0,     1303,
	       0,     1304,        0,     1305,     1306,     1307,     1308,     1309,     1310,     1311,
	       0,        0,     1312,        0,        0,        0,        0,     1313,        0,     1314,
	       0,        0,        0,        0,     1315,        0,        0,        0,     1316,        0,
	       0,     1317,     1318,        0,        0,     1319,        0,     1320,        0,        0,
	       0,     1321,        0,        0,        0,        0,        0,        0,        0,     1322,
	    1323,        0,     1324,        0,     1325,        0,        0,        0,        0,        0,
	    1326,        0,        0,        0,     1327,        0,        0,        0,        0,        0,
	    1328,        0,        0,     1329,     1330,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     1331,        0,        0,        0,        0,
	    1332,        0,        0,        0,        0,        0,        0,        0,     1333,        0,
	       0,        0,        0,     1334,        0,        0,     1335,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     1336,     1337,     1338,     1339,
	       0,        0,     1340,        0,        0,        0,        0,     1341,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     1342,        0,        0,        0,
	       0,        0,        0,        0,     1343,     1344,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	    1345,        0,        0,        0,        0,        0,        0,        0,     1346,     1347,
	       0,     1348,        0,        0,     1349,        0,        0,        0,        0,        0,
	    1350,        0,        0,     1351,     1352,        0,        0,        0,        0,        0,
	       0,     1353,        0,        0,     1354,        0,        0,        0,        0,        0,
	    1355,        0,     1356,        0,     1357,        0,     1358,        0,        0,        0,
	    1359,        0,        0,        0,     1360,     1361,        0,     1362,        0,        0,
	    1363,        0,        0,     1364,        0,     1365,        0,        0,        0,        0,
	       0,     1366,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     1367,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     1368,     1369,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     1370,        0,     1371,        0,
	       0,     1372,        0,        0,        0,        0,        0,        0,        0,        0,
	    1373,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     1374,     1375,        0,     1376,
	    1377,     1378,        0,     1379,     1380,     1381,     1382,     1383,     1384,     1385,
	       0,     1386,        0,        0,     1387,     1388,     1389,     1390,     1391,        0,
	       0,     1392,     1393,        0,     1394,        0,        0,        0,     1395,     1396,
	       0,     1397,        0,     1398,        0,        0,        0,        0,     1399,        0,
	       0,     1400,        0,        0,     1401,     1402,        0,     1403,     1404,     1405,
	       0,     1406,     1407,     1408,        0,     1409,     1410,        0,        0,     1411,
	       0,        0,        0,     1412,        0,     1413,        0,     1414,     1415,     1416,
	    1417,     1418,     1419,     1420,     1421,        0,     1422,     1423,     1424,     1425,
	    1426,     1427,     1428,        0,        0,        0,     1429,     1430,        0,        0,
	       0,        0,     1431,        0,        0,     1432,     1433,     1434,     1435,        0,
	    1436,     1437,        0,        0,        0,     1438,     1439,     1440,        0,        0,
	    1441,        0,     1442,     1443,     1444,     1445,        0,        0,        0,        0,
	    1446,        0,        0,        0,     1447,        0,     1448,     1449,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,     1450,     1451,     1452,
	       0,     1453,        0,        0,     1454,     1455,        0,        0,     1456,        0,
	       0,        0,        0,        0,     1457,        0,     1458,        0,        0,        0,
	       0,        0,        0,        0,     1459,     1460,     1461,     1462,     1463,        0,
	       0,     1464,        0,     1465,        0,        0,     1466,        0,     1467,        0,
	       0,     1468,     1469,        0,        0,        0,        0,     1470,        0,     1471,
	       0,        0,     1472,     1473,     1474,        0,     1475,        0,     1476,        0,
	    1477,     1478,        0,        0,        0,        0,     1479,        0,     1480,     1481,
	       0,     1482,        0,        0,        0,        0,     1483,        0,        0,        0,
	       0,     1484,        0,        0,     1485,        0,        0,        0,     1486,     1487,
	    1488,     1489,     1490,     1491,     1492,     1493,     1494,     1495,     1496,        0,
	       0,     1497,        0,        0,     1498,     1499,     1500,     1501,        0,        0,
	    1502,     1503,     1504,     1505,        0,        0,        0,     1506,        0,        0,
	       0,     1507,        0,        0,        0,        0,     1508,        0,     1509,     1510,
	       0,     1511,        0,     1512,     1513,        0,        0,        0,        0,        0,
	       0,        0,     1514,     1515,        0,     1516,        0,        0,        0,        0,
	       0,        0,        0,     1517,     1518,        0,        0,        0,        0,     1519,
	       0,     1520,     1521,        0,        0,        0,        0,        0,     1522,        0,
	       0,        0,     1523,     1524,        0,        0,     1525,     1526,        0,        0,
	    1527,        0,     1528,     1529,     1530,     1531,     1532,     1533,     1534,        0,
	       0,        0,        0,        0,     1535,     1536,        0,        0,        0,        0,
	       0,     1537,        0,     1538,     1539,     1540,        0,        0,        0,     1541,
	       0,     1542,        0,     1543,        0,     1544,     1545,        0,        0,        0,
	       0,     1546,        0,     1547,        0,     1548,        0,        0,     1549,        0,
	       0,        0,        0,        0,        0,     1550,        0,        0,        0,        0,
	       0,        0,     1551,        0,        0,        0,        0,        0,        0,     1552,
	       0,        0,        0,        0,        0,        0,        0,     1553,        0,        0,
	    1554,     1555,     1556,        0,     1557,        0,     1558,        0,        0,     1559,
	       0,        0,     1560,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     1561,        0,        0,        0,        0,        0,        0,
	    1562,     1563,        0,        0,     1564,     1565,        0,        0,        0,        0,
	       0,        0,     1566,        0,        0,        0,        0,        0,     1567,        0,
	    1568,        0,        0,        0,        0,     1569,        0,        0,     1570,        0,
	       0,        0,        0,        0,     1571,        0,     1572,     1573,        0,     1574,
	       0,        0,        0,     1575,     1576,        0,        0,        0,        0,        0,
	    1577,        0,        0,        0,        0,        0,        0,        0,        0,     1578,
	    1579,        0,        0,        0,        0,        0,        0,        0,        0,     1580,
	       0,        0,     1581,     1582,     1583,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     1584,        0,        0,     1585,
	       0,     1586,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     1587,        0,     1588,     1589,        0,        0,     1590,
	       0,        0,        0,        0,     1591,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     1592,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     1593,        0,        0,        0,     1594,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     1595,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     1596,        0,        0,        0,        0,
	       0,        0,        0,     1597,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,     1598,        0,     1599,
	    1600,        0,        0,     1601,     1602,        0,        0,     1603,     1604,     1605,
	    1606,     1607,        0,     1608,        0,        0,     1609,     1610,        0,     1611,
	    1612,     1613,     1614,        0,        0,     1615,        0,        0,        0,        0,
	       0,     1616,     1617,        0,        0,     1618,     1619,     1620,        0,     1621,
	    1622,        0,     1623,        0,        0,        0,     1624,        0,        0,     1625,
	    1626,     1627,        0,        0,     1628,     1629,     1630,        0,     1631,     1632,
	    1633,     1634,     1635,     1636,        0,        0,        0,        0,        0,        0,
	       0,        0,     1637,     1638,        0,        0,        0,        0,        0,     1639,
	    1640,        0,        0,     1641,        0,        0,        0,        0,        0,     1642,
	    1643,        0,        0,     1644,     1645,     1646,        0,        0,        0,        0,
	    1647,        0,     1648,     1649,        0,        0,        0,        0,        0,        0,
	    1650,        0,        0,        0,        0,        0,        0,        0,     1651,     1652,
	       0,        0,        0,        0,        0,     1653,        0,        0,     1654,        0,
	       0,     1655,     1656,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     1657,     1658,
	    1659,        0,     1660,        0,     1661,        0,     1662,     1663,     1664,     1665,
	       0,     1666,     1667,        0,        0,        0,     1668,        0,     1669,     1670,
	       0,        0,     1671,     1672,        0,        0,        0,     1673,     1674,     1675,
	       0,     1676,     1677,     1678,     1679,     1680,        0,     1681,     1682,        0,
	    1683,        0,     1684,     1685,     1686,        0,        0,        0,        0,     1687,
	    1688,     1689,        0,        0,        0,        0,        0,     1690,        0,        0,
	       0,        0,        0,     1691,        0,     1692,     1693,     1694,     1695,     1696,
	       0,        0,        0,        0,     1697,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     1698,     1699,        0,        0,        0,
	       0,        0,        0,     1700,     1701,     1702,        0,        0,     1703,        0,
	       0,        0,        0,     1704,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     1705,        0,     1706,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     1707,        0,
	    1708,     1709,        0,        0,     1710,        0,        0,        0,     1711,     1712,
	    1713,     1714,     1715,     1716,     1717,        0,        0,        0,     1718,        0,
	    1719,     1720,        0,        0,        0,     1721,        0,     1722,        0,     1723,
	       0,     1724,        0,     1725,     1726,        0,        0,     1727,        0,        0,
	       0,     1728,     1729,        0,        0,        0,        0,     1730,     1731,     1732,
	       0,     1733,     1734,        0,        0,     1735,        0,     1736,        0,     1737,
	       0,        0,        0,        0,        0,        0,        0,     1738,     1739,        0,
	    1740,        0,        0,        0,     1741,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	    1742,        0,     1743,     1744,        0,     1745,     1746,     1747,     1748,        0,
	       0,        0,        0,     1749,        0,        0,        0,     1750,     1751,        0,
	    1752,        0,     1753,        0,        0,     1754,     1755,        0,     1756,     1757,
	       0,        0,     1758,     1759,        0,     1760,        0,        0,        0,     1761,
	       0,     1762,        0,     1763,     1764,        0,        0,        0,     1765,     1766,
	    1767,        0,     1768,     1769,        0,     1770,     1771,     1772,     1773,     1774,
	    1775,        0,        0,        0,     1776,        0,     1777,        0,        0,     1778,
	       0,     1779,     1780,     1781,        0,        0,     1782,        0,        0,        0,
	       0,     1783,        0,        0,        0,        0,        0,        0,     1784,        0,
	    1785,        0,        0,     1786,     1787,        0,        0,        0,        0,        0,
	       0,     1788,        0,     1789,        0,        0,        0,        0,     1790,     1791,
	       0,        0,        0,        0,        0,     1792,     1793,     1794,     1795,        0,
	       0,     1796,        0,        0,     1797,     1798,        0,     1799,     1800,     1801,
	    1802,        0,        0,        0,     1803,     1804,     1805,     1806,        0,        0,
	       0,     1807,        0,        0,     1808,     1809,        0,        0,     1810,        0,
	       0,     1811,        0,        0,     1812,     1813,        0,     1814,     1815,        0,
	       0,     1816,        0,        0,        0,     1817,        0,        0,     1818,        0,
	       0,     1819,        0,     1820,     1821,     1822,     1823,     1824,     1825,     1826,
	    1827,     1828,     1829,        0,        0,        0,     1830,        0,        0,     1831,
	    1832,     1833,        0,        0,     1834,        0,     1835,     1836,     1837,        0,
	       0,     1838,        0,     1839,     1840,     1841,        0,        0,     1842,     1843,
	       0,        0,        0,     1844,     1845,     1846,     1847,     1848,     1849,     1850,
	       0,        0,        0,     1851,        0,        0,        0,        0,        0,     1852,
	       0,        0,        0,        0,        0,     1853,     1854,        0,        0,     1855,
	       0,     1856,     1857,     1858,        0,        0,        0,        0,        0,        0,
	       0,     1859,        0,     1860,        0,        0,     1861,        0,     1862,        0,
	       0,     1863,        0,        0,        0,     1864,     1865,        0,        0,     1866,
	       0,        0,        0,        0,     1867,     1868,     1869,        0,        0,     1870,
	    1871,     1872,     1873,     1874,     1875,     1876,        0,     1877,     1878,        0,
	       0,     1879,        0,        0,     1880,        0,     1881,     1882,     1883,     1884,
	    1885,     1886,        0,     1887,        0,        0,        0,     1888,     1889,        0,
	       0,        0,        0,     1890,        0,        0,     1891,     1892,        0,        0,
	       0,        0,     1893,        0,     1894,        0,     1895,        0,        0,        0,
	    1896,     1897,        0,     1898,        0,     1899,     1900,        0,        0,     1901,
	       0,        0,     1902,        0,     1903,        0,        0,     1904,     1905,     1906,
	       0,     1907,        0,     1908,     1909,        0,     1910,        0,        0,        0,
	    1911,        0,        0,     1912,     1913,        0,        0,        0,        0,        0,
	       0,        0,        0,     1914,     1915,        0,        0,        0,     1916,     1917,
	       0,        0,        0,        0,        0,     1918,        0,        0,        0,     1919,
	       0,        0,     1920,        0,     1921,        0,     1922,     1923,        0,     1924,
	    1925,        0,        0,        0,        0,        0,     1926,     1927,     1928,     1929,
	       0,     1930,     1931,     1932,     1933,     1934,     1935,        0,        0,     1936,
	    1937,        0,     1938,        0,        0,     1939,     1940,        0,        0,        0,
	       0,        0,     1941,     1942,        0,        0,        0,        0,     1943,        0,
	    1944,     1945,        0,        0,        0,     1946,     1947,     1948,        0,        0,
	       0,        0,        0,     1949,        0,        0,        0,        0,     1950,        0,
	       0,        0,        0,     1951,     1952,        0,        0,     1953,     1954,        0,
	    1955,     1956,        0,        0,        0,     1957,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     1958,        0,        0,        0,        0,
	       0,        0,        0,        0,     1959,        0,     1960,        0,     1961,        0,
	    1962,        0,        0,     1963,        0,        0,        0,     1964,        0,        0,
	       0,        0,        0,        0,        0,     1965,        0,        0,        0,        0,
	    1966,        0,        0,        0,        0,     1967,     1968,        0,        0,        0,
	       0,        0,        0,        0,     1969,        0,        0,        0,        0,     1970,
	       0,     1971,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     1972,        0,        0,     1973,        0,        0,        0,        0,        0,
	    1974,        0,        0,        0,        0,        0,        0,        0,        0,     1975,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     1976,
	    1977,     1978,        0,        0,     1979,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     1980,        0,        0,        0,        0,
	       0,        0,     1981,        0,        0,        0,     1982,        0,        0,        0,
	       0,        0,     1983,        0,     1984,     1985,        0,        0,        0,        0,
	       0,     1986,     1987,        0,     1988,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,     1989,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,     1990,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,     1991,        0,        0,
	       0,        0,        0,        0,     1992,        0,     1993,        0,     1994,     1995,
	    1996,     1997,     1998,     1999,     2000,     2001,     2002,        0,        0,     2003,
	    2004,     2005,     2006,        0,     2007,     2008,        0,        0,        0,     2009,
	       0,     2010,     2011,        0,     2012,     2013,        0,        0,        0,        0,
	    2014,        0,     2015,        0,     2016,        0,        0,        0,        0,     2017,
	    2018,        0,        0,     2019,        0,        0,        0,        0,        0,     2020,
	    2021,     2022,     2023,     2024,        0,     2025,        0,        0,        0,     2026,
	    2027,     2028,        0,     2029,     2030,     2031,     2032,        0,        0,     2033,
	    2034,     2035,     2036,        0,        0,        0,     2037,        0,        0,     2038,
	       0,        0,        0,        0,        0,        0,        0,     2039,        0,        0,
	    2040,     2041,        0,     2042,     2043,     2044,     2045,     2046,     2047,     2048,
	    2049,        0,        0,     2050,        0,        0,     2051,        0,        0,     2052,
	       0,        0,     2053,        0,     2054,     2055,     2056,        0,        0,        0,
	    2057,        0,        0,        0,        0,     2058,     2059,        0,        0,        0,
	       0,        0,        0,     2060,     2061,     2062,        0,     2063,     2064,     2065,
	    2066,        0,     2067,     2068,        0,        0,        0,     2069,     2070,     2071,
	    2072,        0,     2073,     2074,        0,        0,     2075,        0,        0,     2076,
	    2077,        0,        0,        0,        0,     2078,        0,        0,        0,     2079,
	       0,        0,        0,     2080,        0,     2081,     2082,     2083,        0,     2084,
	       0,        0,     2085,        0,     2086,     2087,     2088,     2089,     2090,     2091,
	    2092,        0,     2093,     2094,     2095,        0,     2096,        0,     2097,     2098,
	    2099,        0,     2100,     2101,     2102,     2103,     2104,     2105,     2106,        0,
	       0,     2107,        0,        0,     2108,     2109,        0,     2110,     2111,     2112,
	    2113,     2114,        0,        0,     2115,     2116,     2117,     2118,        0,     2119,
	       0,     2120,     2121,        0,     2122,     2123,        0,        0,        0,        0,
	    2124,     2125,     2126,     2127,        0,     2128,     2129,        0,        0,        0,
	       0,     2130,     2131,     2132,        0,        0,        0,        0,     2133,        0,
	       0,     2134,        0,        0,        0,        0,     2135,        0,        0,        0,
	    2136,     2137,        0,     2138,     2139,     2140,     2141,     2142,     2143,     2144,
	    2145,     2146,        0,        0,     2147,        0,     2148,     2149,        0,        0,
	       0,     2150,        0,        0,     2151,        0,        0,        0,        0,        0,
	       0,     2152,     2153,        0,        0,     2154,        0,        0,        0,        0,
	    2155,     2156,        0,     2157,     2158,        0,        0,     2159,        0,     2160,
	    2161,     2162,     2163,     2164,     2165,        0,        0,        0,        0,     2166,
	       0,        0,        0,        0,        0,        0,        0,        0,     2167,     2168,
	       0,     2169,     2170,     2171,        0,        0,        0,     2172,        0,     2173,
	       0,        0,        0,     2174,     2175,        0,        0,     2176,        0,        0,
	       0,        0,     2177,     2178,        0,        0,     2179,     2180,        0,        0,
	       0,        0,     2181,        0,     2182,        0,        0,        0,        0,     2183,
	    2184,     2185,     2186,        0,     2187,        0,     2188,     2189,     2190,        0,
	    2191,        0,        0,        0,     2192,        0,     2193,        0,        0,        0,
	       0,        0,        0,        0,     2194,        0,     2195,     2196,        0,     2197,
	       0,     2198,     2199,     2200,     2201,        0,     2202,     2203,     2204,        0,
	    2205,        0,        0,     2206,     2207,        0,        0,     2208,     2209,        0,
	    2210,        0,     2211,        0,     2212,        0,        0,        0,        0,        0,
	    2213,        0,     2214,        0,        0,     2215,        0,        0,        0,     2216,
	    2217,     2218,     2219,        0,     2220,        0,        0,        0,     2221,        0,
	       0,        0,        0,        0,        0,        0,     2222,        0,     2223,     2224,
	       0,     2225,        0,        0,        0,        0,        0,     2226,     2227,        0,
	       0,     2228,        0,        0,        0,        0,     2229,        0,     2230,        0,
	    2231,        0,     2232,        0,        0,        0,        0,     2233,        0,     2234,
	    2235,     2236,     2237,        0,        0,     2238,        0,        0,        0,        0,
	       0,     2239,     2240,        0,        0,     2241,     2242,        0,        0,     2243,
	    2244,        0,        0,        0,        0,        0,        0,     2245,     2246,        0,
	    2247,        0,     2248,     2249,        0,        0,        0,        0,     2250,        0,
	       0,        0,     2251,        0,     2252,     2253,        0,        0,        0,        0,
	       0,        0,     2254,        0,        0,        0,        0,        0,     2255,        0,
	       0,     2256,        0,     2257,        0,     2258,        0,        0,     2259,     2260,
	    2261,     2262,     2263,        0,     2264,        0,        0,        0,        0,        0,
	       0,        0,     2265,        0,     2266,        0,        0,        0,     2267,        0,
	       0,        0,        0,        0,     2268,        0,        0,        0,        0,        0,
	       0,        0,        0,     2269,        0,     2270,        0,        0,        0,     2271,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	    2272,     2273,     2274,        0,        0,        0,        0,        0,        0,        0,
	    2275,        0,     2276,     2277,        0,     2278,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     2279,     2280,        0,        0,     2281,
	    2282,        0,        0,     2283,        0,        0,        0,        0,     2284,        0,
	       0,        0,        0,        0,     2285,        0,        0,        0,        0,     2286,
	       0,        0,     2287,     2288,     2289,        0,     2290,        0,        0,        0,
	       0,     2291,        0,     2292,     2293,        0,     2294,        0,     2295,        0,
	       0,        0,     2296,        0,     2297,        0,        0,     2298,        0,        0,
	       0,        0,        0,        0,        0,     2299,     2300,        0,     2301,        0,
	    2302,        0,        0,        0,        0,     2303,     2304,        0,        0,        0,
	       0,        0,     2305,        0,        0,        0,     2306,        0,     2307,        0,
	    2308,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     2309,        0,        0,        0,
	       0,        0,        0,        0,        0,     2310,        0,        0,        0,        0,
	       0,        0,        0,        0,     2311,        0,        0,        0,        0,        0,
	    2312,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     2313,        0,        0,        0,        0,        0,     2314,
	       0,        0,     2315,     2316,        0,        0,        0,        0,     2317,     2318,
	    2319,        0,     2320,     2321,        0,     2322,        0,        0,     2323,     2324,
	       0,        0,        0,        0,        0,     2325,        0,        0,     2326,     2327,
	       0,        0,     2328,        0,        0,     2329,        0,     2330,        0,        0,
	       0,     2331,     2332,        0,        0,     2333,        0,     2334,        0,     2335,
	    2336,        0,        0,        0,     2337,     2338,        0,        0,     2339,        0,
	       0,        0,        0,     2340,     2341,        0,        0,        0,     2342,        0,
	    2343,        0,     2344,        0,        0,     2345,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     2346,        0,        0,        0,     2347,     2348,        0,        0,        0,
	    2349,     2350,        0,     2351,        0,        0,        0,     2352,        0,     2353,
	       0,     2354,     2355,        0,        0,     2356,        0,     2357,        0,        0,
	    2358,     2359,        0,     2360,        0,     2361,        0,     2362,        0,     2363,
	       0,     2364,     2365,        0,        0,        0,        0,        0,        0,        0,
	       0,     2366,        0,        0,     2367,     2368,        0,        0,        0,     2369,
	       0,     2370,     2371,     2372,     2373,        0,        0,        0,        0,     2374,
	    2375,        0,     2376,     2377,        0,        0,     2378,        0,        0,        0,
	    2379,     2380,        0,        0,        0,        0,        0,        0,        0,        0,
	    2381,        0,     2382,        0,        0,     2383,     2384,     2385,     2386,     2387,
	       0,        0,     2388,     2389,     2390,     2391,     2392,     2393,        0,        0,
	       0,        0,     2394,     2395,        0,        0,     2396,        0,        0,        0,
	       0,        0,     2397,        0,     2398,     2399,        0,        0,     2400,        0,
	       0,        0,     2401,        0,     2402,        0,     2403,     2404,        0,        0,
	       0,     2405,     2406,     2407,        0,        0,        0,     2408,        0,        0,
	       0,     2409,        0,     2410,     2411,        0,        0,        0,        0,     2412,
	       0,     2413,     2414,        0,        0,        0,        0,     2415,        0,     2416,
	       0,     2417,        0,        0,     2418,     2419,     2420,        0,        0,        0,
	       0,        0,     2421,        0,     2422,        0,        0,     2423,        0,     2424,
	       0,        0,        0,        0,        0,        0,        0,     2425,     2426,        0,
	       0,     2427,        0,        0,     2428,     2429,     2430,     2431,     2432,     2433,
	       0,        0,     2434,        0,        0,        0,        0,     2435,        0,     2436,
	       0,        0,     2437,        0,     2438,        0,     2439,        0,        0,        0,
	       0,        0,     2440,     2441,     2442,        0,        0,        0,     2443,        0,
	    2444,     2445,        0,        0,     2446,        0,        0,        0,     2447,        0,
	       0,        0,     2448,        0,     2449,        0,        0,     2450,        0,        0,
	       0,        0,     2451,        0,        0,        0,        0,     2452,        0,        0,
	       0,        0,     2453,     2454,        0,        0,        0,        0,        0,     2455,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     2456,
	    2457,        0,        0,        0,        0,        0,     2458,        0,        0,        0,
	       0,        0,     2459,        0,        0,        0,        0,     2460,        0,        0,
	       0,        0,     2461,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     2462,
	       0,     2463,     2464,     2465,        0,        0,        0,        0,        0,        0,
	       0,        0,     2466,        0,        0,     2467,        0,        0,        0,        0,
	       0,        0,     2468,        0,     2469,     2470,     2471,        0,        0,     2472,
	       0,     2473,        0,        0,     2474,        0,     2475,     2476,     2477,        0,
	       0,        0,        0,        0,        0,        0,     2478,     2479,     2480,     2481,
	       0,     2482,        0,        0,     2483,        0,        0,        0,     2484,     2485,
	       0,     2486,        0,        0,        0,     2487,        0,     2488,        0,     2489,
	       0,        0,        0,        0,        0,        0,     2490,        0,     2491,        0,
	    2492,     2493,     2494,     2495,        0,     2496,        0,     2497,        0,        0,
	    2498,     2499,        0,        0,        0,        0,     2500,        0,        0,     2501,
	       0,        0,     2502,        0,     2503,     2504,        0,        0,     2505,        0,
	    2506,     2507,        0,        0,     2508,        0,     2509,     2510,     2511,     2512,
	       0,     2513,        0,        0,     2514,        0,        0,        0,        0,        0,
	    2515,        0,     2516,     2517,     2518,     2519,        0,        0,        0,     2520,
	       0,        0,     2521,     2522,     2523,        0,        0,     2524,        0,     2525,
	    2526,        0,     2527,     2528,        0,     2529,        0,     2530,        0,        0,
	       0,        0,     2531,        0,     2532,     2533,        0,     2534,        0,        0,
	    2535,        0,        0,     2536,        0,     2537,        0,     2538,        0,        0,
	       0,        0,     2539,        0,        0,        0,        0,     2540,        0,     2541,
	    2542,        0,     2543,        0,     2544,     2545,     2546,        0,        0,        0,
	    2547,     2548,        0,     2549,        0,     2550,     2551,        0,     2552,     2553,
	       0,     2554,        0,     2555,     2556,        0,        0,     2557,        0,     2558,
	    2559,     2560,     2561,        0,        0,        0,        0,        0,        0,        0,
	       0,     2562,        0,     2563,     2564,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     2565,     2566,     2567,     2568,     2569,
	    2570,        0,        0,        0,     2571,     2572,     2573,        0,     2574,     2575,
	    2576,        0,     2577,        0,     2578,        0,        0,     2579,        0,        0,
	       0,     2580,        0,        0,     2581,        0,        0,     2582,     2583,     2584,
	       0,     2585,     2586,        0,        0,        0,        0,        0,        0,        0,
	       0,     2587,        0,     2588,     2589,        0,        0,        0,        0,     2590,
	       0,     2591,     2592,     2593,     2594,     2595,     2596,        0,     2597,     2598,
	       0,     2599,        0,     2600,        0,        0,     2601,     2602,        0,        0,
	       0,        0,        0,     2603,        0,        0,        0,     2604,        0,        0,
	       0,        0,        0,        0,        0,     2605,     2606,        0,        0,        0,
	       0,        0,        0,        0,     2607,     2608,        0,        0,        0,     2609,
	    2610,     2611,        0,        0,     2612,     2613,     2614,        0,     2615,     2616,
	    2617,     2618,     2619,     2620,     2621,        0,     2622,     2623,     2624,        0,
	    2625,        0,     2626,        0,     2627,     2628,        0,     2629,     2630,     2631,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	    2632,     2633,     2634,     2635,     2636,     2637,     2638,     2639,     2640,     2641,
	       0,     2642,        0,        0,        0,        0,        0,        0,        0,        0,
	    2643,        0,     2644,     2645,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     2646,        0,     2647,        0,     2648,     2649,        0,
	       0,        0,        0,        0,        0,        0,        0,     2650,        0,        0,
	       0,     2651,        0,        0,        0,     2652,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     2653,        0,        0,        0,
	    2654,     2655,     2656,        0,        0,        0,        0,     2657,        0,     2658,
	    2659,        0,        0,     2660,        0,     2661,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     2662,        0,     2663,        0,
	       0,        0,        0,        0,        0,     2664,        0,     2665,        0,     2666,
	       0,        0,        0,        0,     2667,        0,        0,     2668,        0,        0,
	    2669,        0,     2670,        0,        0,        0,        0,        0,     2671,        0,
	       0,     2672,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,     2673,        0,     2674,     2675,        0,        0,        0,     2676,
	       0,        0,        0,     2677,     2678,        0,     2679,        0,        0,        0,
	       0,     2680,        0,        0,        0,     2681,        0,        0,        0,        0,
	       0,     2682,        0,     2683,     2684,        0,     2685,        0,     2686,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     2687,     2688,        0,        0,        0,     2689,        0,        0,        0,
	       0,        0,        0,        0,        0,     2690,        0,        0,     2691,        0,
	       0,        0,     2692,        0,        0,        0,        0,     2693,        0,        0,
	       0,     2694,        0,     2695,        0,        0,     2696,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,     2697,        0,        0,     2698,        0,        0,
	    2699,        0,        0,     2700,     2701,        0,     2702,        0,        0,     2703,
	       0,        0,     2704,        0,        0,        0,        0,     2705,        0,        0,
	    2706,        0,        0,     2707,        0,        0,        0,        0,        0,     2708,
	    2709,     2710,        0,        0,     2711,        0,        0,        0,     2712,        0,
	    2713,        0,     2714,        0,     2715,     2716,     2717,     2718,        0,        0,
	       0,     2719,        0,        0,        0,        0,        0,        0,     2720,     2721,
	       0,        0,     2722,        0,        0,     2723,     2724,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,     2725,     2726,        0,
	       0,     2727,        0,     2728,        0,        0,        0,     2729,        0,        0,
	    2730,        0,        0,     2731,        0,        0,        0,     2732,        0,        0,
	       0,        0,        0,     2733,        0,        0,        0,        0,        0,        0,
	       0,        0,     2734,        0,     2735,        0,     2736,        0,     2737,        0,
	       0,        0,     2738,        0,        0,        0,        0,        0,        0,     2739,
	       0,        0,        0,     2740,     2741,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,     2742,        0,        0,
	       0,        0,     2743,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     2744,        0,     2745,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     2746,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     2747,
	    2748,        0,        0,        0,        0,        0,        0,     2749,        0,     2750,
	       0,        0,        0,        0,        0,        0,     2751,        0,     2752,        0,
	       0,        0,        0,     2753,        0,     2754,        0,        0,        0,     2755,
	       0,        0,        0,        0,        0,        0,        0,     2756,     2757,        0,
	       0,        0,        0,        0,     2758,        0,        0,     2759,        0,        0,
	       0,        0,        0,        0,        0,        0,     2760,        0,        0,        0,
	       0,        0,        0,        0,     2761,     2762,        0,     2763,        0,        0,
	       0,        0,        0,     2764,        0,        0,        0,     2765,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     2766,
	       0,        0,        0,        0,        0,        0,        0,     2767,        0,        0,
	    2768,        0,        0,        0,     2769,        0,        0,        0,     2770,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     2771,        0,
	    2772,     2773,        0,        0,        0,        0,        0,     2774,        0,        0,
	       0,        0,        0,        0,        0,        0,     2775,        0,        0,        0,
	       0,        0,        0,        0,        0,     2776,        0,     2777,     2778,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,     2779,     2780,     2781,     2782,     2783,        0,
	       0,     2784,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,     2785,        0,        0,        0,        0,     2786,        0,     2787,
	    2788,        0,        0,        0,     2789,        0,        0,        0,        0,     2790,
	       0,        0,     2791,     2792,        0,     2793,        0,        0,     2794,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     2795,        0,        0,        0,        0,        0,        0,        0,        0,
	    2796,     2797,     2798,     2799,     2800,     2801,        0,        0,     2802,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     2803,        0,     2804,     2805,        0,        0,        0,
	       0,     2806,     2807,     2808,     2809,        0,     2810,     2811,        0,     2812,
	    2813,     2814,        0,     2815,        0,        0,        0,        0,     2816,     2817,
	       0,        0,     2818,        0,        0,        0,     2819,     2820,        0,        0,
	       0,        0,        0,     2821,        0,        0,        0,        0,        0,        0,
	       0,        0,     2822,        0,        0,        0,        0,        0,        0,        0,
	       0,     2823,     2824,     2825,        0,     2826,        0,        0,        0,        0,
	       0,        0,        0,     2827,        0,     2828,     2829,        0,        0,     2830,
	       0,        0,        0,        0,        0,     2831,        0,     2832,        0,     2833,
	       0,        0,     2834,     2835,     2836,     2837,     2838,     2839,        0,     2840,
	       0,     2841,        0,        0,        0,        0,        0,     2842,        0,        0,
	       0,        0,        0,        0,        0,        0,     2843,     2844,        0,        0,
	       0,     2845,        0,        0,        0,     2846,        0,     2847,        0,        0,
	       0,     2848,        0,        0,        0,     2849,        0,        0,        0,        0,
	       0,        0,        0,     2850,     2851,     2852,        0,        0,        0,        0,
	       0,     2853,        0,     2854,     2855,     2856,        0,     2857,     2858,     2859,
	    2860,        0,     2861,     2862,     2863,     2864,        0,        0,        0,     2865,
	       0,     2866,     2867,        0,     2868,        0,     2869,     2870,     2871,     2872,
	    2873,        0,        0,        0,     2874,     2875,     2876,        0,     2877,        0,
	    2878,     2879,        0,        0,     2880,        0,        0,        0,        0,     2881,
	       0,        0,     2882,     2883,     2884,        0,        0,        0,     2885,     2886,
	       0,     2887,     2888,        0,        0,        0,        0,        0,     2889,        0,
	       0,        0,     2890,     2891,        0,     2892,        0,        0,        0,     2893,
	    2894,     2895,     2896,     2897,     2898,     2899,        0,        0,     2900,        0,
	       0,        0,     2901,     2902,     2903,        0,        0,        0,        0,        0,
	    2904,        0,     2905,        0,     2906,        0,     2907,        0,        0,     2908,
	       0,        0,        0,     2909,     2910,        0,        0,     2911,     2912,     2913,
	       0,     2914,     2915,        0,     2916,     2917,        0,        0,     2918,        0,
	       0,     2919,     2920,        0,        0,     2921,     2922,        0,        0,        0,
	       0,     2923,        0,     2924,        0,        0,        0,     2925,        0,     2926,
	       0,     2927,     2928,     2929,     2930,     2931,        0,     2932,     2933,     2934,
	       0,     2935,     2936,        0,        0,     2937,     2938,     2939,        0,        0,
	       0,        0,     2940,     2941,        0,     2942,     2943,     2944,     2945,     2946,
	       0,        0,        0,        0,     2947,     2948,        0,        0,        0,     2949,
	    2950,        0,     2951,        0,        0,        0,     2952,        0,        0,     2953,
	    2954,     2955,     2956,     2957,        0,        0,        0,     2958,        0,        0,
	    2959,        0,     2960,     2961,     2962,     2963,        0,     2964,        0,        0,
	    2965,        0,     2966,     2967,        0,        0,     2968,     2969,     2970,     2971,
	       0,     2972,        0,     2973,     2974,     2975,     2976,        0,     2977,     2978,
	    2979,     2980,     2981,        0,        0,     2982,        0,        0,     2983,        0,
	       0,     2984,        0,        0,        0,     2985,     2986,        0,     2987,        0,
	       0,        0,     2988,        0,        0,        0,        0,     2989,        0,     2990,
	    2991,     2992,        0,        0,     2993,        0,        0,        0,        0,        0,
	       0,     2994,        0,     2995,        0,        0,     2996,     2997,        0,        0,
	    2998,        0,        0,     2999,     3000,     3001,        0,     3002,        0,        0,
	       0,     3003,        0,     3004,     3005,     3006,     3007,        0,        0,     3008,
	       0,     3009,        0,     3010,     3011,     3012,     3013,        0,     3014,     3015,
	       0,     3016,     3017,     3018,        0,     3019,     3020,     3021,     3022,        0,
	       0,        0,        0,     3023,     3024,        0,     3025,        0,     3026,        0,
	    3027,        0,        0,     3028,        0,        0,     3029,        0,        0,     3030,
	    3031,        0,        0,        0,     3032,     3033,        0,        0,        0,        0,
	    3034,        0,        0,     3035,     3036,        0,        0,        0,     3037,        0,
	       0,        0,        0,        0,     3038,        0,        0,     3039,        0,        0,
	    3040,     3041,        0,        0,     3042,        0,     3043,        0,        0,     3044,
	       0,     3045,     3046,     3047,        0,        0,        0,        0,        0,     3048,
	       0,     3049,     3050,     3051,     3052,     3053,        0,     3054,     3055,        0,
	    3056,     3057,     3058,     3059,     3060,     3061,        0,        0,     3062,     3063,
	       0,        0,     3064,        0,        0,     3065,        0,        0,     3066,        0,
	       0,        0,        0,        0,        0,     3067,     3068,        0,        0,        0,
	    3069,     3070,     3071,     3072,        0,        0,        0,     3073,     3074,        0,
	       0,        0,        0,     3075,        0,        0,        0,        0,     3076,        0,
	    3077,     3078,        0,        0,        0,     3079,     3080,        0,     3081,     3082,
	       0,        0,     3083,        0,     3084,        0,        0,        0,        0,     3085,
	    3086,        0,     3087,        0,        0,     3088,        0,     3089,        0,        0,
	       0,     3090,        0,     3091,        0,     3092,     3093,        0,        0,        0,
	       0,        0,        0,        0,        0,     3094,        0,        0,        0,        0,
	    3095,        0,     3096,     3097,     3098,        0,     3099,     3100,        0,        0,
	    3101,        0,     3102,     3103,        0,        0,     3104,        0,        0,     3105,
	       0,        0,     3106,     3107,        0,     3108,     3109,     3110,        0,        0,
	       0,     3111,        0,     3112,        0,     3113,        0,     3114,        0,        0,
	    3115,        0,     3116,        0,        0,        0,     3117,        0,     3118,        0,
	       0,        0,        0,        0,        0,        0,        0,     3119,     3120,        0,
	       0,        0,        0,        0,        0,        0,        0,     3121,     3122,        0,
	       0,        0,        0,     3123,     3124,        0,     3125,        0,     3126,        0,
	       0,     3127,        0,        0,        0,     3128,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,     3129,        0,        0,
	    3130,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     3131,     3132,        0,        0,
	       0,     3133,        0,     3134,     3135,        0,        0,     3136,        0,        0,
	       0,        0,        0,     3137,     3138,        0,        0,        0,        0,        0,
	       0,        0,     3139,        0,        0,        0,     3140,        0,        0,     3141,
	       0,        0,     3142,        0,        0,     3143,        0,     3144,        0,        0,
	    3145,        0,        0,        0,        0,     3146,        0,     3147,     3148,        0,
	    3149,        0,     3150,     3151,        0,        0,     3152,     3153,        0,     3154,
	       0,        0,        0,     3155,     3156,        0,        0,        0,        0,     3157,
	       0,        0,        0,     3158,        0,        0,        0,     3159,        0,     3160,
	       0,     3161,     3162,     3163,        0,     3164,        0,        0,     3165,        0,
	       0,        0,     3166,     3167,     3168,     3169,     3170,        0,     3171,     3172,
	    3173,        0,     3174,     3175,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,     3176,        0,        0,        0,        0,     3177,
	       0,        0,        0,        0,        0,        0,        0,        0,     3178,        0,
	       0,        0,     3179,        0,        0,     3180,        0,        0,        0,        0,
	       0,     3181,        0,        0,        0,     3182,     3183,     3184,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     3185,        0,
	       0,        0,     3186,        0,        0,        0,        0,     3187,     3188,     3189,
	       0,     3190,        0,     3191,        0,     3192,        0,     3193,        0,        0,
	    3194,        0,        0,        0,        0,        0,        0,        0,     3195,        0,
	       0,        0,        0,        0,        0,        0,     3196,     3197,        0,        0,
	       0,     3198,        0,     3199,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,     3200,        0,        0,        0,     3201,        0,
	    3202,        0,        0,        0,     3203,        0,        0,        0,     3204,        0,
	       0,        0,        0,        0,        0,     3205,     3206,        0,        0,        0,
	    3207,        0,     3208,        0,        0,        0,     3209,        0,     3210,        0,
	    3211,        0,        0,        0,        0,        0,        0,        0,     3212,        0,
	       0,        0,     3213,     3214,        0,        0,     3215,     3216,     3217,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,     3218,        0,        0,        0,        0,     3219,        0,        0,
	       0,        0,        0,     3220,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     3221,        0,        0,     3222,        0,
	       0,     3223,        0,        0,        0,        0,        0,        0,     3224,        0,
	    3225,        0,        0,        0,        0,        0,        0,     3226,        0,        0,
	       0,        0,        0,        0,        0,     3227,     3228,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     3229,        0,
	    3230,     3231,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,     3232,     3233,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     3234,
	       0,        0,        0,        0,        0,        0,        0,        0,     3235,     3236,
	       0,        0,        0,        0,        0,        0,        0,     3237,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     3238,        0,        0,        0,     3239,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     3240,        0,
	       0,     3241,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     3242,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     3243,
	    3244,     3245,        0,     3246,     3247,        0,        0,        0,        0,     3248,
	    3249,        0,     3250,        0,        0,        0,     3251,        0,     3252,     3253,
	    3254,        0,        0,        0,        0,     3255,        0,        0,        0,     3256,
	    3257,        0,        0,        0,     3258,        0,        0,        0,     3259,        0,
	    3260,     3261,     3262,        0,        0,     3263,        0,        0,     3264,     3265,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     3266,     3267,     3268,     3269,     3270,        0,     3271,
	       0,     3272,        0,        0,        0,     3273,     3274,     3275,        0,     3276,
	    3277,     3278,        0,        0,     3279,     3280,     3281,     3282,        0,        0,
	       0,        0,     3283,        0,     3284,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     3285,     3286,
	       0,     3287,        0,        0,        0,     3288,        0,        0,        0,        0,
	    3289,        0,     3290,     3291,     3292,     3293,        0,     3294,     3295,     3296,
	       0,     3297,        0,        0,        0,        0,        0,        0,        0,     3298,
	       0,     3299,        0,        0,        0,     3300,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,     3301,     3302,        0,
	       0,        0,        0,        0,     3303,        0,        0,     3304,        0,     3305,
	    3306,        0,     3307,     3308,     3309,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     3310,        0,        0,        0,
	       0,        0,        0,        0,        0,     3311,     3312,     3313,        0,        0,
	       0,        0,     3314,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,     3315,        0,        0,
	       0,        0,     3316,        0,     3317,        0,     3318,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	    3319,        0,     3320,        0,        0,        0,        0,        0,     3321,        0,
	    3322,     3323,     3324,        0,        0,        0,        0,        0,     3325,        0,
	       0,        0,     3326,     3327,        0,        0,        0,        0,     3328,        0,
	    3329,        0,        0,     3330,        0,        0,        0,        0,        0,        0,
	    3331,        0,        0,        0,        0,        0,     3332,        0,        0,        0,
	       0,     3333,        0,        0,        0,        0,     3334,        0,        0,        0,
	    3335,     3336,        0,        0,        0,        0,        0,     3337,     3338,        0,
	       0,        0,        0,        0,        0,        0,     3339,        0,        0,        0,
	    3340,        0,        0,        0,        0,        0,        0,     3341,        0,     3342,
	       0,        0,        0,     3343,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     3344,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,     3345,        0,        0,        0,        0,        0,
	    3346,     3347,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,     3348,        0,        0,        0,        0,     3349,        0,     3350,
	       0,        0,        0,        0,        0,        0,     3351,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,     3352,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	    3353,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     3354,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,     3355,        0,     3356,        0,     3357,        0,
	       0,        0,     3358,     3359,        0,        0,        0,     3360,     3361,     3362,
	    3363,     3364,        0,     3365,        0,     3366,        0,     3367,        0,        0,
	       0,        0,        0,        0,        0,     3368,     3369,        0,        0,        0,
	    3370,     3371,        0,        0,        0,        0,     3372,        0,        0,        0,
	    3373,        0,        0,     3374,        0,     3375,        0,     3376,        0,     3377,
	       0,     3378,     3379,        0,        0,        0,     3380,     3381,        0,     3382,
	       0,        0,        0,        0,     3383,     3384,        0,        0,     3385,        0,
	       0,     3386,        0,        0,        0,     3387,     3388,        0,        0,        0,
	    3389,     3390,     3391,     3392,        0,        0,     3393,        0,        0,        0,
	       0,        0,     3394,     3395,        0,     3396,        0,     3397,        0,        0,
	    3398,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     3399,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     3400,     3401,        0,     3402,
	    3403,        0,        0,        0,     3404,        0,     3405,     3406,     3407,     3408,
	       0,        0,        0,        0,        0,        0,        0,     3409,     3410,     3411,
	    3412,        0,        0,        0,     3413,        0,        0,        0,        0,     3414,
	    3415,        0,     3416,        0,     3417,        0,        0,        0,        0,     3418,
	       0,     3419,        0,        0,        0,        0,     3420,        0,     3421,     3422,
	       0,        0,        0,        0,        0,        0,     3423,     3424,        0,        0,
	    3425,     3426,     3427,     3428,     3429,     3430,     3431,     3432,     3433,        0,
	       0,     3434,     3435,        0,     3436,     3437,     3438,        0,        0,        0,
	       0,     3439,        0,     3440,        0,        0,        0,        0,        0,        0,
	    3441,        0,        0,        0,     3442,        0,        0,        0,        0,     3443,
	       0,     3444,     3445,     3446,        0,        0,        0,     3447,     3448,     3449,
	    3450,        0,        0,     3451,     3452,        0,        0,     3453,        0,        0,
	       0,     3454,     3455,     3456,     3457,        0,     3458,        0,        0,     3459,
	       0,        0,     3460,        0,        0,     3461,     3462,     3463,        0,        0,
	       0,        0,     3464,     3465,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,     3466,        0,        0,
	    3467,        0,     3468,        0,        0,        0,        0,     3469,        0,        0,
	       0,        0,        0,        0,        0,        0,     3470,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     3471,     3472,
	       0,     3473,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     3474,        0,        0,        0,
	       0,        0,     3475,        0,        0,     3476,        0,     3477,        0,     3478,
	       0,        0,     3479,        0,        0,     3480,        0,        0,        0,        0,
	    3481,        0,        0,        0,        0,     3482,        0,        0,        0,     3483,
	       0,        0,     3484,        0,        0,        0,        0,        0,        0,     3485,
	       0,     3486,        0,        0,     3487,     3488,     3489,        0,     3490,     3491,
	       0,        0,        0,     3492,        0,        0,     3493,     3494,        0,        0,
	       0,        0,     3495,        0,     3496,        0,        0,        0,        0,        0,
	    3497,     3498,     3499,        0,        0,     3500,        0,     3501,     3502,     3503,
	       0,        0,        0,        0,        0,        0,        0,     3504,        0,        0,
	       0,        0,     3505,        0,     3506,        0,        0,        0,        0,     3507,
	       0,     3508,        0,     3509,        0,        0,        0,     3510,        0,        0,
	       0,        0,     3511,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     3512,
	       0,     3513,     3514,        0,        0,     3515,     3516,        0,        0,        0,
	       0,     3517,     3518,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,     3519,     3520,        0,        0,        0,        0,        0,        0,
	    3521,        0,        0,     3522,     3523,        0,     3524,        0,     3525,        0,
	    3526,        0,     3527,        0,     3528,        0,        0,     3529,     3530,     3531,
	    3532,        0,        0,        0,        0,        0,     3533,        0,        0,        0,
	       0,     3534,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     3535,        0,     3536,        0,     3537,     3538,     3539,     3540,        0,
	    3541,     3542,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     3543,        0,        0,     3544,        0,
	       0,        0,        0,        0,     3545,     3546,        0,        0,        0,        0,
	       0,        0,     3547,        0,     3548,     3549,        0,     3550,        0,        0,
	       0,     3551,        0,        0,        0,     3552,        0,        0,     3553,        0,
	    3554,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,     3555,        0,     3556,        0,        0,        0,        0,        0,
	       0,        0,        0,     3557,     3558,     3559,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,     3560,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     3561,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     3562,        0,     3563,        0,
	    3564,        0,     3565,     3566,     3567,        0,     3568,        0,        0,        0,
	       0,        0,        0,        0,     3569,     3570,        0,        0,        0,        0,
	    3571,        0,     3572,     3573,        0,        0,        0,        0,        0,        0,
	       0,     3574,        0,        0,        0,        0,     3575,        0,        0,        0,
	       0,        0,        0,        0,        0,     3576,        0,     3577,        0,     3578,
	       0,     3579,        0,        0,        0,        0,     3580,     3581,     3582,        0,
	    3583,        0,        0,     3584,        0,        0,        0,        0,        0,     3585,
	       0,        0,     3586,     3587,        0,     3588,     3589,     3590,        0,     3591,
	    3592,     3593,     3594,     3595,        0,     3596,        0,     3597,     3598,        0,
	    3599,     3600,        0,        0,     3601,        0,     3602,        0,        0,        0,
	       0,     3603,        0,        0,     3604,        0,        0,     3605,     3606,        0,
	    3607,     3608,        0,        0,        0,        0,     3609,        0,        0,        0,
	       0,     3610,     3611,     3612,     3613,        0,        0,        0,        0,        0,
	       0,        0,        0,     3614,     3615,        0,        0,        0,     3616,        0,
	       0,        0,        0,        0,        0,        0,     3617,        0,     3618,        0,
	       0,        0,     3619,     3620,        0,        0,        0,        0,        0,     3621,
	       0,        0,        0,     3622,        0,        0,     3623,        0,        0,        0,
	       0,     3624,        0,        0,        0,     3625,        0,     3626,     3627,        0,
	    3628,        0,     3629,     3630,        0,     3631,     3632,        0,        0,     3633,
	       0,     3634,     3635,     3636,        0,     3637,     3638,     3639,        0,        0,
	       0,        0,        0,     3640,     3641,        0,     3642,     3643,     3644,     3645,
	    3646,     3647,     3648,     3649,        0,        0,     3650,     3651,        0,        0,
	    3652,     3653,     3654,        0,        0,        0,     3655,     3656,     3657,     3658,
	       0,     3659,     3660,     3661,     3662,        0,        0,     3663,        0,        0,
	       0,        0,     3664,        0,     3665,     3666,     3667,        0,     3668,        0,
	       0,     3669,        0,        0,     3670,        0,        0,        0,     3671,     3672,
	    3673,        0,     3674,     3675,     3676,        0,     3677,     3678,        0,        0,
	       0,        0,     3679,     3680,        0,        0,     3681,        0,        0,        0,
	       0,     3682,        0,        0,     3683,        0,        0,     3684,     3685,     3686,
	       0,     3687,        0,     3688,        0,        0,        0,        0,     3689,        0,
	    3690,        0,        0,        0,     3691,        0,        0,        0,        0,     3692,
	       0,     3693,     3694,     3695,        0,     3696,        0,        0,        0,     3697,
	    3698,        0,     3699,        0,     3700,     3701,     3702,        0,        0,     3703,
	    3704,     3705,        0,     3706,        0,        0,     3707,        0,        0,     3708,
	    3709,     3710,        0,        0,     3711,        0,        0,        0,     3712,        0,
	    3713,     3714,     3715,        0,        0,     3716,        0,        0,        0,        0,
	       0,        0,        0,        0,     3717,     3718,        0,        0,        0,        0,
	       0,        0,     3719,        0,     3720,        0,        0,        0,        0,        0,
	    3721,        0,     3722,        0,        0,        0,        0,     3723,        0,        0,
	       0,        0,        0,        0,        0,     3724,        0,        0,        0,     3725,
	       0,        0,        0,        0,        0,        0,        0,        0,     3726,        0,
	       0,     3727,        0,     3728,     3729,        0,        0,        0,     3730,        0,
	    3731,        0,     3732,     3733,     3734,        0,        0,     3735,        0,        0,
	    3736,        0,        0,     3737,        0,     3738,        0,        0,     3739,        0,
	       0,     3740,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,     3741,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,     3742,        0,        0,     3743,     3744,        0,     3745,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     3746,
	       0,        0,     3747,        0,        0,     3748,     3749,        0,     3750,        0,
	    3751,        0,        0,     3752,     3753,     3754,     3755,     3756,     3757,        0,
	    3758,        0,     3759,     3760,     3761,        0,        0,     3762,        0,        0,
	       0,     3763,        0,        0,        0,        0,        0,     3764,        0,        0,
	       0,        0,        0,        0,        0,        0,     3765,     3766,        0,     3767,
	    3768,        0,     3769,        0,        0,        0,     3770,     3771,        0,        0,
	    3772,        0,     3773,        0,        0,     3774,        0,        0,     3775,        0,
	       0,     3776,     3777,     3778,        0,     3779,        0,     3780,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     3781,
	    3782,        0,        0,        0,        0,     3783,     3784,        0,     3785,        0,
	       0,        0,     3786,        0,     3787,     3788,        0,        0,        0,     3789,
	       0,     3790,        0,        0,        0,        0,        0,     3791,     3792,     3793,
	    3794,        0,     3795,        0,     3796,        0,        0,        0,     3797,     3798,
	       0,     3799,        0,        0,        0,     3800,        0,        0,        0,        0,
	       0,        0,        0,        0,     3801,     3802,        0,        0,        0,        0,
	       0,        0,        0,        0,     3803,     3804,        0,        0,        0,        0,
	       0,     3805,     3806,     3807,        0,     3808,     3809,        0,     3810,        0,
	       0,     3811,     3812,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     3813,        0,        0,        0,     3814,
	    3815,     3816,     3817,        0,        0,        0,     3818,     3819,        0,        0,
	       0,        0,        0,        0,     3820,     3821,     3822,        0,        0,     3823,
	    3824,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     3825,     3826,        0,     3827,        0,        0,     3828,
	       0,     3829,        0,     3830,     3831,        0,     3832,        0,        0,        0,
	    3833,        0,        0,     3834,        0,     3835,        0,        0,        0,        0,
	       0,     3836,        0,     3837,        0,     3838,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     3839,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     3840,
	       0,        0,        0,     3841,     3842,        0,        0,        0,        0,        0,
	    3843,     3844,        0,     3845,        0,     3846,        0,     3847,        0,     3848,
	    3849,     3850,     3851,        0,        0,        0,        0,     3852,        0,        0,
	    3853,        0,     3854,        0,        0,        0,        0,     3855,     3856,     3857,
	    3858,     3859,     3860,        0,        0,        0,        0,        0,        0,     3861,
	       0,        0,     3862,     3863,        0,        0,        0,     3864,     3865,        0,
	    3866,        0,     3867,     3868,     3869,        0,     3870,        0,     3871,     3872,
	       0,     3873,        0,        0,        0,     3874,        0,     3875,     3876,     3877,
	       0,     3878,        0,        0,     3879,     3880,        0,        0,     3881,        0,
	       0,        0,     3882,        0,        0,     3883,     3884,     3885,     3886,     3887,
	    3888,        0,     3889,        0,     3890,        0,        0,        0,        0,     3891,
	       0,     3892,        0,        0,        0,        0,     3893,        0,     3894,        0,
	    3895,        0,     3896,        0,        0,     3897,     3898,     3899,        0,        0,
	       0,        0,        0,     3900,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     3901,     3902,     3903,     3904,
	    3905,        0,        0,        0,        0,        0,        0,        0,        0,     3906,
	       0,        0,        0,        0,     3907,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     3908,        0,     3909,        0,        0,
	    3910,     3911,     3912,        0,        0,     3913,        0,     3914,        0,        0,
	       0,     3915,     3916,        0,     3917,     3918,     3919,        0,        0,     3920,
	       0,     3921,        0,     3922,        0,     3923,        0,     3924,        0,        0,
	       0,        0,        0,        0,        0,        0,     3925,     3926,     3927,     3928,
	    3929,        0,        0,        0,        0,     3930,        0,        0,        0,        0,
	    3931,        0,        0,     3932,        0,        0,        0,     3933,        0,        0,
	       0,     3934,     3935,     3936,        0,        0,        0,        0,     3937,        0,
	       0,        0,     3938,     3939,        0,        0,        0,     3940,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,     3941,        0,        0,        0,     3942,        0,        0,        0,
	       0,        0,     3943,        0,     3944,        0,        0,     3945,        0,        0,
	    3946,        0,        0,        0,        0,        0,        0,     3947,        0,        0,
	       0,     3948,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     3949,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     3950,        0,        0,        0,
	       0,        0,     3951,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     3952,        0,
	       0,        0,        0,        0,     3953,     3954,     3955,        0,     3956,        0,
	    3957,     3958,        0,        0,        0,        0,     3959,        0,     3960,     3961,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     3962,
	       0,        0,     3963,     3964,        0,        0,     3965,     3966,     3967,     3968,
	    3969,     3970,     3971,        0,     3972,        0,        0,     3973,        0,     3974,
	    3975,        0,        0,        0,        0,     3976,        0,     3977,        0,        0,
	       0,        0,        0,        0,        0,     3978,     3979,        0,     3980,        0,
	       0,        0,        0,        0,     3981,     3982,        0,        0,     3983,     3984,
	       0,        0,        0,        0,     3985,        0,        0,        0,        0,     3986,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	    3987,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     3988,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     3989,        0,        0,        0,        0,
	       0,     3990,     3991,     3992,        0,     3993,     3994,        0,     3995,     3996,
	       0,     3997,        0,        0,     3998,        0,        0,     3999,        0,     4000,
	       0,     4001,        0,        0,        0,     4002,     4003,        0,        0,     4004,
	       0,        0,     4005,        0,        0,        0,        0,        0,        0,     4006,
	       0,        0,        0,     4007,     4008,        0,     4009,     4010,        0,     4011,
	       0,     4012,        0,     4013,        0,     4014,     4015,        0,        0,        0,
	       0,        0,        0,        0,     4016,        0,        0,     4017,        0,     4018,
	       0,        0,     4019,        0,     4020,     4021,        0,        0,     4022,        0,
	       0,        0,        0,     4023,        0,     4024,     4025,        0,        0,        0,
	       0,        0,     4026,        0,        0,     4027,        0,        0,     4028,        0,
	       0,        0,     4029,        0,     4030,        0,        0,     4031,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     4032,        0,        0,        0,     4033,        0,     4034,
	       0,     4035,     4036,     4037,        0,     4038,        0,        0,        0,        0,
	       0,        0,     4039,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     4040,        0,        0,        0,        0,        0,     4041,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,     4042,        0,        0,        0,     4043,        0,
	    4044,     4045,     4046,     4047,     4048,        0,        0,        0,        0,     4049,
	    4050,     4051,        0,     4052,     4053,        0,     4054,        0,     4055,        0,
	       0,        0,        0,     4056,        0,        0,        0,     4057,     4058,        0,
	       0,     4059,     4060,     4061,     4062,        0,        0,        0,     4063,     4064,
	       0,     4065,     4066,        0,        0,        0,        0,     4067,     4068,        0,
	    4069,        0,        0,        0,     4070,     4071,        0,        0,        0,        0,
	       0,     4072,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     4073,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     4074,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     4075,        0,        0,     4076,
	       0,        0,        0,        0,     4077,     4078,     4079,        0,        0,     4080,
	       0,     4081,     4082,        0,        0,        0,        0,        0,        0,     4083,
	       0,     4084,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     4085,     4086,        0,        0,     4087,        0,     4088,        0,        0,
	       0,     4089,     4090,        0,     4091,        0,     4092,        0,     4093,     4094,
	       0,        0,        0,     4095,        0,     4096,        0,        0,     4097,     4098,
	       0,        0,        0,     4099,        0,     4100,        0,        0,     4101,        0,
	    4102,        0,        0,        0,     4103,     4104,     4105,        0,     4106,        0,
	    4107,     4108,     4109,        0,     4110,        0,        0,     4111,        0,     4112,
	       0,        0,        0,        0,     4113,        0,     4114,        0,     4115,        0,
	    4116,        0,        0,        0,        0,        0,        0,     4117,        0,     4118,
	       0,     4119,        0,     4120,     4121,        0,        0,     4122,     4123,     4124,
	    4125,        0,     4126,        0,     4127,        0,     4128,        0,     4129,     4130,
	       0,     4131,        0,        0,     4132,        0,     4133,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     4134,        0,
	       0,     4135,     4136,        0,        0,     4137,        0,     4138,        0,     4139,
	       0,     4140,        0,        0,     4141,        0,     4142,        0,        0,        0,
	       0,     4143,        0,        0,        0,        0,        0,        0,        0,     4144,
	       0,        0,     4145,        0,        0,        0,     4146,     4147,        0,     4148,
	       0,        0,        0,        0,     4149,     4150,        0,        0,        0,     4151,
	    4152,        0,        0,        0,     4153,     4154,     4155,     4156,     4157,     4158,
	    4159,     4160,        0,        0,        0,     4161,        0,        0,     4162,        0,
	       0,        0,     4163,        0,        0,        0,        0,        0,        0,        0,
	       0,     4164,        0,        0,        0,        0,     4165,     4166,        0,        0,
	       0,        0,     4167,        0,        0,        0,        0,     4168,        0,     4169,
	       0,        0,        0,        0,        0,     4170,     4171,        0,        0,     4172,
	       0,        0,        0,     4173,        0,        0,        0,     4174,     4175,        0,
	       0,        0,     4176,        0,        0,        0,     4177,        0,        0,     4178,
	       0,        0,        0,        0,        0,     4179,        0,        0,        0,        0,
	    4180,        0,     4181,        0,        0,        0,        0,        0,        0,        0,
	       0,     4182,        0,        0,        0,     4183,     4184,        0,        0,     4185,
	       0,        0,        0,        0,        0,        0,     4186,        0,        0,        0,
	       0,        0,        0,        0,        0,     4187,        0,        0,        0,        0,
	       0,        0,     4188,     4189,        0,        0,     4190,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	    4191,        0,        0,        0,        0,        0,        0,     4192,     4193,     4194,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     4195,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     4196,
	    4197,        0,        0,        0,        0,        0,        0,     4198,     4199,     4200,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     4201,        0,        0,        0,        0,        0,        0,        0,     4202,
	    4203,        0,        0,     4204,        0,     4205,     4206,        0,        0,        0,
	    4207,     4208,     4209,     4210,        0,        0,     4211,        0,     4212,     4213,
	       0,        0,        0,        0,     4214,        0,        0,        0,     4215,        0,
	       0,     4216,     4217,     4218,        0,        0,        0,        0,        0,     4219,
	       0,        0,     4220,     4221,     4222,        0,        0,     4223,        0,        0,
	       0,     4224,        0,     4225,     4226,        0,     4227,        0,     4228,     4229,
	       0,        0,        0,        0,        0,        0,        0,     4230,     4231,     4232,
	       0,     4233,        0,        0,     4234,        0,        0,     4235,     4236,        0,
	       0,        0,        0,        0,        0,        0,     4237,        0,        0,        0,
	       0,        0,        0,     4238,        0,        0,        0,        0,        0,        0,
	       0,        0,     4239,        0,        0,     4240,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	    4241,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,     4242,        0,     4243,        0,        0,        0,        0,     4244,
	       0,        0,        0,     4245,        0,        0,        0,     4246,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	    4247,        0,        0,        0,        0,        0,        0,        0,        0,     4248,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     4249,        0,        0,        0,
	       0,        0,        0,        0,     4250,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     4251,        0,        0,        0,        0,
	       0,     4252,        0,        0,        0,        0,        0,     4253,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     4254,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     4255,        0,        0,        0,     4256,     4257,     4258,     4259,     4260,
	    4261,     4262,     4263,     4264,     4265,     4266,     4267,     4268,     4269,     4270,
	       0,     4271,     4272,     4273,     4274,     4275,        0,     4276,     4277,     4278,
	    4279,     4280,     4281,        0,        0,     4282,     4283,     4284,     4285,     4286,
	    4287,     4288,     4289,     4290,     4291,     4292,     4293,     4294,     4295,     4296,
	    4297,     4298,     4299,     4300,     4301,     4302,     4303,     4304,     4305,     4306,
	       0,     4307,     4308,     4309,     4310,     4311,     4312,     4313,     4314,     4315,
	    4316,     4317,     4318,     4319,        0,     4320,     4321,     4322,     4323,     4324,
	    4325,     4326,        0,     4327,     4328,     4329,     4330,     4331,     4332,     4333,
	    4334,     4335,     4336,     4337,     4338,        0,     4339,     4340,     4341,     4342,
	    4343,     4344,     4345,     4346,     4347,     4348,     4349,     4350,     4351,     4352,
	    4353,     4354,        0,     4355,     4356,     4357,     4358,     4359,        0,     4360,
	    4361,     4362,     4363,     4364,     4365,     4366,     4367,     4368,     4369,     4370,
	    4371,     4372,        0,     4373,     4374,     4375,     4376,     4377,     4378,     4379,
	    4380,     4381,     4382,     4383,     4384,     4385,     4386,     4387,     4388,     4389,
	    4390,     4391,     4392,     4393,     4394,     4395,     4396,        0,     4397,        0,
	    4398,        0,        0,        0,        0,        0,        0,        0,     4399,        0,
	    4400,     4401,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,     4402,     4403,        0,        0,     4404,     4405,        0,     4406,
	    4407,        0,     4408,        0,        0,        0,        0,     4409,        0,     4410,
	    4411,        0,        0,        0,        0,        0,     4412,     4413,     4414,        0,
	       0,        0,     4415,        0,        0,     4416,     4417,        0,     4418,        0,
	       0,        0,        0,     4419,        0,        0,        0,        0,     4420,        0,
	       0,     4421,        0,        0,        0,        0,        0,        0,        0,        0,
	    4422,        0,     4423,        0,     4424,        0,        0,        0,        0,        0,
	    4425,        0,        0,        0,        0,        0,     4426,        0,        0,     4427,
	    4428,     4429,        0,     4430,        0,        0,     4431,        0,        0,     4432,
	       0,        0,        0,        0,        0,        0,        0,     4433,     4434,        0,
	    4435,        0,        0,        0,        0,        0,     4436,     4437,        0,        0,
	    4438,     4439,        0,     4440,        0,     4441,        0,        0,        0,     4442,
	       0,        0,        0,        0,     4443,        0,     4444,        0,     4445,        0,
	       0,        0,        0,        0,     4446,     4447,        0,        0,     4448,        0,
	       0,        0,        0,        0,        0,     4449,     4450,     4451,        0,        0,
	       0,     4452,     4453,        0,        0,     4454,        0,        0,        0,        0,
	    4455,        0,     4456,     4457,        0,     4458,        0,        0,        0,        0,
	       0,        0,        0,     4459,     4460,        0,        0,        0,     4461,     4462,
	       0,     4463,     4464,     4465,     4466,        0,        0,        0,        0,     4467,
	    4468,     4469,        0,        0,     4470,        0,     4471,        0,     4472,     4473,
	    4474,     4475,     4476,     4477,        0,        0,     4478,        0,        0,        0,
	    4479,        0,     4480,        0,        0,     4481,     4482,     4483,     4484,     4485,
	    4486,        0,        0,        0,        0,        0,        0,     4487,        0,     4488,
	       0,     4489,     4490,     4491,     4492,        0,        0,     4493,        0,     4494,
	       0,     4495,        0,        0,     4496,     4497,        0,        0,     4498,        0,
	       0,        0,     4499,     4500,     4501,     4502,        0,        0,        0,        0,
	    4503,        0,     4504,        0,        0,        0,     4505,        0,     4506,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     4507,     4508,        0,        0,        0,        0,        0,
	       0,     4509,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     4510,     4511,        0,        0,     4512,
	    4513,        0,     4514,     4515,        0,     4516,        0,     4517,     4518,        0,
	       0,        0,        0,        0,        0,     4519,        0,        0,     4520,        0,
	    4521,        0,     4522,     4523,     4524,     4525,        0,     4526,     4527,     4528,
	    4529,        0,     4530,     4531,        0,        0,        0,     4532,     4533,     4534,
	       0,     4535,     4536,     4537,        0,     4538,     4539,        0,     4540,        0,
	       0,     4541,        0,        0,     4542,        0,     4543,     4544,     4545,     4546,
	    4547,     4548,     4549,     4550,     4551,        0,     4552,        0,        0,        0,
	       0,        0,     4553,     4554,     4555,        0,        0,        0,        0,        0,
	       0,        0,     4556,     4557,        0,     4558,     4559,     4560,     4561,     4562,
	    4563,        0,        0,     4564,        0,        0,     4565,     4566,        0,     4567,
	    4568,     4569,     4570,     4571,     4572,     4573,        0,     4574,     4575,     4576,
	    4577,     4578,     4579,        0,     4580,        0,     4581,        0,     4582,        0,
	    4583,     4584,        0,        0,        0,        0,     4585,        0,        0,        0,
	    4586,        0,        0,     4587,     4588,        0,        0,     4589,     4590,     4591,
	    4592,     4593,     4594,     4595,     4596,        0,     4597,        0,     4598,        0,
	    4599,        0,        0,        0,     4600,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     4601,        0,
	       0,     4602,        0,     4603,     4604,        0,        0,        0,     4605,        0,
	    4606,        0,        0,        0,        0,        0,     4607,        0,        0,        0,
	       0,        0,        0,        0,     4608,        0,     4609,        0,     4610,     4611,
	    4612,        0,        0,        0,     4613,     4614,        0,     4615,     4616,     4617,
	       0,        0,        0,     4618,     4619,        0,        0,        0,        0,        0,
	    4620,        0,        0,        0,        0,     4621,        0,     4622,        0,     4623,
	       0,        0,        0,     4624,     4625,        0,     4626,     4627,        0,        0,
	    4628,        0,        0,        0,        0,     4629,     4630,     4631,     4632,     4633,
	    4634,     4635,     4636,        0,     4637,        0,        0,        0,        0,        0,
	    4638,        0,     4639,        0,        0,        0,        0,     4640,        0,     4641,
	       0,        0,        0,        0,        0,        0,     4642,        0,        0,     4643,
	    4644,     4645,        0,        0,        0,        0,        0,     4646,        0,        0,
	    4647,        0,     4648,        0,     4649,        0,        0,        0,        0,        0,
	       0,        0,        0,     4650,        0,        0,        0,        0,        0,        0,
	    4651,     4652,        0,        0,        0,        0,     4653,     4654,     4655,     4656,
	       0,        0,     4657,        0,        0,        0,     4658,        0,     4659,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     4660,        0,        0,        0,     4661,        0,        0,     4662,        0,
	    4663,     4664,        0,        0,        0,        0,        0,     4665,     4666,        0,
	       0,        0,        0,        0,        0,     4667,     4668,        0,     4669,        0,
	    4670,     4671,     4672,        0,     4673,     4674,     4675,        0,        0,        0,
	       0,        0,     4676,     4677,        0,        0,     4678,        0,     4679,        0,
	    4680,        0,        0,        0,        0,        0,        0,     4681,     4682,        0,
	    4683,     4684,        0,     4685,     4686,     4687,        0,        0,        0,        0,
	    4688,        0,     4689,     4690,     4691,     4692,        0,     4693,     4694,     4695,
	       0,     4696,     4697,     4698,     4699,     4700,     4701,     4702,        0,     4703,
	       0,        0,     4704,        0,        0,        0,        0,        0,     4705,        0,
	       0,     4706,        0,     4707,        0,     4708,        0,        0,        0,     4709,
	       0,        0,        0,        0,        0,        0,        0,        0,     4710,        0,
	    4711,        0,        0,        0,        0,     4712,        0,        0,        0,        0,
	       0,        0,        0,        0,     4713,        0,        0,        0,        0,        0,
	    4714,     4715,     4716,        0,     4717,     4718,     4719,        0,        0,        0,
	       0,     4720,     4721,        0,        0,     4722,     4723,     4724,        0,        0,
	    4725,        0,     4726,        0,        0,        0,     4727,        0,     4728,     4729,
	       0,     4730,     4731,     4732,        0,     4733,     4734,        0,        0,        0,
	       0,     4735,     4736,     4737,        0,        0,     4738,     4739,        0,     4740,
	       0,     4741,        0,        0,     4742,     4743,     4744,        0,     4745,     4746,
	    4747,     4748,     4749,     4750,     4751,     4752,     4753,     4754,        0,     4755,
	    4756,        0,        0,     4757,     4758,     4759,        0,        0,        0,     4760,
	    4761,        0,        0,     4762,        0,        0,     4763,        0,        0,     4764,
	    4765,        0,     4766,     4767,     4768,     4769,     4770,     4771,        0,     4772,
	    4773,     4774,     4775,     4776,        0,     4777,     4778,        0,        0,     4779,
	    4780,        0,     4781,     4782,     4783,     4784,        0,     4785,     4786,     4787,
	    4788,        0,        0,        0,        0,     4789,        0,        0,        0,     4790,
	       0,     4791,        0,        0,     4792,        0,        0,     4793,        0,     4794,
	       0,     4795,        0,        0,        0,        0,        0,     4796,     4797,     4798,
	    4799,     4800,     4801,     4802,     4803,     4804,        0,        0,     4805,        0,
	    4806,     4807,        0,     4808,        0,        0,     4809,     4810,        0,     4811,
	       0,        0,     4812,     4813,     4814,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     4815,     4816,        0,        0,     4817,
	    4818,     4819,        0,     4820,        0,     4821,        0,     4822,     4823,     4824,
	    4825,        0,     4826,     4827,     4828,        0,     4829,        0,        0,        0,
	    4830,        0,        0,     4831,        0,        0,     4832,     4833,        0,     4834,
	       0,        0,        0,        0,        0,     4835,     4836,     4837,     4838,        0,
	    4839,        0,        0,        0,        0,        0,     4840,     4841,     4842,        0,
	    4843,     4844,     4845,     4846,        0,     4847,     4848,     4849,     4850,     4851,
	    4852,     4853,     4854,     4855,     4856,     4857,     4858,     4859,        0,        0,
	       0,        0,        0,        0,        0,     4860,     4861,        0,        0,     4862,
	    4863,     4864,        0,        0,        0,        0,        0,        0,        0,     4865,
	    4866,        0,        0,     4867,        0,        0,        0,        0,     4868,        0,
	       0,        0,     4869,     4870,        0,        0,        0,        0,     4871,        0,
	       0,     4872,     4873,        0,     4874,        0,     4875,        0,        0,        0,
	       0,        0,        0,        0,     4876,     4877,     4878,     4879,        0,        0,
	       0,        0,     4880,     4881,     4882,     4883,     4884,        0,     4885,     4886,
	    4887,     4888,     4889,        0,     4890,     4891,        0,        0,     4892,     4893,
	       0,        0,        0,     4894,        0,     4895,        0,        0,     4896,        0,
	    4897,        0,        0,     4898,        0,        0,        0,        0,     4899,        0,
	    4900,        0,     4901,        0,        0,        0,     4902,     4903,        0,     4904,
	    4905,     4906,        0,        0,        0,     4907,        0,        0,        0,     4908,
	    4909,        0,        0,        0,        0,        0,     4910,     4911,     4912,        0,
	       0,        0,        0,        0,     4913,     4914,        0,        0,        0,     4915,
	       0,        0,        0,     4916,        0,     4917,     4918,        0,     4919,        0,
	       0,        0,        0,     4920,     4921,     4922,     4923,     4924,        0,     4925,
	       0,        0,        0,        0,        0,        0,     4926,        0,        0,        0,
	    4927,     4928,        0,        0,        0,        0,        0,        0,     4929,     4930,
	    4931,     4932,     4933,        0,        0,        0,        0,        0,        0,        0,
	       0,     4934,        0,        0,        0,        0,        0,        0,     4935,        0,
	       0,        0,     4936,     4937,        0,        0,        0,        0,        0,        0,
	       0,        0,     4938,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     4939,        0,        0,        0,        0,        0,     4940,
	       0,     4941,     4942,     4943,     4944,        0,        0,        0,        0,     4945,
	       0,     4946,        0,        0,        0,        0,        0,     4947,        0,     4948,
	    4949,     4950,        0,        0,        0,     4951,        0,     4952,        0,     4953,
	    4954,        0,     4955,        0,     4956,        0,        0,        0,        0,        0,
	       0,        0,     4957,        0,        0,        0,        0,     4958,     4959,     4960,
	       0,     4961,     4962,        0,     4963,        0,        0,        0,        0,        0,
	       0,        0,        0,     4964,        0,     4965,        0,        0,     4966,        0,
	       0,        0,        0,     4967,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,     4968,        0,        0,
	    4969,        0,     4970,        0,        0,        0,     4971,     4972,     4973,        0,
	       0,     4974,        0,     4975,        0,     4976,        0,        0,     4977,        0,
	       0,        0,        0,     4978,     4979,        0,        0,     4980,        0,        0,
	    4981,     4982,        0,     4983,        0,        0,     4984,        0,        0,        0,
	       0,        0,        0,     4985,        0,     4986,     4987,        0,        0,     4988,
	       0,     4989,     4990,        0,        0,        0,        0,        0,     4991,        0,
	       0,        0,     4992,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,     4993,        0,        0,     4994,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	    4995,        0,        0,        0,        0,     4996,        0,     4997,        0,        0,
	       0,     4998,        0,        0,     4999,        0,        0,        0,        0,     5000,
	       0,     5001,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     5002,     5003,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     5004,     5005,     5006,     5007,     5008,     5009,     5010,
	       0,        0,        0,        0,        0,     5011,        0,        0,        0,        0,
	    5012,     5013,     5014,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,     5015,        0,        0,     5016,        0,        0,
	       0,        0,     5017,        0,        0,        0,        0,        0,     5018,        0,
	       0,        0,     5019,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,     5020,        0,     5021,        0,        0,        0,        0,     5022,
	    5023,     5024,        0,        0,     5025,        0,        0,        0,        0,        0,
	    5026,     5027,        0,     5028,        0,        0,        0,        0,        0,        0,
	       0,     5029,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     5030,     5031,        0,        0,        0,        0,        0,
	       0,        0,     5032,        0,        0,        0,     5033,        0,     5034,        0,
	       0,        0,     5035,     5036,     5037,        0,        0,        0,        0,        0,
	       0,     5038,        0,     5039,        0,        0,        0,        0,        0,        0,
	       0,     5040,        0,        0,        0,        0,        0,        0,        0,     5041,
	       0,        0,        0,        0,        0,     5042,     5043,        0,        0,     5044,
	       0,     5045,        0,        0,        0,        0,        0,        0,     5046,        0,
	       0,        0,        0,        0,        0,        0,     5047,        0,        0,        0,
	       0,     5048,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     5049,
	       0,        0,        0,     5050,        0,        0,        0,        0,        0,     5051,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     5052,        0,        0,        0,        0,     5053,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     5054,        0,     5055,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	    5056,        0,        0,        0,     5057,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     5058,     5059,     5060,     5061,     5062,        0,        0,     5063,        0,
	       0,        0,        0,        0,     5064,        0,        0,        0,     5065,        0,
	       0,        0,     5066,        0,        0,        0,        0,        0,        0,        0,
	       0,     5067,     5068,        0,     5069,        0,        0,     5070,        0,        0,
	       0,        0,        0,        0,        0,     5071,     5072,     5073,     5074,     5075,
	    5076,     5077,     5078,     5079,     5080,        0,        0,        0,        0,        0,
	       0,        0,     5081,     5082,     5083,     5084,        0,        0,        0,        0,
	       0,     5085,        0,     5086,        0,        0,        0,        0,        0,        0,
	    5087,     5088,        0,        0,        0,        0,        0,     5089,     5090,        0,
	       0,     5091,     5092,     5093,     5094,        0,     5095,        0,        0,     5096,
	    5097,     5098,        0,        0,     5099,     5100,     5101,        0,        0,        0,
	    5102,        0,        0,        0,        0,        0,     5103,        0,        0,        0,
	    5104,        0,     5105,     5106,        0,     5107,     5108,     5109,        0,        0,
	    5110,     5111,     5112,     5113,        0,        0,     5114,        0,        0,        0,
	    5115,     5116,        0,     5117,        0,        0,     5118,     5119,        0,        0,
	       0,        0,     5120,        0,        0,        0,        0,     5121,        0,        0,
	       0,     5122,     5123,        0,     5124,     5125,     5126,     5127,     5128,        0,
	       0,        0,     5129,     5130,        0,        0,        0,        0,     5131,        0,
	    5132,        0,     5133,     5134,        0,        0,        0,     5135,     5136,     5137,
	    5138,        0,        0,     5139,        0,        0,        0,        0,     5140,     5141,
	       0,     5142,        0,     5143,     5144,        0,     5145,        0,     5146,        0,
	    5147,        0,        0,     5148,     5149,     5150,        0,     5151,        0,        0,
	       0,     5152,        0,        0,        0,        0,     5153,        0,        0,     5154,
	       0,        0,     5155,        0,        0,     5156,        0,        0,        0,     5157,
	       0,        0,     5158,     5159,        0,        0,        0,        0,        0,        0,
	       0,     5160,     5161,     5162,        0,        0,     5163,        0,     5164,        0,
	       0,        0,        0,     5165,        0,        0,        0,     5166,        0,     5167,
	       0,        0,        0,        0,        0,        0,     5168,        0,        0,     5169,
	    5170,     5171,        0,        0,        0,        0,        0,        0,        0,        0,
	    5172,        0,     5173,        0,        0,        0,     5174,        0,     5175,        0,
	       0,        0,        0,     5176,     5177,     5178,     5179,        0,        0,        0,
	    5180,     5181,        0,     5182,        0,        0,     5183,        0,        0,     5184,
	       0,     5185,        0,        0,        0,        0,        0,     5186,        0,        0,
	       0,     5187,        0,        0,        0,        0,        0,        0,        0,     5188,
	       0,        0,        0,        0,        0,        0,        0,        0,     5189,        0,
	       0,     5190,     5191,     5192,        0,     5193,        0,        0,        0,     5194,
	       0,     5195,        0,        0,        0,        0,     5196,        0,        0,     5197,
	       0,        0,     5198,        0,        0,        0,        0,        0,     5199,        0,
	       0,        0,     5200,     5201,        0,        0,        0,        0,        0,     5202,
	    5203,     5204,        0,        0,        0,        0,        0,        0,        0,     5205,
	       0,        0,        0,        0,     5206,        0,        0,        0,        0,     5207,
	       0,        0,        0,        0,     5208,        0,        0,        0,     5209,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     5210,
	       0,        0,        0,        0,     5211,        0,        0,        0,        0,     5212,
	       0,        0,        0,        0,        0,        0,     5213,        0,        0,        0,
	       0,        0,        0,        0,        0,     5214,        0,     5215,     5216,        0,
	       0,        0,        0,     5217,        0,        0,        0,        0,        0,     5218,
	    5219,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     5220,        0,        0,        0,
	       0,        0,        0,     5221,        0,        0,     5222,        0,        0,        0,
	    5223,        0,        0,        0,     5224,     5225,        0,        0,        0,        0,
	       0,        0,     5226,     5227,        0,        0,        0,        0,        0,        0,
	    5228,        0,        0,     5229,        0,     5230,        0,        0,        0,        0,
	       0,        0,        0,     5231,     5232,     5233,     5234,     5235,        0,        0,
	    5236,     5237,        0,     5238,     5239,        0,     5240,        0,     5241,        0,
	    5242,        0,        0,        0,        0,     5243,        0,        0,        0,        0,
	       0,     5244,     5245,     5246,        0,     5247,     5248,        0,     5249,     5250,
	       0,        0,     5251,        0,        0,     5252,        0,     5253,        0,        0,
	       0,        0,     5254,        0,        0,        0,     5255,        0,        0,        0,
	       0,        0,     5256,        0,        0,        0,        0,        0,     5257,        0,
	    5258,        0,        0,        0,        0,        0,        0,     5259,        0,     5260,
	       0,        0,        0,     5261,        0,        0,        0,        0,        0,     5262,
	       0,        0,        0,        0,     5263,        0,        0,        0,        0,     5264,
	    5265,        0,        0,     5266,     5267,        0,        0,     5268,        0,        0,
	       0,        0,     5269,        0,        0,        0,     5270,        0,     5271,     5272,
	       0,        0,     5273,     5274,        0,        0,        0,        0,        0,     5275,
	       0,        0,     5276,     5277,     5278,     5279,        0,        0,     5280,        0,
	       0,        0,        0,        0,        0,        0,     5281,     5282,        0,     5283,
	    5284,        0,        0,        0,     5285,     5286,        0,        0,     5287,        0,
	    5288,        0,        0,        0,     5289,        0,        0,        0,        0,        0,
	       0,        0,     5290,        0,        0,        0,        0,        0,     5291,        0,
	    5292,     5293,        0,        0,        0,        0,        0,     5294,     5295,     5296,
	       0,        0,        0,        0,        0,     5297,        0,        0,        0,     5298,
	       0,        0,        0,        0,     5299,     5300,        0,        0,        0,        0,
	    5301,        0,        0,        0,     5302,        0,     5303,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,     5304,        0,        0,
	    5305,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     5306,     5307,        0,        0,
	       0,        0,        0,        0,     5308,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     5309,        0,        0,        0,     5310,
	       0,     5311,        0,     5312,        0,        0,     5313,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     5314,     5315,        0,     5316,     5317,
	    5318,     5319,     5320,     5321,     5322,     5323,     5324,        0,     5325,     5326,
	    5327,     5328,     5329,        0,        0,        0,     5330,        0,        0,        0,
	    5331,        0,     5332,        0,     5333,        0,        0,        0,        0,     5334,
	       0,     5335,     5336,        0,        0,        0,        0,     5337,        0,        0,
	       0,     5338,        0,        0,        0,     5339,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     5340,        0,
	       0,        0,        0,        0,        0,     5341,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	    5342,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	    5343,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     5344,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     5345,     5346,        0,        0,        0,
	       0,        0,        0,        0,        0,     5347,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     5348,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,     5349,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     5350,        0,        0,        0,
	       0,        0,     5351,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,     5352,     5353,     5354,     5355,     5356,     5357,
	    5358,     5359,     5360,     5361,     5362,     5363,        0,     5364,     5365,     5366,
	    5367,        0,     5368,     5369,     5370,     5371,     5372,     5373,     5374,     5375,
	    5376,        0,     5377,     5378,     5379,     5380,     5381,     5382,     5383,     5384,
	    5385,     5386,     5387,        0,     5388,     5389,     5390,     5391,     5392,     5393,
	    5394,     5395,        0,     5396,     5397,     5398,     5399,     5400,     5401,     5402,
	    5403,     5404,     5405,     5406,     5407,     5408,     5409,     5410,     5411,     5412,
	    5413,     5414,     5415,     5416,     5417,     5418,     5419,     5420,        0,     5421,
	    5422,     5423,     5424,     5425,     5426,     5427,     5428,     5429,     5430,     5431,
	    5432,     5433,     5434,     5435,     5436,     5437,     5438,     5439,     5440,     5441,
	    5442,     5443,     5444,     5445,     5446,     5447,     5448,     5449,     5450,        0,
	    5451,     5452,     5453,     5454,     5455,     5456,     5457,     5458,     5459,     5460,
	    5461,     5462,     5463,     5464,     5465,     5466,     5467,     5468,     5469,     5470,
	       0,     5471,     5472,     5473,     5474,     5475,     5476,     5477,     5478,     5479,
	    5480,     5481,     5482,     5483,     5484,     5485,     5486,     5487,     5488,     5489,
	    5490,     5491,     5492,     5493,     5494,     5495,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     5496,        0,        0,        0,        0,
	    5497,     5498,        0,     5499,        0,        0,     5500,        0,        0,        0,
	       0,        0,        0,        0,        0,     5501,        0,        0,        0,        0,
	    5502,        0,        0,        0,        0,        0,        0,     5503,     5504,        0,
	       0,        0,        0,        0,        0,        0,     5505,     5506,        0,        0,
	       0,        0,        0,        0,        0,     5507,        0,        0,        0,        0,
	    5508,     5509,     5510,        0,        0,        0,        0,        0,        0,        0,
	    5511,        0,        0,     5512,        0,        0,        0,     5513,     5514,        0,
	    5515,        0,        0,        0,        0,        0,        0,        0,     5516,        0,
	       0,        0,     5517,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     5518,     5519,     5520,        0,     5521,
	    5522,     5523,     5524,     5525,     5526,     5527,     5528,     5529,     5530,     5531,
	    5532,     5533,     5534,     5535,     5536,     5537,     5538,     5539,     5540,     5541,
	    5542,     5543,     5544,     5545,     5546,     5547,     5548,     5549,     5550,     5551,
	    5552,     5553,     5554,     5555,     5556,     5557,     5558,     5559,     5560,     5561,
	    5562,     5563,     5564,     5565,     5566,     5567,     5568,        0,        0,     5569,
	    5570,     5571,     5572,        0,     5573,     5574,     5575,     5576,     5577,     5578,
	    5579,        0,     5580,     5581,     5582,     5583,     5584,        0,     5585,     5586,
	       0,        0,        0,     5587,        0,     5588,        0,        0,     5589,        0,
	       0,     5590,     5591,     5592,     5593,     5594,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     5595,        0,        0,     5596,     5597,
	       0,        0,        0,        0,     5598,     5599,        0,        0,        0,        0,
	       0,     5600,        0,        0,     5601,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     5602,        0,        0,        0,     5603,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     5604,        0,     5605,     5606,     5607,        0,        0,
	    5608,        0,     5609,        0,     5610,        0,     5611,     5612,        0,        0,
	       0,     5613,     5614,        0,     5615,        0,        0,        0,        0,     5616,
	    5617,        0,     5618,     5619,        0,     5620,        0,        0,        0,        0,
	    5621,     5622,        0,        0,     5623,     5624,        0,     5625,     5626,     5627,
	       0,        0,        0,     5628,     5629,        0,        0,        0,     5630,        0,
	    5631,     5632,     5633,        0,        0,     5634,        0,        0,        0,     5635,
	       0,     5636,        0,     5637,     5638,     5639,     5640,     5641,        0,     5642,
	       0,        0,        0,        0,        0,        0,        0,     5643,        0,        0,
	       0,     5644,     5645,        0,     5646,        0,        0,     5647,        0,        0,
	       0,        0,     5648,        0,        0,        0,        0,        0,        0,        0,
	       0,     5649,     5650,     5651,        0,        0,     5652,     5653,        0,        0,
	       0,        0,        0,     5654,     5655,        0,     5656,        0,     5657,     5658,
	       0,     5659,        0,        0,        0,     5660,        0,        0,        0,     5661,
	    5662,        0,        0,     5663,        0,        0,     5664,     5665,     5666,        0,
	    5667,        0,        0,     5668,     5669,     5670,     5671,     5672,        0,        0,
	       0,        0,        0,     5673,     5674,        0,        0,        0,        0,        0,
	       0,     5675,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,     5676,        0,        0,     5677,        0,        0,
	    5678,     5679,        0,     5680,     5681,        0,     5682,        0,     5683,        0,
	    5684,        0,        0,        0,        0,        0,     5685,        0,        0,     5686,
	       0,     5687,        0,        0,        0,     5688,        0,     5689,        0,        0,
	       0,        0,        0,        0,        0,     5690,     5691,        0,        0,        0,
	    5692,        0,        0,        0,        0,        0,        0,        0,     5693,        0,
	    5694,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     5695,     5696,        0,        0,     5697,        0,        0,
	    5698,        0,        0,        0,        0,        0,        0,        0,     5699,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     5700,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	    5701,     5702,     5703,     5704,        0,     5705,     5706,     5707,     5708,     5709,
	    5710,     5711,     5712,     5713,     5714,     5715,     5716,     5717,     5718,     5719,
	    5720,     5721,     5722,     5723,     5724,     5725,        0,     5726,     5727,     5728,
	    5729,     5730,     5731,     5732,     5733,     5734,     5735,     5736,        0,     5737,
	    5738,     5739,     5740,     5741,        0,     5742,     5743,     5744,     5745,     5746,
	    5747,     5748,     5749,     5750,     5751,        0,     5752,     5753,        0,        0,
	       0,     5754,        0,        0,        0,        0,     5755,     5756,        0,     5757,
	       0,        0,        0,        0,     5758,     5759,        0,        0,        0,        0,
	    5760,        0,        0,     5761,        0,        0,        0,     5762,     5763,        0,
	       0,     5764,     5765,        0,     5766,     5767,        0,     5768,     5769,        0,
	       0,        0,        0,        0,     5770,        0,     5771,     5772,        0,     5773,
	    5774,     5775,        0,        0,     5776,     5777,        0,     5778,     5779,     5780,
	    5781,     5782,        0,        0,     5783,        0,     5784,     5785,     5786,        0,
	    5787,     5788,     5789,     5790,        0,     5791,     5792,        0,     5793,        0,
	       0,     5794,        0,        0,        0,     5795,     5796,     5797,        0,        0,
	       0,     5798,        0,        0,     5799,     5800,     5801,     5802,     5803,     5804,
	    5805,        0,        0,     5806,     5807,     5808,        0,     5809,        0,     5810,
	    5811,     5812,     5813,        0,     5814,        0,     5815,     5816,        0,        0,
	    5817,     5818,        0,     5819,     5820,     5821,     5822,     5823,     5824,        0,
	       0,        0,     5825,        0,        0,        0,        0,        0,        0,     5826,
	    5827,     5828,        0,        0,        0,        0,        0,     5829,     5830,        0,
	    5831,        0,        0,     5832,     5833,        0,     5834,        0,        0,     5835,
	    5836,        0,     5837,        0,        0,     5838,        0,        0,        0,        0,
	       0,     5839,        0,     5840,     5841,     5842,     5843,     5844,        0,        0,
	       0,     5845,     5846,        0,        0,     5847,        0,        0,        0,        0,
	       0,        0,     5848,     5849,        0,     5850,        0,        0,     5851,        0,
	       0,        0,        0,     5852,     5853,        0,        0,        0,        0,        0,
	    5854,     5855,        0,        0,        0,        0,        0,        0,        0,     5856,
	       0,     5857,     5858,        0,     5859,     5860,        0,        0,        0,        0,
	    5861,        0,        0,     5862,        0,        0,        0,        0,        0,     5863,
	       0,     5864,        0,     5865,        0,     5866,        0,     5867,        0,     5868,
	       0,     5869,        0,        0,        0,     5870,     5871,     5872,        0,        0,
	    5873,        0,        0,        0,     5874,        0,     5875,        0,     5876,     5877,
	    5878,     5879,        0,     5880,     5881,     5882,     5883,        0,     5884,     5885,
	    5886,     5887,        0,        0,     5888,        0,        0,     5889,        0,        0,
	    5890,     5891,        0,     5892,        0,        0,     5893,        0,        0,        0,
	    5894,     5895,     5896,     5897,        0,     5898,        0,        0,        0,     5899,
	       0,        0,        0,     5900,     5901,     5902,        0,        0,        0,     5903,
	    5904,        0,        0,        0,     5905,     5906,     5907,        0,        0,     5908,
	       0,     5909,        0,     5910,        0,        0,        0,        0,     5911,        0,
	       0,        0,     5912,        0,        0,        0,        0,     5913,     5914,        0,
	       0,        0,     5915,        0,     5916,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     5917,        0,        0,        0,        0,
	    5918,        0,        0,        0,     5919,     5920,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,     5921,        0,     5922,
	       0,        0,        0,        0,        0,        0,        0,     5923,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,     5924,        0,        0,
	    5925,        0,        0,     5926,     5927,     5928,     5929,     5930,     5931,     5932,
	    5933,        0,     5934,        0,        0,        0,        0,     5935,        0,        0,
	    5936,        0,        0,     5937,     5938,        0,        0,     5939,     5940,     5941,
	    5942,     5943,        0,        0,        0,     5944,     5945,        0,     5946,        0,
	    5947,     5948,     5949,     5950,     5951,        0,     5952,     5953,     5954,     5955,
	    5956,     5957,        0,        0,        0,     5958,     5959,     5960,        0,        0,
	       0,        0,        0,     5961,        0,     5962,        0,     5963,        0,     5964,
	    5965,     5966,        0,        0,     5967,     5968,     5969,        0,        0,        0,
	       0,        0,        0,        0,     5970,     5971,        0,        0,        0,        0,
	       0,        0,     5972,     5973,        0,        0,        0,        0,        0,        0,
	    5974,        0,        0,     5975,     5976,     5977,        0,        0,        0,        0,
	    5978,     5979,        0,        0,        0,        0,     5980,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     5981,
	       0,     5982,     5983,        0,     5984,     5985,     5986,     5987,        0,     5988,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	    5989,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,     5990,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     5991,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	    5992,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     5993,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,     5994,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     5995,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     5996,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,     5997,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     5998,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	    5999,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     6000,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     6001,     6002,     6003,     6004,     6005,     6006,     6007,     6008,     6009,
	    6010,     6011,     6012,        0,     6013,     6014,     6015,     6016,        0,     6017,
	       0,     6018,     6019,     6020,     6021,     6022,     6023,     6024,     6025,     6026,
	    6027,     6028,     6029,     6030,     6031,     6032,     6033,     6034,     6035,     6036,
	    6037,     6038,     6039,     6040,     6041,     6042,     6043,     6044,     6045,     6046,
	    6047,     6048,     6049,     6050,     6051,     6052,     6053,     6054,     6055,     6056,
	    6057,     6058,     6059,     6060,     6061,     6062,     6063,        0,     6064,     6065,
	    6066,     6067,     6068,     6069,     6070,        0,     6071,     6072,     6073,        0,
	       0,     6074,     6075,     6076,     6077,     6078,        0,     6079,     6080,     6081,
	    6082,     6083,     6084,     6085,     6086,     6087,     6088,     6089,        0,     6090,
	    6091,     6092,     6093,     6094,     6095,     6096,     6097,     6098,     6099,     6100,
	    6101,     6102,     6103,     6104,     6105,     6106,     6107,     6108,     6109,        0,
	    6110,     6111,     6112,     6113,     6114,     6115,     6116,     6117,     6118,     6119,
	    6120,     6121,     6122,     6123,     6124,     6125,     6126,     6127,     6128,     6129,
	    6130,     6131,     6132,     6133,     6134,     6135,     6136,     6137,     6138,     6139,
	    6140,     6141,        0,     6142,     6143,     6144,        0,     6145,     6146,     6147,
	    6148,     6149,     6150,        0,     6151,     6152,     6153,     6154,     6155,     6156,
	    6157,     6158,     6159,     6160,     6161,        0,     6162,     6163,     6164,     6165,
	    6166,     6167,     6168,     6169,     6170,        0,     6171,     6172,     6173,     6174,
	    6175,        0,     6176,     6177,     6178,     6179,        0,     6180,     6181,        0,
	    6182,     6183,     6184,     6185,     6186,     6187,     6188,     6189,     6190,        0,
	    6191,     6192,     6193,     6194,        0,     6195,     6196,     6197,     6198,     6199,
	       0,     6200,     6201,     6202,     6203,     6204,     6205,     6206,     6207,     6208,
	    6209,     6210,     6211,     6212,        0,     6213,     6214,     6215,     6216,     6217,
	       0,        0,     6218,        0,        0,        0,        0,        0,        0,        0,
	       0,     6219,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     6220,     6221,     6222,     6223,
	       0,     6224,     6225,     6226,     6227,     6228,     6229,     6230,     6231,     6232,
	    6233,     6234,     6235,     6236,     6237,     6238,     6239,     6240,     6241,        0,
	    6242,     6243,     6244,     6245,     6246,     6247,     6248,        0,     6249,     6250,
	    6251,     6252,     6253,     6254,     6255,     6256,     6257,     6258,     6259,        0,
	    6260,     6261,     6262,     6263,        0,     6264,     6265,        0,     6266,     6267,
	       0,     6268,        0,     6269,     6270,        0,        0,        0,        0,        0,
	       0,        0,     6271,        0,        0,        0,     6272,        0,        0,     6273,
	    6274,     6275,     6276,     6277,     6278,        0,        0,        0,        0,     6279,
	    6280,     6281,        0,     6282,     6283,        0,     6284,        0,     6285,     6286,
	    6287,     6288,     6289,     6290,        0,     6291,     6292,     6293,        0,        0,
	    6294,        0,        0,        0,     6295,     6296,        0,        0,        0,        0,
	       0,     6297,        0,        0,        0,     6298,        0,     6299,     6300,        0,
	    6301,        0,        0,     6302,     6303,     6304,     6305,        0,     6306,        0,
	       0,        0,        0,        0,     6307,        0,     6308,     6309,     6310,     6311,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     6312,     6313,        0,     6314,        0,        0,     6315,
	       0,     6316,        0,     6317,     6318,        0,        0,        0,     6319,        0,
	       0,     6320,     6321,     6322,        0,        0,     6323,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,     6324,        0,        0,
	       0,        0,        0,        0,        0,        0,     6325,        0,        0,     6326,
	       0,        0,     6327,        0,        0,     6328,        0,        0,     6329,     6330,
	    6331,        0,     6332,     6333,        0,        0,     6334,     6335,     6336,     6337,
	       0,     6338,        0,        0,     6339,     6340,     6341,     6342,        0,        0,
	    6343,        0,        0,     6344,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,     6345,        0,        0,        0,        0,        0,
	       0,        0,     6346,     6347,     6348,        0,        0,        0,        0,     6349,
	       0,        0,        0,     6350,        0,        0,     6351,     6352,        0,     6353,
	       0,        0,        0,        0,     6354,        0,     6355,     6356,        0,        0,
	    6357,        0,     6358,     6359,     6360,     6361,        0,        0,        0,     6362,
	    6363,     6364,        0,        0,        0,     6365,        0,        0,     6366,        0,
	       0,        0,        0,        0,     6367,        0,     6368,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     6369,        0,
	       0,     6370,        0,        0,     6371,        0,     6372,        0,        0,        0,
	       0,        0,     6373,     6374,        0,        0,        0,        0,     6375,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     6376,     6377,
	       0,        0,     6378,        0,        0,     6379,        0,     6380,        0,        0,
	    6381,        0,     6382,     6383,     6384,        0,        0,     6385,        0,        0,
	       0,     6386,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     6387,     6388,        0,     6389,        0,        0,        0,        0,        0,
	    6390,        0,        0,        0,        0,        0,        0,        0,        0,     6391,
	       0,        0,        0,        0,        0,     6392,        0,     6393,        0,        0,
	       0,     6394,     6395,        0,     6396,        0,        0,        0,     6397,        0,
	       0,        0,        0,        0,        0,        0,     6398,        0,        0,     6399,
	       0,        0,        0,        0,        0,        0,        0,     6400,        0,     6401,
	       0,     6402,        0,        0,     6403,        0,     6404,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,     6405,     6406,        0,     6407,
	    6408,     6409,     6410,     6411,        0,        0,        0,        0,        0,     6412,
	       0,     6413,     6414,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     6415,
	    6416,     6417,     6418,     6419,     6420,     6421,     6422,     6423,     6424,     6425,
	    6426,     6427,     6428,     6429,     6430,     6431,     6432,     6433,     6434,     6435,
	    6436,        0,     6437,     6438,        0,     6439,     6440,     6441,        0,     6442,
	    6443,        0,     6444,     6445,     6446,        0,     6447,     6448,     6449,     6450,
	    6451,     6452,     6453,     6454,     6455,        0,     6456,     6457,     6458,     6459,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     6460,        0,
	       0,     6461,     6462,     6463,        0,     6464,        0,        0,     6465,     6466,
	    6467,        0,        0,        0,     6468,     6469,        0,        0,        0,        0,
	       0,        0,        0,     6470,     6471,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     6472,        0,        0,     6473,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,     6474,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,     6475,     6476,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,     6477,        0,     6478,
	       0,     6479,     6480,     6481,     6482,     6483,     6484,     6485,     6486,     6487,
	    6488,     6489,     6490,        0,     6491,     6492,     6493,     6494,        0,        0,
	    6495,        0,     6496,     6497,        0,     6498,     6499,     6500,        0,        0,
	    6501,     6502,     6503,     6504,     6505,        0,     6506,     6507,        0,     6508,
	       0,     6509,     6510,     6511,     6512,     6513,     6514,     6515,     6516,     6517,
	    6518,     6519,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     6520,        0,        0,     6521,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,     6522,     6523,     6524,     6525,     6526,     6527,        0,     6528,
	    6529,     6530,     6531,     6532,     6533,     6534,     6535,     6536,     6537,     6538,
	    6539,     6540,     6541,     6542,     6543,        0,     6544,     6545,     6546,     6547,
	    6548,        0,     6549,     6550,     6551,        0,        0,     6552,     6553,     6554,
	    6555,     6556,        0,        0,     6557,     6558,     6559,        0,     6560,     6561,
	    6562,     6563,     6564,     6565,     6566,     6567,     6568,     6569,     6570,     6571,
	       0,     6572,     6573,        0,        0,        0,        0,        0,        0,        0,
	    6574,     6575,        0,        0,        0,        0,     6576,     6577,     6578,        0,
	    6579,        0,     6580,        0,        0,        0,     6581,     6582,     6583,        0,
	       0,     6584,        0,        0,        0,        0,        0,     6585,     6586,        0,
	       0,        0,        0,     6587,        0,     6588,        0,        0,        0,        0,
	    6589,        0,        0,        0,        0,        0,        0,     6590,        0,     6591,
	       0,        0,        0,        0,     6592,        0,        0,        0,        0,     6593,
	       0,     6594,        0,     6595,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,     6596,        0,     6597,        0,        0,        0,        0,
	       0,        0,        0,     6598,        0,        0,        0,        0,     6599,        0,
	       0,        0,        0,        0,        0,     6600,        0,        0,        0,     6601,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     6602,        0,        0,        0,     6603,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,     6604,        0,        0,
	    6605,        0,        0,        0,        0,        0,        0,        0,        0,     6606,
	    6607,        0,        0,        0,        0,     6608,     6609,     6610,     6611,     6612,
	       0,     6613,     6614,     6615,        0,        0,        0,     6616,        0,     6617,
	       0,     6618,        0,        0,     6619,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	    6620,        0,        0,     6621,        0,     6622,     6623,        0,        0,     6624,
	    6625,     6626,     6627,        0,        0,     6628,        0,     6629,     6630,        0,
	    6631,     6632,     6633,        0,     6634,     6635,        0,        0,        0,        0,
	    6636,     6637,     6638,        0,     6639,     6640,     6641,     6642,     6643,     6644,
	    6645,     6646,     6647,     6648,     6649,     6650,        0,     6651,        0,     6652,
	    6653,        0,     6654,     6655,     6656,     6657,     6658,     6659,     6660,     6661,
	    6662,        0,     6663,     6664,     6665,     6666,        0,        0,        0,        0,
	       0,     6667,     6668,     6669,     6670,     6671,        0,        0,     6672,     6673,
	    6674,     6675,     6676,     6677,     6678,        0,        0,     6679,     6680,     6681,
	    6682,     6683,     6684,     6685,        0,        0,     6686,     6687,     6688,     6689,
	       0,        0,     6690,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,     6691,
	    6692,     6693,     6694,     6695,        0,     6696,     6697,        0,     6698,     6699,
	    6700,     6701,     6702,     6703,        0,     6704,        0,     6705,     6706,     6707,
	       0,     6708,     6709,     6710,     6711,     6712,     6713,        0,        0,     6714,
	    6715,     6716,        0,     6717,     6718,     6719,     6720,     6721,     6722,     6723,
	    6724,     6725,     6726,     6727,     6728,        0,     6729,     6730,        0,     6731,
	       0,        0,        0,     6732,        0,     6733,     6734,        0,     6735,     6736,
	    6737,        0,     6738,        0,        0,        0,        0,     6739,     6740,        0,
	    6741,     6742,     6743,     6744,     6745,     6746,     6747,     6748,        0,        0,
	    6749,     6750,        0,     6751,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,     6752,     6753,        0,        0,     6754,        0,
	       0,        0,        0,     6755,     6756,        0,        0,     6757,        0,        0,
	       0,        0,        0,        0,     6758,     6759,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     6760,        0,     6761,        0,        0,
	       0,        0,        0,        0,     6762,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     6763,        0,
	       0,        0,     6764,        0,        0,     6765,        0,     6766,     6767,        0,
	       0,        0,        0,        0,     6768,        0,        0,        0,        0,     6769,
	       0,        0,        0,     6770,     6771,     6772,        0,     6773,        0,        0,
	    6774,        0,        0,        0,     6775,        0,        0,     6776,     6777,     6778,
	       0,     6779,     6780,        0,     6781,        0,        0,     6782,        0,     6783,
	       0,     6784,     6785,        0,        0,        0,        0,     6786,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,     6787,        0,     6788,
	    6789,        0,     6790,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,     6791,        0,     6792,     6793,        0,
	    6794,        0,        0,     6795,        0,        0,        0,     6796,        0,     6797,
	       0,        0,        0,        0,        0,        0,     6798,        0,     6799,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,     6800,        0,
	       0,     6801,        0,        0,        0,        0,        0,        0,        0,     6802,
	       0,     6803,        0,     6804,        0,     6805,     6806,        0,        0,        0,
	       0,        0,     6807,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,     6808,     6809,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,        0,        0,        0,
	       0,     6810,     6811,        0,        0,     6812,     6813,     6814,     6815,     6816,
	    6817,     6818,     6819,     6820,     6821,        0,        0,        0,        0,        0,
	       0,        0,        0,        0,        0,        0,        0,     6822,     6823,     6824,
	       0,        0,        0,     6825,     6826,
};

//////////////////////////////////////////////////////////////////////
// Denote a handle as being simple characters

static void
_plainHandle(ChatHandle& handle)
{
	handle &= ~WIDE_HANDLE_BIT;
}

//////////////////////////////////////////////////////////////////////
// Denote a handle as containing wide characters

static void
_wideHandle(ChatHandle& handle)
{
	handle |= WIDE_HANDLE_BIT;
}

//////////////////////////////////////////////////////////////////////
// UINT16 chatGetHandleCharacterSymbol(const wchar_t character)
//  Looks a wide character up in the character map and returns
//  the symbol # for it

UINT16
chatGetHandleCharacterSymbol(const wchar_t character)
{
	return (unsigned short int)s_wideCharacterMap[character & 0xffff];
}

//////////////////////////////////////////////////////////////////////
// Functions for encoding a string of characters as a ChatHandle

template<typename _CharType>
static void
_plainEncode(const _CharType* const callsign, ChatHandle& handle)
{
	size_t i;
	for ( i = 0; callsign[i] && i < MAX_HANDLE_WLEN; ++i )
	{
		if ( callsign[i] == 0 )
			break;
		ChatHandle code = (ChatHandle)s_characterMap[callsign[i]];
		if ( PN_UNLIKELY(code == 0ULL) )
		{
			handle = 0ULL;
			return;
		}
		handle |= code << (i * HANDLE_PLAIN_BITS);
	}
	if ( PN_UNLIKELY(callsign[i]) )
	{
		handle = 0ULL;
		return;
	}

	_plainHandle(handle);
}

static void
_wideEncode(const wchar_t* const callsign, ChatHandle& handle)
{
	ChatHandle code = 1;
	int bits = 0;
	int i;
	for ( i = 0; callsign[i] && i < MAX_WIDE_HANDLE_WCHARS; ++i, bits += HANDLE_WCHAR_BITS )
	{
		if ( callsign[i] == 0 )
		{
			_wideHandle(handle);
			return;
		}
		code = s_wideCharacterMap[(UINT16)(callsign[i] & 0xffff)];
		if ( PN_UNLIKELY(code == 0) )
		{
			handle = 0ULL;
			return;
		}
		handle |= code << bits;
	}
	if ( callsign[i] > 0 && callsign[i] < 0x100 )
	{
		code = s_characterMap[(UINT16)(callsign[i] & 0xff)];
		if ( PN_UNLIKELY(code == 0) )
		{
			handle = 0ULL;
			return;
		}
		handle |= (ChatHandle)code << bits;
		++i;
	}
	if ( PN_UNLIKELY(callsign[i]) )
	{
		handle = 0ULL;
		return;
	}

	_wideHandle(handle);
}

//////////////////////////////////////////////////////////////////////

ChatHandle
chatGetHandle(const char* const callsign)
{
	if ( PN_UNLIKELY(callsign == nullptr || *callsign == 0) )
		return 0;

	ChatHandle handle = 0;
	if ( (unsigned char)callsign[0] <= 0x7f )
		_plainEncode((const unsigned char*)callsign, handle);
	else
	{
		wchar_t wideCallsign[MAX_WIDE_HANDLE_SIZE];
		WWII::ConvertChars(callsign, wideCallsign, sizeof(wideCallsign));
		_wideEncode(wideCallsign, handle);
	}

	return handle;
}

ChatHandle
chatGetHandle(const wchar_t* const callsign)
{
	if ( PN_UNLIKELY(callsign == nullptr || *callsign == 0) )
		return 0;

	ChatHandle handle = 0;
	// Wide character callsigns require that the first
	// character be a wide character otherwise we assume
	// plain text.
	if ( (unsigned int)callsign[0] > (unsigned int)0x7f )
		_wideEncode(callsign, handle);
	else
		_plainEncode(callsign, handle);

	return handle;
}

ChatHandle
chatGetLowercaseHandle(const char* const callsign)
{
	if ( PN_UNLIKELY(callsign == nullptr || *callsign == 0) )
		return 0;

	if ( chatIsWideCallsign(callsign) )
	{
		wchar_t wideCallsign[MAX_HANDLE_WSIZE];
		WWII::ConvertChars(callsign, wideCallsign, sizeof(wideCallsign));
		int i;
		for ( i = 0; callsign[i] && i < MAX_WIDE_HANDLE_LEN; ++i )
			wideCallsign[i] = towlower(wideCallsign[i]);
		wideCallsign[i] = 0;
		return chatGetHandle(wideCallsign);
	}

	char tempCallsign[MAX_HANDLE_WSIZE];
	size_t i;
	for ( i = 0; callsign[i] && i < MAX_HANDLE_WLEN; ++i )
		tempCallsign[i] = (char)tolower(callsign[i]);
	tempCallsign[i] = 0;

	return chatGetHandle(tempCallsign);
}

ChatHandle
chatGetLowercaseHandle(const wchar_t* const callsign)
{
	if ( PN_UNLIKELY(callsign == nullptr || *callsign == 0) )
		return 0;

	wchar_t tempCallsign[MAX_HANDLE_WSIZE];
	size_t i = 0;
	for ( i = 0; callsign[i] != 0 && i < MAX_HANDLE_WLEN; ++i )
	{
		tempCallsign[i] = towlower(callsign[i]);
	}

	tempCallsign[i] = 0;

	return chatGetHandle(tempCallsign);
}

ChatHandle
chatGetLowercaseHandle(ChatHandle handle)
{
	ChatHandle newHandle = 0ULL;
	ChatHandle code = 1;
	if ( false == chatIsWideCallsign(handle) )
	{
		_plainHandle(newHandle);
		// Simple extraction of plain handle
		for ( size_t i = 0; i < MAX_HANDLE_WLEN * HANDLE_PLAIN_BITS; i += HANDLE_PLAIN_BITS )
		{
			unsigned char quad = (unsigned char)(handle & HANDLE_PLAIN_MASK);
			if ( PN_UNLIKELY(quad >= PLAIN_CHARACTERS) )
				break;
			code = s_characterMap[(unsigned int)tolower(s_characters[quad])];
			if ( code == 0 )
				break;
			newHandle |= code << i;
			handle >>= HANDLE_PLAIN_BITS;
		}
	}
	else
	{
		_wideHandle(newHandle);
		// Complex extraction of wide handle
		static const int WCHAR_BITS = MAX_WIDE_HANDLE_WCHARS * HANDLE_WCHAR_BITS;
		int i;
		for ( i = 0; i < WCHAR_BITS; i += HANDLE_WCHAR_BITS )
		{
			unsigned short int quad = (unsigned short int)(handle & HANDLE_WCHAR_MASK);
			if ( PN_UNLIKELY(quad >= Chat::WIDE_HANDLE_CHARACTERS_SUPPORTED) )
				break;
			code = s_wideCharacterMap[(unsigned short int)towlower(s_wideCharacters[quad])];
			if ( code == 0 )
				return newHandle;
			newHandle |= code << i;
			handle >>= HANDLE_WCHAR_BITS;
		}
		if ( code != 0 && i == WCHAR_BITS )
		{
			unsigned char quad = (unsigned char)((handle >> i) & HANDLE_PLAIN_MASK);
			if ( quad >= PLAIN_CHARACTERS )
				return newHandle;
			code = s_characterMap[(unsigned char)tolower(s_characters[quad])];
			newHandle |= code << i;
		}
	}
	return newHandle;
}

bool
chatGetCallsign(char* const callsign, const ChatHandle& srcHandle)
{
	ChatHandle handle = (srcHandle & ~WIDE_HANDLE_BIT);
	if ( PN_UNLIKELY(handle == 0) )
	{
		callsign[0] = 0;
		return false;
	}

	// Is it a wide handle?
	if ( handle != srcHandle )
	{
		wchar_t wideCallsign[MAX_HANDLE_WSIZE];
		if ( PN_UNLIKELY(!chatGetCallsign(wideCallsign, srcHandle)) )
			return false;
		WWII::ConvertChars(wideCallsign, callsign, sizeof(wideCallsign));
		return true;
	}

	size_t i;
	for ( i = 0; handle > 0 && i < MAX_HANDLE_WLEN; ++i )
	{
		callsign[i] = (char)s_characters[(handle & 0x3F)];
		handle >>= 6;
	}
	callsign[i] = 0;

	return true;
}

bool
chatGetCallsign(wchar_t* const callsign, const ChatHandle& srcHandle)
{
	ChatHandle handle = (srcHandle & ~WIDE_HANDLE_BIT);
	if ( PN_UNLIKELY(handle == 0) )
	{
		callsign[0] = 0;
		return false;
	}

	// plain will be true if the callsign consists only of plain (7-bit ascii) characters
	bool plain = (handle == srcHandle);
	// If the callsign is plain characters, only 6 bits are needed per character.
	int bitsPerCharacter = (plain) ? HANDLE_PLAIN_BITS : HANDLE_WCHAR_BITS;
	// The character mask filters out only the lowest X bits from each value as we shift
	unsigned int characterMask = (plain) ? (unsigned int)HANDLE_PLAIN_MASK : (unsigned int)HANDLE_WCHAR_MASK;
	// We have two distinct maps of characters
	const wchar_t* characters = (plain) ? s_characters : s_wideCharacters;

	size_t i;
	for ( i = 0; handle > 0 && i < MAX_HANDLE_WLEN; ++i )
	{
		unsigned short int quad = (unsigned short int)(handle & characterMask);
		callsign[i] = characters[quad];
		handle >>= bitsPerCharacter;
	}
	callsign[i] = 0;

	return true;
}

const char*
chatGetCallsign(const ChatHandle& handle)
{
	static const size_t NumInstances = 15;	// must be a power of 2.
	Chat::Name callsigns[NumInstances];
	static std::atomic<size_t> s_instance = 0;

	const size_t instance = (s_instance++) & NumInstances;
	callsigns[instance] = handle;

	return callsigns[instance].c_str();
}

const wchar_t*
chatGetWideCallsign(const ChatHandle& handle)
{
	static const size_t NumInstances = 15;	// must be a power of 2.
	Chat::NameW callsigns[NumInstances];
	static std::atomic<size_t> s_instance = 0;

	const size_t instance = (s_instance++) & NumInstances;
	callsigns[instance] = handle;

	return callsigns[instance].c_str();
}

bool
chatIsValidHandle(ChatHandle handle)
{
	if ( false == chatIsWideCallsign(handle) )
	{
		if ( PN_UNLIKELY(handle < ((1ULL << HANDLE_PLAIN_BITS) + (1ULL))) )
			return false;	// Less than 2 characters
		if ( PN_UNLIKELY(handle >= (1ULL << (HANDLE_PLAIN_BITS * MAX_HANDLE_WLEN))) )
			return false;
		for ( size_t i = 0; i < MAX_HANDLE_WLEN * HANDLE_PLAIN_BITS; i += HANDLE_PLAIN_BITS )
		{
			unsigned char quad = (unsigned char)(handle & HANDLE_PLAIN_MASK);
			if ( quad == 0 )
				return true;
			if ( PN_UNLIKELY(quad >= PLAIN_CHARACTERS || s_characters[quad] == 0) )
				return false;
			handle >>= HANDLE_PLAIN_BITS;
		}
	}
	else
	{
		const ChatHandle unmaskedHandle = handle & ~WIDE_HANDLE_BIT;
		if ( PN_UNLIKELY(unmaskedHandle < PLAIN_CHARACTERS) )
			return false;	// Less than 1 character
		if ( PN_UNLIKELY(unmaskedHandle >= (1ULL << ((MAX_WIDE_HANDLE_WCHARS * HANDLE_WCHAR_BITS) + HANDLE_PLAIN_BITS))) )
			return false;
		static const int WCHAR_BITS = MAX_WIDE_HANDLE_WCHARS * HANDLE_WCHAR_BITS;
		int i;
		for ( i = 0; i < WCHAR_BITS; i += HANDLE_WCHAR_BITS )
		{
			unsigned short int quad = (unsigned short int)(handle & HANDLE_WCHAR_MASK);
			if ( quad == 0 )
				return true;
			if ( PN_UNLIKELY(quad >= Chat::WIDE_HANDLE_CHARACTERS_SUPPORTED || s_wideCharacters[quad] == 0) )
				return false;
			handle >>= HANDLE_WCHAR_BITS;
		}
		unsigned char quad = (unsigned char)(handle & HANDLE_PLAIN_MASK);
		if ( quad == 0 )
			return true;
		if ( PN_UNLIKELY(quad >= PLAIN_CHARACTERS || s_characters[quad] == 0) )
			return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////
// Determine if a callsign of plain characters is valid.

template<typename _CharType>
static bool _validatePlain(const _CharType* const callsign, size_t len)
{
	if ( PN_UNLIKELY(len < 2 || len > MAX_HANDLE_WLEN) )
		return false;

	for ( size_t i = 0; i < len; ++i )
	{
		const UINT32 character = (UINT32)callsign[i];
		if ( PN_UNLIKELY(character > 0x7f || s_characterMap[character] == 0) )
			return false;
	}
	return true;
}

bool
chatIsValidCallsign(const char* const callsign)
{
	if ( true == chatIsWideCallsign(callsign) )
	{
		wchar_t wideSign[MAX_HANDLE_WSIZE];
		WWII::ConvertChars(callsign, wideSign, sizeof(wideSign));
		return chatIsValidCallsign(wideSign);
	}

	if ( PN_UNLIKELY(callsign[0] == '-') )
		return false;

	return _validatePlain((const unsigned char*)callsign, strlen(callsign));
}

bool
chatIsValidCallsign(const wchar_t* const callsign)
{
	if ( PN_UNLIKELY(callsign[0] == '-') )
		return false;

	if ( false == chatIsWideCallsign(callsign) )
		return _validatePlain(callsign, wcslen(callsign));

	int i;
	for ( i = 0; callsign[i] != 0 && i < MAX_WIDE_HANDLE_WCHARS; ++i )
	{
		if ( PN_UNLIKELY(callsign[i] >= 0xffff || s_wideCharacterMap[(unsigned short int)callsign[i]] == 0) )
			return false;
	}
	for (; callsign[i] != 0 && i < MAX_WIDE_HANDLE_LEN; ++i )
	{
		if ( PN_UNLIKELY(callsign[i] >= 0xff || s_characterMap[callsign[i]] == 0) )
			return false;
	}

	return true;
}
