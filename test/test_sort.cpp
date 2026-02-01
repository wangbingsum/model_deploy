#include <iostream>
#include <algorithm>
#include <vector>
using namespace std;

void sort(vector<int>& vec) {
    int n = vec.size();
    for (int i = 0; i < n - 1; ++i) {
        for (int j = 0; j < n - 1 - i; ++j) {
            if (vec[j] > vec[j + 1]) {
                swap(vec[j], vec[j+1]);
            }
        }
    }
}

template<typename T>
void print(const string& message, vector<T>& vec) {
    cout << message << endl;
    for (auto& item : vec) {
        cout << item << " ";
    }
    cout << endl;
}

int main() {
    vector<int> vec = {2, 4, 6, 8, 3, 5, 8, 0, 1, 2};
    print("before", vec);
    sort(vec);
    print("after", vec);
    
}
