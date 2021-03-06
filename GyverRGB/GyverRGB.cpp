#include "GyverRGB.h"
#include <Arduino.h>

volatile uint8_t pwmRGB;
volatile uint8_t pwmsRGB[20];
volatile boolean anyPWMpinsRGB[20];

GRGB::GRGB(uint8_t rpin, uint8_t gpin, uint8_t bpin) {
	// по умолчанию pwmmode NORM_PWM
	_rpin = rpin;
	_gpin = gpin;
	_bpin = bpin;
	pinMode(_rpin, OUTPUT);
	pinMode(_gpin, OUTPUT);
	pinMode(_bpin, OUTPUT);
}

GRGB::GRGB(uint8_t rpin, uint8_t gpin, uint8_t bpin, boolean pwmmode) {
	_PWMmode = pwmmode;
	
	_rpin = rpin;
	_gpin = gpin;
	_bpin = bpin;
	
	if (!_PWMmode) {	
		pinMode(_rpin, OUTPUT);
		pinMode(_gpin, OUTPUT);
		pinMode(_bpin, OUTPUT);
	} else {
		anyPWMinitRGB(6);	// частота ~150 Гц
			
		anyPWMpinRGB(_rpin);
		anyPWMpinRGB(_gpin);
		anyPWMpinRGB(_bpin);
	}
}

void GRGB::setDirection(boolean direction) {
	_reverse_flag = direction;
}

void GRGB::setRGB(uint8_t new_r, uint8_t new_g, uint8_t new_b) {
	_r = new_r;
	_g = new_g;
	_b = new_b;
	GRGB::setRGB();
}

void GRGB::setHEX(uint32_t color) {
	_r = (color >> 16) & 0xff;
	_g = (color >> 8) & 0xff;
	_b = color & 0xff;
	GRGB::setRGB();
}

void GRGB::setHSV_fast(uint8_t hue, uint8_t sat, uint8_t val) {
	byte h = ((24 * hue / 17) / 60) % 6;
	byte vmin = (long)val - val * sat / 255;
	byte a = (long)val * sat / 255 * (hue * 24 / 17 % 60) / 60;
	byte vinc = vmin + a;
	byte vdec = val - a;
	switch (h) {
		case 0: _r = val; _g = vinc; _b = vmin; break;
		case 1: _r = vdec; _g = val; _b = vmin; break;
		case 2: _r = vmin; _g = val; _b = vinc; break;
		case 3: _r = vmin; _g = vdec; _b = val; break;
		case 4: _r = vinc; _g = vmin; _b = val; break;
		case 5: _r = val; _g = vmin; _b = vdec; break;
	}	
	GRGB::setRGB();
}

void GRGB::setHSV(uint8_t h, uint8_t s, uint8_t v) {
	float r, g, b;
	
	float H = (float)h / 255;
	float S = (float)s / 255;
	float V = (float)v / 255;
	
    int i = int(H * 6);
    float f = H * 6 - i;
    float p = V * (1 - S);
    float q = V * (1 - f * S);
    float t = V * (1 - (1 - f) * S);

    switch(i % 6){
        case 0: r = V, g = t, b = p; break;
        case 1: r = q, g = V, b = p; break;
        case 2: r = p, g = V, b = t; break;
        case 3: r = p, g = q, b = V; break;
        case 4: r = t, g = p, b = V; break;
        case 5: r = V, g = p, b = q; break;
    }
	_r = r * 255;
	_g = g * 255;
	_b = b * 255;
	GRGB::setRGB();
}

// источник: http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/
void GRGB::setKelvin(int16_t temperature) {
  float tmpKelvin, tmpCalc;

  temperature = constrain(temperature, 1000, 40000);
  tmpKelvin = temperature / 100;

  // red
  if (tmpKelvin <= 66) _r = 255;
  else {
    tmpCalc = tmpKelvin - 60;
    tmpCalc = (float)pow(tmpCalc, -0.1332047592);
    tmpCalc *= (float)329.698727446;
    tmpCalc = constrain(tmpCalc, 0, 255);
    _r = tmpCalc;
  }

  // green
  if (tmpKelvin <= 66) {
    tmpCalc = tmpKelvin;
    tmpCalc = (float)99.4708025861 * log(tmpCalc) - 161.1195681661;
    tmpCalc = constrain(tmpCalc, 0, 255);
    _g = tmpCalc;
  } else {
    tmpCalc = tmpKelvin - 60;
    tmpCalc = (float)pow(tmpCalc, -0.0755148492);
    tmpCalc *= (float)288.1221695283;
    tmpCalc = constrain(tmpCalc, 0, 255);
    _g = tmpCalc;
  }

  // blue
  if (tmpKelvin >= 66) _b = 255;
  else if (tmpKelvin <= 19) _b = 0;
  else {
    tmpCalc = tmpKelvin - 10;
    tmpCalc = (float)138.5177312231 * log(tmpCalc) - 305.0447927307;
    tmpCalc = constrain(tmpCalc, 0, 255);
    _b = tmpCalc;
  }
  GRGB::setRGB();
}

// Для hex цветов
void GRGB::fadeTo(uint32_t newColor, uint16_t fadeTime) {
	// находим новые r g b
	byte new_r = (newColor >> 16) & 0xff;
	byte new_g = (newColor >> 8) & 0xff;
	byte new_b = newColor & 0xff;
	GRGB::fadeTo(new_r, new_g, new_b, fadeTime);
}

// Для r, g, b
void GRGB::fadeTo(byte new_r, byte new_g, byte new_b, uint16_t fadeTime) {
	// ищем изменение: новый цвет - текущий (м.б. отрицательным!)
	int deltaR = new_r - _r;
	int deltaG = new_g - _g;
	int deltaB = new_b - _b;

	// ищем наибольшее изменение по модулю
	int deltaMax = 0;
	if (abs(deltaR) > deltaMax) deltaMax = abs(deltaR);
	if (abs(deltaG) > deltaMax) deltaMax = abs(deltaG);
	if (abs(deltaB) > deltaMax) deltaMax = abs(deltaB);

	// Шаг изменения цвета
	float stepR = (float)deltaR / deltaMax;
	float stepG = (float)deltaG / deltaMax;
	float stepB = (float)deltaB / deltaMax;

	// Защита от деления на 0. Завершаем работу
	if (deltaMax == 0) return;

	// Расчет задержки в мкс
	uint32_t stepDelay = (float) 1000 * fadeTime / deltaMax;

	// Дробные величины для плавности, начальное значение = текущему у светодиода
	float thisR = _r, thisG = _g, thisB = _b;

	// Основной цикл изменения яркости
	for (int i = 0; i < deltaMax; i++) {
		thisR += stepR;
		thisG += stepG;
		thisB += stepB;
		
		_r = thisR;
		_g = thisG;
		_b = thisB;
		GRGB::setRGB();
		
		uint32_t us_timer = micros();
		while (micros() - us_timer <= stepDelay);
	}
}

// служебные функции
void GRGB::setRGB() {
	if (!_PWMmode) {						// режим NORM_PWM
		if (_reverse_flag) {				// обратная полярность ШИМ
			analogWrite(_rpin, 255 - _r);
			analogWrite(_gpin, 255 - _g);
			analogWrite(_bpin, 255 - _b);
		} else {							// прямая полярность ШИМ
			analogWrite(_rpin, _r);
			analogWrite(_gpin, _g);
			analogWrite(_bpin, _b);
		}
	} else {								// режим ANY_PWM
		if (_reverse_flag) {				// обратная полярность ШИМ
			anyPWMRGB(_rpin, 255 - _r);
			anyPWMRGB(_gpin, 255 - _g);
			anyPWMRGB(_bpin, 255 - _b);
		} else {							// прямая полярность ШИМ
			anyPWMRGB(_rpin, _r);
			anyPWMRGB(_gpin, _g);
			anyPWMRGB(_bpin, _b);
		}
	}
}

// ***************************** anyPWM *****************************

void anyPWMinitRGB(byte prescaler) // 1 - 7
{
	#if defined(__AVR_ATmega328P__)
	cli();
	TCCR2A = 0;   //при совпадении уровень OC1A меняется на противоположный
	TCCR2B = 5;   //CLK
	OCR2A = 1;
	TIMSK2 = 2;   //разрешаем прерывание по совпадению
	sei();
	TCCR2B = prescaler;   // prescaler
	#endif
}
 
void anyPWMpinRGB(uint8_t pin) {
	#if defined(__AVR_ATmega328P__)
	anyPWMpinsRGB[pin] = 1;
	pinMode(pin, OUTPUT);
	#endif
}

void anyPWMRGB(byte pin, byte duty)
{
	pwmsRGB[pin] = duty;
}

#if (defined(__AVR_ATmega328P__) && ALLOW_ANYPWM)
ISR(TIMER2_COMPA_vect)
{
  TCNT2 = 0;
  anyPWMpinsRGB[0] && pwmsRGB[0] > pwmRGB ? PORTD |= B00000001 : PORTD &= B11111110;
  anyPWMpinsRGB[1] && pwmsRGB[1] > pwmRGB ? PORTD |= B00000010 : PORTD &= B11111101;
  anyPWMpinsRGB[2] && pwmsRGB[2] > pwmRGB ? PORTD |= B00000100 : PORTD &= B11111011;
  anyPWMpinsRGB[3] && pwmsRGB[3] > pwmRGB ? PORTD |= B00001000 : PORTD &= B11110111;
  anyPWMpinsRGB[4] && pwmsRGB[4] > pwmRGB ? PORTD |= B00010000 : PORTD &= B11101111;
  anyPWMpinsRGB[5] && pwmsRGB[5] > pwmRGB ? PORTD |= B00100000 : PORTD &= B11011111;
  anyPWMpinsRGB[6] && pwmsRGB[6] > pwmRGB ? PORTD |= B01000000 : PORTD &= B10111111;
  anyPWMpinsRGB[7] && pwmsRGB[7] > pwmRGB ? PORTD |= B10000000 : PORTD &= B01111111;
  anyPWMpinsRGB[8] && pwmsRGB[8] > pwmRGB ? PORTB |= B00000001 : PORTB &= B11111110;
  anyPWMpinsRGB[9] && pwmsRGB[9] > pwmRGB ? PORTB |= B00000010 : PORTB &= B11111101;
  anyPWMpinsRGB[10] && pwmsRGB[10] > pwmRGB ? PORTB |= B00000100 : PORTB &= B11111011;
  anyPWMpinsRGB[11] && pwmsRGB[11] > pwmRGB ? PORTB |= B00001000 : PORTB &= B11110111;
  anyPWMpinsRGB[12] && pwmsRGB[12] > pwmRGB ? PORTB |= B00010000 : PORTB &= B11101111;
  anyPWMpinsRGB[13] && pwmsRGB[13] > pwmRGB ? PORTB |= B00100000 : PORTB &= B11011111;
  anyPWMpinsRGB[14] && pwmsRGB[14] > pwmRGB ? PORTC |= B00000001 : PORTC &= B11111110;
  anyPWMpinsRGB[15] && pwmsRGB[15] > pwmRGB ? PORTC |= B00000010 : PORTC &= B11111101;
  anyPWMpinsRGB[16] && pwmsRGB[16] > pwmRGB ? PORTC |= B00000100 : PORTC &= B11111011;
  anyPWMpinsRGB[17] && pwmsRGB[17] > pwmRGB ? PORTC |= B00001000 : PORTC &= B11110111;
  anyPWMpinsRGB[18] && pwmsRGB[18] > pwmRGB ? PORTC |= B00010000 : PORTC &= B11101111;
  anyPWMpinsRGB[19] && pwmsRGB[19] > pwmRGB ? PORTC |= B00100000 : PORTC &= B11011111;

  pwmRGB++;
}
#endif