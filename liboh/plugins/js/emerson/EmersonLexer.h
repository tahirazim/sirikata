/** \file
 *  This C header file was generated by $ANTLR version 3.1.3 Mar 17, 2009 19:23:44
 *
 *     -  From the grammar source file : .//Emerson.g
 *     -                            On : 2011-01-05 01:48:35
 *     -                 for the lexer : EmersonLexerLexer *
 * Editing it, at least manually, is not wise. 
 *
 * C language generator and runtime by Jim Idle, jimi|hereisanat|idle|dotgoeshere|ws.
 *
 *
 * The lexer EmersonLexer has the callable functions (rules) shown below,
 * which will invoke the code for the associated rule in the source grammar
 * assuming that the input stream is pointing to a token/text stream that could begin
 * this rule.
 * 
 * For instance if you call the first (topmost) rule in a parser grammar, you will
 * get the results of a full parse, but calling a rule half way through the grammar will
 * allow you to pass part of a full token stream to the parser, such as for syntax checking
 * in editors and so on.
 *
 * The parser entry points are called indirectly (by function pointer to function) via
 * a parser context typedef pEmersonLexer, which is returned from a call to EmersonLexerNew().
 *
 * As this is a generated lexer, it is unlikely you will call it 'manually'. However
 * the methods are provided anyway.
 * * The methods in pEmersonLexer are  as follows:
 *
 *  -  void      pEmersonLexer->T__120(pEmersonLexer)
 *  -  void      pEmersonLexer->T__121(pEmersonLexer)
 *  -  void      pEmersonLexer->T__122(pEmersonLexer)
 *  -  void      pEmersonLexer->T__123(pEmersonLexer)
 *  -  void      pEmersonLexer->T__124(pEmersonLexer)
 *  -  void      pEmersonLexer->T__125(pEmersonLexer)
 *  -  void      pEmersonLexer->T__126(pEmersonLexer)
 *  -  void      pEmersonLexer->T__127(pEmersonLexer)
 *  -  void      pEmersonLexer->T__128(pEmersonLexer)
 *  -  void      pEmersonLexer->T__129(pEmersonLexer)
 *  -  void      pEmersonLexer->T__130(pEmersonLexer)
 *  -  void      pEmersonLexer->T__131(pEmersonLexer)
 *  -  void      pEmersonLexer->T__132(pEmersonLexer)
 *  -  void      pEmersonLexer->T__133(pEmersonLexer)
 *  -  void      pEmersonLexer->T__134(pEmersonLexer)
 *  -  void      pEmersonLexer->T__135(pEmersonLexer)
 *  -  void      pEmersonLexer->T__136(pEmersonLexer)
 *  -  void      pEmersonLexer->T__137(pEmersonLexer)
 *  -  void      pEmersonLexer->T__138(pEmersonLexer)
 *  -  void      pEmersonLexer->T__139(pEmersonLexer)
 *  -  void      pEmersonLexer->T__140(pEmersonLexer)
 *  -  void      pEmersonLexer->T__141(pEmersonLexer)
 *  -  void      pEmersonLexer->T__142(pEmersonLexer)
 *  -  void      pEmersonLexer->T__143(pEmersonLexer)
 *  -  void      pEmersonLexer->T__144(pEmersonLexer)
 *  -  void      pEmersonLexer->T__145(pEmersonLexer)
 *  -  void      pEmersonLexer->T__146(pEmersonLexer)
 *  -  void      pEmersonLexer->T__147(pEmersonLexer)
 *  -  void      pEmersonLexer->T__148(pEmersonLexer)
 *  -  void      pEmersonLexer->T__149(pEmersonLexer)
 *  -  void      pEmersonLexer->T__150(pEmersonLexer)
 *  -  void      pEmersonLexer->T__151(pEmersonLexer)
 *  -  void      pEmersonLexer->T__152(pEmersonLexer)
 *  -  void      pEmersonLexer->T__153(pEmersonLexer)
 *  -  void      pEmersonLexer->T__154(pEmersonLexer)
 *  -  void      pEmersonLexer->T__155(pEmersonLexer)
 *  -  void      pEmersonLexer->T__156(pEmersonLexer)
 *  -  void      pEmersonLexer->T__157(pEmersonLexer)
 *  -  void      pEmersonLexer->T__158(pEmersonLexer)
 *  -  void      pEmersonLexer->T__159(pEmersonLexer)
 *  -  void      pEmersonLexer->T__160(pEmersonLexer)
 *  -  void      pEmersonLexer->T__161(pEmersonLexer)
 *  -  void      pEmersonLexer->T__162(pEmersonLexer)
 *  -  void      pEmersonLexer->T__163(pEmersonLexer)
 *  -  void      pEmersonLexer->T__164(pEmersonLexer)
 *  -  void      pEmersonLexer->T__165(pEmersonLexer)
 *  -  void      pEmersonLexer->T__166(pEmersonLexer)
 *  -  void      pEmersonLexer->T__167(pEmersonLexer)
 *  -  void      pEmersonLexer->T__168(pEmersonLexer)
 *  -  void      pEmersonLexer->T__169(pEmersonLexer)
 *  -  void      pEmersonLexer->T__170(pEmersonLexer)
 *  -  void      pEmersonLexer->T__171(pEmersonLexer)
 *  -  void      pEmersonLexer->T__172(pEmersonLexer)
 *  -  void      pEmersonLexer->T__173(pEmersonLexer)
 *  -  void      pEmersonLexer->T__174(pEmersonLexer)
 *  -  void      pEmersonLexer->T__175(pEmersonLexer)
 *  -  void      pEmersonLexer->T__176(pEmersonLexer)
 *  -  void      pEmersonLexer->T__177(pEmersonLexer)
 *  -  void      pEmersonLexer->T__178(pEmersonLexer)
 *  -  void      pEmersonLexer->T__179(pEmersonLexer)
 *  -  void      pEmersonLexer->T__180(pEmersonLexer)
 *  -  void      pEmersonLexer->T__181(pEmersonLexer)
 *  -  void      pEmersonLexer->T__182(pEmersonLexer)
 *  -  void      pEmersonLexer->T__183(pEmersonLexer)
 *  -  void      pEmersonLexer->T__184(pEmersonLexer)
 *  -  void      pEmersonLexer->T__185(pEmersonLexer)
 *  -  void      pEmersonLexer->T__186(pEmersonLexer)
 *  -  void      pEmersonLexer->T__187(pEmersonLexer)
 *  -  void      pEmersonLexer->T__188(pEmersonLexer)
 *  -  void      pEmersonLexer->T__189(pEmersonLexer)
 *  -  void      pEmersonLexer->T__190(pEmersonLexer)
 *  -  void      pEmersonLexer->T__191(pEmersonLexer)
 *  -  void      pEmersonLexer->T__192(pEmersonLexer)
 *  -  void      pEmersonLexer->T__193(pEmersonLexer)
 *  -  void      pEmersonLexer->T__194(pEmersonLexer)
 *  -  void      pEmersonLexer->T__195(pEmersonLexer)
 *  -  void      pEmersonLexer->T__196(pEmersonLexer)
 *  -  void      pEmersonLexer->StringLiteral(pEmersonLexer)
 *  -  void      pEmersonLexer->DoubleStringCharacter(pEmersonLexer)
 *  -  void      pEmersonLexer->SingleStringCharacter(pEmersonLexer)
 *  -  void      pEmersonLexer->EscapeSequence(pEmersonLexer)
 *  -  void      pEmersonLexer->CharacterEscapeSequence(pEmersonLexer)
 *  -  void      pEmersonLexer->NonEscapeCharacter(pEmersonLexer)
 *  -  void      pEmersonLexer->SingleEscapeCharacter(pEmersonLexer)
 *  -  void      pEmersonLexer->EscapeCharacter(pEmersonLexer)
 *  -  void      pEmersonLexer->HexEscapeSequence(pEmersonLexer)
 *  -  void      pEmersonLexer->UnicodeEscapeSequence(pEmersonLexer)
 *  -  void      pEmersonLexer->NumericLiteral(pEmersonLexer)
 *  -  void      pEmersonLexer->HexIntegerLiteral(pEmersonLexer)
 *  -  void      pEmersonLexer->HexDigit(pEmersonLexer)
 *  -  void      pEmersonLexer->DecimalLiteral(pEmersonLexer)
 *  -  void      pEmersonLexer->DecimalDigit(pEmersonLexer)
 *  -  void      pEmersonLexer->ExponentPart(pEmersonLexer)
 *  -  void      pEmersonLexer->Identifier(pEmersonLexer)
 *  -  void      pEmersonLexer->IdentifierStart(pEmersonLexer)
 *  -  void      pEmersonLexer->IdentifierPart(pEmersonLexer)
 *  -  void      pEmersonLexer->UnicodeLetter(pEmersonLexer)
 *  -  void      pEmersonLexer->UnicodeCombiningMark(pEmersonLexer)
 *  -  void      pEmersonLexer->UnicodeDigit(pEmersonLexer)
 *  -  void      pEmersonLexer->UnicodeConnectorPunctuation(pEmersonLexer)
 *  -  void      pEmersonLexer->Comment(pEmersonLexer)
 *  -  void      pEmersonLexer->LineComment(pEmersonLexer)
 *  -  void      pEmersonLexer->LTERM(pEmersonLexer)
 *  -  void      pEmersonLexer->WhiteSpace(pEmersonLexer)
 *  -  void      pEmersonLexer->Tokens(pEmersonLexer)
 * 
 *
 * The return type for any particular rule is of course determined by the source
 * grammar file.
 */
// [The "BSD licence"]
// Copyright (c) 2005-2009 Jim Idle, Temporal Wave LLC
// http://www.temporal-wave.com
// http://www.linkedin.com/in/jimidle
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef	_EmersonLexer_H
#define _EmersonLexer_H
/* =============================================================================
 * Standard antlr3 C runtime definitions
 */
#include    <antlr3.h>

/* End of standard antlr 3 runtime definitions
 * =============================================================================
 */
 
#ifdef __cplusplus
extern "C" {
#endif

// Forward declare the context typedef so that we can use it before it is
// properly defined. Delegators and delegates (from import statements) are
// interdependent and their context structures contain pointers to each other
// C only allows such things to be declared if you pre-declare the typedef.
//
typedef struct EmersonLexer_Ctx_struct EmersonLexer, * pEmersonLexer;



#ifdef	ANTLR3_WINDOWS
// Disable: Unreferenced parameter,							- Rules with parameters that are not used
//          constant conditional,							- ANTLR realizes that a prediction is always true (synpred usually)
//          initialized but unused variable					- tree rewrite variables declared but not needed
//          Unreferenced local variable						- lexer rule declares but does not always use _type
//          potentially unitialized variable used			- retval always returned from a rule 
//			unreferenced local function has been removed	- susually getTokenNames or freeScope, they can go without warnigns
//
// These are only really displayed at warning level /W4 but that is the code ideal I am aiming at
// and the codegen must generate some of these warnings by necessity, apart from 4100, which is
// usually generated when a parser rule is given a parameter that it does not use. Mostly though
// this is a matter of orthogonality hence I disable that one.
//
#pragma warning( disable : 4100 )
#pragma warning( disable : 4101 )
#pragma warning( disable : 4127 )
#pragma warning( disable : 4189 )
#pragma warning( disable : 4505 )
#pragma warning( disable : 4701 )
#endif

/* ========================
 * BACKTRACKING IS ENABLED
 * ========================
 */

/** Context tracking structure for EmersonLexer
 */
struct EmersonLexer_Ctx_struct
{
    /** Built in ANTLR3 context tracker contains all the generic elements
     *  required for context tracking.
     */
    pANTLR3_LEXER    pLexer;


     void (*mT__120)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__121)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__122)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__123)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__124)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__125)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__126)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__127)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__128)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__129)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__130)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__131)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__132)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__133)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__134)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__135)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__136)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__137)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__138)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__139)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__140)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__141)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__142)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__143)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__144)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__145)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__146)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__147)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__148)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__149)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__150)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__151)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__152)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__153)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__154)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__155)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__156)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__157)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__158)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__159)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__160)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__161)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__162)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__163)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__164)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__165)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__166)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__167)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__168)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__169)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__170)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__171)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__172)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__173)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__174)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__175)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__176)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__177)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__178)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__179)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__180)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__181)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__182)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__183)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__184)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__185)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__186)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__187)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__188)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__189)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__190)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__191)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__192)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__193)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__194)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__195)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mT__196)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mStringLiteral)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mDoubleStringCharacter)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mSingleStringCharacter)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mEscapeSequence)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mCharacterEscapeSequence)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mNonEscapeCharacter)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mSingleEscapeCharacter)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mEscapeCharacter)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mHexEscapeSequence)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mUnicodeEscapeSequence)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mNumericLiteral)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mHexIntegerLiteral)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mHexDigit)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mDecimalLiteral)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mDecimalDigit)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mExponentPart)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mIdentifier)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mIdentifierStart)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mIdentifierPart)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mUnicodeLetter)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mUnicodeCombiningMark)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mUnicodeDigit)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mUnicodeConnectorPunctuation)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mComment)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mLineComment)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mLTERM)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mWhiteSpace)	(struct EmersonLexer_Ctx_struct * ctx);
     void (*mTokens)	(struct EmersonLexer_Ctx_struct * ctx);
    const char * (*getGrammarFileName)();
    void	    (*free)   (struct EmersonLexer_Ctx_struct * ctx);
        
};

// Function protoypes for the constructor functions that external translation units
// such as delegators and delegates may wish to call.
//
ANTLR3_API pEmersonLexer EmersonLexerNew         (pANTLR3_INPUT_STREAM instream);
ANTLR3_API pEmersonLexer EmersonLexerNewSSD      (pANTLR3_INPUT_STREAM instream, pANTLR3_RECOGNIZER_SHARED_STATE state);

/** Symbolic definitions of all the tokens that the lexer will work with.
 * \{
 *
 * Antlr will define EOF, but we can't use that as it it is too common in
 * in C header files and that would be confusing. There is no way to filter this out at the moment
 * so we just undef it here for now. That isn't the value we get back from C recognizers
 * anyway. We are looking for ANTLR3_TOKEN_EOF.
 */
#ifdef	EOF
#undef	EOF
#endif
#ifdef	Tokens
#undef	Tokens
#endif 
#define T__159      159
#define T__158      158
#define MOD      68
#define T__160      160
#define DO      14
#define LEFT_SHIFT_ASSIGN      40
#define COND_EXPR_NOIN      88
#define NOT      80
#define TRIPLE_SHIFT      63
#define T__167      167
#define T__168      168
#define EOF      -1
#define T__165      165
#define T__166      166
#define T__163      163
#define UNARY_PLUS      77
#define T__164      164
#define T__161      161
#define T__162      162
#define SingleStringCharacter      98
#define T__148      148
#define T__147      147
#define T__149      149
#define INSTANCE_OF      59
#define RETURN      23
#define UnicodeLetter      113
#define WhiteSpace      119
#define MESSAGE_SEND      91
#define T__154      154
#define T__155      155
#define T__156      156
#define T__157      157
#define T__150      150
#define T__151      151
#define T__152      152
#define UnicodeCombiningMark      116
#define T__153      153
#define T__139      139
#define LTERM      93
#define T__138      138
#define T__137      137
#define UnicodeDigit      114
#define T__136      136
#define NumericLiteral      96
#define UNARY_MINUS      78
#define DoubleStringCharacter      97
#define T__141      141
#define T__142      142
#define T__140      140
#define T__145      145
#define T__146      146
#define T__143      143
#define T__144      144
#define T__126      126
#define T__125      125
#define T__128      128
#define T__127      127
#define T__129      129
#define TYPEOF      74
#define COND_EXPR      87
#define LESS_THAN      55
#define COMPLEMENT      79
#define NAME_VALUE      71
#define LEFT_SHIFT      61
#define CALL      5
#define CharacterEscapeSequence      100
#define T__130      130
#define T__131      131
#define T__132      132
#define T__133      133
#define T__134      134
#define T__135      135
#define PLUSPLUS      75
#define T__124      124
#define T__123      123
#define T__122      122
#define SUB      65
#define T__121      121
#define T__120      120
#define HexDigit      107
#define NOT_EQUALS      52
#define RIGHT_SHIFT_ASSIGN      41
#define ARRAY_INDEX      6
#define SLIST      9
#define IDENT      53
#define ADD      64
#define GREATER_THAN      56
#define EXP_ASSIGN      44
#define UnicodeEscapeSequence      102
#define FUNC_DECL      83
#define NOT_IDENT      54
#define StringLiteral      95
#define OR_ASSIGN      45
#define FORCOND      18
#define HexIntegerLiteral      109
#define NonEscapeCharacter      104
#define LESS_THAN_EQUAL      57
#define DIV      67
#define SUB_ASSIGN      39
#define OBJ_LITERAL      70
#define WHILE      15
#define MOD_ASSIGN      37
#define CASE      32
#define NEW      25
#define MINUSMINUS      76
#define ARGLIST      85
#define EQUALS      51
#define ARRAY_LITERAL      69
#define FUNC_EXPR      84
#define DecimalDigit      106
#define DIV_ASSIGN      36
#define BREAK      21
#define Identifier      94
#define BIT_OR      48
#define Comment      117
#define EXP      49
#define SingleEscapeCharacter      103
#define ExponentPart      110
#define VAR      12
#define VOID      73
#define FORINIT      17
#define GREATER_THAN_EQUAL      58
#define ADD_ASSIGN      38
#define SWITCH      31
#define IdentifierStart      111
#define FUNC_PARAMS      82
#define DELETE      72
#define MULT      66
#define EMPTY_FUNC_BODY      90
#define TRY      26
#define FUNC      8
#define OR      46
#define VARLIST      11
#define CATCH      28
#define MESSAGE_RECV      92
#define EscapeSequence      99
#define THROW      27
#define UnicodeConnectorPunctuation      115
#define BIT_AND      50
#define HexEscapeSequence      101
#define MULT_ASSIGN      35
#define LineComment      118
#define FOR      16
#define AND      47
#define AND_ASSIGN      43
#define IF      10
#define EXPR_LIST      86
#define PROG      13
#define T__196      196
#define IN      60
#define T__195      195
#define T__194      194
#define T__193      193
#define CONTINUE      22
#define T__192      192
#define T__191      191
#define T__190      190
#define FORITER      19
#define RIGHT_SHIFT      62
#define EscapeCharacter      105
#define UNDEF      4
#define DOT      7
#define TERNARYOP      89
#define IdentifierPart      112
#define WITH      24
#define T__184      184
#define T__183      183
#define T__186      186
#define T__185      185
#define T__188      188
#define T__187      187
#define T__189      189
#define T__180      180
#define T__182      182
#define DEFAULT      30
#define T__181      181
#define POSTEXPR      81
#define TRIPLE_SHIFT_ASSIGN      42
#define FORIN      20
#define DecimalLiteral      108
#define T__175      175
#define T__174      174
#define T__173      173
#define T__172      172
#define T__179      179
#define T__178      178
#define T__177      177
#define FINALLY      29
#define T__176      176
#define LABEL      33
#define T__171      171
#define T__170      170
#define ASSIGN      34
#define T__169      169
#ifdef	EOF
#undef	EOF
#define	EOF	ANTLR3_TOKEN_EOF
#endif

#ifndef TOKENSOURCE
#define TOKENSOURCE(lxr) lxr->pLexer->rec->state->tokSource
#endif

/* End of token definitions for EmersonLexer
 * =============================================================================
 */
/** \} */

#ifdef __cplusplus
}
#endif

#endif

/* END - Note:Keep extra line feed to satisfy UNIX systems */
