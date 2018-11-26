/*
  Input Pull-up Serial

  This example demonstrates the use of pinMode(INPUT_PULLUP). It reads a digital
  input on pin 2 and prints the results to the Serial Monitor.

  The circuit:
  - momentary switch attached from pin 2 to ground
  - built-in LED on pin 13

  Unlike pinMode(INPUT), there is no pull-down resistor necessary. An internal
  20K-ohm resistor is pulled to 5V. This configuration causes the input to read
  HIGH when the switch is open, and LOW when it is closed.

  created 14 Mar 2012
  by Scott Fitzgerald

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/InputPullupSerial
*/
#include <Tone.h>
#include <EEPROM.h>
#include <DTMF.h>

Tone freq1;
Tone freq2;

const int DTMF_freq1[] = { 1336, 1209, 1336, 1477, 1209, 1336, 1477, 1209, 1336, 1477 };
const int DTMF_freq2[] = {  941,  697,  697,  697,  770,  770,  770,  852,  852,  852 };

enum Actions{NewToken, SwitchClosed};
//uint8_t

//sensors definition and map to number
const int PIN_COUNT = 1;
int pins_conected[]={2};  
String phone_number_pin_connected_map[]={"8675309"};

const byte EEPROM_ID = 0x99; // used to identify if valid data in EEPROM
//constants used to identify EEPROM addresses
const int ID_ADDR = 0; // the EEPROM address used to store the ID
const int is_calling_ADDR = 1; // the EEPROM address used to store the pin
const int is_active_ADDR = 2; // the EEPROM address used to store the interval

int sirenaPin = 9;
int descolgarPin = 10;
int tone1Pin=11;
int tone2Pin=12;

//dtmf detection params
int sensorPin = A0;
float n=128.0;
// sampling rate in Hz
float sampling_rate=8926.0;
// Instantiate the dtmf library with the number of samples to be taken
// and the sampling rate.
DTMF dtmf = DTMF(n,sampling_rate);
int nochar_count = 0;
float d_mags[8];

//Aplication state variables
int exit_interval=0;

int call_time=10000;//change to 10000 on production
int call_interval=10000;//change to 30000 on production

bool activada = false;//Cambiar para false al ir a produccion
long activation_time=0;
String activation_code="1111";
bool avisando = false;

//transient state is sensorsReadyToActivateAlarm()
long last_switch_closed_time = 0;
int switches_ready_interval = 10000;

//transient state registerClosedSwitch
int closed_pins_index[PIN_COUNT]={-1};
int closed_pins_real_count=0;

//transient state dialnumber()
long last_dialnumber_call_time=0;
int current_playing_closed_pins_index=0;

//transient state dtmf token parser
String dtmf_tokens_buffer="";
long first_dtmf_start=0;
char dtmf_delimiter = '#';
int token_max_length=5000;
bool tokenStarted=false;

void setup() {
  //start serial connection
  Serial.begin(115200);
  //configure pin 2 as an input and enable the internal pull-up resistor  
  pinMode(2, INPUT_PULLUP);
  pinMode(descolgarPin,OUTPUT);
  
  freq1.begin(tone1Pin);
  freq2.begin(tone2Pin);
    
  pinMode(13, OUTPUT);
  
  byte id = EEPROM.read(ID_ADDR);// read the first byte from the EEPROM
  if( id == EEPROM_ID){
    readAppState();
  }  
}

void loop() {
  //read the pushbutton value into a variable
  //int sensorVal = digitalRead(2);
  //print out the value of the pushbutton
  //Serial.println(sensorVal);   
  if(activada){    
    digitalWrite(13, HIGH);
  }else{
    digitalWrite(13, LOW);
  }
  //Siempre se velan los lazos para notificar en caso d q este algun sensor en estado incorrecto.
  checkPinsSwitchesAndRegister(); 
  
  if(avisando){
    dialNumber(phone_number_pin_connected_map[0]);// Solo para probar en el caso de llamar al numero d aviso (DESCOLGADO DE LINEA FIJA) /cambiar para version celular.
  }

  String token= tokenParser();
  if(!token.equals(""))
    matchAction(NewToken,token);
}

void matchAction(int action,String param){
  if(action == NewToken){    
    if(param.equals(activation_code)) {
      if(activada){
        avisando= false;
  
        activada= false;
        saveAppState();
      }else{
        if(sensorsReadyToActivateAlarm()){
          activada=true;
          saveAppState();
          activation_time=millis();
        }else
        Serial.println("Sistem it is not ready");        
      }         
    }       
  }    
  if(action == SwitchClosed) {
     Blink(200);// Eliminar en produccion
     int index = param.toInt();
     last_switch_closed_time = millis();
     if(activada && (millis - activation_time > exit_interval))   {   
        registerClosedPinIndex(index);
        avisando=true;
        saveAppState();
     }
  } 
}

bool sensorsReadyToActivateAlarm(){ 
  return millis() - last_switch_closed_time > switches_ready_interval;
}

void saveAppState(){
  EEPROM.write(ID_ADDR,EEPROM_ID);
  EEPROM.write(is_calling_ADDR,avisando);
  EEPROM.write(is_active_ADDR,activada);
}

void readAppState(){
  avisando = EEPROM.read(is_calling_ADDR);
  activada = EEPROM.read(is_active_ADDR);
}

void checkPinsSwitchesAndRegister(){  
  char buff[12];
  for(int i=0;i<PIN_COUNT;i++){
     bool isClosed = isSwitchClosedOnPin(pins_conected[i], true);
     if(isClosed){        
        ltoa(i, buff, 10);
        String param = buff;
        matchAction(SwitchClosed, param);                
     }
  }
}

void Blink(int interval){
  digitalWrite(13, HIGH);   
  delay(interval);
  digitalWrite(13, LOW);
}
void playBuzzerTone(){
  
}

bool isSwitchClosedOnPin(int pin,bool isPullUP){
  bool isClosed=false;
  int sensorVal = digitalRead(pin);
  if (sensorVal == HIGH) {
    //switch cerrado sin resistores PULL_UP
    isClosed=true;    
  } else {
    //switch abierto sin resistores PULL_UP
    isClosed=false;    
  }
  if(isPullUP)
    isClosed=!isClosed;

  return isClosed;
}

void playDTMF(int number, long duration)
{
  freq1.play(DTMF_freq1[number], duration);
  freq2.play(DTMF_freq2[number], duration);
}

void dialNumber(String phone_number){    
  if(millis() - last_dialnumber_call_time > (call_time + call_interval )){
    Serial.println("");    
    last_dialnumber_call_time =millis();
    digitalWrite(descolgarPin,HIGH);  
    for(int i = 0; i < phone_number.length(); i ++)
    {
      char curr = phone_number.charAt(i);
      Serial.print( curr );     
      playDTMF(atoi(curr), 50);
      delay(60);
    }
    digitalWrite(descolgarPin,LOW);
  }
}

bool registerClosedPinIndex(int index){  
  bool isNew=true;
  Serial.println("Switch closed");
  for(int i =0;i < PIN_COUNT; i++){     
    if(closed_pins_index[i]==index)
      isNew=false;
  } 
  if(isNew){
    closed_pins_index[closed_pins_real_count]=index;
    closed_pins_real_count++;    
  }
  return isNew;
}

String tokenParser(){

  long curr_time = millis();
  String result = "";
  
  if(curr_time - first_dtmf_start > token_max_length && tokenStarted ){
    Serial.println("end token");   
    String result =dtmf_tokens_buffer;
    dtmf_tokens_buffer="";
    tokenStarted=false;
    return result;
  }
  
  char thischar;
  dtmf.sample(sensorPin);
  dtmf.detect(d_mags,0);
  thischar = dtmf.button(d_mags,1800.);
  if(thischar) {
    Serial.print(thischar);
    nochar_count = 0;
    if(thischar == dtmf_delimiter){
      Serial.println("end token");      
      String result =dtmf_tokens_buffer;
      dtmf_tokens_buffer="";
      tokenStarted=false;
      return result;
    }
    
    if(dtmf_tokens_buffer.length() ==0){
       first_dtmf_start = millis();
       tokenStarted=true;
       Serial.println("start token");
    }

    dtmf_tokens_buffer += thischar;
    // Print the magnitudes for debugging
//#define DEBUG_PRINT
#ifdef DEBUG_PRINT
    for(int i = 0;i < 8;i++) {
      Serial.print("  ");
      Serial.print(d_mags[i]);
    }
    Serial.println("");
#endif
  } else {
    // print a newline 
    if(++nochar_count == 50)Serial.println("");
    // don't let it wrap around
    if(nochar_count > 30000)nochar_count = 51;
  }    
  
  return "";
}

