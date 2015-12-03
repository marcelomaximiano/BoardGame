#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "Bounce2.h"

// ***** LCD
#define BACKLIGHT_PIN 5
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address
#include "LedControlMS.h"
#define DataIn_Pin 4
#define Clock_Pin  7
#define Load_Pin   8

// ***** Leds
LedControl lc = LedControl(DataIn_Pin, Clock_Pin, Load_Pin, 1);
#define ledButtonA    17
#define ledButtonB    18
#define ledButtonC    19
#define ledWhite1     20
#define ledWhite2     21
#define ledSnakeRed   22
#define ledSnakeGreen 23

// ***** Buttons
#define buttonRot  A0
#define buttonA    A1
#define buttonB    A2
#define buttonC    A3
#define snakeWire  9
#define snakeStart 10
#define snakeEnd   11
Bounce debouncerRot = Bounce();
Bounce debouncerA   = Bounce();
Bounce debouncerB   = Bounce();
Bounce debouncerC   = Bounce();
Bounce debouncerSW  = Bounce();
Bounce debouncerS1  = Bounce();
Bounce debouncerS2  = Bounce();

// buttons events
#define evtBtnRot    0
#define evtBtnA      1
#define evtBtnB      2
#define evtBtnC      3
#define evtSW        4
#define evtS1Press   5
#define evtS1Release 6
#define evtS2Press   7
#define evtS2Release 8

// ***** Rotary Encoder
int Rot_Pin1 = 12;
int Rot_Pin2 = 2;
int encoderValue = 1;
int lastEncoded = LOW;

// ***** Sound Volume
int soundVolume = 0;  // max 31

// ***** Acentos para mensagens gerais
char strAccents[11] = "          ";

void setup()
{
  Serial.begin(9600);
  Wire.begin();

  uint32_t seed = millis();
  randomSeed(seed);

  lc.shutdown(0, false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0, 12);
  /* and clear the display */
  lc.clearDisplay(0);

  // initialize Rotary Encoder
  pinMode (Rot_Pin1, INPUT);
  pinMode (Rot_Pin2, INPUT);

  // inicializa os botoes
  initButton(buttonRot);
  initButton(buttonA);
  initButton(buttonB);
  initButton(buttonC);
  initButton(snakeWire);
  initButton(snakeStart);
  initButton(snakeEnd);

  // Switch on the backlight
  pinMode (BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);
  lcd.begin(20, 4);              // initialize the lcd
  lcd.clear();
  lcd.setCursor(0, 1);
  displayRomStr(10020);     // O Segredo
  loadChars(strAccents);
  lcd.setCursor(0, 2);
  displayRomStr(10040);     // da Piramide
  delay(3000);
}

// Timers
unsigned long timer1 = 0;
unsigned long timer1_interval = 1000;
unsigned long timer2 = 0;
unsigned long timer2_interval = 300;
unsigned long timer3 = 0;
unsigned long timer3_interval = 20000;
unsigned long timer4 = 0;
unsigned long timer4_interval = 30000;

// etapas do jogo
#define HowManyPlayers 1
#define Begin          2
#define Roulette       3
#define GameOver       4

bool gameOver = false;

int NumberPlayers = 2;
int buttonPressed = -1;

char* players[]   = { "vermelho", "azul", "marrom", "amarelo", "branco" };
int rouletteVal[] = { 5, 4, 0, 3, 2, 1, 3, 6, 1, 3, 2, 4, -1, 0, 6, 1 };
int playersPos[]  = { 0, 0, 0, 0, 0 };
int playersPenalty[]  = { 0, 0, 0, 0, 0 };

int currentPlayer = 0;
int roulettePos = 1;

// niveis de dificuldade das perguntas
#define questionLevel01 1
#define questionLevel02 1
#define questionLevel03 1

void loop()
{
  unsigned long currentMillis = millis();

  // numero de jogadores
  ligaLed(ledButtonA, true);
  ligaLed(ledButtonB, true);
  ligaLed(ledButtonC, true);

  digitalWrite ( BACKLIGHT_PIN, HIGH );
  loadChars(strAccents);
  lcd.clear();
  lcd.setCursor(0, 0);
  displayRomStr(10100);     // Numero de jogadores
  lcd.setCursor(0, 1);
  char str[10];
  sprintf(str, "%1d", NumberPlayers);
  lcd.write(str);
  lcd.setCursor(9, 1);
  lcd.write("A) +");
  lcd.setCursor(9, 2);
  lcd.write("B) -");
  lcd.setCursor(9, 3);
  lcd.write("C) confirma");
  buttonPressed = -1;

  while (true)
  {
    buttonPressed = getButton();

    if (buttonPressed == evtBtnA) {
      NumberPlayers++;
      if (NumberPlayers > 5) {
        NumberPlayers = 5;
      }
    }
    if (buttonPressed == evtBtnB) {
      NumberPlayers--;
      if (NumberPlayers < 2) {
        NumberPlayers = 2;
      }
    }

    if (buttonPressed != -1) {
      lcd.setCursor(0, 1);
      char str[10];
      sprintf(str, "%1d", NumberPlayers);
      lcd.write(str);
    }

    if (buttonPressed == evtBtnC) {
      uint32_t seed = millis();
      randomSeed(seed);
      lcd.clear();
      ligaLed(ledButtonA, false);
      ligaLed(ledButtonB, false);
      ligaLed(ledButtonC, false);
      displayPlayers();
      waitConfirm(3000);
      displayPressToBegin();
      game();
      break;
    }
    buttonPressed = -1;
  }
}

void displayPressToBegin() {
  digitalWrite(BACKLIGHT_PIN, HIGH);

  ligaLed(ledButtonA, false);
  ligaLed(ledButtonB, false);
  ligaLed(ledButtonC, false);

  lcd.clear();
  lcd.setCursor(0, 0);
  displayRomStr(10060);     // Pressione C para
  lcd.setCursor(0, 1);
  displayRomStr(10080);     // iniciar a partida

  waitConfirm(0);
}

void game() {
  int rotV = lastEncoded;

  // reinicializa a posição dos jogadores
  for (int i = 0; i <= 4; i++) {
    playersPos[i] = 0;
    playersPenalty[i]  = 0;
  }
  currentPlayer = 0;
  gameOver = false;

  while (true)
  {
    turnOffAllLeds();
    ligaLed(ledButtonA, false);
    ligaLed(ledButtonB, false);
    ligaLed(ledButtonC, true);

    digitalWrite(BACKLIGHT_PIN, HIGH);
    lcd.clear();
    displayPlayer();
    lcd.setCursor(0, 1);
    lcd.write("Gire a roleta!");
    ligaLed(roulettePos, true);

    while (true)
    {
      // **********  Rotary Encoder
      readRotaryEncoder();
      if (rotV != lastEncoded) {
        ringSeq(encoderValue);
      }

      buttonPressed = getButton();

      if ((buttonPressed == evtBtnRot) || (buttonPressed == evtBtnC)) {
        loadChars(strAccents);
        lcd.clear();
        ligaLed(ledButtonA, false);
        ligaLed(ledButtonB, false);
        ligaLed(ledButtonC, false);
        spinRoulette();

        lcd.clear();
        lcd.setCursor(0, 0);
        displayPlayer();

        if (roulettePos < 1) roulettePos = 1;
        if (roulettePos > 16) roulettePos = 16;
        
        int rv = rouletteVal[roulettePos - 1];

        if (rv == -1) {
          playersPos[currentPlayer] = 0;
          lcd.setCursor(0, 1);
          displayRomStr(10180);     // Volte ao inicio!
        }
        else if (rv == 0) {
          lcd.setCursor(0, 1);
          displayRomStr(10200);     // Fique na mesma casa!
        }
        else {
          playersPos[currentPlayer] = playersPos[currentPlayer] + rv;
          if (playersPos[currentPlayer] > 54) {
            playersPos[currentPlayer] = 55;
          }

          lcd.setCursor(0, 1);
          displayRomStr(10220);     // Avance X casa
          char str[10];
          sprintf(str, "%1d", rv);
          lcd.setCursor(7, 1);
          lcd.write(str);
          if (rv > 1) {
            lcd.setCursor(13, 1);
            lcd.write("s");
          }

          waitConfirm(2000);

          switch (playersPos[currentPlayer]) {
            case 5: case 16: case 22: case 31: case 42: case 47: case 52:
              displayGetCard();
              break;
            case 8: case 14:
              snake(50);
              break;
            case 27: case 37:
              snake(40);
              break;
            case 44: case 50:
              snake(30);
              break;
            case 20:
              delay(2000);
              playersPos[currentPlayer] = 12;
              lcd.setCursor(0, 1);
              displayRomStr(10240);     // Volte para a casa 12
              break;
            case 25:
              delay(2000);
              playersPos[currentPlayer] = 33;
              lcd.setCursor(0, 1);
              displayRomStr(10260);     // Avance p/ a casa 33
              break;
          }

          if (playersPos[currentPlayer] == 55) {
            delay(2000);
            lcd.clear();
            lcd.setCursor(0, 0);
            displayRomStr(10280);     // Parabens!!!
            lcd.setCursor(0, 1);
            displayPlayer();
            lcd.setCursor(0, 2);
            displayRomStr(10300);     // Venceu o jogo !!!
            ligaLed(ledWhite1, true);
            ligaLed(ledWhite2, true);
            gameOver = true;
            delay(5000);
            displayGameOver();
            break;
          }
        }
        waitConfirm(3000);
        displayPos();
        waitConfirm(3000);

        currentPlayer++;
        if (currentPlayer > NumberPlayers - 1) {
          currentPlayer = 0;
        }
        break;
      }
    }
    if (gameOver) {
      break;
    }
  }
  buttonPressed = -1;
}

void snake(int time) {
  loadChars(strAccents);

  turnOffAllLeds();
  ligaLed(ledSnakeGreen, true);

  lcd.clear();
  lcd.setCursor(0, 1);
  displayRomStr(10320);     // Desafio da Cobra
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);
  displayRomStr(10660);     // O objetivo é levar
  lcd.setCursor(0, 1);
  displayRomStr(10680);     // a argola até a base
  lcd.setCursor(0, 2);
  displayRomStr(10700);     // vermelha, sem encostar
  lcd.setCursor(0, 3);
  displayRomStr(10720);     // na cobra metálica.
  waitConfirm(5000);

  lcd.clear();
  displayPlayer();
  lcd.write(",");
  lcd.setCursor(0, 1);
  displayRomStr(10340);     // voce tem XX segundos
  lcd.setCursor(9, 1);
  lcd.print(time, DEC);
  lcd.setCursor(0, 2);
  displayRomStr(10360);     // para atravessar o
  lcd.setCursor(0, 3);
  displayRomStr(10380);     // Vale das Cobras !!!
  waitConfirm(5000);

  lcd.clear();
  lcd.setCursor(0, 0);
  displayRomStr(10400);     // Para começar
  lcd.setCursor(0, 1);
  displayRomStr(10420);     // encoste a argola na
  lcd.setCursor(0, 2);
  displayRomStr(10440);     // parte metalica da
  lcd.setCursor(0, 3);
  displayRomStr(10460);     // base preta.
  buttonPressed = -1;

  while (true) {
    buttonPressed = getButton();
    if (buttonPressed == evtS1Press) {
      break;
    }
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  displayRomStr(10740);     // Pode começar !!!
  buttonPressed = -1;

  while (true) {
    buttonPressed = getButton();
    if (buttonPressed == evtS1Release) {
      break;
    }
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  displayRomStr(10760);     // Não encoste
  lcd.setCursor(0, 1);
  displayRomStr(10780);     // na cobra!

  unsigned long currentMillis = millis();
  timer2 = currentMillis;
  timer2_interval = 1000;
  buttonPressed = -1;

  while (true) {
    currentMillis = millis();
    buttonPressed = getButton();

    if (buttonPressed == evtS2Press) {
      playersPos[currentPlayer] = playersPos[currentPlayer] + 1;
      lcd.clear();
      lcd.setCursor(0, 0);
      displayRomStr(10280);     // Parabéns !!!
      lcd.setCursor(0, 1);
      displayRomStr(10800);     // Você conseguiu !!!
      lcd.setCursor(0, 3);
      displayRomStr(10600);     // Avance uma casa
      lcd.write("Avance 1 casa.");
      break;
    }

    if ((buttonPressed == evtSW) || (time < 0)) {
      playersPos[currentPlayer] = playersPos[currentPlayer] - 1;
      ligaLed(ledSnakeRed, true);
      ligaLed(ledSnakeGreen, false);
      lcd.clear();
      lcd.setCursor(0, 0);
      displayRomStr(10820);     // A cobra mordeu você!
      lcd.setCursor(0, 2);
      displayRomStr(10640);     // Retorne uma casa
      break;
    }

    if (currentMillis - timer2 > timer2_interval) {
      lcd.setCursor(0, 3);
      lcd.write("Tempo : ");
      lcd.print(time, DEC);
      lcd.write(" ");
      timer2 = currentMillis;
      time--;
    }
  }

  waitConfirm(3000);
  turnOffAllLeds();
}

void ligaLed(int i, bool flag) {
  int dig = 0;
  int ld = 0;

  if ((i >= 1) && (i <= 8)) {
    dig = 0;
    if (i == 6) {
      i = 7;
    }
    else if (i == 7) {
      i = 6;
    }
    if (i == 8) {
      ld = 0;
    }
    else {
      ld = i;
    }
  }
  else if ((i >= 9) && (i <= 16)) {
    dig = 1;
    i = i - 8;
    if (i == 8) {
      ld = 0;
    }
    else {
      ld = i;
    }
  }
  else if ((i >= 17) && (i <= 24)) {
    dig = 3;
    i = i - 16;
    if (i == 8) {
      ld = 0;
    }
    else {
      ld = i;
    }
  }

  lc.setLed(0, dig, ld, flag);
}

void clearRing() {
  for (int r = 4; r <= 7; r++) {
    lc.setRow(0, r, B00000000);
  }
}

void ringSeq(int n) {
  clearRing();
  for (int c = 1; c <= n; c++) {
    ring(c);
  }
}

void ring(int n) {
  int dig = 0;
  int ld = 0;

  switch (n) {
    case 0:
      dig = 7;
      ld = 3;
      break;
    case 1 ... 4:
      dig = 4;
      ld = n - 1;
      break;
    case 5 ... 8:
      dig = 5;
      ld = n - 5;
      break;
    case 9 ... 12:
      dig = 6;
      ld = n - 9;
      break;
    case 13 ... 15:
      dig = 7;
      ld = n - 13;
      break;
    default:
      break;
  }
  lc.setLed(0, dig, ld, true);
}

void readRotaryEncoder() {
  int n = LOW;
  n = digitalRead(Rot_Pin1);
  if ((lastEncoded == LOW) && (n == HIGH)) {
    if (digitalRead(Rot_Pin2) == LOW) {
      encoderValue--;
    }
    else {
      encoderValue++;
    }
    if (encoderValue > 15) encoderValue = 15;
    if (encoderValue < 1) encoderValue = 1;
  }
  lastEncoded = n;
}

void initButton(int pinButton) {
  pinMode(pinButton, INPUT);
  digitalWrite(pinButton, HIGH);
  pinMode(pinButton, INPUT_PULLUP);

  switch (pinButton) {
    case buttonRot:
      debouncerRot.attach(pinButton);
      debouncerRot.interval(5); // interval in ms
      break;
    case buttonA:
      debouncerA.attach(pinButton);
      debouncerA.interval(5); // interval in ms
      break;
    case buttonB:
      debouncerB.attach(pinButton);
      debouncerB.interval(5); // interval in ms
      break;
    case buttonC:
      debouncerC.attach(pinButton);
      debouncerC.interval(5); // interval in ms
      break;
    case snakeWire:
      debouncerSW.attach(pinButton);
      debouncerSW.interval(5); // interval in ms
      break;
    case snakeStart:
      debouncerS1.attach(pinButton);
      debouncerS1.interval(5); // interval in ms
      break;
    case snakeEnd:
      debouncerS2.attach(pinButton);
      debouncerS2.interval(5); // interval in ms
      break;
    default:
      break;
  }
}

// verifica qual botão foi acionado
int getButton() {
  // evento do botao Rotary Encoder
  debouncerRot.update();
  static int lastBtnRotVal = -1;
  int BtnRotVal = debouncerRot.read();
  if (BtnRotVal != lastBtnRotVal) {
    lastBtnRotVal = BtnRotVal;
    if ( BtnRotVal == LOW ) {
      // evento Release
    }
    else {
      // evento Press
      return evtBtnRot;
    }
  }
  // evento do botao A
  debouncerA.update();
  static int lastBtnAVal = -1;
  int BtnAVal = debouncerA.read();
  if (BtnAVal != lastBtnAVal) {
    lastBtnAVal = BtnAVal;
    if ( BtnAVal == LOW ) {
      // evento Release
    }
    else {
      // evento Press
      return evtBtnA;
    }
  }

  // evento do botao B
  debouncerB.update();
  static int lastBtnBVal = -1;
  int BtnBVal = debouncerB.read();
  if (BtnBVal != lastBtnBVal) {
    lastBtnBVal = BtnBVal;
    if ( BtnBVal == LOW ) {
      // evento Release
    }
    else {
      // evento Press
      return evtBtnB;
    }
  }

  // evento do botao C
  debouncerC.update();
  static int lastBtnCVal = -1;
  int BtnCVal = debouncerC.read();
  if (BtnCVal != lastBtnCVal) {
    lastBtnCVal = BtnCVal;
    if ( BtnCVal == LOW ) {
      // evento Release
    }
    else {
      // evento Press
      return evtBtnC;
    }
  }

  // evento encostar a argola no arame
  debouncerSW.update();
  static int lastBtnSWVal = -1;
  int BtnSWVal = debouncerSW.read();
  if (BtnSWVal != lastBtnSWVal) {
    lastBtnSWVal = BtnSWVal;
    if ( BtnSWVal == HIGH ) {
      // evento Release
    }
    else {
      // evento Press
      return evtSW;
    }
  }

  // evento encostar a argola no ponto de inicio
  debouncerS1.update();
  static int lastBtnS1Val = -1;
  int BtnS1Val = debouncerS1.read();
  if (BtnS1Val != lastBtnS1Val) {
    lastBtnS1Val = BtnS1Val;
    if ( BtnS1Val == HIGH ) {
      // evento Release
      return evtS1Release;
    }
    else {
      // evento Press
      return evtS1Press;
    }
  }

  // evento encostar a argola no ponto final
  debouncerS2.update();
  static int lastBtnS2Val = -1;
  int BtnS2Val = debouncerS2.read();
  if (BtnS2Val != lastBtnS2Val) {
    lastBtnS2Val = BtnS2Val;
    if ( BtnS2Val == HIGH ) {
      // evento Release
      return evtS2Release;
    }
    else {
      // evento Press
      return evtS2Press;
    }
  }

  getSoundVolume();

  return -1;
}

// mostra os jogados ativos no jogo e a sequencia deles
void displayPlayers() {
  digitalWrite(BACKLIGHT_PIN, HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  displayRomStr(10120);     // Ordem dos jogadores
  lcd.setCursor(0, 1);
  lcd.write("1-");
  lcd.write(players[0]);
  lcd.setCursor(11, 1);
  lcd.write("2-");
  lcd.write(players[1]);
  if (NumberPlayers > 2) {
    lcd.setCursor(0, 2);
    lcd.write("3-");
    lcd.write(players[2]);
  }
  if (NumberPlayers > 3) {
    lcd.setCursor(11, 2);
    lcd.write("4-");
    lcd.write(players[3]);
  }
  if (NumberPlayers > 4) {
    lcd.setCursor(0, 3);
    lcd.write("5-");
    lcd.write(players[4]);
  }
}

// apaga todos os leds
void turnOffAllLeds() {
  lc.shutdown(0, false);
  lc.clearDisplay(0);
}

// faz os leds da roleta acenderem sequencialmente
// com velocidade indicada pelo valor do rotary encoder.
void spinRoulette() {
  int randNumber = random(20);

  digitalWrite(BACKLIGHT_PIN, LOW);

  int turns = randNumber + (encoderValue * 2);
  int i = 0;
  int speed = 180 - (encoderValue * 10);
  if (speed < 10) speed = 10;

  while (i <= turns) {
    ligaLed(roulettePos, true);
    delay(speed);
    speed = speed + 1;
    ligaLed(roulettePos, false);
    roulettePos++;
    if (roulettePos > 16) {
      roulettePos = 1;
    }
    i++;
  }

  int rv = rouletteVal[roulettePos - 1];

  // evita que o jogador atual sofra a penalidade
  // de voltar ao início mais de uma vez
  if (rv == -1) {
    if (playersPenalty[currentPlayer] > 0) {
      roulettePos++;
      rv = rouletteVal[roulettePos - 1];
    }
    playersPenalty[currentPlayer]++;
  }

  ligaLed(roulettePos, true);
  digitalWrite(BACKLIGHT_PIN, HIGH);
}

// mostra o placar geral, com a posição
// no tabuleiro de cada jogador
void displayPos() {
  digitalWrite(BACKLIGHT_PIN, HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  displayRomStr(10140);     // Placar geral:

  lcd.setCursor(0, 1);
  lcd.write(players[0]);
  char str[10];
  sprintf(str, " %2d", playersPos[0]);
  lcd.setCursor(10, 1);
  lcd.write(str);

  if (NumberPlayers > 1) {
    lcd.setCursor(0, 2);
    lcd.write(players[1]);
    char str[10];
    sprintf(str, " %2d", playersPos[1]);
    lcd.setCursor(10, 2);
    lcd.write(str);
  }
  if (NumberPlayers > 2) {
    lcd.setCursor(0, 3);
    lcd.write(players[2]);
    char str[10];
    sprintf(str, " %2d", playersPos[2]);
    lcd.setCursor(10, 3);
    lcd.write(str);
  }

  if (NumberPlayers > 3) {
    waitConfirm(3000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(players[3]);
    char str[10];
    sprintf(str, " %2d", playersPos[3]);
    lcd.setCursor(10, 0);
    lcd.write(str);
  }

  if (NumberPlayers > 4) {
    lcd.setCursor(0, 1);
    lcd.write(players[4]);
    char str[10];
    sprintf(str, " %2d", playersPos[4]);
    lcd.setCursor(10, 1);
    lcd.write(str);
  }

  buttonPressed = -1;
  waitConfirm(3000);
}

// mostra a mensagem de fim de jogo
void displayGameOver() {
  digitalWrite(BACKLIGHT_PIN, HIGH);
  lcd.clear();
  lcd.setCursor(0, 1);
  displayRomStr(10160);     // game over
  waitConfirm(0);
}


// exibe uma questao aleatoria, lendo da memoria EEPROM ------------------
#define eepromChip 0x50    //romAddress of 24LC256 eeprom chip
#define recCount 47
#define recordLength 160
#define fieldLength   20
// questions in EEPROM
#define questLevel01_begin  0
#define questLevel01_end   13
#define questLevel02_begin 14
#define questLevel02_end   32
#define questLevel03_begin 33
#define questLevel03_end   46

// exibe a mensagem para o jogador retirar uma carta
// e indicar, através dos botões, a cor dela.
void displayGetCard() {
  loadChars(strAccents);
  digitalWrite(BACKLIGHT_PIN, HIGH);
  lcd.clear();
  displayPlayer();
  lcd.setCursor(0, 1);
  displayRomStr(10480);     // deve pegar uma carta
  lcd.setCursor(0, 2);
  displayRomStr(10500);     // e pressionar o botao
  lcd.setCursor(0, 3);
  displayRomStr(10520);     // da mesma cor.

  // espera a resposta
  ligaLed(ledButtonA, true);
  ligaLed(ledButtonB, true);
  ligaLed(ledButtonC, true);

  buttonPressed = -1;
  int level = 0;

  while (true)
  {
    buttonPressed = getButton();

    if (buttonPressed == 1) {  // botao vermelho = pergunta dificil
      level = 3;
      buttonPressed = -1;
      break;
    }
    if (buttonPressed == 2) {  // botao azul = pergunta media
      level = 2;
      buttonPressed = -1;
      break;
    }
    if (buttonPressed == 3) {  // botao verde = pergunta facil
      level = 1;
      buttonPressed = -1;
      break;
    }
    buttonPressed = -1;
  }

  lcd.clear();
  lcd.write("O ");
  displayPlayer();
  lcd.setCursor(0, 1);
  displayRomStr(10540);     // deve responder a
  lcd.setCursor(0, 2);
  displayRomStr(10560);     // pergunta a seguir:

  displayQuestion(level);
}

void displayPlayer() {
  lcd.write("Jogador ");
  lcd.write(players[currentPlayer]);
}

// exibe uma pergunta aleatória, de acordo com o nível
// de dificuldade indicado na variável level.
// Lê a pergunta da memória EEPROM e exibe no display
// lcd utilizando caracteres acentuados, que são
// carregados dinamicamente de acordo com os acentos
// que serão necessários para a pergunta a ser exibida.
void displayQuestion(int level) {
  static int questionNumber = -1;
  int rnd = -1;
  int tries = 0;
  buttonPressed = -1;


  digitalWrite(BACKLIGHT_PIN, HIGH);

  // Sorteia uma questão dentro do range
  // de dificuldade indicada no parâmetro level.
  // Se coincindir com o número sorteado na
  // rodada anterior, tenta sortear novamente.
  do
  {
    switch (level) {
      case 1:
        rnd = random(questLevel01_begin, questLevel01_end);
        break;
      case 2:
        rnd = random(questLevel02_begin, questLevel02_end);
        break;
      case 3:
        rnd = random(questLevel03_begin, questLevel03_end);
        break;
    }
    if (rnd != questionNumber) {
      break;
    }
    tries++;
  } while (tries < 3);

  questionNumber = rnd;

  unsigned char rdata[21] = "ABCDEFGHIJ1234567890";
  char xdata[21] = "ABCDEFGHIJ1234567890";
  unsigned int romAddr = questionNumber * recordLength;;
  char mp3[5] = "????";
  int mp3Ind = 0;
  char aDelay[3] = "00";
  int  questDelay = 0;
  char accents[11] = "          ";
  char correct = 'X';

  // le o header do registro no rom -------
  readEEPROM(eepromChip, romAddr, rdata, fieldLength);
  strncpy(xdata, (char*)rdata, fieldLength);

  // le o nome do arquivo mp3 da questao
  mp3[0] = xdata[0];
  mp3[1] = xdata[1];
  mp3[2] = xdata[2];
  mp3[3] = xdata[3];
  mp3Ind = atoi(mp3);

  // le o tempo de espera entre a questao e as alternativas
  aDelay[0] = xdata[5];
  aDelay[1] = xdata[6];
  questDelay = atoi(aDelay) * 1000;

  // le os caracteres especiais da pergunta
  for (int i = 0; i < 10; i++) {
    accents[i] = xdata[i + 8];
  }
  loadChars(accents);

  // le da rom as quatro linhas da questao
  lcd.clear();
  romAddr = romAddr + fieldLength;
  for (int i = 0; i < 4; i++) {
    readEEPROM(eepromChip, romAddr, rdata, fieldLength);
    strncpy(xdata, (char*)rdata, fieldLength);
    if (xdata[0] != ' ') {
      lcd.setCursor(0, i);
      lcdDisplay(xdata, accents);
    }
    romAddr = romAddr + fieldLength;
  }

  delay(1000);
  soundPlay(mp3Ind);
  waitConfirm(questDelay);

  // le as tres alternativas
  lcd.clear();

  for (int i = 0; i < 3; i++) {
    readEEPROM(eepromChip, romAddr, rdata, fieldLength);
    strncpy(xdata, (char*)rdata, fieldLength);
    // procura por * no final da string, para saber se é a alternativa correta
    if (xdata[19] == '*') {
      correct = char(65 + i);
    }
    xdata[18] = 0;   // trunca a string em 18 caracteres
    lcd.setCursor(0, i);
    lcd.write(char(65 + i));
    lcd.write(")");
    lcdDisplay(xdata, accents);
    romAddr = romAddr + fieldLength;
  }

  // espera a resposta
  ligaLed(ledButtonA, true);
  ligaLed(ledButtonB, true);
  ligaLed(ledButtonC, true);

  buttonPressed = -1;
  bool ra = false;

  while (true)
  {
    buttonPressed = getButton();

    if (buttonPressed == 1) {
      if (correct == 'A') ra = true;
      buttonPressed = -1;
      break;
    }
    if (buttonPressed == 2) {
      if (correct == 'B') ra = true;
      buttonPressed = -1;
      break;
    }
    if (buttonPressed == 3) {
      if (correct == 'C') ra = true;
      buttonPressed = -1;
      break;
    }
    buttonPressed = -1;
  }

  lcd.clear();

  if (ra == true) {
    lcd.setCursor(0, 0);
    displayRomStr(10580);     // Resposta correta !!!
    lcd.setCursor(0, 1);
    displayRomStr(10600);     // Avance 1 casa
    playersPos[currentPlayer] = playersPos[currentPlayer] + 1;
    if (playersPos[currentPlayer] > 54) {
      playersPos[currentPlayer] = 55;
    }
  } else {
    lcd.setCursor(0, 0);
    displayRomStr(10620);     // Resposta errada !!!
    lcd.setCursor(0, 1);
    displayRomStr(10640);     // Retorne 1 casa
    playersPos[currentPlayer] = playersPos[currentPlayer] - 1;
  }

  waitConfirm(2000);
}

// espera pelo pressionamento do botão C, ou pelo tempo estipulado na variável timeWait,
// se timeWait for igual a zero, espera indefinidamente
void waitConfirm(int timeWait) {
  unsigned long currentMillis = millis();

  ligaLed(ledButtonA, false);
  ligaLed(ledButtonB, false);
  ligaLed(ledButtonC, true);

  timer1 = currentMillis;
  buttonPressed = -1;

  while (true)
  {
    currentMillis = millis();
    if (timeWait > 0) {
      if (currentMillis - timer1 > timeWait) {
        timer1 = currentMillis;
        buttonPressed = -1;
        break;
      }
    }

    buttonPressed = getButton();

    if (buttonPressed == 3) {
      buttonPressed = -1;
      break;
    }
    buttonPressed = -1;
  }
}

// exibe texto acentuado no display
void lcdDisplay(char str[], char accents[]) {
  int p = 0;
  int i = 0;
  char s[21];

  int len1 = 0;
  while (str[len1] != 0) {
    len1++;
  };

  int len2 = 0;
  while ((accents[len2] != 0) && (accents[len2] != 32)) {
    len2++;
  };

  for (i = 0; i < len1; i++) {
    p = -1;
    int cp = 0;
    while (cp < len2) {
      if (str[i] == accents[cp]) {
        p = cp;
        break;
      }
      cp++;
    }
    if (p >= 0) {
      s[i] = char(p + 1);
    } else {
      s[i] = str[i];
    }
  }
  s[i] = 0;

  lcd.write(s);
}

// Caracteres acentuados
const uint8_t charBitmap[][12] = {
  { 0b01110, 0b00000, 0b01110, 0b00001, 0b01111, 0b10001, 0b01111, 0b00000 }, // ã
  { 0b00010, 0b00100, 0b01110, 0b00001, 0b01111, 0b10001, 0b01111, 0b00000 }, // á
  { 0b00100, 0b01010, 0b01110, 0b00001, 0b01111, 0b10001, 0b01111, 0b00000 }, // â
  { 0b00010, 0b00100, 0b01110, 0b10001, 0b11111, 0b10001, 0b10001, 0b00000 }, // Á
  { 0b00010, 0b00100, 0b01110, 0b10001, 0b11111, 0b10000, 0b01110, 0b00000 }, // é
  { 0b00100, 0b01010, 0b01110, 0b10001, 0b11111, 0b10000, 0b01110, 0b00000 }, // ê
  { 0b00010, 0b00100, 0b00000, 0b01100, 0b00100, 0b00100, 0b01110, 0b00000 }, // í
  { 0b00010, 0b00100, 0b01110, 0b00100, 0b00100, 0b00100, 0b01110, 0b00000 }, // Í
  { 0b00010, 0b00100, 0b01110, 0b10001, 0b10001, 0b10001, 0b01110, 0b00000 }, // ó
  { 0b01110, 0b00000, 0b01110, 0b10001, 0b10001, 0b10001, 0b01110, 0b00000 }, // õ
  { 0b01110, 0b10001, 0b01110, 0b10001, 0b10001, 0b10001, 0b01110, 0b00000 }, // ô
  { 0b00010, 0b00100, 0b10001, 0b10001, 0b10001, 0b10011, 0b01101, 0b00000 }, // ú
  { 0b00000, 0b00000, 0b01110, 0b10000, 0b10000, 0b10001, 0b01110, 0b11000 }  // ç
};

// carrega os caracteres especiais para a ROM do LCD
void loadChars(char str[]) {
  int len = 0;
  do {
    len++;
  } while ((str[len] != 0) && (str[len] != 32));

  for (int i = 0; i < len; i++) {
    char c = str[i];
    if (c == '@') lcd.createChar (i + 1, (uint8_t *)charBitmap[0]);
    if (c == '#') lcd.createChar (i + 1, (uint8_t *)charBitmap[1]);
    if (c == '$') lcd.createChar (i + 1, (uint8_t *)charBitmap[2]);
    if (c == '&') lcd.createChar (i + 1, (uint8_t *)charBitmap[3]);
    if (c == '*') lcd.createChar (i + 1, (uint8_t *)charBitmap[4]);
    if (c == '/') lcd.createChar (i + 1, (uint8_t *)charBitmap[5]);
    if (c == '<') lcd.createChar (i + 1, (uint8_t *)charBitmap[6]);
    if (c == '>') lcd.createChar (i + 1, (uint8_t *)charBitmap[7]);
    if (c == '_') lcd.createChar (i + 1, (uint8_t *)charBitmap[8]);
    if (c == ']') lcd.createChar (i + 1, (uint8_t *)charBitmap[9]);
    if (c == '[') lcd.createChar (i + 1, (uint8_t *)charBitmap[10]);
    if (c == '{') lcd.createChar (i + 1, (uint8_t *)charBitmap[11]);
    if (c == '}') lcd.createChar (i + 1, (uint8_t *)charBitmap[12]);
  }
}

// lê a memória EEPROM usando comunicação I2c
void readEEPROM(int deviceromAddress, unsigned int eeromAddress, unsigned char* data, unsigned int num_chars)
{
  unsigned char i = 0;
  Wire.beginTransmission(deviceromAddress);
  Wire.write((int)(eeromAddress >> 8));   // MSB
  Wire.write((int)(eeromAddress & 0xFF)); // LSB
  Wire.endTransmission();

  Wire.requestFrom(deviceromAddress, num_chars);

  while (Wire.available()) data[i++] = Wire.read();
}

void soundPlay(int soundIndex) {
  soundSetVolume(soundVolume);
  Serial.write(0x7E);
  Serial.write(0x04);
  Serial.write(0xA0); // A0 for SD card
  Serial.write((byte)0x00);
  Serial.write(soundIndex); // track number
  Serial.write(0x7E);
}

void soundPause() {
  Serial.write(0x7E);
  Serial.write(0x02);
  Serial.write(0xA3);
  Serial.write(0x7E);
}

void soundStop() {
  Serial.write(0x7E);
  Serial.write(0x02);
  Serial.write(0xA4);
  Serial.write(0x7E);
}

void soundSetVolume(int soundVolume) {
  // 00-mute
  // 31-max volume
  Serial.write(0x7E);
  Serial.write(0x03);
  Serial.write(0xA7);
  Serial.write(soundVolume); //  volume max 31 (1F)
  Serial.write(0x7E);
}

#define volPin A6    // select the input pin for the potentiometer

void getSoundVolume() {
  static unsigned long lastTime = millis();
  soundVolume = 30;
  return;
  if (millis() - lastTime > 1000) {
    lastTime = millis();
    int sensorValue = analogRead(volPin);
    int v = map(sensorValue, 530, 1023, 0, 30);
    if (v < 0) {
      v = 0;
    }
    if (v > 30) {
      v = 30;
    }
    if (v != soundVolume) {
      soundVolume = v;
      soundSetVolume(soundVolume);
    }
  }
}

void displayRomStr(unsigned int eepromAddress) {
  unsigned char rdata[21] = "ABCDEFGHIJ1234567890";
  char xdata[21] = "ABCDEFGHIJ1234567890";
  unsigned int romAddr = 10000;

  // se a lista de acentos estiver vazia
  // carrega da EEPROM
  if (strAccents[0] == 32) {
    readEEPROM(eepromChip, romAddr, rdata, 20);
    strncpy(strAccents, (char*)rdata, 10);
    strAccents[11] = 0;
  }

  readEEPROM(eepromChip, eepromAddress, rdata, 20);
  strncpy(xdata, (char*)rdata, 20);
  Serial.println(xdata);

  lcdDisplay(xdata, strAccents);
}

