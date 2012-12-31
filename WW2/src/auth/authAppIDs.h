#pragma once

namespace Auth
{
	namespace Apps
	{
		enum
		{
			BROWSER,				//0 open their browser to display error page
								//game down, login denied, etc
			WWIIOL,
			WILDCARDS,
			TRAVELLER,
			LOBBY,					//playnet general chat
			WWIIOL_PRESENTER,  		//5
			VERSION_UPDATER,	  	//6
			WEB_DOWNLOAD,			//7  ONLY redirect to a ftp url to d/l
			LOGIN_DENIED,			//8
			WEB_REDIRECT,			//9  all other web needs web page for d/l
			WWIIOLBETA		=	12
			//add more apIDs here
			, RapidAssaultAuth = 21
			, RapidAssaultBeta = 22
			, RapidAssaultRelease = 23
			, MAX_APPID
		} ;

		// Game configuration.
		enum { LIVE_APP_ID = WWIIOL } ;
		enum { BETA_APP_ID = WWIIOLBETA } ;
		enum { PRESENTER = WWIIOL_PRESENTER } ;
	}
}
