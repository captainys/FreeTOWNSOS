#include "CUTIL.H"
size_t Uitoa_fl(char *str,size_t len,unsigned int data)
{
	size_t used=0;
	size_t i;
	for(i=len-1; i<len; --i)
	{
		str[i]='0'+data%10;
		data/=10;
		++used;
		if(0==data)
		{
			break;
		}
	}
	--i;
	while(i<len)
	{
		str[i]=' ';
		--i;
	}
	return used;
}
