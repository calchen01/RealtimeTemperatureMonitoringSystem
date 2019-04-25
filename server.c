#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <pthread.h>
#include <float.h>

char msg[101];
int instr; // 0: default / no instruction, 1: change scale to C, 2: change scale to F, 3: stand-by mode,
		       // 4: resume from stand-by mode / change display to temperature, 5: change light to red, 
		       // 6: change light to green, 7: turn off light, 8: change display to CAFE, 9: change display to CIS
int currScale; // 0: C / default, 1: F
int standby; // 0: not standing by / default, 1: standing by
int disconnected; // 0: connected / default, 1: disconnected
int count;
double curr;
double max;
double min;
double avg;
pthread_mutex_t lockInstr;
pthread_mutex_t lockCurrScale;
pthread_mutex_t lockStandby;
pthread_mutex_t lockDisconnected;
pthread_mutex_t lockCount;
pthread_mutex_t lockCurr;
pthread_mutex_t lockMax;
pthread_mutex_t lockMin;
pthread_mutex_t lockAvg;

int start_server(int PORT_NUMBER) {
  // structs to represent the server and client
  struct sockaddr_in server_addr, client_addr;
  int sock; // socket descriptor
  // 1. socket: creates a socket descriptor that you later use to make other system calls
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	  perror("Socket");
	  exit(1);
  }
  int temp;
  if (setsockopt(sock,SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(int)) == -1) {
	  perror("Setsockopt");
	  exit(1);
  }
  // configure the server
  server_addr.sin_port = htons(PORT_NUMBER); // specify port number
  server_addr.sin_family = AF_INET;         
  server_addr.sin_addr.s_addr = INADDR_ANY; 
  bzero(&(server_addr.sin_zero), 8);
  // 2. bind: use the socket and associate it with the port number
  if (bind(sock, (struct sockaddr*) &server_addr, sizeof(struct sockaddr)) == -1) {
	  perror("Unable to bind");
	  exit(1);
  }
  // 3. listen: indicates that we want to listen to the port to which we bound; second arg is number of allowed connections
  if (listen(sock, 1) == -1) {
	  perror("Listen");
	  exit(1);
  }   
  // once you get here, the server is set up and about to start listening
  // printf("\nServer configured to listen on port %d\n", PORT_NUMBER);
  fflush(stdout);
  // 4. accept: wait here until we get a connection on that port
  int sin_size = sizeof(struct sockaddr_in);
  int fd = accept(sock, (struct sockaddr*) &client_addr, (socklen_t*) &sin_size);
  if (fd != -1) {
	// printf("Server got a connection from (%s, %d)\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	// buffer to read data into
	char request[1024];
	// 5. recv: read incoming message (request) into buffer
	int bytes_received = recv(fd, request, 1024, 0);
	// null-terminate the string
	request[bytes_received] = '\0';
	// print it to standard out
	if (request[5] != 't' && request[5] != 'f')
	  printf("This is the incoming request: %c\n", request[5]);
    if (request[5] == 'C') {
      pthread_mutex_lock(&lockInstr);
      instr = 1;
      pthread_mutex_unlock(&lockInstr);
      pthread_mutex_lock(&lockCurrScale);
      currScale = 0;;
      pthread_mutex_unlock(&lockCurrScale);
    } else if (request[5] == 'F') {
      pthread_mutex_lock(&lockInstr);
      instr = 2;
      pthread_mutex_unlock(&lockInstr);
      pthread_mutex_lock(&lockCurrScale);
      currScale = 1;
      pthread_mutex_unlock(&lockCurrScale);
    } else if (request[5] == 'S') {
      pthread_mutex_lock(&lockInstr);
      instr = 3;
      pthread_mutex_unlock(&lockInstr);
      pthread_mutex_lock(&lockStandby);
      standby = 1;
      pthread_mutex_unlock(&lockStandby);
    } else if (request[5] == 'E') {
      pthread_mutex_lock(&lockInstr);
      instr = 4;
      pthread_mutex_unlock(&lockInstr);
      pthread_mutex_lock(&lockStandby);
      standby = 0;
      pthread_mutex_unlock(&lockStandby);
    } else if (request[5] == 'R') {
      pthread_mutex_lock(&lockInstr);
      instr = 5;
      pthread_mutex_unlock(&lockInstr);
    } else if (request[5] == 'G') {
      pthread_mutex_lock(&lockInstr);
      instr = 6;
      pthread_mutex_unlock(&lockInstr);
    } else if (request[5] == 'O') {
      pthread_mutex_lock(&lockInstr);
      instr = 7;
      pthread_mutex_unlock(&lockInstr);
    } else if (request[5] == 'A') {
      pthread_mutex_lock(&lockInstr);
      instr = 8;
      pthread_mutex_unlock(&lockInstr);
    } else if (request[5] == 'I') {
      pthread_mutex_lock(&lockInstr);
      instr = 9;
      pthread_mutex_unlock(&lockInstr);
    }
    char portNumStr[5];
    sprintf(portNumStr, "%d", PORT_NUMBER);
    // this is the message that we'll send back
    char* reply1 = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><head><meta http-equiv=\"refresh\" content=\"1\" /></head><body><p id=\"text\">";
    char* reply2 = "Current temperature: ";
    pthread_mutex_lock(&lockCurr);
    double temp1 = curr;
    pthread_mutex_unlock(&lockCurr);
    char currStr[7];
    pthread_mutex_lock(&lockCurrScale);
    if (currScale == 0) {
      pthread_mutex_unlock(&lockCurrScale);
      snprintf(currStr, 7, "%.2f", temp1);
  	  currStr[5] = ' ';
  	  currStr[6] = 'C';
    } else {
  	  pthread_mutex_unlock(&lockCurrScale);
  	  temp1 = temp1 * 9 / 5 + 32;
  	  snprintf(currStr, 7, "%.2f", temp1);
  	  currStr[5] = ' ';
  	  currStr[6] = 'F';
    }

    char* reply3 = "<br/>Maximum temperature so far: ";
    pthread_mutex_lock(&lockMax);
    double temp2 = max;
    pthread_mutex_unlock(&lockMax);
    char maxStr[7];
    pthread_mutex_lock(&lockCurrScale);
    if (currScale == 0) {
  	  pthread_mutex_unlock(&lockCurrScale);
  	  snprintf(maxStr, 7, "%.2f", temp2);
  	  maxStr[5] = ' ';
  	  maxStr[6] = 'C';
    } else {
  	  pthread_mutex_unlock(&lockCurrScale);
  	  temp2 = temp2 * 9 / 5 + 32;
  	  snprintf(maxStr, 7, "%.2f", temp2);
  	  maxStr[5] = ' ';
  	  maxStr[6] = 'F';
    }

    char* reply4 = "<br/>Minimum temperature so far: ";
    char minStr[7];
    pthread_mutex_lock(&lockMin);
    double temp3 = min;
    pthread_mutex_unlock(&lockMin);
    pthread_mutex_lock(&lockCurrScale);
    if (currScale == 0) {
      pthread_mutex_unlock(&lockCurrScale);
  	  snprintf(minStr, 7, "%.2f", temp3);
  	  minStr[5] = ' ';
  	  minStr[6] = 'C';
    } else {
  	  pthread_mutex_unlock(&lockCurrScale);
  	  temp3 = temp3 * 9 / 5 + 32;
  	  snprintf(minStr, 7, "%.2f", temp3);
  	  minStr[5] = ' ';
  	  minStr[6] = 'F';
    }

    char* reply5 = "<br/>Average temperature so far: ";
    char avgStr[7];
    pthread_mutex_lock(&lockAvg);
    double temp4 = avg;
    pthread_mutex_unlock(&lockAvg);
    pthread_mutex_lock(&lockCurrScale);
    if (currScale == 0) {
  	  pthread_mutex_unlock(&lockCurrScale);
  	  snprintf(avgStr, 7, "%.2f", temp4);
  	  avgStr[5] = ' ';
  	  avgStr[6] = 'C';
    } else {
  	  pthread_mutex_unlock(&lockCurrScale);
  	  temp4 = temp4 * 9 / 5 + 32;
  	  snprintf(avgStr, 7, "%.2f", temp4);
  	  avgStr[5] = ' ';
  	  avgStr[6] = 'F';
    }

    char* reply6 = "</p><button id=\"cButton\">Celcius</button><button id=\"fButton\">Fahrenheit</button><br/><br/><button id=\"sButton\">Stand-by</button><button id=\"eButton\">Resume</button><br/><br/><button id=\"rButton\">Change light to red</button><button id=\"gButton\">Change light to green</button><button id=\"oButton\">Turn off light</button><br/><br/><button id=\"aButton\">Change display to CAFE</button><button id=\"iButton\">Change display to CIS</button><script>var text = document.getElementById(\"text\");\nvar cButton = document.getElementById(\"cButton\");\nvar fButton = document.getElementById(\"fButton\");\nvar sButton = document.getElementById(\"sButton\");\nvar eButton = document.getElementById(\"eButton\");\nvar rButton = document.getElementById(\"rButton\");\nvar gButton = document.getElementById(\"gButton\");\nvar oButton = document.getElementById(\"oButton\");\nvar aButton = document.getElementById(\"aButton\");\nvar iButton = document.getElementById(\"iButton\");\ncButton.addEventListener(\"click\", handleCButton);\nfButton.addEventListener(\"click\", handleFButton);\nsButton.addEventListener(\"click\", handleSButton);\neButton.addEventListener(\"click\", handleEButton);\nrButton.addEventListener(\"click\", handleRButton);\ngButton.addEventListener(\"click\", handleGButton);\noButton.addEventListener(\"click\", handleOButton);\naButton.addEventListener(\"click\", handleAButton);\niButton.addEventListener(\"click\", handleIButton);\nfunction handleCButton() {\nconst Http = new XMLHttpRequest();\nconst url = \"http://localhost:";
    char* reply7 = "/C\";\nHttp.open(\"GET\", url);\nHttp.send();\n}\nfunction handleFButton() {\nconst Http = new XMLHttpRequest();\nconst url = \"http://localhost:";
    char* reply8 = "/F\";\nHttp.open(\"GET\", url);\nHttp.send();\n}\nfunction handleSButton() {\nconst Http = new XMLHttpRequest();\nconst url = \"http://localhost:";
    char* reply9 = "/S\";\nHttp.open(\"GET\", url);\nHttp.send();\n}\nfunction handleEButton() {\nconst Http = new XMLHttpRequest();\nconst url = \"http://localhost:";
    char* reply10 = "/E\";\nHttp.open(\"GET\", url);\nHttp.send();\n}\nfunction handleRButton() {\nconst Http = new XMLHttpRequest();\nconst url = \"http://localhost:";
    char* reply11 = "/R\";\nHttp.open(\"GET\", url);\nHttp.send();\n}\nfunction handleGButton() {\nconst Http = new XMLHttpRequest();\nconst url = \"http://localhost:";
    char* reply12 = "/G\";\nHttp.open(\"GET\", url);\nHttp.send();\n}\nfunction handleOButton() {\nconst Http = new XMLHttpRequest();\nconst url = \"http://localhost:";
    char* reply13 = "/O\";\nHttp.open(\"GET\", url);\nHttp.send();\n}\nfunction handleAButton() {\nconst Http = new XMLHttpRequest();\nconst url = \"http://localhost:";
    char* reply14 = "/A\";\nHttp.open(\"GET\", url);\nHttp.send();\n}\nfunction handleIButton() {\nconst Http = new XMLHttpRequest();\nconst url = \"http://localhost:";
    char* reply15 = "/I\";\nHttp.open(\"GET\", url);\nHttp.send();\n}</script></body></html>";
    // 6. send: send the outgoing message (response) over the socket
    // note that the second argument is a char*, and the third is the number of chars	
    send(fd, reply1, strlen(reply1), 0);
    pthread_mutex_lock(&lockDisconnected);
    if (disconnected == 1) {
      pthread_mutex_unlock(&lockDisconnected);
      char* disconnectMsg = "Arduino disconnected";
      send(fd, disconnectMsg, strlen(disconnectMsg), 0);
    } else {
      pthread_mutex_unlock(&lockDisconnected);
      pthread_mutex_lock(&lockStandby);
      if (standby == 0) {
        pthread_mutex_unlock(&lockStandby);
        send(fd, reply2, strlen(reply2), 0);
        send(fd, currStr, 7, 0);
        send(fd, reply3, strlen(reply3), 0);
        send(fd, maxStr, 7, 0);
        send(fd, reply4, strlen(reply4), 0);
        send(fd, minStr, 7, 0);
        send(fd, reply5, strlen(reply5), 0);
        send(fd, avgStr, 7, 0);
      } else
        pthread_mutex_unlock(&lockStandby);
    }
    send(fd, reply6, strlen(reply6), 0);
    send(fd, portNumStr, 4, 0);
    send(fd, reply7, strlen(reply7), 0);
    send(fd, portNumStr, 4, 0);
    send(fd, reply8, strlen(reply8), 0);
    send(fd, portNumStr, 4, 0);
    send(fd, reply9, strlen(reply9), 0);
    send(fd, portNumStr, 4, 0);
    send(fd, reply10, strlen(reply10), 0);
    send(fd, portNumStr, 4, 0);
    send(fd, reply11, strlen(reply11), 0);
    send(fd, portNumStr, 4, 0);
    send(fd, reply12, strlen(reply12), 0);
    send(fd, portNumStr, 4, 0);
    send(fd, reply13, strlen(reply13), 0);
    send(fd, portNumStr, 4, 0);
    send(fd, reply14, strlen(reply14), 0);
    send(fd, portNumStr, 4, 0);
    send(fd, reply15, strlen(reply15), 0);
    // 7. close: close the connection
    close(fd);
    // printf("Server closed connection\n");
  }
  // 8. close: close the socket
  close(sock);
  // printf("Server shutting down\n");
  return 0; // return 0 for standard shutdown
}

// this code configures the file descriptor for use as a serial port.
void configure(int fd) {
  struct termios pts;
  tcgetattr(fd, &pts);
  cfsetospeed(&pts, 9600);
  cfsetispeed(&pts, 9600);
  tcsetattr(fd, TCSANOW, &pts);
}

void* usbCom(void* p) {
  // get the name from the command line
  char* filename = "/dev/cu.usbmodem14601";
  while (1) {
    // try to open the file for reading and writing
    // may need to change the flags depending on your platform
    int fd = open(filename, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
      // perror("Could not open file\n");
      continue;
    } else {
      printf("Successfully opened %s for reading and writing\n", filename);
      pthread_mutex_lock(&lockDisconnected);
      disconnected = 0;
      pthread_mutex_unlock(&lockDisconnected);
    }
    configure(fd);
    // write the rest of the program below, using the read and write system calls.
    int j = 0;
    char buf[100];
    int end = 0;
    int countNeg = 0;
    while (1) {
      int i = 0;
      int bytes_read = read(fd, buf, 100);
      if (countNeg > 5000000) {
        pthread_mutex_lock(&lockDisconnected);
        disconnected = 1;
        pthread_mutex_unlock(&lockDisconnected);
        printf("Arduino disconnected\n");
        break;
      }
      if (bytes_read > 0) {
        countNeg = 0;
        while (i < 100 && i < bytes_read && buf[i] != '\n') {
          if (buf[i] == 'C' || buf[i] == 'F')
            end = 1;
          msg[j] = buf[i];
          i++;
          j++;
        }
        if (buf[i] == '\n' && end == 1) {
          pthread_mutex_lock(&lockInstr);
          if (instr == 1) {
            write(fd, "1", 1);
          } else if (instr == 2) {
            write(fd, "2", 1);
          } else if (instr == 3) {
            write(fd, "3", 1);
          } else if (instr == 4) {
            write(fd, "4", 1);
          } else if (instr == 5) {
            write(fd, "5", 1);
          } else if (instr == 6) {
            write(fd, "6", 1);
          } else if (instr == 7) {
            write(fd, "7", 1);
          } else if (instr == 8) {
            write(fd, "8", 1);
          } else if (instr == 9) {
            write(fd, "9", 1);
          }
          instr = 0;
          pthread_mutex_unlock(&lockInstr);
          msg[j] = '\0';
          printf("%s\n", &msg[0]);
          /* update values */
          char* str = NULL;
          double num = strtod(&msg[18], &str);
          // valid temperature range: 5 - 40 C / 41 - 104 F
          if (num >= 5 && num <= 104) {
            if (num >= 40.5)
          	  num = (num - 32) * 5 / 9;
            pthread_mutex_lock(&lockCurr);
            curr = num;
            pthread_mutex_unlock(&lockCurr);
            if (str != NULL && num != 0) {
              if (num > max) {
                pthread_mutex_lock(&lockMax);
        	      max = num;
                pthread_mutex_unlock(&lockMax);
              }
              if (num < min) {
                pthread_mutex_lock(&lockMin);
        	      min = num;
                pthread_mutex_unlock(&lockMin);
              }
              pthread_mutex_lock(&lockAvg);
              pthread_mutex_lock(&lockCount);
              avg = (avg * count + num) / (count + 1); // calculate the new avg
              pthread_mutex_unlock(&lockAvg);
              count++;
              pthread_mutex_unlock(&lockCount);
            }
          }
          j = 0;
          end = 0;
        }
      }
      countNeg++;
    }
    close(fd);
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  /* initialize global variables */
  instr = 0;
  currScale = 0;
  standby = 0;
  disconnected = 0;
  count = 0;
  curr = 0;
  max = -DBL_MAX;
  min = DBL_MAX;
  avg = 0;
  pthread_mutex_init(&lockInstr, NULL);
  pthread_mutex_init(&lockCurrScale, NULL);
  pthread_mutex_init(&lockStandby, NULL);
  pthread_mutex_init(&lockDisconnected, NULL);
  pthread_mutex_init(&lockCount, NULL);
  pthread_mutex_init(&lockCurr, NULL);
  pthread_mutex_init(&lockMax, NULL);
  pthread_mutex_init(&lockMin, NULL);
  pthread_mutex_init(&lockAvg, NULL);
  // check the number of arguments
  if (argc != 2) {
    printf("\nUsage: %s [port_number]\n", argv[0]);
    exit(-1);
  }
  int port_number = atoi(argv[1]);
  if (port_number <= 1024) {
    printf("\nPlease specify a port number greater than 1024\n");
    exit(-1);
  }
  /* create new thread */
  pthread_t thread2;
  int ret = pthread_create(&thread2, NULL, &usbCom, NULL);
  if (ret != 0) {
    printf("Cannot create thread 2\n");
    return 1; // return 1 for error
  }
  while (1) {
    start_server(port_number);
  }
  pthread_join(thread2, NULL);
  return 0; // return 0 for success
}