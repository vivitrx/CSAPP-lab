#include <iostream>
using namespace std;

int main() {
    cout << "Please input the number you want to solve:" << endl;
    long i;
    cin >> hex >> i;
    cout << "Input value: " << i << endl;
    
    long difference = i - 0x402470;
    cout << "Difference (i - 0x402470): " << difference << endl;
    
    long eax = difference / 8;
    cout << "Your solution is: " << eax << endl;
    return 0;
}