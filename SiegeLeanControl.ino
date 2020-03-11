// Siege Leaning Controls
#include <math.h>
#include <Keyboard.h>
#include <EEPROM.h>

// Analog Pins
#define Pin_X A0
#define Pin_Y A1
#define Pin_Z A2
#define Pin_Threshold A3
#define Pin_Target A4

// Digital Pins
#define Pin_Switch 2
#define Pin_Led_L 3
#define Pin_Led_R 4
#define Pin_Button 5
#define Pin_Led 13

// Constants
#define S 0.330 // Sensitivity [V/g]
#define Vref 3.3 // Analog Reference Voltage [V]

// Variables
int Lean_Left, Lean_Right, Offset_X, Offset_Y, Offset_Z;
bool buttonRead = 1, buttonLast = 1;

void setup() {
  Serial.begin(9600);
  Keyboard.begin();

  // Digital Inputs
  pinMode(Pin_Switch, INPUT);
  pinMode(Pin_Button, INPUT);
  pinMode(Pin_Led_L, OUTPUT);
  pinMode(Pin_Led_R, OUTPUT);
  pinMode(Pin_Led, OUTPUT);
}

void loop() {
  int Switch_Enable = digitalRead(Pin_Switch);

  Serial.write(27); // ESC command
  Serial.print("[2J"); // clear screen command
  Serial.write(27);// ESC command
  Serial.print("[H"); // cursor to home command
  Serial.print("Enable Switch: ");
  Serial.print(Switch_Enable);

  if (Switch_Enable == HIGH) {
    digitalWrite(Pin_Led, HIGH);

    // Accelerometer Target/Threshold
    float Adc_Target = analogRead(Pin_Target);
    float Adc_Threshold = analogRead(Pin_Threshold);

    float Ang_Target = (Adc_Target / 1023) * 25 + 5; // ADC to Angle
    float Ang_Threshold = (Adc_Threshold / 1023) * 5; // ADC to Angle

    // Accelerometer Calibration
    int Adc_X = analogRead(Pin_X);
    int Adc_Y = analogRead(Pin_Y);
    int Adc_Z = analogRead(Pin_Z);

    buttonRead = !digitalRead(Pin_Button);
    if (buttonRead != buttonLast && buttonRead == HIGH) {
      Offset_X = 512 - Adc_X;
      Offset_Y = 512 - Adc_Y;
      Offset_Z = 648 - Adc_Z;
      EEPROM.write(0, Offset_X);
      EEPROM.write(1, Offset_Y);
      EEPROM.write(2, Offset_Z);
    } else {
      Offset_X = EEPROM.read(0);
      Offset_Y = EEPROM.read(1);
      Offset_Z = EEPROM.read(2);
    }
    buttonLast = buttonRead;

    int Adc_X_Corr = Adc_X + Offset_X;
    int Adc_Y_Corr = Adc_Y + Offset_Y;
    int Adc_Z_Corr = Adc_Z + Offset_Z;

    // Calculate Accelerations
    float Acc_X = (((Adc_X_Corr * Vref) / 1024) - (Vref / 2)) / S;
    float Acc_Y = (((Adc_Y_Corr * Vref) / 1024) - (Vref / 2)) / S;
    float Acc_Z = (((Adc_Z_Corr * Vref) / 1024) - (Vref / 2)) / S;

    // Calculate Angles
    float Theta = atan(Acc_X / sqrt(sq(Acc_Y) + sq(Acc_Z))) * (180 / M_PI);
    float Psi = atan(Acc_Y / sqrt(sq(Acc_X) + sq(Acc_Z))) * (180 / M_PI);
    float Phi = atan(sqrt(sq(Acc_X) + sq(Acc_Y)) / Acc_Z) * (180 / M_PI);

    // Latch Leaning Command
    if (Theta > Ang_Target) {
      Lean_Left = 1;
      digitalWrite(Pin_Led_L, HIGH);
      Keyboard.press(KEY_F7);
    }
    if (Theta < Ang_Target - Ang_Threshold) {
      Lean_Left = 0;
      digitalWrite(Pin_Led_L, LOW);
      Keyboard.release(KEY_F7);
    }

    if (Theta < -Ang_Target) {
      Lean_Right = 1;
      digitalWrite(Pin_Led_R, HIGH);
      Keyboard.press(KEY_F9);
    }
    if (Theta > -Ang_Target + Ang_Threshold) {
      Lean_Right = 0;
      digitalWrite(Pin_Led_R, LOW);
      Keyboard.release(KEY_F9);
    }
    Serial.print("\n\n                 |   X   |    Y    |    Z");
    Serial.print("\n--------------------------------------------");

    Serial.print("\nRaw Bit Value |  ");
    Serial.print(Adc_X);
    Serial.print("  |   ");
    Serial.print(Adc_Y);
    Serial.print("   |   ");
    Serial.print(Adc_Z);

    Serial.print("\nBit Offset |  ");
    Serial.print(EEPROM.read(0));
    Serial.print("  |   ");
    Serial.print(EEPROM.read(1));
    Serial.print("   |   ");
    Serial.print(EEPROM.read(2));

    Serial.print("\nCorr. Bit Value |  ");
    Serial.print(Adc_X_Corr);
    Serial.print("  |   ");
    Serial.print(Adc_Y_Corr);
    Serial.print("   |   ");
    Serial.print(Adc_Z_Corr);

    Serial.print("\nAcceleration [g] | ");
    Serial.print(Acc_X);
    Serial.print("  |  ");
    Serial.print(Acc_Y);
    Serial.print("  |  ");
    Serial.print(Acc_Z);

    Serial.print("\nTilt Angle [Deg] | ");
    Serial.print(Theta);
    Serial.print(" | ");
    Serial.print(Psi);
    Serial.print("   |  ");
    Serial.print(Phi);

    Serial.print("\nL: ");
    Serial.print(Lean_Left);
    Serial.print("\t   R: ");
    Serial.print(Lean_Right);
    Serial.print("\n\n");

    Serial.print("------Calibration------");
    Serial.print("\nButton Read: ");
    Serial.print(buttonRead);
    Serial.print("\nButton Last: ");
    Serial.print(buttonLast);
    Serial.print("\nTarget: ");
    Serial.print(Ang_Target);
    Serial.print(" Deg.\nThreshold: ");
    Serial.print(Ang_Threshold);
    Serial.print(" Deg.");
  } else {
    Lean_Left = 0;
    Lean_Right = 0;

    digitalWrite(Pin_Led, LOW);
    digitalWrite(Pin_Led_L, LOW);
    digitalWrite(Pin_Led_R, LOW);
  }
  //delay(100); // 10 Hz Refresh rate for serial monitor
}
