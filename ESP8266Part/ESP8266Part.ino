#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char* host = "ESP8266_1";
ESP8266WebServer server(80);

#include <Adafruit_NeoPixel.h>

#define PIN            2                  // Номер вывода МК, к которому подключен первый WS2812b
#define NUMPIXELS      2                  // Число светодиодов

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

//! Страничка HTML контроллера при обновлении прошивки
//! Будьте внимательны, при заполнении неккоректными данными Wi-Fi, и по включению режима обновления
//! беспроводной доступ к контроллеру может быть потерян 

const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";

//! Рабочий режим сети Wi-Fi (Дом)

const char *ssid =  "******";             // Имя вайфай точки доступа
const char *pass =  "********";           // Пароль от точки доступа

const char *mqtt_server = "192.168.0.41"; // Имя сервера MQTT
const int mqtt_port = 1883;               // Порт для подключения к серверу MQTT
const char *mqtt_user = "**";             // Логин от сервера
const char *mqtt_pass = "****";           // Пароль от сервера


//! Тестовый режим сети Wi-Fi (На работе)
/*
const char *ssid = "KBM";                      // Имя вайфай точки доступа
const char *pass = "23112311";                 // Пароль от точки доступа

const char *mqtt_server = "m10.cloudmqtt.com";  // Имя сервера MQTT
const int mqtt_port = 16029;                    // Порт для подключения к серверу MQTT
const char *mqtt_user = "kzvcomct";             // Логин от сервера
const char *mqtt_pass = "-mW311FSubSi";         // Пароль от сервера
*/

#define BUFFER_SIZE 400

												//! Для принятия команд по UART

char incomingBytes[5];

int pack1 = 0;
int pack2 = 0;
int pack3 = 0;
int pack4 = 0;

int com1 = 0;
int com2 = 0;

//! Настройки топиков и кнопок

const char *topic_1 = "/home/hall/state";
const char *topic_1PWM = "/home/hall/light/statePWM";

const char *topic_2 = "/home/bathroom/state";
const char *topic_2PWM = "/home/bathroom/light/statePWM";

const char *topic_update = "/home/hall/switch_1/update";

int button_1_state = 0;
int button_2_state = 0;

int button_1_statePWM = 0;
int button_2_statePWM = 0;

bool button_1_state_change = false;
bool button_2_state_change = false;

//! Функция получения данных от сервера MQTT

void callback(const MQTT::Publish& pub)
{
	String payload = pub.payload_string();

	if (String(pub.topic()) == topic_update)
	{
		web_update();
	}

	//! Первый топик

	if (String(pub.topic()) == topic_1)  // проверяем из нужного ли нам топика пришли данные
	{
		button_1_state = payload.toInt(); // преобразуем полученные данные в тип integer
		button_1_state_change = true;     // Флаг изменения топика
	}

	//! Второй топик

	if (String(pub.topic()) == topic_2)  // проверяем из нужного ли нам топика пришли данные
	{
		button_2_state = payload.toInt(); // преобразуем полученные данные в тип integer
		button_2_state_change = true;     // Флаг изменения топика
	}


}

WiFiClient wclient;
PubSubClient client(wclient, mqtt_server, mqtt_port);

void setup() {
	//  WiFi.mode(WIFI_AP_STA);   //w
	Serial.begin(115200);

	pixels.begin();

	indication_1();

}

//! Проверка наличия сообщения в UART, расшифровка, и принятие решения

void uart_check()
{
	if (Serial.available())   // Если что-то пришло по uart
	{
		Serial.readBytes(incomingBytes, 4);   // Читаем 4 байта

		pack1 = incomingBytes[0] - '0';       // Форматирование данных
		pack2 = incomingBytes[1] - '0';
		com1 = pack1 * 10 + pack2;

		pack3 = incomingBytes[2] - '0';
		pack4 = incomingBytes[3] - '0';
		com2 = pack3 * 10 + pack4;

		if ((com1 == 77) and (com2 == 77))                       // Включение прошивки по воздуху. Разрабатывается
		{
			web_update();
		}

		//! Первая кнопка ВКЛ-ВЫКЛ

		if ((com1 == 10) and (com2 == 10))
		{

			if (button_1_state == 0)
			{
				client.publish(topic_1, "1");
				pixels.setPixelColor(0, pixels.Color(0, 0, 200));
			}

			if (button_1_state == 1)
			{
				client.publish(topic_1, "0");
				pixels.setPixelColor(0, pixels.Color(150, 0, 0));
			}

			pixels.show();
		}

		//! Первая кнопка ШИМ

		if (com1 == 11)                      // Включение и отключение света в коридоре
		{
			button_1_statePWM = (com2 - 50)*(-5) + 50;

			if ((button_1_statePWM < 97) and (button_1_statePWM > 3))
			{

				client.publish(topic_1PWM, String(button_1_statePWM));

				pixels.setPixelColor(0, pixels.Color(255, 130, 0));
				pixels.show();

			}
		}

		//! Вторая кнопка

		if ((com1 == 20) and (com2 == 10))
		{

			if (button_2_state == 0)
			{
				client.publish(topic_2, "1");
				pixels.setPixelColor(1, pixels.Color(0, 0, 200));
			}

			if (button_2_state == 1)
			{
				client.publish(topic_2, "0");
				pixels.setPixelColor(1, pixels.Color(150, 0, 0));
			}
			pixels.show();
		}

		//! Вторая кнопка ШИМ

		if (com1 == 21)                      // Включение и отключение света в коридоре
		{
			button_2_statePWM = (com2 - 50)*(-5) + 50;

			if ((button_2_statePWM < 97) and (button_2_statePWM > 3))
			{

				client.publish(topic_2PWM, String(button_2_statePWM));

				pixels.setPixelColor(0, pixels.Color(255, 130, 0));
				pixels.show();

			}
		}


		if ((com1 == 77) and (com2 == 11))
		{
			pixels.setPixelColor(0, pixels.Color(255, 200, 0));
			pixels.setPixelColor(1, pixels.Color(255, 200, 0));
			pixels.show();
		}

		if ((com1 == 77) and (com2 == 12))
		{
			pixels.setPixelColor(0, pixels.Color(0, 0, 200));
			pixels.setPixelColor(1, pixels.Color(0, 0, 200));
			pixels.show();
			delay(400);
			pixels.setPixelColor(0, pixels.Color(0, 0, 0));
			pixels.setPixelColor(1, pixels.Color(0, 0, 0));
			pixels.show();
		}

	}
}

void web_update()
{
	MDNS.begin(host);
	server.on("/", HTTP_GET, []() {
		server.sendHeader("Connection", "close");
		server.sendHeader("Access-Control-Allow-Origin", "*");
		server.send(200, "text/html", serverIndex);
	});
	server.on("/update", HTTP_POST, []() {
		server.sendHeader("Connection", "close");
		server.sendHeader("Access-Control-Allow-Origin", "*");
		server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
		ESP.restart();
	}, []() {
		HTTPUpload& upload = server.upload();
		if (upload.status == UPLOAD_FILE_START) {
			Serial.setDebugOutput(true);
			WiFiUDP::stopAll();
			Serial.printf("Update: %s\n", upload.filename.c_str());
			uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
			if (!Update.begin(maxSketchSpace)) {//start with max available size
				Update.printError(Serial);
			}
		}
		else if (upload.status == UPLOAD_FILE_WRITE) {
			if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
				Update.printError(Serial);
			}
		}
		else if (upload.status == UPLOAD_FILE_END) {
			if (Update.end(true)) { //true to set the size to the current progress
				Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
			}
			else {
				Update.printError(Serial);
			}
			Serial.setDebugOutput(false);
		}
		yield();
	});

	server.begin();
	MDNS.addService("http", "tcp", 80);

	Serial.printf("Ready! Open http://%s.local in your browser\n", host);
	Serial.println(WiFi.localIP());

	while (1)
	{
		server.handleClient();
		delay(5);
	}
}

void indication_1()
{
	pixels.setPixelColor(0, pixels.Color(250, 0, 0));
	pixels.setPixelColor(1, pixels.Color(250, 0, 0));
	pixels.show();

	delay(400);

	Serial.println();
	Serial.println("Ok. This is just the beginning");
}

void indication_2()
{
	pixels.setPixelColor(0, pixels.Color(50, 50, 0));
	pixels.setPixelColor(1, pixels.Color(50, 50, 0));
	pixels.show();

	Serial.print("Connect to «");
	Serial.print(ssid);
	Serial.println("»");
}

void indication_3()
{
	Serial.print("Connected to «");
	Serial.print(ssid);
	Serial.println("»");

	Serial.print("Connected to «");
	Serial.print(mqtt_server);
	Serial.println("»");

	pixels.setPixelColor(0, pixels.Color(0, 0, 250));
	pixels.setPixelColor(1, pixels.Color(0, 0, 250));
	pixels.show();

	delay(600);

	pixels.setPixelColor(0, pixels.Color(0, 0, 0));
	pixels.setPixelColor(1, pixels.Color(0, 0, 0));
	pixels.show();
}

void loop() {

	if (WiFi.status() != WL_CONNECTED)
	{

		indication_2();

		WiFi.begin(ssid, pass);

		if (WiFi.waitForConnectResult() != WL_CONNECTED)
			return;
	}

	// подключаемся к MQTT серверу
	if (WiFi.status() == WL_CONNECTED)
	{
		if (!client.connected())
		{
			if (client.connect(MQTT::Connect("sensor_2").set_auth(mqtt_user, mqtt_pass)))
			{

				indication_3();

				client.set_callback(callback);
				client.subscribe(topic_1);
				client.subscribe(topic_2);
				client.subscribe(topic_1PWM);
				client.subscribe(topic_2PWM);
				client.subscribe(topic_update);
			}

		}

		if (client.connected())
			client.loop();
	}

	uart_check();

	if (button_1_state_change)
	{
		if (button_1_state == 1)
		{
			pixels.setPixelColor(0, pixels.Color(0, 0, 200));
		}

		if (button_1_state == 0)
		{
			pixels.setPixelColor(0, pixels.Color(150, 0, 0));
		}
		pixels.show();
		button_1_state_change = false;
	}

	if (button_2_state_change)
	{
		if (button_2_state == 1)
		{
			pixels.setPixelColor(1, pixels.Color(0, 0, 200));
		}

		if (button_2_state == 0)
		{
			pixels.setPixelColor(1, pixels.Color(150, 0, 0));
		}
		pixels.show();
		button_2_state_change = false;
	}

}