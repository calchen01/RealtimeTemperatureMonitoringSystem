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
int instr; //0: default/no instruction, 1: change scale to C, 2: change scale to F, 3: change light to red,
		   //4: change light to green, 5: change light to blue, 6: change display to PENN, 7: change display to MCIT
int count;
double max;
double min;
double avg;
double lastFive[5];
pthread_mutex_t lockInstr;
pthread_mutex_t lockCount;
pthread_mutex_t lockMax;
pthread_mutex_t lockMin;
pthread_mutex_t lockAvg;
pthread_mutex_t lockLastFive;

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
  printf("\nServer configured to listen on port %d\n", PORT_NUMBER);
  fflush(stdout);
  // 4. accept: wait here until we get a connection on that port
  int sin_size = sizeof(struct sockaddr_in);
  int fd = accept(sock, (struct sockaddr*) &client_addr, (socklen_t*) &sin_size);
  if (fd != -1) {
	printf("Server got a connection from (%s, %d)\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	// buffer to read data into
	char request[1024];
	// 5. recv: read incoming message (request) into buffer
	int bytes_received = recv(fd, request, 1024, 0);
	// null-terminate the string
	request[bytes_received] = '\0';
	// print it to standard out
	printf("This is the incoming request: %c\n", request[5]);
  if (request[5] == 'C') {
    pthread_mutex_lock(&lockInstr);
    instr = 1;
    pthread_mutex_unlock(&lockInstr);
  } else if (request[5] == 'F') {
    pthread_mutex_lock(&lockInstr);
    instr = 2;
    pthread_mutex_unlock(&lockInstr);
  } else if (request[5] == 'R') {
    pthread_mutex_lock(&lockInstr);
    instr = 3;
    pthread_mutex_unlock(&lockInstr);
  } else if (request[5] == 'G') {
    pthread_mutex_lock(&lockInstr);
    instr = 4;
    pthread_mutex_unlock(&lockInstr);
  } else if (request[5] == 'B') {
    pthread_mutex_lock(&lockInstr);
    instr = 5;
    pthread_mutex_unlock(&lockInstr);
  } else if (request[5] == 'P') {
    pthread_mutex_lock(&lockInstr);
    instr = 6;
    pthread_mutex_unlock(&lockInstr);
  } else if (request[5] == 'M') {
    pthread_mutex_lock(&lockInstr);
    instr = 7;
    pthread_mutex_unlock(&lockInstr);
  }
  char portNumStr[5];
  sprintf(portNumStr, "%d", PORT_NUMBER);
  // this is the message that we'll send back
  char* reply1 = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><body><p id=\"currTemp\">";
  char* reply2 = &msg[0];
  char* reply3 = "</p><button id=\"cButton\">Celcius</button><button id=\"fButton\">Fahrenheit</button><br/><br/><button id=\"rButton\">Change light to red</button><button id=\"gButton\">Change light to green</button><button id=\"bButton\">Change light to blue</button><br/><br/><button id=\"pButton\">Change display to PENN</button><button id=\"mButton\">Change display to MCIT</button><script>var currTemp = document.getElementById(\"currTemp\");\nvar cButton = document.getElementById(\"cButton\");\nvar fButton = document.getElementById(\"fButton\");\nvar rButton = document.getElementById(\"rButton\");\nvar gButton = document.getElementById(\"gButton\");\nvar bButton = document.getElementById(\"bButton\");\nvar pButton = document.getElementById(\"pButton\");\nvar mButton = document.getElementById(\"mButton\");\ncButton.addEventListener(\"click\", handleCButton);\nfButton.addEventListener(\"click\", handleFButton);\nrButton.addEventListener(\"click\", handleRButton);\ngButton.addEventListener(\"click\", handleGButton);\nbButton.addEventListener(\"click\", handleBButton);\npButton.addEventListener(\"click\", handlePButton);\nmButton.addEventListener(\"click\", handleMButton);\nfunction handleCButton() {\nvar arr = currTemp.innerHTML.split(\" \");\nif (arr[5] == \"F\") {\nconst Http = new XMLHttpRequest();\nconst url = \"http://localhost:";
  char* reply4 = "/C\";\nHttp.open(\"GET\", url);\nHttp.send();\nvar fTemp = parseInt(arr[3], 10);\nvar cTemp = (fTemp - 32) * 5 / 9;\ncurrTemp.innerHTML = \"The temperature is \" + cTemp.toFixed(2) + \" degrees C\";\n}\n}\nfunction handleFButton() {\nvar arr = currTemp.innerHTML.split(\" \");\nif (arr[5] == \"C\") {\nconst Http = new XMLHttpRequest();\nconst url = \"http://localhost:";
  char* reply5 = "/F\";\nHttp.open(\"GET\", url);\nHttp.send();\nvar cTemp = parseInt(arr[3], 10);\nvar fTemp = cTemp * 9 / 5 + 32;\ncurrTemp.innerHTML = \"The temperature is \" + fTemp.toFixed(2) + \" degrees F\";\n}\n}\nfunction handleRButton() {\nconst Http = new XMLHttpRequest();\nconst url = \"http://localhost:";
  char* reply6 = "/R\";\nHttp.open(\"GET\", url);\nHttp.send();\n}\nfunction handleGButton() {\nconst Http = new XMLHttpRequest();\nconst url = \"http://localhost:";
  char* reply7 = "/G\";\nHttp.open(\"GET\", url);\nHttp.send();\n}\nfunction handleBButton() {\nconst Http = new XMLHttpRequest();\nconst url = \"http://localhost:";
  char* reply8 = "/B\";\nHttp.open(\"GET\", url);\nHttp.send();\n}\nfunction handlePButton() {\nconst Http = new XMLHttpRequest();\nconst url = \"http://localhost:";
  char* reply9 = "/P\";\nHttp.open(\"GET\", url);\nHttp.send();\n}\nfunction handleMButton() {\nconst Http = new XMLHttpRequest();\nconst url = \"http://localhost:";
  char* reply10 = "/M\";\nHttp.open(\"GET\", url);\nHttp.send();\n}</script></body></html>";
  // 6. send: send the outgoing message (response) over the socket
  // note that the second argument is a char*, and the third is the number of chars	
  send(fd, reply1, strlen(reply1), 0);
  send(fd, reply2, strlen(reply2), 0);
  send(fd, reply3, strlen(reply3), 0);
  send(fd, portNumStr, 4, 0);
  send(fd, reply4, strlen(reply4), 0);
  send(fd, portNumStr, 4, 0);
  send(fd, reply5, strlen(reply5), 0);
  send(fd, portNumStr, 4, 0);
  send(fd, reply6, strlen(reply6), 0);
  send(fd, portNumStr, 4, 0);
  send(fd, reply7, strlen(reply7), 0);
  send(fd, portNumStr, 4, 0);
  send(fd, reply8, strlen(reply8), 0);
  send(fd, portNumStr, 4, 0);
  send(fd, reply9, strlen(reply9), 0);
  send(fd, portNumStr, 4, 0);
  send(fd, reply10, strlen(reply10), 0);
  // 7. close: close the connection
  close(fd);
  printf("Server closed connection\n");
  }
  // 8. close: close the socket
  close(sock);
  printf("Server shutting down\n");
  return 0; //return 0 for standard shutdown
}

// This code configures the file descriptor for use as a serial port.
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
  // try to open the file for reading and writing
  // you may need to change the flags depending on your platform
  int fd = open(filename, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd < 0) {
    perror("Could not open file\n");
    exit(1);
  }
  else {
    printf("Successfully opened %s for reading and writing\n", filename);
  }
  configure(fd);
  // Write the rest of the program below, using the read and write system calls.
  int j = 0;
  char buf[100];
  int end = 0;
  while (1) {
    int i = 0;
    int bytes_read = read(fd, buf, 100);
    if (bytes_read > 0) {
      while (i < 100 && i < bytes_read && buf[i] != '\n') {
        if (buf[i] == 'C' || buf[i] == 'F') {
          end = 1;
        }
        msg[j] = buf[i];
        i++;
        j++;
      }
      if (buf[i] == '\n' && end == 1) {
        pthread_mutex_lock(&lockInstr);
        printf("instr = %d\n", instr);
        if (instr == 1) {
          int bytes_written = write(fd, "4", 1);
        } else if (instr == 2) {
          int bytes_written = write(fd, "5", 1);
        } else if (instr == 3) {
          
        } else if (instr == 4) {
          
        } else if (instr == 5) {
          
        } else if (instr == 6) {
          
        } else if (instr == 7) {
          
        }
        instr = 0;
        pthread_mutex_unlock(&lockInstr);
        msg[j] = '\0';
        printf("%s\n", &msg[0]);
        /* update values */
        char* str = NULL;
        double num = strtod(&msg[18], &str);
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
          avg = (avg * count + num) / (count + 1); //calculate the new avg
          pthread_mutex_unlock(&lockAvg);
          count++;
          if (count <= 5) {
            lastFive[count - 1] = num;
            pthread_mutex_unlock(&lockCount);
          } else { //count > 5
            pthread_mutex_unlock(&lockCount);
            pthread_mutex_lock(&lockLastFive);
            for (int i = 0; i <= 3; i++) {
         	  lastFive[i] = lastFive[i + 1];
        	}
        	lastFive[4] = num;
          pthread_mutex_unlock(&lockLastFive);
          }
        }
        j = 0;
        end = 0;
      }
    }
  }
  close(fd);
  return NULL;
}

void* printStats(void* p) {
  while (1) {
    sleep(5);
    /* print stats */
    pthread_mutex_lock(&lockCount);
    printf("count = %d\n", count);
    if (count == 0) {
      pthread_mutex_unlock(&lockCount);
    } else {
      pthread_mutex_unlock(&lockCount);
      pthread_mutex_lock(&lockMax);
      printf("\nMaximum temperature so far: %.2f degrees C\n", max);
      pthread_mutex_unlock(&lockMax);
      pthread_mutex_lock(&lockMin);
      printf("Minimum temperature so far: %.2f degrees C\n", min);
      pthread_mutex_unlock(&lockMin);
      pthread_mutex_lock(&lockAvg);
      printf("Average temperature so far: %.2f degrees C\n", avg);
      pthread_mutex_unlock(&lockAvg);
      printf("Last 5 temperature readings (degrees C): ");
      pthread_mutex_lock(&lockCount);
      int startLoop = count;
      pthread_mutex_unlock(&lockCount);
      if (startLoop > 5)
        startLoop = 5;
      pthread_mutex_lock(&lockLastFive);
      for (int i = (startLoop - 1); i >= 0; i--) {
        printf("%.2f  ", lastFive[i]);
      }
      pthread_mutex_unlock(&lockLastFive);
      printf("\n");
    }
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  /* initialize global variables */
  instr = 0;
  count = 0;
  max = -DBL_MAX;
  min = DBL_MAX;
  avg = 0;
  for (int i = 0; i < 5; i++) {
    lastFive[i] = 0;
  }
  pthread_mutex_init(&lockInstr, NULL);
  pthread_mutex_init(&lockCount, NULL);
  pthread_mutex_init(&lockMax, NULL);
  pthread_mutex_init(&lockMin, NULL);
  pthread_mutex_init(&lockAvg, NULL);
  pthread_mutex_init(&lockLastFive, NULL);

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
  /* create new threads */
  pthread_t thread1;
  int ret = pthread_create(&thread1, NULL, &usbCom, NULL);
  if (ret != 0) {
    printf("Cannot create thread 1\n");
    return 1; //return 1 for error 1
  }
  pthread_t thread2;
  ret = pthread_create(&thread2, NULL, &printStats, NULL);
  if (ret != 0) {
    printf("Cannot create thread 2\n");
    return 2; // return 2 for error 2
  }
  while (1) {
    start_server(port_number);
  }
  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
  return 0; // return 0 for success
}