#include "io.hpp"

string
file_read(const string& path)
{
  string s;
  try {
    ifstream ifs(path);
    ifs.exceptions(ifstream::failbit | ifstream::badbit);
    stringstream string;
    string << ifs.rdbuf();
    s = move(string.str());
  } catch (ios::failure& e) {
    throw ios::failure("error reading '" + path + "' " + e.what());
  }
  return s;
}

std::vector<std::string>
split(const std::string& s, char delim)
{
  std::vector<std::string> elems;
  split(s, delim, std::back_inserter(elems));
  return elems;
}

vector<float>
string_to_proportions(const string& str)
{
  auto parts = split(str, ':');
  vector<float> res;
  for (auto& part : parts) {
    res.push_back(stof(part));
  }
  return res;
}
