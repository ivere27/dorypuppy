#include <iostream>
#include <stdlib.h>
#include <string>
#include <unistd.h>

using namespace std;

int main(int argc, char* argv[])
{

  cout << getpid() << " CHILD - This is stdout" << endl;
  if (argc > 1) {
    cout << getpid() << " usleep " << argv[1] << " * 1000" << endl;
    usleep(strtol(argv[1],NULL,0) * 1000);
  }

  cerr << getpid() << " CHILD - This is stderr" << endl;
  return 55;
}