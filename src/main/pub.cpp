#include "../cxxopts.hpp"
#include <iostream>
#include <zmqpp/zmqpp.hpp>
#include <snappy.h>
#include "../defines.h"

using namespace std;
using namespace zmqpp;
using namespace cxxopts;

int main(int argc, char *argv[]) {

  context ctx;
  socket publisher(ctx, socket_type::publish);
  message message;
  string compressed;
  string endpoint(DEFAULT_ENDPOINT_BIND);
  string topic(DEFAULT_TOPIC);
  string payload;
  string format(FORMAT_PLAIN);

  Options options(argv[0]);
  options.add_options()
      ("e,endpoint", "Endpoint URL (" + endpoint + ")", value<string>())
      ("t,topic", "Topic to use (" + topic + ")", value<string>())
      ("m,message", "Message to be sent", value<string>())
      ("c,compress", "Compress payload with snappy")
      ("h,help", "Print usage");

  auto options_parsed = options.parse(argc, argv);

  if (options_parsed.count("help")) {
    cout << options.help() << endl;
    return 0;
  }

  if (options_parsed.count("endpoint")) {
    endpoint = options_parsed["endpoint"].as<string>();
  }

  if (options_parsed.count("topic")) {
    topic = options_parsed["topic"].as<string>();
  }

  if (options_parsed.count("compress")) {
    format = FORMAT_COMPRESSED;
  }

  publisher.bind(endpoint);
  this_thread::sleep_for(chrono::milliseconds(200));

  if (options_parsed.count("message")) {
    payload = options_parsed["message"].as<string>();
    cout << "Message: " << topic << ":" << payload << endl;
    if (FORMAT_COMPRESSED == format) {
      snappy::Compress(
          payload.data(),
          payload.length(),
          &compressed
          );
      message << topic << format << compressed;
    } else {
      message << topic << format << payload;
    }
    publisher.send(message);
  } else {
    cout << "Reading from STDIN, CTRL-D to exit" << endl;
    if (FORMAT_COMPRESSED == format) {
      while (getline(cin, payload)) {
        snappy::Compress(payload.data(), payload.length(), &compressed);
        message << topic << format << compressed;
        publisher.send(message);
      }
    } else {
      while (getline(cin, payload)) {
        message << topic << format << payload;
        publisher.send(message);
      }
    }
  }

  publisher.close();

  return 0;
}
