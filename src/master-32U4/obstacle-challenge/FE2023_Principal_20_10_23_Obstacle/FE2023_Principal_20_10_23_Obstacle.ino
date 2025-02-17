#include "definiciones.h"


bool robotActivo = false;
int errores = 0;
byte speed = 20;
unsigned long buenaTramaA = 0;
unsigned long errorTramaA = 0;
unsigned long buenaTramaB = 0;
unsigned long errorTramaB = 0;
bool estadoFreno = 1;
long distanciaBT;
long tiempoLED;
int angPID = 0;
bool activePID = true;

Servo direccion;
MPU6050 mpu(Wire);

void setup(void) {
  direccion.attach(DIR);
  pinMode(BTN, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  setupSPI();
  digitalWrite(LED, 1);
  digitalWrite(BUZZ, 1);
  direccion.write(90 + offsetMPU);
  delay(100);
  digitalWrite(BUZZ, 0);
  setupMPU();
  tiempoLED = millis() + 500;
  while ((i2c.TOF[0] == 0) or (i2c.TOF[1] == 0)) {
    updateMPU();
    updateSPI();
    updateLed();
  }
  digitalWrite(BUZZ, 1);
  delay(50);
  digitalWrite(BUZZ, 0);
  delay(50);
  digitalWrite(BUZZ, 1);
  delay(50);
  digitalWrite(BUZZ, 0);
  updateMPU();
  resetCarAngle();
  setDireccionCamara(90);
  activarHL();
  activarUS("F");
  while (digitalRead(BTN)) {
    updateRobot();
    if (verdeCerca()) intermitente('I');
    else if (rojoCerca()) intermitente('D');
    else intermitente('A');
  }
  intermitente('A');
  delay(1000);
  updateMPU();
  resetCarAngle();
  moveInicio();
  bool vueltaLectura = true; 
  for (uint8_t i = 0; i < 2; i++) {
    if (sentidoRonda == 1) moveSector(90, sectorRonda[1], sentidoRonda, 1, vueltaLectura);
    else moveSector(270, sectorRonda[1], sentidoRonda, 1, vueltaLectura);
    moveSector(180, sectorRonda[2], sentidoRonda, 2, vueltaLectura);
    if (sentidoRonda == 1) moveSector(270, sectorRonda[3], sentidoRonda, 3, vueltaLectura);
    else moveSector(90, sectorRonda[3], sentidoRonda, 3, vueltaLectura);
    if (i == 0) {
      moveSector(0, sectorRonda[0], sentidoRonda, 0, false);
      vueltaLectura = false;
      vuelta++;
    }
  }
  byte ultimoBloque = bloqueCerca(sectorRonda[0]);
  if (ultimoBloque == 0) {
    ultimoBloque = mirarUltimoBloque(sectorRonda[3]);
    if (ultimoBloque == 1) {  // Ultimo Bloque Verde
      moveSector(0, sectorRonda[0], sentidoRonda, 0, false);
      vuelta++;
      if (sentidoRonda == 1) moveSector(90, sectorRonda[1], sentidoRonda, 1, false);
      else moveSector(270, sectorRonda[1], sentidoRonda, 1, false);
      moveSector(180, sectorRonda[2], sentidoRonda, 2, false);
      if (sentidoRonda == 1) moveSector(270, sectorRonda[3], sentidoRonda, 3, false);
      else moveSector(90, sectorRonda[3], sentidoRonda, 3, false);
      moveSector(0, sectorRonda[0], sentidoRonda, 0, false);
      robotStop();
    }
    else {
      for (uint8_t i = 0; i < sizeof(sectorRonda); i++) {
        if (sectorRonda[i] == 1) sectorRonda[i] = 5;
        else if (sectorRonda[i] == 2) sectorRonda[i] = 6;
        else if (sectorRonda[i] == 5) sectorRonda[i] = 1;
        else if (sectorRonda[i] == 6) sectorRonda[i] = 2;
        else if (sectorRonda[i] == 8) sectorRonda[i] = 10;
        else if (sectorRonda[i] == 10) sectorRonda[i] = 8;
      }
      giroEnlace(distAnterior, 550, 0, sentidoRonda);
      moveDistancia(500, vMinSector, vMinSector);
      while (!commandFinished) updateRobot();
      if (sentidoRonda == 1) {
        intermitente('D');
        setVelocidad(10);
        activePID = false;
        motorResetCount("DI");
        while (abs(abs(getCarAngle()) - 180) > 10) {
          pidCurva(250, 2, 0, 0);
          updateRobot();
        }
        angPID = 180;
        activePID = true;
        dirAnteriorCurva = 180;
        moveDistancia(100, vMinSector, vMinSector);
        while (!commandFinished) updateRobot();
        while (i2c.TOF[0] < mediaTOFpared) updateRobot();
        distAnterior = 200;
        vuelta++;
        sentidoRonda = -1;
        moveSector(90, sectorRonda[3], sentidoRonda, 3, false);
        moveSector(0, sectorRonda[2], sentidoRonda, 2, false);
        moveSector(270, sectorRonda[1], sentidoRonda, 1, false);
        moveSector(180, sectorRonda[0], sentidoRonda, 0, false);
      }
      else {
        intermitente('I');
        setVelocidad(10);
        activePID = false;
        motorResetCount("DI");
        while (abs(abs(getCarAngle()) - 180) > 10) {
          pidCurva(-250, 2, 0, 0);
          updateRobot();
        }
        angPID = 180;
        activePID = true;
        dirAnteriorCurva = 180;
        moveDistancia(100, vMinSector, vMinSector);
        while (!commandFinished) updateRobot();
        while (i2c.TOF[1] < mediaTOFpared) updateRobot();
        distAnterior = 200;
        vuelta++;
        sentidoRonda = 1;
        moveSector(270, sectorRonda[3], sentidoRonda, 3, false);
        moveSector(0, sectorRonda[2], sentidoRonda, 2, false);
        moveSector(90, sectorRonda[1], sentidoRonda, 1, false);
        moveSector(180, sectorRonda[0], sentidoRonda, 0, false);
      }
    }
  }
  else if (ultimoBloque == 2) {  // Last obstacle red
    int sectorCambio = sectorRonda[0];
    for (uint8_t i = 0; i < sizeof(sectorRonda); i++) {
      if (sectorRonda[i] == 1) sectorRonda[i] = 5;
      else if (sectorRonda[i] == 2) sectorRonda[i] = 6;
      else if (sectorRonda[i] == 5) sectorRonda[i] = 1;
      else if (sectorRonda[i] == 6) sectorRonda[i] = 2;
      else if (sectorRonda[i] == 8) sectorRonda[i] = 10;
      else if (sectorRonda[i] == 10) sectorRonda[i] = 8;
    }
    if (sentidoRonda == 1) {
      giroEnlace(distAnterior, 0, 0, sentidoRonda);
      intermitente('I');
      moveDistancia(200, vMinSector, vMinSector);
      while (!commandFinished) updateRobot();
      if (sectorCambio == 4) {
        while (i2c.TOF[0] > mediaTOFbloque) updateRobot();
        moveDistancia(200, vMinSector, vMinSector);
        while (!commandFinished) updateRobot();
        setVelocidad(10);
        activePID = false;
        motorResetCount("DI");
        while (abs(abs(getCarAngle()) - 90) > 10) {
          pidCurva(-150, 2, 0, 0);
          updateRobot();
        }
        angPID = 270;
        activePID = true;
        dirAnteriorCurva = -90;
        moveDistancia(100, vMinSector, vMinSector);
        while (!commandFinished) updateRobot();
        setVelocidad(10);
        activePID = false;
        motorResetCount("DI");
        while (abs(abs(getCarAngle()) - 180) > 10) {
          pidCurva(-150, 2, 0, 0);
          updateRobot();
        }
        angPID = 180;
        activePID = true;
        dirAnteriorCurva = 180;
        moveDistancia(450, vMinSector, vMinSector);
        while (!commandFinished) updateRobot();
      }
      else {
        setVelocidad(10);
        activePID = false;
        motorResetCount("DI");
        while (abs(abs(getCarAngle()) - 180) > 10) {
          pidCurva(-250, 2, 0, 0);
          updateRobot();
        }
        angPID = 180;
        activePID = true;
        dirAnteriorCurva = 180;
        moveDistancia(100, vMinSector, vMinSector);
        while (!commandFinished) updateRobot();
        while (i2c.TOF[0] > mediaTOFbloque) updateRobot();
      }
      distAnterior = 800;
      vuelta++;
      sentidoRonda = -1;
      moveSector(90, sectorRonda[3], sentidoRonda, 3, false);
      moveSector(0, sectorRonda[2], sentidoRonda, 2, false);
      moveSector(270, sectorRonda[1], sentidoRonda, 1, false);
      moveSector(180, sectorRonda[0], sentidoRonda, 0, false);
    }
    else {
      giroEnlace(distAnterior, 550, 0, sentidoRonda);
      intermitente('I');
      moveDistancia(200, vMinSector, vMinSector);
      while (!commandFinished) updateRobot();
      if (sectorCambio == 4) {
        while (i2c.TOF[0] > mediaTOFbloque) updateRobot();
        moveDistancia(200, vMinSector, vMinSector);
        while (!commandFinished) updateRobot();
        setVelocidad(10);
        activePID = false;
        motorResetCount("DI");
        while (abs(abs(getCarAngle()) - 90) > 10) {
          pidCurva(-150, 2, 0, 0);
          updateRobot();
        }
        angPID = 270;
        activePID = true;
        dirAnteriorCurva = -90;
        moveDistancia(100, vMinSector, vMinSector);
        while (!commandFinished) updateRobot();
        setVelocidad(10);
        activePID = false;
        motorResetCount("DI");
        while (abs(abs(getCarAngle()) - 180) > 10) {
          pidCurva(-150, 2, 0, 0);
          updateRobot();
        }
        angPID = 180;
        activePID = true;
        dirAnteriorCurva = 180;
        moveDistancia(450, vMinSector, vMinSector);
        while (!commandFinished) updateRobot();
      }
      else{
        setVelocidad(10);
        activePID = false;
        motorResetCount("DI");
        while (abs(abs(getCarAngle()) - 180) > 10) {
          pidCurva(-250, 2, 0, 0);
          updateRobot();
        }
        angPID = 180;
        activePID = true;
        dirAnteriorCurva = 180;
        moveDistancia(100, vMinSector, vMinSector);
        while (!commandFinished) updateRobot();
        while (i2c.TOF[1] < mediaTOFpared) updateRobot();
      }
      distAnterior = 200;
      vuelta++;
      sentidoRonda = 1;
      moveSector(270, sectorRonda[3], sentidoRonda, 3, false);
      moveSector(0, sectorRonda[2], sentidoRonda, 2, false);
      moveSector(90, sectorRonda[1], sentidoRonda, 1, false);
      moveSector(180, sectorRonda[0], sentidoRonda, 0, false);
    }
  }
  else {  // Last obstacle green
    moveSector(0, sectorRonda[0], sentidoRonda, 0, false);
    vuelta++;
    if (sentidoRonda == 1) moveSector(90, sectorRonda[1], sentidoRonda, 1, false);
    else moveSector(270, sectorRonda[1], sentidoRonda, 1, false);
    moveSector(180, sectorRonda[2], sentidoRonda, 2, false);
    if (sentidoRonda == 1) moveSector(270, sectorRonda[3], sentidoRonda, 3, false);
    else moveSector(90, sectorRonda[3], sentidoRonda, 3, false);
    moveSector(0, sectorRonda[0], sentidoRonda, 0, false);
    robotStop();
  }
  robotStop();
}

void loop(void) {
  updateRobot();
}

void updateRobot() {
  updateMPU();
  if (activePID) pidGyro(angPID, 2, 0, 0);
  enviarSPIA();
  enviarSPIB();
  updateLed();
  updateBeep();
}

void moveInicio() {
  motorStart(7);
  desactivarHL();
  if (verdeCerca()) {
    intermitente('I');
    while (abs(getCarAngle()) < 70) {
      updateMPU();
      enviarSPIA();
      enviarSPIB();
      updateLed();
      updateBeep();
      pidGyro(265, 2, 0, 0);
    }
    intermitente('D');
    while (abs(getCarAngle()) > 5 or i2c.TOF[1] > mediaTOFbloque) {
      updateMPU();
      enviarSPIA();
      enviarSPIB();
      updateLed();
      updateBeep();
      pidGyro(0, 2, 0, 0);
    }
    intermitente('A');
    distAnterior = 800;
  } else if (rojoCerca()) {
    intermitente('D');
    while (abs(getCarAngle()) < 70) {
      updateMPU();
      enviarSPIA();
      enviarSPIB();
      updateLed();
      updateBeep();
      pidGyro(90, 2, 0, 0);
    }
    intermitente('I');
    while (abs(getCarAngle()) > 5 or i2c.TOF[0] > mediaTOFbloque) {
      updateMPU();
      enviarSPIA();
      enviarSPIB();
      updateLed();
      updateBeep();
      pidGyro(0, 2, 0, 0);
    }
    intermitente('A');
    distAnterior = 200;
  }
  else {
    commandB.distBloqueCerca = 70;
    commandB.distBloqueLejos = 120;
    distAnterior = 500;
  }
  while (i2c.TOF[0] < mediaTOF0direccion and i2c.TOF[1] < mediaTOF1direccion) updateRobot();
  beep(100);
  activarHL();
  motorBreak(-15);
  bool sentidoLeido = false;
  long tiempo = millis() + 100;
  bool motorEncendido = false;
  while (!sentidoLeido) {
    updateRobot();
    if (i2c.TOF[1] > mediaTOF1direccion) {
      intermitente('D');
      sentidoRonda = 1;
      sentidoLeido = true;
    }
    else if (i2c.TOF[0] > mediaTOF0direccion) {
      intermitente('I');
      sentidoRonda = -1;
      if (distAnterior == 800) distAnterior = 200;
      else if (distAnterior == 200) distAnterior = 800;
      sentidoLeido = true;
    }
    else if (millis() > tiempo and !motorEncendido) {
      motorStart(5);
      motorEncendido = true;
    }
  }
  motorStartPower(0);
  sectorRonda[1] = leerSector();
  desactivarHL();
  moveDistancia(50, 10, 10);
  while (!commandFinished) updateRobot();
}

byte bloqueCerca(byte sector) {
  if (sector == 1 or sector == 3 or sector == 7 or sector == 8) return 1;
  else if (sector == 2 or sector == 4 or sector == 9 or sector == 10) return 2;
  else return 0;
}

byte mirarUltimoBloque(byte sector) {
  if (sector == 1) return 1;
  else if (sector == 2) return 2;
  else if (sector == 3) return 1;
  else if (sector == 4) return 2;
  else if (sector == 5) return 1;
  else if (sector == 6) return 2;
  else if (sector == 7) return 1;
  else if (sector == 8) return 2;
  else if (sector == 9) return 2;
  else if (sector == 10) return 1;
  else if (sector == 11) return 1;
}

void robotStop() {
  motorStartPower(0);
  intermitente('E');
  luzFreno(true);
  while (1) updateRobot();
}
