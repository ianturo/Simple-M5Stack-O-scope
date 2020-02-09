
/*
 * Para usar este codigo es necesario un M5Stack 
 * o un M5StickC.
 * Es solo una prueba de concepto para quienes se 
 * encuentren aprendiendo sobre adruino o 
 * microcontroladores
 */
 
#ifdef ARDUINO_M5Stick_C            /*Directivas para M5Core*/
  #include <M5StickC.h>
  #define AltoEncabezado 8 
  #define TamTexto 1
  #define FactorPuntosY 10
  #define FactorPuntosX 20
#endif
#ifdef ARDUINO_M5Stack_Core_ESP32   /*Directivas para M5StickC*/
  #include <M5Stack.h>   
  #define AltoEncabezado 14  
  #define TamTexto 2 
  #define FactorPuntosX 100
  #define FactorPuntosY 50
#endif
#include <SPI.h>


#define TriggerPIN     2   
#define AnalogPIN     36  
#define SpeedPIN       5  
#define PuntosTotal 1600

int xPos = 0;
int maxPoints=PuntosTotal;
int Puntos[PuntosTotal];
int PuntosA[PuntosTotal];
int intentos=0;
int Alto,Ancho;
int Ciclos, CiclosD;
float TiempoCiclo=0, TiempoCicloAnt=0,FactorPeriodo=10;
unsigned long TiempoCicloS,TiempoCicloE;
float Vpp,VppAnt,DPS;
int Maximo=0,Minimo=4096,Arriba,Abajo;
int trigger=0,triggerant=0, PuntoIni=0, PuntoIniAnt=0;

//int pinMapPB0 = PIN_MAP[AnalogPIN].adc_channel;

//const adc_dev *dev = PIN_MAP[AnalogPIN].adc_device;

float FreqR;
int DCup,DCdown;
 
 
/*******************************************************/
/*  Dibuja la cuadricula y los parametros basicos      */
/*******************************************************/
void drawgrid(){
  int i,j;
  trigger=Alto-(map(trigger, Minimo, Maximo, 0, Alto-(AltoEncabezado*2)))-AltoEncabezado;
  for(i=0;i<5;i++){
      for(j=0;j<100;j++)
        M5.lcd.drawPixel(i*(Ancho/5)+2,Alto/FactorPuntosY*(j+1),TFT_DARKGREY);  
    }
  for(i=0;i<5;i++){
    for(j=0;j<150;j++)
      M5.lcd.drawPixel(Ancho/FactorPuntosX*(j+1),i*(Alto/5)+AltoEncabezado,TFT_DARKGREY);  
  }
  M5.lcd.fillRect(0,0,Ancho,AltoEncabezado+1,TFT_BLACK);
  M5.lcd.fillRect(0,Alto-AltoEncabezado,Ancho,AltoEncabezado,TFT_BLACK);
  M5.lcd.setTextColor(TFT_WHITE);
  M5.lcd.setCursor(0, 0);
  M5.lcd.printf("DC:%d% Vpp:%0.1f ",DCup,Vpp);
  M5.lcd.print(char(126));
  if(FreqR<1000)
    M5.lcd.printf("%0.1fHz",FreqR);
  else
    M5.lcd.printf("%0.1fKHz",FreqR/1000);
  M5.lcd.setCursor(0, Alto-AltoEncabezado-1);
  M5.lcd.printf("%0.2f msec",FactorPeriodo/0.5);
  if(trigger!=triggerant){
    for(int i=0;i<20;i++){
      M5.lcd.drawFastHLine(i*(Ancho/20)+1,triggerant,5,TFT_BLACK);
      M5.lcd.drawFastHLine(i*(Ancho/20)+1,trigger,5,TFT_BLUE);
    }
  }
}


/**************************/
/*      Dibuja los puntos */
/**************************/

void drawpoints(){
  for( xPos=0;xPos<maxPoints;xPos++) //Re-Map points according Min-Max;
    Puntos[xPos]=Alto-map(Puntos[xPos], Minimo, Maximo, 0, (Alto-(AltoEncabezado*2)))-AltoEncabezado;
  //M5.lcd.fillScreen(TFT_BLACK);
  for(int i=0;i<Ancho;i++){
    M5.lcd.drawLine(i-1,PuntosA[i+PuntoIniAnt],i,PuntosA[i+PuntoIniAnt+1],TFT_BLACK);
    M5.lcd.drawLine(i-1,Puntos[i+PuntoIni],i,Puntos[i+PuntoIni+1],TFT_GREEN);
  }
  Serial.println("Termina dibujar puntos");
  drawgrid(); 
  Serial.println("Termina Dibujar Cuadricula");
  for( xPos=0;xPos<maxPoints;xPos++)
    PuntosA[xPos]=Puntos[xPos];
}

/************************************************************************************************/
/*  Analiza la los puntos para encontrar el punto de disparo, el Vpp y la frecuencia aproximada */
/************************************************************************************************/

void Analiza(){
  int sensor=0,sensorant=0, muestras=0;
  Maximo=0;
  Minimo=4096;
  for(int m=0;m<maxPoints;m++){
    if(Puntos[m]>Maximo )
      Maximo=Puntos[m];
    if(Puntos[m]<Minimo )
      Minimo=Puntos[m];
  }
  trigger=(Maximo+Minimo)/2;
  xPos=0;
  do{
    xPos++;
    muestras++;
  }while(!(Puntos[xPos] < trigger && Puntos[xPos+1]>=trigger ) && muestras<(maxPoints-Ancho));
  Arriba=0;
  Abajo=4096;
  DCup=0;
  DCdown=0;
  Ciclos=0;
  CiclosD=0;
  for(int m=0;m<maxPoints;m++){
    if(Puntos[m] < trigger && Puntos[m+1]>=trigger )
      Ciclos++;
    if(Puntos[m] > trigger && Puntos[m+1]<=trigger )
      CiclosD++;
    if(Puntos[m]>trigger)
      DCup++;
    if(Puntos[m]>trigger)
      DCdown++;
    if(Puntos[m]>Arriba)
      Arriba=Puntos[m];
    if(Puntos[m]<Abajo)
      Abajo=Puntos[m];
  }
  Vpp=(Arriba-Abajo) * 3.0 / (4096.0);
  Ciclos=(Ciclos+CiclosD)/2;
  DCup=DCup*100/maxPoints;
  DCdown=DCdown*100/maxPoints;
  PuntoIniAnt=PuntoIni;
  PuntoIni=muestras;
  FreqR=(Ciclos*1000000)/TiempoCiclo;
}


void setup() {
  Serial.begin(115200);
  pinMode(AnalogPIN, INPUT);
  M5.begin();
  M5.lcd.begin();
  #ifdef ARDUINO_M5Stick_C
    M5.lcd.setRotation(3);
  #endif
  M5.Lcd.setTextSize(TamTexto);
  M5.lcd.fillScreen(TFT_BLACK);
  Alto = M5.lcd.height();
  Ancho = M5.lcd.width();
  Serial.print("Alto:");
  Serial.print(Alto);
  Serial.print(" Ancho:");
  Serial.println(Ancho);
  noInterrupts();
}


/**********************************************************************************************/
/*     Toma las muestras segun un aproximado de fecuencia y el tiempo medio de muestreo       */
/**********************************************************************************************/
void ControlMuestreo() {
    if(M5.BtnA.wasPressed())
    FactorPeriodo=FactorPeriodo/2.0;
  if(FactorPeriodo<0.1)
    FactorPeriodo=0;
  if(M5.BtnB.wasPressed()){
    FactorPeriodo=FactorPeriodo*2.0;
    if(FactorPeriodo==0)
      FactorPeriodo=0.1;
  }
  if(FactorPeriodo>5000)
    FactorPeriodo=5000;

}

void loop() {
  triggerant=trigger;
  trigger=analogRead(TriggerPIN);
  TiempoCicloAnt=TiempoCiclo;
  ControlMuestreo();
// ----------------------------------- Captura de datos
  DPS=30*FactorPeriodo; //Delay por muestra
  if(FactorPeriodo>60)
    maxPoints=PuntosTotal/(FactorPeriodo/60);
  else
    maxPoints=PuntosTotal;
  if(FactorPeriodo){
    TiempoCicloS=micros();
    for(xPos=0;xPos<maxPoints;xPos++){
      Puntos[xPos]=analogRead(AnalogPIN);
      delayMicroseconds(DPS);
      }
  }
  else{
    TiempoCicloS=micros();
    for(xPos=0;xPos<maxPoints;xPos++){
      Puntos[xPos]=analogRead(AnalogPIN);
      }
  }
  TiempoCicloE=micros();
  TiempoCiclo=(TiempoCicloE-TiempoCicloS);    
  Analiza();
  drawpoints();
  M5.update();
}
