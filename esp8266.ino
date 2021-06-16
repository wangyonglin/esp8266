#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ESP.h>

//esp_chip_id
char esp_name[64] = {0};

const byte  dns_port = 53;
const char  *mqtt_broker = "broker.wangyonglin.com";
const int    mqtt_port = 1883;
const char  *mqtt_username = "wangyonglin";
const char  *mqtt_password = "W@ng0811";
const char  *mqtt_topic   = "jslg";
//暂时存储wifi账号密码
char sta_ssid[32] = {0};
char sta_password[64] = {0};
//配网页面代码
const char* page_html = "\
<!DOCTYPE html>\r\n\
<html lang='en'>\r\n\
<head>\r\n\
  <meta charset='UTF-8'>\r\n\
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>\r\n\
  <title>Document</title>\r\n\
</head>\r\n\
<body style='background-color: brown;text-align: center;'>\r\n\
  <form name='input' action='/' method='POST'>\r\n\
        wifi名称: <br>\r\n\
        <input type='text' name='ssid'><br>\r\n\
        wifi密码:<br>\r\n\
        <input type='text' name='password'><br>\r\n\
        <input type='submit' value='保存'>\r\n\
    </form>\r\n\
</body>\r\n\
</html>\r\n\
";


IPAddress apIP(192, 168, 1, 1);//esp8266-AP-IP地址
DNSServer dnsServer;//创建dnsServer实例
ESP8266WebServer server(80);//创建WebServer
WiFiClient espClient;
PubSubClient client(espClient);

void handleRoot() {//访问主页回调函数
  server.send(200, "text/html", page_html);
}

void handleRootPost() {//Post回调函数
  Serial.println("handleRootPost");
  if (server.hasArg("ssid")) {//判断是否有账号参数
    Serial.print("got ssid:");
    strcpy(sta_ssid, server.arg("ssid").c_str());//将账号参数拷贝到sta_ssid中
    Serial.println(sta_ssid);
  } else {//没有参数
    Serial.println("error, not found ssid");
    server.send(200, "text/html", "<meta charset='UTF-8'>error, not found ssid");//返回错误页面
    return;
  }
  //密码与账号同理
  if (server.hasArg("password")) {
    Serial.print("got password:");
    strcpy(sta_password, server.arg("password").c_str());
    Serial.println(sta_password);
  } else {
    Serial.println("error, not found password");
    server.send(200, "text/html", "<meta charset='UTF-8'>error, not found password");
    return;
  }

  server.send(200, "text/html", "<meta charset='UTF-8'>保存成功");//返回保存成功页面
  delay(2000);
  //连接wifi
  connectNewWifi();
}



void initSoftAP(void) { //初始化AP模式
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  if (WiFi.softAP(esp_name)) {
    Serial.println("ESP8266 SoftAP is right");
  }
}

void initWebServer(void) { //初始化WebServer
  //server.on("/",handleRoot);
  //上面那行必须以下面这种格式去写否则无法强制门户
  server.on("/", HTTP_GET, handleRoot);//设置主页回调函数
  server.onNotFound(handleRoot);//设置无法响应的http请求的回调函数
  server.on("/", HTTP_POST, handleRootPost);//设置Post请求回调函数
  server.begin();//启动WebServer
  Serial.println("HTTP started!");
}

void initDNS(void) { //初始化DNS服务器
  if (dnsServer.start(dns_port, "*", apIP)) { //判断将所有地址映射到esp8266的ip上是否成功
    Serial.println("start dnsserver success.");
  }
  else Serial.println("DNS  start failed.");
}

void connectNewWifi(void) {
  WiFi.mode(WIFI_STA);//切换为STA模式
  WiFi.setAutoConnect(true);//设置自动连接
  WiFi.begin();//连接上一次连接成功的wifi
  Serial.println("");
  Serial.print("WIFI  connect");
  int count = 0;

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    count++;
    if (count > 10) { //如果5秒内没有连上，就开启Web配网 可适当调整这个时间
      initSoftAP();
      initWebServer();
      initDNS();
      break;//跳出 防止无限初始化
    }
    Serial.print(".");


  }
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED) { //如果连接上 就输出IP信息 防止未连接上break后会误输出
    Serial.println("WIFI connected!");
    Serial.print("WIFI ip[");
    Serial.print(WiFi.localIP());
    Serial.println("]");
    server.stop();
    initMQTT();

  }
}

void initMQTT(void) {
  Serial.println("MQTT init");
  if (WiFi.status() == WL_CONNECTED) { //如果连接上 就输出IP信息 防止未连接上break后会误输出

    //连接MQTT
    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);
    while (!client.connected()) {
      Serial.println("MQTT  connecting...");
      if (client.connect(esp_name, mqtt_username, mqtt_password)) {
        Serial.println("MQTT  connected");
        // 连接成功时订阅主题
        client.subscribe(mqtt_topic);
      } else {
        Serial.print("MQTT failed with state");
        Serial.println(client.state());
      }
    }


  }

}
//mqtt callback
void callback(char* topic, byte* payload, unsigned int length) {

 // Serial.print("MQTT arrived in topic: ");
  //Serial.println(topic);
  //Serial.print("MQTT: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

}

void wangyonglin__esp_init(void) {
  Serial.begin(9600);
  memset(esp_name, 0, sizeof(esp_name));
  sprintf(esp_name, "ESP8266%08X", ESP.getChipId());
  Serial.println(esp_name);
  WiFi.hostname(esp_name);
  //pinMode(LED_BUILTIN, OUTPUT);
 // digitalWrite(LED_BUILTIN, LOW);
}

void wangyonglin__esp_cleanup() {


}

void setup() {
  wangyonglin__esp_init();
  connectNewWifi();
  wangyonglin__esp_cleanup();
}
void loop() {
  server.handleClient();
  dnsServer.processNextRequest();
  client.loop();
  unsigned char result[1024];
  memset(result, '\0', sizeof(result));
  while (Serial.available()) //判断串口缓冲器是否有数据装入
  {
    Serial.readBytes(result, 1024);
    client.publish(mqtt_topic, (const char*)result);
  }

}
