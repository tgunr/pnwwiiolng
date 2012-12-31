// barewnd.h
//

#ifndef barewnd_HEADER

  #define barewnd_HEADER


	typedef struct tagDEMDETAIL
	{
		char	ulns;
		int		ulnsdeg;
		int		ulnsmin;
		int		ulnssec;
		char	ulew;
		int		ulewdeg;
		int		ulewmin;
		int		ulewsec;

		char	lrns;
		int		lrnsdeg;
		int		lrnsmin;
		int		lrnssec;
		char	lrew;
		int		lrewdeg;
		int		lrewmin;
		int		lrewsec;

	} DEMDETAIL, *LPDEMDETAIL;


#endif
