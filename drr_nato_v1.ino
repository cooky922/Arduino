#include <LiquidCrystal_I2C.h> 

constexpr int led_green_pin = 7;
constexpr int led_yellow_pin = 8;
constexpr int led_red_pin = 12;
constexpr int buzzer_pin = 4;

bool buzzer_set = false;

namespace drr {
  enum class WarningSign {
    None,
    Green,    // green warning if wind_speed > 0.0
    Yellow,   // yellow warning if wind_speed > 4.0
    Red       // red warning if wind_speed > 6.0
  };

  class Anemometer {
    private:
      static constexpr unsigned long debounce_delay = 1000;  // the debounce time; increase if the output flickers
      static unsigned long s_last_debounce_time;
      static int s_anemo_count;

      static constexpr int s_pin_interrupt = 2;

    public:

      void init() {
        pinMode(s_pin_interrupt, INPUT_PULLUP);// set the interrupt pin
        attachInterrupt(
          digitalPinToInterrupt(s_pin_interrupt), 
          +[] { if (digitalRead(s_pin_interrupt) == LOW) s_anemo_count++;  }, 
          FALLING
        );
      }

      // returns wind speed in m/s
      float get_wind_speed() const {
        return (s_anemo_count * 8.75) / 100;
      }

      // get warning sign
      WarningSign get_warning_sign() const {
        const float ws = get_wind_speed();
        return ws > 6.0 ? WarningSign::Red :
               ws > 4.0 ? WarningSign::Yellow :
               ws > 0.0 ? WarningSign::Green :
                          WarningSign::None;
      }

      template <typename F>
      void invoke_if_measured(F f) {
        if ((millis() - s_last_debounce_time) > debounce_delay) {
          s_last_debounce_time = millis();
          f();
          s_anemo_count = 0;
        }
      }  
  };

  static unsigned long Anemometer::s_last_debounce_time = 0;
  static int Anemometer::s_anemo_count = 0;

  void switch_led_warning_sign(WarningSign sign) {
    switch (sign) {
      case WarningSign::None:
        digitalWrite(led_green_pin, LOW);
        digitalWrite(led_yellow_pin, LOW);
        digitalWrite(led_red_pin, LOW);
        break;
      case WarningSign::Green:
        digitalWrite(led_green_pin, HIGH);
        digitalWrite(led_yellow_pin, LOW);
        digitalWrite(led_red_pin, LOW);
        break;
      case WarningSign::Yellow:
        digitalWrite(led_green_pin, LOW);
        digitalWrite(led_yellow_pin, HIGH);
        digitalWrite(led_red_pin, LOW);
        break;
      case WarningSign::Red:
        digitalWrite(led_green_pin, LOW);
        digitalWrite(led_yellow_pin, LOW);
        digitalWrite(led_red_pin, HIGH);
        digitalWrite(buzzer_pin, HIGH);
        buzzer_set = true;
        break;
    }
  }
}

LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x3F for a 16 chars and 2 line display
drr::Anemometer anemo;
drr::WarningSign current_sign = drr::WarningSign::None;
 
void setup() {
  Serial.begin(9600);
  anemo.init();
  pinMode(led_green_pin, OUTPUT);
  pinMode(led_yellow_pin, OUTPUT);
  pinMode(led_red_pin, OUTPUT);
  pinMode(buzzer_pin, OUTPUT);

  lcd.init();
  lcd.clear();         
  lcd.backlight();
  lcd.setCursor(3, 0);
  lcd.print("Anemometer");
  delay(3000);
  lcd.clear();
}
 
void loop() {
  anemo.invoke_if_measured(+[]{
    const float wind_speed = anemo.get_wind_speed();
    const auto  w_sign     = anemo.get_warning_sign();

    lcd.clear();
    lcd.setCursor(0, 0);         
    lcd.print("Wind Speed");
    lcd.setCursor(0, 1);
    lcd.print(wind_speed);
    lcd.print("m/s");

    Serial.print(wind_speed);
    Serial.println("m/s");

    // switch LED pins
    if (w_sign != current_sign) {
      drr::switch_led_warning_sign(w_sign);
      current_sign = w_sign;
    }
  });
  delay(1);

  if (buzzer_set) {
    delay(1000);
    digitalWrite(buzzer_pin, LOW);
    buzzer_set = false;
  }

}