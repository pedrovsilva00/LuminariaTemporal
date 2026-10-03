#include <GxEPD2_BW.h>
#include <Fonts/FreeSansBold18pt7b.h> // titulo
#include <Fonts/FreeSans12pt7b.h> // subtitulo, caso necessario adicionar outra para info menores
#include <Fonts/FreeMono9pt7b.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include "bitmaps.h"
#include <WiFiManager.h>

#define LED_PIN     21   // Pino do ESP32 conectado na fita LED
#define NUM_LEDS    24   // Quantidade de LEDs da fita
Adafruit_NeoPixel fita(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800); 
uint32_t corLed = 0;

// Pinos utilizados: (CS=5, DC=19, RST=15, BUSY=22)
GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(GxEPD2_420_GDEY042T81(5, 19, 15, 22));
// Variaveis icone enum
enum IconeClima {inoitelimpa,idialimpo,idianuvem,inoitenuvem,ichuva,itempesta,inuvem,ineblina};
enum IconeLua {inova,icrescente,icheia,iminguante};
// variaveis timer
unsigned long tempoUltimaAtualizacao = 0;
const unsigned long INTERVALO_UMAHORA = 3600000;
// Dias da semana sem "-feira" e com iniciais maiúsculas
const char* dias[] = {"Domingo", "Segunda", "Terca", "Quarta", "Quinta", "Sexta", "Sabado"};
// Meses por extenso em português 
const char* meses[] = {"","Janeiro", "Fevereiro", "Marco", "Abril", "Maio", "Junho", "Julho", "Agosto", "Setembro", "Outubro", "Novembro", "Dezembro"};
String dateDisp = "";
String diaDisp = "";
String cores = "";
// Criamos um "tipo" de variável chamado DadosClima
struct DadosClima {
  int temp_atual;
  int temp_max;
  int temp_min;
  int umidade;
  int condicao;
  int lua;
  String vento;
  int probchuva;
  String tempo;
  String data;
  String dia;
  String hora;
  bool sucesso; 
};
// Variaveis botão push
const int PINO_BOTAO = 27; // Mude para o pino que for usar
bool ledsLigados = true;
bool estadoAnteriorBotao = HIGH;
unsigned long tempoUltimoClique = 0;

void conectarWiFi() {
  // Na primeira conexão com a internet, o esp32 ira criar um wifi aberto, onde deve conectar com um celular e definir as credenciais de acesso da rede wi-fi
  WiFiManager wifiManager;
  wifiManager.setDebugOutput(false);

  wifiManager.setConfigPortalTimeout(180);

  if (!wifiManager.autoConnect("Luminaria-Setup")) {

    return; 
  }
}

//definir icone de clima baseado na condition_slug da API
IconeClima descobrirIcone(String slug) {
  if (slug == "clear_day")   return idialimpo;
  if (slug == "clear_night") return inoitelimpa; 
  if (slug == "rain")        return ichuva;
  if (slug == "storm")       return itempesta; 
  if (slug == "cloud")       return inuvem;
  if (slug == "cloudly_day") return idianuvem;
  if (slug == "cloudy_night")         return inoitenuvem;
  if (slug == "fog")         return ineblina;

} 

// icone da lua baseado na moon_phase da API
IconeLua descobrirLua (String phase) {
   if (phase == "new")   return inova;
   if (phase == "waxing_crescent")   return icrescente;
   if (phase == "first_quarter")   return icrescente;
   if (phase == "waxing_gibbous")   return icrescente;
   if (phase == "full")   return icheia;
   if (phase == "waning_gibbous")   return iminguante;
   if (phase == "last_quarter")   return iminguante;
   if (phase == "waning_crescent")   return iminguante;
}

DadosClima buscarDados() { 
  DadosClima clima = {}; 
  clima.sucesso = false; 

  if (WiFi.status() == WL_CONNECTED) {
    // 1. Criamos um cliente seguro e dizemos para ele não barrar o HTTPS
    WiFiClientSecure client;
    client.setInsecure(); // Ignora a validação do certificado SSL
    
    // key é gerada na criação da conta do hgbrasil
    // woeid é o codigo da sua cidade
    HTTPClient http; 
    String url = "https://api.hgbrasil.com/weather?key=1e6370f0&woeid=455912"; // alterar key e woeid
    
    http.begin(client, url); 
    
    int httpCode = http.GET();

    if (httpCode == 200) { 
      String payload = http.getString();
      JsonDocument doc; 
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        // Preenchemos o nosso pacote com os dados do JSON!
        clima.temp_atual = doc["results"]["temp"];
        clima.umidade = doc["results"]["humidity"];
        clima.temp_max = doc["results"]["forecast"][0]["max"];
        clima.temp_min = doc["results"]["forecast"][0]["min"];
        clima.vento = doc["results"]["wind_speedy"].as<String>();
        clima.probchuva = doc["results"]["forecast"][0]["rain_probability"];
        clima.tempo = doc["results"]["description"].as<String>();
        clima.data = doc["results"]["date"].as<String>();
        clima.dia = doc["results"]["forecast"][0]["weekday"].as<String>();
        clima.hora = doc["results"]["time"].as<String>();
        //icone clima
        String slug = doc["results"]["condition_slug"].as<String>();
        clima.condicao = descobrirIcone(slug);
        //fase lua
        String phase = doc["results"]["moon_phase"].as<String>();
        clima.lua = descobrirLua(phase);
        //data e dia semana
        int dia = 0;
        int mes = 0;
        int ano = 0;
        sscanf(clima.data.c_str(), "%d/%d/%d", &dia, &mes, &ano);
        String nomeMes = meses[mes];
        char bufferData[32];
        sprintf(bufferData, "%d %s %d", dia, nomeMes, ano);
        dateDisp = String(bufferData); // variavel global data
        // dia semana
        int idxDia = 0; // Padrão Domingo
        if (clima.dia.equalsIgnoreCase("Dom")) idxDia = 0;
        else if (clima.dia.equalsIgnoreCase("Seg")) idxDia = 1;
        else if (clima.dia.equalsIgnoreCase("Ter")) idxDia = 2;
        else if (clima.dia.equalsIgnoreCase("Qua")) idxDia = 3;
        else if (clima.dia.equalsIgnoreCase("Qui")) idxDia = 4;
        else if (clima.dia.equalsIgnoreCase("Sex")) idxDia = 5;
        // Trata o sábado com ou sem acento, prevenindo falhas de encoding da API
        else if (clima.dia.equalsIgnoreCase("Sáb") || clima.dia.equalsIgnoreCase("Sab")) idxDia = 6;

        diaDisp = String(dias[idxDia]); 
        clima.sucesso = true; 

      }
    }
    http.end(); 
  }
  
  return clima; 
}

void acenderLeds() {
  // Pinta todos os LEDs com a cor que está salva na variável global
  for (int i = 0; i < NUM_LEDS; i++) {
    fita.setPixelColor(i, corLed);
  }
  fita.show();
}

void apagarLeds(){
  fita.clear(); // Zera a cor de todos os LEDs
  fita.show();
}


void displaySetup (DadosClima clima) {
  //icone clima principal
  char cor[12] = {0};  const unsigned char* iconclima = nullptr;
  // Projetei para usar a energia do esp32 para alimentar o display e os leds, então nenhuma cor utiliza o maximo de brilho
  switch (clima.condicao) {
    case 0: iconclima = bitmap_clear_night; corLed = fita.Color(70,0,130); cores = "Roxo"; break; // roxo noite limpa
    case 1: iconclima = bitmap_clear_day; corLed = fita.Color(0,102,102); cores="Azul Claro"; break; //  dia limpo azul claro
    case 2: iconclima = bitmap_cloudy_day; corLed = fita.Color(0,0,230); cores ="Azul Escuro"; break; // azul escuro dia nublado
    case 3: iconclima = bitmap_cloudy_night; corLed = fita.Color(0,230,0); cores="Verde"; break; // verde noite nublada
    case 4: iconclima = bitmap_rain; corLed = fita.Color(235,0,220); cores="Rosa"; break; // rosa chuva
    case 5: iconclima = bitmap_storm; corLed = fita.Color(230,0,0); cores="Vermelho"; break; // vermelho tempestade
    case 6: iconclima = bitmap_clouds;  corLed = fita.Color(150,150,150); cores="Branco"; break; // nublado branco
    case 7: iconclima = bitmap_fog; corLed = fita.Color(230,130,0); cores="Amarelo"; break; // amarelo neblina
    default:  iconclima = bitmap_fog;  corLed = fita.Color(120,120,120); cores="Branco Fraco"; break;   // caso falhe  branco fraco
  }
  
  // icone lua e nome da fase
  const unsigned char* iconlua = nullptr;
  String Lua = "erro";
  int x = 0;
  switch (clima.lua) {
    case 0: iconlua = bitmap_new_moon; Lua = "Nova"; x =30; break; // nova
    case 1: iconlua = bitmap_rise_moon; Lua = "Crescente"; x=72;break; // crescente
    case 2: iconlua = bitmap_full_moon; Lua = "Cheia"; x=114; break; // cheia
    case 3: iconlua = bitmap_waning_moon; Lua = "Minguante"; x=156; break; // minguante
    default:  iconlua = bitmap_new_moon; x =30;break;   // caso falhe 
  }

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    
    // cabeçalho
    display.drawBitmap(201, 3, iconclima, 90, 90, GxEPD_WHITE, GxEPD_BLACK);
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(7, 40);
    if (diaDisp.equalsIgnoreCase("Terca")) {
      display.print(diaDisp);
      display.fillRect(73, 41, 3, 7, GxEPD_BLACK); // gambiarra para o ç
    }else {display.print(diaDisp);} // dia semana

    display.setFont(&FreeSans12pt7b);

    char palavra1[20] = {0};
    char palavra2[20] = {0};

    int palavrasEncontradas = sscanf(clima.tempo.c_str(), "%19s %19s", palavra1, palavra2);
    if (strcmp(palavra1, "Parcialmente") == 0) {
    strcpy(palavra1, "Pouco"); 
    }
    // Aqui fiz uma gambiarra para a palavra parcialmente que estourava o campo, trocando pela palavra pouco
    else if (strcmp(palavra1, "parcialmente") == 0) {
      strcpy(palavra1, "Pouco"); 
    }
// altera o local de impressão no display, dependendo a quantidade de palavras (1 ou 2)
    if (palavrasEncontradas == 1) {
 
      display.setCursor(299, 50);
      display.print(palavra1);
    } 
    else if (palavrasEncontradas == 2) {
      display.setCursor(299, 40);
      display.print(palavra1);

      display.setCursor(302, 65);
      display.print(palavra2);
    } 
    display.setFont(&FreeSans12pt7b);// data 
    display.setCursor(4, 72);
    display.print(dateDisp); 
    display.drawLine(6, 100, 395, 100, GxEPD_BLACK); // linha horizontal cima

    // meio esquerdo
    display.drawLine(235, 100, 235, 280, GxEPD_BLACK); // linha vertical meio
    display.drawBitmap(35,120, bitmap_wind, 30, 30, GxEPD_WHITE, GxEPD_BLACK); // icone vento
    display.setFont(&FreeSans12pt7b);
    display.setCursor(69, 143);
    display.print(clima.vento); // resultado api da velocidade do vento
    display.drawBitmap(5,170, bitmap_brisa, 30, 30, GxEPD_WHITE, GxEPD_BLACK); // icone brisa/cat
    display.setFont(&FreeMono9pt7b);
    display.setCursor(45, 180); // chance chuva
    display.print("Chance");
    display.setCursor(45, 202); 
    display.print("de Chuva");
    display.setFont(&FreeSans12pt7b);
    display.setCursor(140, 192); 
    display.print(clima.probchuva);// resultado api propabilidade de chuva
    display.print("%");

    display.setFont(&FreeMono9pt7b);
    display.setCursor(50, 234); 
    display.print("Lua ");  
    display.print(Lua);
    display.drawBitmap(x,245, iconlua, 32, 32, GxEPD_WHITE, GxEPD_BLACK); // fases lua

    // meio direito
    display.drawBitmap(255,115, bitmap_temp, 32, 32, GxEPD_WHITE, GxEPD_BLACK); // icone temperatura
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(288, 141);
    display.print(clima.temp_atual); // temperatura atual
    display.drawBitmap(328,115, bitmap_celcius, 28, 28, GxEPD_WHITE, GxEPD_BLACK); // icone celcius

    display.setFont(&FreeSans12pt7b);
    display.drawBitmap(275,152, bitmap_maxmin, 80, 70, GxEPD_WHITE, GxEPD_BLACK); //icone max e min
    display.setCursor(293, 179);
    display.print(clima.temp_max); //temperatura maxima api
    display.setCursor(293, 215);
    display.print(clima.temp_min);//remperatura minima api


    display.setFont(&FreeMono9pt7b);
    display.setCursor(248, 243);
    display.print("Umidade");
    display.setFont(&FreeSans12pt7b);
    display.setCursor(330, 246);
    // logica para não imprimir 100% de umidade, se limitando a 99
    if (clima.umidade == 100) {
      display.print("99");
    }else {
      display.print(clima.umidade);
    }
    //  api umidade
    display.drawBitmap(359,226, bitmap_umidade, 24, 24, GxEPD_WHITE, GxEPD_BLACK); // icone umidade

    display.setFont(&FreeMono9pt7b);
    display.setCursor(250, 271);

    display.print(cores);//Cor da Luz da luminaria

    
    display.drawLine(5, 280, 395, 280, GxEPD_BLACK); // linha horizontal baixo
    // rodape direito
    display.setCursor(10, 295);
    display.print("Last Update "); 
    display.print(clima.hora); // ultima atualização da api
    // rodape esquerdo
    display.setCursor(213, 295);
    uint32_t ramLivreKB = ESP.getFreeHeap() / 1024;
    display.printf("ram %uKB ", ramLivreKB); // ram livre
    float tempCPU = temperatureRead(); 
    display.printf("cpu %.0fC", tempCPU); // cpu temperatura

  } while (display.nextPage());
  
  display.powerOff();

}


void setup() {
  Serial.begin(115200);
  delay(500); // caso qeuria visualizar o Serial.println deve alterar esse campo para 3000
  display.init(115200, true, 2, false); 
  display.setRotation(0); // Modo Paisagem
  display.setTextColor(GxEPD_BLACK);
// 2. Inicializa a fita LED 
  fita.begin();
  fita.show();

  pinMode(PINO_BOTAO, INPUT_PULLUP);//butão push
  
}
void loop() {
  unsigned long tempoAtual = millis();

  bool estadoAtualBotao = digitalRead(PINO_BOTAO);

  // Se o botão foi pressionado (LOW) e antes estava solto (HIGH)
  if (estadoAtualBotao == LOW && estadoAnteriorBotao == HIGH && (millis() - tempoUltimoClique > 200)) {
    tempoUltimoClique = millis();
    ledsLigados = !ledsLigados; // Inverte: se estava ligado, vira falso. Se estava desligado, vira verdadeiro.

    if (ledsLigados) {
      // Usa a cor global para ligar os leds
      for (int i = 0; i < NUM_LEDS; i++) {
        fita.setPixelColor(i, corLed);
      }
      fita.show();
    } else {
      apagarLeds(); 
    }
  }
  estadoAnteriorBotao = estadoAtualBotao;

  if (tempoAtual - tempoUltimaAtualizacao >= INTERVALO_UMAHORA || tempoUltimaAtualizacao == 0) {
    tempoUltimaAtualizacao = tempoAtual; 
    apagarLeds();
    // LIGA O WI-FI 
    conectarWiFi(); 

    DadosClima climaAtual = buscarDados();

    if (climaAtual.sucesso) {

      // ATUALIZA A TELA
      displaySetup(climaAtual); 
      
      // Coloca o e-paper em modo de baixo consumo (dormir)
      display.powerOff(); 

      // Desliga o rádio do Wi-Fi completamente
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      Serial.println("[SISTEMA] Wi-Fi desligado para economizar energia.");

      // ATUALIZA OS LEDS (Eles ficam travados nessa cor, sem gastar CPU)
      if (ledsLigados) {
        acenderLeds();
      }
    }
  }
}