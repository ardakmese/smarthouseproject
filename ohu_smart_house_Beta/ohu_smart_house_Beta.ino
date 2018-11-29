// Bu proje aşağıdaki linkteki webserver basic 0.1 versiyonu temel alınarak hazırlanmıştır
// http://startingelectronics.org/software/arduino/web-server/basic-01/
//
// 
// Proje ÖMER HALİSDEMİR ÜNİVERSİTESİ EEM bölümü öğrencileri bitirme tezi olarak hazırlanmıştır
// Projeyi ve siteyi geliştiren: Arda AKMEŞE
// Date: 9/2/2017
//
#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <Servo.h>
#define REQ_BUF_SZ   60
// maximum length of file name including path
#define FILE_NAME_LEN  20
// HTTP request type
#define HTTP_invalid   0
#define HTTP_GET       1
#define HTTP_POST      2

// file types
#define FT_HTML       0
#define FT_ICON       1
#define FT_CSS        2
#define FT_JAVASCRIPT 3
#define FT_JPG        4
#define FT_PNG        5
#define FT_GIF        6
#define FT_TEXT       7
#define FT_INVALID    8
boolean dns_state[3] = {0};
boolean garaj_state[2] = {0}; 
// pin used for Ethernet chip SPI chip select
#define PIN_ETH_SPI   10
File webFile;                     // the web page file on the SD card
char HTTP_req[REQ_BUF_SZ] = {0};  // buffered HTTP request stored as null terminated string
char req_index = 0;               // index into HTTP_req buffer
unsigned char LED_state[3] = {0};
long log_time_ms = 5000; // how often to log data in milliseconds
long prev_log_time = 0;   // previous time log occurred
Servo garaj;
// the media access control (ethernet hardware) address for the shield:
const byte mac[] PROGMEM = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
//the IP address for the shield:
const byte ip[] = { 192, 168, 1, 40 };  // does not work if this is put into Flash
// the router's gateway address:
const byte gateway[] PROGMEM = { 192, 168, 1, 1 };
// the subnet:
const byte subnet[] PROGMEM = { 255, 255, 255, 0 };
EthernetServer server(80);

void setup() {
  int i;
pinMode(7, OUTPUT); // çamaşır makinesi output olarak belirlendi
pinMode(8, OUTPUT); // bulaşık makinesi
pinMode(6, OUTPUT); // klima
     for (i = 26; i <= 34; i++) {
        pinMode(i, OUTPUT);    // set pins as outputs
        digitalWrite(i, LOW);  // switch the output pins off
    }
  // deselect Ethernet chip on SPI bus
  pinMode(PIN_ETH_SPI, OUTPUT);
  digitalWrite(PIN_ETH_SPI, HIGH);
 garaj.attach(5); 
  Serial.begin(115200);       // for debugging
  
  if (!SD.begin(4)) {
    return;  // SD card initialization failed
  }

  Ethernet.begin((uint8_t*)mac, ip, gateway, subnet);
  server.begin();  // start listening for clients
}

void loop() {
  // if an incoming client connects, there will be bytes available to read:
  EthernetClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (ServiceClient(&client)) {
        // received request from client and finished responding
        break;
      }
    }  // while (client.connected())
    delay(1);
    client.stop();
  }  // if (client)
  // log the data
  checkleds();
}// void loop()

void checkleds()
{
    EthernetClient client = server.available();  // try to get client

    if (client) {  // got client?
        boolean currentLineIsBlank = true;
        while (client.connected()) {
            if (client.available()) {   // client data available to read
                char c = client.read(); // read 1 byte (character) from client
                // limit the size of the stored received HTTP request
                // buffer first part of HTTP request in HTTP_req array (string)
                // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
                if (req_index < (REQ_BUF_SZ - 1)) {
                    HTTP_req[req_index] = c;          // save HTTP request character
                    req_index++;
                }
                // last line of client request is blank and ends with \n
                // respond to client only after last line received
                if (c == '\n' && currentLineIsBlank) {
                    // send a standard http response header
                    client.println("HTTP/1.1 200 OK");
                    // remainder of header follows below, depending on if
                    // web page or XML page is requested
                    // Ajax request - send XML file
                    if (StrContains(HTTP_req, "ajax_inputs")) {
                        // send rest of HTTP header
                        client.println("Content-Type: text/xml");
                        client.println("Connection: keep-alive");
                        client.println();
                        SetLEDs();
                        // send XML file containing input states
                        XML_response(client);
                
                     
                    }
                    else {  // web page request
                        // send rest of HTTP header
                        client.println("Content-Type: text/html");
                        client.println("Connection: keep-alive");
                        client.println();
                        // send web page
                        webFile = SD.open("index.htm");        // open web page file
                        if (webFile) {
                            while(webFile.available()) {
                                client.write(webFile.read()); // send web page to client
                            }
                            webFile.close();
                        }
                    }
                    // display received HTTP request on serial port
                    //Serial.print(HTTP_req);
                    // reset buffer index and all buffer elements to 0
                    req_index = 0;
                    StrClear(HTTP_req, REQ_BUF_SZ);
                    break;
                }
                // every line of text received from the client ends with \r\n
                if (c == '\n') {
                    // last character on line of received text
                    // starting new line with next character read
                    currentLineIsBlank = true;
                } 
                else if (c != '\r') {
                    // a text character was received from client
                    currentLineIsBlank = false;
                }
            } // end if (client.available())
        } // end while (client.connected())
        delay(1);      // give the web browser time to receive the data
        client.stop(); // close the connection
    } // end if (client)
}


void SetLEDs(void)
{
  int val = 0;

    
    if (StrContains(HTTP_req, "garj=1")) {
        garaj_state[0] = 1;  // save LED state
        
         garaj.write(180);
    }
    else if (StrContains(HTTP_req, "garj=0")) {
        garaj_state[0] = 0;  // save LED state
        val = 0;
         garaj.write(val);
    }
  if (StrContains(HTTP_req, "dn1=1")) {
        dns_state[0] = 1;  // save LED state
        
    digitalWrite(8, HIGH);
    }
    else if (StrContains(HTTP_req, "dn1=0")) {
    
      dns_state[0] = 0;
    digitalWrite(8, LOW);
    }
     if (StrContains(HTTP_req, "dn2=1")) {
        dns_state[1] = 1;  // save LED state
        
    digitalWrite(7, HIGH);
    }
    else if (StrContains(HTTP_req, "dn2=0")) {
    
      dns_state[1] = 0;
    digitalWrite(7, LOW);
    }
 if (StrContains(HTTP_req, "dn3=1")) {
        dns_state[2] = 1;  // save LED state
        
    digitalWrite(6, HIGH);
    }
    else if (StrContains(HTTP_req, "dn3=0")) {
      dns_state[2] = 0;  // save LED state
        
    digitalWrite(6, LOW);
    }
   
        char str_on[12] = {0};
    char str_off[12] = {0};
    unsigned char i;
    unsigned int  j;
    int LED_num = 1;
    for (i = 0; i < 3; i++) {
        for (j = 1; j <= 0x80; j <<= 1) {
            sprintf(str_on,  "LED%d=%d", LED_num, 1);
            sprintf(str_off, "LED%d=%d", LED_num, 0);
            if (StrContains(HTTP_req, str_on)) {
                LED_state[i] |= (unsigned char)j;  // save LED state
                digitalWrite(LED_num + 25, HIGH);
                Serial.println("ON");
            }
            else if (StrContains(HTTP_req, str_off)) {
                LED_state[i] &= (unsigned char)(~j);  // save LED state
                digitalWrite(LED_num + 25, LOW);
            }
            LED_num++;
        }
    }
}

// send the XML file with analog values, switch status
//  and LED status
void XML_response(EthernetClient cl)
{
    unsigned char i;
    unsigned int  j;
     int analog_val;
    

    // read analog pin A2
  
 
    cl.print("<?xml version = \"1.0\" ?>");
    cl.print("<inputs>");
      analog_val = analogRead(2) - 30.5;
    cl.print("<analog>");
    cl.print(analog_val);
    cl.print("</analog>");


    
    cl.print("<garaj>");
    if (garaj_state[0]) {
        cl.print("checked");
    }
    else {
        cl.print("unchecked");
    }
    cl.println("</garaj>");
    for (i = 0; i < 3; i++) {
        for (j = 1; j <= 0x80; j <<= 1) {
            cl.print("<LED>");
            if ((unsigned char)LED_state[i] & j) {
                cl.print("checked");
                //Serial.println("ON");
            }
            else {
                cl.print("unchecked");
            }
            cl.println("</LED>");
        }
    }
    cl.print("</inputs>");
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

// searches for the string sfind in the string str
// returns 1 if string found
// returns 0 if string not found


char StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);
    
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }

    return 0;
}


bool ServiceClient(EthernetClient *client)
{
  static boolean currentLineIsBlank = true;
  char cl_char;
  File webFile;
  // file name from request including path + 1 of null terminator
  char file_name[FILE_NAME_LEN + 1] = {0};  // requested file name
  char http_req_type = 0;
  char req_file_type = FT_INVALID;
  const char *file_types[] = {"text/html", "image/x-icon", "text/css", "application/javascript", "image/jpeg", "image/png", "image/gif", "text/plain"};
  
  static char req_line_1[40] = {0};  // stores the first line of the HTTP request
  static unsigned char req_line_index = 0;
  static bool got_line_1 = false;

  if (client->available()) {   // client data available to read
    cl_char = client->read();
    
    if ((req_line_index < 39) && (got_line_1 == false)) {
      if ((cl_char != '\r') && (cl_char != '\n')) {
        req_line_1[req_line_index] = cl_char;
        req_line_index++;
      }
      else {
        got_line_1 = true;
        req_line_1[39] = 0;
      }
    }
    
    if ((cl_char == '\n') && currentLineIsBlank) {
      // get HTTP request type, file name and file extension type index
      http_req_type = GetRequestedHttpResource(req_line_1, file_name, &req_file_type);
      if (http_req_type == HTTP_GET) {         // HTTP GET request
        if (req_file_type < FT_INVALID) {      // valid file type
          webFile = SD.open(file_name);        // open requested file
          if (webFile) {
            // send a standard http response header
            client->println(F("HTTP/1.1 200 OK"));
            client->print(F("Content-Type: "));
            client->println(file_types[req_file_type]);
            client->println(F("Connection: close"));
            client->println();
            // send web page
            while(webFile.available()) {
              int num_bytes_read;
              char byte_buffer[64];
              // get bytes from requested file
              num_bytes_read = webFile.read(byte_buffer, 64);
              // send the file bytes to the client
              client->write(byte_buffer, num_bytes_read);
            }
            webFile.close();
          }
          else {
            // failed to open file
          }
        }
        else {
          // invalid file type
        }
      }
      else if (http_req_type == HTTP_POST) {
        // a POST HTTP request was received
      }
      else {
        // unsupported HTTP request received
      }
      req_line_1[0] = 0;
      req_line_index = 0;
      got_line_1 = false;
      // finished sending response and web page
      return 1;
    }
    if (cl_char == '\n') {
      currentLineIsBlank = true;
    }
    else if (cl_char != '\r') {
      currentLineIsBlank = false;
    }
  }  // if (client.available())
  return 0;
}

// extract file name from first line of HTTP request
char GetRequestedHttpResource(char *req_line, char *file_name, char *file_type)
{
  char request_type = HTTP_invalid;  // 1 = GET, 2 = POST. 0 = invalid
  char *str_token;
  
  *file_type = FT_INVALID;
  
  str_token =  strtok(req_line, " ");    // get the request type
  if (strcmp(str_token, "GET") == 0) {
    request_type = HTTP_GET;
    str_token =  strtok(NULL, " ");      // get the file name
    if (strcmp(str_token, "/") == 0) {
      strcpy(file_name, "index.htm");
      *file_type = FT_HTML;
    }
    else if (strlen(str_token) <= FILE_NAME_LEN) {
      // file name is within allowed length
      strcpy(file_name, str_token);
      // get the file extension
      str_token = strtok(str_token, ".");
      str_token = strtok(NULL, ".");
      
      if      (strcmp(str_token, "htm") == 0) {*file_type = 0;}
      else if (strcmp(str_token, "png") == 0) {*file_type = 1;}

      else {*file_type = 8;}
    }
    else {
      // file name too long
    }
  }
  else if (strcmp(str_token, "POST") == 0) {
    request_type = HTTP_POST;
  }

  return request_type;
}


