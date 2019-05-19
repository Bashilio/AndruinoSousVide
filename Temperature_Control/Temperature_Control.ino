//20180724 add EEPROM Format
//20180826 add display mode for Json 
//                          add parameter (b_JSON_MODE)
//                          add char* m_show_json_float(char* cp_title, float f_value, bool b_brackets);
//                          add char* m_show_json_int(char* cp_title, int i_value, bool b_brackets);
//                          add char* m_show_json_cp(char* cp_title, char* cp_value, bool b_brackets);
//20181120 fix m_setdata() and m_seteep()
//                          "if(f_value > 0){"  to  "if(f_value >= 0){"
//
//------ include Start ------//
#include <OneWire.h>
#include <DallasTemperature.h>

#include <EEPROM.h>   //20180724
//====== include End   ======//  

//------ Pin Define Start ------//
//--Arduino UNO--//
#define BTN_POWER 7                     //手動開關偵測腳位
#define RELAY_SW 8                      //繼電器控制
// Arduino數位腳位12接到1-Wire裝置
#define ONE_WIRE_BUS 12 
#define ledPin 13

//--STM32--//
//#define PA1 7
//#define PA3 8


//====== Pin Define End   ======//
//------ EEPROM Format Start ------//
//  FF    =   RESERVE
//  A0~A9 =   SN area
//  BB    =   0x10 10 bytes uc_MODE: 0~4
//  C0~C1 =   0x11 2 bytes ui_MAX_WORKING_MIN  (maxt)  C0:MSB      C1:LSB
//  D0~D1 =   0x13 2 bytes f_TARG_TEMP                 D0:Interge  D1:1/256
//  EE    =   0x15 1 byte  f_TEMP_RANG   ±0 ~ ±25.5
//0x000  A0  A1  A2  A3  A4  A5  A6  A7    A8  A9  FF  FF  FF  FF  FF  FF
//0x010  BB  C0  C1  D0  D1  EE  FF  FF    FF  FF  FF  FF  FF  FF  FF  FF
//0x020  FF  FF  FF  FF  FF  FF  FF  FF    FF  FF  FF  FF  FF  FF  FF  FF
//  ...
//0x3FF  FF  FF  FF  FF  FF  FF  FF  FF    FF  FF  FF  FF  FF  FF  FF  FF

//====== EEPROM Format Start ======//

//------ Others Define Start ------//
#define BAUDRATE 9600
//====== Others Define End   ======//

//------ Parameter Declare Start ------//
  //String   str_IAM = "I'm L-110-0000"; //L系列-110V-模組編號
  String   str_IAM = "";
  String   str_tmp = "";
  boolean  b_SYST_POWE   = false;       //系統總開關 0:關  1:開 預設為關
  boolean  b_RELAY= false;              //繼電器開關 0:關  1:開  預設為關
  unsigned long ul_START_TIME;          //工作起始時間 Device 重新上電時的重新計算
  unsigned char uc_MODE = 0;            //工作持繼時間 0:不限時 1:20分鐘 2:60分鐘  3:120分鐘 4:自定時間
  unsigned char uc_tType = 0;           //顯示工作持續時間單位  0:以分鐘顯示(不顯示在series port) 1:以秒顯示
  //unsigned int  ui_MAX_WORKING_MIN = 60;
  unsigned int  ui_MAX_WORKING_MIN = 0;

  boolean  b_HEAT_MODE = true;   //FALSE:制冷 TRUE:加熱
  boolean  b_SHOW_TEMP = false;
  boolean  b_JSON_MODE = true;    //20180826
  float    f_TARG_TEMP = 0;  //開關參數工作溫度區間
  float    f_TEMP_RANG = 0;

  String   str_CMD = "";
  String   str_check = "";
  char*    cp = "";
  
  //unsigned  ucar_eep[1024];   //20180724
//====== Parameter Declare End   ======//

/********************************************************************/
// Setup a oneWire instance to communicate with any OneWire devices  
// (not just Maxim/Dallas temperature ICs) 
OneWire oneWire(ONE_WIRE_BUS); 
/********************************************************************/
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
/********************************************************************/ 

//------ Function Declare Start ------//
void m_read_cmd(void);
void m_setdata(char* cp);
void m_showinfo(void);
float m_show_working_time(unsigned char uc_tType);
void m_poweron(void);
void m_poweroff(void);

float m_read_temp(void);

void m_relay_on(void);
void m_relay_off(void);

void m_set_range(float f_range);
void m_set_temperature(float f_temp);

int serial_putc( char c, struct __file * );
void printf_begin(void);

//-------------------20180724--------------------//
void m_seteep(char* cp);
void m_showeep(void);
void m_ReadParameterFromEEP(void);

//-------------------20180826--------------------//
char* m_show_json_float(char* cp_title, float f_value, bool b_brackets);
char* m_show_json_int(char* cp_title, int i_value, bool b_brackets);
char* m_show_json_cp(char* cp_title, char* cp_value, bool b_brackets);

//====== Function Declare End   ======//
  
void setup() {
   pinMode(ledPin, OUTPUT);
   pinMode(RELAY_SW, OUTPUT);
   pinMode(BTN_POWER, INPUT);
   
   // start serial port 
   Serial.begin(BAUDRATE); 
   Serial.println("Connected..."); 


//   Serial.println(m_show_json_int("TEST_INT", 9987));
//   Serial.println(m_show_json_int("TEST_INT2", -1243));
//   Serial.println(m_show_json_cp("TEST_CP", "abcd"));
//   Serial.println(m_show_json_float("TEST_FLOAT", 23.489));
//   Serial.println(m_show_json_float("TEST_FLOAT2", -126.629));


   m_ReadParameterFromEEP();    //Read parameter from EEPROM 
   
   sensors.begin(); 
   printf_begin();   //開啟使用printf函數直接顯示在Serial Port，必需在Serial.begin之後
   ul_START_TIME = millis();

}

void loop() {
    //---read command from RS232
    m_read_cmd();

    if(digitalRead(BTN_POWER) == true){
      b_SYST_POWE = !b_SYST_POWE;
      Serial.println("BTN_IN Button clicked, delay 1 sec ");
      delay(1000);
    }else{
      
      float f_temp = m_read_temp();
      if(b_SHOW_TEMP == true){
          if(b_JSON_MODE == false){
            Serial.println(f_temp);
          }else{
            Serial.println(m_show_json_float("TEMPERATURE", f_temp, true));
          }
          delay(1000);
      }

      //Check System Power situation
      if((b_SYST_POWE == true) & (b_RELAY == false)){
       // m_relay_on();

      }else if((b_SYST_POWE == true) & (b_RELAY == true)){
        delay(1000);
        if(uc_MODE != 0){
          float f_cont_time = m_show_working_time(0);
          int i_s = (int)(f_cont_time*100)%100*0.6;
          if(f_cont_time >= ui_MAX_WORKING_MIN){
            printf("Mission Completed!\nTotal Time : %d m %d s\n", (int)f_cont_time, i_s); 
            b_SYST_POWE = false;
          }
        }
      }    
      if(b_SYST_POWE == false ){
        ul_START_TIME = millis(); //重新上電，重新計時        
        if(b_RELAY == true){
          m_relay_off();
        }
      }
  
      if(b_SYST_POWE == true){
        //hotting
        if (b_HEAT_MODE == true){
            if((f_temp < (f_TARG_TEMP - f_TEMP_RANG)) & (b_RELAY == false)){
                m_relay_on();
            }else if((f_temp > (f_TARG_TEMP + f_TEMP_RANG)) & (b_RELAY == true)){
                m_relay_off();        
            }
        //colding
        }else if(b_HEAT_MODE == false){
            if((f_temp > (f_TARG_TEMP + f_TEMP_RANG)) & (b_RELAY == false)){
                m_relay_on();
            }else if((f_temp < (f_TARG_TEMP - f_TEMP_RANG)) & (b_RELAY == true)){
                m_relay_off();
            }
        }      
      }
    }
}

//=================================================================================//
void m_set_range(float f_range){
  f_TEMP_RANG = f_range;

  if(b_JSON_MODE == false){
    Serial.print("Set temperature range = ±");
    Serial.print(f_TEMP_RANG);
    Serial.println("℃");
  }else{
    Serial.println(m_show_json_float("SET_RANG", f_TEMP_RANG, true));
  }
  
}
void m_set_temperature(float f_temp){
  f_TARG_TEMP = f_temp;

  if(b_JSON_MODE == false){
    Serial.print("Set target temperature = ");
    Serial.print(f_TARG_TEMP);
    Serial.println("℃");
  }else{
    Serial.println(m_show_json_float("SET_TEMPERATURE", f_TARG_TEMP, true));
  }
  
}

void m_relay_on(){
  b_RELAY = true;
  digitalWrite(RELAY_SW, HIGH);
  if(b_JSON_MODE == false){
    Serial.println("Relay trun on"); 
  }else{
    Serial.println(m_show_json_cp("RELAY_ON", "Relay trun on", true));
  }
}
void m_relay_off(){
  b_RELAY = false;
  digitalWrite(RELAY_SW,LOW) ;
  if(b_JSON_MODE == false){
    Serial.println("Relay trun off"); 
  }else{
    Serial.println(m_show_json_cp("RELAY_OFF", "Relay trun off", true));
  }
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
    }else if(strcasecmp(cp_par, "mode") == 0){
      i_item = 3;
    }else if(strcasecmp(cp_par, "maxt") == 0){
      i_item = 4;
    }

    cp_coin = strstr(cp, "@");
    cp_par = cp;
    *cp_coin = '\0';

    f_value = atof(cp_par);

    if(f_value >= 0){
      switch(i_item){
        case 1:
          m_set_temperature(f_value);
          break;
        case 2:
          m_set_range(f_value);
          break;
        case 3:
          uc_MODE = (unsigned char)f_value;
          if(uc_MODE == 1){
            ui_MAX_WORKING_MIN = 20;
          }else if(uc_MODE == 2){
            ui_MAX_WORKING_MIN = 60;
          }else if(uc_MODE == 3){
            ui_MAX_WORKING_MIN = 120;
          }
          break;
        case 4:
          ui_MAX_WORKING_MIN = (unsigned int)f_value;
          break;
      }
    }
}

void m_showinfo(){
  str_tmp = "";
  if(b_SYST_POWE == false){
    if(b_JSON_MODE == false){
      Serial.println("System Closed");
    }else{
      Serial.println(m_show_json_cp("SHOWINFO", "System Closed", true));
    }
    
  }else{
    str_tmp += "{";
    float f_cont_time = m_show_working_time(0);
    int i_s = (int)(f_cont_time*100)%100*0.6;
    if(uc_MODE == 0){
      if(b_JSON_MODE == false){
        printf("Working Mode : %d\n", (int)uc_MODE);
        Serial.println("Target working time is unlimited,");
        printf("Wworking Time : %d m %d s\n", (int)f_cont_time, i_s); 
      }else{
//        Serial.println(m_show_json_int("SHOWINFO_WORKING_MODE", (int)uc_MODE, true));
//        Serial.println(m_show_json_cp("SHOWINFO_TARGET_WORKING_TIME", "unlimited", true));
//        Serial.println(m_show_json_int("SHOWINFO_WORKING_TIME_M", (int)f_cont_time, true ));
//        Serial.println(m_show_json_int("SHOWINFO_WORKING_TIME_S", i_s, true ));
        str_tmp += m_show_json_int("SHOWINFO_WORKING_MODE", (int)uc_MODE, false);
        str_tmp += ",";
        str_tmp += m_show_json_cp("SHOWINFO_TARGET_WORKING_TIME", "unlimited", false);
        str_tmp += ",";
        str_tmp += m_show_json_int("SHOWINFO_WORKING_TIME_M", (int)f_cont_time, false );
        str_tmp += ",";
        str_tmp += m_show_json_int("SHOWINFO_WORKING_TIME_S", i_s, false );
        str_tmp += ",";
      }
    }else{
      if(b_JSON_MODE == false){
        printf("Working Mode : %d\n", (int)uc_MODE);;
        printf("Target working time is %d min\n", ui_MAX_WORKING_MIN);
        printf("Wworking Time : %d m %d s\n", (int)f_cont_time, i_s); 
      }else{
//        Serial.println(m_show_json_int("SHOWINFO_WORKING_MODE", (int)uc_MODE, true));
//        Serial.println(m_show_json_int("SHOWINFO_TARGET_WORKING_TIME_M", ui_MAX_WORKING_MIN, true));
//        Serial.println(m_show_json_int("SHOWINFO_WORKING_TIME_M", (int)f_cont_time, true ));
//        Serial.println(m_show_json_int("SHOWINFO_WORKING_TIME_S", i_s, true ));
        str_tmp += m_show_json_int("SHOWINFO_WORKING_MODE", (int)uc_MODE, false);
        str_tmp += ",";
        str_tmp += m_show_json_int("SHOWINFO_TARGET_WORKING_TIME_M", ui_MAX_WORKING_MIN, false);
        str_tmp += ",";
        str_tmp += m_show_json_int("SHOWINFO_WORKING_TIME_M", (int)f_cont_time, false);
        str_tmp += ",";
        str_tmp += m_show_json_int("SHOWINFO_WORKING_TIME_S", i_s, false);
        str_tmp += ",";
      }
      
    }
    
    if(b_HEAT_MODE = true){
      if(b_JSON_MODE == false){
        Serial.println("Working Mode: Heating");
      }else{
//        Serial.println(m_show_json_cp("SHOWINFO_HEAT_MODE", "Heating", true));
        str_tmp += m_show_json_cp("SHOWINFO_HEAT_MODE", "Heating", false);
        str_tmp += ",";
      }
       
    }else{
      if(b_JSON_MODE == false){
        Serial.println("Working Mode: Colding");
      }else{
//        Serial.println(m_show_json_cp("SHOWINFO_HEAT_MODE", "Colding", true));
        str_tmp += m_show_json_cp("SHOWINFO_HEAT_MODE", "Colding", false);
        str_tmp += ",";
      }
       
    }

    if(b_JSON_MODE == false){
      Serial.print("Target Temperature is ");
      Serial.print(f_TARG_TEMP); 
      Serial.println(" ℃");
     //printf("Working range: %f ~ %f ℃\n", (f_TARG_TEMP + f_TEMP_RANG), (f_TARG_TEMP - f_TEMP_RANG));
      Serial.print("Working range: "); 
      Serial.print((f_TARG_TEMP + f_TEMP_RANG)); 
      Serial.print(" ~ "); 
      Serial.print((f_TARG_TEMP - f_TEMP_RANG)); 
      Serial.println(" ℃");
    }else{
//      Serial.println(m_show_json_float("SHOWINFO_TEMPERATURE", f_TARG_TEMP, true));
//      Serial.println(m_show_json_float("SHOWINFO_WORKING_TEMPERATURE_UPER", (f_TARG_TEMP + f_TEMP_RANG), true));
//      Serial.println(m_show_json_float("SHOWINFO_WORKING_TEMPERATURE_LOWER", (f_TARG_TEMP - f_TEMP_RANG), true));
      str_tmp += m_show_json_float("SHOWINFO_TEMPERATURE", f_TARG_TEMP, false);
      str_tmp += ",";
      str_tmp += m_show_json_float("SHOWINFO_WORKING_TEMPERATURE_UPER", (f_TARG_TEMP + f_TEMP_RANG), false);
      str_tmp += ",";
      str_tmp += m_show_json_float("SHOWINFO_WORKING_TEMPERATURE_LOWER", (f_TARG_TEMP - f_TEMP_RANG), false);
      str_tmp += "}";
      Serial.println(str_tmp);
    }
  }
}

float m_read_temp(){
  sensors.requestTemperatures(); 
  return sensors.getTempCByIndex(0);
}

void m_read_cmd(void){
  //----------------------------------------------------//
//----Serial回傳字串，字串結尾沒有\r\n            ----//
//----空指標表示不可使用null，需使用大寫NULL才可以----//
//----------------------------------------------------//
    while(Serial.available()){
        char c = Serial.read();
        if(c != ';'){
            if((c != '\n')||(c != '\r')){
                str_CMD += c;
            }
        }else{
            str_CMD.trim();
            Serial.println(str_CMD);   
            if(str_CMD == "whoru"){
                Serial.print("I'm ");
                Serial.println(str_IAM); ;
            }else if(str_CMD == "poweron"){
                if(b_SYST_POWE == false){
                  b_SYST_POWE = true;
                  Serial.println(m_show_json_cp("SYSTEM", "System Power on", true)); 
                  //Serial.println("System Power on");
                }else{
                  Serial.println(m_show_json_cp("SYSTEM", "System Power already on", true));
                  //Serial.println("System Power already on");
                }
                //b_SYST_POWE = true;
            }else if(str_CMD == "poweroff"){
                if(b_SYST_POWE == true){
                  b_SYST_POWE = false;
                  Serial.println(m_show_json_cp("SYSTEM", "System Power off.", true));
                  //Serial.println("System Power off.");
                }else{
                  Serial.println(m_show_json_cp("SYSTEM", "System Power already Stopped.", true));
                  //Serial.println("System Power already Stopped.");
                }
               //b_SYST_POWE = false;
            }else if(str_CMD == "showinfo"){
              m_showinfo();
            }else if(str_CMD == "showon"){
              b_SHOW_TEMP = true;
              Serial.println("Show temperature on"); 
            }else if(str_CMD == "showoff"){
              b_SHOW_TEMP = false;
              Serial.println("Show temperature off");
            }else if(str_CMD == "showeep"){
              m_showeep();
            }else if(str_CMD == "relayon"){
              m_relay_on();
            }else if(str_CMD == "relayoff"){
              m_relay_off();
            }

            if((cp = strcasestr(str_CMD.c_str(), "SETDATA")) != NULL){
              cp = strstr(cp, " ");
              cp++;
              strcat(cp, "@");
              m_setdata(cp);
              cp = "";
            }

            if((cp = strcasestr(str_CMD.c_str(), "SETEEP")) != NULL){
              cp = strstr(cp, " ");
              cp++;
              strcat(cp, "@");
              m_seteep(cp);
              cp = "";
            }
            str_CMD = "";                //一定要記得將s字串內容清空，否則會一直累加
        }
        delay(5);
    }
}
float m_show_working_time(unsigned char uc_tType){
  //show working time return value's unit 0:min 1:sec
  unsigned long ul_time_now = millis();
  float f_time = 0.0;
  ul_time_now = ul_time_now - ul_START_TIME;
  if(uc_tType == 0){
    f_time = ul_time_now / 1000.0;
    f_time /= 60;

  }else{
    f_time = ul_time_now / 1000.0;
    printf("Working Time = %f sec\n");
    
  } 
  return f_time;
}
//---------------------Test----------------------//
int serial_putc( char c, struct __file * ){
  Serial.write( c );
  return c;
}

void printf_begin(void){
  fdevopen( &serial_putc, 0 );
}

//-------------------20180724--------------------//
void m_seteep(char* cp){
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
    }else if(strcasecmp(cp_par, "mode") == 0){
      i_item = 3;
    }else if(strcasecmp(cp_par, "maxt") == 0){
      i_item = 4;
    }else if(strcasecmp(cp_par, "title") == 0){
      i_item = 5;
      Serial.println("i_item = 5");
    }

    cp_coin = strstr(cp, "@");
    cp_par = cp;
    *cp_coin = '\0';

    f_value = atof(cp_par);
    int i_tmp = 0;
    if(f_value >= 0){
      switch(i_item){
        //seteep temp
        case 1:
          m_set_temperature(f_value);
          i_tmp = (int)f_value;
          Serial.print("Temperature 0x13 = "); 
          Serial.println(i_tmp, HEX); 
          EEPROM.write(0x13, i_tmp);
          
          i_tmp = (int)((f_value - (int)f_value)*255);         
          Serial.print("Temperature 0x14 = "); 
          Serial.println(i_tmp, HEX);
          EEPROM.write(0x14, i_tmp);
          break;

        //seteep rang
        case 2:
          m_set_range(f_value);
          i_tmp = (int)(f_value*10);
          Serial.print("RANG 0x15 = "); 
          Serial.println(i_tmp, HEX); 
          EEPROM.write(0x15, i_tmp);
          break;
        //seteep mode
        case 3:
          uc_MODE = (unsigned char)f_value;
          Serial.print("uc_MODE 0x10 = "); 
          Serial.println(uc_MODE, HEX); 
          EEPROM.write(0x10, uc_MODE);
          if(uc_MODE == 1){
            ui_MAX_WORKING_MIN = 20;
          }else if(uc_MODE == 2){
            ui_MAX_WORKING_MIN = 60;
          }else if(uc_MODE == 3){
            ui_MAX_WORKING_MIN = 120;
          }
          break;

        //seteep maxt
        case 4:
          ui_MAX_WORKING_MIN = (unsigned int)f_value;
          i_tmp = ui_MAX_WORKING_MIN/0x100;
          Serial.print("ui_MAX_WORKING_MIN 0x11 = "); 
          Serial.println(i_tmp, HEX); 
          EEPROM.write(0x11, i_tmp);
          i_tmp = ui_MAX_WORKING_MIN%0x100;
          Serial.print("ui_MAX_WORKING_MIN 0x12= "); 
          Serial.println(i_tmp, HEX); 
          EEPROM.write(0x12, i_tmp);
          break;
        case 5:
          Serial.println("set title");
          char car_title[10] = "L-110-1100";
          for(int ix = 0; ix< 0xa; ix++){
            EEPROM.write(ix, car_title[ix]);
          }
          break;
      }
    }
}

void m_showeep(void){
  byte by_eep;
  for(int ix = 0; ix<EEPROM.length(); ix++){
      by_eep = EEPROM.read(ix);
      if(ix%0x10 == 0x0){
         Serial.print("[0x");
         Serial.print(ix, HEX);
         Serial.print("]");   
         Serial.println();
      } 
      if(by_eep < 0x10){
         Serial.print("0" + String(by_eep, HEX));
      }else{
         Serial.print(by_eep, HEX);
      }
      Serial.print("  ");      
      if(ix%0x10 == 0xf){
        Serial.println();
      }
  }
}

void m_ReadParameterFromEEP(void){
  f_TARG_TEMP = 0; 
  f_TEMP_RANG = 0;
   byte by_eep;
   //char *cp_0 = "L-110-0000";
   //Serial.print("address ");
   for(int ix = 0; ix<EEPROM.length(); ix++){
      switch(ix){
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
          str_IAM += (char)EEPROM.read(ix);
          break;
        case 0x10:
          uc_MODE = EEPROM.read(ix);
          Serial.print("uc_MODE = ");
          Serial.println(uc_MODE, DEC);
          break;
        case 0x11:
          ui_MAX_WORKING_MIN += ((int)EEPROM.read(ix))*256;                
          break;
        case 0x12:
          ui_MAX_WORKING_MIN += (int)EEPROM.read(ix);
          Serial.print("ui_MAX_WORKING_MIN = ");
          Serial.println(ui_MAX_WORKING_MIN, DEC);
          break;
        case 0x13:
          f_TARG_TEMP = 0;
          f_TARG_TEMP += (int)EEPROM.read(ix);
          break;
        case 0x14:
          f_TARG_TEMP += (float)EEPROM.read(ix)/256;
          Serial.print("f_TARG_TEMP = ");
          Serial.println(f_TARG_TEMP);          
          break;
        case 0x15:
          f_TEMP_RANG += ((float)EEPROM.read(ix))/10.0;
          Serial.print("f_TEMP_RANG = ");
          Serial.println(f_TEMP_RANG);  
          break;  
      }
   }
   Serial.print("I'm ");
   Serial.println(str_IAM);
   return;
}
//--------------------- JSON Output----------------------//
char* m_show_json_float(char* cp_title, float f_value, bool b_brackets){
  char car_show[255];
  char c_tmp = 0;
  char car_tmp[2];
  car_tmp[1] = '\0';


  memset(car_show, 0, sizeof(car_show));
  if(b_brackets == true){
    strcat(car_show, "{");
  }
  strcat(car_show, "\"");
  strcat(car_show, cp_title);
  strcat(car_show, "\":\"");

  if(f_value < 0){
    f_value *= -1;
    car_tmp[0] = '-';
    strcat(car_show, car_tmp);
  }

  int i_value = (int)f_value;     //此宣告需放在判斷正負號之後
  float f_v2 = f_value - i_value;

  
  if(i_value >= 10000){
    c_tmp = (i_value/10000 + '0');
    car_tmp[0] = c_tmp;
    strcat(car_show, car_tmp);
  }
 
  i_value%=10000;
  if(i_value >= 1000){
    c_tmp = (i_value/1000 + '0');
    car_tmp[0] = c_tmp;
    strcat(car_show, car_tmp); 
  }
  i_value%=1000;
  if(i_value >= 100){
    c_tmp = (i_value/100 + '0');
    car_tmp[0] = c_tmp;
    strcat(car_show, car_tmp);   
  }
  i_value%=100;

  if(i_value >= 10){
    c_tmp = (i_value/10 + '0');
    car_tmp[0] = c_tmp;
    strcat(car_show, car_tmp);  
  }
  i_value%=10;
  
  c_tmp = (i_value + '0');
  car_tmp[0] = c_tmp;
  strcat(car_show, car_tmp);

  //小數點
  car_tmp[0] = '.';
  strcat(car_show, car_tmp);

  for(int ix = 0; ix<2 ; ix++){
    f_v2 *= 10;
    i_value = (int)f_v2;
    i_value %= 10;
    
    c_tmp = (i_value + '0');
    car_tmp[0] = c_tmp;
    strcat(car_show, car_tmp);
  }
  
 
  strcat(car_show, "\"");
  if(b_brackets == true){
    strcat(car_show, "}");
  }
  return car_show;
}
char* m_show_json_int(char* cp_title, int i_value, bool b_brackets){
  char car_show[255];
  char c_tmp = 0;
  char car_tmp[2];
  car_tmp[1] = '\0';
  
  memset(car_show, 0, sizeof(car_show));
  if(b_brackets == true){
    strcat(car_show, "{");
  }  
  strcat(car_show, "\"");
  strcat(car_show, cp_title);
  strcat(car_show, "\":\"");

  if(i_value < 0){
    i_value *= -1;
    car_tmp[0] = '-';
    strcat(car_show, car_tmp);
  }
  
  if(i_value >= 10000){
    c_tmp = (i_value/10000 + '0');
    car_tmp[0] = c_tmp;
    strcat(car_show, car_tmp);
  }
 
  i_value%=10000;
  if(i_value >= 1000){
    c_tmp = (i_value/1000 + '0');
    car_tmp[0] = c_tmp;
    strcat(car_show, car_tmp); 
  }
  i_value%=1000;
  if(i_value >= 100){
    c_tmp = (i_value/100 + '0');
    car_tmp[0] = c_tmp;
    strcat(car_show, car_tmp);   
  }
  i_value%=100;

  if(i_value >= 10){
    c_tmp = (i_value/10 + '0');
    car_tmp[0] = c_tmp;
    strcat(car_show, car_tmp);  
  }
  i_value%=10;
  
  c_tmp = (i_value + '0');
  car_tmp[0] = c_tmp;
  strcat(car_show, car_tmp);
  strcat(car_show, "\"");
  if(b_brackets == true){
    strcat(car_show, "}");
  }

  return car_show;  
}
char* m_show_json_cp(char* cp_title, char* cp_value, bool b_brackets){
  char car_show[255];
  memset(car_show, 0, sizeof(car_show));
  if(b_brackets == true){
    strcat(car_show, "{");
  }
  strcat(car_show, "\"");
  strcat(car_show, cp_title);
  strcat(car_show, "\":\"");

  strcat(car_show, cp_value);
  strcat(car_show, "\"");
  if(b_brackets == true){
    strcat(car_show, "}");
  }
  return car_show;  
}

