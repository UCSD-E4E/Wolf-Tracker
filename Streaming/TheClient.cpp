//Author: Herbert Torosyan & Tom Xie

#include <stdint.h>

#include <linux/input.h>

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#ifndef EV_SYN
#define EV_SYN 0
#endif

char *events[EV_MAX + 1]={};
char *keys[KEY_MAX + 1] = {};

char *absval[5] = { "Value", "Min  ", "Max  ", "Fuzz ", "Flat " };

char *relatives[REL_MAX + 1] = {};

char *absolutes[ABS_MAX + 1] ={};

char *misc[MSC_MAX + 1] ={};

char *leds[LED_MAX + 1] ={};
char *repeats[REP_MAX + 1]={};

char *sounds[SND_MAX + 1] ={};

char **names[EV_MAX + 1] ={};

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)	((array[LONG(bit)] >> OFF(bit)) & 1)


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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>

#include <list>
//#include <portaudio.h>
/*
 * Client.cpp
 *
 *  Created on: Dec 20, 2013
 *      Author: herbert
 */


using namespace cv;

IplImage* img;
int       is_data_ready = 0;
int       sock, sockSerial, sockArd;
char*     server_ip;
int       server_port;
int       width;
int       height;
Mat imgage;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int key;

//Define the methods that are being used
void* streamClient(void* arg);
void* streamSerial(void* arg);
void  quit(char* msg, int retval);
void* getArduino(void* arg);

int main(int argc, char** argv)
{


    pthread_t thread_c;
    pthread_t thread_serial;
    pthread_t thread_getArduino;


    if (argc != 5) {
        quit("Usage: stream_client <server_ip> <server_port> <width> <height>", 0);
    }

    /* get the parameters */
    server_ip   = argv[1];
    server_port = atoi(argv[2]);
    width       = atoi(argv[3]);
    height      = atoi(argv[4]);

    /* create image */
    img = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
    cvZero(img);

    /* run the streaming client as a separate thread */
    if (pthread_create(&thread_c, NULL, streamClient, NULL)) {
        quit("pthread_create failed.", 1);
    }
    if (pthread_create(&thread_serial, NULL, streamSerial, NULL)) {
        quit("pthread_create failed.", 1);
    }
    if (pthread_create(&thread_getArduino, NULL, getArduino, NULL)) {
        quit("pthread_create failed.", 1);
    }

    //Initalize the GUI window that will display the frames
    cvNamedWindow("stream_client", CV_WINDOW_AUTOSIZE);

    while(key != 'q') {
        /**
         * Display the received image, make it thread safe
         * by enclosing it using pthread_mutex_lock
         */
        pthread_mutex_lock(&mutex);
        if (is_data_ready) {
        	//When the data is read we show the image to the client and reset the is_data_ready variable so we can recive a new image
            imshow("stream_client", imgage);
            is_data_ready = 0;
            key = cvWaitKey(20);//was 30poop
        }
        //release the mutec lock
        pthread_mutex_unlock(&mutex);

    }

    /* user has pressed 'q', terminate the streaming client */
    if (pthread_cancel(thread_c)) {
        quit("pthread_cancel failed.", 1);
    }
    if (pthread_cancel(thread_serial)) {
        quit("pthread_cancel failed.", 1);
    }
    if (pthread_cancel(thread_getArduino)) {
        quit("pthread_cancel failed.", 1);
    }

    /* free memory */
    cvDestroyWindow("stream_client");
    quit(NULL, 0);
}

/**
 * This is the streaming client, run as separate thread and recives the frames from the server
 */
void* streamClient(void* arg)
{

	//std::vector<uchar>::size_type size;

    struct  sockaddr_in server;

    /* make this thread cancellable using pthread_cancel() */
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    /* create socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        quit("socket() failed.", 1);
    }

    /* setup server parameters */
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(server_ip);
    server.sin_port = htons(server_port);

    /* conct to server */
    while (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        quit("connect() failed.", 1);
    }

    int  imgsize = img->imageSize;
    char sockdata[imgsize];
    int  i, j, k, bytes;

    /* start receiving images */
    while(1)
    {
    	   //set the parameters for the matrix
    	   imgage = Mat::zeros( height,width, CV_8UC3);
    	   int  imgSize = imgage.total()*imgage.elemSize();
    	   uchar sockData[imgSize];

    	 //Receive data here

    	           	vector<int> param = vector<int>(2);

    	            unsigned int size;
    	            //first we get the size of the image
    	           	bytes = recv(sock,&size,sizeof(unsigned int),0);
    	           	vector<uchar> buff = vector<uchar>(size);
    	           	//Now using the size we know how much we have to receive
    	           	for(unsigned int x=0;x<size;x++)
    	           	{
    	              bytes = recv(sock, &buff[x], sizeof(uchar), 0);
    	           	}


    	   pthread_mutex_lock(&mutex);

    	   //Now we decode the compressed image and store it in imgage which is the frame that will be displayed to the user
    	   imgage=imdecode((Mat)buff,CV_LOAD_IMAGE_COLOR);
    	   buff.clear();
    	//Notify the main thread that the data is ready for display
        is_data_ready = 1;
        pthread_mutex_unlock(&mutex);

        /* have we terminated yet? */
        pthread_testcancel();

        /* no, take a rest for a while */
        usleep(1500);

    }
}

/**
 * This function provides a way to exit nicely from the system
 */
void quit(char* msg, int retval)
{
    if (retval == 0) {
        fprintf(stdout, "%s", (msg == NULL ? "" : msg));
        fprintf(stdout, "\n");
    } else {
        fprintf(stderr, "%s", (msg == NULL ? "" : msg));
        fprintf(stderr, "\n");
    }

    //if (sock) close(sock);
    close(sock);
    close(sockSerial);
    if (img) cvReleaseImage(&img);

    pthread_mutex_destroy(&mutex);

    exit(retval);
}

/*
 * This method proccesses the serial data from the XBOX controller
 */
void* streamSerial(void * arg)
{
	//comand line args
    //sudo xboxdrv --config conf.xboxdrv
    // sudo chmod +rwx ../../usr/bin/evtest


	//INITALIZATION OF XBOX DRIVER VARIABLES
	events[EV_SYN] = "Sync";			events[EV_KEY] = "Key";
			events[EV_REL] = "Relative";			events[EV_ABS] = "Absolute";
			events[EV_MSC] = "Misc";			events[EV_LED] = "LED";
			events[EV_SND] = "Sound";			events[EV_REP] = "Repeat";
			events[EV_FF] = "ForceFeedback";		events[EV_PWR] = "Power";
			events[EV_FF_STATUS] = "ForceFeedbackStatus";



				keys[KEY_1] = "1";				keys[KEY_2] = "2";
				keys[KEY_3] = "3";				keys[KEY_4] = "4";
				keys[KEY_5] = "5";				keys[KEY_6] = "6";
				keys[KEY_7] = "7";				keys[KEY_8] = "8";
				keys[KEY_9] = "9";				keys[KEY_0] = "0";
				keys[KEY_MINUS] = "Minus";			keys[KEY_EQUAL] = "Equal";
				keys[KEY_BACKSPACE] = "Backspace";		keys[KEY_TAB] = "Tab";
				keys[KEY_Q] = "Q";				keys[KEY_W] = "W";
				keys[KEY_E] = "E";				keys[KEY_R] = "R";
				keys[KEY_T] = "T";				keys[KEY_Y] = "Y";
				keys[KEY_U] = "U";				keys[KEY_I] = "I";
				keys[KEY_O] = "O";				keys[KEY_P] = "P";
				keys[KEY_LEFTBRACE] = "LeftBrace";		keys[KEY_RIGHTBRACE] = "RightBrace";
				keys[KEY_ENTER] = "Enter";			keys[KEY_LEFTCTRL] = "LeftControl";
				keys[KEY_A] = "A";				keys[KEY_S] = "S";
				keys[KEY_D] = "D";				keys[KEY_F] = "F";
				keys[KEY_G] = "G";				keys[KEY_H] = "H";
				keys[KEY_J] = "J";				keys[KEY_K] = "K";
				keys[KEY_L] = "L";				keys[KEY_SEMICOLON] = "Semicolon";
				keys[KEY_APOSTROPHE] = "Apostrophe";	keys[KEY_GRAVE] = "Grave";
				keys[KEY_LEFTSHIFT] = "LeftShift";		keys[KEY_BACKSLASH] = "BackSlash";
				keys[KEY_Z] = "Z";				keys[KEY_X] = "X";
				keys[KEY_C] = "C";				keys[KEY_V] = "V";
				keys[KEY_B] = "B";				keys[KEY_N] = "N";
				keys[KEY_M] = "M";				keys[KEY_COMMA] = "Comma";
				keys[KEY_DOT] = "Dot";			keys[KEY_SLASH] = "Slash";
				keys[KEY_RIGHTSHIFT] = "RightShift";	keys[KEY_KPASTERISK] = "KPAsterisk";
				keys[KEY_LEFTALT] = "LeftAlt";		keys[KEY_SPACE] = "Space";
				keys[KEY_CAPSLOCK] = "CapsLock";		keys[KEY_F1] = "F1";
				keys[KEY_F2] = "F2";			keys[KEY_F3] = "F3";
				keys[KEY_F4] = "F4";			keys[KEY_F5] = "F5";
				keys[KEY_F6] = "F6";			keys[KEY_F7] = "F7";
				keys[KEY_F8] = "F8";			keys[KEY_F9] = "F9";
				keys[KEY_F10] = "F10";			keys[KEY_NUMLOCK] = "NumLock";
				keys[KEY_SCROLLLOCK] = "ScrollLock";	keys[KEY_KP7] = "KP7";
				keys[KEY_KP8] = "KP8";			keys[KEY_KP9] = "KP9";
				keys[KEY_KPMINUS] = "KPMinus";		keys[KEY_KP4] = "KP4";
				keys[KEY_KP5] = "KP5";			keys[KEY_KP6] = "KP6";
				keys[KEY_KPPLUS] = "KPPlus";		keys[KEY_KP1] = "KP1";
				keys[KEY_KP2] = "KP2";			keys[KEY_KP3] = "KP3";
				keys[KEY_KP0] = "KP0";			keys[KEY_KPDOT] = "KPDot";
				keys[KEY_ZENKAKUHANKAKU] = "Zenkaku/Hankaku"; keys[KEY_102ND] = "102nd";
				keys[KEY_F11] = "F11";			keys[KEY_F12] = "F12";
				keys[KEY_RO] = "RO";			keys[KEY_KATAKANA] = "Katakana";
				keys[KEY_HIRAGANA] = "HIRAGANA";		keys[KEY_HENKAN] = "Henkan";
				keys[KEY_KATAKANAHIRAGANA] = "Katakana/Hiragana"; keys[KEY_MUHENKAN] = "Muhenkan";
				keys[KEY_KPJPCOMMA] = "KPJpComma";		keys[KEY_KPENTER] = "KPEnter";
				keys[KEY_RIGHTCTRL] = "RightCtrl";		keys[KEY_KPSLASH] = "KPSlash";
				keys[KEY_SYSRQ] = "SysRq";			keys[KEY_RIGHTALT] = "RightAlt";
				keys[KEY_LINEFEED] = "LineFeed";		keys[KEY_HOME] = "Home";
				keys[KEY_UP] = "Up";			keys[KEY_PAGEUP] = "PageUp";
				keys[KEY_LEFT] = "Left";			keys[KEY_RIGHT] = "Right";
				keys[KEY_END] = "End";			keys[KEY_DOWN] = "Down";
				keys[KEY_PAGEDOWN] = "PageDown";		keys[KEY_INSERT] = "Insert";
				keys[KEY_DELETE] = "Delete";		keys[KEY_MACRO] = "Macro";
				keys[KEY_MUTE] = "Mute";			keys[KEY_VOLUMEDOWN] = "VolumeDown";
				keys[KEY_VOLUMEUP] = "VolumeUp";		keys[KEY_POWER] = "Power";
				keys[KEY_KPEQUAL] = "KPEqual";		keys[KEY_KPPLUSMINUS] = "KPPlusMinus";
				keys[KEY_PAUSE] = "Pause";			keys[KEY_KPCOMMA] = "KPComma";
				keys[KEY_HANGUEL] = "Hanguel";		keys[KEY_HANJA] = "Hanja";
				keys[KEY_YEN] = "Yen";			keys[KEY_LEFTMETA] = "LeftMeta";
				keys[KEY_RIGHTMETA] = "RightMeta";		keys[KEY_COMPOSE] = "Compose";
				keys[KEY_STOP] = "Stop";			keys[KEY_AGAIN] = "Again";
				keys[KEY_PROPS] = "Props";			keys[KEY_UNDO] = "Undo";
				keys[KEY_FRONT] = "Front";			keys[KEY_COPY] = "Copy";
				keys[KEY_OPEN] = "Open";			keys[KEY_PASTE] = "Paste";
				keys[KEY_FIND] = "Find";			keys[KEY_CUT] = "Cut";
				keys[KEY_HELP] = "Help";			keys[KEY_MENU] = "Menu";
				keys[KEY_CALC] = "Calc";			keys[KEY_SETUP] = "Setup";
				keys[KEY_SLEEP] = "Sleep";			keys[KEY_WAKEUP] = "WakeUp";
				keys[KEY_FILE] = "File";			keys[KEY_SENDFILE] = "SendFile";
				keys[KEY_DELETEFILE] = "DeleteFile";	keys[KEY_XFER] = "X-fer";
				keys[KEY_PROG1] = "Prog1";			keys[KEY_PROG2] = "Prog2";
				keys[KEY_WWW] = "WWW";			keys[KEY_MSDOS] = "MSDOS";
				keys[KEY_COFFEE] = "Coffee";		keys[KEY_DIRECTION] = "Direction";
				keys[KEY_CYCLEWINDOWS] = "CycleWindows";	keys[KEY_MAIL] = "Mail";
				keys[KEY_BOOKMARKS] = "Bookmarks";		keys[KEY_COMPUTER] = "Computer";
				keys[KEY_BACK] = "Back";			keys[KEY_FORWARD] = "Forward";
				keys[KEY_CLOSECD] = "CloseCD";		keys[KEY_EJECTCD] = "EjectCD";
				keys[KEY_EJECTCLOSECD] = "EjectCloseCD";	keys[KEY_NEXTSONG] = "NextSong";
				keys[KEY_PLAYPAUSE] = "PlayPause";		keys[KEY_PREVIOUSSONG] = "PreviousSong";
				keys[KEY_STOPCD] = "StopCD";		keys[KEY_RECORD] = "Record";
				keys[KEY_REWIND] = "Rewind";		keys[KEY_PHONE] = "Phone";
				keys[KEY_ISO] = "ISOKey";			keys[KEY_CONFIG] = "Config";
				keys[KEY_HOMEPAGE] = "HomePage";		keys[KEY_REFRESH] = "Refresh";
				keys[KEY_EXIT] = "Exit";			keys[KEY_MOVE] = "Move";
				keys[KEY_EDIT] = "Edit";			keys[KEY_SCROLLUP] = "ScrollUp";
				keys[KEY_SCROLLDOWN] = "ScrollDown";	keys[KEY_KPLEFTPAREN] = "KPLeftParenthesis";
				keys[KEY_KPRIGHTPAREN] = "KPRightParenthesis"; keys[KEY_F13] = "F13";
				keys[KEY_F14] = "F14";			keys[KEY_F15] = "F15";
				keys[KEY_F16] = "F16";			keys[KEY_F17] = "F17";
				keys[KEY_F18] = "F18";			keys[KEY_F19] = "F19";
				keys[KEY_F20] = "F20";			keys[KEY_F21] = "F21";
				keys[KEY_F22] = "F22";			keys[KEY_F23] = "F23";
				keys[KEY_F24] = "F24";			keys[KEY_PLAYCD] = "PlayCD";
				keys[KEY_PAUSECD] = "PauseCD";		keys[KEY_PROG3] = "Prog3";
				keys[KEY_PROG4] = "Prog4";			keys[KEY_SUSPEND] = "Suspend";
				keys[KEY_CLOSE] = "Close";			keys[KEY_PLAY] = "Play";
				keys[KEY_FASTFORWARD] = "Fast Forward";	keys[KEY_BASSBOOST] = "Bass Boost";
				keys[KEY_PRINT] = "Print";			keys[KEY_HP] = "HP";
				keys[KEY_CAMERA] = "Camera";		keys[KEY_SOUND] = "Sound";
				keys[KEY_QUESTION] = "Question";		keys[KEY_EMAIL] = "Email";
				keys[KEY_CHAT] = "Chat";			keys[KEY_SEARCH] = "Search";
				keys[KEY_CONNECT] = "Connect";		keys[KEY_FINANCE] = "Finance";
				keys[KEY_SPORT] = "Sport";			keys[KEY_SHOP] = "Shop";
				keys[KEY_ALTERASE] = "Alternate Erase";	keys[KEY_CANCEL] = "Cancel";
				keys[KEY_BRIGHTNESSDOWN] = "Brightness down"; keys[KEY_BRIGHTNESSUP] = "Brightness up";
				keys[KEY_MEDIA] = "Media";			keys[KEY_UNKNOWN] = "Unknown";
				keys[BTN_0] = "Btn0";			keys[BTN_1] = "Btn1";
				keys[BTN_2] = "Btn2";			keys[BTN_3] = "Btn3";
				keys[BTN_4] = "Btn4";			keys[BTN_5] = "Btn5";
				keys[BTN_6] = "Btn6";			keys[BTN_7] = "Btn7";
				keys[BTN_8] = "Btn8";			keys[BTN_9] = "Btn9";
				keys[BTN_LEFT] = "LeftBtn";			keys[BTN_RIGHT] = "RightBtn";
				keys[BTN_MIDDLE] = "MiddleBtn";		keys[BTN_SIDE] = "SideBtn";
				keys[BTN_EXTRA] = "ExtraBtn";		keys[BTN_FORWARD] = "ForwardBtn";
				keys[BTN_BACK] = "BackBtn";			keys[BTN_TASK] = "TaskBtn";
				keys[BTN_TRIGGER] = "Trigger";		keys[BTN_THUMB] = "ThumbBtn";
				keys[BTN_THUMB2] = "ThumbBtn2";		keys[BTN_TOP] = "TopBtn";
				keys[BTN_TOP2] = "TopBtn2";			keys[BTN_PINKIE] = "PinkieBtn";
				keys[BTN_BASE] = "BaseBtn";			keys[BTN_BASE2] = "BaseBtn2";
				keys[BTN_BASE3] = "BaseBtn3";		keys[BTN_BASE4] = "BaseBtn4";
				keys[BTN_BASE5] = "BaseBtn5";		keys[BTN_BASE6] = "BaseBtn6";
				keys[BTN_DEAD] = "BtnDead";			keys[BTN_A] = "BtnA";
				keys[BTN_B] = "BtnB";			keys[BTN_C] = "BtnC";
				keys[BTN_X] = "BtnX";			keys[BTN_Y] = "BtnY";
				keys[BTN_Z] = "BtnZ";			keys[BTN_TL] = "BtnTL";
				keys[BTN_TR] = "BtnTR";			keys[BTN_TL2] = "BtnTL2";
				keys[BTN_TR2] = "BtnTR2";			keys[BTN_SELECT] = "BtnSelect";
				keys[BTN_START] = "BtnStart";		keys[BTN_MODE] = "BtnMode";
				keys[BTN_THUMBL] = "BtnThumbL";		keys[BTN_THUMBR] = "BtnThumbR";
				keys[BTN_TOOL_PEN] = "ToolPen";		keys[BTN_TOOL_RUBBER] = "ToolRubber";
				keys[BTN_TOOL_BRUSH] = "ToolBrush";		keys[BTN_TOOL_PENCIL] = "ToolPencil";
				keys[BTN_TOOL_AIRBRUSH] = "ToolAirbrush";	keys[BTN_TOOL_FINGER] = "ToolFinger";
				keys[BTN_TOOL_MOUSE] = "ToolMouse";		keys[BTN_TOOL_LENS] = "ToolLens";
				keys[BTN_TOUCH] = "Touch";			keys[BTN_STYLUS] = "Stylus";
				keys[BTN_STYLUS2] = "Stylus2";		keys[BTN_TOOL_DOUBLETAP] = "Tool Doubletap";
				keys[BTN_TOOL_TRIPLETAP] = "Tool Tripletap"; keys[BTN_GEAR_DOWN] = "WheelBtn";
				keys[BTN_GEAR_UP] = "Gear up";		keys[KEY_OK] = "Ok";
				keys[KEY_SELECT] = "Select";		keys[KEY_GOTO] = "Goto";
				keys[KEY_CLEAR] = "Clear";			keys[KEY_POWER2] = "Power2";
				keys[KEY_OPTION] = "Option";		keys[KEY_INFO] = "Info";
				keys[KEY_TIME] = "Time";			keys[KEY_VENDOR] = "Vendor";
				keys[KEY_ARCHIVE] = "Archive";		keys[KEY_PROGRAM] = "Program";
				keys[KEY_CHANNEL] = "Channel";		keys[KEY_FAVORITES] = "Favorites";
				keys[KEY_EPG] = "EPG";			keys[KEY_PVR] = "PVR";
				keys[KEY_MHP] = "MHP";			keys[KEY_LANGUAGE] = "Language";
				keys[KEY_TITLE] = "Title";			keys[KEY_SUBTITLE] = "Subtitle";
				keys[KEY_ANGLE] = "Angle";			keys[KEY_ZOOM] = "Zoom";
				keys[KEY_MODE] = "Mode";			keys[KEY_KEYBOARD] = "Keyboard";
				keys[KEY_SCREEN] = "Screen";		keys[KEY_PC] = "PC";
				keys[KEY_TV] = "TV";			keys[KEY_TV2] = "TV2";
				keys[KEY_VCR] = "VCR";			keys[KEY_VCR2] = "VCR2";
				keys[KEY_SAT] = "Sat";			keys[KEY_SAT2] = "Sat2";
				keys[KEY_CD] = "CD";			keys[KEY_TAPE] = "Tape";
				keys[KEY_RADIO] = "Radio";			keys[KEY_TUNER] = "Tuner";
				keys[KEY_PLAYER] = "Player";		keys[KEY_TEXT] = "Text";
				keys[KEY_DVD] = "DVD";			keys[KEY_AUX] = "Aux";
				keys[KEY_MP3] = "MP3";			keys[KEY_AUDIO] = "Audio";
				keys[KEY_VIDEO] = "Video";			keys[KEY_DIRECTORY] = "Directory";
				keys[KEY_LIST] = "List";			keys[KEY_MEMO] = "Memo";
				keys[KEY_CALENDAR] = "Calendar";		keys[KEY_RED] = "Red";
				keys[KEY_GREEN] = "Green";			keys[KEY_YELLOW] = "Yellow";
				keys[KEY_BLUE] = "Blue";			keys[KEY_CHANNELUP] = "ChannelUp";
				keys[KEY_CHANNELDOWN] = "ChannelDown";	keys[KEY_FIRST] = "First";
				keys[KEY_LAST] = "Last";			keys[KEY_AB] = "AB";
				keys[KEY_NEXT] = "Next";			keys[KEY_RESTART] = "Restart";
				keys[KEY_SLOW] = "Slow";			keys[KEY_SHUFFLE] = "Shuffle";
				keys[KEY_BREAK] = "Break";			keys[KEY_PREVIOUS] = "Previous";
				keys[KEY_DIGITS] = "Digits";		keys[KEY_TEEN] = "TEEN";
				keys[KEY_TWEN] = "TWEN";			keys[KEY_DEL_EOL] = "Delete EOL";
				keys[KEY_DEL_EOS] = "Delete EOS";		keys[KEY_INS_LINE] = "Insert line";
				keys[KEY_DEL_LINE] = "Delete line";


					relatives[REL_X] = "X";			relatives[REL_Y] = "Y";
					relatives[REL_Z] = "Z";			relatives[REL_HWHEEL] = "HWheel";
					relatives[REL_DIAL] = "Dial";		relatives[REL_WHEEL] = "Wheel";
					relatives[REL_MISC] = "Misc";


						absolutes[ABS_X] = "X";			absolutes[ABS_Y] = "Y";
						absolutes[ABS_Z] = "Z";			absolutes[ABS_RX] = "Rx";
						absolutes[ABS_RY] = "Ry";		absolutes[ABS_RZ] = "Rz";
						absolutes[ABS_THROTTLE] = "Throttle";	absolutes[ABS_RUDDER] = "Rudder";
						absolutes[ABS_WHEEL] = "Wheel";		absolutes[ABS_GAS] = "Gas";
						absolutes[ABS_BRAKE] = "Brake";		absolutes[ABS_HAT0X] = "Hat0X";
						absolutes[ABS_HAT0Y] = "Hat0Y";		absolutes[ABS_HAT1X] = "Hat1X";
						absolutes[ABS_HAT1Y] = "Hat1Y";		absolutes[ABS_HAT2X] = "Hat2X";
						absolutes[ABS_HAT2Y] = "Hat2Y";		absolutes[ABS_HAT3X] = "Hat3X";
						absolutes[ABS_HAT3Y] = "Hat 3Y";		absolutes[ABS_PRESSURE] = "Pressure";
						absolutes[ABS_DISTANCE] = "Distance";	absolutes[ABS_TILT_X] = "XTilt";
						absolutes[ABS_TILT_Y] = "YTilt";		absolutes[ABS_TOOL_WIDTH] = "Tool Width";
						absolutes[ABS_VOLUME] = "Volume";	absolutes[ABS_MISC] = "Misc";


							misc[MSC_SERIAL] = "Serial";	misc[MSC_PULSELED] = "Pulseled";
							misc[MSC_GESTURE] = "Gesture";	misc[MSC_RAW] = "RawData";
							misc[MSC_SCAN] = "ScanCode";


								leds[LED_NUML] = "NumLock";		leds[LED_CAPSL] = "CapsLock";
								leds[LED_SCROLLL] = "ScrollLock";	leds[LED_COMPOSE] = "Compose";
								leds[LED_KANA] = "Kana";		leds[LED_SLEEP] = "Sleep";
								leds[LED_SUSPEND] = "Suspend";	leds[LED_MUTE] = "Mute";
								leds[LED_MISC] = "Misc";



									repeats[REP_DELAY] = "Delay";		repeats[REP_PERIOD] = "Period";


										sounds[SND_CLICK] = "Click";		sounds[SND_BELL] = "Bell";
										sounds[SND_TONE] = "Tone";



											names[EV_SYN] = events;			names[EV_KEY] = keys;
											names[EV_REL] = relatives;			names[EV_ABS] = absolutes;
											names[EV_MSC] = misc;			names[EV_LED] = leds;
											names[EV_SND] = sounds;			names[EV_REP] = repeats;



		int fd, rd, i, j, k;
		struct input_event ev[64];
		int version;
		unsigned short id[4];
		unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
		char name[256] = "Unknown";
		int abs[5];



		//Now we need to check to se what port the controller is connected to, make sure to check
		//to see if the port is one of these if its not then go ahead and add another.
		if ((fd = open("/dev/input/event12", O_RDONLY)) < 0) {
		if ((fd = open("/dev/input/event13", O_RDONLY)) < 0) {
		if ((fd = open("/dev/input/event11", O_RDONLY)) < 0) {
		if ((fd = open("/dev/input/event16", O_RDONLY)) < 0) {
			if((fd = open("/dev/input/event15", O_RDONLY))<0)
			{
				if((fd = open("/dev/input/event17", O_RDONLY))<0)
							{
					perror("evtest");
										return 0;
							}
			}
		}
		}
		}}
		if (ioctl(fd, EVIOCGVERSION, &version)) {
			perror("evtest: can't get version");
			return 0;
		}

		//PRINTING CONTROLLER INFO
		printf("Input driver version is %d.%d.%d\n",
			version >> 16, (version >> 8) & 0xff, version & 0xff);

		ioctl(fd, EVIOCGID, id);
		printf("Input device ID: bus 0x%x vendor 0x%x product 0x%x version 0x%x\n",
			id[ID_BUS], id[ID_VENDOR], id[ID_PRODUCT], id[ID_VERSION]);

		ioctl(fd, EVIOCGNAME(sizeof(name)), name);
		printf("Input device name: \"%s\"\n", name);

		memset(bit, 0, sizeof(bit));
		ioctl(fd, EVIOCGBIT(0, EV_MAX), bit[0]);
		printf("Supported events:\n");

		//PRINTING THE EVENTS AVALIABLE TO THE USER
		for (i = 0; i < EV_MAX; i++)
			if (test_bit(i, bit[0])) {
				printf("  Event type %d (%s)\n", i, events[i] ? events[i] : "?");
				if (!i) continue;
				ioctl(fd, EVIOCGBIT(i, KEY_MAX), bit[i]);
				for (j = 0; j < KEY_MAX; j++)
					if (test_bit(j, bit[i])) {
						printf("    Event code %d (%s)\n", j, names[i] ? (names[i][j] ? names[i][j] : "?") : "?");
						if (i == EV_ABS) {
							ioctl(fd, EVIOCGABS(j), abs);
							for (k = 0; k < 5; k++)
								if ((k < 3) || abs[k])
									printf("      %s %6d\n", absval[k], abs[k]);
						}
					}
			}


		printf("Testing ... (interrupt to exit)\n");






	struct  sockaddr_in server;
    int serverSock=0;
    string test ="";
	    /* make this thread cancellable using pthread_cancel() */
	    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	    /* create socket */
	    if ((sockSerial = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
	        quit("socket() failed.", 1);
	    }

	    /* setup server parameters */
	    memset(&server, 0, sizeof(server));
	    server.sin_family = AF_INET;
	    server.sin_addr.s_addr = inet_addr(server_ip);
	    server.sin_port = htons(8887);

	    /* conct to server */
	    while (connect(sockSerial, (struct sockaddr*)&server, sizeof(server)) < 0) {
	        quit("connect() failed.", 1);
	    }

	    //This while loop checks for input from the controller and proccess' it to be sent to the client
	    while(1)
	    {   usleep(1000);
	    	rd = read(fd, ev, sizeof(struct input_event) * 64);

	    			if (rd < (int) sizeof(struct input_event)) {
	    				printf("yyy\n");
	    				perror("\nevtest: error reading");
	    				return 0;
	    			}



	    			//Notice ,test=names[ev[i].type][ev[i].code])=="Y", the Y is the code for that specific controller action
	    			//so i check for the apporopriate codes and then send the data that the server needs to interpret
	    			for (i = 0; i < rd / sizeof(struct input_event); i++)
	    				if((test=names[ev[i].type][ev[i].code])=="Y"){
	    					//test=names[ev[i].type][ev[i].code];
	    					char temp = test[0];
	    					int  val = ev[i].value;
	    					//we only want to send values 1 to 10 so we divide by the max value the joystick can output
	    					double tempval = 32767.0;
	    					tempval = (double)val/tempval;
	    					tempval = tempval*10;
	    					val = (int)tempval;
	    					send(sockSerial,&temp,sizeof(char),0);
	    					send(sockSerial,&val,sizeof(int),0);
	    	                //usleep(100000);
	                    }
	    				else if((test=names[ev[i].type][ev[i].code])=="X"){
    					  //test=names[ev[i].type][ev[i].code];
    					  char temp = test[0];
    					  int  val = ev[i].value;
	    				  double tempval = 32767.0;
	    				  tempval = (double)val/tempval;
	    				  tempval = tempval*10;
	    				  val = (int)tempval;
    					  send(sockSerial,&temp,sizeof(char),0);
	    				  send(sockSerial,&val,sizeof(int),0);
    	                  //usleep(100000);
                        }
	    				else if((test=names[ev[i].type][ev[i].code])=="Rx"){
    					  //test=names[ev[i].type][ev[i].code];
    					  char temp = 'x';
    					  int  val = ev[i].value;
	    				  double tempval = 32767.0;
	    				  tempval = (double)val/tempval;
	    				  tempval = tempval*10;
	    				  val = (int)tempval;
    					  send(sockSerial,&temp,sizeof(char),0);
	    				  send(sockSerial,&val,sizeof(int),0);
    	                  //usleep(100000);
                        }
	    				else if((test=names[ev[i].type][ev[i].code])=="Ry"){
    					  //test=names[ev[i].type][ev[i].code];
    					  char temp = 'y';
    					  int  val = ev[i].value;
	    				  double tempval = 32767.0;
	    				  tempval = (double)val/tempval;
	    				  tempval = tempval*10;
	    				  val = (int)tempval;
    					  send(sockSerial,&temp,sizeof(char),0);
	    				  send(sockSerial,&val,sizeof(int),0);
    	                  //usleep(100000);
                        }
	    				else if((test=names[ev[i].type][ev[i].code])=="BtnA"){
    					  //test=names[ev[i].type][ev[i].code];
    					  char temp = 'A';
    					  send(sockSerial,&temp,sizeof(char),0);
    	                  usleep(100000);
                        }
	    				else if((test=names[ev[i].type][ev[i].code])=="BtnX"){
    					  //test=names[ev[i].type][ev[i].code];
    					  char temp = 'T';
    					  send(sockSerial,&temp,sizeof(char),0);
    	                  usleep(100000);
                        }
	    				else if((test=names[ev[i].type][ev[i].code])=="BtnY"){
    					  //test=names[ev[i].type][ev[i].code];
	    				  //exit(1);
	    				  key = 'q';
    					  char temp = 'Z';
    					  //send(sockSerial,&temp,sizeof(char),0);
    	                  usleep(100000);
                        }
	    				else if((test=names[ev[i].type][ev[i].code])=="BtnB"){
    					  //test=names[ev[i].type][ev[i].code];
    					  char temp = 'B';
    					  send(sockSerial,&temp,sizeof(char),0);
    	                  usleep(100000);
                        }
	    	        pthread_testcancel();

	    }


return 0;
}


/*
 * This method recieves the arduino data and displays it to the user
 */
void* getArduino (void * arg)
{
	    struct  sockaddr_in server;
	    char val = 'x';
	    string test ="";
		    /* make this thread cancellable using pthread_cancel() */
		    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

		    /* create socket */
		    if ((sockArd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		        quit("socket() failed.", 1);
		    }

		    /* setup server parameters */
		    memset(&server, 0, sizeof(server));
		    server.sin_family = AF_INET;
		    server.sin_addr.s_addr = inet_addr(server_ip);
		    server.sin_port = htons(8886);

		    /* conct to server */
		    while (connect(sockArd, (struct sockaddr*)&server, sizeof(server)) < 0) {
		        quit("connect() failed.", 1);
		    }

		    while(1)
		    {   usleep(700);

		      //receive the data and then send it to cout.
		      recv(sockArd,&val,sizeof(char),0);
		      std::cout<< val;
		    	        pthread_testcancel();

		    }


	return 0;


}
/*printf("Event: time %ld.%06ld, type %d (%s), code %d (%s), value %d\n",
					ev[i].time.tv_sec, ev[i].time.tv_usec, ev[i].type,
					events[ev[i].type] ? events[ev[i].type] : "?",
					ev[i].code,
					names[ev[i].type] ? (names[ev[i].type][ev[i].code] ? names[ev[i].type][ev[i].code] : "?") : "?",
					ev[i].value);*/
