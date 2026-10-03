# LuminariaTemporal com ESP32 (E-paper + NeoPixel)

![Imagens do Projeto Finalizado](Imagens)

## 📖 Sobre o Projeto

Este projeto consiste em uma luminária inteligente e painel de informações (dashboard) de alto padrão e baixo consumo. O sistema integra iluminação ambiente controlável e uma tela de e-paper para exibição de dados estáticos, tudo processado por um **ESP32**.

O grande diferencial deste hardware é a sua **arquitetura de gestão de energia**. Para evitar problemas de *brownout* (reinicializações por queda de tensão) comuns no ESP32, o sistema utiliza uma técnica de *time-sharing*: a fita de LED é desligada temporariamente durante o pico de consumo do Wi-Fi, garantindo estabilidade absoluta. Além de que, atualiza as informações de hora em hora, pois a API utilizada compartilha novos dados de hora em hora (se for mudar a API é possível criar atualizações mais frequentes ou mais demoradas) 

## ✨ Principais Funcionalidades

* **Dashboard de Baixo Consumo:** Tela E-paper (SPI) que mantém a última informação na tela mesmo sem energia.
* **Iluminação Endereçável:** Controle de 24 LEDs WS2812B (NeoPixel), onde a cor é baseada no clima atual.
* **Gestão Anti-Brownout:** Lógica de software que intercala o uso de energia entre a rede Wi-Fi e os LEDs de alta potência.
* **Interface Física:** Controle tátil integrado (Push Button para iluminação e Chave Alavanca para Hard Reset/Sleep via pino `EN`).
* **Configuração Dinâmica de Rede:** Integração com `WiFiManager` para conexão em novas redes sem necessidade de reprogramação.
* **API HGBrasil** para previsão do tempo, gratuita e nacional.

## 🛠️ Lista de Componentes (Hardware)

* 1x Placa de Desenvolvimento **ESP32** (NodeMCU ou equivalente)
* 1x Display 4,2" **E-paper** (Interface SPI)
* 1x Fita de LED Endereçável **WS2812B** (24 LEDs utilizados)
* 1x Conector **USB-C Fêmea** (Breakout board para entrada de energia 5V)
* 1x **Chave Alavanca / Toggle Switch** (Ligada ao pino EN e GND para liga/desliga de hardware)
* 1x **Botão Push** (Ligado ao GPIO 4 e GND com resistor pull-up interno)
* 1x Placa Ilhada (Perfboard) dupla face

## ⚡ Esquema de Ligação Resumido

O projeto utiliza um barramento de energia (5V e GND) criado fisicamente na placa ilhada para proteger o regulador de tensão do ESP32.

* **Alimentação (USB-C):** O 5V alimenta simultaneamente o pino `VIN` do ESP32 e o fio de energia da fita LED. O GND é comum a todo o circuito.
* **Display E-paper:** Alimentado exclusivamente pelo pino `3V3` do ESP32 para respeitar o nível lógico.
* **Fita LED (WS2812B):** Pino de dados (`DIN`) conectado a um GPIO de saída configurado no código.
* **Botões:** Conectados diretamente entre os GPIOs / Pino `EN` e o GND (sem resistores externos).

## 🚀 Como Instalar e Usar

1. Clone este repositório: `git clone https://github.com/pedrovsilva00/LuminariaTemporal.git`
2. Abra o código principal na IDE do Arduino ou PlatformIO.
3. Instale as bibliotecas necessárias listadas no código (ex: `Adafruit_NeoPixel`, `WiFiManager`, `ArduinoJson`, `GxEPD2_BW`).
4. Carregue o código no ESP32.
   > **Aviso de Segurança:** Ao programar o ESP32 via cabo USB conectado ao computador, desconecte a fonte externa da porta USB-C da placa ilhada para evitar corrente reversa, ou utilize um cabo USB sem o pino de 5V (apenas dados).
5. Ao ligar a primeira vez, o ESP32 criará um Ponto de Acesso (AP). Conecte-se a ele para configurar as credenciais da sua rede Wi-Fi local.


**Observação:** a parte da estrutura do projeto ainda não está como gostaria, quero no futuro passar para a impressão 3D, por isso não publiquei a confecção do case. 
---
*Desenvolvido por [pedrovsilva00]*
