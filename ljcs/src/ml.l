%option c++
%option yyclass="MLexer"
%option noyywrap

%{
/* 
 * %option debug
 * %option outfile="mlexer.cpp"
 *
 * ml.l
 * 
 */

#include "ml.h"
#include "ds_utils.h"
#include "rmacro.h"

#include <stack>
#include <sstream>
#include <fstream>

// ? 在cygwin? flex与linux下有所不同
//     1. 添加下面的using
//     2. 好像是引用多重变量会有错,把{ws}替换为[ \t]就ok
//     3. 编译时添加宏定义 -DYY_STACK_USED
// 
using std::cin;
using std::cout;
using std::cerr;

#ifndef ENABLE_ECHO
	#define ECHO
#endif

namespace {

void trim(std::string& str) {
	static const std::string SPACE_CHARS = " \t\r\n";
	
	size_t pos = str.find_last_not_of(SPACE_CHARS);
	if( pos != str.npos && pos != (str.size() - 1) )
		str.erase(pos + 1);
	pos = str.find_first_not_of(SPACE_CHARS);
	str.erase(0, pos);
}

struct Node { RMacro* rmacro; YY_BUFFER_STATE buf; };
typedef std::stack<Node> TMRStack;

class MLexer
	: public yyFlexLexer
	, public Lexer
{
public:
	MLexer(MacroMgr& env, cpp::File& file, const void* tag=0, std::istream* in=0, std::ostream* out=0, int line=1)
		: yyFlexLexer(in, out)
		, Lexer(env, file, tag)
		, in_(in)
		, line_(line)
	{
		/* set_debug(1); */
		
		emptyline = 0;
	}

	~MLexer() {
		while( !mrs_.empty() )
			finish_replace();

		if( in_ != 0 && in_ != &std::cin ) {
			delete in_;
			in_ = 0;
		}
	}

	int yylex();

private:	// Lexer implements
	virtual void do_next_token() {
		token_.word.clear();
		token_.type = yylex();
		if( token_.type != 0 ) {
			token_.line = line_;
			if( token_.word.empty() )
				token_.word.assign(yytext, yyleng);
		}
	}

private:
	void parse_macro_include(char* text, size_t len, bool sys_header);

	void start_replace(RMacro* rmacro, StrVector& args);

	void finish_replace();

public:	// for macro replace
	RMacro* find_macro( const std::string& name ) { return env_.find_macro(name); }

public:	// for macro language callback
	void on_macro_define( size_t line
		, const std::string& name
		, bool  has_args
		, const StrVector* args
		, const std::string* value
		, const std::string* rvalue );

	void on_macro_undef( size_t line, const std::string& name );

	void on_macro_include( size_t line, const std::string& file, bool sys_header );

	void on_macro_error( size_t line, const std::string& e ) {
#ifndef NDEBUG
		std::cerr << ">>>>MLEX ERROR(" << file_.filename << ":" << line << ") : " << e << std::endl;
#endif
	}

private:
	std::istream*	in_;
	size_t			line_;
	TMRStack		mrs_;

private:	// for yylex, local var
	// for macro define
	std::string 				name;
	bool						has_args;
	bool						need_rvalue;
	StrVector					args;
	std::string					value;
	std::string					rvalue;
	
	// for macro replace
	RMacro*						rmacro;
	int							layer;
	
	// for lineno
	size_t emptyline;
	
	// others
	char*	p;
	char	ch;
	size_t	oldlineno;
};

char* STR_NEW_LINE = "\n";
char* STR_SPACE = " ";

inline bool is_ws(char ch)   { return( ch==' ' || ch=='\t' ); }

#define skip_ws(p) while( ::is_ws(*p) ) { ++p; }
#define skip_id(p) while( ::isalnum(*p) || *p=='_' ) { ++p; }

struct _macro_argpos { short sign; short idx; };

#define ASSIGN_VALUES	{ \
							if( need_rvalue ) \
								rvalue.append(yytext, yyleng); \
							value.append(yytext, yyleng); \
						}

}//anonymouse namespace

/*
%x comment		多行注释
%x hcomment		处于行首的多行注?%x chr			字符
%x str			字符?%x smc			确定己是宏语句，准备开始宏(mc)
%x mc			宏语?%x mcd			宏define
%x mcdn			宏define，分析宏name
%x mcda			宏define，分析宏args
%x mcdv			宏define，分析宏value
%x mcdvc		宏define，分析宏value中的字符
%x mcdvs		宏define，分析宏value中的字符?%x smr			分析宏替换参数，查找开始符?%x mr			分析宏替换参数，分析参数及参数完成符），并调用替换函?%x mrc			分析宏替换参数，参数中的字符
%x mrs			分析宏替换参数，参数中的字符?%x cc			c++关系字词法分?*/

/*
// not generate mlexer out

#ifdef ECHO
#undef ECHO
#endif

#define ECHO
*/

%}

%x comment
%x hcomment
%x str
%x chr
%x smc
%x mc
%x mcd
%x mcdn
%x mcda
%x mcdv
%x mcdvc
%x mcdvs
%x smr
%x mr
%x mrc
%x mrs
%x cc

alpha		[_A-Za-z]
dig			[0-9]
id			{alpha}({alpha}|{dig})*
ws			[ \t]
macro		^#[ \t]*
multiline	^.*\\[ \t]*\r?\n

num1		{dig}*[lLuU]?
num2		{dig}*"."{dig}+[fF]?
num3		{dig}+"."{dig}*[fF]?
num4		0(x|X)[0-9A-Fa-f]*[uU]?[lL]?
num5		{dig}*["."]?{dig}*[eE]("-"|"+")?{dig}+[fF]?

number		{num1}|{num2}|{num3}|{num4}|{num5}

%%

^[ \t]*"/*"				BEGIN(hcomment);
<hcomment>"*/"			yy_set_bol(1);	BEGIN(INITIAL);
<hcomment>[^*\n]+		/* eat comment in chunks */
<hcomment>"*"			/* eat the lone star */
<hcomment>\n			ECHO;			++line_;
<hcomment><<EOF>>		yy_set_bol(1);	BEGIN(INITIAL);

^[ \t]+					/* trimleft */	ECHO;	yy_set_bol(1);

<*>{multiline}			{	/* meger multi line */
							p = yytext + yyleng - 1;
							while( *p != '\\' )
								--p;
							if( p != yytext ) {
								do {
									unput(*(--p));
								} while(p != yytext);
							}
							++emptyline;
							yy_set_bol(1);
						}

{macro}.*("/*"|"//").*	{	/* remove comment from MACRO line */
							value.clear();
							p = yytext;
							bool need_more_line = false;
							while( *p != '\0' ) {
								assert( *p != EOF );
								
								if( *p=='\"' || *p=='\'' ) {	// skip str or char
									ch = *p;
									do {
										++p;
										if( *p=='\\' ) { ++p; assert(*p!='\0'); ++p; }
									} while( *p!=ch && *p!='\0' );
									continue;
								}
								if( *(p++)!='/' ) continue;
								
								if( *p!='*' && *p!='/' )
									continue;
								
								// find comment, save useful text
								value.append(yytext, p-1);
								if( !value.empty() && !is_ws(value[value.size()-1]) )
									value += ' ';	// if no ws, add ws
								
								if( *(p++)=='/' ) {  yytext = p; break; }		// find (//...) comment
								
								// try to find (/*...*/) comment end sign in current line
								while( *p != '\0' ) {
									if( *(p++)!='*' ) continue;
									if( *p=='/' ) { ++p; yytext = p; break; }	// find comment end
								}
								if( *p!='\0' || yytext==p )
									continue;
								
								// try to find comment end sign more line
								bool finish = false;
								while( !finish ) {
									ch = yyinput();
									while( ch=='*' ) {
										ch = yyinput();
										if( ch=='/' ) {
											need_more_line = true;				// find comment end
											finish = true; 
										}
									}
									if( ch=='\n' )		++emptyline;
									else if( ch==EOF )	finish = true;			// not need more
								}
								yytext = p;
								break;
							}
							
							while( yytext < p )
								unput(*(--p));
							for(int i=(int)(value.size()-1); i>=0; --i )
								unput(value[i]);
							
							yy_set_bol(1);
							if( !need_more_line )
								BEGIN(smc);
						}

<INITIAL,smc>{macro}	BEGIN(mc);

<mc>define[ \r]+{id}\(.*\).*	{
							char* end = yytext + yyleng;
							p = yytext + 7;			// skip define_
							skip_ws(p);
							yytext = p;
							while( *p!='(' )   ++p;	// get name
							name.assign(yytext, p);
							
							++p;
							skip_ws(p);
							
							// parse args
							bool succeed = true;
							args.clear();
							while( succeed && *p!=')' ) {
								skip_ws(p);
								if( ::isalpha(*p) || *p=='_' ) {
									yytext = p;
									skip_id(p);	// get arg
									args.resize( args.size() + 1 );
									args.back().assign(yytext, p);
									
									skip_ws(p);
									if( *p==',' )		{ ++p; continue; }
									else if( *p==')' )	{ continue; }
								} else if( *p=='.' ) {
									yytext = p;
									while( *p=='.' ) { ++p; }	// get ...
									args.resize( args.size() + 1 );
									args.back().assign(yytext, p);
									
									skip_ws(p);
									if( *p!=')' )		{ ++p; continue; }
									else if( *p==')' )	{ continue; }
								}
								succeed = false;
							}
							
							if( succeed ) {
								++p;
								skip_ws(p);
								if( *p=='\0' ) {
									on_macro_define(line_, name, true, &args, 0, 0);
									BEGIN(INITIAL);
								} else {
									while( p < end )
										unput(*(--end));
									has_args = true;
									need_rvalue = false;
									value.clear();
									rvalue.clear();
									BEGIN(mcdv);
								}
							} else {
								on_macro_error( line_, "parse macro args failed!" );
								BEGIN(INITIAL);
							}
						}

<mc>define[ \r]+{id}.*	{
							char* end = yytext + yyleng;
							p = yytext + 7;						// skip define_
							skip_ws(p);
							yytext = p;
							skip_id(p);							// get name
							name.assign(yytext, p);

							skip_ws(p);
							if( p==end ) {
								on_macro_define(line_, name, false, 0, 0, 0);
								BEGIN(INITIAL);
							} else {
								while( p < end )
									unput(*(--end));
								has_args = false;
								args.clear();
								need_rvalue = false;
								value.clear();
								rvalue.clear();
								BEGIN(mcdv);
							}
						}

<mc>undef[ \t]+{id}.*	{
							yytext += 6;							// skip undef_
							skip_ws(yytext);
							p = yytext;
							skip_id(p);								// get name
							name.assign(yytext, p);
							on_macro_undef(line_, name);
							BEGIN(INITIAL);
						}

<mc>include[ \t]*\".+\".*	parse_macro_include(yytext, yyleng, false);
<mc>include[ \t]*\<.+\>.*	parse_macro_include(yytext, yyleng, true);
<mc>\n						unput(*yytext);	BEGIN(INITIAL);
<mc>.*						/*std::cerr << "WARN(skip macro) :  #" << yytext << std::endl;*/ BEGIN(INITIAL);
<mc><<EOF>>					BEGIN(INITIAL);

<mcdv>[ \t]*\/\/.*		/* eat line comment and the before ws */
<mcdv>[lL]?\'			ASSIGN_VALUES	BEGIN(mcdvc);
<mcdv>[lL]?\"			ASSIGN_VALUES	BEGIN(mcdvs);
<mcdv>{number}			ASSIGN_VALUES
<mcdv>[ \t]*#+[ \t]*		value.append(yytext, yyleng);

<mcdv>{id}				{
							size_t argc = args.size();
							if( need_rvalue || argc>0 )	{
								size_t pos = 0;
								while( pos < argc ) {
									if( args[pos].compare(0, args[pos].size(), yytext, yyleng)==0 )
										break;
									++pos;
								}
								
								if( pos < argc ) {
									//std::cout << "find replace arg : " << args[pos] << std::endl;
									assert( pos < 0x0000ffff );
									_macro_argpos ap = { (short)0, (short)pos };
									if( !need_rvalue ) {
										need_rvalue = true;
										rvalue = value;
									}
									size_t sharpnum = 0;
									size_t pos = rvalue.size();
									while( pos > 0 && is_ws(rvalue[pos-1]) ) --pos;
									while( pos > 0 && rvalue[pos-1]=='#' ) { --pos;	++sharpnum; }
									while( pos > 0 && is_ws(rvalue[pos-1]) ) --pos;
									
									if( sharpnum>0 )	rvalue.erase(pos);
									if( sharpnum==1 )	rvalue += '\"';
									rvalue.append( (const char*)&ap, sizeof(ap) );
									if( sharpnum==1 )	rvalue += '\"';
									
								} else if( need_rvalue ) {
									rvalue.append(yytext, yyleng);
								}
							}
							value.append(yytext, yyleng);
						}

<mcdv>.					ASSIGN_VALUES

<mcdv>\n				{
							on_macro_define(line_, name, has_args, &args, &value, need_rvalue ? &rvalue : 0);
							unput(*yytext);
							BEGIN(INITIAL);
						}

<mcdv><<EOF>>			{
							on_macro_define(line_, name, has_args, &args, &value, need_rvalue ? &rvalue : 0);
							BEGIN(INITIAL);
						}

<mcdvc>[^\\\r\n\']+		ASSIGN_VALUES
<mcdvs>[^\\\r\n\"]+		ASSIGN_VALUES
<mcdvc,mcdvs>\\.		ASSIGN_VALUES
<mcdvc>\'				ASSIGN_VALUES	BEGIN(mcdv);
<mcdvs>\"				ASSIGN_VALUES	BEGIN(mcdv);
<mcdvc,mcdvs>\n			unput(*yytext);	BEGIN(mcdv);
<mcdvc,mcdvs><<EOF>>	BEGIN(mcdv);

{id}					{
							name.assign( yytext, yyleng );
							rmacro = find_macro(name);
							if( rmacro==0 || rmacro->in_use ) {
								p = yytext + yyleng;
								while( yytext < p )
									unput(*(--p));
								BEGIN(cc);
							} else if( (*rmacro)->args!=0 ) {
								BEGIN(smr);
							} else {
								start_replace(rmacro, args);
							}
						}

<smr>[ \t]*				/* ignore */
<smr>\/\/.*				/* ignore */
<smr>"/*"				yy_push_state(comment);
<smr>\n					++emptyline;
<smr>\(					layer=1; value.clear(); args.clear();	BEGIN(mr);
<smr>.					{
							on_macro_error(line_, "smr(" + (*rmacro)->name + ") find '(' failed!");
							BEGIN(INITIAL);	token_.word = name; return TK_ID;
						}

<smr><<EOF>>			{
							on_macro_error(line_, "smr(" + (*rmacro)->name + ") find '(' eof!");
							BEGIN(INITIAL);	token_.word = name; return TK_ID;
						}

<mr>[^,\/\'\"\(\)\n]*	value.append(yytext, yyleng);
<mr>[ \t]*\/\/.*			/* eat line comment and the before ws */
<mr>[lL]?\'				value.append(yytext, yyleng);	BEGIN(mrc);
<mr>[lL]?\"				value.append(yytext, yyleng);	BEGIN(mrs);
<mr>"/*"				yy_push_state(comment);
<mr>,					{
							if( layer==1 ) {
								trim(value);
								args.push_back(value);
								value.clear();
							} else {
								value.append(yytext, yyleng);
							}
						}

<mr>\(					value.append(yytext, yyleng);	++layer;
<mr>\)					{
							--layer;
							if( layer > 0 ) {
								value.append(yytext, yyleng);
							} else {
								trim(value);
								if( !value.empty() )
									args.push_back(value);
								start_replace(rmacro, args);
								BEGIN(INITIAL);
							}
						}
<mr>.					value.append(yytext, yyleng);
<mr>\n					value.append(STR_SPACE, 1);	++emptyline;
<mr><<EOF>>				on_macro_error(line_, "parse macro args failed!");	BEGIN(INITIAL);

<mrc>[^\\\r\n\']+		value.append(yytext, yyleng);
<mrs>[^\\\r\n\"]+		value.append(yytext, yyleng);
<mrc,mrs>\\.			value.append(yytext, yyleng);
<mrc>\'					value.append(yytext, yyleng);	BEGIN(mr);
<mrs>\"					value.append(yytext, yyleng);	BEGIN(mr);
<mrc,mrs>\n				unput(*yytext);	BEGIN(mr);
<mrc,mrs><<EOF>>		BEGIN(mr);

"/*"					oldlineno = line_;					yy_push_state(comment);
<comment>"*/"			if(oldlineno!=line_) yy_set_bol(1);	yy_pop_state();
<comment>[^*\n]+		/* eat comment in chunks */
<comment>"*"			/* eat the lone star */
<comment>\n				ECHO;	++line_;
<comment><<EOF>>		yy_pop_state();

<*>\r					/* skip \r */

[ \t]*\/\/.*				/* eat line comment and the before ws */

[ \t]+$					/* trim right*/

[ \t]+					ECHO;

\n						{
							++emptyline;
							line_ += emptyline;
							while( emptyline > 0 ) {
								yytext = STR_NEW_LINE;	yyleng = 1;	ECHO;
								--emptyline;
							}
						}

<<EOF>>					{
							if( mrs_.empty() )
								yyterminate();
							else
								finish_replace();
						}

[lL]?\'					token_.word.assign(yytext, yyleng);	ECHO;	BEGIN(chr);
[lL]?\"					token_.word.assign(yytext, yyleng);	ECHO;	BEGIN(str);
<chr>[^\\\r\n\']+		token_.word.append(yytext, yyleng);	ECHO;
<str>[^\\\r\n\"]+		token_.word.append(yytext, yyleng);	ECHO;
<chr,str>\\.			token_.word.append(yytext, yyleng);	ECHO;
<chr>\'					token_.word.append(yytext, yyleng);	ECHO;	BEGIN(INITIAL);	return TK_CHAR;
<str>\"					token_.word.append(yytext, yyleng);	ECHO;	BEGIN(INITIAL);	return TK_STRING;
<chr>\n					{
							on_macro_error(line_, "parse char not find end sign!");
							++line_;	ECHO;	BEGIN(INITIAL);	return TK_CHAR;
						}
<str>\n					{
							on_macro_error(line_, "parse str not find end sign!");
							++line_;	ECHO;	BEGIN(INITIAL);	return TK_STRING;
						}

{number}				ECHO;	return TK_NUMBER;

<cc>asm					ECHO;	BEGIN(INITIAL); return KW_ASM;
<cc>auto				ECHO;	BEGIN(INITIAL); return KW_AUTO;
<cc>bool				ECHO;	BEGIN(INITIAL); return KW_BOOL;
<cc>break				ECHO;	BEGIN(INITIAL); return KW_BREAK;
<cc>case				ECHO;	BEGIN(INITIAL); return KW_CASE;
<cc>catch				ECHO;	BEGIN(INITIAL); return KW_CATCH;
<cc>char				ECHO;	BEGIN(INITIAL); return KW_CHAR;
<cc>class				ECHO;	BEGIN(INITIAL); return KW_CLASS;
<cc>const				ECHO;	BEGIN(INITIAL); return KW_CONST;
<cc>const_cast			ECHO;	BEGIN(INITIAL); return KW_CONST_CAST;
<cc>continue			ECHO;	BEGIN(INITIAL); return KW_CONTINUE;
<cc>default				ECHO;	BEGIN(INITIAL); return KW_DEFAULT;
<cc>delete				ECHO;	BEGIN(INITIAL); return KW_DELETE;
<cc>do					ECHO;	BEGIN(INITIAL); return KW_DO;
<cc>double				ECHO;	BEGIN(INITIAL); return KW_DOUBLE;
<cc>dynamic_cast		ECHO;	BEGIN(INITIAL); return KW_DYNAMIC_CAST;
<cc>else				ECHO;	BEGIN(INITIAL); return KW_ELSE;
<cc>enum				ECHO;	BEGIN(INITIAL); return KW_ENUM;
<cc>explicit			ECHO;	BEGIN(INITIAL); return KW_EXPLICIT;
<cc>export				ECHO;	BEGIN(INITIAL); return KW_EXPORT;
<cc>extern				ECHO;	BEGIN(INITIAL); return KW_EXTERN;
<cc>false				ECHO;	BEGIN(INITIAL); return KW_FALSE;
<cc>float				ECHO;	BEGIN(INITIAL); return KW_FLOAT;
<cc>for					ECHO;	BEGIN(INITIAL); return KW_FOR	;
<cc>friend				ECHO;	BEGIN(INITIAL); return KW_FRIEND;
<cc>goto				ECHO;	BEGIN(INITIAL); return KW_GOTO;
<cc>if					ECHO;	BEGIN(INITIAL); return KW_IF;
<cc>inline				ECHO;	BEGIN(INITIAL); return KW_INLINE;
<cc>int					ECHO;	BEGIN(INITIAL); return KW_INT;
<cc>long				ECHO;	BEGIN(INITIAL); return KW_LONG;
<cc>mutable				ECHO;	BEGIN(INITIAL); return KW_MUTABLE;
<cc>namespace			ECHO;	BEGIN(INITIAL); return KW_NAMESPACE;
<cc>new					ECHO;	BEGIN(INITIAL); return KW_NEW;
<cc>operator			ECHO;	BEGIN(INITIAL); return KW_OPERATOR;
<cc>private				ECHO;	BEGIN(INITIAL); return KW_PRIVATE;
<cc>protected			ECHO;	BEGIN(INITIAL); return KW_PROTECTED;
<cc>public				ECHO;	BEGIN(INITIAL); return KW_PUBLIC;
<cc>register			ECHO;	BEGIN(INITIAL); return KW_REGISTER;
<cc>reinterpret_cast	ECHO;	BEGIN(INITIAL); return KW_REINTERPRET_CAST;
<cc>return				ECHO;	BEGIN(INITIAL); return KW_RETURN;
<cc>short				ECHO;	BEGIN(INITIAL); return KW_SHORT;
<cc>signed				ECHO;	BEGIN(INITIAL); return KW_SIGNED;
<cc>sizeof				ECHO;	BEGIN(INITIAL); return KW_SIZEOF;
<cc>static				ECHO;	BEGIN(INITIAL); return KW_STATIC;
<cc>static_cast			ECHO;	BEGIN(INITIAL); return KW_STATIC_CAST;
<cc>struct				ECHO;	BEGIN(INITIAL); return KW_STRUCT;
<cc>switch				ECHO;	BEGIN(INITIAL); return KW_SWITCH;
<cc>template			ECHO;	BEGIN(INITIAL); return KW_TEMPLATE;
<cc>this				ECHO;	BEGIN(INITIAL); return KW_THIS;
<cc>throw				ECHO;	BEGIN(INITIAL); return KW_THROW;
<cc>true				ECHO;	BEGIN(INITIAL); return KW_TRUE;
<cc>try					ECHO;	BEGIN(INITIAL); return KW_TRY;
<cc>typedef				ECHO;	BEGIN(INITIAL); return KW_TYPEDEF;
<cc>typeid				ECHO;	BEGIN(INITIAL); return KW_TYPEID;
<cc>typename			ECHO;	BEGIN(INITIAL); return KW_TYPENAME;
<cc>union				ECHO;	BEGIN(INITIAL); return KW_UNION;
<cc>unsigned			ECHO;	BEGIN(INITIAL); return KW_UNSIGNED;
<cc>using				ECHO;	BEGIN(INITIAL); return KW_USING;
<cc>virtual				ECHO;	BEGIN(INITIAL); return KW_VIRTUAL;
<cc>void				ECHO;	BEGIN(INITIAL); return KW_VOID;
<cc>volatile			ECHO;	BEGIN(INITIAL); return KW_VOLATILE;
<cc>wchar_t				ECHO;	BEGIN(INITIAL); return KW_WCHAR_T;
<cc>while				ECHO;	BEGIN(INITIAL); return KW_WHILE;

<cc>{id}				ECHO;	BEGIN(INITIAL);	return TK_ID;

"::"					ECHO;	return SG_DBL_COLON;
".*"					ECHO;	return SG_DOT_STAR;
"->*"					ECHO;	return SG_PTR_STAR;
"->"					ECHO;	return SG_PTR_TO;
"##"					ECHO;	return SG_HASH_HASH;
"..."					ECHO;	return SG_ELLIPSIS;
"++"					ECHO;	return SG_INC;
"--"					ECHO;	return SG_DEC;
"!="					ECHO;	return SG_NE;
"=="					ECHO;	return SG_EQ;
"<="					ECHO;	return SG_LE;
">="					ECHO;	return SG_GE;
"+="					ECHO;	return SG_ADD_ASSIGN;
"-="					ECHO;	return SG_SUB_ASSIGN;
"*="					ECHO;	return SG_MUL_ASSIGN;
"/="					ECHO;	return SG_DIV_ASSIGN;
"%="					ECHO;	return SG_MOD_ASSIGN;
"&&"					ECHO;	return SG_AND;
"||"					ECHO;	return SG_OR;
"&="					ECHO;	return SG_BIT_AND_ASSIGN;
"|="					ECHO;	return SG_BIT_OR_ASSIGN;
"&&="					ECHO;	return SG_AND_ASSIGN;
"||="					ECHO;	return SG_OR_ASSIGN;
"^="					ECHO;	return SG_XOR_ASSIGN;
"<<"					ECHO;	return SG_LSTREAM;
">>"					ECHO;	return SG_RSTREAM;
"<<="					ECHO;	return SG_LSTREAM_ASSIGN;
">>="					ECHO;	return SG_RSTREAM_ASSIGN;

.						ECHO;	return *yytext;

%%

void MLexer::parse_macro_include(char* text, size_t len, bool sys_header) {
	char ss = sys_header ? '<' : '\"';
	char es = sys_header ? '>' : '\"';

	text += 7;				// find start pos, skip sizeof("#include")
	while( *text != ss )
		++text;
	++text;

	char* end = text;		// find end pos
	while( *end != es )
		++end;
	len = end - text;
	*end = '\0';

	if( len==0 )
		return;

	std::string filename(text, len);
	on_macro_include(line_, filename, sys_header);
}

void MLexer::start_replace(RMacro* rmacro, StrVector& args) {
	assert( rmacro!=0 && !rmacro->in_use );
	RMacro& r = *rmacro;
	if( r->value==0 )
		return;

	Node node = { rmacro, YY_CURRENT_BUFFER };
	std::istream* in = 0;
	size_t size = 0;
	
	if( r->rvalue==0 ) {
		std::string& text = *(r->value);
		//std::cout << "replace with : " << text << std::endl;
		in = new std::istringstream(text);
		
	} else {
		// replace
		std::string& text = *(r->rvalue);
		std::stringstream* ss = new std::stringstream();
		if( ss != 0 ) {
			size_t ps = 0, pe = 0, sz = text.size(), count = 0;
			while( pe < sz ) {
				if( text[pe]=='\0' ) {
					assert( (pe + sizeof(_macro_argpos))<=sz && text[pe+1]=='\0');
					count = pe - ps;
					ss->write( &text[ps], count );
					size += count;
					_macro_argpos* ap = (_macro_argpos*)(&text[pe]);
					if( (size_t)(ap->idx) < args.size() )
						*ss << args[ap->idx];
					pe += sizeof(_macro_argpos);
					ps = pe;
				} else {
					++pe;
				}
			}
			count = pe - ps;
			ss->write( &text[ps], count );
			
			in = ss;
		}
	}

	if( in==0 )
		return;
	size += 2;
	if( size > YY_BUF_SIZE )
		size = YY_BUF_SIZE;
	YY_BUFFER_STATE buf = yy_create_buffer(in, size);
	if( buf==0 ) {
		delete in;
		
	} else {
		yypush_buffer_state(buf);
		//yy_switch_to_buffer(buf);
		r.in_use = true;
		mrs_.push(node);
	}
}

void MLexer::finish_replace() {
	assert( !mrs_.empty() );
	Node& node = mrs_.top();
	delete yyin;
	//yy_delete_buffer( YY_CURRENT_BUFFER );
	//yy_switch_to_buffer( node.buf );
	yypop_buffer_state();
	node.rmacro->in_use = false;
	mrs_.pop();
}

void MLexer::on_macro_define(size_t line
	, const std::string& name
	, bool  has_args
	, const StrVector* args
	, const std::string* value
	, const std::string* rvalue )
{
	cpp::Macro* macro = cpp::Element::create<cpp::Macro>(file_, name, line, line);

	macro->decl.resize( 8 + name.size() );
	macro->decl = "#define ";
	macro->decl += name;
	
	if( has_args ) {
		macro->decl += '(';
		if( args!=0 ) {
			macro->args = new StrVector(*args);
			if( macro->args==0 )
				throw std::bad_alloc();
				
			StrVector::const_iterator it = args->begin();
			StrVector::const_iterator end = args->end();
			for( ; it!=end; ++it ) {
				macro->decl += *it;
			}
			
		} else {
			macro->set_empty_args();
		}
		macro->decl += ')';
	}

	if( value!=0 ) {
		macro->value = new std::string(*value);
		if( macro->value==0 )
			throw std::bad_alloc();
		if( value->size() < 128 ) {
			macro->decl += ' ';
			macro->decl += *(macro->value);
		}
	}

	if( rvalue!=0 ) {
		macro->rvalue = new std::string(*rvalue);
		if( macro->rvalue==0 )
			throw std::bad_alloc();
	}

	cpp::scope_insert(file_.scope, macro);
	env_.macro_insert(macro);
}

void MLexer::on_macro_undef( size_t line, const std::string& name ) {
	cpp::Undef* macro = cpp::Element::create<cpp::Undef>(file_, name, line, line);

	macro->decl.resize( 7 + name.size() );
	macro->decl = "#undef ";
	macro->decl += name;

	cpp::scope_insert(file_.scope, macro);
	env_.macro_remove(name);
}

void MLexer::on_macro_include( size_t line, const std::string& file, bool sys_header ) {
	cpp::Include* macro = cpp::Element::create<cpp::Include>(file_, "_include_", line, line);

	macro->filename = file;
	macro->sys_header = sys_header;

	macro->decl.resize( 11 + file.size() );
	macro->decl = "#include ";
	macro->decl += sys_header ? '<' : '\"';
	macro->decl += file;
	macro->decl += sys_header ? '>' : '\"';

	cpp::scope_insert(file_.scope, macro);
	env_.on_include(*macro, file_, tag_);
}

//-----------------------------------------------------------------------------
//

Lexer* Lexer::create(MacroMgr& env, cpp::File& file, std::ostream* out, const void* tag) {
	std::istream* in = new std::ifstream(file.filename.c_str());
	if( in != 0 ) {
		MLexer* lexer = new MLexer(env, file, tag, in, out);
		if( lexer != 0 )
			return lexer;
		delete in;
	}
	return 0;
}

Lexer* Lexer::create(MacroMgr& env
	, cpp::File& file
	, const char* text
	, size_t len
	, int sline
	, std::ostream* out
	, const void* tag)
{
	std::stringstream* ss = new std::stringstream();
	if( ss != 0 ) {
		ss->write(text, len);
		MLexer* lexer = new MLexer(env, file, tag, ss, out, sline);
		if( lexer != 0 )
			return lexer;
		delete ss;
	}
	return 0;
}

