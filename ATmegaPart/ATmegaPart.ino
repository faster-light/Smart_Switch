int val = 0;                      // таймер ёмкости
boolean flag = 0;                  // значение кнопки
int avg = 0;
int avg_2 = 0;

int avg_old = 0;
int avg_old_2 = 0;

int ignore = 300;
int ignore_2 = 300;

void setup() {
//	pinMode(13, OUTPUT);      // это выход - светодиод
	Serial.begin(115200);
}

int check_button(int pin_number)
{
	for (int i = 0; i <= 20; i++)
	{
		pinMode(5, OUTPUT);       // устанавливаем ноль принудительно
		pinMode(5, INPUT);        // готовимся считывать значение

		while (digitalRead(5) == LOW) {
			val++;                    //считаем, за сколько зарядилась ёмкость
		}

		avg = avg + val;
		delay(1);
	}
}

void loop() {
	avg = 0;


	for (int i = 0; i <= 20; i++)
	{
		pinMode(5, OUTPUT);       // устанавливаем ноль принудительно
		pinMode(5, INPUT);        // готовимся считывать значение

		while (digitalRead(5) == LOW) {
			val++;                    //считаем, за сколько зарядилась ёмкость
		}

		avg = avg + val;
//		delay(1);
	}
	val = 0;
	for (int i = 0; i <= 20; i++)
	{
		pinMode(15, OUTPUT);       // устанавливаем ноль принудительно
		pinMode(15, INPUT);        // готовимся считывать значение

		while (digitalRead(15) == LOW) {
			val++;                    //считаем, за сколько зарядилась ёмкость
		}

		avg_2 = avg_2 + val;

//		delay(1);
	}

	delay(1);

	avg = avg / 20;
	avg_2 = avg_2 / 20;

	if (ignore > 0)
		ignore--;

	if (ignore_2 > 0)
		ignore_2--;

	if ((abs(avg_old - avg) > 20) and (ignore <= 0) and (ignore_2 <= 10))  
	{
		Serial.println("2010");
		Serial.println("2010");
		ignore = 50;
	}

	if ((abs(avg_old_2 - avg_2) > 30) and (ignore_2 <= 0) and (ignore <= 10))
	{
	
		Serial.println("1010");
		Serial.println("1010");
		ignore_2 = 50;
	}

//	Serial.println(avg);

	avg_old = avg;
	avg_old_2 = avg_2;

	val = 0;
}