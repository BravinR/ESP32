#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"

//These are the pin definitions for our setup
#define THERM_PIN 34 //Pin the thermistor is attached to
#define BUTTON_PIN 26 //Pin the button is attached to 
#define LED_PIN 27 //Pin the LED is attached to

//These values are provided from the 10K thermistor we are using for this setup
#define NOMINAL_RES 10000 //Nominal resistance of thermistor (ohms)
#define NOMINAL_TEMP 25 //Nominal temperature of thermistor (deg C)
#define BETA_COEFF 3950 //Beta coefficient of thermistor 
#define O_RES 10000 //Value of other resistor used as divider for thermistor (ohms)

//This is the name of the ESP32 WiFi "router" that shows when you look for a WiFi network on 
//your device. PLEASE put your name in front of Webserver so we dont get them mixed up
//const char ap_name[] = "Bravin webserver";

//Adding variables with SSID/Password Combination
const char* ssid = ""
const char* password = ""

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

DNSServer dnsServer;
//Declaration of the server used to handle web requests. The port 80 is passed. 
//Most websites use port 80 for HTML webpages that you view on your phone/computer etc.
AsyncWebServer server(80);

/**
 * This is the declaration of the constant message body that we will send to the client (you)
 * when you connect to the ESP32. This is written in HTML and most of it is defined below, however
 * the end of this HTML will be added later (with the temperature and button status). 
 * the PROGMEM = R"rawliteral(DATA_HERE)rawliteral" stores the string in program memory and not RAM
 * <!DOCTYPE HTML><html> this is the header telling your device it is viewing an HTML page
 * <head> this is where info about the page is sent
 * <body> the message body and main page content
 * <h3> makes text big
 * <br> break (HTML version of newline)
 * <a href="URL"> a link with a url but different visible text
 */
const char index_html_header[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <title>IEEE & IoT Remote Sensing Workshop</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <style>
      body {
        background-color: #190618;
        margin:0 auto;
        color:white;
      }

      h1 {
        color: white;
        margin-left: 40px;
      }
      .textCenter {
        display: flex;
        justify-content: center;
        background-color:white;
        border-radius:5px;
        margin:auto;
        margin-top: 10px;
        width:50%;
        color:blue;
        padding:10px;
      }
      .titleCenter {
        margin-top: 10px;
        display: flex;
        justify-content: center;
      }
      .center {
        border-radius:5px;
        margin: auto;
        width: 50%;
        border: 3px solid #cd1fe0;
        padding: 10px;
      }
      .button {
        background-color: #5f2bd4;
        border: none;
        border-radius: 30px;
        color: white;
        padding: 15px 32px;
        text-align: center;
        text-decoration: none;
        display: inline-block;
        font-size: 16px;
        margin: 4px 2px;
        cursor: pointer;
      }
    </style>
  </head>
  <body>
    <h1 class="titleCenter">IEEE & IoT Remote Sensing Workshop</h1>
    <br /><br />
    <div class="center">
      <a class="button" href="/?led=on">LED On</a><br />
      <a class="button" href="/?led=off">LED Off</a><br />
      <a class="button" href="/">Update Inputs</a><br />
    </div>
)rawliteral";

//We will define this function later but we must declare it now
inline float getTemp();

//We will define this function later but we must declare it now
inline bool isPressed();

/**
 * This function returns the rest of the HTML which is added at the end of the constant 
 * portion declared earlier. It addes the temperature of the thermistor, a newline,
 * and wether or not the button is pressed. It then closes the body and html blocks.
 * @return the rest of the HTML body 
 */
String getHTML(){
  float temp = getTemp();//Read the thermistor and get the temperature

  //See if the button is pressed. This arg ? if_true : if_false is called a conditional operator
  //and it is a simplified version of an if statement. If the button is pressed "Pressed" is returned
  //and otherwise "Not Pressed" is returned. Note it must be cast as a String() object in this case,
  //however most conditional operators do not need this.
  String pressed = String(isPressed() ? "Pressed" : "Not Pressed");

  String html = index_html_header; //First copy the whole html block declared earlier to RAM here
  html+= "<div class='textCenter'>";
  html += "Temperature : "+String(temp)+" DegF<br>";//Add the temperature to the html body
  html += "Button is "+pressed;//Add the button status
  html += "</div>";
  html += "</body></html>";//Close the body and html block

  return html;//Return the complete HTML block to be sent to the client
}

/**
 * This method is used when you (the client) connect to the ESP32 and make a request.
 * Arguments from the client passed after the ? are parsed and accessible here.
 * Response data is returned to the client at the end of the method.
 * This method is used for all requests due to the simplicity of this setup. Real web apps
 * use multiple request handlers.
 * @param request pointer to web request object with request info
 */
void globalHandleRequest(AsyncWebServerRequest *request){
  
  int params = request->params();//Number of params sent by client
  for(int i=0;i<params;i++){//View all the parameters sent by the client
    AsyncWebParameter* p = request->getParam(i);//Request object pointer
    String param = p->name();//Name of the argument
    String value = p->value();//Value of the argument
    Serial.print(param);//Print the param name 
    Serial.print(" = ");
    Serial.println(value);//Print the param value
    if(param == "led"){//The param is "led" so the client is sending an instruction
      bool ledValue = value == "on";//Convert the value string to a boolean
      digitalWrite(LED_PIN,ledValue);//Set the LED to the value passed, either on or off
    }
  }

  //Reply to the request from the client
  //200 is the status code and 200 stands for HTTP:OK
  //"text/html" tells the client that it is getting an html page as ascii text
  //getHTML() gets the html string from the method defined before and sends it to the client.
  //Note that this means that when it is called the temperature and button state are read then
  //and are up to date each time. 
  request->send(200,"text/html",getHTML()); 
}

//This method is called once during the start of the program
void setup(){
  Serial.begin(115200);//Start Serial. BTW Most devices use this baud rate and not 9600
  Serial.println();//Do a newline because there is usually noise when connected to ESP32

  pinMode(LED_PIN,OUTPUT);//Set the LED as an output
  pinMode(BUTTON_PIN,INPUT_PULLUP);//Set the button as an pullup input

  Serial.println("Setting up AP Mode");
  WiFi.mode(WIFI_AP);//AP stands for Access Point and it basically turns the ESP32 into a "router"
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.printf("WiFi Failed!\n");
      return;
  }

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Setting up Async WebServer");
  /**
   * This starts the web server hosted on the ESP32. This is what the ESP32 does when the client makes
   * a request at the / endpoint which is the default index, and the one we will be using.
   * The HTTP_GET stands for the type of request which is made. We will only be using GET requests
   * because we are sending generic, insensitve data. Other types are POST, PUT, DELETE, etc.
   * The fancy [] thing at the end is called a lambda function. It is simply a void function which
   * is declared as an argument, and is done for when a specific action is to be executed. Basically
   * the stuff inside it will get executed when the request is made. 
   */
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      globalHandleRequest(request);//Use the global handler
  });

  server.onNotFound(notFound);
  server.begin();//Start the server and begin listening for requests
}

//This method runs after the setup method and loops forever
void loop(){
  dnsServer.processNextRequest();//If a new client connects direct them to the right IP address
}

/**
 * Called when the temperature is requested. It will read the thermistor and return the temp.
 * The Steinhart-Hart equation is used for the calculation of the temp.
 * @return the temperature in fahrenheit 
 */
float getTemp(){
  //This is the output voltage of the thermistor voltage divider-- but out of 1V instead of 3.3V
  float vo = 0;
  for(int i=0;i<10;i++){//Take 10 mesurements
    vo += analogRead(THERM_PIN);
  }
  vo /= 40950.0;//Take the average of the 10 measurements out of 1.0
  vo *= 1.2;//Correct for ADC error (not 100% sure why this is happening)
  //Calculate value of thermistor resistance using output voltage & other resistor value
  float thermResistance = vo * O_RES / (1 - vo);
  float tempK = log(thermResistance / NOMINAL_RES) / BETA_COEFF;//Start on kelvin calculation
  tempK += 1.0 / (NOMINAL_TEMP + 273.15);//Inverse of temp in kelvin
  tempK = 1.0 / tempK;//Actual kelvin temp
  float tempC = tempK - 273.15;//Convert to celsius
  float tempF = tempC * 1.8 + 32;//Convert to fahrenheit
  return tempF;//Return the temp
}

/**
 * Called to see if the button is being pressed
 * @return true if button is pressed and false if it is not
 */
bool isPressed(){
  //Read the button pin, note its the opposite of the value because it is pullup
  bool pressed = !digitalRead(BUTTON_PIN);
  return pressed;
}