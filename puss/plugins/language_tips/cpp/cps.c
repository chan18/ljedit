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

static MLToken* spliter_next_token(BlockSpliter* spliter, gboolean skip_comment) {
	MLToken* token;
	gsize count;

	g_assert( spliter->pos < spliter->end );

	if( spliter->policy==SPLITER_POLICY_USE_LEXER ) {
		while( (spliter->pos + 1)==spliter->end ) {
			if( spliter->end >= spliter->cap ) {
				count = spliter->cap ? spliter->cap * 2 : 32;
				token = g_slice_alloc(sizeof(MLToken) * count);
				if( spliter->cap ) {
					memcpy(token, spliter->tokens, sizeof(MLToken)*spliter->cap);
					g_slice_free1(sizeof(MLToken)*spliter->cap, spliter->tokens);
				}
				spliter->cap = count;
				spliter->tokens = token;
			}

			token = spliter->tokens + spliter->end;

#ifdef USE_COMMENT_TOKEN
			cpp_macro_lexer_next(spliter->env, token);

#else
			for(;;) {
				cpp_macro_lexer_next(spliter->env, token);
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
				cpp_keywords_check(token, spliter->env->parser->keywords_table);
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

static gboolean spliter_skip_pair_round_brackets(BlockSpliter* spliter) {
	MLToken* token;
	gint layer = 1;
	while( layer > 0 ) {
		if( (token = spliter_next_token(spliter, TRUE))==0 )
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

static gboolean spliter_skip_pair_angle_bracket(BlockSpliter* spliter) {
	MLToken* token;
 	gint layer = 1;
 	while( layer > 0 ) {
		if( (token = spliter_next_token(spliter, TRUE))==0 )
			return FALSE;

		switch( token->type ) {
		case '<':
			++layer;
			break;
		case '>':
			--layer;
			break;
		case '(':
			spliter_skip_pair_round_brackets(spliter);
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

void spliter_init(BlockSpliter* spliter, ParseEnv* env) {
	spliter->policy = SPLITER_POLICY_USE_LEXER;
	spliter->env = env;
	spliter->tokens = 0;
	spliter->cap = 0;
	spliter->end = 0;
	spliter->pos = 0;
}

void spliter_init_with_tokens(BlockSpliter* spliter, ParseEnv* env, MLToken* tokens, gint count) {
	spliter->policy = SPLITER_POLICY_USE_TOKENS;
	spliter->env = env;
	spliter->tokens = tokens;
	spliter->cap = count;
	spliter->end = count;
	spliter->pos = 0;
}

void spliter_final(BlockSpliter* spliter) {
	if( spliter->policy==SPLITER_POLICY_USE_LEXER ) {
		g_slice_free1(sizeof(MLToken)*spliter->cap, spliter->tokens);
		spliter->tokens = 0;
		spliter->cap = 0;
		spliter->env->lexer = 0;
	}
}

gboolean cps_fun_or_var(ParseEnv* env, Block* block) {
	if( !cps_fun(env, block) )
		return cps_var(env, block);

	return TRUE;
}

static gboolean cps_class_or_var(ParseEnv* env, Block* block) {
	if( !cps_class(env, block) )
		return cps_var(env, block);

	return TRUE;
}

TParseFn spliter_next_block(BlockSpliter* spliter, Block* block) {
	MLToken* token;
	TParseFn fn = 0;
	gboolean use_template = FALSE;
	gboolean stop_with_blance = FALSE;
	gboolean already_end_sign = FALSE;
	gboolean run_sign = TRUE;

	switch( spliter->policy ) {
	case SPLITER_POLICY_USE_LEXER:
		spliter->end -= spliter->pos;
		g_memmove( spliter->tokens, spliter->tokens + spliter->pos, sizeof(MLToken) * spliter->end);
		break;

	default:
		g_assert( spliter->policy==SPLITER_POLICY_USE_TOKENS );
		spliter->tokens += spliter->pos;
		spliter->end -= spliter->pos;
	} 

	spliter->pos = -1;

	while( run_sign && fn==0 ) {
		if( (token = spliter_next_token(spliter, TRUE))==0 )
			return 0;

		switch( token->type ) {
		case ';':
		case '=':	fn = cps_var;				break;
		case '~':	fn = cps_destruct;			stop_with_blance = TRUE;	break;
		case '{':	fn = cps_block;				stop_with_blance = TRUE;	break;
		case '}':
			run_sign = FALSE;
			stop_with_blance = TRUE;
			already_end_sign = TRUE;
			break;
		case '(':	fn = use_template ? &cps_template : cps_fun_or_var;	stop_with_blance = TRUE;	break;
		case ':':
			if( spliter->pos==1 ) {
				fn = cps_label;
				already_end_sign = TRUE;
			} else {
				fn = cps_var;
			}
			break;
		case '<':
			if( !spliter_skip_pair_angle_bracket(spliter) )
				fn = cps_skip_block;
			break;
		case KW_EXPLICIT:	fn = cps_fun;			stop_with_blance = TRUE;	break;
		case KW_USING:		fn = cps_using;										break;
		case KW_TYPEDEF:	fn = cps_typedef;									break;
		case KW_NAMESPACE:	fn = &cps_namespace;	stop_with_blance = TRUE;	break;
		case KW_TEMPLATE:
			use_template = TRUE;
			if( (token = spliter_next_token(spliter, TRUE))==0 )
				return 0;
			else if( token->type != '<' )
				fn = cps_skip_block;
			else if( !spliter_skip_pair_angle_bracket(spliter) )
				fn = cps_skip_block;
			break;
		case KW_OPERATOR:	fn = use_template ? cps_template : cps_operator;	stop_with_blance = TRUE;	break;
		case KW_EXTERN:
			{
				if( (token = spliter_next_token(spliter, TRUE))==0 )
					return 0;

				switch( token->type ) {
				case KW_TEMPLATE:
					fn = cps_extern_template;
					break;
				case TK_STRING:
					if( (token = spliter_next_token(spliter, TRUE))==0 )
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
					if( (token = spliter_next_token(spliter, TRUE))==0 )
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
	// g_print("fn=%p\n", fn);

	// NOTICE : VS2010 merge functions as one function witch with same implements
	// so every cps_function MUST have different implements!!!
	// or not use function address compare!!!
	// 
	// if( (fn!=cps_label) && (fn!=CPS_BREAK_OUT_BLOCK) ) {
	if( !already_end_sign ) {
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
					if( (token = spliter_next_token(spliter, FALSE))==0 )
						break;
					if( token->type==TK_BLOCK_COMMENT || token->type==TK_LINE_COMMENT )
						if( token->line==layer )
							break;
					--(spliter->pos);
					break;
				}
			}

			if( (token = spliter_next_token(spliter, TRUE))==0 )
				break;
		}
	}

	++(spliter->pos);
	block->tokens = spliter->tokens;
	block->count = spliter->pos;

	return fn;
}

#include <assert.h>

MLToken* parse_scope(ParseEnv* env, MLToken* tokens, gsize count, CppElem* parent, gboolean use_block_end) {
	BlockSpliter spliter;
	Block block;
	TParseFn fn;
	
	assert( cpp_elem_has_subscope(parent) );

	memset(&block, 0, sizeof(block));
	block.parent = parent;

	spliter_init_with_tokens(&spliter, env, tokens, count);

	while( (fn = spliter_next_block(&spliter, &block)) != 0 ) {
		(*fn)(env, &block);
		continue;
	}

	spliter_final(&spliter);

	return use_block_end ? block.tokens : 0;
}

