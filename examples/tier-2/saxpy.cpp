#include "saxpy.hpp"

using namespace std::chrono;

auto
check_saxpy(vector<int> in1, vector<int> in2, vector<int> out, int size, float constant)
{
  auto pos = -1;
  for (auto i = 0; i < size; ++i) {
    int v = (constant * (float)in1[i]) + in2[i];
    if (v != out[i]) {
      cout << "[" << i << "] = " << v << " != " << out[i] << "\n";
      pos = i;
      break;
    }
  }
  return pos;
}

void
saxpy(int argc, char* argv[])
{

  if (argc <= 3) {
    cout << "usage:\n"
         << "<size> <chunksize> <constant> [--devices <plat.dev,...>][--static <prop:prop...>] "
            "[--dynamic <chunks>] [--check]"
            "(--kernel <kernelpath>)]\n"
         << "  eg:\n"
         << "  static: 1024 128 3.14 --devices 0.0,0.1,1.0 --static 0.3:0.2 --check\n"
         << "  dynamic: 10240 128 3.14 --devices 1.0,1.1 --dynamic 4\n";
    throw runtime_error("wrong number of arguments");
  }

  string scheduler = "static";
  auto size = stoi(argv[1]);
  auto chunksize = stoi(argv[2]);
  string propsStr = "0.5";
  float constant = atof(argv[3]);
  auto check = false;
  string kernelPath = "examples/tier-2/saxpy.cl";
  vector<float> props;
  auto argcRest = argc - 1;
  string platDevStr = "0.0";
  auto chunks = 1;
  for (int i = 3; i <= argcRest; ++i) {
    string arg(argv[i]);
    if (arg == "--check") {
      check = true;
    } else if (arg == "--static") {
      scheduler = "static";
      if (argcRest < (i + 1)) {
        throw runtime_error("wrong number of arguments: no static props");
      }
      ++i;
      propsStr = argv[i];
    } else if (arg == "--dynamic") {
      scheduler = "dynamic";
      if (argcRest < (i + 1)) {
        throw runtime_error("wrong number of arguments: no dynamic chunks");
      }
      ++i;
      chunks = stoi(argv[i]);
    } else if (arg == "--kernel") {
      if (argcRest < (i + 1)) {
        throw runtime_error("wrong number of arguments: no kernel path");
      }
      ++i;
      kernelPath = argv[i];
    } else if (arg == "--devices") {
      if (argcRest < (i + 1)) {
        throw runtime_error("wrong number of arguments: no list of platform:device");
      }
      ++i;
      platDevStr = argv[i];
    }
  }

  if (scheduler == "static") {
    props = string_to_proportions(propsStr);
  }

  vector<tuple<uint, uint>> selPlatDev;
  vector<string> platDevList = split(platDevStr, ',');
  for (const auto& platDev : platDevList) {
    vector<string> platDevUnit = split(platDev, '.');
    auto plat = stoi(platDevUnit.at(0));
    auto dev = stoi(platDevUnit.at(1));
    selPlatDev.emplace_back(std::make_tuple(plat, dev));
  }

  cout << "Config:\n";
  cout << "  scheduler: " << scheduler << "\n";
  cout << "  size: " << size << "\n";
  cout << "  chunksize: " << chunksize << "\n";
  cout << "  constant: " << constant << "\n";
  cout << "  check: " << (check ? "yes" : "no") << "\n";
  cout << "  kernel path:" << kernelPath << "\n";
  cout << "  platform.device list: ";
  for (auto& platDev : selPlatDev) {
    cout << std::get<0>(platDev) << "." << std::get<1>(platDev) << " ";
  }
  cout << "\n";
  cout << "  static props: ";
  for (auto& prop : props) {
    cout << prop << " ";
  }
  cout << "\n";
  cout << "  dynamic chunks: " << chunks << "\n";

  string kernelStr;
  try {
    kernelStr = file_read(kernelPath);
  } catch (std::ios::failure& e) {
    cout << "io failure: " << e.what() << "\n";
  }

  size_t lws = 128;
  size_t gws = size;

  auto in1Array = make_shared<vector<int>>(size, 1);
  auto in2Array = make_shared<vector<int>>(size, 2);
  auto outArray = make_shared<vector<int>>(size, 0);

  auto timeInit = std::chrono::system_clock::now().time_since_epoch();

  ecl::StaticScheduler stSched;
  ecl::DynamicScheduler dynSched;

  ecl::Device dev1(0, 0);
  ecl::Device dev2(1, 0);

  vector<ecl::Device> devices;

  for (auto& platDev : selPlatDev) {
    auto plat = std::get<0>(platDev);
    auto dev = std::get<1>(platDev);
    devices.emplace_back(ecl::Device(plat, dev));
  }

  ecl::Runtime runtime(move(devices), { gws, 0, 0 }, lws);

  if (scheduler == "static") {
    runtime.setScheduler(&stSched);
    stSched.setRawProportions(props);
  } else {
    runtime.setScheduler(&dynSched);
    dynSched.setChunks(chunks);
  }

  runtime.setInBuffer(in1Array);
  runtime.setInBuffer(in2Array);
  runtime.setOutBuffer(outArray);
  runtime.setKernel(kernelStr, "saxpy");

  runtime.setKernelArg(0, in1Array);
  runtime.setKernelArg(1, in2Array);
  runtime.setKernelArg(2, outArray);
  runtime.setKernelArg(3, size);
  runtime.setKernelArg(4, constant);

  runtime.run();

  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diffMs = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - timeInit).count();

  cout << "time: " << diffMs << "\n";

  runtime.printStats();

  if (check) {
    auto in1 = *in1Array.get();
    auto in2 = *in2Array.get();
    auto out = *outArray.get();

    auto pos = check_saxpy(in1, in2, out, size, constant);
    auto ok = pos == -1;

    if (ok) {
      cout << "Success\n";
    } else {
      cout << "Failure\n";
    }
  } else {
    cout << "Done\n";
  }
}
