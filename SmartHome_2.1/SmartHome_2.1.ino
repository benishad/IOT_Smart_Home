#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>  // "툴 - 라이브러리 관리" 에서 Adafruit NeoPixel 검색 후 설치
#include <Keypad.h>             // "툴 - 라이브러리 관리" 에서 keypad검색 후 "keypad" 설치
#include <MFRC522.h>            // "툴 - 라이브러리 관리" 에서 MFRC522 검색 후 설치
#include <Servo.h>
#include <SPI.h>

#define LED1_Pin 6    // LED 핀 설정
#define LED_COUNT 15  // LED 갯수

Adafruit_NeoPixel LED1(LED_COUNT, LED1_Pin, NEO_GRB + NEO_KHZ800);

#define SS_PIN 53   // 키패드 SPI 통신을 위한 핀 설정
#define RST_PIN 49  // 키패드 SPI 통신을 위한 핀 설정

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;  // 키패드 객체 생성

Servo servo1; // 서보 객체 생성
Servo servo2; // 서보 객체 생성

byte nuidPICC[4];

char secretCode[] = {'1', '4', '7', '8'}; // 비밀번호
int position = 0; // 비밀번호 대조 위치확인용
int wrong = 0;  // 비밀번호 틀린 횟수

const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {23, 25, 27, 29};  // 키패드 핀
byte colPins[COLS] = {31, 33, 35, 37};  // 키패드 핀

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);

int MotorA = 2; // 모터 핀
int MotorB = 3; // 모터 핀
int MotorC = 4; // 모터 핀
int MotorD = 5; // 모터 핀
int MotorPWM = 11; // 모터 속도조절 핀
int MotorPWM2 = 12; // 모터 속도조절 핀
int motorState = 0; // 모터 on/off 상태
int motorSpeed = 255; // 모터 속도

int LedBright = 255;  // Led 밝기
int BuzzerPin = 7;  // 부저 핀
int Servo1Pin = 8;  // 서보 핀
int Servo2Pin = 10; // 서보 핀

int MotorState = 0; // 모터 on/off 상태
int Servo2State = 0;

int neopixelState = 0;
void setup() {
  Serial.begin(9600); // 시리얼 통신 시작
  Serial1.begin(9600);  // 블루투스 시리얼 통신 시작
  SPI.begin();  // SPI 통신 시작
  rfid.PCD_Init(); // Init MFRC522

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
  pinMode(MotorPWM, OUTPUT);
  setLocked(true);
  Servo2(true);
}

void loop() {
  // 블루투스로 부터 값을 받아 들여 그에 맞는 동작 수행
  switch (Serial1.read()) {
    case 'A': // 블루투스로부터 A값을 받아오면 모터 동작
      if (MotorState == 0) {
        Motor(true);
        MotorState++;
      } else {
        Motor(false);
        MotorState = 0;
      }
      break;
    case 'B': // 블루투스로부터 B값을 받아오면 서보모터 동작
      if (Servo2State == 0) {
        Servo2(true);
        Servo2State++;
      } else {
        Servo2(false);
        Servo2State = 0;
      }
      break;
    case 'C': // 블루투스로부터 C값을 받아오면 모터 동작
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
    case 'D': // 블루투스로부터 D값을 받아오면 LED 동작
      if (neopixelState == 0) {
        for (int i = 0; i < 8; i++) {
          LED1.setPixelColor(i, 0, 0, 0);
          LED1.show();
        }
        neopixelState++;
      } else {
        for (int i = 0; i < 8; i++) {
          LED1.setPixelColor(i, LedBright, LedBright, LedBright);
          LED1.show();
        }
        neopixelState = 0;
      }
      break;
    case 'F': // 블루투스로부터 F값을 받아오면 모터 속도 50%
      motorSpeed = 127;
      break;
    case 'G': // 블루투스로부터 G값을 받아오면 모터 속도 100%
      motorSpeed = 255;
      break;
    case 'H': // 블루투스로부터 H값을 받아오면 밝기 50%
      LedBright = 127;
      break;
    case 'I': // 블루투스로부터 I값을 받아오면 밝기 100%
      LedBright = 255;
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

  // RFID 태그 시 문이 열림
  if (rfid.uid.uidByte[0] != nuidPICC[0] ||
      rfid.uid.uidByte[1] != nuidPICC[1] ||
      rfid.uid.uidByte[2] != nuidPICC[2] ||
      rfid.uid.uidByte[3] != nuidPICC[3] ) {
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
    if (key == 'A') { // 키패드에서 A가 입력되면 모터 동작
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
    } else if (key == 'B') { // 키패드에서 B가 입력되면 서보모터 동작
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
    } else if (key == 'C') { // 키패드에서 C가 입력되면 모터 동작
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
    } else if (key == 'D') { // 키패드에서 D가 입력되면 LED 동작
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
          LED1.setPixelColor(i, LedBright, LedBright, LedBright);
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
    delay(500);
    servo1.detach();
  } else {
    servo1.attach(Servo1Pin);
    servo1.write(180);
    delay(500);
    servo1.detach();
  }
}

void Motor(int locked) {
  if (locked) {
    digitalWrite(MotorC, LOW);
    digitalWrite(MotorD, HIGH);
    analogWrite(MotorPWM, 55);
    delay(4300);
    digitalWrite(MotorC, LOW);
    digitalWrite(MotorD, LOW);
    analogWrite(MotorPWM, 55);
  } else {
    digitalWrite(MotorC, HIGH);
    digitalWrite(MotorD, LOW);
    analogWrite(MotorPWM, 55);
    delay(4300);
    digitalWrite(MotorC, LOW);
    digitalWrite(MotorD, LOW);
    analogWrite(MotorPWM, 55);
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
