//#include "llvm/ADT/STLExtras.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <map>

using namespace std;

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum Token {
   tok_eof = -1,

   // commands
   tok_let = -2,
   tok_in = -3,
   tok_end = -4,
   tok_rec = -7,
   tok_if = -8,
   tok_else = -9,
   tok_then = -10,

   // primary
   tok_identifier = -5,
   tok_number = -6,

   //op
   tok_mod = -11,
   tok_ne = -12,
   tok_or = -13,
   tok_and = -16,
   //boolean
   tok_false = -14,
   tok_true = -15
};
enum Type {
   type_default = 0,
   type_int = 1,
   type_bool = 2
};
static string types[3] = {"","int","bool"};

static std::string IdentifierStr; // Filled in if tok_identifier
static double NumVal;             // Filled in if tok_number
static map<string, int> Returntype;   

static void releaseMap()
{
   Returntype.erase(Returntype.begin(), Returntype.end());
   Returntype["true"] = 2;
   Returntype["false"] = 2;
}
//an element added when a variable is used or declared
static string getType(string name) {
   if (Returntype.count(name) == 0)
       return "";
   map<string, int>::iterator key = Returntype.find(name);
   return types[key->second];
}

/// gettok - Return the next token from standard input.
static int gettok() {
   static int LastChar = ' ';

   // Skip any whitespace.
   while (isspace(LastChar))
       LastChar = getchar();

   if (isalpha(LastChar)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
       IdentifierStr = LastChar;
       while (isalnum((LastChar = getchar()))||LastChar=='_')
           IdentifierStr += LastChar;

       if (IdentifierStr == "let")
           return tok_let;
       if (IdentifierStr == "rec")
           return tok_rec;
       if (IdentifierStr == "in")
           return tok_in;
       if (IdentifierStr == "mod")
           return tok_mod;
       if (IdentifierStr == "if")
           return tok_if;
       if (IdentifierStr == "else")
           return tok_else;
       if (IdentifierStr == "then")
           return tok_then;
       if (IdentifierStr == "or")
           return tok_or;
       if (IdentifierStr == "false")
           return tok_false;
       if (IdentifierStr == "true")
           return tok_true;
       return tok_identifier;
   }

   if (isdigit(LastChar) || LastChar == '.') { // Number: [0-9.]+
       std::string NumStr;
       do {
           NumStr += LastChar;
           LastChar = getchar();
       } while (isdigit(LastChar) || LastChar == '.');

       NumVal = strtod(NumStr.c_str(), nullptr);
       return tok_number;
   }

   if (LastChar == '#') {
       // Comment until end of line.
       do
           LastChar = getchar();
       while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

       if (LastChar != EOF)
           return gettok();
   }

   // Check for end of file.  Don't eat the EOF.
   if (LastChar == EOF)
       return tok_eof;

   //检查是否是;;结束符号
   if (LastChar == ';')
   {
       int ThisChar = LastChar;
       LastChar = getchar();
       if (LastChar == ';') {
           LastChar = getchar();
           return tok_end;
       }
       else
       {
           //LastChar = getchar();
           return ';';
       }
   }
   if (LastChar == '<')
   {
       int ThisChar = LastChar;
       LastChar = getchar();
       if (LastChar == '>') {
           LastChar = getchar();
           return tok_ne;
       }
       else
       {
           //LastChar = getchar();
           return '<';
       }
   }
   if (LastChar == '|')
   {
       int ThisChar = LastChar;
       LastChar = getchar();
       if (LastChar == '|') {
           LastChar = getchar();
           return tok_or;
       }
       else
       {
           //LastChar = getchar();
           return '|';
       }

   }
   if (LastChar == '&')
   {
       int ThisChar = LastChar;
       LastChar = getchar();
       if (LastChar == '&') {
           LastChar = getchar();
           return tok_and;
       }
       else
       {
           //LastChar = getchar();
           return '&';
       }

   }

   // Otherwise, just return the character as its ascii value,such as , . > + = etc.
   int ThisChar = LastChar;
   LastChar = getchar();
   return ThisChar;
}

//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//

namespace {
   /// ExprAST - Base class for all expression nodes.
   class ExprAST {
   public:
       virtual void dump(int depth) {
           for (int i = 0; i < depth; i++)
               cout << ' ';
       }
       virtual int updateType(int type) { return 0; }
       virtual ~ExprAST() = default;
   };

   void dumpExpr(ExprAST* ast) {
       cout << endl;
       ast->dump(0);
   }

   /// NumberExprAST - Expression class for numeric literals like "1.0".
   class NumberExprAST : public ExprAST {
       double Val;

   public:
       NumberExprAST(double Val) : Val(Val) {}

       virtual void dump(int depth) override {
           ExprAST::dump(depth);
           cout << "|-" << Val << ": int" << endl;
       }
       virtual int updateType(int type){return 1;}
   };

   /// VariableExprAST - Expression class for referencing a variable, like "a".
   class VariableExprAST : public ExprAST {
       std::string Name;

   public:
       VariableExprAST(const std::string& Name) : Name(Name) {}
       virtual void dump(int depth) override {
           ExprAST::dump(depth);
           string type = getType(Name);
           if(type == "")
               cout << "|-" << Name +": "+Name<<"'"<< endl;
           else
               cout << "|-" << Name << ": " << type << endl;
       }
       virtual int updateType(int type)
       {   if(Returntype[Name]<type)
               Returntype[Name] = type;
           return Returntype[Name];
       }
   };

   /// BinaryExprAST - Expression class for a binary operator.
   class BinaryExprAST : public ExprAST {
       char Op;
       std::unique_ptr<ExprAST> LHS, RHS;

   public:
       BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
           std::unique_ptr<ExprAST> RHS)
           : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {
           updateType(0);
       }

       virtual void dump(int depth) {
           ExprAST::dump(depth);
           if (Op == tok_mod)
               cout << "|-" << "mod" <<": int"<< endl;
           else if (Op == tok_ne)
               cout << "|-" << "<>" << ": int" << endl;
           else if(Op == tok_or)
               cout << "|-" << "||" << ": bool" << endl;
           else if (Op == tok_and)
               cout << "|-" << "&&" << ": bool" << endl;
           else if (Op=='='||Op=='<'||Op=='>')
               cout << "|-" << Op << ": bool" << endl;
           else cout << "|-" << Op << ": int" << endl;
           LHS->dump(depth + 4);
           RHS->dump(depth + 4);
       }
       virtual int updateType(int type) {
           if (Op == tok_or || Op == tok_and )
           {
               LHS->updateType(2);
               RHS->updateType(2);
               return 2;
           }
           else
           {
               LHS->updateType(1);
               RHS->updateType(1);
               return 1;
           }
       }
   };

   /// CallExprAST - Expression class for function calls.
   class CallExprAST : public ExprAST {
       std::string Callee;
       std::vector<std::unique_ptr<ExprAST>> Args;

   public:
       CallExprAST(const std::string& Callee,
           std::vector<std::unique_ptr<ExprAST>> Args)
           : Callee(Callee), Args(std::move(Args)) {}

       virtual void dump(int depth) override 
       {
           ExprAST::dump(depth);
           if(getType(Callee) == "")
               cout << "|-" << Callee <<": "<<Callee<<"'"<< endl;
           else 
               cout << "|-" << Callee << ": " <<getType(Callee)<< endl;
           for (int i = 0; i < Args.size(); i++)
           {
               Args[i]->dump(depth + 4);
           }
       }
       virtual int updateType(int type)
       {
           if (Returntype[Callee]>0)
               return Returntype[Callee];
           Returntype[Callee] = type;
           return type;
       }
   };

   class IfElseExprAST :public ExprAST {
       std::vector<std::unique_ptr<ExprAST>> IfArgs;
       std::vector<std::unique_ptr<ExprAST>>ThenArgs;

   public:
       IfElseExprAST(std::vector<std::unique_ptr<ExprAST>> IfArgs, std::vector<std::unique_ptr<ExprAST>>ThenArgs)
           :IfArgs(std::move(IfArgs)), ThenArgs(std::move(ThenArgs)) {}

       virtual void dump(int depth) override {
           int ifsize = IfArgs.size();
           int thensize = ThenArgs.size();
           //cout << ifsize << "  " << thensize << endl;
           if (ifsize == 1&&thensize==1) {
               ExprAST::dump(depth);
               cout << "|-if" << endl;
               IfArgs[0]->dump(depth + 4);
               ExprAST::dump(depth);
               cout << "|-then" << endl;
               ThenArgs[0]->dump(depth + 4);
           }
           else if (ifsize == thensize)
           {
               ExprAST::dump(depth);
               cout << "|-if" << endl;
               IfArgs[0]->dump(depth + 4);
               ExprAST::dump(depth);
               cout << "|-then" << endl;
               ThenArgs[0]->dump(depth + 4);
               for (int i = 1; i < ifsize; i++)
               {
                   cout << "|else -if" << endl;
                   IfArgs[i]->dump(depth + 4);
                   ExprAST::dump(depth);
                   cout << "|-then" << endl;
                   ThenArgs[i]->dump(depth + 4);
               }
           }
           //除了配对的if、elseif 和then，最后还有一个else
           else if (ifsize + 1 == thensize)
           {  
               ExprAST::dump(depth);
               cout << "|-if" << endl;
               IfArgs[0]->dump(depth + 4);
               ExprAST::dump(depth);
               cout << "|-then" << endl;
               ThenArgs[0]->dump(depth + 4);
               for (int i = 1; i < ifsize; i++)
               {
                   cout << "|else -if" << endl;
                   IfArgs[i]->dump(depth + 4);
                   ExprAST::dump(depth);
                   cout << "|-then" << endl;
                   ThenArgs[i]->dump(depth + 4);
               }
               cout << "|-else" << endl;
               ThenArgs[ifsize]->dump(depth + 4);
           }
       }
       virtual int updateType(int type)
       {
           int max=0;
           int temp = 0;
           for (int i = 0; i < IfArgs.size(); i++)
           {
               IfArgs[i]->updateType(2);
           }
           for (int i = 0; i < ThenArgs.size(); i++)
           {
               temp = ThenArgs[i]->updateType(0);
               if (max < temp) max = temp;
           }
           //cout << max<<endl;
           return max;
       }
   };

   /// PrototypeAST - This class represents the "prototype" for a function,
   /// which captures its name  and its argument names (thus implicitly the number
   /// of arguments the function takes).
   class PrototypeAST:public ExprAST{
       std::string Name;
       std::vector<std::string> Args;

   public:
       PrototypeAST(const std::string& Name, std::vector<std::string> Args)
           : Name(Name), Args(std::move(Args)) {}

       const std::string& getName() const { return Name; }

       void print(int depth) {
           for (int i = 0; i < depth; i++) {
               cout << ' ';
           }
       }
       void dump(int depth) 
       {
           //this is a Val
           if (Args.size() == 0)
           {
               print(depth);
               cout << "|-Val: " << getName();
               if (getType(Name) == "")
               {
                   cout << ": " << Name << "'" << endl;
               }
               else cout << ": " << getType(Name) << endl;
           }
           //else this is a function
           else
           {   //print function Name
               print(depth);
               cout << "|-" << getName()<<": ";
               for (int i = 0; i < Args.size(); i++) {
                   if (getType(Args[i]) == "")
                   {
                       cout << Args[i] << "'->";
                   }
                   else
                       cout << getType(Args[i]) << "->";
               }

               //print returnType
               if (getType(Name) == "")
               {
                   cout << Name << "'" << endl;
               }
               else
                   cout << getType(Name) << endl;
               cout << endl;

               //print functionArgs,Parameters0
               if (Args.size() > 0)
                   cout << "|-" << "Parameters:";
               if (getType(Args[0]) == "")
                   cout << Args[0] << "'";
               else
                   cout << getType(Args[0]);
               //print Parameters form index1
               for (int i = 1; i < Args.size(); i++) {
                   if (getType(Args[i]) == "")
                       cout << "," << Args[i] << "'";
                   else
                       cout << "," << getType(Args[i]);
               }
               cout << endl;
               //print each parameter
               for (int i = 0; i < Args.size(); i++) {
                   print(depth + 4);
                   if (getType(Args[i]) == "")
                       cout << "|-" << Args[i] <<": "<<Args[i]<<"'"<< endl;
                   else 
                       cout << "|-" << Args[i] << ": " <<getType(Args[i])<< endl;
               }
           }
       }
       virtual int updateType(int type)
       {   if(type>Returntype[Name])
               Returntype[Name] = type;
           return 0;
       }
   };

   /// FunctionAST - This class represents a function definition itself.
   class FunctionAST :public ExprAST {
       std::unique_ptr<PrototypeAST> Proto;
       std::unique_ptr<ExprAST> Body;

   public:
       FunctionAST(std::unique_ptr<PrototypeAST> Proto,
           std::unique_ptr<ExprAST> Body)
           : Proto(std::move(Proto)), Body(std::move(Body)) {
           updateType(0);
       }

       virtual void dump(int depth) {
           Proto->dump(depth);
           Body->dump(depth);
       }
       virtual int updateType(int type)
       {
           int temp = Body->updateType(0);
           Proto->updateType(temp);
           return 0;
       }
   };

} // end anonymous namespace

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
static int CurTok;
static int getNextToken() { return CurTok = gettok(); }

/// BinopPrecedence - This holds the precedence for each binary operator that is
/// defined.
static std::map<char, int> BinopPrecedence;

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
static int GetTokPrecedence() {
   //if (!isascii(CurTok))
   //  return -1;

   // Make sure it's a declared binop.
   int TokPrec = BinopPrecedence[CurTok];
   if (TokPrec <= 0)
       return -1;
   return TokPrec;
}

/// LogError* - These are little helper functions for error handling.
std::unique_ptr<ExprAST> LogError(const char* Str) {
   fprintf(stderr, "Error: %s\n", Str);
   return nullptr;
}
std::unique_ptr<PrototypeAST> LogErrorP(const char* Str) {
   LogError(Str);
   return nullptr;
}

static std::unique_ptr<ExprAST> ParseExpression();

/// numberexpr ::= number
///消耗一个token 转化为数字
static std::unique_ptr<ExprAST> ParseNumberExpr() {
   auto Result = make_unique<NumberExprAST>(NumVal);
   getNextToken(); // consume the number
   return std::move(Result);
}

/// parenexpr ::= '(' expression ')'
/// 消耗掉一对()中的表达式
static std::unique_ptr<ExprAST> ParseParenExpr() {
   getNextToken(); // eat (.
   auto V = ParseExpression();
   if (!V)
       return nullptr;

   if (CurTok != ')')
       return LogError("expected ')'");
   getNextToken(); // eat ).
   return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
/// 消耗掉一个标识符，根据其后面是否有括号返回变量或函数调用
/// 将其转换为 identifier+
/// case identifer or digit:args
/// case '(': parse Expressions
/// default:exit
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
   std::string IdName = IdentifierStr;

   getNextToken(); // eat identifier.

   std::vector<std::unique_ptr<ExprAST>> Args;
   while (true)
   {     //identifier or number will be args
       if (CurTok == tok_identifier)
       {
           auto Arg = make_unique<VariableExprAST>(IdentifierStr);
           Args.push_back(std::move(Arg));
           getNextToken();
       }
       else if (CurTok == tok_number)
       {
           auto Arg = ParseNumberExpr();
           Args.push_back(std::move(Arg));
           //getNextToken();
       }
       else if (CurTok == '(')
       {
           auto Arg = ParseParenExpr();
           Args.push_back(std::move(Arg));
           /*getNextToken();*/
       }
       else
       {
           break;
       }
   }
   return make_unique<CallExprAST>(IdName, std::move(Args));

}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
/// 消耗掉一个操作数，可以是数字、变量、调用或一对小括号
static std::unique_ptr<ExprAST> ParsePrimary() {
   switch (CurTok) {
   default:
       //cout << CurTok <<IdentifierStr<< endl;
       return LogError("unknown token when expecting an expression");
   case tok_identifier:
       return ParseIdentifierExpr();
   case tok_false:
       return ParseIdentifierExpr();
   case tok_true:
       return ParseIdentifierExpr();
   case tok_number:
       return ParseNumberExpr();
   case '(':
       return ParseParenExpr();
   }
}

/// binoprhs
///   ::= ('+' primary)*
/// 消耗运算符及其后面的表达式直到表达式的结束
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
   std::unique_ptr<ExprAST> LHS) {
   // If this is a binop, find its precedence.
   while (true) {
       int TokPrec = GetTokPrecedence();

       // If this is a binop that binds at least as tightly as the current binop,
       // consume it, otherwise we are done.
       //5*6+3,
       if (TokPrec < ExprPrec)
           return LHS;

       // Okay, we know this is a binop.
       int BinOp = CurTok;
       //cout << "BinOp:" << CurTok;;
       getNextToken(); // eat binop

       // Parse the primary expression after the binary operator.
       auto RHS = ParsePrimary();
       if (!RHS)
           return nullptr;

       // If BinOp binds less tightly with RHS than the operator after RHS, let
       // the pending operator take RHS as its LHS.
       int NextPrec = GetTokPrecedence();
       //8+5*3,如果8*5+3则不重置RHS
       if (TokPrec < NextPrec) {
           RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
           if (!RHS)
               return nullptr;
       }

       // Merge LHS/RHS.
       LHS = make_unique<BinaryExprAST>(BinOp, std::move(LHS),
           std::move(RHS));
   }
}

/// expression
///   ::= primary binoprhs
///转换一个表达式
static std::unique_ptr<ExprAST> ParseExpression() {
   auto LHS = ParsePrimary();
   if (!LHS)
       return nullptr;

   return ParseBinOpRHS(0, std::move(LHS));
}

/// prototype
///   ::= id '(' id* ')'
/// 转换成函数原型
static std::unique_ptr<PrototypeAST> ParsePrototype() {
   if (CurTok != tok_identifier && CurTok != tok_identifier && CurTok != tok_rec)
       return LogErrorP("Expected function name in prototype");
   //如果有rec，消耗掉这个标识符
   if (CurTok == tok_rec)
   {
       getNextToken();
   }
   std::string FnName = IdentifierStr;

   //if (CurTok != '(')
   //  return LogErrorP("Expected '(' in prototype");

   std::vector<std::string> ArgNames;
   while (getNextToken() == tok_identifier)
       ArgNames.push_back(IdentifierStr);
   if (CurTok != '=')
       return LogErrorP("Expected '=' in prototype");

   // success.
   getNextToken(); // eat '='.

   return make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}
static std::unique_ptr<ExprAST> ParseIfElse();
/// definition ::= 'def' prototype expression
/// 遇到def，构造一个函数，包括函数原型和暂时只能是表达式的函数体
static std::unique_ptr<ExprAST> ParseDefinition() {
   getNextToken(); // eat let.
   auto Proto = ParsePrototype();
   if (!Proto)
       return nullptr;
   //let in 的内嵌
   while (CurTok == tok_let)
   {
       ParseDefinition();
       //consume in
       if (CurTok == tok_in) getNextToken();
   }
   //if的处理
   if (CurTok == tok_if)
   {
       auto E = ParseIfElse();
       std::unique_ptr<ExprAST> result = make_unique<FunctionAST>(std::move(Proto), std::move(E));
       dumpExpr(result.get());
       return result;
   }
   //其他情况，直接处理一个表达式
   else {
       if (auto E = ParseExpression())
       {
           std::unique_ptr<ExprAST> result = make_unique<FunctionAST>(std::move(Proto), std::move(E));
           dumpExpr(result.get());
           return result;
       }
   }
   return nullptr;
}

static std::unique_ptr<ExprAST> ParseIfElse() {
   std::vector<std::unique_ptr<ExprAST>> IfArgs;
   std::vector<std::unique_ptr<ExprAST>>ThenArgs;
   //consume if
   getNextToken();
   //get an expression
   if (auto ifSentence = ParseExpression())
   {
       //if next token is "then" 
       if (CurTok == tok_then)
       {   //consume then
           getNextToken();
           while (CurTok == tok_let)
           {
               ParseDefinition();
           }
           //expression
           auto thenSentence = ParseExpression();
           IfArgs.push_back(std::move(ifSentence));
           ThenArgs.push_back(std::move(thenSentence));
       }
       else LogError("expected then !!");
       //consume "else"
       while (true)
       {   //case else
           if (CurTok == tok_else)
           {
               //consume else
               getNextToken();
               if (CurTok == tok_if)
               {
                   //consume if
                   getNextToken();
                   if (auto ifSentence = ParseExpression())
                   {
                       //if next token is "then" 
                       if (CurTok == tok_then)
                       {   //consume then
                           getNextToken();
                           while (CurTok == tok_let)
                           {
                               ParseDefinition();
                           }
                           //expression
                           auto thenSentence = ParseExpression();
                           IfArgs.push_back(std::move(ifSentence));
                           ThenArgs.push_back(std::move(thenSentence));
                       }
                   }
               }
               else
               {
                   while (CurTok == tok_let)
                   {
                       ParseDefinition();
                   }
                   auto thenSentence = ParseExpression();
                   ThenArgs.push_back(std::move(thenSentence));
                   break;
               }
           }
           //case other
           else break;
       }
   }
   unique_ptr<IfElseExprAST> result= make_unique<IfElseExprAST>(std::move(IfArgs), std::move(ThenArgs));
   return result;
}

/// toplevelexpr ::= expression
///消耗一个表达式，并打印其结构，并返回一个虚拟的函数，函数体是该表达式，函数原型是给定的
static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
   if (auto E = ParseExpression()) {
       dumpExpr(E.get());
       // Make an anonymous proto.
       auto Proto = make_unique<PrototypeAST>("__anon_expr",
           std::vector<std::string>());
       return make_unique<FunctionAST>(std::move(Proto), std::move(E));
   }
   return nullptr;
}

/// external ::= 'extern' prototype
///消耗掉extern及其后面的函数原型声明
static std::unique_ptr<PrototypeAST> ParseExtern() {
   getNextToken(); // eat extern.
   return ParsePrototype();
}

//===----------------------------------------------------------------------===//
// Top-Level parsing
//===----------------------------------------------------------------------===//
/// 处理函数定义，转换函数定义，失败则切换到下一个token
static void HandleDefinition() {
   if (ParseDefinition()) {
       fprintf(stderr, "Parsed a function definition.\n");
   }
   else {
       // Skip token for error recovery.
       getNextToken();
   }
}
/// 处理extern，转换函数原型，失败则切换到下一个token
static void HandleExtern() {
   if (ParseExtern()) {
       fprintf(stderr, "Parsed an extern\n");
   }
   else {
       // Skip token for error recovery.
       getNextToken();
   }
}
//把一个表达书转化为一个匿名函数，失败则切换到下一个token
static void HandleTopLevelExpression() {
   // Evaluate a top-level expression into an anonymous function.
   if (ParseTopLevelExpr()) {
       fprintf(stderr, "Parsed a top-level expr\n");
   }
   else {
       // Skip token for error recovery.
       getNextToken();
   }
}

/// top ::= definition | external | expression | ';'
static void MainLoop() {
   while (true) {
       fprintf(stderr, "ready> ");
       switch (CurTok) {
       case tok_eof:
           return;
       case tok_end: // ignore top-level semicolons.
           getNextToken();
           // releaseMap();
           break;
       case tok_let:
           HandleDefinition();
           // releaseMap();
           break;
       case tok_if:
           dumpExpr(ParseIfElse().get());
           // releaseMap();
           break;
       default:
           HandleTopLevelExpression();
           // releaseMap();
           break;
       }
   }
}
int main() {
   // Install standard binary operators.
   // 1 is lowest precedence.
   BinopPrecedence['<'] = 10;
   BinopPrecedence['>'] = 10;
   BinopPrecedence['='] = 10;
   BinopPrecedence['&'] = 10;
   BinopPrecedence[tok_or] = 10;
   BinopPrecedence[tok_ne] = 10;
   BinopPrecedence[tok_and] = 10;
   BinopPrecedence['+'] = 20;
   BinopPrecedence['-'] = 20;
   BinopPrecedence['*'] = 40; // highest.
   BinopPrecedence['/'] = 40; // highest.
   BinopPrecedence[tok_mod] = 40;

   releaseMap();
   // Prime the first token.
   fprintf(stderr, "ready> ");
   getNextToken();

   // Run the main "interpreter loop" now.
   MainLoop();

   return 0;
}
