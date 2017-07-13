# Contributing Guidelines
Adhering to the following guidelines will increase the likelihood of Lumberyard accepting your pull requests. 

## Compiler Compatibility:
-	Use the C++11 standard whenever possible. 
-	Use only those features of C++11 that are commonly supported by Microsoft Visual Studio 2013/2015 (refer to https://msdn.microsoft.com/en-us/library/hh567368.aspx). 

## Formatting:
-	Lumberyard recommends using the Uncrustify code beautifier to keep your C++ code consistent with the engine code. Refer to http://uncrustify.sourceforge.net/. 
-	Apply indentation in a consistent manner:
	-	Files should start without any indentation.
	-	Use a single additional level of indentation for each nested block of code.
	-	Indent all lines of a block by the same amount.
	-	Make lines a reasonable length.
-	Indent preprocessor statements in a similar way to regular code.
-	When positioning curly braces, open braces on a new line and keep them flush with the outer block's indentation.
-	Always use curly braces for flow control statements. 
-	Each line of code should only include a single statement. 
-	Naming conventions for classes, functions, types and files should adhere to CamelCase and specify what the function does. 
-	All header files must include the directive, "#pragma once". 
-	Use forward declarations to minimize header file dependencies. Compile times are a concern so please put in the effort to minimize include chains. 
-	The following syntax should be used when including header files: #include <Package/SubdirectoryChain/Header.h>
This rule helps disambiguate files from different packages that have the same name. <Object.h> might appear relatively often, but <AZRender/Object.h> is far less likely to. 

## Classes:
-	You must define a default constructor if your class defines member variables and has no other constructors. Unless you have a very specifically targeted optimization, always initialize all the variables to a known state even if the variable state is invalid. 
-	Do not assume any specific properties based on the choice of struct vs class; always use <type_traits> to check the actual properties
-	Public declarations come before private declarations.  Methods should be declared before data members. 
-	All methods that do not modify internal state should be const. All function parameters passed by pointer or reference should be marked const unless they are output parameters. 
-	Use the override keyword wherever possible and omit he keyword virtual when using override.
-	Use the final keyword where its use can be justified. 

## Scoping:
-	All of your code should be in at least a namespace named after the package and conform to the naming convention specified earlier in this document.
-	Place a function's variable declarations in the narrowest possible scope and always initialize variables in their declaration.
-	Static member or global variables that are concrete class objects are completely forbidden. If you must have a global object it should be a pointer, and it must be constructed and destroyed via appropriate functions.

## Commenting Code:
All engineers have a responsibility to maintain and contribute to existing documentation. When documenting code, please follow these guidelines:
-	Use /// for comments.
-	Use /**..*/ for block comments.
-	Use @param, etc. for commands.
-	Full sentences with good grammar are preferable to abbreviated notes. 
