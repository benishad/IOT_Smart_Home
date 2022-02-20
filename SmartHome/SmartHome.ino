#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>  // "툴 - 라이브러리 관리" 에서 Adafruit NeoPixel 검색 후 설치
#include <Keypad.h>             // "툴 - 라이브러리 관리" 에서 keypad검색 후 "keypad" 설치
#include <MFRC522.h>            // "툴 - 라이브러리 관리" 에서 MFRC522 검색 후 설치
#include <Servo.h>
#include <SPI.h>

#define LED1_Pin 6
#define LED_COUNT 15

Adafruit_NeoPixel LED1(LED_COUNT, LED1_Pin, NEO_GRB + NEO_KHZ800);

#define SS_PIN 53
#define RST_PIN 49

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

Servo servo1;
Servo servo2;

byte nuidPICC[4];

char secretCode[] = {'1', '2', '3', '4'};
int position = 0;
int wrong = 0;

const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {23, 25, 27, 29};
byte colPins[COLS] = {31, 33, 35, 37};

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);

int MotorA = 2;
int MotorB = 3;
int MotorC = 4;
int MotorD = 5;
int motorState = 0;

int BuzzerPin = 7;
int Servo1Pin = 8;
int Servo2Pin = 10;

int MotorState = 0;
int Servo2State = 0;

int neopixelState = 0;
void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  SPI.begin();

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  LED1.begin();
  LED1.show();
  LED1.setBrightness(20);

  pinMode(MotorA, OUTPUT);
  pinMode(MotorB, OUTPUT);
  pinMode(MotorC, OUTPUT);
  pinMode(MotorD, OUTPUT);
  setLocked(true);
  //Motor(true);
  Servo2(true);
}

void loop() {
  switch (Serial1.read()) {
    case 'A':
      if (MotorState == 0) {
        Motor(true);
        MotorState++;
      } else {
        Motor(false);
        MotorState = 0;
      }
      break;
    case 'B':
      if (Servo2State == 0) {
        Servo2(true);
        Servo2State++;
      } else {
        Servo2(false);
        Servo2State = 0;
      }
      break;
    case 'C':
      if (motorState == 0) {
        digitalWrite(MotorA, LOW);
        digitalWrite(MotorB, LOW);
        motorState++;
      } else {
        digitalWrite(MotorA, LOW);
        digitalWrite(MotorB, HIGH);
        motorState = 0;
      }
      break;
    case 'D':
      if (neopixelState == 0) {
        for (int i = 0; i < 8; i++) {
          LED1.setPixelColor(i, 0, 0, 0);
          LED1.show();
        }
        neopixelState++;
      } else {
        for (int i = 0; i < 8; i++) {
          LED1.setPixelColor(i, 255, 255, 255);
          LED1.show();
        }
        neopixelState = 0;
      }
      break;
    default:
      //nothing
      break;
  }
  doorLock();
  nfc();
}


void nfc() {
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  if ( ! rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  if (rfid.uid.uidByte[0] != nuidPICC[0] ||
      rfid.uid.uidByte[1] != nuidPICC[1] ||
      rfid.uid.uidByte[2] != nuidPICC[2] ||
      rfid.uid.uidByte[3] != nuidPICC[3] ) {
    Serial.println(F("A new card has been detected."));

    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }
    setLocked(false);
    analogWrite(BuzzerPin, 200);
    delay(100);
    analogWrite(BuzzerPin, 0);
    delay(100);
    analogWrite(BuzzerPin, 200);
    delay(100);
    analogWrite(BuzzerPin, 0);
  }
  else {
    Serial.println(F("Card read previously."));
    setLocked(false);
    analogWrite(BuzzerPin, 200);
    delay(100);
    analogWrite(BuzzerPin, 0);
    delay(100);
    analogWrite(BuzzerPin, 200);
    delay(100);
    analogWrite(BuzzerPin, 0);
  }

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

//--------------------------------------------------------------------

void doorLock() {
  char key = keypad.getKey(); // 키패드에서 입력된 값을 가져옵니다.

  if (key) {
    Serial.println(key);
  }

  if ((key >= '0' && key <= '9') || (key == '*' || key == '#')) {
    // 키패드에서 입력된 값을 조사하여 맞게 입력된 값일 경우(키패드에 있는 버튼이 맞을경우) 비교

    if (key == '*' || key == '#') { // *, # 버튼을 눌렀을 경우
      position = 0;
      wrong = 0; // 입력 초기화
      setLocked(true); // 잠금상태로 세팅
      analogWrite(BuzzerPin, 200);
      delay(500);
      analogWrite(BuzzerPin, 0);
    } else if (key == secretCode[position]) { // 해당 자리에 맞는 비밀번호가 입력됬을 경우
      position++; // 다음 자리로 넘어 감
      wrong = 0; // 비밀번호 오류 값을 0으로 만듬
      analogWrite(BuzzerPin, 200);
      delay(100);
      analogWrite(BuzzerPin, 0);
    } else if (key != secretCode[position]) { // 해당 자리에 맞지 않는 비밀번호가 입력됬을 경우
      position = 0; // 비밀번호를 맞았을 경우를 0으로 만듬
      setLocked(true); // 잠금상태로 세팅
      wrong++; // 비밀번호 오류 값을 늘려준다
      analogWrite(BuzzerPin, 200);
      delay(100);
      analogWrite(BuzzerPin, 0);
    }

    if (position == 4) { // 4자리 비밀번호가 모두 맞았을 경우
      setLocked(false);
      analogWrite(BuzzerPin, 200);
      delay(100);
      analogWrite(BuzzerPin, 0);
      delay(100);
      analogWrite(BuzzerPin, 200);
      delay(100);
      analogWrite(BuzzerPin, 0);
    }

    if (wrong == 4) { // 비밀번호 오류를 4번 했을 경우
      wrong = 0; // 비밀번호 오류 값을 0으로 만들어 준다.
      analogWrite(BuzzerPin, 200);
      delay(1000);
      analogWrite(BuzzerPin, 0);
    }
  }
  if ((key >= 'A' && key <= 'D')) {
    if (key == 'A') {
      analogWrite(BuzzerPin, 200);
      delay(100);
      analogWrite(BuzzerPin, 0);

      if (MotorState == 0) {
        Motor(true);
        MotorState++;
      } else {
        Motor(false);
        MotorState = 0;
      }
    } else if (key == 'B') {
      analogWrite(BuzzerPin, 200);
      delay(100);
      analogWrite(BuzzerPin, 0);

      if (Servo2State == 0) {
        Servo2(true);
        Servo2State++;
      } else {
        Servo2(false);
        Servo2State = 0;
      }
    } else if (key == 'C') {
      analogWrite(BuzzerPin, 200);
      delay(100);
      analogWrite(BuzzerPin, 0);

      if (motorState == 0) {
        digitalWrite(MotorA, LOW);
        digitalWrite(MotorB, LOW);
        motorState++;
      } else {
        digitalWrite(MotorA, LOW);
        digitalWrite(MotorB, HIGH);
        motorState = 0;
      }
    } else if (key == 'D') {
      analogWrite(BuzzerPin, 200);
      delay(100);
      analogWrite(BuzzerPin, 0);

      if (neopixelState == 0) {
        for (int i = 0; i < 8; i++) {
          LED1.setPixelColor(i, 0, 0, 0);
          LED1.show();
        }
        neopixelState++;
      } else {
        for (int i = 0; i < 8; i++) {
          LED1.setPixelColor(i, 255, 255, 255);
          LED1.show();
        }
        neopixelState = 0;
      }
    } else {

    }
  }
}

void setLocked(int locked) {
  if (locked) {
    servo1.attach(Servo1Pin);
    servo1.write(0);
    delay(100);
    servo1.detach();
  } else {
    servo1.attach(Servo1Pin);
    servo1.write(90);
    delay(100);
    servo1.detach();
  }
}

void Motor(int locked) {
  if (locked) {
    digitalWrite(MotorC, LOW);
    digitalWrite(MotorD, HIGH);
    delay(4300);
    digitalWrite(MotorC, LOW);
    digitalWrite(MotorD, LOW);
  } else {
    digitalWrite(MotorC, HIGH);
    digitalWrite(MotorD, LOW);
    delay(4300);
    digitalWrite(MotorC, LOW);
    digitalWrite(MotorD, LOW);
  }
}

void Servo2(int locked) {
  if (locked) {
    servo2.attach(Servo2Pin);
    servo2.write(0);
    delay(500);
    servo2.detach();
  } else {
    servo2.attach(Servo2Pin);
    servo2.write(180);
    delay(500);
    servo2.detach();
  }
}
