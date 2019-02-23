#ifndef ENGINECL_EXAMPLES_COMMON_IO_HPP
#define ENGINECL_EXAMPLES_COMMON_IO_HPP 1

#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <CL/cl.h>

using std::ifstream;
using std::ios;
using std::move;
using std::string;
using std::stringstream;
using std::vector;

string
file_read(const string& path);

vector<string>
split(const std::string& s, char delim);

template<typename Out>
void
split(const std::string& s, char delim, Out result)
{
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    *(result++) = item;
  }
}

vector<float>
string_to_proportions(const string& str);

#endif // ENGINECL_EXAMPLES_COMMON_IO_HPP
