#include "../cxxopts.hpp"
#include <zmqpp/zmqpp.hpp>
#include <iostream>
#include <csignal>
#include <snappy.h>
#include "../defines.h"

using namespace cxxopts;
using namespace std;
using namespace zmqpp;

bool _running = true;

static void stop(int signum) {
  _running = false;
}



int main(int argc, char *argv[]) {

  context ctx;
  socket subscriber(ctx, socket_type::subscribe);
  message message;
  string endpoint(DEFAULT_ENDPOINT);
  string topic(DEFAULT_TOPIC);
  string payload;

  Options options(string(APP_NAME));
  options.add_options()
      ("e,endpoint", "Endpoint URL (" + endpoint + ")", value<string>())
      ("t,topic", "Topic to use (" + topic + ")", value<string>())
      ("h,help", "Print usage");

  auto options_parsed = options.parse(argc, argv);

  if (options_parsed.count("help")) {
    cout << APP_NAME << " ver. " << APP_VERSION << endl;
    cout << options.help() << endl;
    return 0;
  }

  if (options_parsed.count("endpoint")) {
    endpoint = options_parsed["endpoint"].as<string>();
  }

  if (options_parsed.count("topic")) {
    topic = options_parsed["topic"].as<string>();
  }

  subscriber.set(zmqpp::socket_option::receive_timeout, SOCKET_TIMEOUT);
  subscriber.connect(endpoint);
  subscriber.subscribe(topic);

  cout << "Endpoint: " << endpoint << endl << "Topic: " << topic << endl;

  std::signal(SIGINT, stop);

  while (_running) {
    subscriber.receive(message);
    if (message.parts() == 2) {
      message >> topic >> payload;
      cout << "[" << topic << "] Got message: " << payload << endl;
    } else if (message.parts() == 3) {
      string compression_format;
      message >> topic >> compression_format >> payload;
      if (FORMAT_COMPRESSED == compression_format) {
        string uncompressed;
        snappy::Uncompress(payload.data(), payload.length(), &uncompressed);
        cout << "[" << topic << "] Got message (compressed): " << uncompressed << endl;
      } else {
        cout << "[" << topic << "] Got message (uncompressed): " << payload << endl;
      }
    }
  }

  cout << "Disconnecting... ";
  cout.flush();
  subscriber.disconnect(endpoint);
  cout << "done." << endl;

  return 0;
}
