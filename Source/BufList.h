
/*//-----------------------------------------------------------------------------
	Niue Development Environment
	Copyright 2008 T. Lansbergen, All Rights Reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted for non-commercial use only.
	
	This software is provided ``as is'' and any express or implied warranties,
	including, but not limited to, the implied warranties of merchantability and
	fitness for a particular purpose are disclaimed. In no event shall
	authors be liable for any direct, indirect, incidental, special,
	exemplary, or consequential damages (including, but not limited to,
	procurement of substitute goods or services; loss of use, data, or profits;
	or business interruption) however caused and on any theory of liability,
	whether in contract, strict liability, or tort (including negligence or
	otherwise) arising in any way out of the use of this software, even if
	advised of the possibility of such damage. 

*///-----------------------------------------------------------------------------


#ifndef BUFLIST_H
	#define BUFLIST_H
	#include <Locker.h>

	class BList;
	class Buff;
	class NiueWindow;

	class BufList:public BLocker{
		public:
			BufList();
			virtual ~BufList();

			Buff *BuffAt(int32 index);
			char *NameAt(int32 index);
			int32 Count(){return cnt;}

			void SetBuff(int32 index,Buff *);
			void SetName(int32 index,const char*);
			void SetCount(int32 c){cnt=c;}
		private:
			CList *bf,*nl;
			int32 cnt;

	};

	extern BufList *gb; //Global Buffer List
#endif //BUFLIST_H
