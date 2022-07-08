## Content1

homework1_ParseAST.cpp是用于生成AST，输入test_homework1.txt中的其中一条或多条即可打印出抽象语法树。
		windows下使用方法：g++ homework1_ParseAST.cpp -o homework1.exe
					homework1.exe
					然后输入测试用例即可，测试用例在test_homework1.txt中。

## Content2					

homework2_TypeChecking.cpp是类型检查，输入test_homework2.txt中的其中一条或多条即可打印出包含类型检查的抽象语法树。
		windows下使用方法：g++ homework2_TypeChecking.cpp -o homework2.exe
					homework2.exe
					然后输入测试用例即可，测试用例在test_homework2.txt。

设计思想：在万花筒的基础上，增添了一些ocaml的关键字，可以支持ocaml的关键字和标识符；修改了各种AST的数据结构，使得可以在构造树时进行类型检查最终能打印出可以进行类型检查的树，能够支持ocaml的语法；增加了If else的AST结构和相关的PARSE函数；支持bool类型和int类型以及无法判断的类型，例如let a = a;;中a的类型无法判断，输出为a'。

## Content3

homework3_CodeGeneration_GIT.cpp是语义分析的数值结果，输入test_homework3.txt中的其中一条或多条即可打印出代码的语义分析的解释结果。

windows下使用方法：g++ homework3_CodeGeneration_GIT.cpp -o homework3.exe
					homework3.exe
					然后输入测试用例即可，测试用例在test_homework3.txt。

设计思想：在前两次作业的基础上，为每个ExprAST添加新的虚函数getLexeme，包含了各种表达式的运算顺序，最终返回表达式的数值和类型。

注意：由于个人的疏忽，Content3的内容与原要求（中间代码生成）有出入，请自行甄别。