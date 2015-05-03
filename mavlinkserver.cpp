/***
 * This example expects the serial port has a loopback on it.
 *
 * Alternatively, you could use an Arduino:
 *
 * <pre>
 *  void setup() {
 *    Serial.begin(<insert your baudrate here>);
 *  }
 *
 *  void loop() {
 *    if (Serial.available()) {
 *      Serial.write(Serial.read());
 *    }
 *  }
 * </pre>
 */

#include <string>
#include <iostream>
#include <cstdio>

// OS Specific sleep
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "serial/serial.h"
#include "mavlinkserver.h"

using std::string;
using std::exception;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;

void my_sleep(unsigned long milliseconds) {
#ifdef _WIN32
      Sleep(milliseconds); // 100 ms
#else
      usleep(milliseconds*1000); // 100 ms
#endif
}

void enumerate_ports()
{
	vector<serial::PortInfo> devices_found = serial::list_ports();

	vector<serial::PortInfo>::iterator iter = devices_found.begin();

	while( iter != devices_found.end() )
	{
		serial::PortInfo device = *iter++;

		printf( "(%s, %s, %s)\n", device.port.c_str(), device.description.c_str(),
     device.hardware_id.c_str() );
	}
}

void print_usage()
{
	cerr << "Usage: test_serial {-e|<serial port address>} ";
    cerr << "<baudrate> [test string]" << endl;
}

int run(int argc, char **argv)
{
  if(argc < 2) {
	  print_usage();
    return 0;
  }

  // Argument 1 is the serial port or enumerate flag
  string port(argv[1]);

  if( port == "-e" ) {
	  enumerate_ports();
	  return 0;
  }
  else if( argc < 3 ) {
	  print_usage();
	  return 1;
  }

  // Argument 2 is the baudrate
  unsigned long baud = 0;
#if defined(WIN32) && !defined(__MINGW32__)
  sscanf_s(argv[2], "%lu", &baud);
#else
  sscanf(argv[2], "%lu", &baud);
#endif

  // port, baudrate, timeout in milliseconds
  serial::Serial my_serial(port, baud, serial::Timeout::simpleTimeout(1000));

  cout << "Is the serial port open?";
  if(my_serial.isOpen())
    cout << " Yes." << endl;
  else
    cout << " No." << endl;

  // Get the Test string
  int count = 0;
  string test_string;
  if (argc == 4) {
    test_string = argv[3];
  } else {
    test_string = "Testing.";
  }


  uint8_t cp;
  mavlink_status_t status, lastStatus;
  uint8_t msgReceived = false;
  mavlink_message_t message;

  // Test the timeout at 500ms
  my_serial.setTimeout(serial::Timeout::max(), 500, 0, 500, 0);
  count = 0;
  cout << "Timeout == 500ms." << endl;
  while (true) {
    int result = my_serial.read(&cp, 1);
    if (result > 0) {
      // the parsing
      msgReceived = mavlink_parse_char(MAVLINK_COMM_1, cp, &message, &status);
      // check for dropped packets
      if ((lastStatus.packet_rx_drop_count != status.packet_rx_drop_count)) {
        printf("ERROR: DROPPED %d PACKETS\n", status.packet_rx_drop_count);
        unsigned char v = cp;
        fprintf(stderr, "%02x ", v);
      }
      lastStatus = status;
    } else {
      fprintf(stderr, "ERROR (or 500ms timeout)\n");
    }
    if (msgReceived) {
      // Report info
      printf("Received message from serial with ID #%d (sys:%d|comp:%d):\n",
             message.msgid, message.sysid, message.compid);

      fprintf(stderr, "Received serial data: ");
      unsigned int i;
      uint8_t buffer[MAVLINK_MAX_PACKET_LEN];

      // check message is write length
      unsigned int messageLength = mavlink_msg_to_send_buffer(buffer, &message);

      // message length error
      if (messageLength > MAVLINK_MAX_PACKET_LEN) {
        fprintf(stderr,
                "\nFATAL ERROR: MESSAGE LENGTH IS LARGER THAN BUFFER SIZE\n");
      }

      // print out the buffer
      else {
        for (i = 0; i < messageLength; i++) {
          unsigned char v = buffer[i];
          fprintf(stderr, "%02x ", v);
        }
        fprintf(stderr, "\n");
      }
      switch (message.msgid)
      {

        case MAVLINK_MSG_ID_HEARTBEAT:
        {
          printf("MAVLINK_MSG_ID_HEARTBEAT\n");
          break;
        }
      }
    }
  }
  return 0;
}

int main(int argc, char **argv) {
  try {
    return run(argc, argv);
  } catch (exception &e) {
    cerr << "Unhandled Exception: " << e.what() << endl;
  }
}
