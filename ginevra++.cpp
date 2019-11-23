/* File: ginevra++.cpp
 * Author: Cole Blakley
 * Purpose:  This program acts as a very simple preprocessor. It is based on the
 *  preprocessor ginevra as given in Arthur Pyster's _Compiler Design and Construction_
 *  (1988 edition). It takes in a .cpp or .h text file, tokenizes it, and recognizes
 *  any #define statements, which it then uses to replace any defined macros, just
 *  like `#define APPLE 8` in C will replace all occurrences of the `APPLE` token
 *  with `8`.
 */
#include <iostream>
#include <fstream>
#include <string>
#include <map>

enum class State : std::int_least16_t {
    Start, Identifier, InIdentifier, InComment, String, InSingleQuote,
    InDoubleQuote, EoF, EoL, Other, Bad
};

namespace Token {
    constexpr int Define = 256;
    constexpr int String = 257;
    constexpr int Identifier = 258;
    constexpr int EoF = 259;
}

class Scanner {
private:
    std::ifstream file;
    char getCh();
public:
    Scanner(const std::string &path);
    int nextToken();
    std::string nextLine();
    std::string currText;
    char currChar;
    int lineNum = 1;
    int currColumn = 1;
};

Scanner::Scanner(const std::string &path) : file(path)
{
    if(!file) {
	std::cerr << "error: could not open input file: " << path << '\n';
	exit(1);
    } else if(file.peek() == EOF) {
	exit(1);
    }
    // Extract first char from stream so nextToken() can be safely called the
    // first time
    currChar = getCh();
}

char Scanner::getCh()
{
    if(file.peek() == '\n') {
	++lineNum;
	currColumn = 1;
    }
    return file.get();
}

std::string Scanner::nextLine()
{
    std::string line;
    std::getline(file, line);
    return line;
}

int Scanner::nextToken()
{
    State currState = State::Start;
    currText = "";
    bool done = false;
    while(!done) {
	switch(currState) {
	case State::Start:
	    if(currChar == ' ' || currChar == '\t') {
	        break;
	    } else if(currChar == EOF) {
		currState = State::EoF;
		done = true;
	    } else if(currChar == '\n') {
		currText += currChar;
		currState = State::EoL;
		currChar = getCh();
		done = true;
	    } else if(currChar == '\'') {
		currText += currChar;
		currState = State::InSingleQuote;
	    } else if(currChar == '"') {
		currText += currChar;
		currState = State::InDoubleQuote;
	    } else if(std::isalpha(currChar)) {
		currText += currChar;
		currState = State::InIdentifier;
	    } else if(currChar == '/' && file.peek() == '*') {
		currChar = getCh();
		currState = State::InComment;
	    } else if(currChar == '\\' && file.peek() == '\n') {
		currChar = getCh();
	    } else {
		currText += currChar;
		currChar = getCh();
		currState = State::Other;
		done = true;
	    }
	    break;
	case State::InSingleQuote:
	    if(currChar == '\'') {
		currText += currChar;
		currState = State::String;
		currChar = getCh();
		done = true;
	    } else if(currChar == '\\' && file.peek() == '\'') {
		currText += '\'';
		currChar = getCh();
	    } else if(currChar == '\\' && file.peek() == '\n') {
		currChar = getCh(); //Skip backslash newline
	    } else if(currChar == EOF || currChar == '\n') {
		currState = State::Bad;
		done = true;
	    } else {
		currText += currChar;
	    }
	    break;
	case State::InDoubleQuote:
	    if(currChar == '"') {
		currText += currChar;
		currState = State::String;
		currChar = getCh();
		done = true;
	    } else if(currChar == '\\' && file.peek() == '"') {
		currText += '\'';
		currChar = getCh();
	    } else if(currChar == '\\' && file.peek() == '\n') {
		currChar = getCh(); //Skip backslash newline
	    } else if(currChar == EOF || currChar == '\n') {
		currState = State::Bad;
		done = true;
	    } else {
		currText += currChar;
	    }
	    break;
	case State::InComment:
	    if(currChar == '*' && file.peek() == '/') {
		//End of comment
		currChar = getCh();
		currState = State::Start;
	    } else if(currChar == EOF) {
		std::cerr << "Error: Unexpected end of input"; exit(1);
	    }
	    break;
	case State::InIdentifier:
	    if(std::isalnum(currChar)) {
		currText += currChar;
		break;
	    }
	    currState = State::Identifier;
	    done = true;
	    break;
	default:
	    break;
	}
	if(!done) {
	    currChar = getCh();
	}
    }

    switch(currState) {
    case State::Bad: {
	std::cerr << "malformed token " << currText;
	return nextToken();
    }
    case State::EoF:
	return Token::EoF;
    case State::EoL:
    case State::Other:
	return currText[0];
    case State::Identifier:
	if(currText == "define") {
	    return Token::Define;
	} else {
	    return Token::Identifier;
	}
    case State::String:
	return Token::String;
    default:
	assert(0 && "Deciding what to return from getToken()");
    }

    assert(0 && "Deciding what to return from getToken()");
    return EOF;
}

/**
   Adds a new symbol/value to the symbol table.
   key: the identifier (the word after #define)
   value: the value of identifier (ex: some number constant)
*/
void defineSymbol(std::map<std::string,std::string> &table, Scanner &scanner,
		  int token)
{
    std::string value;
    const std::string key(scanner.currText);
    token = scanner.nextToken();
    while(true) {
	if(token == Token::EoF) {
	    std::cerr << "error: premature end of file\n";
	    exit(1);
	} else if(token == Token::Identifier) {
	    if(table.find(scanner.currText) != table.end()) {
		value += table[scanner.currText];
	    } else {
		value += scanner.currText;
	    }
	} else if(token == '\n') {
	    table[key] = value;
	    return;
	} else {
	    value += scanner.currText;
	}
	token = scanner.nextToken();
    }
}

int main(int argc, char **argv)
{
    if(argc != 2) {
	std::cout << "usage: ginevra++ filename[.cpp,.h]\n";
	exit(1);
    }
    std::string path(argv[1]);
    if(path.size() < 2 || (path.substr(path.size()-2) != ".h"
			   && path.substr(path.size()-4) != ".cpp")) {
	std::cerr << "Invalid file extension\n"; exit(1);
    }
    Scanner scanner(path);
    std::map<std::string,std::string> symbolTable;

    int token = Token::EoF;
    while((token = scanner.nextToken()) != Token::EoF) {
	if(scanner.currColumn == 1 && token == '#') {
	    token = scanner.nextToken();
	    if(token == Token::Define) {
		token = scanner.nextToken();
		if(token == Token::EoF) {
		    std::cerr << "error: premature end of file\n"; exit(1);
		} else if(token == '\n') {
		    std::cerr << "error: premature end of #define\n";
		} else if(token == Token::Identifier) {
		    if(symbolTable.count(scanner.currText) > 0) {
			std::cerr << "error: multiple symbol definitions\n";
		    }
		    defineSymbol(symbolTable, scanner, token);
		} else {
		    std::cerr << "error: identifier expected after #define\n";
		}
	    } else {
		std::cerr << "warning: # in column 1, but not a #define\n";
		std::cout << '#' << scanner.currText << ' ' << scanner.nextLine() << '\n';
	    }
	} else if(token == Token::Identifier) {
	    if(symbolTable.find(scanner.currText) != symbolTable.end()) {
		std::cout << symbolTable[scanner.currText] << ' ';
	    } else {
		std::cout << scanner.currText << ' ';
	    }
	} else if(token == '\n') {
	    std::cout << '\n';
	} else {
	    std::cout << scanner.currText;
	}
    }
    
    return 0;
}
