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

#include <DTMF.h>

Tone freq1;
Tone freq2;

const int DTMF_freq1[] = { 1336, 1209, 1336, 1477, 1209, 1336, 1477, 1209, 1336, 1477 };
const int DTMF_freq2[] = {  941,  697,  697,  697,  770,  770,  770,  852,  852,  852 };

enum Actions{NewToken, SwitchClosed};
//uint8_t
const int PIN_COUNT = 1;
int pins_conected[]={2};  
String phone_number_pin_connected_map[]={"8675309"};

int closed_pins_index[PIN_COUNT]={-1};
int closed_pins_real_count=0;

int current_playing_closed_pins_index=0;

int descolgarPin = 10;
int tone1Pin=11;
int tone2Pin=12;

int call_time=10000;//change to 10000 on production
int call_interval=10000;//change to 30000 on production

long last_dialnumber_call=0;

bool activada = false;//Cambiar para false al ir a produccion
String activation_code="1111";
bool avisando = false;

String dtmf_tokens_buffer="";
long first_dtmf_start=0;
char dtmf_delimiter = '#';
int token_max_length=5000;
bool tokenStarted=false;

int sensorPin = A0;
float n=128.0;
// sampling rate in Hz
float sampling_rate=8926.0;

// Instantiate the dtmf library with the number of samples to be taken
// and the sampling rate.
DTMF dtmf = DTMF(n,sampling_rate);
int nochar_count = 0;
float d_mags[8];

void setup() {
  //start serial connection
  Serial.begin(115200);
  //configure pin 2 as an input and enable the internal pull-up resistor  
  pinMode(2, INPUT_PULLUP);
  pinMode(descolgarPin,OUTPUT);
  
  freq1.begin(tone1Pin);
  freq2.begin(tone2Pin);
    
  pinMode(13, OUTPUT);
}

void loop() {
  //read the pushbutton value into a variable
  //int sensorVal = digitalRead(2);
  //print out the value of the pushbutton
  //Serial.println(sensorVal); 
  if(activada){
    checkPinsSwitchesAndRegister(); 
    digitalWrite(13, HIGH);
  }else{
    digitalWrite(13, LOW);
  }
  if(avisando)
    dialNumber(phone_number_pin_connected_map[0]);// Solo para probar en el caso de llamar al numero d aviso (DESCOLGADO DE LINEA FIJA) /cambiar para version celular.

  String token= tokenParser();
  if(!token.equals(""))
    matchAction(NewToken,token);
}

void matchAction(int action,String param){
  if(action == NewToken){    
    if(param.equals(activation_code)) {
      activada = !activada;    
      if(!activada)
        avisando= false;    
    }       
  }    
  if(action == SwitchClosed) {
     Blink(200);// Eliminar en produccion
     int index = param.toInt();
     if(activada)   {   
        registerClosedPinIndex(index);
        avisando=true;
     } 
  } 
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
  if(millis() - last_dialnumber_call > (call_time + call_interval )){
    Serial.println("");
    Serial.println(phone_number);
    last_dialnumber_call =millis();
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
