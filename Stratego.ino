//Sending 
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// The port that is used by the server to listen
#define LISTEN_PORT 80

// NDL network information
const char* ssid = "NDL_24G";
const char* password = "RT-AC66U";

MDNSResponder mdns;
ESP8266WebServer server(LISTEN_PORT);


//Servo and magnet
#include <Servo.h>
#define Servo_PWM D8
#define MAGNET D6
Servo MG995_Servo;

//Switch
#define switchPin D7

//Motors
//pins of the big motor
#define vertDirPin D1
#define vertStepPin D2
//pins of the small motor
#define horDirPin D3
#define horStepPin D4
#define msPin D5 //for lower speed, lower torque
int curX = 7; //coordinates of the orientation point (before moving to somewhere go back to this point)
int curY = -1;


void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.println("Start connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.print(".");
  Serial.print("Connected to ");
  Serial.print(ssid);
  Serial.print(". IP address: ");
  Serial.println(WiFi.localIP());
  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }

  // Allow Cross-Origin Resource Sharing (needed to allow requests from the JS front-end)
  server.enableCORS(true);

  // Listen to /move and /remove API-calls
  server.on("/move", handleMove);
  server.on("/remove", handleRemove);
  server.on("/win", handleWin);

  // Start the server
  server.begin();

  Serial.println("HTTP server started");
  
  //Servo and magnet
  pinMode(MAGNET, OUTPUT);
  digitalWrite(MAGNET, HIGH);
  
  //Motors
  pinMode(horStepPin, OUTPUT);
  pinMode(horDirPin, OUTPUT);
  pinMode(vertDirPin, OUTPUT);
  pinMode(vertStepPin, OUTPUT);
  pinMode(switchPin, INPUT_PULLUP);
  pinMode(msPin,OUTPUT);
  digitalWrite(horDirPin, LOW);
  digitalWrite(vertDirPin, LOW);
  
}

//----------------------servo and maget code functions-------------------------
//write(90) and it stops
//further away from 90 the faster it goes
//below 90 is clockwise (make magnet go up)
//above 90 is counterclockwise (make magnet go down)

// Let the servo go down
void servoDown() {
  yield();
  MG995_Servo.attach(Servo_PWM);
  MG995_Servo.write(110);
  delay(1700);
  MG995_Servo.detach();
  
}

// Let the servo go up
void servoUp() {
  MG995_Servo.attach(Servo_PWM);
  while(!digitalRead(switchPin)){
     yield();
     MG995_Servo.write(80);
  }
  MG995_Servo.detach();
}

void pickUpPiece() {
  servoDown();
  digitalWrite(MAGNET, LOW);
  delay(2000);
  servoUp();
}

void putDownPiece() {
  servoDown();
  delay(1000);
  digitalWrite(MAGNET, HIGH);
  delay(4000); //4000 not 2000 otherwise pawn still sticks
  servoUp();
}

void moveTo(int toX, int toY) {
  int diffX = toX - curX;
  int diffY = toY - curY;

  digitalWrite(msPin, HIGH); //ms pins on for 1/16 step
  if(diffX > 0){
    digitalWrite(vertDirPin, HIGH);
  }
  for(int s = 0; s < abs(diffX); s++){
      for(int i = 0; i < 2250; i++){
        yield(); //give other processes on the Wemos a chance (otherwise it crashes)
        digitalWrite(vertStepPin, HIGH);
        delayMicroseconds(300); //the lower the faster the steps are (increase in speed)
        digitalWrite(vertStepPin, LOW);
        delayMicroseconds(300);
     }
  }
  digitalWrite(vertDirPin, LOW);
  
  if(diffY < 0){
    digitalWrite(horDirPin, HIGH);
  }
  for(int s = 0; s < abs(diffY); s++){
      for(int i = 0; i < 2400; i++){
        yield(); //give other processes on the Wemos a chance (otherwise it crashes)
        digitalWrite(horStepPin, HIGH);
        delayMicroseconds(300); //the lower the faster the steps are (increase in speed)
        digitalWrite(horStepPin, LOW);
        delayMicroseconds(300);
     }
  }
  digitalWrite(horDirPin, LOW); 
  digitalWrite(msPin, LOW); //ms pins not needed
  curX = toX;
  curY = toY;
}

// handleRemove handles a remove request from the players
void handleRemove() {
   Serial.println("remove");
// Retrieve the coordinates from the request parameters
  String fX = server.arg("fromX");
  String fY = server.arg("fromY");

// Convert the coordinates to integers
  int fromX = fX.toInt();
  int fromY = fY.toInt();

// Example code to remove a piece
moveTo(fromX, fromY);
pickUpPiece();
delay(2000);
moveTo(7, -1);
putDownPiece();
delay(2000);

// When everything is finished, send OK to the players
  server.send(200, "text/plain", "Succeeded");
}

// handleMove handles a move request from the players
void handleMove() {
// Retrieve the coordinates from the request parameters
  String fX = server.arg("fromX");
  String fY = server.arg("fromY");
  String tX = server.arg("toX");
  String tY = server.arg("toY");

// Convert the coordinates to integers
  int fromX = fX.toInt();
  int fromY = fY.toInt();
  int toX = tX.toInt();
  int toY = tY.toInt();

// Example code to move a piece
moveTo(fromX, fromY);
pickUpPiece();
delay(2000);
moveTo(toX, toY);
putDownPiece();
delay(2000);

// When everything is finished, send OK to the players
  server.send(200, "text/plain", "Succeeded");
}

// handleWin handles the signal that a game is finished
void handleWin() {
// Exmaple code to move the motors to inital position:
 moveTo(7, -1);
 
 // When everything is finished, send OK to the players
  server.send(200, "text/plain", "Succeeded");
}

void loop() {
  server.handleClient();
  //MG995_Servo.attach(Servo_PWM);
// yield(); //servo did some unexpected things, this made it less I think
//  pickUpPiece();
//  delay(2000);
//  putDownPiece();
//  delay(2000);

  
}
  
