#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

int Button_T1 = 2; // Конопка для команды 1
int Button_T2 = 3; //  Кнопка для команды 2
int Button_L_Start = 4; // Запустить таймер для ведущего
int Button_L_Reset = 7; // Установить таймер для ведущего

int Led_T1 = 5; // Светодиод для команды 1
int Led_T2 = 6; // Светодиод для команды 2

int Analog_Led = A0; // For test вместо дисплея для таймера

boolean Is_Interrupt = false; // Если одна из команд нажала кнопку, то таймер станавливается (можно ли прерывать)
boolean Is_Led_T = false; // режим готовности (1 - светодиоды команд сброшены)(0- светодиоды команд не сброшеы)
boolean Step = false;// для кнопки старта таймера (0- разрешить старт таймера и звук фольстарта сбросив ledы, 1 - запустить таймер)

volatile int const Timer_Time_L = 0; // Время мигания светодиода (в дальнейшем для экрана таймера)нижняя граница
volatile int const Timer_Time_H = 20; //верхняя граница
int const Time = Timer_Time_H + 1; // Из за того что таймер работает не так как нужно (пропадает секунда)
volatile int i = Time; // Счетчик в цикле для мигания светодиодом Время мигания светодиода

int TonePin = 9; // Ножка для динамика

// Частоты для генерации звука
int T1_tone = 1000; // Кнопка команды 1
int T2_tone = 1000; // Кнопка команды 2
int Start_tone = 500; // Старт таймера
int FalseStart_tone = 150; // Фольтстарт

void Timer_Init() { // Инициальзация таймера
  cli(); // отключить глобальные прерывания
  TCCR1A = 0; // установить TCCR1A регистр в 0
  TCCR1B = 0;
  OCR1A = 15624; // установка регистра совпадения 7812 для 0.5 секунд
  TCCR1B |= (1 << WGM12); // включение в CTC режим

  // Установка битов CS10 и CS12 на коэффициент деления 1024
  TCCR1B |= (1 << CS10);
  TCCR1B |= (1 << CS12);
}

ISR(TIMER1_COMPA_vect) // Функция прерывания
{
  if (i-- > Timer_Time_L) {
    digitalWrite(Analog_Led, !digitalRead(Analog_Led));
    Serial.print(i);
    Serial.print(" ");
  } else {
    TIMSK1 &= ~(1 << OCIE1A); // turn off the timer interrupt
    i = Time;
    Is_Interrupt = false;
    tone(TonePin, T1_tone, 800);
    Serial.println("");
    Serial.println("Время вышло");
  }
}

// Инициализация светодиодов
void Led_Init() {
  pinMode(Led_T1, OUTPUT);
  pinMode(Led_T2, OUTPUT);
  pinMode(Analog_Led, OUTPUT); // test
}

// Инициализация кнопок
void Button_Init() {
  pinMode(Button_L_Start, INPUT_PULLUP);
  pinMode(Button_L_Reset, INPUT_PULLUP);
  pinMode(Button_T1, INPUT_PULLUP);
  pinMode(Button_T2, INPUT_PULLUP);
}

// Функции обработки прерываний с кнопок команд
void Team_1_pressed() {
  if (Is_Led_T) {
    tone(TonePin, FalseStart_tone, 550);
    Is_Led_T = false;
    digitalWrite(Led_T1, HIGH);
    Step = false;
  }
  if (Is_Interrupt)  {
    tone(TonePin, T1_tone, 550);
    Is_Interrupt = false;
    digitalWrite(Led_T1, HIGH);
  }
  //Serial.print(Is_Interrupt);
  //Serial.println(" Команда 1 нажала кнопку");
}

void Team_2_pressed() {
  if (Is_Led_T) {
    tone(TonePin, FalseStart_tone, 550);
    Is_Led_T = false;
    digitalWrite(Led_T2, HIGH);
    Step = false;
  }
  if (Is_Interrupt) {
    tone(TonePin, T2_tone, 550);
    Is_Interrupt = false;
    digitalWrite(Led_T2, HIGH);
  }
}


void Timer_Reset() {
  Is_Interrupt = false;
  TIMSK1 &= ~(1 << OCIE1A);
  i = Time;
  digitalWrite(Led_T1, LOW);
  digitalWrite(Led_T2, LOW);
  Serial.println("");
  Serial.println("Ведущий сбросил таймер");
  Is_Led_T = true;
  Step = true;
  LCD_set_time();
  delay(200);
}

void Timer_Start() {
  Is_Interrupt = true;
  //digitalWrite(Led_T1, LOW);
  //digitalWrite(Led_T2, LOW);
  Serial.println(" Ведущий запустил таймер");
}

void LCD_set_time() {
  lcd.clear();
  lcd.setCursor(7, 0);
  if (i > Timer_Time_H) {
    lcd.setCursor(7, 0);
    lcd.print(Timer_Time_H);
  } else {
    lcd.setCursor(7, 0);
    lcd.print(i);
  }
}

void setup() {
  noInterrupts();
  Led_Init();
  Button_Init();
  Serial.begin(9600);

  attachInterrupt(0, Team_1_pressed, FALLING);
  attachInterrupt(1, Team_2_pressed, FALLING);

  Timer_Init();
  interrupts();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(3, 0);
  lcd.print("Brain Ring");
}

void loop() {

  if (Is_Interrupt) {
    TIMSK1 |= (1 << OCIE1A);
    if (i > Timer_Time_H) {
      lcd.setCursor(7, 0);
      lcd.print(Timer_Time_H);
    } else {
      lcd.setCursor(7, 0);
      lcd.print(i);
    }

    if (digitalRead(Button_L_Reset) == LOW) {
      Timer_Reset();
    }
  } else {
    TIMSK1 &= ~(1 << OCIE1A);
    digitalWrite(Analog_Led, HIGH);
    if (digitalRead(Button_L_Start) == LOW) {
      if (!Step) {
        TIMSK1 &= ~(1 << OCIE1A); // turn off the timer interrupt
        LCD_set_time();
        Is_Led_T = true;
        digitalWrite(Led_T1, LOW);
        digitalWrite(Led_T2, LOW);
        Serial.println("пред запуск");
        Serial.println(Step);
        Step = !Step;
        delay(600);
      } else {
        tone(TonePin, Start_tone, 450);
        Timer_Start();
        Serial.println("запуск");
        Step = !Step;
        TIMSK1 |= (1 << OCIE1A);  // включение прерываний по совпадению
        sei();// включить глобальные прерывания
        //delay(100);
      }

    }
    if (digitalRead(Button_L_Reset) == LOW) {
      Timer_Reset();
    }
  }
}
