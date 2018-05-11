  
#include <OneWire.h>
#include <DallasTemperature.h>

// Arduino數位腳位2接到1-Wire裝置
#define ONE_WIRE_BUS 7 
#define ledPin 13
/********************************************************************/
// Setup a oneWire instance to communicate with any OneWire devices  
// (not just Maxim/Dallas temperature ICs) 
OneWire oneWire(ONE_WIRE_BUS); 
/********************************************************************/
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
/********************************************************************/ 

  // put your setup code here, to run once:
  boolean  b_power_sw = true;   //0:關  1:開
  boolean  b_hot_cold = true;   //0:制冷 1:加熱
  boolean  b_switch   = true;   //0:關  1:開
  boolean  b_reset    = false;  //0:關  1:開
  boolean  b_reload   = false;  //0:關  1:開
  boolean  b_situation= false;   //0:關  1:開
  int      i_time_min = -1;   //<0:持續 >0：工作控制時間，連動b_power_sw
  float    f_stop_temperature = 60;  //開關參數工作溫度區間，連動b_hot_cold,b_switch
  float    f_temp_rang = 0.3;
  String   str_cmd = "";
  String   str_check = "";
  char*    cp = "";
  
void setup() {
 // start serial port 
 pinMode(ledPin, OUTPUT);
 Serial.begin(115200); 
 Serial.println("Dallas Temperature IC Control Library Demo"); 
 // Start up the library 
 sensors.begin(); 

}
void m_set_range(float f_range){
  f_temp_rang = f_range;
}
void m_set_temperature(float f_temp){
  f_stop_temperature = f_temp;
}
void m_reload(){
  b_reload = false;
}

void m_reset(){
  b_reset = false;
}

void m_switch_off(){
  b_situation = false;
  digitalWrite(ledPin,LOW) ;
}

void m_switch_on(){
  b_situation = true;
  digitalWrite(ledPin, HIGH);
}

void m_setdata(char* cp){
    int i_item = 0;
    float f_value = -1.0;
    //Serial.println(cp); 
    char* cp_par = cp;
    char* cp_coin = NULL;
    cp_coin = strstr(cp, " ");
    cp = cp_coin+1;
    *cp_coin = '\0';
    if(strcasecmp(cp_par, "temp") == 0){
      i_item = 1;
    }else if(strcasecmp(cp_par, "rang") == 0){
      i_item = 2;
    }

    cp_coin = strstr(cp, "@");
    cp_par = cp;
    *cp_coin = '\0';

    f_value = atof(cp_par);

    if(f_value > 0){
      switch(i_item){
        case 1:
          m_set_temperature(f_value);
          break;
        case 2:
          m_set_range(f_value);
          break;
      }
    }
}

void m_showinfo(){
  if(b_hot_cold = true){
    Serial.println("Working Mode: Heating"); 
  }else{
    Serial.println("Working Mode: Colding"); 
  }
  Serial.print("Target Temperature is "); 
  Serial.print(f_stop_temperature);
  Serial.println(" ℃");

  Serial.print("Working range: "); 
  Serial.print(f_stop_temperature + f_temp_rang);
  Serial.print(" ~ ");
  Serial.print(f_stop_temperature - f_temp_rang);
  Serial.println(" ℃");
  
}

float m_read_temp(){
  sensors.requestTemperatures(); // Send the command to get temperature readings 
  return sensors.getTempCByIndex(0);
}

void loop() {
    
//----------------------------------------------------//
//----Serial回傳字串，字串結尾沒有\r\n            ----//
//----空指標表示不可使用null，需使用大寫NULL才可以----//
//----------------------------------------------------//
    //if(Serial.available() > 0){
    while(Serial.available()){
        char c = Serial.read();
        if(c != ';'){
            str_cmd += c;
            //Serial.println(str_cmd);
        }else{
            Serial.println(str_cmd);   
    
            //if(str_cmd == "START\r"){
            if(str_cmd == "START"){
                digitalWrite(ledPin, HIGH);
            }
            if(str_cmd == "STOP"){
               digitalWrite(ledPin,LOW) ;
            }
            if(str_cmd == "showinfo"){
              m_showinfo();
            }
            if((cp = strcasestr(str_cmd.c_str(), "SETDATA")) != NULL){
              cp = strstr(cp, " ");
              cp++;
              strcat(cp, "@");
              m_setdata(cp);
              cp = "";
            }
            str_cmd = "";                //一定要記得將s字串內容清空，否則會一直累加
        }
        delay(5);
    }
    
    float f_temp = m_read_temp();
    Serial.println(f_temp);
    delay(1000);
//-------------------------------------------------
    if(b_reset == true){
        m_reset();
    }
    if(b_reload == true){
        m_reload();
    }

    //heating
    if (b_hot_cold == true){
        if((f_temp < (f_stop_temperature - f_temp_rang)) & (b_situation == false)){
            m_switch_on();
        }else if((f_temp > (f_stop_temperature + f_temp_rang)) & (b_situation == true)){
            m_switch_off();        
        }
    //colding
    }else if(b_hot_cold == false){
        if((f_temp > (f_stop_temperature + f_temp_rang)) & (b_situation == false)){
            m_switch_on();
        }else if((f_temp < (f_stop_temperature - f_temp_rang)) & (b_situation == true)){
            m_switch_off();
        }
    }      

}
