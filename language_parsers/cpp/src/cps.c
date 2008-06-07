// cps.c
//

#include "cps.h"

#include <string.h>
#include "keywords.h"

// !!! if define USE_COMMENT_TOKEN, comments token will in block tokens, all cps_xxx.c need rewrite
// 
// !!! use COMMENT_TOKEN can parse var/fun/class comment, but cps_xxx need do more things on it
// 
// 
//#define USE_COMMENT_TOKEN

static MLToken* spliter_next_token(BlockSpliter* spliter, CppParser* env, gboolean skip_comment) {
	MLToken* token;
	g_assert( spliter->pos < spliter->end );

	if( spliter->policy==SPLITER_POLICY_USE_TEXT ) {
		while( (spliter->pos + 1)==spliter->end ) {
			if( spliter->end >= spliter->cap ) {
				spliter->cap = spliter->cap ? spliter->cap * 2 : 32;
				spliter->tokens = g_renew(MLToken, spliter->tokens, spliter->cap);
			}

			token = spliter->tokens + spliter->end;

#ifdef USE_COMMENT_TOKEN
			cpp_macro_lexer_next(spliter->lexer, token, &(env->macro_environ), env);
#else
			for(;;) {
				cpp_macro_lexer_next(spliter->lexer, token, &(env->macro_environ), env);
				if( token->type==TK_BLOCK_COMMENT || token->type==TK_LINE_COMMENT )
					continue;
				break;
			}
#endif
			g_assert( token->type != TK_MACRO );

			if( token->type==TK_EOF )
				return 0;

			++(spliter->end);

			if( token->type==TK_ID )
				cpp_keywords_check(token, env->keywords_table);
			else if( skip_comment && (token->type==TK_LINE_COMMENT || token->type==TK_BLOCK_COMMENT) )
				++(spliter->pos);
		}

	} else {
		if( (spliter->pos + 1)==spliter->end )
			return 0;
	}

	++(spliter->pos);
	g_assert( spliter->pos < spliter->end );

	return spliter->tokens + spliter->pos;
}

static gboolean spliter_skip_pair_round_brackets(BlockSpliter* spliter, CppParser* env) {
	MLToken* token;
	gint layer = 1;
	while( layer > 0 ) {
		if( (token = spliter_next_token(spliter, env, TRUE))==0 )
			return FALSE;

		switch( token->type ) {
		case '(':
			++layer;
			break;
		case ')':
			--layer;
			break;
		case '{':
		case '}':
		case ';':
			--(spliter->pos);
			return FALSE;
		}
	}

	return TRUE;
}

static gboolean spliter_skip_pair_angle_bracket(BlockSpliter* spliter, CppParser* env) {
	MLToken* token;
 	gint layer = 1;
 	while( layer > 0 ) {
		if( (token = spliter_next_token(spliter, env, TRUE))==0 )
			return FALSE;

		switch( token->type ) {
		case '<':
			++layer;
			break;
		case '>':
			--layer;
			break;
		case '(':
			spliter_skip_pair_round_brackets(spliter, env);
			break;
		case '{':
		case '}':
		case ';':
			--(spliter->pos);
			return FALSE;
		}
	}

	return TRUE;
}

void spliter_init_with_text(BlockSpliter* spliter, gchar* buf, gsize len, gint start_line) {
	spliter->policy = SPLITER_POLICY_USE_TEXT;
	spliter->tokens = 0;
	spliter->cap = 0;
	spliter->end = 0;
	spliter->pos = 0;
	spliter->lexer = g_new(CppLexer, 1);
	cpp_lexer_init(spliter->lexer, buf, len, start_line);
}

void spliter_init_with_tokens(BlockSpliter* spliter, MLToken* tokens, gint count) {
	spliter->policy = SPLITER_POLICY_USE_TOKENS;
	spliter->lexer = 0;
	spliter->tokens = tokens;
	spliter->cap = count;
	spliter->end = count;
	spliter->pos = 0;
}

void spliter_final(BlockSpliter* spliter) {
	if( spliter->policy==SPLITER_POLICY_USE_TEXT ) {
		g_free(spliter->tokens);
		cpp_lexer_final(spliter->lexer);
		g_free(spliter->lexer);
		spliter->tokens = 0;
		spliter->cap = 0;
		spliter->lexer = 0;
	}
}

// cps
// 
gboolean cps_var(Block* block, CppElem* parent);
gboolean cps_fun(Block* block, CppElem* parent);
gboolean cps_using(Block* block, CppElem* parent);
gboolean cps_namespace(Block* block, CppElem* parent);
gboolean cps_typedef(Block* block, CppElem* parent);
gboolean cps_template(Block* block, CppElem* parent);
gboolean cps_destruct(Block* block, CppElem* parent);
gboolean cps_operator(Block* block, CppElem* parent);
gboolean cps_extern_template(Block* block, CppElem* parent);
gboolean cps_extern_scope(Block* block, CppElem* parent);
gboolean cps_class(Block* block, CppElem* parent);
gboolean cps_enum(Block* block, CppElem* parent);
gboolean cps_block(Block* block, CppElem* parent);
gboolean cps_impl_block(Block* block, CppElem* parent);
gboolean cps_label(Block* block, CppElem* parent);
gboolean cps_fun_or_var(Block* block, CppElem* parent);
gboolean cps_breakout_block(Block* block, CppElem* parent);
gboolean cps_skip_block(Block* block, CppElem* parent);
gboolean cps_class_or_var(Block* block, CppElem* parent);

gboolean cps_fun_or_var(Block* block, CppElem* parent) {
	if( block->style==BLOCK_STYLE_BLOCK )
		return cps_fun(block, parent);

	// TODO :
	/*
	Scope tmp;
	try {
		parse_function(lexer, tmp);
		scope_insert_elems(scope, tmp.elems);
		tmp.elems.clear();

	} catch(ParseError&) {
		parse_trace("ingore parse_function, use parse_var!");
#ifdef SHOW_PARSE_DEBUG_INFOS
	std::cerr << "    FilePos  : " << lexer.block().filename() << ':' << lexer.block().start_line() << std::endl;
	std::cerr << "    ";	lexer.block().dump(std::cerr) << std::endl;
#endif
		lexer.reset();
		parse_var(lexer, scope);
	}
	return TRUE;
	*/
	return cps_fun(block, parent);
}

gboolean cps_class_or_var(Block* block, CppElem* parent) {
	/*
	Scope tmp;
	try {
		parse_class(lexer, tmp);
		scope_insert_elems(scope, tmp.elems);
		tmp.elems.clear();

	} catch(ParseError&) {
		parse_trace("ingore parse_class, use parse_var!");
#ifdef SHOW_PARSE_DEBUG_INFOS
		std::cerr << "    FilePos  : " << lexer.block().filename() << ':' << lexer.block().start_line() << std::endl;
		std::cerr << "    ";	lexer.block().dump(std::cerr) << std::endl;
#endif
		lexer.reset();
		parse_var(lexer, scope);
	}
	*/
	return TRUE;
}

// break-out sign
// 
gboolean CPS_BREAK_OUT_BLOCK(Block* block, CppElem* parent) { return TRUE; }

TParseFn spliter_next_block(BlockSpliter* spliter, Block* block) {
	MLToken* token;
	TParseFn fn = 0;
	gboolean use_template = FALSE;
	gboolean stop_with_blance = FALSE;

	if( spliter->policy==SPLITER_POLICY_USE_TEXT ) {
		spliter->end -= spliter->pos;
		g_memmove( spliter->tokens, spliter->tokens + spliter->pos, sizeof(MLToken) * spliter->end);
	}

	spliter->pos = -1;

	while( fn==0 ) {
		if( (token = spliter_next_token(spliter, block->env, TRUE))==0 )
			return 0;

		switch( token->type ) {
		case ';':
		case '=':	fn = cps_var;				break;
		case '~':	fn = cps_destruct;			stop_with_blance = TRUE;	break;
		case '{':	fn = cps_block;				stop_with_blance = TRUE;	break;
		case '}':	fn = CPS_BREAK_OUT_BLOCK;	stop_with_blance = TRUE;	break;
		case '(':	fn = use_template ? &cps_template : cps_fun_or_var;	stop_with_blance = TRUE;	break;
		case ':':
			if( spliter->pos==1 )
				fn = cps_label;
			else
				fn = cps_skip_block;
			break;
		case '<':
			if( !spliter_skip_pair_angle_bracket(spliter, block->env) )
				fn = cps_skip_block;
			break;
		case KW_EXPLICIT:	fn = cps_fun;			stop_with_blance = TRUE;	break;
		case KW_USING:		fn = cps_using;										break;
		case KW_TYPEDEF:	fn = cps_typedef;									break;
		case KW_NAMESPACE:	fn = &cps_namespace;	stop_with_blance = TRUE;	break;
		case KW_TEMPLATE:
			use_template = TRUE;
			if( (token = spliter_next_token(spliter, block->env, TRUE))==0 )
				return 0;
			else if( token->type != '>' )
				fn = cps_skip_block;
			else if( !spliter_skip_pair_angle_bracket(spliter, block->env) )
				fn = cps_skip_block;
			break;
		case KW_OPERATOR:	fn = use_template ? cps_template : cps_operator;	stop_with_blance = TRUE;	break;
		case KW_EXTERN:
			{
				if( (token = spliter_next_token(spliter, block->env, TRUE))==0 )
					return 0;

				switch( token->type ) {
				case KW_TEMPLATE:
					fn = cps_extern_template;
					break;
				case TK_STRING:
					if( (token = spliter_next_token(spliter, block->env, TRUE))==0 )
						return 0;

					if( token->type=='{' ) {
						fn = cps_extern_scope;
						stop_with_blance = TRUE;
					}
					break;
				}
			}
			break;
		case KW_STRUCT:
		case KW_CLASS:
		case KW_UNION:
			{
				gsize mark = spliter->pos;
				while( fn==0 ) {
					if( (token = spliter_next_token(spliter, block->env, TRUE))==0 )
						return 0;

					switch( token->type ) {
					case ':':
					case KW_PUBLIC:
					case KW_PROTECTED:
					case KW_PRIVATE:
					case '{':	fn = use_template ? cps_template : cps_class;			break;
					case '(':	fn = use_template ? cps_template : cps_fun_or_var;		stop_with_blance = TRUE;	break;
					case ';':	fn = use_template ? cps_template : cps_class_or_var;	break;
					case '=':	fn = cps_var;	break;
					}
				}
				spliter->pos = mark;
			}
			break;
		case KW_ENUM:
			fn = cps_enum;
			break;
		case KW_ASM:			case KW_BREAK:			case KW_CASE:
		case KW_CATCH:			case KW_CONST_CAST:		case KW_CONTINUE:
		case KW_DEFAULT:		case KW_DELETE:			case KW_DO:
		case KW_DYNAMIC_CAST:	case KW_ELSE:			case KW_FOR:
		case KW_FRIEND:			case KW_GOTO:			case KW_IF:
		case KW_NEW:			case KW_REINTERPRET_CAST:
		case KW_RETURN:			case KW_STATIC_CAST:	case KW_SWITCH:
		case KW_THIS:			case KW_THROW:			case KW_TRY:
		case KW_WHILE:
			fn = cps_skip_block;
			break;
		default:
			g_assert( token->type != TK_MACRO );
			break;
		}
	}

	block->style = BLOCK_STYLE_LINE;

	if( fn && (fn != cps_label) ) {
		gint layer = 0;
		token = spliter->tokens + spliter->pos;
		for(;;) {
			if( token->type=='{' ) {
				block->style = BLOCK_STYLE_BLOCK;
				++layer;

			} else if( token->type=='}' ) {
				block->style = BLOCK_STYLE_BLOCK;
				--layer;
				if( layer==0 && stop_with_blance )
					break;

			} else if( token->type==';' ) {
				if( layer==0 ) {
					layer = token->line;
					if( (token = spliter_next_token(spliter, block->env, FALSE))==0 )
						break;
					if( token->type==TK_BLOCK_COMMENT || token->type==TK_LINE_COMMENT )
						if( token->line==layer )
							break;
					--(spliter->pos);
					break;
				}
			}

			if( (token = spliter_next_token(spliter, block->env, TRUE))==0 )
				break;
		}
	}

	++(spliter->pos);
	block->tokens = spliter->tokens;
	block->count = spliter->pos;

	return fn;
}

void parse_scope(Block* block, CppElem* parent) {
	TParseFn fn;
	BlockSpliter spliter;

	spliter_init_with_tokens(&spliter, block->tokens, block->count);

	while( (fn = spliter_next_block(&spliter, block)) != 0 ) {
		if( fn==CPS_BREAK_OUT_BLOCK )
			break;

		(*fn)(block, 0);
	}

	spliter_final(&spliter);
}

void parse_impl_scope(Block* block, CppElem* parent) {
}

