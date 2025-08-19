/*

set param

плата: 
все інше по дефолту

Програматор: 

*/
#define VERS 1.18  // Added PIN field support
#define DEF_SSN       751236933

//#define INIT_MAGIC    0xA5	// код оновлення базових параметрів, після оновлення прошити ще раз з кодом 0x5A
#define INIT_MAGIC    0x5A	// базовий код ініціалізації

#include <Button2.h>
#include <Ticker.h>
#include <EEPROM.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "WiFi.h"
#include <WiFiClientSecure.h>
#include <WiFiAP.h>
#include "date_time.h"

// Default pin/ID values (from defines)
#define DEF_IN_1      34
#define DEF_OUT_1     12
#define DEF_OUT_2     14
#define DEF_OUT_3     27
#define DEF_OUT_4     26
#define DEF_OUT_5     25
#define DEF_OUT_6     33
#define DEF_OUT_7     32
#define DEF_OUT_8     13

#define DEF_BUTTON_1  4
#define DEF_BUTTON_2  16
#define DEF_BUTTON_3  17
#define DEF_BUTTON_4  5
#define DEF_BUTTON_5  18
#define DEF_BUTTON_6  23
#define DEF_BUTTON_7  19
#define DEF_BUTTON_8  22

#define DEF_RELAY_ON  0
#define DEF_RELAY_OFF 1

#define WFF

bool st_con;

// EEPROM addresses for hardware config
#define ADR_SSID      {0, 50}
#define ADR_PASS      {50, 50}
#define ADR_IP        {100, 20}
#define ADR_SUB       {120, 20}
#define ADR_GATE      {140, 20}
#define ADR_DNS1      {160, 20}
#define ADR_DNS2      {180, 20}
#define ADR_DHCPF     {200, 3}
#define ADR_HOST      {203, 50}
#define ADR_PORT      {253, 7}
#define ADR_LOGIN     {260, 32}      // login (max 32)
#define ADR_PASSWORD  {292, 32}   // password (max 32)
#define ADR_SESSION   {324, 32}    // X-Session-ID (max 32, для 64-бітного числа достатньо 20)
#define ADR_IN_1       356
#define ADR_OUT_1      357
#define ADR_OUT_2      358
#define ADR_OUT_3      359
#define ADR_OUT_4      360
#define ADR_OUT_5      361
#define ADR_OUT_6      362
#define ADR_OUT_7      363
#define ADR_OUT_8      364
#define ADR_BUTTON_1   365
#define ADR_BUTTON_2   366
#define ADR_BUTTON_3   367
#define ADR_BUTTON_4   368
#define ADR_BUTTON_5   369
#define ADR_BUTTON_6   370
#define ADR_BUTTON_7   371
#define ADR_BUTTON_8   372
#define ADR_SSN        373
#define ADR_RELAY_ON   377
#define ADR_RELAY_OFF  378
#define ADR_INIT_MAGIC 379
#define ADR_TIMEZONE  {380, 32}
#define ADR_PIN       {412, 2}   // 2 bytes for short int PIN

#define EEPROM_SIZE   414

#define USER_AGENT "ESP32"

struct EEPROM_Address {
	int adr;
	int length;
};

String ip_adr;
String ip_sub;
String ip_gate;
String ip_dns1;
String ip_dns2;
String timezone;

IPAddress  ip_adr_ob;
IPAddress  ip_sub_ob;
IPAddress  ip_gate_ob;
IPAddress  ip_dns1_ob;
IPAddress  ip_dns2_ob;
IPAddress  ip_ob;

String host;
String port;
String dhcp_f;

String server_login;
String server_password;
String session_id;
// Глобальні змінні для роботи з EEPROM
int in_1, out_1, out_2, out_3, out_4, out_5, out_6, out_7, out_8;
int button_1, button_2, button_3, button_4, button_5, button_6, button_7, button_8;
uint32_t ssn;
int relay_on, relay_off;
uint32_t timestamp;
byte day, month, dayOfWeek, hour, minute, second;
int timeOffset;
bool check_fw_update_flag;
unsigned short pin;  // PIN code for authentication

// Ticker для інкрементації timestamp кожну секунду
Ticker timestampTicker;

WiFiClientSecure client;
// Для HTTPS без перевірки сертифіката
// client.setInsecure() буде викликано у setup()

// Масив для кнопок (заповнюється після ініціалізації EEPROM)
uint8_t g_btns[8];
String log_file; // Лог-файл для збереження подій
#define MAX_LOG_SIZE 1024 * 32 // Максимальний розмір логу (32КБ)
// Функція ініціалізації HW-конфігу з EEPROM
void init_hw_config_from_eeprom() {
	uint8_t v;
	uint8_t magic = EEPROM.read(ADR_INIT_MAGIC);
	if(magic != INIT_MAGIC) {
		// INIT: Записати дефолти
		// IN_1
		in_1 = DEF_IN_1;
		EEPROM.write(ADR_IN_1, in_1);
		// OUT_1-8
		uint8_t outs[8] = {DEF_OUT_1, DEF_OUT_2, DEF_OUT_3, DEF_OUT_4, DEF_OUT_5, DEF_OUT_6, DEF_OUT_7, DEF_OUT_8};
		int* outs_ptr[8] = {&out_1,&out_2,&out_3,&out_4,&out_5,&out_6,&out_7,&out_8};
		for(int i=0;i<8;i++) {
			*outs_ptr[i] = outs[i];
			EEPROM.write(ADR_OUT_1+i, outs[i]);
		}
		// BUTTON_1-8
		uint8_t btns[8] = {DEF_BUTTON_1, DEF_BUTTON_2, DEF_BUTTON_3, DEF_BUTTON_4, DEF_BUTTON_5, DEF_BUTTON_6, DEF_BUTTON_7, DEF_BUTTON_8};
		int* btns_ptr[8] = {&button_1,&button_2,&button_3,&button_4,&button_5,&button_6,&button_7,&button_8};
		for(int i=0;i<8;i++) {
			*btns_ptr[i] = btns[i];
			EEPROM.write(ADR_BUTTON_1+i, btns[i]);
			g_btns[i] = *btns_ptr[i];
		}
		// SSN
		ssn = DEF_SSN;
		for(int i=0;i<4;i++) EEPROM.write(ADR_SSN+i, (DEF_SSN>>(8*i))&0xFF);
		// RELAY ON/OFF
		relay_on = DEF_RELAY_ON;
		relay_off = DEF_RELAY_OFF;
		EEPROM.write(ADR_RELAY_ON, relay_on);
		EEPROM.write(ADR_RELAY_OFF, relay_off);
		// MAGIC
		EEPROM.write(ADR_INIT_MAGIC, INIT_MAGIC);
		write_str_eepr(ADR_SESSION, "11111111111");
		// Initialize PIN to 0
		pin = 0;
		EEPROM.write(ADR_PIN.adr, (byte)(pin & 0xFF));
		EEPROM.write(ADR_PIN.adr + 1, (byte)((pin >> 8) & 0xFF));
		EEPROM.commit();
		ESP.restart();
	} else {
		in_1 = EEPROM.read(ADR_IN_1);
		int* outs_ptr[8] = {&out_1,&out_2,&out_3,&out_4,&out_5,&out_6,&out_7,&out_8};
		for(int i=0;i<8;i++) {
			*outs_ptr[i] = EEPROM.read(ADR_OUT_1+i);
		}
		int* btns_ptr[8] = {&button_1,&button_2,&button_3,&button_4,&button_5,&button_6,&button_7,&button_8};
		for(int i=0;i<8;i++) {
			*btns_ptr[i] = EEPROM.read(ADR_BUTTON_1+i);
			g_btns[i] = *btns_ptr[i];
		}
		ssn = 0;
		for(int i=0;i<4;i++) {
			v = EEPROM.read(ADR_SSN+i);
			ssn |= ((uint32_t)v) << (8*i);
		}
		relay_on = EEPROM.read(ADR_RELAY_ON);
		relay_off = EEPROM.read(ADR_RELAY_OFF);
		// Read PIN from EEPROM
		pin = (short)(EEPROM.read(ADR_PIN.adr) | (EEPROM.read(ADR_PIN.adr + 1) << 8));
	}
}

// Далі у коді використовуємо in_1, out_1 ... ssn, relay_on, relay_off замість дефайнів

#include <HTTPClient.h>
#include <Update.h>
#include <mbedtls/md.h>

String getDateTimeString();
void addLogEntry(const String& entry, const String& data_string);
void sendLogToServer();
void logRelayChange(int relayNumber, bool isOn, bool isLocal, const String& data_string);
void logClockInitialization(const String& data_string);
void logError(const String& errorString);
void logSystemEvent(const String& eventMessage, const String& data_string);

void check_fw_update() {
	HTTPClient http;
	String url = "https://" + host + "/relay_servak/firmware_update_dir/firmware_vers.txt";
	http.begin(client, url);
	http.setUserAgent(USER_AGENT);
	if(session_id.length() > 0) {
		http.addHeader("Cookie", "X-Session-ID=" + session_id);
	}
	int httpCode = http.GET();
	if(httpCode == 200) {
		String new_ver_str = http.getString();
		int ver_int, ver_frac;
		float new_ver_float = 0.0;
		sscanf(new_ver_str.c_str(), "%f", &new_ver_float);
		
		if(new_ver_float > VERS + 0.005) { // Додаємо невелику похибку для надійного порівняння
			logSystemEvent("[updt:" + String(__LINE__) + "] Found new firmware version: " + String(new_ver_float, 2) + " (" + String(VERS, 2) + " is current)", getDateTimeString());
			http.end();

			// Отримати SHA256
			String sha_url = "https://" + String(host) + "/relay_servak/firmware_update_dir/firmware_update.sha256";
			http.begin(client, sha_url);
			http.setUserAgent(USER_AGENT);
			if(session_id.length() > 0) {
				http.addHeader("Cookie", "X-Session-ID=" + session_id);
			}
			int shaCode = http.GET();
			if(shaCode != 200) {
				logError("[updt:" + String(__LINE__) + "] Failed to download firmware hash!");
				http.end();
				return;
			}
			String expected_sha256 = http.getString();
			expected_sha256.trim();
			int spaceIndex = expected_sha256.indexOf(' ');
			if(spaceIndex != -1) {
				expected_sha256 = expected_sha256.substring(0, spaceIndex);
			}
			http.end();

			// Скачати firmware
			String bin_url = "http://" + String(host) + "/relay_servak/firmware_update_dir/firmware_update.bin";
			http.begin(client, bin_url);
			http.setUserAgent(USER_AGENT);
			if(session_id.length() > 0) {
				http.addHeader("Cookie", "X-Session-ID=" + session_id);
			}
			int binCode = http.GET();
			if(binCode == 200) {
				int contentLength = http.getSize();
				WiFiClient * stream = http.getStreamPtr();

				// Підготувати SHA256
				mbedtls_md_context_t sha_ctx;
				const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
				unsigned char result[32];
				mbedtls_md_init(&sha_ctx);
				mbedtls_md_setup(&sha_ctx, md_info, 0);
				mbedtls_md_starts(&sha_ctx);

				if(Update.begin(contentLength)) {
					size_t written = 0;
					uint8_t buff[512];
					while (http.connected() && (contentLength > 0 || contentLength == -1)) {
						size_t len = stream->available();
						if(len) {
							int c = stream->readBytes(buff, (len > sizeof(buff)) ? sizeof(buff) : len);
							Update.write(buff, c);
							mbedtls_md_update(&sha_ctx, buff, c);
							if(contentLength > 0) contentLength -= c;
							written += c;
						}
						delay(1);
					}
					// Фініш хешу
					mbedtls_md_finish(&sha_ctx, result);
					mbedtls_md_free(&sha_ctx);

					// Перетворити у hex рядок
					String computed_hash;
					for(int i = 0; i < 32; i++) {
						char hex[3];
						sprintf(hex, "%02x", result[i]);
						computed_hash += hex;
					}

					logSystemEvent("[updt:" + String(__LINE__) + "] Expected SHA256: " + expected_sha256);
					logSystemEvent("[updt:" + String(__LINE__) + "] Computed SHA256: " + computed_hash);

					if(computed_hash.equalsIgnoreCase(expected_sha256)) {
						if(Update.end(true)) {
							logSystemEvent("[updt:" + String(__LINE__) + "] Update successful, rebooting...");
							ESP.restart();
						} else {
							logError("[updt:" + String(__LINE__) + "] Update failed at end!");
						}
					} else {
						logError("[updt:" + String(__LINE__) + "] SHA256 mismatch, aborting update!");
						Update.abort();
					}
				} else {
					logError("[updt:" + String(__LINE__) + "] Not enough space for update!");
				}
			} else {
				logError("[updt:" + String(__LINE__) + "] Failed to download firmware bin");
			}
			http.end();
			return;
		}
		else 
		logSystemEvent("[updt:" + String(__LINE__) + "] No new firmware version found");
	}
	http.end();
	return;
}


inline String conv_str(const String& x) {
	return convert_string(x.c_str(), x.length());
}


inline String conv_str(const char* x) {
	return convert_string(x, strlen(x));
}


class MyRand {
	uint32_t a;
	
	public:
		MyRand() {
			a = 0xad20f153;
		}
		uint32_t rand() {
			uint32_t b = a;
			b ^= b << 13;
			b ^= b >> 17;
			b ^= b << 5;
			return a = b;
		}
		void srand(uint32_t i) {
			a = i ^ 0xad20f153;
		}
		void correct(uint32_t i) {
			a = i ^ a;
		}
};

MyRand my_rand;

const byte DNS_PORT = 53;

uint8_t tmr;
bool status_init;
int contr_sum, cs;

uint8_t state = 0;
Button2 *pBtns = nullptr;
Ticker btnscanT, btnscanT2;

// Вказуємо ім'я та пароль WiFi-мережі
char ssid1[20] = "ceh1_el";
//char ssid1[20] = "Redmi Note 7";
String ssid3;
char ssid4[20] = "esp32at";
char password1[20] = "24185870";
String password3;
//char password4[20] = "24185870";
char password4[20] = "";

byte status_pin_out[8];
byte status_pin_relay[8];
int status_in;
byte online;
char reqest[999];

bool reconnect_wf_f;

WiFiServer server(80);
DNSServer dnsServer;

void button_handle(uint8_t gpio)
{
	if(gpio == button_1) {
		state = 1;
	} else if(gpio == button_2) {
		state = 2;
	} else if(gpio == button_3) {
		state = 3;
	} else if(gpio == button_4) {
		state = 4;
	} else if(gpio == button_5) {
		state = 5;
	} else if(gpio == button_6) {
		state = 6;
	} else if(gpio == button_7) {
		state = 7;
	} else if(gpio == button_8) {
		state = 8;
	}
}

void button_callback(Button2 &b)
{
	for (int i = 0; i < sizeof(g_btns) / sizeof(g_btns[0]); ++i) {
		if (pBtns[i] == b) {
			button_handle(pBtns[i].getPin());
		}
	}
}


void button_init()
{
	uint8_t args = sizeof(g_btns) / sizeof(g_btns[0]);
	pBtns = new Button2 [args];
	for (int i = 0; i < args; ++i) {
		pBtns[i] = Button2(g_btns[i]);
		pBtns[i].setPressedHandler(button_callback);
	}
}

void button_loop() {
	for (int i = 0; i < sizeof(g_btns) / sizeof(g_btns[0]); ++i) {
		pBtns[i].loop();
	}
	if(tmr)
		tmr--;
	else
		tmr = online == 0 ? 200 : 5;
}

// Функція для інкрементації timestamp кожну секунду
void updateTimestamp() {
	if(timestamp != 0) {
		timestamp++;
		if(timestamp % 86400 == 3661) {
			check_fw_update_flag = true;
		}
	}
}


String read_str_eepr(EEPROM_Address addr, char* str) {
	String result = "";
	int i;
	char c;
	for(i = 0; i < addr.length - 1; i++) {
		c = (char)EEPROM.read(i + addr.adr);
		if(c == 0)
			break;
		if(str != NULL)
			*(&(str[0]) + i) = c;
		result += c;
	}
	if(str != NULL)
		*(&(str[0]) + i) = 0;
	return result;
}


void eeprom_update(int adr, byte val) {
	if(EEPROM.read(adr) != val) {
		EEPROM.write(adr, val);
	}
}

void write_str_eepr(EEPROM_Address addr, const char* str) {
	int i;
	for(i = 0; i < addr.length - 1 && *(&(str[0]) + i) != 0; i++) {
		eeprom_update(i + addr.adr, (char)(*(&(str[0]) + i)));
	}
	eeprom_update(i + addr.adr, (char)0);
	EEPROM.commit();
}


bool authorize_on_server() {
	client.setInsecure();
	String sid = session_id;
	String login = server_login;
	String password = server_password;
	String host_str = host;
	int port_num = port.toInt();
	if(host_str.length() == 0 || port_num == 0) return 0;
	
	// --- 1. Перевірка існуючої сесії ---
	if (!client.connect(host_str.c_str(), port_num)) {
		logError("[auth:" + String(__LINE__) + "] Server is unavailable");
		return 0;
	}
	String get_req = "GET /?authorization=check HTTP/1.1\r\n";
	get_req += "Host: " + host_str + "\r\n";
	get_req += "User-Agent: " + String(USER_AGENT) + "\r\n";
	if(sid.length() > 0) get_req += "Cookie: X-Session-ID=" + sid + "\r\n";
	if(timezone.length() > 0) get_req += "X-Timezone: " + timezone + "\r\n";
	get_req += "Connection: close\r\n\r\n";
	client.print(get_req);

	unsigned long timeout = millis();
	while (client.connected() && !client.available() && millis() - timeout < 3000) { delay(1); }
	if (!client.available()) {
		logError("[auth:" + String(__LINE__) + "] GET timeout");
		client.stop();
		return 0;
	}
	String response = client.readString();
	int idxDate = response.indexOf("Date: ");
	if(idxDate > 0) {
		int end = response.indexOf('\r', idxDate);
		String new_date = response.substring(idxDate, end);
		timestamp = dateStrToTimestamp(new_date.c_str());
	}
	int idxTimeOffset = response.indexOf("X-Timezone-Offset: ");
	if(idxTimeOffset > 0) {
		idxTimeOffset += strlen("X-Timezone-Offset: ");
		int end = response.indexOf('\r', idxTimeOffset);
		String new_timeOffset = response.substring(idxTimeOffset, end);
		timeOffset = new_timeOffset.toInt();
	}
	if(timestamp != 0) {
		logClockInitialization(getDateTimeString());
	}
	if (response.indexOf("200 OK") > 0) {
		logSystemEvent("[auth:" + String(__LINE__) + "] Session is available (200 OK)");
		client.stop();
		return 1;
	}
	if (response.indexOf("400 Bad Request") > 0) {
		logSystemEvent("[auth:" + String(__LINE__) + "] Session is not available (400 Bad Request)");
		client.stop();
		// --- 2. POST авторизація ---
		if (!client.connect(host_str.c_str(), port_num)) {
			logError("[auth:" + String(__LINE__) + "] Server is unavailable (POST)");
			return 0;
		}
		String body = "reestr=false&login=" + login + "&password=" + password;
		String post_req = "POST / HTTP/1.1\r\n";
		post_req += "Host: " + host_str + "\r\n";
		post_req += "User-Agent: " + String(USER_AGENT) + "\r\n";
		if(timezone.length() > 0) post_req += "X-Timezone: " + timezone + "\r\n";
		post_req += "Content-Type: application/x-www-form-urlencoded\r\n";
		post_req += "Content-Length: " + String(body.length()) + "\r\n";
		post_req += "Connection: close\r\n\r\n";
		post_req += body;
		client.print(post_req);
		timeout = millis();
		while (client.connected() && !client.available() && millis() - timeout < 3000) { delay(1); }
		if (!client.available()) {
			logError("[auth:" + String(__LINE__) + "] POST timeout");
			client.stop();
			return 0;
		}
		String post_resp = client.readString();
		if (post_resp.indexOf("200 OK") > 0) {
			int idx = post_resp.indexOf("Set-Cookie: X-Session-ID=");
			if(idx > 0) {
				idx += strlen("Set-Cookie: X-Session-ID=");
				int end = post_resp.indexOf(';', idx);
				String new_sid = post_resp.substring(idx, end);
				session_id = new_sid;
				//write_str_eepr(ADR_SESSION, session_id.c_str());
				logSystemEvent("[auth:" + String(__LINE__) + "] New session_id: " + session_id);
				client.stop();
				return 1;
			}
		}
		if (post_resp.indexOf("500") > 0) {
			logError("[auth:" + String(__LINE__) + "] Invalid login/password");
			status_in = 1;
			client.stop();
			return 0;
		}
		client.stop();
		return 0;
	}
	client.stop();
	return 0;
}


bool connect_wf_2() {
	ssid3 = read_str_eepr(ADR_SSID, NULL);
	password3 = read_str_eepr(ADR_PASS, NULL);
	ip_adr = read_str_eepr(ADR_IP, NULL);
	ip_sub = read_str_eepr(ADR_SUB, NULL);
	ip_gate = read_str_eepr(ADR_GATE, NULL);
	ip_dns1 = read_str_eepr(ADR_DNS1, NULL);
	ip_dns2 = read_str_eepr(ADR_DNS2, NULL);
	host = read_str_eepr(ADR_HOST, NULL);
	port = read_str_eepr(ADR_PORT, NULL);
	dhcp_f = read_str_eepr(ADR_DHCPF, NULL);

	WiFi.begin(ssid3.c_str(), password3.c_str());
	if(dhcp_f.c_str()[0] == '0') {
		ip_adr_ob.fromString(ip_adr);
		ip_gate_ob.fromString(ip_gate);
		ip_sub_ob.fromString(ip_sub);
		ip_dns1_ob.fromString(ip_dns1);
		ip_dns2_ob.fromString(ip_dns2);
		WiFi.config(ip_adr_ob, ip_gate_ob, ip_sub_ob, ip_dns1_ob, ip_dns2_ob);
	}
	logSystemEvent("[wifi:" + String(__LINE__) + "] Connecting to WiFi: " + ssid3);

	byte ii = 0;
	while(WiFi.status() != WL_CONNECTED) {
		ii++;
		if(ii == 10 && !st_con)
			break;
		delay(500);
	}
	if(WiFi.status() == WL_CONNECTED) {
		return 0;
	} else {
		return 1;
	}
}


void reconnect_wf() {
	if(WiFi.status() != WL_CONNECTED) {
		WiFi.disconnect(true);
		WiFi.mode(WIFI_OFF);
		reconnect_wf_f = true;
		online = 0;
		tmr = 50;
		logSystemEvent("[wifi:" + String(__LINE__) + "] WiFi reconnection initiated", getDateTimeString());
	}
}


String pars_req(const char* param, String req) {
	int index = req.indexOf(param);
	if(index == -1)
		return "";
	int ind_start = index + strlen(param) + 1;
	String result = "";
	int i;
	char c;
	for(i = 0; i < 49; i++) {
		if (ind_start + i >= req.length())
			break;
		c = req.c_str()[ind_start + i];
		if((byte)c <= 32 || c == '&') 
			break;
		result += c;
	}
	return conv_str(result);
}


bool connect_wf() {

	if(status_in  == 0) {
		WiFi.mode(WIFI_STA);
		return connect_wf_2();
	}
	else {
		delay(100);
		WiFi.mode(WIFI_AP_STA);
		IPAddress ip(192, 168, 4, 1);
		IPAddress subnet(255, 255, 255, 0);
		WiFi.softAPConfig(ip, ip, subnet);
		if (!WiFi.softAP(ssid4, password4)) {
			log_e("Soft AP creation failed.");
			while(1);
		}
		Serial.println(WiFi.softAPIP());
		server.begin();
		dnsServer.start(DNS_PORT, "*", ip);
		while(true) {
			dnsServer.processNextRequest();
			WiFiClient client2 = server.available();   // Listen for incoming clients
			delay(100);
			Serial.print(WiFi.softAPgetStationNum());
			if(client2) {
				Serial.println("New Client.");
				while(client2.connected()) {
					if(client2.available()) {
						// Обробка запиту від клієнта
						String request = client2.readStringUntil('\r\r');
						Serial.println(request);
						while(client2.available()) client2.read();
						
						ssid3 = pars_req("ssid", request);
						password3 = pars_req("pass", request);
						ip_adr = pars_req("ip_adr", request);
						ip_sub = pars_req("ip_sub", request);
						ip_gate = pars_req("ip_gate", request);
						ip_dns1 = pars_req("ip_dns1", request);
						ip_dns2 = pars_req("ip_dns2", request);
						host = pars_req("host", request);
						port = pars_req("port", request);
						dhcp_f = pars_req("dhcp_f", request);
						String server_login_tmp = pars_req("server_login", request);
						String server_password_tmp = pars_req("server_password", request);
						String timezone_tmp = pars_req("timezone", request);
						String restart = pars_req("command", request);
						request = "";
						
						if(!ssid3.isEmpty() && !password3.isEmpty()) {
							write_str_eepr(ADR_SSID, ssid3.c_str());
							write_str_eepr(ADR_PASS, password3.c_str());
						}
						else {
							ssid3 = read_str_eepr(ADR_SSID, NULL);
							password3 = read_str_eepr(ADR_PASS, NULL);
						}
						// --- Server login/password EEPROM logic ---
						if(!server_login_tmp.isEmpty() && !server_password_tmp.isEmpty()) {
							server_login = server_login_tmp;
							server_password = server_password_tmp;
							write_str_eepr(ADR_LOGIN, server_login.c_str());
							write_str_eepr(ADR_PASSWORD, server_password.c_str());
						} else {
							server_login = read_str_eepr(ADR_LOGIN, NULL);
							server_password = read_str_eepr(ADR_PASSWORD, NULL);
						}
						// --- Timezone EEPROM logic ---
						if(!timezone_tmp.isEmpty()) {
							timezone = timezone_tmp;
							write_str_eepr(ADR_TIMEZONE, timezone.c_str());
						} else {
							timezone = read_str_eepr(ADR_TIMEZONE, NULL);
						}
						// --- PIN EEPROM logic ---
						String pin_tmp = pars_req("pin", request);
						if(!pin_tmp.isEmpty()) {
							unsigned short pin = pin_tmp.toInt();
							EEPROM.write(ADR_PIN.adr, (byte)(pin & 0xFF));
							EEPROM.write(ADR_PIN.adr + 1, (byte)((pin >> 8) & 0xFF));
							EEPROM.commit();
						}
						else {
							pin = (unsigned short)(EEPROM.read(ADR_PIN.adr) | (EEPROM.read(ADR_PIN.adr + 1) << 8));
						}
						if(!dhcp_f.isEmpty()) {
							write_str_eepr(ADR_DHCPF, dhcp_f.c_str());
						}
						else {
							dhcp_f = read_str_eepr(ADR_DHCPF, NULL);
						}
						if(!ip_adr.isEmpty() && ip_adr_ob.fromString(ip_adr) != IPAddress(0, 0, 0, 0))
							write_str_eepr(ADR_IP, ip_adr.c_str());
						else
							ip_adr = read_str_eepr(ADR_IP, NULL);
						if(!ip_sub.isEmpty() && ip_sub_ob.fromString(ip_sub) != IPAddress(0, 0, 0, 0))
							write_str_eepr(ADR_SUB, ip_sub.c_str());
						else
							ip_sub = read_str_eepr(ADR_SUB, NULL);
						if(!ip_gate.isEmpty() && ip_gate_ob.fromString(ip_gate) != IPAddress(0, 0, 0, 0))
							write_str_eepr(ADR_GATE, ip_gate.c_str());
						else
							ip_gate = read_str_eepr(ADR_GATE, NULL);
						if(!ip_dns1.isEmpty() && ip_dns1_ob.fromString(ip_dns1) != IPAddress(0, 0, 0, 0))
							write_str_eepr(ADR_DNS1, ip_dns1.c_str());
						else
							ip_dns1 = read_str_eepr(ADR_DNS1, NULL);
						if(!ip_dns2.isEmpty() && ip_dns2_ob.fromString(ip_dns2) != IPAddress(0, 0, 0, 0))
							write_str_eepr(ADR_DNS2, ip_dns2.c_str());
						else
							ip_dns2 = read_str_eepr(ADR_DNS2, NULL);
						if(!host.isEmpty())
							write_str_eepr(ADR_HOST, host.c_str());
						else
							host = read_str_eepr(ADR_HOST, NULL);
						if(!port.isEmpty())
							write_str_eepr(ADR_PORT, port.c_str());
						else
							port = read_str_eepr(ADR_PORT, NULL);
						
						request = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
							request += "<style>";
								request += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
								request += "form { background: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 0 10px rgba(0,0,0,0.1); max-width: 400px; margin: auto; }";
								request += "input[type=text], input[type=password], select { width: 100%; padding: 8px; margin: 6px 0 12px 0; border: 1px solid #ccc; border-radius: 4px; }";
								request += "input[type=submit] { background: #4CAF50; color: white; padding: 10px 15px; border: none; border-radius: 4px; cursor: pointer; }";
								request += "input[type=button] { background: #4CAF50; color: white; padding: 10px 15px; border: none; border-radius: 4px; cursor: pointer; }";
								request += "input[type=submit]:hover { background: #45a049; }";
								request += "input[type=button]:hover { background: #45a049; }";
							request += "</style>";
							
							request += "<script>";
								request += "function toggleFields() {";
									request += "var dhcp = document.getElementById('dhcp_f').value;";
									request += "var fields = document.getElementsByClassName('ipfield');";
									request += "for (var i = 0; i < fields.length; i++) {";
										request += "fields[i].disabled = (dhcp != '0');";
									request += "}";
								request += "}";
								request += "window.onload = toggleFields;";
							request += "</script>";

							request += "<script>";
								request += "function saveAndRestart() {";
									request += "var form = document.forms[0];";
									request += "var cmd = document.createElement('input');";
									request += "cmd.type = 'hidden';";
									request += "cmd.name = 'command';";
									request += "cmd.value = 'restart';";
									request += "form.appendChild(cmd);";
									request += "form.submit();";
								request += "}";
							request += "</script>";
							
							request += "<script>";
								request += "function autoSubnet(){";
									request += "var ip=document.getElementsByName('ip_adr')[0].value;";
									request += "var subnetField=document.getElementsByName('ip_sub')[0];";
									request += "if(ip.startsWith('192.168.')){subnetField.value='255.255.255.0';}";
									request += "else if(ip.startsWith('172.')){";
										request += "var secondOctet=parseInt(ip.split('.')[1]);";
										request += "if(secondOctet>=16&&secondOctet<=31)subnetField.value='255.255.0.0';";
										request += "else subnetField.value='';";
									request += "}";
									request += "else if(ip.startsWith('10.')){subnetField.value='255.0.0.0';}";
									request += "else{subnetField.value='';}";
								request += "}";
							request += "</script>";

						request += "</head><body>";
							request += "<h2>WiFi Configuration</h2>";
							request += "<form method='GET' action='/'>";
								request += "SSID:<br><input type='text' name='ssid' value='" + ssid3 + "' autofocus><br>";
								request += "Password:<br><input type='password' name='pass' value='" + password3 + "'><br>";
								request += "DHCP:<br><select name='dhcp_f' id='dhcp_f' onchange='toggleFields()'>";
									if(dhcp_f == "0") {
										request += "<option value='0' selected>OFF (Static IP)</option>";
										request += "<option value='1'>ON (Dynamic IP)</option>";
									} else {
										request += "<option value='0'>OFF (Static IP)</option>";
										request += "<option value='1' selected>ON (Dynamic IP)</option>";
									}
								request += "</select><br>";
								request += "IP Address:<br>";
								request += "<input type='text' class='ipfield' name='ip_adr' value='" + ip_adr + "' onblur='autoSubnet()'><br>";
								request += "Subnet Mask:<br><input type='text' class='ipfield' name='ip_sub' value='" + ip_sub + "'><br>";
								request += "Gateway:<br><input type='text' class='ipfield' name='ip_gate' value='" + ip_gate + "'><br>";
								request += "DNS 1:<br><input type='text' class='ipfield' name='ip_dns1' value='" + ip_dns1 + "'><br>";
								request += "DNS 2:<br><input type='text' class='ipfield' name='ip_dns2' value='" + ip_dns2 + "'><br>";
								request += "Host:<br><input type='text' name='host' value='" + host + "'><br>";
								request += "Port:<br><input type='text' name='port' value='" + port + "'><br>";
								request += "<hr>";
								request += "Server Login:<br><input type='text' name='server_login' value='" + server_login + "'><br>";
								request += "Server Password:<br><input type='password' name='server_password' value='" + server_password + "'><br>";
								request += "PIN Code (4 digits):<br><input type='number' name='pin' min='0' max='9999' value='" + String(pin) + "'><br>";
								request += "Timezone:<br><input type='text' name='timezone' value='" + timezone + "' placeholder='Europe/Kyiv'><br>";
								request += "<input type='submit' value='Save'>";
								request += "<input type='button' value='Save & Restart' onclick='saveAndRestart()'>";
							request += "</form>";
						request += "</body></html>";
						
						client2.println("HTTP/1.1 200 OK");
						client2.print("Content-Length: ");
						client2.println(request.length());
						client2.println("Server: esp32at");
						client2.println("Content-Type: text/html");
						client2.println("Connection: close\r\n");
						client2.println(request);
						client2.stop();
						while(client2.available()) client2.read();
						
						if(restart == "restart") {
							Serial.println(" --== ESP.RESTART ==--");
							ESP.restart();
						}
					}
				}
			}
		}
	}
	return 0;
}


void setup() {
	EEPROM.begin(EEPROM_SIZE);
	init_hw_config_from_eeprom();
	inic_relays();
	
	//ite_str_eepr(ADR_TIMEZONE, "Europe/Sofia");
	//ite_str_eepr(ADR_TIMEZONE, "Europe/Kyiv");
	//ite_str_eepr(ADR_LOGIN, "Yo7h!fvgkYbhde6");
	//ite_str_eepr(ADR_PASSWORD, "!ndy6xj38nBTVkn");

	Serial.begin(115200);
	client.setInsecure();
	log_file = "";
	dhcp_f = '0';
	
	check_fw_update_flag = false;
	pinMode(in_1, INPUT);
	
	st_con = 0;
	timeOffset = 0;
	timestamp = 0;
	day = 0;
	month = 0;
	hour = 0;
	minute = 0;
	second = 0;
	dayOfWeek = 0;

	status_in = digitalRead(in_1);
	logSystemEvent("[init:" + String(__LINE__) + "] System initialized, firmware version: " + String(VERS));
	// Зчитування логіна, пароля та session_id з EEPROM у глобальні змінні
	logSystemEvent("[init:" + String(__LINE__) + "] Reading data from EEPROM");
	server_login = read_str_eepr(ADR_LOGIN, NULL);
	server_password = read_str_eepr(ADR_PASSWORD, NULL);
	session_id = read_str_eepr(ADR_SESSION, NULL);
	timezone = read_str_eepr(ADR_TIMEZONE, NULL);

	connect_wf();
	if(authorize_on_server()) {
		check_fw_update();
	}
	reconnect_wf_f = false;
	logSystemEvent("[init:" + String(__LINE__) + "] System started, firmware version: " + String(VERS), getDateTimeString());

	// Запуск Ticker для інкрементації timestamp кожну секунду
	timestampTicker.attach_ms(1000, updateTimestamp);

	inic_relays();
	pinMode(in_1, INPUT);
	
	for(byte i = 0; i < 8; i++) {
		status_pin_out[i] = 'X' - 48;
		status_pin_relay[i] = 'X' - 48;
	}

	online = 0;
	tmr = 200;
	status_init = 1;
	button_init();
	btnscanT.attach_ms(30, button_loop);
	btnscanT2.attach_ms(30, button_state_loop);
}

void set_relays() {
	for(byte i = 0; i < 8; i++)
		if(status_pin_out[i] > 1)
			status_pin_out[i] = (status_pin_relay[i] = (status_pin_relay[i] == 1 ? 1 : 0));
		else
			status_pin_relay[i] = status_pin_out[i];
		
	digitalWrite(out_1, status_pin_relay[0] == 1 ? relay_on : relay_off);
	digitalWrite(out_2, status_pin_relay[1] == 1 ? relay_on : relay_off);
	digitalWrite(out_3, status_pin_relay[2] == 1 ? relay_on : relay_off);
	digitalWrite(out_4, status_pin_relay[3] == 1 ? relay_on : relay_off);
	digitalWrite(out_5, status_pin_relay[4] == 1 ? relay_on : relay_off);
	digitalWrite(out_6, status_pin_relay[5] == 1 ? relay_on : relay_off);
	digitalWrite(out_7, status_pin_relay[6] == 1 ? relay_on : relay_off);
	digitalWrite(out_8, status_pin_relay[7] == 1 ? relay_on : relay_off);
}

void inic_relays() {
	pinMode(out_1, OUTPUT);
	pinMode(out_2, OUTPUT);
	pinMode(out_3, OUTPUT);
	pinMode(out_4, OUTPUT);
	pinMode(out_5, OUTPUT);
	pinMode(out_6, OUTPUT);
	pinMode(out_7, OUTPUT);
	pinMode(out_8, OUTPUT);
	digitalWrite(out_1, relay_off);
	digitalWrite(out_2, relay_off);
	digitalWrite(out_3, relay_off);
	digitalWrite(out_4, relay_off);
	digitalWrite(out_5, relay_off);
	digitalWrite(out_6, relay_off);
	digitalWrite(out_7, relay_off);
	digitalWrite(out_8, relay_off);
}


String convert_string(const char* buff, int lnght)
{
	String buff2;
	int i = 0;
	while(i < lnght) {
		if(buff[i] != '%') {
			if(buff[i] == '+')
				buff2 += ' ';
			else
				buff2 += buff[i];
			i++;
		}
		else {
			buff2 += (char)((buff[i + 1] <= '9' ? buff[i + 1] - '0' : buff[i + 1] - 'A' + 10) * 16 + (buff[i + 2] <= '9' ? buff[i + 2] - '0' : buff[i + 2] - 'A' + 10));
			i += 3;
		}
	}
	return buff2;
}

void button_state_loop() {
	if(state) {
		status_pin_out[state - 1] = (status_pin_relay[state - 1] == 1 ? 0 : 1);
		logRelayChange(state, status_pin_out[state - 1], true, getDateTimeString());
		state = 0;
		set_relays();
	}
}



void loop() {
	static bool sendLogToServer_flag = false;
	static bool fail_cnnect_server = false;
	
	if(reconnect_wf_f) {
		if(tmr)
			return;
		reconnect_wf_f = false;
		if(!connect_wf()) 
			authorize_on_server();
	}
	
	if(sendLogToServer_flag) {
		logSystemEvent("[loop:" + String(__LINE__) + "] Sending log file to server", getDateTimeString());
		sendLogToServer_flag = false;
		sendLogToServer();
		return;
	}
	
	if(check_fw_update_flag == true) {
		logSystemEvent("[loop:" + String(__LINE__) + "] Checking for firmware update", getDateTimeString());
		check_fw_update_flag = false;
		if(authorize_on_server()) {
			check_fw_update();
		}
	}
	while(true) {
		
		if(tmr)
			continue;
		if(online)
			online--;
		
#ifdef WFF
		if (!client.connected()) {
			if (!client.connect(host.c_str(), port.toInt(), 5000)) {
				if(fail_cnnect_server == false) {
					fail_cnnect_server = true;
					logError("[loop:" + String(__LINE__) + "] Failed to connect to server: " + host + ":" + port, getDateTimeString());
				}
				status_init = 1;
				if(WiFi.status() != WL_CONNECTED)
					reconnect_wf();
				return;
			}
			else {
				fail_cnnect_server = false;
			}
		}

		byte ii = 10;
		char buff[64];  // Increased buffer size for PIN parameter

		// Формуємо та надсилаємо запит
		client.print("POST /relay_servak/update_state");
		client.print(" HTTP/1.1\r\n");
		client.print("Host: ");
		client.print(host.c_str());
		client.print("\r\n");
		// Додаємо кукі X-Session-ID
		if(session_id.length() > 0) {
			client.print("Cookie: X-Session-ID=");
			client.print(session_id);
			client.print("\r\n");
		}
		client.print("User-Agent: " + String(USER_AGENT) + "\r\n");
		client.print("Accept-Language: uk-ua\r\n");
		// Calculate content length including PIN parameter
		int contentLength = 68 + 9; // 68 original + 9 for "&pin=0000"
		client.print("Content-Length: ");
		client.print(contentLength);
		client.print("\r\n");
		if(!online)
			client.print("Connection: close\r\n\r\n");
		else
			client.print("Connection: keep-alive\r\n\r\n");
		client.print("sn=");
		client.printf("%0.10d", ssn);
		sprintf(buff, "%0.10d", ssn);
		client.print("&relays=");
		for(byte i = 0; i < 8; i++, ii++) {
			client.print((char)(status_pin_relay[i] + 48));
			buff[ii] = (char)(status_pin_relay[i] + 48);
		}
		client.printf("&acp=%.2f000000", VERS);
		sprintf(&(buff[ii]), "%.2f000000", VERS);
		if(status_init) {
			my_rand.srand(ssn);
		}
		client.printf("&init=%s", ((status_init == 1) ? "true" : "fals"));
		ii += 10;
		sprintf(&(buff[ii]), "%s", ((status_init == 1) ? "true" : "fals"));
		ii += 4;
		contr_sum = 0;
		for(byte iii = 0; iii < ii; iii++) {
			contr_sum += (int)((byte)buff[iii] ^ (byte)my_rand.rand());
		}
		// Add PIN to the request
		client.printf("&pin=%04d", pin);
		// Add PIN to the checksum buffer
		sprintf(&(buff[ii]), "%04d", pin);
		ii += 4;
		
		// Calculate checksum including PIN
		contr_sum = 0;
		for(byte iii = 0; iii < ii; iii++) {
			contr_sum += (int)((byte)buff[iii] ^ (byte)my_rand.rand());
		}
		client.printf("&cs=%0.10d", contr_sum);
		my_rand.correct(contr_sum);

		// Очікуємо та обробляємо відповідь
		bool hasResponse = false;
		unsigned long timeout = millis() + 5000; // 5 секунд таймаут
		
		memset(reqest, 0, sizeof(reqest));
		int reqestPos = 0;
		
		while(client.connected() && millis() < timeout) {
			while(client.available()) {
				if(reqestPos < sizeof(reqest) - 1) {
					char c = client.read();
					reqest[reqestPos++] = c;
					hasResponse = true;
				}
			}
			
			if(hasResponse && !client.available()) {
				break; // Вся відповідь отримана
			}
		}
		
		reqest[reqestPos] = 0;
		
		// Обробка відповіді
		if(!hasResponse || millis() >= timeout) {
			client.stop();
			status_init = 1;
			return;
		}

		if(reqest[9] == '2' && reqest[10] == '0' && reqest[11] == '0' && reqest[0] != 0) {
			logSystemEvent("[loop:" + String(__LINE__) + "] HTTP 200 OK received", getDateTimeString());
			bool connectionClosed = false;
			char* connHeader = strstr(reqest, "Connection:");
			if(connHeader != NULL) {
				// Пропускаємо "Connection:" та пробіли
				connHeader += 10;
				while(*connHeader == ' ') connHeader++;
				
				// Перевіряємо значення
				if(strncmp(connHeader, "close", 5) == 0) {
					connectionClosed = true;
				}
			}
			
			// Перевіряємо наявність заголовка X-Get: log_file
			char* logHeader = strstr(reqest, "X-Get: log_file");
			if(logHeader != NULL) {
				connectionClosed = true;
				sendLogToServer_flag = true;
				logSystemEvent("[loop:" + String(__LINE__) + "] Log file request received from server", getDateTimeString());
			}
			
			if(check_fw_update_flag) {
				connectionClosed = true;
				logSystemEvent("[loop:" + String(__LINE__) + "] Firmware update flag set", getDateTimeString());
			}

			int i = 0;
			int ii = 0;
			byte j;
			int k;
			int onl_tmp = 0;
			while(reqest[i] != '{') {
				i++;
			}
			while(reqest[i] != ':') {
				i++;
			}
			i += 2;
			for(j = 0; j < 8; j++, ii++) {
				status_pin_out[j] = (reqest[i + j] - 48);
				if(status_pin_out[j] <= 1) {
					logRelayChange(j + 1, status_pin_out[j], false, getDateTimeString());
				}
				buff[ii] = reqest[i + j];
			}
			while(reqest[i] != ':') {
				i++;
			}
			i += 2;
			j = 0;
			k = 1;
			while(reqest[i + j] != '"') {
				j++;
			}
			for(; j != 0; j--) {
				onl_tmp += (int)(reqest[i + j - 1] - 48) * k;
				k *= 10;
			}
			
			if(onl_tmp != 0) {
				online = onl_tmp;
				tmr = 5;
			}
			buff[ii] = 0;
			sprintf(&(buff[ii]), "%d", onl_tmp);
			ii = strlen(buff);
			contr_sum = 0;
			for(byte iii = 0; iii < ii; iii++) {
				contr_sum += (int)((byte)buff[iii] ^ (byte)my_rand.rand());
			}
			
			while(reqest[i] != ':') {
				i++;
			}
			i += 2;
			j = 0;
			k = 1;
			cs = 0;
			while(reqest[i + j] != '"') {
				j++;
			}
			for(; j != 0; j--, ii++) {
				cs += (int)(reqest[i + j - 1] - 48) * k;
				k *= 10;
			}
			if(cs != contr_sum) {
				status_init = 1;
				client.stop();
				return;
			}
			else {
				if(status_init)
					status_init = 0;
				else
					set_relays();
				my_rand.correct(contr_sum);
				
				if(connectionClosed) {
					client.stop();
					return;
				}
				// Продовжуємо цикл для наступного обміну
				continue;
			}
		}
		else if(reqest[9] == '5' && reqest[10] == '0' && reqest[11] == '2' && reqest[0] != 0) {
			logError("[loop:" + String(__LINE__) + "] Bad Gateway");
			status_init = 1;
			client.stop();
			return;
		}
		else {
			logError("[loop:" + String(__LINE__) + "] Error response");
			status_init = 1;
			client.stop();
			authorize_on_server();
			return;
		}
#endif
	}
}


// Функції для роботи з логом подій

// Функція для формування рядка дати/часу для логування
String getDateTimeString() {
	if (timestamp != 0) {
		timestampToDate(timestamp + timeOffset, &day, &month, &dayOfWeek, &hour, &minute, &second);
		char timeStr[30];
		sprintf(timeStr, "[%02d.%02d %02d:%02d:%02d]: ", day, month, hour, minute, second);
		return String(timeStr);
	} else {
		return "[non_actual_time]: ";
	}
}

// Додавання запису до логу з перевіркою максимального розміру
void addLogEntry(const String& entry, const String& data_string) {
	// Формуємо запис з часовою міткою або з переданим рядком дати/часу
	String logEntry = data_string + entry + "\n";
	
	// Перевіряємо останній запис у логу на дублікат
	if (log_file.length() > 0) {
		// Знаходимо останній рядок
		int lastNewlinePos = log_file.lastIndexOf('\n', log_file.length() - 2); // Шукаємо передостанній \n
		if (lastNewlinePos >= 0) {
			// Отримуємо останній рядок
			String lastLine = log_file.substring(lastNewlinePos + 1, log_file.length() - 1); // -1 щоб виключити останній \n
			
			// Видаляємо часову мітку з останнього рядка
			int timeStampEndPos = lastLine.indexOf(": ");
			if (timeStampEndPos > 0) {
				String lastLineContent = lastLine.substring(timeStampEndPos + 2); // +2 щоб пропустити ": "
				
				// Порівнюємо з новим записом
				if (lastLineContent == entry) {
					// Знайдено дублікат, не додаємо запис
					return;
				}
			}
		}
	}
	
	// Перевіряємо, чи не перевищуємо максимальний розмір логу
	if (log_file.length() + logEntry.length() > MAX_LOG_SIZE) {
		// Видаляємо 10-12 рядків замість найстаріших записів
		
		// Знаходимо позиції перших 12 рядків
		int linePositions[13] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}; // Позиції початку кожного рядка (0-12)
		linePositions[0] = 0; // Початок першого рядка
		
		// Знаходимо позиції перших 12 символів \n
		int lineCount = 1;
		for (int i = 0; i < log_file.length() && lineCount < 13; i++) {
			if (log_file[i] == '\n') {
				linePositions[lineCount] = i + 1; // Позиція після \n
				lineCount++;
			}
		}
		
		// Якщо маємо принаймні 12 рядків, видаляємо 10-12
		if (lineCount >= 12 && linePositions[9] >= 0 && linePositions[12] >= 0) {
			// Видаляємо рядки 10, 11, 12 (індекси 9, 10, 11)
			String firstPart = log_file.substring(0, linePositions[9]); // Зберігаємо перші 9 рядків
			String lastPart = log_file.substring(linePositions[12]); // Зберігаємо все після 12-го рядка
			log_file = firstPart + lastPart;
		} else {
			// Якщо недостатньо рядків, використовуємо старий метод
			int bytesToRemove = logEntry.length();
			int cutPosition = bytesToRemove;
			while (cutPosition < log_file.length() && log_file[cutPosition] != '\n') {
				cutPosition++;
			}
			if (cutPosition < log_file.length()) {
				cutPosition++; // Включаємо символ нового рядка
			}
			log_file = log_file.substring(cutPosition);
		}
	}
	
	// Додаємо новий запис
	log_file += logEntry;
	Serial.print(logEntry);
}

// Додавання запису про зміну стану реле
void logRelayChange(int relayNumber, bool isOn, bool isLocal, const String& data_string) {
	String source = isLocal ? "local" : "remote";
	String state = isOn ? "ON" : "OFF";
	addLogEntry("REMOTE: Relay " + String(relayNumber) + " " + state + " - " + source + " command", data_string);
}

// Додавання запису про авторизацію/ініціалізацію годинника
void logClockInitialization(const String& data_string) {
	addLogEntry("SYSTEM: Clock initialized, timezone: " + timezone + ", offset: " + String(timeOffset) + " sec", data_string);
}

// Додавання запису про помилку
void logError(const String& errorMessage, const String& data_string) {
	addLogEntry("ERROR: " + errorMessage, data_string);
}

// Перевантажена функція для сумісності з існуючим кодом
void logError(const String& errorMessage) {
	logError(errorMessage, getDateTimeString());
}

// Додавання запису про системну подію
void logSystemEvent(const String& eventMessage, const String& data_string) {
	addLogEntry("SYSTEM: " + eventMessage, data_string);
}

void logSystemEvent(const String& eventMessage) {
	addLogEntry("SYSTEM: " + eventMessage, getDateTimeString());
}

// Відправка логу на сервер
void sendLogToServer() {
	if (!client.connected()) {
		if (!client.connect(host.c_str(), port.toInt(), 5000)) {
			Serial.println("Помилка підключення при відправці логу!");
			return;
		}
	}
	
	// Формуємо та надсилаємо запит
	String ssn_str = String("Gadget serial number: " + String(ssn) + '\n');
	String relays_state_str = getDateTimeString() + String("Relays state: ");
	for (int i = 0; i < 8; i++) {
		relays_state_str += String(status_pin_relay[i]);
		relays_state_str += " ";
	}
	relays_state_str += "\n";
	String postRequest = "POST /relay_servak/log_file HTTP/1.1\r\n";
	postRequest += "Host: " + host + "\r\n";
	postRequest += "User-Agent: " + String(USER_AGENT) + "\r\n";
	if (session_id.length() > 0) {
		postRequest += "Cookie: X-Session-ID=" + session_id + "\r\n";
	}

	postRequest += "Content-Type: text/plain\r\n";
	postRequest += "Content-Length: " + String(log_file.length() + ssn_str.length() + relays_state_str.length()) + "\r\n";
	postRequest += "Connection: close\r\n\r\n";
	
	Serial.println("Log file length: " + String(log_file.length()));
	
	client.print(postRequest);
	client.print(ssn_str);
	
	// Відправляємо лог посимвольно, щоб уникнути проблем з нуль-термінаторами
	for (size_t i = 0; i < log_file.length(); i++) {
		client.write(log_file[i]);
	}
	client.print(relays_state_str);
	// Очікуємо відповідь і закриваємо з'єднання
	unsigned long timeout = millis();
	while (client.connected() && !client.available() && millis() - timeout < 5000) { 
		delay(1); 
	}
	
	if (client.available()) {
		String response = client.readString();
	}
	
	client.stop();
}
