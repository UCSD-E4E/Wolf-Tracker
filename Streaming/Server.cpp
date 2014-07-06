//Author: Herbert Torosyan & Tom Xie


#define FREQ 22050  //Sample Rate
#define CAP_SIZE 2048  //How much to capture at a time (latency)
#define NUM_BUFFERS 3
#define BUFFER_SIZE 4096

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "cv.h"
#include "highgui.h"
#include <SerialStream.h>
#include <stdio.h>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>

#define PORT 8888
using namespace cv;
using namespace LibSerial;
using namespace std;

int serialIsOn=0;

CvCapture*  capture=0;
CvCapture*  capture2=0;
Mat   img0;
Mat   img1;
Mat   img2;
int  is_data_ready = 0;
char cameraTriger='0';
int  serversock, clientsock;
int bloop=0;
int trig=1;
int acounter=0;

std::fstream file; //fstream object

//allows multiple program threads to share the same resource,
//such as file access, but not simultaneously
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//used for serial streaming to arduino
SerialStream serial_port ;


const int SRATE = 44100;
const int SSIZE = 1024;
//ALuint Buffer;
//ALuint Source;
int temp3=0;

//Define methods that are going to be used
void* streamServer(void* arg);
void* streamSerial(void* arg);
void  quit(char* msg, int retval);
void* readFromArduino(void* arg);

int main(int argc, char** argv)
{

   //defines an API for creating and manipulating threads
   pthread_t   thread_s;
   pthread_t   thread_serial;
   pthread_t   thread_read;


     int key;
     int flag =  0;

     //capture from the default cam which is represented by 0
     capture = cvCaptureFromCAM(0);

     if(capture==0)
      quit("CAM 0 not active",1);

     //grabs a frame from camera
     img0 = cvQueryFrame(capture);

   // print the width and height of the frame, needed by the client
   fprintf(stdout, "width = %d\nheight = %d\n\n", img0.size().width, img0.size().height);
   fprintf(stdout, "Press 'q' to quit.\n\n");

   // run the streaming server as a separate thread,pthread_create() shall store the ID of the cre-
   //ated thread in the location referenced by thread_s
   if (pthread_create(&thread_s, NULL, streamServer, NULL)) {
      quit("pthread creation failed.", 1);
   }
   // run the serial stream as a separate thread,pthread_create() shall store the ID of the cre-
   //ated thread in the location referenced by thread_serial
   if (pthread_create(&thread_serial, NULL, streamSerial, NULL)) {
      quit("pthread creation failed.", 1);
   }
   // run the readFromArduino as a separate thread,pthread_create() shall store the ID of the cre-
   //ated thread in the location referenced by thread_read
   if (pthread_create(&thread_read, NULL, readFromArduino, NULL)) {
      quit("pthread creation failed.", 1);
   }

   /* This is the main program loop that operates by grabing a frame from the selected camera.
    * You can choose to switch between two cameras and i handle that through the cameraTeigger
    * variable which indicates which camera to query the frame from. If the secondary camera is
    * selected i have to release the current one from memory and initalize the secondary one to
    * become the primary one and starting grabbing frames from that camera.
    */
   while(true) {
	   if(bloop!=2 && is_data_ready==0)
	   {
	   //If cameraTriger is 0 then we are capturing from the default camera.
	   if(cameraTriger=='0')
	   {
		   //If caputre2 is initalized we first have to deallocate the memory because
		   //most computer can only handle one usb input stream, when it get to more than
		   //one they end up throwing an exception stating that the current bus is busy.
		   if(capture2!=0)
		   {
			   //deallocates the camera from memory
			   cvReleaseCapture(&capture2);
			   //initalized the default cam
			   capture = cvCaptureFromCAM(0);
			   capture2=0;
		   }
		   //caputres the frame from the default cam
		   img0 = cvQueryFrame(capture);
	   }
	   else
	   {
		   if(capture!=0)
		   {
		   	  cvReleaseCapture(&capture);
		   	  capture2 = cvCaptureFromCAM(1);
		   	  capture=0;
		   }

		   		img0 = cvQueryFrame(capture2);


	   }
	   img2=img0;
	   //This trigger is used in the streaming thread to signal when the content is ready to be sent over the socket.
	   is_data_ready = 1;
	   }

	         Mat img5;
	         usleep(10);//original is 50
   }

   //exited the while loop because user pressed 'q'
   fprintf(stderr,"Exiting Main!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

   //cancel the threads
   if (pthread_cancel(thread_s)) {
       quit("pthread_cancel failed.", 1);
   }
   if (pthread_cancel(thread_serial)) {
       quit("pthread_cancel failed.", 1);
   }
   if (pthread_cancel(thread_read)) {
       quit("pthread_cancel failed.", 1);
   }

}

/**
* Sends the frames to the client
*/
void* streamServer(void* arg)
{
   // Structure describing an Internet socket address
   struct sockaddr_in server;

   // make thread cancelable
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

   //creates a socket
   if ((serversock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
       quit("socket() failed", 1);
   }

   //populating the IP, port, and protocol data
   memset(&server, 0, sizeof(server));
   server.sin_family = AF_INET;
   server.sin_port = htons(PORT);
   server.sin_addr.s_addr = INADDR_ANY;

   // binds the address to the serversocket
   if (bind(serversock, (const sockaddr*)&server, sizeof(server)) == -1) {
       quit("bind() failed", 1);
   }

   // listening for a connection
   if (listen(serversock, 10) == -1) {
       quit("listen() failed.", 1);
   }


   /*In the call to accept(), the server is put to sleep and when
   for an incoming client request, the three way TCP handshake*
   is complete, the function accept () wakes up and returns the
   socket descriptor representing the client socket. */
   int mytrig=3;
   while(mytrig)
   {
   if ((clientsock = accept(serversock, NULL, NULL)) == -1) {
	   perror("accept() failed");
   }
   else
   {
	   //So if the accept was succesfull we set the mytrig variable to 0
	   //so we can exit the while loop and move on.
	   mytrig=0;
   }
   }


   //used to ensure correct number of bytes sent
   unsigned int bytes=0;
   unsigned int size;

   while(1)
   {

       //enable the mutex lock to make the below access of shared data safe
       //pthread_mutex_lock(&mutex);
       vector<uchar> buff;

       //First we check to make sure the data is read to be sent using the is_data_ready trigger
       if (is_data_ready) {
			//we set bloop equal to 2 becuase we dont want the main method to grab any data when we are sending data
			bloop=2;
			vector<int> param = vector<int>(2);
			param[0]=CV_IMWRITE_JPEG_QUALITY;


			//This is the comprestion factor, the lower the number the higher the compresssion and the lower the image quality.
			param[1]=65;


			//Now after setting the compression parameters we encode the matric into a compressed jpeg to be sent over
			//to the client
			imencode(".jpeg",img2,buff,param);

			//now we reset bloop so the main method can continue grabing and frames from the camera
			bloop=0;

			//first we need to get the size of the buffer and send that over, so
			//the client knows how many bytes to accept to store in the vector
			size=buff.size();

			bytes = send(clientsock,&size,sizeof(unsigned int),0);

			if (bytes != (sizeof(unsigned int))) {
				 fprintf(stderr, "Connection closed images transfer.\n");
				 close(clientsock);

				 //If the bytes where not correctly sent then we loop trying to restablish connection with the client
				 int mytrig1=3;
				 while(mytrig1)
				 {
				 if ((clientsock = accept(serversock, NULL, NULL)) <0) {
				   perror("accept() failed");
					 //quit("accept() failed image", 1);
				 }
				 else
				 {
				   mytrig1=0;
				 }
				 }
			 }
			else{

			//now that we know the transmition of the file size was succesful we begin sending all the elements of the vector separately
			  for(unsigned int x=0;x<size;x++)
			   {
					bytes += send(clientsock, &buff[x], sizeof(uchar), 0);
			   }
			  trig=1;
			   //restart the connection if not all the bytes were sent
			  if (bytes != (size*sizeof(uchar)+sizeof(unsigned int))) {
				   fprintf(stderr, "Connection closed images transfer.\n");
				   close(clientsock);

				   //If the bytes where not correctly sent then we loop trying to restablish connection with the client
				   int mytrig1=3;
				   while(mytrig1)
				   {
				   if ((clientsock = accept(serversock, NULL, NULL)) <0) {
					   perror("accept() failed");
					   //quit("accept() failed image", 1);
				   }
				   else
				   {
					   mytrig1=0;
				   }
				   }
			   }}
			   trig=1;
			   bytes=0;
			   buff.clear();
			   size=0;

       }
      // pthread_mutex_unlock(&mutex);
       is_data_ready = 0;

       // check to see if terminate request issued
      // pthread_testcancel();

       usleep(150);
   }

}

/**
* print and exit
*/
void quit(char* msg, int retval)
{
    fprintf(stderr, "just went inQUITTTTTT.\n");


   fprintf(stdout, "%s", (msg == NULL ? "" : msg));
   fprintf(stdout, "\n");

   //if (file) file->close();
   serial_port.Close();
   if (clientsock) close(clientsock);
   if (serversock) close(serversock);
   if (capture) cvReleaseCapture(&capture);
   if (capture2) cvReleaseCapture(&capture2);
   if(!(img0.empty())) img0.release();
   if(!(img1.empty())) img1.release();
   if(!(img2.empty())) img2.release();
   pthread_mutex_destroy(&mutex);

   exit(retval);
}


/*
 * aquires the serial data from the client
 */
void* streamSerial(void* arg)
{
	 char test = 'w';
	 //so we have to establish a socket connection again similary to what we did in the stream of the matrix data
	 struct sockaddr_in server;
     int serverserialSock=0;
     int clientserialSock=0;
	   // make thread cancelable
	   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	   pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	   //creates a socket
	   if ((serverserialSock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	       quit("socket() failed", 1);
	   }

	   //populating the IP, port, and protocol data
	   memset(&server, 0, sizeof(server));
	   server.sin_family = AF_INET;
	   server.sin_port = htons(8887);//using different port instead of 8888 so we dont interfere
	   server.sin_addr.s_addr = INADDR_ANY;
	   //server.sin_addr.s_addr =  inet_addr("128.54.214.56");

	   // binds the address to the serversocket
	   if (bind(serverserialSock, (const sockaddr*)&server, sizeof(server)) == -1) {
	       quit("bind() failed 2", 1);
	   }

	   // listening for a connection
	   if (listen(serverserialSock, 10) == -1) {
	       quit("listen() failed.", 1);
	   }


	   /*In the call to accept(), the server is put to sleep and when
	   for an incoming client request, the three way TCP handshake*
	   is complete, the function accept () wakes up and returns the
	   socket descriptor representing the client socket. */
	   int trig3=3;
       while(trig3)
       {
       if ((clientserialSock = accept(serverserialSock, NULL, NULL)) == -1) {
           //quit("accept() failed", 1);
    	   perror("accept() failed");

       }
       else
     	  trig3=0;
       }

	   // First we open our arduino device file which is /dev/ttyUSB0.
       // This portion of the code is setting up the serial_port for writing
       serial_port.Open("../../../../dev/ttyUSB0") ;
       if(!serial_port.good())
       {
    	   serial_port.Close();
    	   serial_port.Open("../../../../dev/ttyACM0") ;
       }
       if ( ! serial_port.good() )
            {
                std::cerr << "[" << __FILE__ << ":" << __LINE__ << "] "
                          << "Error: Could not open serial port."
                          << std::endl ;
                //exit(1) ;
            }
       serial_port.SetBaudRate( SerialStreamBuf::BAUD_9600 ) ;
            if ( ! serial_port.good() )
            {
                std::cerr << "Error: Could not set the baud rate." <<
       std::endl ;
                //exit(1) ;
            }
            //
            // Set the number of data bits.
            //
            serial_port.SetCharSize( SerialStreamBuf::CHAR_SIZE_8 ) ;
            if ( ! serial_port.good() )
            {
                std::cerr << "Error: Could not set the character size." <<
       std::endl ;
                //exit(1) ;
            }
            //
            // Disable parity.
            //
            serial_port.SetParity( SerialStreamBuf::PARITY_NONE ) ;
            if ( ! serial_port.good() )
            {
                std::cerr << "Error: Could not disable the parity." <<
       std::endl ;
                //exit(1) ;
            }
            //
            // Set the number of stop bits.
            //
            serial_port.SetNumOfStopBits( 1 ) ;
            if ( ! serial_port.good() )
            {
                std::cerr << "Error: Could not set the number of stop bits."
                          << std::endl ;
                //exit(1) ;
            }
            //
            // Turn off hardware flow control.
            //
            serial_port.SetFlowControl( SerialStreamBuf::FLOW_CONTROL_NONE ) ;
            if ( ! serial_port.good() )
            {
                std::cerr << "Error: Could not use hardware flow control."
                          << std::endl ;
                //exit(1) ;
            }
            //
            // Do not skip whitespace characters while reading from the
            // serial port.
            //
            // serial_port.unsetf( std::ios_base::skipws ) ;
            //
            // Wait for some data to be available at the serial port.
            //
            //
            // Keep reading data from serial port and print it to the screen.
            //
         // Wait for some data to be available at the serial port.
            //


       serialIsOn=1;
	   if(file==0)
	   {
		   fprintf(stderr,"FAILEDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD");
		   perror("fopen");
	   }
	   //used to keep track of bytes received
	   int bytes = -1;
	   int heartbeat=0;
	   while(1)
	   {
		   //if(heartbeat==1200)
		 // {
			//   char h='H';
			////   char b='B';
			//   char t='T';
		  //	   heartbeat=0;
		  //	serial_port.write(&h, 1);
		  //	serial_port.write(&b, 1);
		  //	serial_port.write(&t, 1);
		 // }
		   bytes = -1;
		   char temp = 'q';
		   int val=0;

		   //we aquire the serial data from the client
		   bytes = recv(clientserialSock,&temp,sizeof(char),0);

		   //check to see correct number of bytes received
		   if (bytes != (sizeof(char))) {
			   trig=0;
		                  fprintf(stderr, "Connection closed stream serial.\n");
		                  close(clientserialSock);


		                  int trig1 = 2;

		                  while(trig1)
		                  {
			                  fprintf(stderr, "Connection closed into the wild.\n");

		                  if ((clientserialSock = accept(serverserialSock, NULL, NULL)) <0) {
			                  fprintf(stderr, "Connection closed into the if.\n");
			            	   perror("accept() failed");
		                      //quit("accept() failed", 1);
		                  }
		                  else{


		                	  trig1=0;
			                  fprintf(stderr, "Connection closed in the else.\n");

		                  }
		                  }
		                  fprintf(stderr, "Connection closed didnt stay in wildloop.\n");
		              }

		   //std::cout<<"Char recv is " << temp;   PRINTING TEH CHARACTER PRESSED

		   //If the char received is one of these char then it was a joystick movement by the cleint
		   if(temp=='x' || temp=='y' || temp=='Y' || temp=='X')
		   {
			   //Now we aquire the magnitude of the joystick movment
			   bytes = recv(clientserialSock,&val,sizeof(int),0);
			   temp3=1;
			   //check to see correct number of bytes received
			   if (bytes != (sizeof(int))) {
				   trig=0;
			                  fprintf(stderr, "Connection closed stream serial part 2.\n");
			                  close(clientserialSock);

			                  int trig2=2;
			                  while(trig2)
			                  {
			                  if ((clientserialSock = accept(serverserialSock, NULL, NULL)) <0) {
			                      //quit("accept() failed", 1);
			                  }
			                  else
			                	  trig2=0;
			                  }
			              }
			char controlVals[] = {'0','1','2','3','4','5','6','7','8','9'};
			//deadzone from -3 to 3 exclusive
			if(val<3 && val>-3)
				val=0;
			   //this writes the magnitude of the joystick to the arduino
               serial_port.write(&temp,1);

               //Now we also have to check the sighn and write the appropriate one
			   if(val>=0){
				char plus = '+';
				serial_port.write(&plus,1);
			    if(val==10) val = 9;
			    serial_port.write(&controlVals[val],1);
			   }
			   else{
				char minus = '-';
				serial_port.write(&minus,1);
				if(val==-10) val = -9;
				serial_port.write(&controlVals[(val*-1)],1);
			   }

			   temp='q';
			   val=0;

		   }
		   else
		   {
			   temp3=1;
			   //If the char A is recived then the client wants to switch cameras
			   if(temp=='A')
			   {
				   acounter++;
			   }
			   //The reason we check to se when acounter is 2 is because we want the user to realease the key before
			   //we switch cameras
			   if(temp=='A' && acounter==2)
			   {
				   acounter=0;
				   //now we set the camera triger varaible which is used in the main thread to select the appropriate camera
				   if(cameraTriger=='0')
				   {
					   cameraTriger='1';
				   }
				   else
				   {
					   cameraTriger='0';
				   }
			   }
			   //We will only reach this is the user select to center the camera, in which we write C to the arduino
			   else
			   {
			   char plop = 'C';
			   serial_port.write(&plop, 1);
			   serial_port.write(&plop, 1);
			   serial_port.write(&plop, 1);
			   }

			   temp = 'q';
		   }
		   //std::cout<<". \n";
	       // check to see if terminate request issued
	       //pthread_testcancel();
		     temp3=0;

           heartbeat++;
		   usleep(1000); //was a 1000
	   }
return 0;
}

/*
 * Reads and transmits data from the arduino to the client
 */
void* readFromArduino(void* arg)
{std:cerr<<"in method";
	struct sockaddr_in server;
	     int serverserialSock=0;
	     int clientserialSock=0;
		   // make thread cancelable
		   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		   pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

		   //creates a socket
		   if ((serverserialSock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		       quit("socket() failed", 1);
		   }

		   //populating the IP, port, and protocol data
		   memset(&server, 0, sizeof(server));
		   server.sin_family = AF_INET;
		   server.sin_port = htons(8886);//using different port instead of 8888 so we dont interfere
		   server.sin_addr.s_addr = INADDR_ANY;
		   //server.sin_addr.s_addr =  inet_addr("128.54.214.56");

		   // binds the address to the serversocket
		   if (bind(serverserialSock, (const sockaddr*)&server, sizeof(server)) == -1) {
		       quit("bind() failed 2", 1);
		   }

		   // listening for a connection
		   if (listen(serverserialSock, 10) == -1) {
		       quit("listen() failed.", 1);
		   }


		   /*In the call to accept(), the server is put to sleep and when
		   for an incoming client request, the three way TCP handshake*
		   is complete, the function accept () wakes up and returns the
		   socket descriptor representing the client socket. */
		   int trig3=3;
	       while(trig3)
	       {
	       if ((clientserialSock = accept(serverserialSock, NULL, NULL)) == -1) {
	           //quit("accept() failed", 1);
	    	   perror("accept() failed");

	       }
	       else
	     	  trig3=0;
	       }


		   //used to keep track of bytes received
		   int bytes = -1;
		   int heartbeat1=0;

		   while(1)
		   {
			   bytes = 0;
			   char temp = 'q';
			   int val=0;
			   if(serialIsOn==1)
			   {
			//Used for debugging
			   if(heartbeat1%100 == 0)
			   {
				   cerr<<heartbeat1;
				   cerr<<"\n";
			   }

			   //We grab the data from the arduino
			   serial_port.read(&temp, 1);
			   	 //And now we send it to the client
			     bytes = send(clientserialSock,&temp,sizeof(char),0);
			     //if the heartbeat1 variable reaches 1200 and we succesfully transmitted the data we write a heart beat
			     //to the ardunio to tell it that the connection is still live
			     //EXPERIENCING SOME PROBLEMS HERE!!!!!
			     if(heartbeat1>=1200 && bytes==sizeof(char))
			    				  {

			    					   char h='H';
			    					   char b='B';
			    					   char t='T';
			    				  	   heartbeat1=0;
			    				  	serial_port.write(&h, 1);
			    				  	serial_port.write(&b, 1);
			    				  	serial_port.write(&t, 1);
			    				  }

			  }

			     if(heartbeat1==600000)
			    			   {
			    				   heartbeat1=0;
			    			   }
			    heartbeat1++;
			    bytes=0;
			   usleep(250);
		   }
return 0;
}

