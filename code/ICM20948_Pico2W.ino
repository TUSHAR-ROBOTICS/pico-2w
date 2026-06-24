/*
  ICM-20948 IMU with Raspberry Pi Pico 2W
  Arduino IDE | C++ | I2C | SparkFun ICM-20948 Library

  Wiring:
    ICM-20948 VCC  -> Pico 3V3 (Pin 36)
    ICM-20948 GND  -> Pico GND (Pin 38)
    ICM-20948 SDA  -> Pico GP4 (Pin 6)  [I2C0]
    ICM-20948 SCL  -> Pico GP5 (Pin 7)  [I2C0]
    ICM-20948 AD0  -> GND               [I2C addr = 0x68]
*/

#include <Wire.h>
#include "ICM_20948.h"

// ─── Configuration ────────────────────────────────────────────────────────────
#define I2C_SDA_PIN   4       // GP4
#define I2C_SCL_PIN   5       // GP5
#define I2C_CLOCK     400000  // 400 kHz Fast Mode
#define ICM_I2C_ADDR  0x68    // AD0 = GND → 0x68 | AD0 = 3V3 → 0x69
#define PRINT_DELAY   100     // ms between Serial prints

// ─── Globals ─────────────────────────────────────────────────────────────────
ICM_20948_I2C myICM;

// ─── Helper: print a float with label ────────────────────────────────────────
void printF(const char* label, float value, const char* unit, bool newline = false) {
  Serial.print(label);
  Serial.print(value, 3);
  Serial.print(unit);
  if (newline) Serial.println();
  else         Serial.print("  ");
}

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);   // Wait for USB Serial on Pico

  Serial.println("\n=== ICM-20948 + Pico 2W ===\n");

  // Init I2C on chosen pins
  Wire.setSDA(I2C_SDA_PIN);
  Wire.setSCL(I2C_SCL_PIN);
  Wire.begin();
  Wire.setClock(I2C_CLOCK);

  delay(200);  // Let sensor power up

  // Init ICM-20948
  bool initialized = false;
  while (!initialized) {
    myICM.begin(Wire, ICM_I2C_ADDR == 0x69 ? 1 : 0);
    // begin() second arg: 0 → AD0 low (0x68), 1 → AD0 high (0x69)

    Serial.print("ICM-20948 init status: ");
    Serial.println(myICM.statusString());

    if (myICM.status == ICM_20948_Stat_Ok) {
      initialized = true;
    } else {
      Serial.println("Retrying in 500 ms...");
      delay(500);
    }
  }

  Serial.println("ICM-20948 connected!\n");

  // ── Optional: configure full-scale ranges & DLPF ──────────────────────────
  // Disable I2C master (we're the master)
  myICM.startupDefault();

  // Accelerometer: ±4g
  ICM_20948_fss_t fss;
  fss.a = gpm4;      // ±4g  (options: gpm2, gpm4, gpm8, gpm16)
  fss.g = dps500;    // ±500 °/s (options: dps250, dps500, dps1000, dps2000)
  myICM.setFullScale((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), fss);

  // Enable DLPF on accel (low-pass filter, reduces noise)
  ICM_20948_dlpcfg_t dlpcfg;
  dlpcfg.a = acc_d473bw_n499bw;
  dlpcfg.g = gyr_d361bw4_n376bw5;
  myICM.setDLPFcfg((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), dlpcfg);

  ICM_20948_INTERNAL_PWR_MGMT_1_t pwr;
  pwr.CLKSEL = 1;                // Auto-select clock
  myICM.setBank(0);

  Serial.println("Accel | Gyro | Mag | Temp");
  Serial.println("--------------------------------");
}

// ─── Loop ────────────────────────────────────────────────────────────────────
void loop() {
  // Check for fresh data
  if (myICM.dataReady()) {
    myICM.getAGMT();  // Fetch Accel + Gyro + Mag + Temp

    // ── Accelerometer (mg) ─────────────────────────────────────────────────
    Serial.print("Accel [mg]  X:");
    Serial.print(myICM.accX(), 2);
    Serial.print("  Y:");
    Serial.print(myICM.accY(), 2);
    Serial.print("  Z:");
    Serial.println(myICM.accZ(), 2);

    // ── Gyroscope (°/s) ───────────────────────────────────────────────────
    Serial.print("Gyro  [d/s] X:");
    Serial.print(myICM.gyrX(), 2);
    Serial.print("  Y:");
    Serial.print(myICM.gyrY(), 2);
    Serial.print("  Z:");
    Serial.println(myICM.gyrZ(), 2);

    // ── Magnetometer (µT) ─────────────────────────────────────────────────
    Serial.print("Mag   [uT]  X:");
    Serial.print(myICM.magX(), 2);
    Serial.print("  Y:");
    Serial.print(myICM.magY(), 2);
    Serial.print("  Z:");
    Serial.println(myICM.magZ(), 2);

    // ── Temperature (°C) ──────────────────────────────────────────────────
    Serial.print("Temp  [C]  ");
    Serial.println(myICM.temp(), 2);

    Serial.println("---");
  } else {
    Serial.println("Waiting for data...");
  }

  delay(PRINT_DELAY);
}
