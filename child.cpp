#include <iostream>
#include <string>
#include <unistd.h>

using namespace std;

int main(int argc, char* argv[])
{

  cout << "CHILD - This is stdout" << endl;
  if (argc > 1) {
    string t = argv[1];
    cout << "usleep " << t << " * 1000" << endl;
    usleep(stoi(t) * 1000);
  }

  cerr << "CHILD - This is stderr" << endl;
  return 55;
}