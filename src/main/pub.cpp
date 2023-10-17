#include "../cxxopts.hpp"
#include <iostream>
#include <zmqpp/zmqpp.hpp>
#include <snappy.h>
#include <csignal>
#include <cmath>
#include "../defines.h"

using namespace std;
using namespace zmqpp;
using namespace cxxopts;

bool _running = true;

static void stop(int signum) {
  _running = false;
}

double getPoint(int i, double tot, double (*func)(double), double f, double r = 1, double n = 0.1) {
  double a = (double)i / (double)tot;
  return r * func(2 * M_PI * a * f) + (rand() % 100) / 100.0 * n * r;
}

int main(int argc, char *argv[]) {

  context ctx;
  socket publisher(ctx, socket_type::publish);
  message message;
  string compressed;
  string endpoint(DEFAULT_ENDPOINT_BIND);
  string topic(DEFAULT_TOPIC);
  string payload;
  string format(FORMAT_PLAIN);

  Options options(string(APP_NAME));
  options.add_options()
      ("e,endpoint", "Endpoint URL (" + endpoint + ")", value<string>())
      ("t,topic", "Topic to use (" + topic + ")", value<string>())
      ("m,message", "Message to be sent", value<string>())
      ("c,compress", "Compress payload with snappy")
      ("s,stress", "Run a stress test (neglects -m if present)")
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

  if (options_parsed.count("compress")) {
    format = FORMAT_COMPRESSED;
  }

  publisher.bind(endpoint);
  this_thread::sleep_for(chrono::milliseconds(200));

  // stress test
  if (options_parsed.count("stress")) {
    std::signal(SIGINT, stop);
    std::ostringstream pld;
    unsigned long int i = 0, n = 0;
    int n_pts = 200;
    while (_running) {
      pld.str("");
      pld.clear();
      pld << "{"
          << "\"a\":" << ((rand() % 100) / 10.0) << ", "
          << "\"b\":" << ((rand() %100) / 10.0 +5) << ", "
          << "\"long name\":" << ((rand() % 100) / 10.0 - 3) << ", "
          << "\"d\":" << ((rand() % 100) / 2.0);
      if (0 == i % 3) {
        pld << ", \"traj" << n << "\":[[";
        for (int j = 0; j < n_pts; j++) {
          pld << getPoint(j, n_pts, sin, 3) << ", ";
        }
        pld << getPoint(n_pts, n_pts, sin, 3) << "], [";
        for (int j = 0; j < n_pts; j++) {
          pld << getPoint(j, n_pts, cos, 3) << ", ";
        }
        pld << getPoint(n_pts, n_pts, cos, 3) << "]]";
        n++;
      }
      if (0 == i % 10) {
        pld << ", \"wrong\": [1,2,3]";
      }
      pld << "}";

      if (FORMAT_PLAIN == format) {
        message << topic << format << pld.str();
      } else {
        snappy::Compress(pld.str().data(), pld.str().length(), &compressed);
        message << topic << format << compressed;
      }
      publisher.send(message);
      std::this_thread::sleep_for(std::chrono::milliseconds(SOCKET_TIMEOUT / 10));
      i++;
    }
  }

  // Message is on CLI
  else if (options_parsed.count("message")) {
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
  }

  // Read from STDIN
  else {
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
