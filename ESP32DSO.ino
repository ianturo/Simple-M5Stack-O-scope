
/*
 * Para usar este codigo es necesario una placa con 
 * procesador ESP32 y una pantalla ST7735
 * Es solo una prueba de concepto para quienes se 
 * encuentren aprendiendo sobre adruino o 
 * microcontroladores
 */

#include <Adafruit_GFX.h>   
#include <Adafruit_ST7735.h> 
#include <SPI.h>


#define AltoEncabezado 8 
#define TamTexto 1
#define FactorPuntosY 30
#define FactorPuntosX 40
#define PuntosTotal    1600
#define BotonA 39
#define BotonB 34

// Pines para Maple Mini
#define ST7735_CS    14
#define ST7735_RST   33
#define ST7735_DC    27
#define ST7735_SCLK  18 
#define ST7735_MOSI  23

#define AnalogPIN  2

Adafruit_ST7735 tft = Adafruit_ST7735(ST7735_CS,  ST7735_DC, ST7735_RST);


// position of the line on screen
int xPos = 0, Boton, BotonAnt, EstadoBA, EstadoBB, EstadoBAAnt, EstadoBBAnt;
int Puntos[PuntosTotal];
int PuntosA[PuntosTotal];
int intentos=0,Alto,Ancho,Ciclos, CiclosD,maxPoints=PuntosTotal;
float TiempoCiclo=0, TiempoCicloAnt=0,FactorPeriodo=10;
unsigned long TiempoCicloS,TiempoCicloE; 
float Vpp,VppAnt,DPS;
int Maximo=0,Minimo=4096,Arriba,Abajo;
int trigger=0,triggerant=0, PuntoIni=0, PuntoIniAnt=0;

float Frec,FreqR;
int DCup,DCdown;
 
/*******************************************************/
/*  Dibuja la cuadricula y los parametros basicos      */
/*******************************************************/
void drawgrid(){
  int i,j;
  trigger=Alto-(map(trigger, Minimo, Maximo, 0, Alto-(AltoEncabezado*2)))-AltoEncabezado;
  for(i=0;i<5;i++){
      for(j=0;j<100;j++)
        tft.drawPixel(i*(Ancho/5)+2,Alto/FactorPuntosY*(j+1),ST7735_WHITE);  
    }
  for(i=0;i<5;i++){
    for(j=0;j<150;j++)
      tft.drawPixel(Ancho/FactorPuntosX*(j+1),i*(Alto/5)+AltoEncabezado,ST7735_WHITE);  
  }
  tft.fillRect(0,0,Ancho,AltoEncabezado+1,ST7735_BLACK);
  tft.fillRect(0,Alto-AltoEncabezado,Ancho,AltoEncabezado,ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(0, 0);
  tft.printf("DC:%d% Vpp:%0.1f ",DCup,Vpp);
  tft.print(char(126));
  if(FreqR<1000)
    tft.printf("%0.1fHz",FreqR);
  else
    tft.printf("%0.1fKHz",FreqR/1000);
  tft.setCursor(0, Alto-AltoEncabezado-1);
  tft.printf("%0.2f msec",FactorPeriodo/0.5);
  if(trigger!=triggerant){
    for(int i=0;i<20;i++){
      tft.drawFastHLine(i*(Ancho/20)+1,triggerant,5,ST7735_BLACK);
      tft.drawFastHLine(i*(Ancho/20)+1,trigger,5,ST7735_BLUE);
    }
  }
}


/**************************/
/*      Dibuja los puntos */
/**************************/

void drawpoints(){
  for( xPos=0;xPos<maxPoints;xPos++) //Re-Map points according Min-Max;
    Puntos[xPos]=Alto-map(Puntos[xPos], Minimo, Maximo, 0, (Alto-(AltoEncabezado*2)))-AltoEncabezado;
  //tft.fillScreen(ST7735_BLACK);
  for(int i=0;i<Ancho;i++){
    tft.drawLine(i-1,PuntosA[i+PuntoIniAnt],i,PuntosA[i+PuntoIniAnt+1],ST7735_BLACK);
    tft.drawLine(i-1,Puntos[i+PuntoIni],i,Puntos[i+PuntoIni+1],ST7735_GREEN);
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
  //triggerant=trigger;
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
  FreqR=(Ciclos*100)/TiempoCiclo;
}



/**********************************************************************************************/
/*     Toma las muestras segun un aproximado de fecuencia y el tiempo medio de muestreo       */
/**********************************************************************************************/

void ControlMuestreo() {
  EstadoBA = digitalRead(BotonA);
  if (EstadoBA != EstadoBAAnt){
    delay(50);
    FactorPeriodo=FactorPeriodo/2.0;
    EstadoBAAnt = EstadoBA;
  }
  EstadoBB = digitalRead(BotonB);
  if (EstadoBB != EstadoBBAnt){
    delay(50);
    if(FactorPeriodo==0)
      FactorPeriodo=0.1;
    FactorPeriodo=FactorPeriodo*2.0;
    EstadoBBAnt = EstadoBB;
  }
  if(FactorPeriodo<0.1)
    FactorPeriodo=0;
  if(FactorPeriodo>5000)
    FactorPeriodo=5000;
}



void loop() {
  TiempoCicloAnt=TiempoCiclo;
  ControlMuestreo();
  DPS=30*FactorPeriodo; 
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
  TiempoCiclo=(TiempoCicloE-TiempoCicloS)/maxPoints*160.0/1000.0;    
  Analiza();
  drawpoints();

}

void setup() {
  pinMode(AnalogPIN, INPUT);
  pinMode(BotonA, INPUT);
  pinMode(BotonB, INPUT);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST7735_BLACK);
  Alto = tft.height();
  Ancho = tft.width();
  tft.setTextColor(ST7735_GREEN);
}
