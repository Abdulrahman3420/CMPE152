#include <iostream>
using namespace std;
int main() {
    int a = 10;
    int b = 20;
    int c = a + b * 2;
    cout << "Result = " << c << endl;
    if (c >= 50) {
        c = c - 5;
        cout << "After adjustment: " << c << endl;
    } else {
        cout << "c is less than 50" << endl;
    }
    return 0;
}
