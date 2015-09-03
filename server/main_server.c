#include "common.h"

#undef	DBG_ON
#undef	FILE_NAME	
#define	DBG_ON  			(0x01)
#define	FILE_NAME 			"main_server:"


int main(int argc,int ** argv)
{

	dbg_printf("test\n");

	return(0);
}