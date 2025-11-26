#define TRIGGER_PIN 25
#define ECHO_PIN 26

const int ledPins[8] = {15, 0, 16, 5, 19, 21, 22, 23};

hw_timer_t *timer = NULL;

volatile unsigned long echoStartTime = 0;
volatile unsigned long echoEndTime = 0;
volatile unsigned long duration = 0;
volatile bool newMeasurement = false;

unsigned long lastMeasureTime = 0;
unsigned long lastLedChangeTime = 0;

unsigned long loopCount = 0;

float distance = 0.0;

int ledIndex = 0;

unsigned long ledInterval = 0;

bool triggerActive = false;

// ISR para detetar as transições do ECHO e medir o tempo do pulso
void IRAM_ATTR onEchoChange() {
  if (digitalRead(ECHO_PIN) == HIGH) {
    echoStartTime = micros();  // Início do pulso ECHO
  } else {
    echoEndTime = micros();    // Fim do pulso ECHO
    duration = echoEndTime - echoStartTime;
    newMeasurement = true;
  }
}

 // ISR do temporizador: alterna o estado do TRIGGER a cada 33334 us
 // A cada 2 chamadas completa-se um ciclo (~15Hz).
void IRAM_ATTR onTimer() {
  if (!triggerActive) {
    digitalWrite(TRIGGER_PIN, HIGH);   // Inicia trigger
    triggerActive = true;
  } else {
    digitalWrite(TRIGGER_PIN, LOW);    // Termina trigger
    triggerActive = false;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIGGER_PIN, LOW);

  for (int i = 0; i < 8; i++) pinMode(ledPins[i], OUTPUT);

  // Interrupção para medir o pulso de eco 
  attachInterrupt(digitalPinToInterrupt(ECHO_PIN), onEchoChange, CHANGE);

  // Configuração do temporizador de hardware
  timer = timerBegin(0, 80, true); // 80 MHz/80 = 1us por tick
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 33334, true);  // Chama a cada 33334 us
  timerAlarmEnable(timer);

  lastMeasureTime = micros();
  lastLedChangeTime = millis();
}

void loop() {
  loopCount++;

  if (newMeasurement) {
    newMeasurement = false;
    //Calculo da distância
    unsigned long dur = duration;
    distance = dur * 0.0343 / 2;

    // Calcula o intervalo entre transições de LED com base na distância:
    // Distância mínima (5 cm) => ledInterval = 50 ms (máxima velocidade)
    // Distância máxima (500 cm) => ledInterval = 5000 ms (mínima velocidade)
    ledInterval = constrain(map((int)distance, 5, 500, 50, 5000), 50, 5000);

    // Resultados para análise 
    Serial.printf("Distância: %.2f cm | ledInterval: %lums\n", distance, ledInterval);
  }

  // Avança o próximo Led com base no ledInterval
  unsigned long now = millis();
  if (now - lastLedChangeTime >= ledInterval) {
    lastLedChangeTime = now;

    // Apaga todos os LEDs
    for (int i = 0; i < 8; i++) digitalWrite(ledPins[i], LOW);
    // Acende apenas o LED atual
    digitalWrite(ledPins[ledIndex], HIGH);

    // Próximo LED
    ledIndex = (ledIndex + 1) % 8;
  }
}
