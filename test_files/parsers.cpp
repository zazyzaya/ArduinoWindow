#include <string>
#include <iostream>
#include <fstream>

#define CIVIL_SUNRISE 0
#define SUNRISE 1
#define SUNSET 2
#define CIVIL_SUNSET 3
#define N_SUNTIMES 4

#define SUN_HOUR 0
#define SUN_MIN 1
#define SUN_SEC 2

using namespace std; 

// gcc parsers.cpp -o main.out -lstdc++; ./main.out 

void parse_timestamp(string tstr, int* times) {
    string timestr = tstr.substr(0, 2); 
    cout << timestr << ":"; 
    int time_int = stoi(timestr); 
    times[SUN_HOUR] = time_int; 

    timestr = tstr.substr(3,2); 
    cout << timestr << ":"; 
    time_int = stoi(timestr); 
    times[SUN_MIN] = time_int; 

    timestr = tstr.substr(6,2); 
    cout << timestr << "\n"; 
    time_int = stoi(timestr);
    times[SUN_SEC] = time_int; 
}

void parse(string resp, int** times) {
    string substr; 
    string timestr; 
    int time_int; 

    // Sunrise 
    size_t idx = resp.find("<sunrise>"); 
    substr = resp.substr(idx+9, 8); 
    parse_timestamp(substr, times[SUNRISE]); 

    // Civil sunrise 
    idx = resp.find("<civil>"); 
    substr = resp.substr(idx+7, 8); 
    parse_timestamp(substr, times[CIVIL_SUNRISE]); 

    // Chop out first half so string::find hits the sunset times 
    resp = resp.substr(idx+6); 
    
    // Sunset 
    idx = resp.find("<sunset>"); 
    substr = resp.substr(idx+8, 8); 
    parse_timestamp(substr, times[SUNSET]); 

    // Civil Sunset
    idx = resp.find("<civil>"); 
    substr = resp.substr(idx+7, 8); 
    parse_timestamp(substr, times[CIVIL_SUNSET]); 
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

    int** times = new int*[N_SUNTIMES]; 
    for (int i=0; i<N_SUNTIMES; i++){
        times[i] = new int[3]; 
    }

    parse(contents, times); 

    for (int i=0; i<N_SUNTIMES; i++) {
        for (int j=0; j<3; j++){
            cout << times[i][j] << ","; 
        }
        cout << "\n"; 
    }
}