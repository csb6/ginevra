/* File: better.cpp
 * Author: Cole Blakley
 * Purpose: This program acts as a very simple preprocessor. It is based on the
 *  preprocessor ginevra as given in Arthur Pyster's _Compiler Design and Construction_
 *  (1988 edition). It takes in a .cpp or .h text file, tokenizes it, and recognizes
 *  any #define statements, which it then uses to replace any defined macros, just
 *  like `#define APPLE 8` in C will replace all occurrences of the `APPLE` token
 *  with `8`
 *
 * This is a procedural rewrite of ginevra++.cpp. While the resulting code is
 * somewhat uglier and less extensible when compared to ginevra++.cpp, the
 * conditional logic, finite state machine, and parsing process are much more
 * straighforward while still providing nearly all of the same functionality.
 * As an added bonus, this file is quite a few LOC shorter than main.cpp.
 */
#include <iostream>
#include <fstream>
#include <string>
#include <utility> //for std::pair
#include <map>

// The different types of tokens
enum class State {
    Start, Identifier, InIdentifier, InComment, String, InSingleQuote,
    InDoubleQuote, EoF, Bad, Other
};

// Extract the next token from the text stream, additionally determining
// the type (state) of that token, returning both as a pair.
const std::pair<State,std::string> getToken(std::ifstream &file)
{
    bool done = false;
    char currChar = file.get();
    State currState = State::Start;
    std::string tokenText;
    while(!done) {
	switch(currState) {
	case State::Start: {
	    // Skip whitespace; not significant
	    if(currChar == ' ' || currChar == '\t') {
		break;
	    // Start of `#define` or another `identifier`
	    } else if(currChar == '#' || std::isalpha(currChar)) {
		currState = State::InIdentifier;
		tokenText += currChar;
	    // Opening of single-quote string
	    } else if(currChar == '\'') {
		currState = State::InSingleQuote;
		tokenText += currChar;
	    // Opening of double-quote string
	    } else if(currChar == '"') {
		currState = State::InDoubleQuote;
		tokenText += currChar;
	    // Opening of multi-line comment
	    } else if(currChar == '/' && file.peek() == '*') {
		file.ignore(1);
		currState = State::InComment;
	    // File stream is empty; stop extracting 
	    } else if(currChar == EOF) {
		currState = State::EoF;
		done = true;
	    // Ignore newlines
	    } else if(currChar == '\n') {
		tokenText += currChar;
	    // "Other" possibilities include things like parenthese, brackets,
	    // and other non-string/indentifier/comment things
	    } else {
		currState = State::Other;
		tokenText += currChar;
	    }
	    break;
	}
	case State::InIdentifier: {
	    // Identifiers are words (with optional method calls on them)
	    if(std::isalpha(currChar) || currChar == '.') {
		tokenText += currChar;
		break;
	    }
	    // If reached char that isn't part of identifier, put it back, stop
	    file.putback(currChar);
	    currState = State::Identifier;
	    done = true;
	    break;
	}
	case State::InSingleQuote: {
	    // Closing of string
	    if(currChar == '\'') {
		currState = State::String;
		tokenText += currChar;
		done = true;
	    // Escape backslashes within the string
	    } else if(currChar == '\\' && file.peek() == '\'') {
		tokenText += currChar + file.get();
	    // Can't have newline in middle of string
	    } else if(currChar == '\n') {
		currState = State::Bad;
		std::cerr << "\nError: Malformed string\n";
		currChar = file.get(); //Move onto next line
		done = true;
	    // Can't have string cut-off by end of file
	    } else if(currChar == EOF) {
		currState = State::Bad;
		std::cerr << "\nError: Unexpected end of file\n";
		done = true;
		// Otherwise, just collect the contents of the string
	    } else {
		tokenText += currChar;
	    }
	    break;
	}
	// Double quoted strings are treated essentially the same as single-quotes 
	case State::InDoubleQuote: {
	    if(currChar == '"') {
		currState = State::String;
		tokenText += currChar;
		done = true;
	    } else if(currChar == '\\' && file.peek() == '\"') {
		file.get();
	    } else if(currChar == '\n') {
		currState = State::Bad;
		std::cerr << "\nError: Malformed string\n";
		tokenText += currChar;
		done = true;
	    } else if(currChar == EOF) {
		currState = State::Bad;
		std::cerr << "\nFatal error: Unexpected end of file\n";
		exit(1);
	    } else {
		tokenText += currChar;
	    }
	    break;
	}
	// Ignore all characters in comments until closing `*/`, then stop
	case State::InComment: {
	    if(currChar == '*' || file.peek() == '/') {
		file.ignore(1);
		if(file.peek() == '\n') file.ignore(1);
		currState = State::Start;
	    } else if(currChar == EOF) {
		currState = State::Bad;
		std::cerr << "\nFatal error: Unexpected end of file\n";
		exit(1);
	    }
	    break;
	}
	// Try to process `Other` characters, ignoring comments as necessary
	case State::Other: {
	    if(currChar == ' ' || currChar == '\n') {
		done = true;
	    } else if(currChar == '/' && file.peek() == '*') {
		file.ignore(1);
		currState = State::InComment;
		break;
	    }
	    tokenText += currChar;
	    break;
	}
	default:
	    // Have no clue how to deal with the current character, stop
	    currState = State::Bad;
	    done = true;
	}
	// Every iteration until token is complete, pull a character from the
	// stream
	if(!done) {
	    currChar = file.get();
	}
    }
    // Tell call what type (state) the token is, what the token's content is
    return {currState, tokenText};
}

int main(int argc, char **argv)
{
    if(argc != 2) {
	std::cout << "usage: ./better filename[.cpp,.h]\n";
	exit(1);
    }
    const std::string path(argv[1]);
    if(path.size() < 2 || (path.substr(path.size()-2) != ".h"
			   && path.substr(path.size()-4) != ".cpp")) {
	std::cerr << "Invalid file extension\n";
	exit(1);
    }
    
    std::ifstream file(path);
    if(!file) {
	std::cerr << "Error: file " << path << " can't be found\n";
	exit(1);
    }
    std::map<std::string, std::string> symbolTable;
    while(file) {
	const auto [tokenState, tokenText] = getToken(file);
	// Add symbol/value from all `#define SYMBOL value` statements
	if(tokenText == "#define") {
	    const auto [symbolState, symbol] = getToken(file);
	    std::string value;
	    std::getline(file, value);
	    if(symbolState != State::Identifier) {
		std::cerr << "\nError: expected identifier after #define\n";
		std::cout << symbol << ' ' << value << '\n';
	    } else {
		if(symbolTable.count(symbol) > 0) {
		    std::cout << "\nWarning: symbol " << symbol << " redefined\n";
		}
		// Add the mapping
		symbolTable[symbol] = value;
	    }
	// Print out identifiers separated with 1 space; replace any known symbols
	// with their mapped values
	} else if(tokenState == State::Identifier) {
	    if(symbolTable.count(tokenText) < 1) {
		std::cout << tokenText << ' ';
	    } else {
		std::cout << symbolTable[tokenText] << ' ';
	    }
	} else {
	    std::cout << tokenText;
	}
    }
    std::cout << '\n';
    return 0;
}
