/** \file
 *  This C header file was generated by $ANTLR version 3.1.3 Mar 17, 2009 19:23:44
 *
 *     -  From the grammar source file : .//EmersonTree.g
 *     -                            On : 2011-01-05 18:10:55
 *     -           for the tree parser : EmersonTreeTreeParser *
 * Editing it, at least manually, is not wise. 
 *
 * C language generator and runtime by Jim Idle, jimi|hereisanat|idle|dotgoeshere|ws.
 *
 *
 * The tree parser EmersonTree has the callable functions (rules) shown below,
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
 * a parser context typedef pEmersonTree, which is returned from a call to EmersonTreeNew().
 *
 * The methods in pEmersonTree are  as follows:
 *
 *  - pANTLR3_STRING      pEmersonTree->program(pEmersonTree)
 *  - void      pEmersonTree->sourceElements(pEmersonTree)
 *  - void      pEmersonTree->sourceElement(pEmersonTree)
 *  - void      pEmersonTree->functionDeclaration(pEmersonTree)
 *  - void      pEmersonTree->functionExpression(pEmersonTree)
 *  - void      pEmersonTree->formalParameterList(pEmersonTree)
 *  - void      pEmersonTree->functionBody(pEmersonTree)
 *  - void      pEmersonTree->statement(pEmersonTree)
 *  - void      pEmersonTree->statementBlock(pEmersonTree)
 *  - void      pEmersonTree->statementList(pEmersonTree)
 *  - void      pEmersonTree->variableStatement(pEmersonTree)
 *  - void      pEmersonTree->variableDeclarationList(pEmersonTree)
 *  - void      pEmersonTree->variableDeclarationListNoIn(pEmersonTree)
 *  - void      pEmersonTree->variableDeclaration(pEmersonTree)
 *  - void      pEmersonTree->variableDeclarationNoIn(pEmersonTree)
 *  - void      pEmersonTree->initialiser(pEmersonTree)
 *  - void      pEmersonTree->initialiserNoIn(pEmersonTree)
 *  - void      pEmersonTree->expressionStatement(pEmersonTree)
 *  - void      pEmersonTree->ifStatement(pEmersonTree)
 *  - void      pEmersonTree->iterationStatement(pEmersonTree)
 *  - void      pEmersonTree->doWhileStatement(pEmersonTree)
 *  - void      pEmersonTree->whileStatement(pEmersonTree)
 *  - void      pEmersonTree->forStatement(pEmersonTree)
 *  - void      pEmersonTree->forStatementInitialiserPart(pEmersonTree)
 *  - void      pEmersonTree->forInStatement(pEmersonTree)
 *  - void      pEmersonTree->forInStatementInitialiserPart(pEmersonTree)
 *  - void      pEmersonTree->continueStatement(pEmersonTree)
 *  - void      pEmersonTree->breakStatement(pEmersonTree)
 *  - void      pEmersonTree->returnStatement(pEmersonTree)
 *  - void      pEmersonTree->withStatement(pEmersonTree)
 *  - void      pEmersonTree->labelledStatement(pEmersonTree)
 *  - void      pEmersonTree->switchStatement(pEmersonTree)
 *  - void      pEmersonTree->caseBlock(pEmersonTree)
 *  - void      pEmersonTree->caseClause(pEmersonTree)
 *  - void      pEmersonTree->defaultClause(pEmersonTree)
 *  - void      pEmersonTree->throwStatement(pEmersonTree)
 *  - void      pEmersonTree->tryStatement(pEmersonTree)
 *  - void      pEmersonTree->msgSendStatement(pEmersonTree)
 *  - void      pEmersonTree->msgRecvStatement(pEmersonTree)
 *  - void      pEmersonTree->catchClause(pEmersonTree)
 *  - void      pEmersonTree->finallyClause(pEmersonTree)
 *  - void      pEmersonTree->expression(pEmersonTree)
 *  - void      pEmersonTree->expressionNoIn(pEmersonTree)
 *  - void      pEmersonTree->assignmentExpression(pEmersonTree)
 *  - void      pEmersonTree->assignmentExpressionNoIn(pEmersonTree)
 *  - void      pEmersonTree->leftHandSideExpression(pEmersonTree)
 *  - void      pEmersonTree->newExpression(pEmersonTree)
 *  - void      pEmersonTree->memberExpression(pEmersonTree)
 *  - void      pEmersonTree->memberExpressionSuffix(pEmersonTree)
 *  - void      pEmersonTree->callExpression(pEmersonTree)
 *  - void      pEmersonTree->callExpressionSuffix(pEmersonTree)
 *  - void      pEmersonTree->arguments(pEmersonTree)
 *  - void      pEmersonTree->indexSuffix(pEmersonTree)
 *  - void      pEmersonTree->propertyReferenceSuffix(pEmersonTree)
 *  - void      pEmersonTree->assignmentOperator(pEmersonTree)
 *  - void      pEmersonTree->conditionalExpression(pEmersonTree)
 *  - void      pEmersonTree->conditionalExpressionNoIn(pEmersonTree)
 *  - void      pEmersonTree->logicalANDExpression(pEmersonTree)
 *  - void      pEmersonTree->logicalORExpression(pEmersonTree)
 *  - void      pEmersonTree->logicalORExpressionNoIn(pEmersonTree)
 *  - void      pEmersonTree->logicalANDExpressionNoIn(pEmersonTree)
 *  - void      pEmersonTree->bitwiseORExpression(pEmersonTree)
 *  - void      pEmersonTree->bitwiseORExpressionNoIn(pEmersonTree)
 *  - void      pEmersonTree->bitwiseXORExpression(pEmersonTree)
 *  - void      pEmersonTree->bitwiseXORExpressionNoIn(pEmersonTree)
 *  - void      pEmersonTree->bitwiseANDExpression(pEmersonTree)
 *  - void      pEmersonTree->bitwiseANDExpressionNoIn(pEmersonTree)
 *  - void      pEmersonTree->equalityExpression(pEmersonTree)
 *  - void      pEmersonTree->equalityExpressionNoIn(pEmersonTree)
 *  - void      pEmersonTree->relationalOps(pEmersonTree)
 *  - void      pEmersonTree->relationalExpression(pEmersonTree)
 *  - void      pEmersonTree->relationalOpsNoIn(pEmersonTree)
 *  - void      pEmersonTree->relationalExpressionNoIn(pEmersonTree)
 *  - void      pEmersonTree->shiftOps(pEmersonTree)
 *  - void      pEmersonTree->shiftExpression(pEmersonTree)
 *  - void      pEmersonTree->additiveExpression(pEmersonTree)
 *  - void      pEmersonTree->multiplicativeExpression(pEmersonTree)
 *  - void      pEmersonTree->unaryOps(pEmersonTree)
 *  - void      pEmersonTree->unaryExpression(pEmersonTree)
 *  - void      pEmersonTree->postfixExpression(pEmersonTree)
 *  - void      pEmersonTree->primaryExpression(pEmersonTree)
 *  - void      pEmersonTree->arrayLiteral(pEmersonTree)
 *  - void      pEmersonTree->objectLiteral(pEmersonTree)
 *  - void      pEmersonTree->propertyNameAndValue(pEmersonTree)
 *  - void      pEmersonTree->propertyName(pEmersonTree)
 *  - void      pEmersonTree->literal(pEmersonTree)
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
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

#ifndef	_EmersonTree_H
#define _EmersonTree_H
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
typedef struct EmersonTree_Ctx_struct EmersonTree, * pEmersonTree;



  #include <stdlib.h>
		#include <string.h>
		#include <antlr3.h>
  #include "EmersonUtil.h" 
	 #define APP(s) program_string->append(program_string, s);

	//	pANTLR3_STRING program_string;


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

/* ruleAttributeScopeDecl(scope)
 */
/* makeScopeSet() 
 */
 /** Definition of the msgSendStatement scope variable tracking
 *  structure. An instance of this structure is created by calling
 *  EmersonTree_msgSendStatementPush().
 */
typedef struct  EmersonTree_msgSendStatement_SCOPE_struct
{
    /** Function that the user may provide to be called when the
     *  scope is destroyed (so you can free pANTLR3_HASH_TABLES and so on)
     *
     * \param POinter to an instance of this typedef/struct
     */
    void    (ANTLR3_CDECL *free)	(struct EmersonTree_msgSendStatement_SCOPE_struct * frame);
    
    /* =============================================================================
     * Programmer defined variables...
     */
    pANTLR3_STRING prev_program_string;
    unsigned int  prev_program_len;
    char* firstExprString;
    char* secondExprString;
    pANTLR3_STRING init_program_string;

    /* End of programmer defined variables
     * =============================================================================
     */
} 
    EmersonTree_msgSendStatement_SCOPE, * pEmersonTree_msgSendStatement_SCOPE;
/* ruleAttributeScopeDecl(scope)
 */
/* makeScopeSet() 
 */
 /** Definition of the assignmentExpression scope variable tracking
 *  structure. An instance of this structure is created by calling
 *  EmersonTree_assignmentExpressionPush().
 */
typedef struct  EmersonTree_assignmentExpression_SCOPE_struct
{
    /** Function that the user may provide to be called when the
     *  scope is destroyed (so you can free pANTLR3_HASH_TABLES and so on)
     *
     * \param POinter to an instance of this typedef/struct
     */
    void    (ANTLR3_CDECL *free)	(struct EmersonTree_assignmentExpression_SCOPE_struct * frame);
    
    /* =============================================================================
     * Programmer defined variables...
     */
    char* op;

    /* End of programmer defined variables
     * =============================================================================
     */
} 
    EmersonTree_assignmentExpression_SCOPE, * pEmersonTree_assignmentExpression_SCOPE;
/* ruleAttributeScopeDecl(scope)
 */
/* makeScopeSet() 
 */
 /** Definition of the assignmentExpressionNoIn scope variable tracking
 *  structure. An instance of this structure is created by calling
 *  EmersonTree_assignmentExpressionNoInPush().
 */
typedef struct  EmersonTree_assignmentExpressionNoIn_SCOPE_struct
{
    /** Function that the user may provide to be called when the
     *  scope is destroyed (so you can free pANTLR3_HASH_TABLES and so on)
     *
     * \param POinter to an instance of this typedef/struct
     */
    void    (ANTLR3_CDECL *free)	(struct EmersonTree_assignmentExpressionNoIn_SCOPE_struct * frame);
    
    /* =============================================================================
     * Programmer defined variables...
     */
    char* op;

    /* End of programmer defined variables
     * =============================================================================
     */
} 
    EmersonTree_assignmentExpressionNoIn_SCOPE, * pEmersonTree_assignmentExpressionNoIn_SCOPE;
/* ruleAttributeScopeDecl(scope)
 */
/* makeScopeSet() 
 */
 /** Definition of the relationalExpression scope variable tracking
 *  structure. An instance of this structure is created by calling
 *  EmersonTree_relationalExpressionPush().
 */
typedef struct  EmersonTree_relationalExpression_SCOPE_struct
{
    /** Function that the user may provide to be called when the
     *  scope is destroyed (so you can free pANTLR3_HASH_TABLES and so on)
     *
     * \param POinter to an instance of this typedef/struct
     */
    void    (ANTLR3_CDECL *free)	(struct EmersonTree_relationalExpression_SCOPE_struct * frame);
    
    /* =============================================================================
     * Programmer defined variables...
     */
    char* op;

    /* End of programmer defined variables
     * =============================================================================
     */
} 
    EmersonTree_relationalExpression_SCOPE, * pEmersonTree_relationalExpression_SCOPE;
/* ruleAttributeScopeDecl(scope)
 */
/* makeScopeSet() 
 */
 /** Definition of the relationalExpressionNoIn scope variable tracking
 *  structure. An instance of this structure is created by calling
 *  EmersonTree_relationalExpressionNoInPush().
 */
typedef struct  EmersonTree_relationalExpressionNoIn_SCOPE_struct
{
    /** Function that the user may provide to be called when the
     *  scope is destroyed (so you can free pANTLR3_HASH_TABLES and so on)
     *
     * \param POinter to an instance of this typedef/struct
     */
    void    (ANTLR3_CDECL *free)	(struct EmersonTree_relationalExpressionNoIn_SCOPE_struct * frame);
    
    /* =============================================================================
     * Programmer defined variables...
     */
    char* op;

    /* End of programmer defined variables
     * =============================================================================
     */
} 
    EmersonTree_relationalExpressionNoIn_SCOPE, * pEmersonTree_relationalExpressionNoIn_SCOPE;
/* ruleAttributeScopeDecl(scope)
 */
/* makeScopeSet() 
 */
 /** Definition of the shiftExpression scope variable tracking
 *  structure. An instance of this structure is created by calling
 *  EmersonTree_shiftExpressionPush().
 */
typedef struct  EmersonTree_shiftExpression_SCOPE_struct
{
    /** Function that the user may provide to be called when the
     *  scope is destroyed (so you can free pANTLR3_HASH_TABLES and so on)
     *
     * \param POinter to an instance of this typedef/struct
     */
    void    (ANTLR3_CDECL *free)	(struct EmersonTree_shiftExpression_SCOPE_struct * frame);
    
    /* =============================================================================
     * Programmer defined variables...
     */
    char* op;

    /* End of programmer defined variables
     * =============================================================================
     */
} 
    EmersonTree_shiftExpression_SCOPE, * pEmersonTree_shiftExpression_SCOPE;

/** Context tracking structure for EmersonTree
 */
struct EmersonTree_Ctx_struct
{
    /** Built in ANTLR3 context tracker contains all the generic elements
     *  required for context tracking.
     */
    pANTLR3_TREE_PARSER	    pTreeParser;

    /* ruleAttributeScopeDef(scope)
     */
    /** Pointer to the  msgSendStatement stack for use by pEmersonTree_msgSendStatementPush()
     *  and pEmersonTree_msgSendStatementPop()
     */
    pANTLR3_STACK pEmersonTree_msgSendStatementStack;
    ANTLR3_UINT32 pEmersonTree_msgSendStatementStack_limit;
    pEmersonTree_msgSendStatement_SCOPE   (*pEmersonTree_msgSendStatementPush)(struct EmersonTree_Ctx_struct * ctx);
    pEmersonTree_msgSendStatement_SCOPE   pEmersonTree_msgSendStatementTop;
    /* ruleAttributeScopeDef(scope)
     */
    /** Pointer to the  assignmentExpression stack for use by pEmersonTree_assignmentExpressionPush()
     *  and pEmersonTree_assignmentExpressionPop()
     */
    pANTLR3_STACK pEmersonTree_assignmentExpressionStack;
    ANTLR3_UINT32 pEmersonTree_assignmentExpressionStack_limit;
    pEmersonTree_assignmentExpression_SCOPE   (*pEmersonTree_assignmentExpressionPush)(struct EmersonTree_Ctx_struct * ctx);
    pEmersonTree_assignmentExpression_SCOPE   pEmersonTree_assignmentExpressionTop;
    /* ruleAttributeScopeDef(scope)
     */
    /** Pointer to the  assignmentExpressionNoIn stack for use by pEmersonTree_assignmentExpressionNoInPush()
     *  and pEmersonTree_assignmentExpressionNoInPop()
     */
    pANTLR3_STACK pEmersonTree_assignmentExpressionNoInStack;
    ANTLR3_UINT32 pEmersonTree_assignmentExpressionNoInStack_limit;
    pEmersonTree_assignmentExpressionNoIn_SCOPE   (*pEmersonTree_assignmentExpressionNoInPush)(struct EmersonTree_Ctx_struct * ctx);
    pEmersonTree_assignmentExpressionNoIn_SCOPE   pEmersonTree_assignmentExpressionNoInTop;
    /* ruleAttributeScopeDef(scope)
     */
    /** Pointer to the  relationalExpression stack for use by pEmersonTree_relationalExpressionPush()
     *  and pEmersonTree_relationalExpressionPop()
     */
    pANTLR3_STACK pEmersonTree_relationalExpressionStack;
    ANTLR3_UINT32 pEmersonTree_relationalExpressionStack_limit;
    pEmersonTree_relationalExpression_SCOPE   (*pEmersonTree_relationalExpressionPush)(struct EmersonTree_Ctx_struct * ctx);
    pEmersonTree_relationalExpression_SCOPE   pEmersonTree_relationalExpressionTop;
    /* ruleAttributeScopeDef(scope)
     */
    /** Pointer to the  relationalExpressionNoIn stack for use by pEmersonTree_relationalExpressionNoInPush()
     *  and pEmersonTree_relationalExpressionNoInPop()
     */
    pANTLR3_STACK pEmersonTree_relationalExpressionNoInStack;
    ANTLR3_UINT32 pEmersonTree_relationalExpressionNoInStack_limit;
    pEmersonTree_relationalExpressionNoIn_SCOPE   (*pEmersonTree_relationalExpressionNoInPush)(struct EmersonTree_Ctx_struct * ctx);
    pEmersonTree_relationalExpressionNoIn_SCOPE   pEmersonTree_relationalExpressionNoInTop;
    /* ruleAttributeScopeDef(scope)
     */
    /** Pointer to the  shiftExpression stack for use by pEmersonTree_shiftExpressionPush()
     *  and pEmersonTree_shiftExpressionPop()
     */
    pANTLR3_STACK pEmersonTree_shiftExpressionStack;
    ANTLR3_UINT32 pEmersonTree_shiftExpressionStack_limit;
    pEmersonTree_shiftExpression_SCOPE   (*pEmersonTree_shiftExpressionPush)(struct EmersonTree_Ctx_struct * ctx);
    pEmersonTree_shiftExpression_SCOPE   pEmersonTree_shiftExpressionTop;


     pANTLR3_STRING (*program)	(struct EmersonTree_Ctx_struct * ctx);
     void (*sourceElements)	(struct EmersonTree_Ctx_struct * ctx);
     void (*sourceElement)	(struct EmersonTree_Ctx_struct * ctx);
     void (*functionDeclaration)	(struct EmersonTree_Ctx_struct * ctx);
     void (*functionExpression)	(struct EmersonTree_Ctx_struct * ctx);
     void (*formalParameterList)	(struct EmersonTree_Ctx_struct * ctx);
     void (*functionBody)	(struct EmersonTree_Ctx_struct * ctx);
     void (*statement)	(struct EmersonTree_Ctx_struct * ctx);
     void (*statementBlock)	(struct EmersonTree_Ctx_struct * ctx);
     void (*statementList)	(struct EmersonTree_Ctx_struct * ctx);
     void (*variableStatement)	(struct EmersonTree_Ctx_struct * ctx);
     void (*variableDeclarationList)	(struct EmersonTree_Ctx_struct * ctx);
     void (*variableDeclarationListNoIn)	(struct EmersonTree_Ctx_struct * ctx);
     void (*variableDeclaration)	(struct EmersonTree_Ctx_struct * ctx);
     void (*variableDeclarationNoIn)	(struct EmersonTree_Ctx_struct * ctx);
     void (*initialiser)	(struct EmersonTree_Ctx_struct * ctx);
     void (*initialiserNoIn)	(struct EmersonTree_Ctx_struct * ctx);
     void (*expressionStatement)	(struct EmersonTree_Ctx_struct * ctx);
     void (*ifStatement)	(struct EmersonTree_Ctx_struct * ctx);
     void (*iterationStatement)	(struct EmersonTree_Ctx_struct * ctx);
     void (*doWhileStatement)	(struct EmersonTree_Ctx_struct * ctx);
     void (*whileStatement)	(struct EmersonTree_Ctx_struct * ctx);
     void (*forStatement)	(struct EmersonTree_Ctx_struct * ctx);
     void (*forStatementInitialiserPart)	(struct EmersonTree_Ctx_struct * ctx);
     void (*forInStatement)	(struct EmersonTree_Ctx_struct * ctx);
     void (*forInStatementInitialiserPart)	(struct EmersonTree_Ctx_struct * ctx);
     void (*continueStatement)	(struct EmersonTree_Ctx_struct * ctx);
     void (*breakStatement)	(struct EmersonTree_Ctx_struct * ctx);
     void (*returnStatement)	(struct EmersonTree_Ctx_struct * ctx);
     void (*withStatement)	(struct EmersonTree_Ctx_struct * ctx);
     void (*labelledStatement)	(struct EmersonTree_Ctx_struct * ctx);
     void (*switchStatement)	(struct EmersonTree_Ctx_struct * ctx);
     void (*caseBlock)	(struct EmersonTree_Ctx_struct * ctx);
     void (*caseClause)	(struct EmersonTree_Ctx_struct * ctx);
     void (*defaultClause)	(struct EmersonTree_Ctx_struct * ctx);
     void (*throwStatement)	(struct EmersonTree_Ctx_struct * ctx);
     void (*tryStatement)	(struct EmersonTree_Ctx_struct * ctx);
     void (*msgSendStatement)	(struct EmersonTree_Ctx_struct * ctx);
     void (*msgRecvStatement)	(struct EmersonTree_Ctx_struct * ctx);
     void (*catchClause)	(struct EmersonTree_Ctx_struct * ctx);
     void (*finallyClause)	(struct EmersonTree_Ctx_struct * ctx);
     void (*expression)	(struct EmersonTree_Ctx_struct * ctx);
     void (*expressionNoIn)	(struct EmersonTree_Ctx_struct * ctx);
     void (*assignmentExpression)	(struct EmersonTree_Ctx_struct * ctx);
     void (*assignmentExpressionNoIn)	(struct EmersonTree_Ctx_struct * ctx);
     void (*leftHandSideExpression)	(struct EmersonTree_Ctx_struct * ctx);
     void (*newExpression)	(struct EmersonTree_Ctx_struct * ctx);
     void (*memberExpression)	(struct EmersonTree_Ctx_struct * ctx);
     void (*memberExpressionSuffix)	(struct EmersonTree_Ctx_struct * ctx);
     void (*callExpression)	(struct EmersonTree_Ctx_struct * ctx);
     void (*callExpressionSuffix)	(struct EmersonTree_Ctx_struct * ctx);
     void (*arguments)	(struct EmersonTree_Ctx_struct * ctx);
     void (*indexSuffix)	(struct EmersonTree_Ctx_struct * ctx);
     void (*propertyReferenceSuffix)	(struct EmersonTree_Ctx_struct * ctx);
     void (*assignmentOperator)	(struct EmersonTree_Ctx_struct * ctx);
     void (*conditionalExpression)	(struct EmersonTree_Ctx_struct * ctx);
     void (*conditionalExpressionNoIn)	(struct EmersonTree_Ctx_struct * ctx);
     void (*logicalANDExpression)	(struct EmersonTree_Ctx_struct * ctx);
     void (*logicalORExpression)	(struct EmersonTree_Ctx_struct * ctx);
     void (*logicalORExpressionNoIn)	(struct EmersonTree_Ctx_struct * ctx);
     void (*logicalANDExpressionNoIn)	(struct EmersonTree_Ctx_struct * ctx);
     void (*bitwiseORExpression)	(struct EmersonTree_Ctx_struct * ctx);
     void (*bitwiseORExpressionNoIn)	(struct EmersonTree_Ctx_struct * ctx);
     void (*bitwiseXORExpression)	(struct EmersonTree_Ctx_struct * ctx);
     void (*bitwiseXORExpressionNoIn)	(struct EmersonTree_Ctx_struct * ctx);
     void (*bitwiseANDExpression)	(struct EmersonTree_Ctx_struct * ctx);
     void (*bitwiseANDExpressionNoIn)	(struct EmersonTree_Ctx_struct * ctx);
     void (*equalityExpression)	(struct EmersonTree_Ctx_struct * ctx);
     void (*equalityExpressionNoIn)	(struct EmersonTree_Ctx_struct * ctx);
     void (*relationalOps)	(struct EmersonTree_Ctx_struct * ctx);
     void (*relationalExpression)	(struct EmersonTree_Ctx_struct * ctx);
     void (*relationalOpsNoIn)	(struct EmersonTree_Ctx_struct * ctx);
     void (*relationalExpressionNoIn)	(struct EmersonTree_Ctx_struct * ctx);
     void (*shiftOps)	(struct EmersonTree_Ctx_struct * ctx);
     void (*shiftExpression)	(struct EmersonTree_Ctx_struct * ctx);
     void (*additiveExpression)	(struct EmersonTree_Ctx_struct * ctx);
     void (*multiplicativeExpression)	(struct EmersonTree_Ctx_struct * ctx);
     void (*unaryOps)	(struct EmersonTree_Ctx_struct * ctx);
     void (*unaryExpression)	(struct EmersonTree_Ctx_struct * ctx);
     void (*postfixExpression)	(struct EmersonTree_Ctx_struct * ctx);
     void (*primaryExpression)	(struct EmersonTree_Ctx_struct * ctx);
     void (*arrayLiteral)	(struct EmersonTree_Ctx_struct * ctx);
     void (*objectLiteral)	(struct EmersonTree_Ctx_struct * ctx);
     void (*propertyNameAndValue)	(struct EmersonTree_Ctx_struct * ctx);
     void (*propertyName)	(struct EmersonTree_Ctx_struct * ctx);
     void (*literal)	(struct EmersonTree_Ctx_struct * ctx);
     ANTLR3_BOOLEAN (*synpred4_EmersonTree)	(struct EmersonTree_Ctx_struct * ctx);
     ANTLR3_BOOLEAN (*synpred38_EmersonTree)	(struct EmersonTree_Ctx_struct * ctx);
     ANTLR3_BOOLEAN (*synpred40_EmersonTree)	(struct EmersonTree_Ctx_struct * ctx);
     ANTLR3_BOOLEAN (*synpred42_EmersonTree)	(struct EmersonTree_Ctx_struct * ctx);
     ANTLR3_BOOLEAN (*synpred47_EmersonTree)	(struct EmersonTree_Ctx_struct * ctx);
     ANTLR3_BOOLEAN (*synpred75_EmersonTree)	(struct EmersonTree_Ctx_struct * ctx);
     ANTLR3_BOOLEAN (*synpred83_EmersonTree)	(struct EmersonTree_Ctx_struct * ctx);
     ANTLR3_BOOLEAN (*synpred153_EmersonTree)	(struct EmersonTree_Ctx_struct * ctx);
     ANTLR3_BOOLEAN (*synpred160_EmersonTree)	(struct EmersonTree_Ctx_struct * ctx);
     ANTLR3_BOOLEAN (*synpred161_EmersonTree)	(struct EmersonTree_Ctx_struct * ctx);
     ANTLR3_BOOLEAN (*synpred163_EmersonTree)	(struct EmersonTree_Ctx_struct * ctx);
     ANTLR3_BOOLEAN (*synpred164_EmersonTree)	(struct EmersonTree_Ctx_struct * ctx);
    // Delegated rules
    const char * (*getGrammarFileName)();
    void	    (*free)   (struct EmersonTree_Ctx_struct * ctx);
        
};

// Function protoypes for the constructor functions that external translation units
// such as delegators and delegates may wish to call.
//
ANTLR3_API pEmersonTree EmersonTreeNew         (pANTLR3_COMMON_TREE_NODE_STREAM instream);
ANTLR3_API pEmersonTree EmersonTreeNewSSD      (pANTLR3_COMMON_TREE_NODE_STREAM instream, pANTLR3_RECOGNIZER_SHARED_STATE state);

/** Symbolic definitions of all the tokens that the tree parser will work with.
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
#define LEFT_SHIFT_ASSIG      198
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
#define T__197      197
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

/* End of token definitions for EmersonTree
 * =============================================================================
 */
/** \} */

#ifdef __cplusplus
}
#endif

#endif

/* END - Note:Keep extra line feed to satisfy UNIX systems */
