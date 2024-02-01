#include <LiquidCrystal.h>
#include <DHT.h>
#include <Thread.h>
#include <StaticThreadController.h>
#include <ThreadController.h>

// Definimos el pin digital donde se conecta el sensor
#define DHTPIN 12
// Dependiendo del tipo de sensor
#define DHTTYPE DHT11

#define COLS 16 // Columnas del LCD
#define ROWS 2 // Filas del LCD

#define EJE_X 0
#define EJE_Y 1

#define BOTON 2
#define JOYSTICK 13
#define LED2 11
#define LED1 1
#define LOW 0
#define HIGH 1
#define TRIG 9
#define ECHO 10

#define DIRECCION_ARRIBA 0
#define DIRECCION_ABAJO 1

#define MAX_INTENSITY 250

LiquidCrystal lcd(8, 7, 6, 5, 4, 3);
DHT dht(DHTPIN, DHTTYPE);

String bebidas[] = {"i.  Cafe solo ", "ii. Cafe cortado ", "iii. Cafe doble ", "iv. Cafe premium ", "v.  Chocolate ", " "};
float precios[] = {1.00, 1.10, 1.25, 1.50, 2.00}; 
String admin_op[] = {"i.   Ver temperatura", "ii.  Ver distancia sensor", "iii. Ver contador", "iv.  Modificar precios"};

int now[] = {0, 1};
int admin_now[] = {0,1};

int admin_cursor = 1;
int cursor = 1;
int show_time = 0;
int time_pass = 0;
float price = 0.00;
int scroll = 0;

int counter = 0;
int time = 0;
int enable_reset = 0;
int in_admin = 0;

volatile bool start = false;
volatile bool stop = false;
volatile int change = 0;
long start_time = 0;
long stop_time = 0;

ThreadController controller = ThreadController();

Thread startThread = Thread();
Thread distanceThread = Thread();
Thread lcdThread = Thread();
Thread waitThread = Thread();
Thread tempThread = Thread();
Thread switchThread = Thread();
Thread adminlcdThread = Thread();
Thread admintempThread = Thread();
Thread admintimeThread = Thread();
Thread admindistThread = Thread();
Thread adminpriceThread = Thread();
Thread measureThread = Thread();

void callback_start() {
  enable_reset = 0;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("CARGANDO ...");
  
  counter = counter + 1;
  
  if (counter % 2 != 0){
    digitalWrite(LED1, HIGH);
  }
  
  if (counter % 2 == 0){
    digitalWrite(LED1, LOW);
  }

  if (counter == 6){
    counter = 0;
    startThread.enabled = false;
    measureThread.enabled = true;
    distanceThread.enabled = true;
    waitThread.enabled = true;
  }
}

void callback_distance() {
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);          //Enviamos un pulso de 10us
  digitalWrite(TRIG, LOW);
  float t = pulseIn(ECHO, HIGH); //obtenemos el ancho del pulso
  float d = t/59;
  if (d < 100 ){
    waitThread.enabled = false;
    tempThread.enabled = true;
    distanceThread.enabled = false;
  }  
}

void callback_wait() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("ESPERANDO");
  lcd.setCursor(0,1);
  lcd.print("CLIENTE");
}

void callback_temp() {
  enable_reset = 1;

  float t = dht.readTemperature();
  float h = dht.readHumidity();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("TEMP: ");
  lcd.print(t);
  lcd.print(" C");
  lcd.setCursor(0,1);
  lcd.print("HUM: ");
  lcd.print(h);
  lcd.print(" %");

  counter = counter + 1;
  
  if (counter == 5){
    tempThread.enabled = false;
    lcdThread.enabled = true;
    counter = 0;
  }
}

void callback_lcd() {
  lcd.clear();
  int direccion = -1;
  int valorX = analogRead(EJE_X);
  int valorY = analogRead(EJE_Y);
  counter = counter + 1;
  scroll = scroll + 1;
  
  if (valorY > 700) {
    direccion = DIRECCION_ARRIBA;
  } else if (valorY < 200) {
    direccion = DIRECCION_ABAJO;
  }

  if (direccion == DIRECCION_ARRIBA) {
    if (cursor != 1){
      if (now[0] != 0 && cursor % 2 != 0) {
        now[0] = now[0] - 2;
        now[1] = now[1] - 2;
      }
      cursor--;
      scroll = 0;
    }

  } else if (direccion == DIRECCION_ABAJO) {
    if (now[1] != 6 && cursor != 5){
      if (cursor % 2 == 0) {
        now[0] = now[0] + 2;
        now[1] = now[1] + 2;
      }
      cursor++;
      scroll = 0;
    }
  }

  lcd.setCursor(0,0);
  if (cursor % 2 != 0){
    if (scroll < 4 ){
      lcd.print(bebidas[now[0]]);
      
    }else if(scroll > 6){
      scroll = 0;
    }else{
      lcd.print(precios[now[0]]);
      lcd.print(" E");
    }

  }else{
    lcd.print(bebidas[now[0]]);
  }

  lcd.setCursor(0,1);
  if (cursor % 2 == 0){
    if(scroll < 4){
      lcd.print(bebidas[now[1]]);
    }else if(scroll > 6){
      scroll = 0;
    }else if (cursor != 5) {
      lcd.print(precios[now[1]]);
      lcd.print(" E");
    }
    
  }else{
    lcd.print(bebidas[now[1]]);
  }
  

  int boton = digitalRead(JOYSTICK);
  if (boton == LOW){
    scroll = 0;
    if (in_admin == 0){
      counter = 0;
      time = random(4,8);
      lcdThread.enabled = false;
      switchThread.enabled = true;
      

    }else if (in_admin == 1 && counter > 3){
      counter = 0;
      price = precios[cursor-1];
      lcdThread.enabled = false;
      adminpriceThread.enabled = true;
    }
  } else if (valorX < 200 && counter > 3){
    scroll = 0;
    counter = 0;
    lcdThread.enabled = false;
    adminlcdThread.enabled = true;
  }
}

void callback_switch() {
  int intens = 0;
  int percent = 0;

  counter = counter + 1;
  
  if (counter <= time){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Preparando Cafe...");
    
    intens = (MAX_INTENSITY / time) * counter;
    
    analogWrite(LED2, intens);
    
    lcd.setCursor(0,1);
    lcd.print((intens*100/MAX_INTENSITY));
    lcd.print(" %");
  }

  if (counter > time && counter <= (time + 3)){
    lcd.clear();
    lcd.print("RETIRE BEBIDA");
    analogWrite(LED2, 0);
  }

  if (counter > (time + 3)){
    switchThread.enabled = false;
    lcdThread.enabled = true;
  }
}

void pulse() {
  if (change == 0){
    start = true;
    change = change + 1;
  }else{
    stop = true;
    change = 0;
  }
}

void callback_measure() {
  
  if (stop_time - start_time >= 2000 && stop_time - start_time <= 3000 && enable_reset != 0){
    counter = 0;
    scroll = 0;
    in_admin = 0;
    start_time = 0;
    stop_time = 0;
    show_time = 0;

    cursor = 1;
    now[0] = 0;
    now[1] = 1;
    admin_now[0] = 0;
    admin_now[1] = 1;
    admin_cursor = 1;

    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    switchThread.enabled = false;
    lcdThread.enabled = false;
    tempThread.enabled = false;
    adminlcdThread.enabled = false;
    admindistThread.enabled = false;
    admintempThread.enabled = false;
    adminpriceThread.enabled = false;
    distanceThread.enabled = true;
    waitThread.enabled = true;
    
  } else if (stop_time - start_time >= 5000){
    counter = 0;
    scroll = 0;
    in_admin = 1;
    enable_reset = 1;
    start_time = 0;
    stop_time = 0;
    show_time = 0;
   
    cursor = 1;
    now[0] = 0;
    now[1] = 1;
    admin_now[0] = 0;
    admin_now[1] = 1;
    admin_cursor = 1;

    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    switchThread.enabled = false;
    lcdThread.enabled = false;
    tempThread.enabled = false;
    distanceThread.enabled = false;
    waitThread.enabled = false;
    admindistThread.enabled = false;
    admintempThread.enabled = false;
    adminpriceThread.enabled = false;
    adminlcdThread.enabled = true;
  }
}

void callback_adminlcd() {
  lcd.clear();
  int direccion = -1;
  int valorY = analogRead(EJE_Y);
  scroll = scroll + 1;
  
  if (valorY > 700) {
    direccion = DIRECCION_ARRIBA;
  } else if (valorY < 200) {
    direccion = DIRECCION_ABAJO;
  }

  if (direccion == DIRECCION_ARRIBA) {
    if (admin_cursor != 1){
      if (admin_now[0] != 0 && admin_cursor % 2 != 0) {
        admin_now[0] = admin_now[0] - 2;
        admin_now[1] = admin_now[1] - 2;
      }
      admin_cursor--;
      scroll = 0;
    }

  } else if (direccion == DIRECCION_ABAJO) {
    if (admin_now[1] != 4 && admin_cursor != 4){
      if (admin_cursor % 2 == 0) {
        admin_now[0] = admin_now[0] + 2;
        admin_now[1] = admin_now[1] + 2;
      }
      admin_cursor++;
      scroll = 0;
    }
  }

  lcd.setCursor(0,0);
  if (admin_cursor % 2 != 0){
    if(scroll < 4){
      lcd.print(admin_op[admin_now[0]].substring(0,9));
    }else if(scroll > 6){
      scroll = 0;
    }else{
      lcd.print(admin_op[admin_now[0]].substring(9));
    }
  }else{
    lcd.print(admin_op[admin_now[0]]);
  }
  

  lcd.setCursor(0,1);
  if (admin_cursor % 2 == 0){
    if(scroll < 4 ){
      if(admin_cursor == 2){
        lcd.print(admin_op[admin_now[1]].substring(0,9));
      }else{
        lcd.print(admin_op[admin_now[1]].substring(0,15));
      }
    }else if(scroll > 6){
      scroll = 0;
    }else{
      if(admin_cursor == 2){
        lcd.print(admin_op[admin_now[1]].substring(9));
      }else{
        lcd.print(admin_op[admin_now[1]].substring(15));
      }
    }
  }else{
    lcd.print(admin_op[admin_now[1]]);
  }

  int boton = digitalRead(JOYSTICK);
  if (boton == LOW){
    scroll = 0;
    if (admin_cursor == 1){
      adminlcdThread.enabled = false;
      admintempThread.enabled = true;
    }else if(admin_cursor == 2){
      adminlcdThread.enabled = false;
      admindistThread.enabled = true;
    }else if(admin_cursor == 3){
      adminlcdThread.enabled = false;
      show_time = 1;
    }else{
      adminlcdThread.enabled = false;
      lcdThread.enabled = true;
    }
  }
}

void callback_admintemp() {
  int valorX = analogRead(EJE_X);
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("TEMP: ");
  lcd.print(t);
  lcd.print(" C");
  lcd.setCursor(0,1);
  lcd.print("HUM: ");
  lcd.print(h);
  lcd.print(" %");

  if (valorX < 200){
    admintempThread.enabled = false;
    adminlcdThread.enabled = true;
  }
}

void callback_admintime() {
  
  time_pass = time_pass + 1;

  if(show_time != 0){ 
    int valorX = analogRead(EJE_X);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("TIME: ");
    lcd.print(time_pass);
    lcd.print(" s.");

    if (valorX < 200){
      show_time = 0;
      adminlcdThread.enabled = true;
    }
  }
}

void callback_admindist() {
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);          //Enviamos un pulso de 10us
  digitalWrite(TRIG, LOW);

  float t = pulseIn(ECHO, HIGH); //obtenemos el ancho del pulso
  float d = t/59;
  int valorX = analogRead(EJE_X);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Distancia: ");
  lcd.print(d);
  lcd.print(" cm");

  if (valorX < 200){
    admindistThread.enabled = false;
    adminlcdThread.enabled = true;
  }
    
}

void callback_adminprice() {
  lcd.clear();
  int direccion = -1;
  int valorY = analogRead(EJE_Y);
  int valorX = analogRead(EJE_X);
  counter = counter + 1;
  
  if (valorY > 700) {
    direccion = DIRECCION_ARRIBA;
  } else if (valorY < 200) {
    direccion = DIRECCION_ABAJO;
  }

  if (direccion == DIRECCION_ARRIBA) {
    price = price + 0.05;

  } else if (direccion == DIRECCION_ABAJO && price != 0) {
    price = price - 0.05;
  }

  lcd.setCursor(0,0);
  lcd.print(price);
  lcd.print(" E");
  lcd.setCursor(0,1);
  Serial.println(precios[cursor-1]);
  lcd.print("Pulsa Confirmar");

  int boton = digitalRead(JOYSTICK);
  if (boton == LOW && counter > 3){
    counter = 0;
    precios[cursor-1] = price;
    adminpriceThread.enabled = false;
    lcdThread.enabled = true;
  } else if (valorX < 200){
    counter = 0;
    adminpriceThread.enabled = false;
    lcdThread.enabled = true;
  }
}

void setup() {
  pinMode(BOTON, INPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  
  pinMode(JOYSTICK, INPUT_PULLUP);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  digitalWrite(TRIG, LOW);
  
  lcd.begin(COLS, ROWS);
  
  dht.begin();

  startThread.enabled = true;
  startThread.setInterval(1000);
  startThread.onRun(callback_start);

  distanceThread.enabled = false;
  distanceThread.setInterval(100);
  distanceThread.onRun(callback_distance);

  waitThread.enabled = false;
  waitThread.setInterval(100);
  waitThread.onRun(callback_wait);

  lcdThread.enabled = false;
  lcdThread.setInterval(200);
  lcdThread.onRun(callback_lcd);

  tempThread.enabled = false;
  tempThread.setInterval(1000);
  tempThread.onRun(callback_temp);

  switchThread.enabled = false;
  switchThread.setInterval(1000);
  switchThread.onRun(callback_switch);

  adminlcdThread.enabled = false;
  adminlcdThread.setInterval(200);
  adminlcdThread.onRun(callback_adminlcd);

  admintempThread.enabled = false;
  admintempThread.setInterval(500);
  admintempThread.onRun(callback_admintemp);

  admintimeThread.enabled = true;
  admintimeThread.setInterval(1000);
  admintimeThread.onRun(callback_admintime);

  admindistThread.enabled = false;
  admindistThread.setInterval(500);
  admindistThread.onRun(callback_admindist);

  adminpriceThread.enabled = false;
  adminpriceThread.setInterval(200);
  adminpriceThread.onRun(callback_adminprice);

  measureThread.enabled = false;
  measureThread.setInterval(250);
  measureThread.onRun(callback_measure);

  attachInterrupt(digitalPinToInterrupt(BOTON), pulse, CHANGE);
}

void loop() {
  
  if(startThread.shouldRun()){
    startThread.run();
  }

  if(admintimeThread.shouldRun()){
    admintimeThread.run();
  }

  if(distanceThread.shouldRun()){
    distanceThread.run();
  }

  if(waitThread.shouldRun()){
    waitThread.run();
  }

  if(tempThread.shouldRun()){
    tempThread.run();
  }
  
  if(lcdThread.shouldRun()){
    lcdThread.run();
  }

  if(switchThread.shouldRun()){
    switchThread.run();
  }

  if(adminlcdThread.shouldRun()){
    adminlcdThread.run();
  }

  if(admintempThread.shouldRun()){
    admintempThread.run();
  }

  if(admindistThread.shouldRun()){
    admindistThread.run();
  }

  if(adminpriceThread.shouldRun()){
    adminpriceThread.run();
  }

  if(measureThread.shouldRun()){
    measureThread.run();
  }

  if(start == true){
    start_time = millis();
    start = false;
  }

  if(stop == true){
    stop_time = millis();
    stop = false;
  }

}
