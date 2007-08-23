// cp.cpp
// 

#include "crt_debug.h"

#include "cp.h"

#include "cps/next_block.h"

namespace cxx_parser {

void parse(Lexer& lexer, bool& stopsign) {
	cpp::File& file = lexer.file();
	LexerEnviron env(file);
	Block block(env);
	BlockLexer blexer(block);
	TParseFn fn = 0;

	while( !stopsign && next_block(block, fn, &lexer) ) {
		assert( fn != 0 );
		blexer.reset();
		try {
			fn( blexer, file.scope );
		} catch(BreakOutError&) {

#ifdef SHOW_PARSE_DEBUG_INFOS
			std::cerr << "Error BreakOut When Parse File ("
				<< block.filename() << ':'
				<< block.start_line() << ')'
				<< std::endl;
#endif

		} catch(ParseError&) {

#ifdef SHOW_PARSE_DEBUG_INFOS
			std::cerr << "    FilePos  : " << block.filename() << ':' << block.start_line() << std::endl;
			std::cerr << "    ";	block.dump(std::cerr) << std::endl << std::endl;
#endif

		}
	}

	// test
	//file.dump(std::cout);
}

};// namespace cxx_parser

