// ======================================================
// Webserver program that sends button information
// to ESP8266 and has the MP3 player playing the
// next song or start all over
// ======================================================

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

// Your routers credentials
const char* ssid = "Control booth";
const char* password = "MontyLives";

// ==========================================
// initial variables
// ========================================== 

String textosend_string;
String button1ON;
String button2ON;

// =========================================
// Here is the HTML page
// =========================================
String getPage()
  {
  String page = "<!DOCTYPE HTML>";
  page += "<html>";
  page += "<head>";
  page += "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0 maximum-scale = 2.5, user-scalable=1\">";
  page += "<title>Luc's button demo</title>";
  page += "<body style='background-color:powderblue; onload='myFunction()'>";
  
  page += "</head>";
  page += "<body>";

  page += "<style>";
  page += ".buttonstyle";
  page += "{";
  page += "background-color: BurlyWood;";
  page += "border-radius: 12px;";
  page += "color: black;" ;
  page += "font-size: 1.2em;";
  page += "border: 2px solid; background-color: BurlyWood;";
  page += "}";
  page += "</style>";
    
  page += "<h1 style='color:red'>Luc's MP3 Control</h1>";
  page += "<br>";

  page += "<FORM action=\"/\" method=\"post\">";
  page += "<button type=\"submit\" name=\"button1on\" id=\"button1on\" value=\"but1ON\" class = \"buttonstyle\">Next</button>";
  page += "</form>";
  page += "<br>";
  page += "<br>";
  
  page += "<FORM action=\"/\" method=\"post\">";
  page += "<button type=\"submit\" name=\"button2on\" id=\"button2on\" value=\"but2ON\" class = \"buttonstyle\">RESTART</button>";
  page += "</form>";
  page += "<br>";
  page += "</body>";
  page += "</html>";
  return page;
  }


// ==================================================
// Handle for page not found
// ==================================================
void handleNotFound()
{
  server.send(200, "text/html", getPage());
}


// ==================================================
// Handle submit form
// ==================================================
void handleSubmit()
{
  //Text to show
   
   if (server.hasArg("button1on"))
      {
      button1ON = server.arg("button1on");
      Serial.print("Thereceived button1 is:             ");
      Serial.println(button1ON);
      //pinMode(D5, OUTPUT);
      digitalWrite(D5, HIGH);
      delay(10);
      digitalWrite(D5, LOW);
      delay(60);
      digitalWrite(D5, HIGH);
      delay(5);
      } 

   if (server.hasArg("button2on"))
      {
      button2ON = server.arg("button2on");
      Serial.print("Thereceived button2 is:             ");
      Serial.println(button2ON);
     // pinMode(D6, OUTPUT);
      digitalWrite(D6, HIGH);
      delay(20);
      digitalWrite(D6, LOW);
      delay(60);
      digitalWrite(D6, HIGH);
      }         
  server.send(200, "text/html", getPage());       //Response to the HTTP request
}  


// ===================================================
// Handle root
// ===================================================
void handleRoot() 
{   
  if (server.args() ) 
    {
    handleSubmit();
    } 
  else 
    {
    server.send(200, "text/html", getPage());  
    }
}


// ===================================================
// Setup
// ===================================================
void setup()
{
  delay(1000);
  Serial.begin(115200);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid,password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();
  delay(500);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
}


// ===================================================
// Loop
// ===================================================
void loop()
{  
  server.handleClient(); 
  delay(50);
}
