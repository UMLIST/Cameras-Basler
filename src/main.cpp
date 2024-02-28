#include <iostream>

// Generate a Hello, World! Message. 
// Argv[1] is a name that user can enter as an argument 
// to main function to test functionality

int main(int argc, char** argv) 
{
    std::cout << "HELLO, " << argv[1] << "!"<< std::endl;
}