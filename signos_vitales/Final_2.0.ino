//-----------MAX30100
//Activar e incluir estas librerías para el max30100

#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
PulseOximeter pox; //Abreviando el "objeto"
//----------Sensor de sonido FC04LM393
int sensor = 3;
//----------Sensor de T° NTC de manera ANALÓGICA
/*
  El valor de temperatura es leido por un termistor de tipo NTC en configuracion de divisor de tension con resistencia de 10 K.
  Se aplica la ecuacion de "Steinhart-Hart" y sus coeficientes mediante: http://www.thinksrs.com/downloads/programs/Therm%20Calc/NTCCalibrator/NTCcalculator.htm
*/
int Vo;
float R1 = 10000;              // resistencia fija del divisor de voltaje
float logR2, R2, TEMPERATURA;
float c_1 = 2.108508173e-03, c_2 = 0.7979204727e-04, c_3 = 6.535076315e-07;

//-------OTRAS VARIABLES: -------------------------------------------------------
int pulsador = 4;
int x;
float t_x;
int estado = 1, bpm2, bpm1, c1 = 0, bpm_suma, c2, c3, spo2_2, spo2_1, son, c4=1, c5=1, son_ej, c6=0, c7=1, c8=1, bpm_rec;
int spo2_ej, spo2_rec, son_rec, soni, a, soni_2, L;
float t_1, t_2, t_3, t_4, t_5, t_6, t_7, t_8, t_9, t_10, t_11, t_12, t_13, t_14, t_15, t_16;
float bpm_final, spo2_final, temp_suma = 0, temp, temp_prom, bpm_ej, temp_ej, frec_resp=0, frec_resp_ej=0;
float frec_resp_rec, tiemp_rec, T_rec, frec_resp_actual,zz;
void setup()
{
  //-----------MAX30100 como pulsioxímetro (spo2 y bpm)-----------------
  Serial.begin(115200);
  Serial.print("Initializing pulse oximeter..");
  // Inicializar
  while (!pox.begin()) { // se analizará hasta que el sensor esté listo
    Serial.println("FAILED");
    delay(200);
  }
  Serial.println("INICIO EXITOSO");//empieza todo
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA); //Activa el infrarojo e inicia medidas

  //-----------FC04LM393 como sensor de frecuencia respiratoria (FR)----------
  pinMode (sensor, INPUT);
  //-----------LED, como actuador ante una alarma
  pinMode(11, OUTPUT);
  //-----------PULSADOR, para activar cuando culmine la rutina
  pinMode(pulsador, INPUT);

}

void loop() {
  pox.update();
  fsm_pdb1();
  zz=medir_bpm();
  //Serial.println(c1);
  /*son = medir_sonido();
  if (son == 1) {
    Serial.println(son);
    Serial.println(millis());
  }
*/
}
void fsm_pdb1()
{
  switch (estado)
  {
    case 1:
      Serial.println("estado 1");
      estado = 2;
      Serial.println("estado 2");
      break;

    case 2:
      bpm2 = zz;
      if (bpm2 != 0)
      {
        bpm1 = bpm2;
        c1 = 0;
        bpm_suma = 0;
        c2 = 0;
        c3 = 0;
        estado = 3;
        Serial.println("estado 3");
        Serial.println("Analizando signos vitales iniciales estables");
      }

      break;

    case 3:
      bpm2 = zz;
      if (c1 >= 5)
      {
        bpm1 = bpm2;
        estado = 4;
        
      }
      else if (bpm2 != bpm1) {
        bpm1 = bpm2;
        c1 = c1 + 1;
      }
      else {
        estado = 5;
        //Serial.println("estado 5");
      }
      break;

    case 4:
      bpm2 = zz;
      //Serial.println("bpm actual");
      //Serial.println(bpm2);
      
      if (c1 >= 8)
      {
        bpm_final = round(bpm_suma / 2); //Valor inicial de bpm
        spo2_2 = medir_spo2();
        spo2_final = (spo2_2 + spo2_1) / 2; //Valor inicial de spo2
        estado = 8;
        Serial.println("estado 8");
      }
      else if (bpm2 != bpm1) {
        bpm1 = bpm2;
        bpm_suma = bpm_suma + bpm2;
        c1 = c1 + 1;
        spo2_1 = medir_spo2();
      }
      else {
        estado = 5;
        //Serial.println("estado 5");
      }
      break;

    case 5:
      son = medir_sonido();
      if ((son == HIGH) && (c2 == 0)) {
        estado = 6;
        Serial.println("estado 6");
      }
      else {
        t_1 = millis();
        estado = 7;
        //Serial.println("estado 7");
      }
      break;

    case 6:
      c2 = 1;
      t_2 = millis();
      estado = 7;
      //Serial.println("estado 7");
      break;

    case 7:
      //Serial.println("Analisis de tiempo");
      if ((son == HIGH) && ((t_1 - t_2) > 1200) && (c2 == 1)) { //se toma como 1200 para evitar los otros highs en ese intervalo
        frec_resp = 60000 / (t_1 - t_2); //Valor inicial de frecuencia respiratoria
        t_2 = t_1;
        estado = 3;
        //Serial.println("estado 3");
      }
      else {
        estado = 3;
        //Serial.println("estado 3");
      }
      break;

    case 8:
      for (int i = 1; i < 10000; i++) { //10000 datos ya que los lee muy rápido
        temp = medir_T();
        temp_suma = temp_suma + temp;
      }
      temp_prom = temp_suma / 10000; //Valor inicial de Temperatura
      estado = 9;
      Serial.println("estado 9");
      break;

    case 9:
      if ((temp_prom > 37.2) || (spo2_final < 90) || (bpm_final > 100) || (frec_resp > 30)) { //parámetros fisiológicos fuera de rango para restringir el ejercicio
        estado = 10;
        Serial.println("estado 10");
      }
      else {
        t_3 = millis();
        estado = 11;
        Serial.println("estado 11");
      }
      break;

    case 10:
      Serial.println("La terapia se postpone, consulte con su médico");
      Serial.print("Temperatura en °C  ");
      Serial.println(temp_prom);
      Serial.print("% de saturación de oxígeno  ");
      Serial.println(spo2_final);
      Serial.print("Frecuencia cardíaca promedio  ");
      Serial.println(bpm_final);
      Serial.print("Frecuencia respiratoria promedio  ");
      Serial.println(frec_resp);
      while (1 == 1) {

      }
      break;

    case 11:
      Serial.println("Dentro de los parámetros adecuados, inicio de la terapia física");
      Serial.print("Temperatura en °C  ");
      Serial.println(temp_prom);
      Serial.print("% de saturación de oxígeno  ");
      Serial.println(spo2_final);
      Serial.print("Frecuencia cardíaca promedio  ");
      Serial.println(bpm_final);
      Serial.print("Frecuencia respiratoria promedio  ");
      Serial.println(frec_resp);
      /*Serial.println(medir_T());
      Serial.println(medir_spo2());
      Serial.println(zz);
      Serial.println(medir_frec_resp());
      
      bpm2 = zz;
      Serial.println("bpm actual");
      Serial.println(bpm2);
      */
      
      bpm_ej = zz;
      spo2_ej = medir_spo2();
      if ((millis() - t_3) >= 10000) {
        c4 = 1;
        c5 = 1;
        c6 = 0;
        c7 = 1;
        c8 = 1;
        
        estado=12;
        //Serial.println("estado 12");
      }
      while(1==1){
        
        }
      
      break;

    case 12:
      bpm_ej = medir_bpm();
      spo2_ej = medir_spo2();
      Serial.println(medir_bpm());
      Serial.println(c5);
      if (c4 == 1) {
        t_4 = millis();
      }
      if (c5 == 1) {
        t_5 = millis();
      }
      //Análisis de bpm durante el ejercicio
      if (bpm_ej > 100) {
        c4 = 0;
        t_15 = millis();
        estado = 19;
        Serial.println("estado 19");
      }
      else {
        c4 = 1;
        estado = 13;
        //Serial.println("estado 13");
      }
      
      //Análisis de spo2 durante el ejercicio
      if (medir_spo2() < 90) {
        c5 = 0;
        estado = 21;
        Serial.println("estado 21");
      }
      else {
        c5 = 1;
        estado = 13;
        //Serial.println("estado 13");
      }
      break;

    case 13:
      son_ej = medir_sonido();
      if ((son_ej == HIGH) && (c6 == 0)) {
        estado = 14;
        Serial.println("estado 14");
      }
      else {
        t_8 = millis();
        estado = 15;
        //Serial.println("estado 15");
      }
      break;

    case 14:
      c6 = 1;
      t_9 = millis();
      estado = 15;
      //Serial.println("estado 15");
      break;

    case 15:
      Serial.println("Análisis de tiempo");
      if ((son_ej == HIGH) && ((t_8 - t_9) > 1000) && (c6 == 1)) {
        estado = 18;
        Serial.println("estado 18");
      }
      else {
        estado = 16;
        //Serial.println("estado 16");
      }

      break;

    case 16:
      temp_ej = medir_T();
      if (c8 == 1) {
        t_12 = millis();
      }
      //Análisis de alarma de temperatura
      if (temp_ej > 37.2) {
        c8 = 0;
        estado = 25;
        Serial.println("estado 25");
      }
      else {
        c8 = 1;
        estado = 17;
        //Serial.println("estado 17");
      }
      break;

    case 17:
      //Ver si es que se analiza el tiempo de recuperación, se activará el pulsador cuando pase un tiempo determinado y se cumplan ciertas condiciones
      bpm_ej = medir_bpm();
      spo2_ej = medir_spo2();
      if (millis() > 10000) {
        estado = 27;
        Serial.println("estado 27");
      }
      else {
        estado = 12;
        //Serial.println("estado 12");
      }
      break;

    case 18:
      frec_resp_ej = 6000 / (t_8 - t_9);
      t_9 = t_8;
      if (c7 == 1) {
        t_10 = millis();
      }
      //Análisis de Alarma de frecuencia respiratoria
      if (frec_resp_ej > 30) {
        c7 = 0;
        estado = 23;
        Serial.println("estado 23");
      }
      else {
        c7 = 1;
        estado = 16;
        //Serial.println("estado 16");
      }
      break;

    case 19:
      t_6 = millis();
      //Activa la alarma, enciende led y se apaga todo
      if ((t_6 - t_4) > 6000) {
        estado = 20;
        Serial.println("estado 20");
      }
      else {
        estado = 13;
        //Serial.println("estado 13");
      }
      break;

    case 20:
      Serial.println("Se encuentra fuera del límite de BPM, se detiene el ejercicio");
      led_parpadeo();
      break;

    case 21:
      t_7 = millis();
      if (t_7 - t_5 > 6000) {
        estado = 22;
        Serial.println("estado 22");
      }
      else {
        estado = 13;
        //Serial.println("estado 13");
      }
      break;

    case 22:
      Serial.println("Se encuentra fuera del límite de spo2, se detiene el ejercicio");
      led_parpadeo();
      break;

    case 23:
      Serial.println("estado 23");
      t_11 = millis();
      if (t_11 - t_10 > 3000) {
        estado = 24;
        Serial.println("estado 24");
      }
      else {
        estado = 16;
        //Serial.println("estado 16");
      }

      break;

    case 24:
      Serial.println("Se encuentra fuera del límite de frecuencia respiratoria, se detiene el ejercicio");
      led_parpadeo();
      break;

    case 25:
      t_13 = millis();
      if (t_13 - t_12 > 3000) {
        estado = 26;
        Serial.println("estado 26");
      }
      else {
        estado = 17;
        //Serial.println("estado 17");
      }
      break;

    case 26:
      Serial.println("Se encuentra fuera del límite de temperatura, se detiene el ejercicio");
      led_parpadeo();
      break;

    case 27:
      x = digitalRead(pulsador);
      if (x == HIGH) {
        bpm_rec = medir_bpm();
        spo2_rec = medir_spo2();
        frec_resp_rec = frec_resp_ej;
        T_rec = medir_T();
        t_14 = millis();
        estado = 28;
        Serial.println("estado 28");
      }
      else {
        estado = 12;
        //Serial.println("estado 12");
      }

      break;

    case 28:
      //compararemos hasta que los valores se parezcan, se tomará un cierto margen de error, no se colocará la T° para el tiempo de recuperación
      //Serial.println("estado 28");
      if ((medir_bpm() <= bpm_final + 2) && (medir_frec_resp() <= frec_resp + 2) && (medir_spo2() >= spo2_final - 1) && (medir_spo2() <= spo2_final + 1)) {
        tiemp_rec = millis() - t_14;
        estado = 29;
        Serial.println("estado 29");
      }
        
        Serial.println(medir_bpm());
        Serial.println(medir_frec_resp());
        Serial.println(medir_spo2());
      break;

    case 29:
      //Tiempo de recuperación
      Serial.println(tiemp_rec);
      //SVo
      Serial.println(bpm_final);
      Serial.println(frec_resp);
      Serial.println(spo2_final);
      Serial.println(temp_prom);
      //SV
      Serial.println(bpm_rec);
      Serial.println(spo2_rec);
      Serial.println(frec_resp_rec);
      Serial.println(T_rec);
      while (1 == 1) {

      }

      break;

  }

}

//Medición de la frecuencia cardíaca
int medir_bpm()
{
  pox.update();
  return pox.getHeartRate();

}
//Medición de la saturación de oxígeno
int medir_spo2()
{
  pox.update();
  return pox.getSpO2();
}

//Medición de sonidos HIGH o LOW que ayudarán al cálculo de la frecuencia respiratoria, 
//artificio: si sale un HIGH, será verdadero si se mantiene por 15 veces seguidas, se 
//evitan ruidos externos o picos de golpes
int medir_sonido()//brinda HIGH o LOW
{
  soni = digitalRead(sensor);
  if (soni == HIGH) 
  {
    a = 0;
    //Serial.println(millis());
    for (int j = 0; j < 30; j++) 
    {
      soni_2 = digitalRead(sensor);
      //Serial.println(soni_2);
      if ( soni_2 == HIGH) 
      {
        a = a + 1;
      }
      delay(1);
      //Serial.println(a);
    }
    //Serial.println(a);
    if (a >= 15) 
    {
          //Serial.println("cvbyuncvgbhjn");
          //Serial.println(millis());
          L = HIGH;
    }
    else 
    {
          L = LOW;
         
    }

  }
  else 
  {
    L = LOW;
  }
  //Serial.println(a);
  a=0;
  return L;


}
//Medición de la temperatura corporal
float medir_T()
{
  Vo = analogRead(A0);      // lectura de A0
  R2 = R1 * (1023.0 / (float)Vo - 1.0); // equivalencia de la R del NTC a partir de la R1 y voltaje leído
  logR2 = log(R2);      // logaritmo de R2 necesario para ecuacion de "Steinhart-Hart" (S-H)
  TEMPERATURA = (1.0 / (c_1 + c_2 * logR2 + c_3 * logR2 * logR2 * logR2)); // ecuacion S-H
  TEMPERATURA = (TEMPERATURA - 273.15)*36.00/32.00;   // Kelvin a Centigrados (Celsius)
  return TEMPERATURA;
}

//Medición de la frecuencia respiratoria en caso no se use en modo paralelo para todos los sensores
float medir_frec_resp()
{
  son_rec = medir_sonido();
  if (son_rec == HIGH) {
    t_16 = millis();
    while ((millis() - t_16) < 1200) {

    }
    while (medir_sonido() == LOW) {

    }
    frec_resp_actual = 60000 / (millis() - t_16);
    return frec_resp_actual;

  }

}
//Parpadeo de led cuando se activa una alarma y no dejará de parpadear
void led_parpadeo()
{
  t_x = millis();
  while (1 == 1)
  {
    if (millis() - t_x < 100) {
      digitalWrite(11, HIGH);
    }
    else {
      digitalWrite(11, LOW);
      if (millis() - t_x > 200)
      {
        t_x = millis();
        Serial.println(t_x);
      }
    }
  }
}
