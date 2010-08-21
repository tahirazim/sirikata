/*
  Copyright 2008 Chris Lambrou.
  All rights reserved.
*/

// This is the parser grammar for Emerson language

grammar Emerson;

options
{
	output=AST;
	backtrack=true;
	memoize=true;
 ASTLabelType = pANTLR3_BASE_TREE; 
// ASTLabelType = pANTLR3_COMMON_TREE; 
	language = C;
}


tokens
{
  UNDEF; // imaginary opertator for some undefined operator. we don't knwo what happened
  CALL;  // imaginary operator for function calls
		ARRAY_INDEX; // imag operator for array indexing
		DOT;   // imag op for property referencing
		FUNC;  // imag op for the function
  SLIST; // imag op for a list of statements
		IF;    // imag op for if-else conditional clause
		VARLIST; // list of variables declared. each declaration defined by the VAR
		VAR;   // imag op for var decl
		PROG; // imag operator for complete program
		DO ; // imag operator for do-while statements
		WHILE; // imag op for the while statement
		FOR;  // imag op for the "for" statement
			FORINIT; // for initializer part
			FORCOND; // for conditional part
			FORITER; // for iteration part
  FORIN;   // for in statement operator
		BREAK;  // break statement
		CONTINUE; //continue statement
		RETURN; // return statement
		WITH;   // the with statement
		NEW;    // the new operator
  TRY;
  THROW;
		CATCH;
		FINALLY;
  DEFAULT;
		SWITCH;
		CASE;
  LABEL;	
		ASSIGN;
  MULT_ASSIGN;
		DIV_ASSIGN;
		MOD_ASSIGN;
  ADD_ASSIGN;
		SUB_ASSIGN;
		LEFT_SHIFT_ASSIGN;
		RIGHT_SHIFT_ASSIGN;
		TRIPLE_SHIFT_ASSIGN;
		AND_ASSIGN;
		EXP_ASSIGN;
		OR_ASSIGN;
  OR; 
		AND;
		BIT_OR;
		EXP;
		BIT_AND;
		EQUALS;
		NOT_EQUALS;
		IDENT;
		NOT_IDENT;
		LESS_THAN;
		GREATER_THAN;
		LESS_THAN_EQUAL;
		GREATER_THAN_EQUAL;
		INSTANCE_OF;
		IN;
		LEFT_SHIFT;
		RIGHT_SHIFT;
		TRIPLE_SHIFT;
		ADD;
		SUB;
		MULT;
		DIV;
		MOD;
		ARRAY_LITERAL;
		OBJ_LITERAL;
		NAME_VALUE;
  DELETE;
		VOID;
		TYPEOF;
		PLUSPLUS;
		MINUSMINUS;
		UNARY_PLUS;
		UNARY_MINUS;
		COMPLEMENT;
		NOT;
		POSTEXPR;
		FUNC_PARAMS;
		FUNC_DECL;
		FUNC_EXPR;
  ARGLIST;
		EXPR_LIST;
		COND_EXPR;
}

@header
{
  #include <stdlib.h>;
		#include <stdio.h>;
		#include "EmersonUtil.h";
}


	

program
	: a=LTERM* sourceElements LTERM* EOF -> ^(PROG sourceElements) // omitting LTERM and EOF

	;

sourceElements
	: sourceElement (LTERM* sourceElement)* -> sourceElement+  // omitting the LTERM 
	;
	
sourceElement
	: functionDeclaration -> functionDeclaration
	| statement -> statement
	;
	
// functions
functionDeclaration
	: 'function' LTERM* Identifier LTERM* formalParameterList LTERM* functionBody -> ^( FUNC_DECL Identifier formalParameterList functionBody)
	;

functionExpression
	: 'function' LTERM* Identifier? LTERM* formalParameterList LTERM* functionBody -> ^( FUNC_EXPR Identifier?  formalParameterList functionBody)
	;
	
formalParameterList
	: '(' (LTERM* i1=Identifier (LTERM* ',' LTERM* i2=Identifier)*)? LTERM* ')' -> ^( FUNC_PARAMS $i1? $i2*)
	;

functionBody
 : '{' LTERM* '}'
	| '{' LTERM* (sourceElements -> sourceElements) LTERM* '}'
	;

// statements
statement
	: statementBlock
	| variableStatement
	| emptyStatement
	| expressionStatement
	| ifStatement
	| iterationStatement
	| continueStatement
	| breakStatement
	| returnStatement
	| withStatement
	| labelledStatement
	| switchStatement
	| throwStatement
	| tryStatement
	;
	
statementBlock
 : '{' LTERM* '}'
	| '{' LTERM* (statementList->statementList) LTERM* '}' 
	;
	
statementList
	: statement (LTERM* statement)* -> ^(SLIST statement+)
	;
	
variableStatement
	: 'var' LTERM* variableDeclarationList (LTERM | ';') -> ^( VARLIST variableDeclarationList)
	;
	
variableDeclarationList
	: variableDeclaration (LTERM* ',' LTERM* variableDeclaration)* -> variableDeclaration+
	;
	
variableDeclarationListNoIn
	: variableDeclarationNoIn (LTERM* ',' LTERM* variableDeclarationNoIn)* -> variableDeclarationNoIn+
	;
	
variableDeclaration
	: Identifier LTERM* initialiser? -> ^(VAR Identifier initialiser?)
	;
	
variableDeclarationNoIn
	: Identifier LTERM* initialiserNoIn? ->  ^(VAR Identifier initialiserNoIn?)
	;
	
initialiser
	: '=' LTERM* assignmentExpression -> assignmentExpression 
	;
	
initialiserNoIn
	: '=' LTERM* assignmentExpressionNoIn -> assignmentExpressionNoIn
	;
	
emptyStatement
	: ';'
	;
	
expressionStatement
	: expression (LTERM | ';') -> expression
	;
	
ifStatement
: 'if' LTERM* '(' LTERM* expression LTERM* ')' LTERM* s1=statement (LTERM* 'else' LTERM* s2=statement)? -> ^(IF expression $s1 $s2?)
	;
	
iterationStatement
	: doWhileStatement
	| whileStatement
	| forStatement
	| forInStatement
	;
	
doWhileStatement
	: 'do' LTERM* statement LTERM* 'while' LTERM* '(' expression ')' (LTERM | ';') -> ^( DO  statement expression )
	;
	
whileStatement
	: 'while' LTERM* '(' LTERM* expression LTERM* ')' LTERM* statement -> ^(WHILE  expression statement)
	;
	
forStatement
	: 'for' LTERM* '(' (LTERM* init=forStatementInitialiserPart)? LTERM* ';' (LTERM* cond=expression)? LTERM* ';' (LTERM* iter=expression)? LTERM* ')' LTERM* statement ->  ^(FOR ^(FORINIT $init)?   ^(FORCOND $cond)?  ^(FORITER $iter)?  statement)
	;
	
forStatementInitialiserPart
	: expressionNoIn
	| 'var' LTERM* variableDeclarationListNoIn -> ^(VARLIST variableDeclarationListNoIn)
	;
	
forInStatement
	: 'for' LTERM* '(' LTERM* forInStatementInitialiserPart LTERM* 'in' LTERM* expression LTERM* ')' LTERM* statement -> ^(FORIN forInStatementInitialiserPart expression statement)
	;
	
forInStatementInitialiserPart
	: leftHandSideExpression -> leftHandSideExpression
	| 'var' LTERM* variableDeclarationNoIn -> ^(VAR variableDeclarationNoIn)
	;

continueStatement
	: 'continue' Identifier? (LTERM | ';')  -> ^(CONTINUE Identifier?)
	;

breakStatement
	: 'break' Identifier? (LTERM | ';') -> ^(BREAK Identifier?)
	;

returnStatement
	: 'return' expression? (LTERM | ';') -> ^(RETURN expression?)
	;
	
withStatement
	: 'with' LTERM* '(' LTERM* expression LTERM* ')' LTERM* statement -> ^(WITH expression statement)
	;

labelledStatement
	: Identifier LTERM* ':' LTERM* statement -> ^( LABEL Identifier statement)
	;
	
switchStatement
	: 'switch' LTERM* '(' LTERM* expression LTERM* ')' LTERM* caseBlock -> ^(SWITCH expression caseBlock)
	;
	
caseBlock
	: '{' (LTERM* case1=caseClause)* (LTERM* defaultClause (LTERM* case2=caseClause)*)? LTERM* '}' -> 	^($case1)* (^(defaultClause)*)? (^($case2)*)?
	;

caseClause
	: 'case' LTERM* expression LTERM* ':' LTERM* statementList? -> ^( CASE expression statementList?)
	;
	
defaultClause
	: 'default' LTERM* ':' LTERM* statementList? -> ^(DEFAULT statementList?)
	;
	
throwStatement
	: 'throw' expression (LTERM | ';') -> ^(THROW expression)
	;

tryStatement
	: 'try' LTERM* statementBlock LTERM* (finallyClause | catchClause (LTERM* finallyClause)?) -> ^(TRY statementBlock finallyClause?) 
	;
       
catchClause
	: 'catch' LTERM* '(' LTERM* Identifier LTERM* ')' LTERM* statementBlock -> ^(CATCH Identifier statementBlock)
	;
	
finallyClause
	: 'finally' LTERM* statementBlock -> ^( FINALLY statementBlock )
	;

// expressions
expression
	: assignmentExpression (LTERM* ',' LTERM* assignmentExpression)* ->  ^(EXPR_LIST assignmentExpression+)
	;
	
expressionNoIn
	: assignmentExpressionNoIn (LTERM* ',' LTERM* assignmentExpressionNoIn)* -> ^(EXPR_LIST assignmentExpressionNoIn+)

	;
	
assignmentExpression
	: conditionalExpression -> ^(COND_EXPR conditionalExpression)
	| leftHandSideExpression LTERM* assignmentOperator LTERM* assignmentExpression ->  ^(assignmentOperator  leftHandSideExpression assignmentExpression)
	;
	
assignmentExpressionNoIn
	: conditionalExpressionNoIn
	| leftHandSideExpression LTERM* assignmentOperator LTERM* assignmentExpressionNoIn ->  ^(assignmentOperator leftHandSideExpression assignmentExpressionNoIn ) 
	;
	
leftHandSideExpression
	: callExpression -> callExpression
	| newExpression -> newExpression
	;
	
newExpression
	: memberExpression -> memberExpression
	| 'new' LTERM* newExpression -> ^( NEW newExpression)
	;
	
memberExpression

	: (primaryExpression -> primaryExpression) ( LTERM* memberExpressionSuffix  -> ^(memberExpressionSuffix $memberExpression) )* 
	
	  {
			  //pANTLR3_STRING prev = emerson_printAST($memberExpression.tree); 
					//printf("\%s\n", prev->chars);
					emerson_createTreeMirrorImage($memberExpression.tree); 
					//pANTLR3_STRING newT = emerson_printAST($memberExpression.tree);
					//printf("\%s\n", newT->chars);
					
			} 
	| (functionExpression -> functionExpression) (LTERM* memberExpressionSuffix -> ^(memberExpressionSuffix $memberExpression) )*
	{
	  
					emerson_createTreeMirrorImage($memberExpression.tree); 
	}
	| ('new' LTERM* expr=memberExpression LTERM* arguments -> ^(NEW $expr arguments)) (LTERM* memberExpressionSuffix -> ^(memberExpressionSuffix $memberExpression) )*  
	{
	  
					emerson_createTreeMirrorImage($memberExpression.tree); 
	}
	;
	
memberExpressionSuffix
	: indexSuffix -> indexSuffix 
	| propertyReferenceSuffix -> propertyReferenceSuffix 
	;

callExpression
 : (memberExpression LTERM* arguments -> ^(CALL memberExpression arguments)) (LTERM* callExpressionSuffix  -> ^(callExpressionSuffix $callExpression))*
	;
	
callExpressionSuffix
	: arguments -> arguments
	| indexSuffix -> indexSuffix
	| propertyReferenceSuffix -> propertyReferenceSuffix
	;

arguments
	: '(' (LTERM* (assignmentExpression) (LTERM* ',' LTERM* assignmentExpression)*)? LTERM* ')' -> ^(ARGLIST assignmentExpression*) 
	;
	
indexSuffix
	: '[' LTERM* expression LTERM* ']' -> ^(ARRAY_INDEX expression)
	;	
	
propertyReferenceSuffix
	: '.' LTERM* Identifier -> ^(DOT Identifier)
	;
	
assignmentOperator
	: '=' -> ^(ASSIGN)| '*=' -> ^(MULT_ASSIGN)| '/=' -> ^(DIV_ASSIGN) | '%=' -> ^(MOD_ASSIGN)| '+=' -> ^(ADD_ASSIGN)| '-=' -> ^(SUB_ASSIGN)| '<<=' -> ^(LEFT_SHIFT_ASSIGN)| '>>=' -> ^(RIGHT_SHIFT_ASSIGN)| '>>>=' -> ^(TRIPLE_SHIFT_ASSIGN)| '&='-> ^(AND_ASSIGN)| '^='-> ^(EXP_ASSIGN) | '|=' -> ^(OR_ASSIGN)
	;

conditionalExpression
	: logicalORExpression (LTERM* '?' LTERM* expr1=assignmentExpression LTERM* ':' LTERM* expr2=assignmentExpression)? -> ^(logicalORExpression ($expr1 $expr2)? )
	;

conditionalExpressionNoIn
	: logicalORExpressionNoIn (LTERM* '?' LTERM* expr1=assignmentExpressionNoIn LTERM* ':' LTERM* expr2=assignmentExpressionNoIn)? -> ^(logicalORExpressionNoIn ($expr1 $expr2)?)
	;


logicalORExpression
	: (logicalANDExpression -> logicalANDExpression)(LTERM* '||' LTERM* logicalANDExpression -> ^(OR $logicalORExpression logicalANDExpression) )*
	;
	
	logicalANDExpression
	: (bitwiseORExpression -> bitwiseORExpression)(LTERM* '&&' LTERM* bitwiseORExpression  -> ^(AND $logicalANDExpression bitwiseORExpression) )*
	;
	
logicalORExpressionNoIn
	: (logicalANDExpressionNoIn -> logicalANDExpressionNoIn)(LTERM* '||' LTERM* logicalANDExpressionNoIn -> ^(OR $logicalORExpressionNoIn logicalANDExpressionNoIn) )* 
	;
	

logicalANDExpressionNoIn
	: (bitwiseORExpressionNoIn -> bitwiseORExpressionNoIn) (LTERM* '&&' LTERM* bitwiseORExpressionNoIn -> ^(AND $logicalANDExpressionNoIn bitwiseORExpressionNoIn) )*
	;
	
bitwiseORExpression
	: (bitwiseXORExpression -> bitwiseXORExpression) (LTERM* '|' LTERM* bitwiseXORExpression -> ^(BIT_OR $bitwiseORExpression bitwiseXORExpression) )*
	;
	
bitwiseORExpressionNoIn
	: (bitwiseXORExpressionNoIn -> bitwiseXORExpressionNoIn) (LTERM* '|' LTERM* bitwiseXORExpressionNoIn -> ^( BIT_OR $bitwiseORExpressionNoIn bitwiseXORExpressionNoIn))*
	;
	
bitwiseXORExpression
: (bitwiseANDExpression -> bitwiseANDExpression) (LTERM* '^' LTERM* bitwiseANDExpression -> ^( EXP $bitwiseXORExpression bitwiseANDExpression))*
	;
	
bitwiseXORExpressionNoIn
	: (bitwiseANDExpressionNoIn -> bitwiseANDExpressionNoIn)(LTERM* '^' LTERM* bitwiseANDExpressionNoIn -> ^( EXP $bitwiseXORExpressionNoIn bitwiseANDExpressionNoIn) )*
	;
	
bitwiseANDExpression
	: (equalityExpression -> equalityExpression) (LTERM* '&' LTERM* equalityExpression -> ^(BIT_AND $bitwiseANDExpression equalityExpression) )* 
	;
	
bitwiseANDExpressionNoIn
	: (equalityExpressionNoIn -> equalityExpressionNoIn) (LTERM* '&' LTERM* equalityExpressionNoIn -> ^(BIT_AND $bitwiseANDExpressionNoIn equalityExpressionNoIn) )*
	;
	
equalityExpression
	: (relationalExpression -> relationalExpression)(LTERM* equalityOps LTERM* relationalExpression -> ^(equalityOps $equalityExpression relationalExpression) )*
	;

equalityOps
:  '==' -> ^(EQUALS)
| '!=' -> ^(NOT_EQUALS)
| '===' -> ^(IDENT)
| '!==' -> ^(NOT_IDENT)
;

equalityExpressionNoIn
	: (relationalExpressionNoIn -> relationalExpressionNoIn)(LTERM* equalityOps LTERM* relationalExpressionNoIn -> ^(equalityOps $equalityExpressionNoIn relationalExpressionNoIn))*
	;
	

relationalOps
: '<'  -> ^(LESS_THAN)
| '>'  -> ^(GREATER_THAN)
| '<=' -> ^(LESS_THAN_EQUAL)
| '>=' -> ^(GREATER_THAN_EQUAL)
| 'instanceof' -> ^(INSTANCE_OF)
| 'in'         -> ^(IN)
;

relationalExpression
	: (shiftExpression -> shiftExpression )(LTERM* relationalOps LTERM* shiftExpression -> ^(relationalOps $relationalExpression shiftExpression))* 
	;

relationalOpsNoIn
: '<'  -> ^(LESS_THAN)
| '>'  -> ^(GREATER_THAN)
| '<=' -> ^(LESS_THAN_EQUAL)
| '>=' -> ^(GREATER_THAN_EQUAL)
| 'instanceof' -> ^(INSTANCE_OF)
;

relationalExpressionNoIn
	: (shiftExpression -> shiftExpression) (LTERM* relationalOpsNoIn LTERM* shiftExpression -> ^(relationalOpsNoIn $relationalExpressionNoIn shiftExpression ))*
	;

shiftOps
:'<<' -> ^(LEFT_SHIFT)
| '>>'-> ^(RIGHT_SHIFT)
| '>>>' -> ^(TRIPLE_SHIFT)
;

shiftExpression
	: (additiveExpression -> additiveExpression)(LTERM* shiftOps LTERM* additiveExpression -> ^(shiftOps $shiftExpression additiveExpression) )*
 ;	


addOps
: '+' -> ^(ADD)
| '-' -> ^(SUB)
;


additiveExpression
	: (multiplicativeExpression -> multiplicativeExpression)(LTERM* addOps LTERM* multiplicativeExpression -> ^(addOps $additiveExpression multiplicativeExpression) )* 
	;

multOps
: '*' -> ^(MULT)
| '/' -> ^(DIV)
| '%' -> ^(MOD)
;

multiplicativeExpression
	: (unaryExpression -> unaryExpression )(LTERM* multOps LTERM* unaryExpression -> ^(multOps $multiplicativeExpression unaryExpression))*
	;


postfixExpression
 :(leftHandSideExpression -> leftHandSideExpression) (('--' -> $postfixExpression '--') | ('++' -> $postfixExpression '++'))?
	;


unaryOps
:'delete' -> ^(DELETE)
| 'void' -> ^(VOID)
| 'typeof' -> ^(TYPEOF)
| '++'  -> ^(PLUSPLUS)
| '--'  -> ^(MINUSMINUS)
| '+'   -> ^(UNARY_PLUS)
| '-'   -> ^(UNARY_MINUS)
| '~'   -> ^(COMPLEMENT)
| '!'   -> ^(NOT)
;


unaryExpression
	: postfixExpression -> ^(POSTEXPR postfixExpression)
	| unaryOps e=unaryExpression -> ^(unaryOps $e)
	;
	

primaryExpression
	: 'this'
	| Identifier
	| literal
	| arrayLiteral
	| objectLiteral
	| '(' LTERM* expression LTERM* ')' -> expression
	;
	
// arrayLiteral definition.
arrayLiteral
	: '[' LTERM* e1=assignmentExpression? (LTERM* ',' (LTERM* e2=assignmentExpression)?)* LTERM* ']' -> ^(ARRAY_LITERAL $e1? $e2*)
	;
       
// objectLiteral definition.
objectLiteral
	: '{' LTERM* p1=propertyNameAndValue? (LTERM* ',' LTERM* p2=propertyNameAndValue)* LTERM* '}' -> ^(OBJ_LITERAL $p1? $p2*)
	;
	
propertyNameAndValue
	: propertyName LTERM* ':' LTERM* assignmentExpression -> ^(NAME_VALUE propertyName assignmentExpression)
	;

propertyName
	: Identifier
	| StringLiteral
	| NumericLiteral
	;

// primitive literal definition.
literal
	: 'null'
	| 'true'
	| 'false'
	| StringLiteral
	| NumericLiteral
	;
	
// lexer rules.

StringLiteral
	: '"' DoubleStringCharacter* '"'
	| '\'' SingleStringCharacter* '\''
	;
	
fragment DoubleStringCharacter
	: ~('"' | '\\' | LTERM)	
	| '\\' EscapeSequence
	;

fragment SingleStringCharacter
	: ~('\'' | '\\' | LTERM)	
	| '\\' EscapeSequence
	;

fragment EscapeSequence
	: CharacterEscapeSequence
	| '0'
	| HexEscapeSequence
	| UnicodeEscapeSequence
	;
	
fragment CharacterEscapeSequence
	: SingleEscapeCharacter
	| NonEscapeCharacter
	;

fragment NonEscapeCharacter
	: ~(EscapeCharacter | LTERM)
	;

fragment SingleEscapeCharacter
	: '\'' | '"' | '\\' | 'b' | 'f' | 'n' | 'r' | 't' | 'v'
	;

fragment EscapeCharacter
	: SingleEscapeCharacter
	| DecimalDigit
	| 'x'
	| 'u'
	;
	
fragment HexEscapeSequence
	: 'x' HexDigit HexDigit
	;
	
fragment UnicodeEscapeSequence
	: 'u' HexDigit HexDigit HexDigit HexDigit
	;
	
NumericLiteral
	: DecimalLiteral
	| HexIntegerLiteral
	;
	
fragment HexIntegerLiteral
	: '0' ('x' | 'X') HexDigit+
	;
	
fragment HexDigit
	: DecimalDigit | ('a'..'f') | ('A'..'F')
	;
	
fragment DecimalLiteral
	: DecimalDigit+ '.' DecimalDigit* ExponentPart?
	| '.'? DecimalDigit+ ExponentPart?
	;
	
fragment DecimalDigit
	: ('0'..'9')
	;

fragment ExponentPart
	: ('e' | 'E') ('+' | '-') ? DecimalDigit+
	;

Identifier
	: IdentifierStart IdentifierPart*
	;
	
fragment IdentifierStart
	: UnicodeLetter
	| '$'
	| '_'
        | '\\' UnicodeEscapeSequence
        ;
        
fragment IdentifierPart
	: (IdentifierStart) => IdentifierStart // Avoids ambiguity, as some IdentifierStart chars also match following alternatives.
	| UnicodeDigit
	| UnicodeConnectorPunctuation
	;
	
fragment UnicodeLetter		// Any character in the Unicode categories "Uppercase letter (Lu)", 
	: '\u0041'..'\u005A'	// "Lowercase letter (Ll)", "Titlecase letter (Lt)",
	| '\u0061'..'\u007A'	// "Modifier letter (Lm)", "Other letter (Lo)", or "Letter number (Nl)".
	| '\u00AA'
	| '\u00B5'
	| '\u00BA'
	| '\u00C0'..'\u00D6'
	| '\u00D8'..'\u00F6'
	| '\u00F8'..'\u021F'
	| '\u0222'..'\u0233'
	| '\u0250'..'\u02AD'
	| '\u02B0'..'\u02B8'
	| '\u02BB'..'\u02C1'
	| '\u02D0'..'\u02D1'
	| '\u02E0'..'\u02E4'
	| '\u02EE'
	| '\u037A'
	| '\u0386'
	| '\u0388'..'\u038A'
	| '\u038C'
	| '\u038E'..'\u03A1'
	| '\u03A3'..'\u03CE'
	| '\u03D0'..'\u03D7'
	| '\u03DA'..'\u03F3'
	| '\u0400'..'\u0481'
	| '\u048C'..'\u04C4'
	| '\u04C7'..'\u04C8'
	| '\u04CB'..'\u04CC'
	| '\u04D0'..'\u04F5'
	| '\u04F8'..'\u04F9'
	| '\u0531'..'\u0556'
	| '\u0559'
	| '\u0561'..'\u0587'
	| '\u05D0'..'\u05EA'
	| '\u05F0'..'\u05F2'
	| '\u0621'..'\u063A'
	| '\u0640'..'\u064A'
	| '\u0671'..'\u06D3'
	| '\u06D5'
	| '\u06E5'..'\u06E6'
	| '\u06FA'..'\u06FC'
	| '\u0710'
	| '\u0712'..'\u072C'
	| '\u0780'..'\u07A5'
	| '\u0905'..'\u0939'
	| '\u093D'
	| '\u0950'
	| '\u0958'..'\u0961'
	| '\u0985'..'\u098C'
	| '\u098F'..'\u0990'
	| '\u0993'..'\u09A8'
	| '\u09AA'..'\u09B0'
	| '\u09B2'
	| '\u09B6'..'\u09B9'
	| '\u09DC'..'\u09DD'
	| '\u09DF'..'\u09E1'
	| '\u09F0'..'\u09F1'
	| '\u0A05'..'\u0A0A'
	| '\u0A0F'..'\u0A10'
	| '\u0A13'..'\u0A28'
	| '\u0A2A'..'\u0A30'
	| '\u0A32'..'\u0A33'
	| '\u0A35'..'\u0A36'
	| '\u0A38'..'\u0A39'
	| '\u0A59'..'\u0A5C'
	| '\u0A5E'
	| '\u0A72'..'\u0A74'
	| '\u0A85'..'\u0A8B'
	| '\u0A8D'
	| '\u0A8F'..'\u0A91'
	| '\u0A93'..'\u0AA8'
	| '\u0AAA'..'\u0AB0'
	| '\u0AB2'..'\u0AB3'
	| '\u0AB5'..'\u0AB9'
	| '\u0ABD'
	| '\u0AD0'
	| '\u0AE0'
	| '\u0B05'..'\u0B0C'
	| '\u0B0F'..'\u0B10'
	| '\u0B13'..'\u0B28'
	| '\u0B2A'..'\u0B30'
	| '\u0B32'..'\u0B33'
	| '\u0B36'..'\u0B39'
	| '\u0B3D'
	| '\u0B5C'..'\u0B5D'
	| '\u0B5F'..'\u0B61'
	| '\u0B85'..'\u0B8A'
	| '\u0B8E'..'\u0B90'
	| '\u0B92'..'\u0B95'
	| '\u0B99'..'\u0B9A'
	| '\u0B9C'
	| '\u0B9E'..'\u0B9F'
	| '\u0BA3'..'\u0BA4'
	| '\u0BA8'..'\u0BAA'
	| '\u0BAE'..'\u0BB5'
	| '\u0BB7'..'\u0BB9'
	| '\u0C05'..'\u0C0C'
	| '\u0C0E'..'\u0C10'
	| '\u0C12'..'\u0C28'
	| '\u0C2A'..'\u0C33'
	| '\u0C35'..'\u0C39'
	| '\u0C60'..'\u0C61'
	| '\u0C85'..'\u0C8C'
	| '\u0C8E'..'\u0C90'
	| '\u0C92'..'\u0CA8'
	| '\u0CAA'..'\u0CB3'
	| '\u0CB5'..'\u0CB9'
	| '\u0CDE'
	| '\u0CE0'..'\u0CE1'
	| '\u0D05'..'\u0D0C'
	| '\u0D0E'..'\u0D10'
	| '\u0D12'..'\u0D28'
	| '\u0D2A'..'\u0D39'
	| '\u0D60'..'\u0D61'
	| '\u0D85'..'\u0D96'
	| '\u0D9A'..'\u0DB1'
	| '\u0DB3'..'\u0DBB'
	| '\u0DBD'
	| '\u0DC0'..'\u0DC6'
	| '\u0E01'..'\u0E30'
	| '\u0E32'..'\u0E33'
	| '\u0E40'..'\u0E46'
	| '\u0E81'..'\u0E82'
	| '\u0E84'
	| '\u0E87'..'\u0E88'
	| '\u0E8A'
	| '\u0E8D'
	| '\u0E94'..'\u0E97'
	| '\u0E99'..'\u0E9F'
	| '\u0EA1'..'\u0EA3'
	| '\u0EA5'
	| '\u0EA7'
	| '\u0EAA'..'\u0EAB'
	| '\u0EAD'..'\u0EB0'
	| '\u0EB2'..'\u0EB3'
	| '\u0EBD'..'\u0EC4'
	| '\u0EC6'
	| '\u0EDC'..'\u0EDD'
	| '\u0F00'
	| '\u0F40'..'\u0F6A'
	| '\u0F88'..'\u0F8B'
	| '\u1000'..'\u1021'
	| '\u1023'..'\u1027'
	| '\u1029'..'\u102A'
	| '\u1050'..'\u1055'
	| '\u10A0'..'\u10C5'
	| '\u10D0'..'\u10F6'
	| '\u1100'..'\u1159'
	| '\u115F'..'\u11A2'
	| '\u11A8'..'\u11F9'
	| '\u1200'..'\u1206'
	| '\u1208'..'\u1246'
	| '\u1248'
	| '\u124A'..'\u124D'
	| '\u1250'..'\u1256'
	| '\u1258'
	| '\u125A'..'\u125D'
	| '\u1260'..'\u1286'
	| '\u1288'
	| '\u128A'..'\u128D'
	| '\u1290'..'\u12AE'
	| '\u12B0'
	| '\u12B2'..'\u12B5'
	| '\u12B8'..'\u12BE'
	| '\u12C0'
	| '\u12C2'..'\u12C5'
	| '\u12C8'..'\u12CE'
	| '\u12D0'..'\u12D6'
	| '\u12D8'..'\u12EE'
	| '\u12F0'..'\u130E'
	| '\u1310'
	| '\u1312'..'\u1315'
	| '\u1318'..'\u131E'
	| '\u1320'..'\u1346'
	| '\u1348'..'\u135A'
	| '\u13A0'..'\u13B0'
	| '\u13B1'..'\u13F4'
	| '\u1401'..'\u1676'
	| '\u1681'..'\u169A'
	| '\u16A0'..'\u16EA'
	| '\u1780'..'\u17B3'
	| '\u1820'..'\u1877'
	| '\u1880'..'\u18A8'
	| '\u1E00'..'\u1E9B'
	| '\u1EA0'..'\u1EE0'
	| '\u1EE1'..'\u1EF9'
	| '\u1F00'..'\u1F15'
	| '\u1F18'..'\u1F1D'
	| '\u1F20'..'\u1F39'
	| '\u1F3A'..'\u1F45'
	| '\u1F48'..'\u1F4D'
	| '\u1F50'..'\u1F57'
	| '\u1F59'
	| '\u1F5B'
	| '\u1F5D'
	| '\u1F5F'..'\u1F7D'
	| '\u1F80'..'\u1FB4'
	| '\u1FB6'..'\u1FBC'
	| '\u1FBE'
	| '\u1FC2'..'\u1FC4'
	| '\u1FC6'..'\u1FCC'
	| '\u1FD0'..'\u1FD3'
	| '\u1FD6'..'\u1FDB'
	| '\u1FE0'..'\u1FEC'
	| '\u1FF2'..'\u1FF4'
	| '\u1FF6'..'\u1FFC'
	| '\u207F'
	| '\u2102'
	| '\u2107'
	| '\u210A'..'\u2113'
	| '\u2115'
	| '\u2119'..'\u211D'
	| '\u2124'
	| '\u2126'
	| '\u2128'
	| '\u212A'..'\u212D'
	| '\u212F'..'\u2131'
	| '\u2133'..'\u2139'
	| '\u2160'..'\u2183'
	| '\u3005'..'\u3007'
	| '\u3021'..'\u3029'
	| '\u3031'..'\u3035'
	| '\u3038'..'\u303A'
	| '\u3041'..'\u3094'
	| '\u309D'..'\u309E'
	| '\u30A1'..'\u30FA'
	| '\u30FC'..'\u30FE'
	| '\u3105'..'\u312C'
	| '\u3131'..'\u318E'
	| '\u31A0'..'\u31B7'
	| '\u3400'
	| '\u4DB5'
	| '\u4E00'
	| '\u9FA5'
	| '\uA000'..'\uA48C'
	| '\uAC00'
	| '\uD7A3'
	| '\uF900'..'\uFA2D'
	| '\uFB00'..'\uFB06'
	| '\uFB13'..'\uFB17'
	| '\uFB1D'
	| '\uFB1F'..'\uFB28'
	| '\uFB2A'..'\uFB36'
	| '\uFB38'..'\uFB3C'
	| '\uFB3E'
	| '\uFB40'..'\uFB41'
	| '\uFB43'..'\uFB44'
	| '\uFB46'..'\uFBB1'
	| '\uFBD3'..'\uFD3D'
	| '\uFD50'..'\uFD8F'
	| '\uFD92'..'\uFDC7'
	| '\uFDF0'..'\uFDFB'
	| '\uFE70'..'\uFE72'
	| '\uFE74'
	| '\uFE76'..'\uFEFC'
	| '\uFF21'..'\uFF3A'
	| '\uFF41'..'\uFF5A'
	| '\uFF66'..'\uFFBE'
	| '\uFFC2'..'\uFFC7'
	| '\uFFCA'..'\uFFCF'
	| '\uFFD2'..'\uFFD7'
	| '\uFFDA'..'\uFFDC'
	;
	
fragment UnicodeCombiningMark	// Any character in the Unicode categories "Non-spacing mark (Mn)"
	: '\u0300'..'\u034E'	// or "Combining spacing mark (Mc)".
	| '\u0360'..'\u0362'
	| '\u0483'..'\u0486'
	| '\u0591'..'\u05A1'
	| '\u05A3'..'\u05B9'
	| '\u05BB'..'\u05BD'
	| '\u05BF' 
	| '\u05C1'..'\u05C2'
	| '\u05C4'
	| '\u064B'..'\u0655'
	| '\u0670'
	| '\u06D6'..'\u06DC'
	| '\u06DF'..'\u06E4'
	| '\u06E7'..'\u06E8'
	| '\u06EA'..'\u06ED'
	| '\u0711'
	| '\u0730'..'\u074A'
	| '\u07A6'..'\u07B0'
	| '\u0901'..'\u0903'
	| '\u093C'
	| '\u093E'..'\u094D'
	| '\u0951'..'\u0954'
	| '\u0962'..'\u0963'
	| '\u0981'..'\u0983'
	| '\u09BC'..'\u09C4'
	| '\u09C7'..'\u09C8'
	| '\u09CB'..'\u09CD'
	| '\u09D7'
	| '\u09E2'..'\u09E3'
	| '\u0A02'
	| '\u0A3C'
	| '\u0A3E'..'\u0A42'
	| '\u0A47'..'\u0A48'
	| '\u0A4B'..'\u0A4D'
	| '\u0A70'..'\u0A71'
	| '\u0A81'..'\u0A83'
	| '\u0ABC'
	| '\u0ABE'..'\u0AC5'
	| '\u0AC7'..'\u0AC9'
	| '\u0ACB'..'\u0ACD'
	| '\u0B01'..'\u0B03'
	| '\u0B3C'
	| '\u0B3E'..'\u0B43'
	| '\u0B47'..'\u0B48'
	| '\u0B4B'..'\u0B4D'
	| '\u0B56'..'\u0B57'
	| '\u0B82'..'\u0B83'
	| '\u0BBE'..'\u0BC2'
	| '\u0BC6'..'\u0BC8'
	| '\u0BCA'..'\u0BCD'
	| '\u0BD7'
	| '\u0C01'..'\u0C03'
	| '\u0C3E'..'\u0C44'
	| '\u0C46'..'\u0C48'
	| '\u0C4A'..'\u0C4D'
	| '\u0C55'..'\u0C56'
	| '\u0C82'..'\u0C83'
	| '\u0CBE'..'\u0CC4'
	| '\u0CC6'..'\u0CC8'
	| '\u0CCA'..'\u0CCD'
	| '\u0CD5'..'\u0CD6'
	| '\u0D02'..'\u0D03'
	| '\u0D3E'..'\u0D43'
	| '\u0D46'..'\u0D48'
	| '\u0D4A'..'\u0D4D'
	| '\u0D57'
	| '\u0D82'..'\u0D83'
	| '\u0DCA'
	| '\u0DCF'..'\u0DD4'
	| '\u0DD6'
	| '\u0DD8'..'\u0DDF'
	| '\u0DF2'..'\u0DF3'
	| '\u0E31'
	| '\u0E34'..'\u0E3A'
	| '\u0E47'..'\u0E4E'
	| '\u0EB1'
	| '\u0EB4'..'\u0EB9'
	| '\u0EBB'..'\u0EBC'
	| '\u0EC8'..'\u0ECD'
	| '\u0F18'..'\u0F19'
	| '\u0F35'
	| '\u0F37'
	| '\u0F39'
	| '\u0F3E'..'\u0F3F'
	| '\u0F71'..'\u0F84'
	| '\u0F86'..'\u0F87'
	| '\u0F90'..'\u0F97'
	| '\u0F99'..'\u0FBC'
	| '\u0FC6'
	| '\u102C'..'\u1032'
	| '\u1036'..'\u1039'
	| '\u1056'..'\u1059'
	| '\u17B4'..'\u17D3'
	| '\u18A9'
	| '\u20D0'..'\u20DC'
	| '\u20E1'
	| '\u302A'..'\u302F'
	| '\u3099'..'\u309A'
	| '\uFB1E'
	| '\uFE20'..'\uFE23'
	;

fragment UnicodeDigit		// Any character in the Unicode category "Decimal number (Nd)".
	: '\u0030'..'\u0039'
	| '\u0660'..'\u0669'
	| '\u06F0'..'\u06F9'
	| '\u0966'..'\u096F'
	| '\u09E6'..'\u09EF'
	| '\u0A66'..'\u0A6F'
	| '\u0AE6'..'\u0AEF'
	| '\u0B66'..'\u0B6F'
	| '\u0BE7'..'\u0BEF'
	| '\u0C66'..'\u0C6F'
	| '\u0CE6'..'\u0CEF'
	| '\u0D66'..'\u0D6F'
	| '\u0E50'..'\u0E59'
	| '\u0ED0'..'\u0ED9'
	| '\u0F20'..'\u0F29'
	| '\u1040'..'\u1049'
	| '\u1369'..'\u1371'
	| '\u17E0'..'\u17E9'
	| '\u1810'..'\u1819'
	| '\uFF10'..'\uFF19'
	;
	
fragment UnicodeConnectorPunctuation	// Any character in the Unicode category "Connector punctuation (Pc)".
	: '\u005F'
	| '\u203F'..'\u2040'
	| '\u30FB'
	| '\uFE33'..'\uFE34'
	| '\uFE4D'..'\uFE4F'
	| '\uFF3F'
	| '\uFF65'
	;
	
Comment
	: '/*' (options {greedy=false;} : .)* '*/' {$channel=HIDDEN;}
	;

LineComment
	: '//' ~(LTERM)* {$channel=HIDDEN;}
	;

LTERM
	: '\n'		// Line feed.
	| '\r'		// Carriage return.
	| '\u2028'	// Line separator.
	| '\u2029'	// Paragraph separator.
	;

WhiteSpace // Tab, vertical tab, form feed, space, non-breaking space and any other unicode "space separator".
	: ('\t' | '\v' | '\f' | ' ' | '\u00A0')	{$channel=HIDDEN;}
	;

