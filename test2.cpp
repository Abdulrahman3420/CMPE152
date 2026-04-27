#include <iostream>
using namespace std;
int main() {
    float x = 3.5;
    float y = 1.5;
    float sum = x + y;
    float product = x * y;
    bool isPositive = true;
    cout << "x = " << x << endl;
    cout << "y = " << y << endl;
    cout << "sum = " << sum << endl;
    cout << "product = " << product << endl;
    if (isPositive) {
        cout << "All values are positive" << endl;
    }
    return 0;
}
