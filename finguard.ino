#include "global.h"
typedef union{
  float f;
  uint8_t byte[4];
}FLOAT_VAL_t;

void Error_Handler(uint16_t code, uint16_t line,char* file){
  char str[80];
  uint8_t i=0;

  GPIO_WRITE(13, HIGH);
  GPIO_MODE(13,OUTPUT);
  GPIO_WRITE(12, HIGH);
  GPIO_MODE(12,OUTPUT);

  
  //enable UART , 9600 bits per seconds
  //send data through serial ports
  UCSR0B=	0x08; // enable TX
  UCSR0C = 0x06; //8-bit, async, 1 stop-bit,no parity
  UBRR0 = 103; //9600 baudrate @16Mhz

  sprintf(str,"ERROR: %x in line %u\ file %s\n", code, line, file);

  do{
    UDR0 = str[i]; //transmit str[i]
    i++; // point to next char is str
    while((UCSR0A & 0x20) == 0); //wait until UDRE0 bit is set(1)=0010 0000
  } while(str[i] != '\0'); // do while not NUL

  for(;;);
}

int main(void){
  uint16_t err_code;
  char System_str[]= "System Initialized\n";

  float TEMP_read=0;
  FLOAT_VAL_t TEMP_Celcius;
  uint32_t TEMP_x100;
  uint32_t TEMP_timeout=0;

  float TDS_ppm=0;
  FLOAT_VAL_t TDS_val;
  uint32_t TDS_x100;
  uint32_t TDS_timeout=0;

  float PH_lvl=0;
  FLOAT_VAL_t PH_val;
  uint32_t PH_x100;
  uint32_t PH_timeout = 0;
  

  uint32_t BLE_timeout=0;
  uint8_t  BLE_connected=0;
  uint8_t  BLE_connected_pre=0;
  char BLE_connected_str[] = "BLE_CONNECTED\n";
  char BLE_disconnected_str[] = "BLE_DISCONNECTED\n";

  uint8_t received_byte;

  char str[64];
  
  CHECK_ERROR_FATAL(TIMER2_init());
  sei();
  CHECK_ERROR_FATAL(TIMER1_init());
  CHECK_ERROR_FATAL(TIMER0_init());
  


  CHECK_ERROR_FATAL(ADC_init());
  CHECK_ERROR_FATAL(UART_init());
  CHECK_ERROR_FATAL(SWUART_init());
  CHECK_ERROR_FATAL(BLE_Init());
  
  CHECK_ERROR_FATAL(DS18B20_Init());
  CHECK_ERROR_FATAL(TDS_Init());
  CHECK_ERROR_FATAL(PH_Init());

  CHECK_ERROR_FATAL(UART_Write_String(System_str, strlen(System_str)));
  CHECK_ERROR_FATAL(DS18B20_RequestTemperature());
  TEMP_timeout = SYS_TICK;

  for(;;){
    //BLE_connection
    BLE_connected_pre = BLE_connected;
    CHECK_ERROR_FATAL(BLE_Connected(&BLE_connected));  
    if(BLE_connected != BLE_connected_pre){
      if(BLE_connected){
        CHECK_ERROR_FATAL(UART_Write_String(BLE_connected_str, strlen(BLE_connected_str)));
      }
      else{
        CHECK_ERROR_FATAL(UART_Write_String(BLE_disconnected_str, strlen(BLE_disconnected_str)));
      }
    }
     
     //Read Temperature Sensor
    if((SYS_TICK - TEMP_timeout) >=750){
      TEMP_timeout = SYS_TICK;
      CHECK_ERROR_FATAL(DS18B20_GetTemperatureC(&TEMP_read));    
      CHECK_ERROR_FATAL(DS18B20_RequestTemperature());
    }
     //Read Salinity Sensor
    if((SYS_TICK - TDS_timeout)>= 2000){
      TDS_timeout = SYS_TICK;
      CHECK_ERROR_FATAL(TDS_Read(&TDS_ppm,TEMP_read));
    }
     //Read PH sensor
    if((SYS_TICK - PH_timeout) >=1000) {
      PH_timeout = SYS_TICK;
      CHECK_ERROR_FATAL(PH_Read(&PH_lvl));
    }
     //BLE transfering
    if((SYS_TICK - BLE_timeout) >=3000){
      BLE_timeout=SYS_TICK;
      if(BLE_connected){
          str[0]= SENSOR_ID;
          str[1]= TEMP_ID;
          TEMP_Celcius.f=TEMP_read;
          str[2]= TEMP_Celcius.byte[0];
          str[3]= TEMP_Celcius.byte[1];
          str[4]= TEMP_Celcius.byte[2];
          str[5]= TEMP_Celcius.byte[3];
          str[6]= TDS_ID;
          TDS_val.f = TDS_ppm;
          str[7]= TDS_val.byte[0];
          str[8]= TDS_val.byte[1];
          str[9]= TDS_val.byte[2];
          str[10]= TDS_val.byte[3];
          str[11]= PH_ID;
          PH_val.f = PH_lvl;
          str[12]= PH_val.byte[0];
          str[13]= PH_val.byte[1];
          str[14]= PH_val.byte[2];
          str[15]= PH_val.byte[3];
          BLE_Write_String(str,16);
      }
      else{
        TEMP_x100 = (uint32_t)(TEMP_read * 100.0f);
        snprintf(str, sizeof(str), "Water Temperature  = %lu.%02lu C\n",TEMP_x100/100,TEMP_x100 % 100);
        CHECK_ERROR_FATAL(UART_Write_String(str,strlen(str)));
        TDS_x100 = (uint32_t)(TDS_ppm * 100.0f);
        snprintf(str, sizeof(str), "TDS  = %lu.%02lu ppm\n",TDS_x100/100,TDS_x100 % 100);
        CHECK_ERROR_FATAL(UART_Write_String(str,strlen(str)));
        PH_x100 = (uint32_t)(PH_lvl * 100.0f);
        snprintf(str, sizeof(str), "PH  = %lu.%02lu \n",PH_x100/100,PH_x100 % 100);
        CHECK_ERROR_FATAL(UART_Write_String(str,strlen(str))); 
      }
    }
  }

}
