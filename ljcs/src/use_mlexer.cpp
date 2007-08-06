// use_mlexer.cpp
// 

// !!!use this replace mlexer.cpp
// 

// add CRT Debug
// 
#include "crt_debug.h"


/*
yyFlexLexer::~yyFlexLexer()
{
	delete [] yy_state_buf;
	yyfree(yy_start_stack  );
	yy_delete_buffer( YY_CURRENT_BUFFER );

	// !!! Flex Memory Leak, need add this!!
	yyfree(yy_buffer_stack  );
}
*/

#include "mlexer.cpp"

