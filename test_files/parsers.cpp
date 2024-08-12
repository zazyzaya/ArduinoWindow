#include <string>
#include <iostream>
#include <fstream>

using namespace std; 

void parse(string resp) {
    cout << resp; 
}

int main() {
    ifstream f; 
    f.open("sunrise_example.txt"); 

    string contents = ""; 
    string line; 
    while (f) {
        f >> line; 
        contents.append(line);
        contents.append("\n");
    }

    parse(contents); 
}