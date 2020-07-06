/*Autor: Hilário José Silveira Castro


*/
//--------------------------------------------------------------------------------------------------------------------
//Include das bibliotecas do experimento
//Include das bibliotecas do experimento
//Include das bibliotecas do experimento
#include <WiFi.h> // bibliotecas para uso do Wi-Fi
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <String.h>// biblioteca que permite o trabalho com strings na plataforma
#include <NTPClient.h> // Bibliotecas para o NTP Server (versão de: https://github.com/taranais/NTPClient)
#include <SD.h> // Biblioteca para funções do SD Card para fazer o data logger
#include <SPI.h> // Biblioteca para comunicação SPI a ser utilizada no SD
#include "FS.h"



//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
//Variáveis e funções
//Variáveis e funções
//Variáveis e funções
//Criando vetor ponteiro com ssid e respectivas senhas de redes conhecidas
//As posições de cada array tem que estar na mesma referência com os outros arrays para cada rede
const char *ssid[] = {"Coloque SSID da rede aqui ", "Coloque SSID da rede aqui "}; #pode colocar mais redes se preferir
const char *password[] =  {"Coloque Senha da rede aqui ", "Coloque senha da rede aqui "};
const char *mac[] = {"48:2C:A0:B1:F4:46", "10:62:D0:9D:60:BF"};//  MAC das redes conhecidas
int NRC = 2; //Número de Redes conhecidas
//-------------------------------------------------------------------------------------------------------------------
//Variáveis para o Sd Card
const int SDCS_PIN = 5;
File myFile;       // Objeto responsável por escrever/Ler do cartão SD
//-------------------------------------------------------------------------------------------------------------------
//função para criar client com conexões seguras
//A plataforma irá apresentar alguns resultados por meio de uma html utilizando a porta 80
WiFiServer server(80);
WiFiClientSecure client;
WiFiUDP ntpUDP; // Declaração do Protocolo UDP
//-------------------------------------------------------------------------------------------------------------------
// Servidor NTP para pesquisar a hora
//const char* servidorNTP = "a.st1.ntp.br";
const char* servidorNTP = "time.google.com";
//Fuso horário em segundos (-03h = -10800 seg)
const int fusoHorario = -10800;
//variável para guardar timestamp
String horario;
NTPClient timeClient(ntpUDP, servidorNTP, fusoHorario, 60000);

//Struct com os dados do dia e hora
struct Date {
  int dayOfWeek;
  int day;
  int month;
  int year;
  int hours;
  int minutes;
  int seconds;
  unsigned int timestamp;
};
//Nomes dos dias da semana
//Na função do NTP server, ele retorna um número int de 0 até 6, 0 é domingo e 6 é sábado,
//respectivamente os nomes dos dias em inglês ou portugues ou português (comentar o que não deseja usar
//char* dayOfWeekNames[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
char* dayOfWeekNames[] = {"Domingo", "Segunda", "Terça", "Quarta", "Quinta", "Sexta", "Sabado"};

//-------------------------------------------------------------------------------------------------------------------

//Struct com os valores médios dos sensores
//Sempre que um sensor for adicionado, será lançado no struct para armazenar globalmente sua média

//Variável auxiliar para contar o número de leituras para fazer a média
int SensorAux = 0;
//Variável auxiliar para definir o número de amostras de cada sensor
int NumLei = 20;
//Variáveis para fazer referência aos sensores
//Este três arrays farão parte da forma de leitura dos sensores, caso seja necessário mais sensores altere a quantidade
//na referência de quantidade de elementos
int QuantidadeDeSensores = 0;
int PortDosSensores[10];
unsigned long int ValoresDosSensores[10];
String NomeDosSensores[10];

//-------------------------------------------------------------------------------------------------------------------
//Variável para definir a cada quanto tempo será registrado as amostras dos sensores e das RSSI das redes no SD CaRD
int TempoDeAmostragem = 60000; //Essa referência de tempo é em milisegundos

//Variáveis para relacionar os atuadores
int QuantidadeDeAtuadores = 0;
int PortDosAtuadores[6];
int TempDosAtuadores[6];
String NomeDosAtuadores[6];
String HoraDosAtuadores[6]; // Horário para ligar o atuador no modo automático
int TimeStampDosAtuadores[6]; // Guarda o TimeStamp atual dos atuadores no momento que estes são acionados
long int TempoLigadoAtuador[6];
bool autoaux[6];

//Variável para controlar o modo de operação do sistema, entre automático e manual
String modo;

//Variáveis auxiliares para controlar a passagem de tempo
unsigned int temp = 0;
unsigned int temp2 = 0;
unsigned int tempAtuador = 0;
//Variáveis para apresentar último horário de registro de tempo
String data1; // data em formato BR
String hora; // Registro de horas
String hora2; // Registro de horas para conferir atuadores
String DoW; //Dia da semana
String timestamp; // timestamp
//-------------------------------------------------------------------------------------------------------------------

//Para o WatchDog

hw_timer_t *timer = NULL; //faz o controle do temporizador (interrupção por tempo)

//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

//Prototipo das funções criadas - elas estarão descritas mais a seguir
//Prototipo das funções criadas - elas estarão descritas mais a seguir
//Prototipo das funções criadas - elas estarão descritas mais a seguir
// Confere o vetor de redes conhecidas e se conecta a uma
void Conectar_Em_Rede_Conhecida();
//Função para salvar informações no SD card
void SaveSD(int i, String Men1, String Men2, String Men3, String Men4, String Men5, String Men6, String Men7);
//Função auxiliar da SaveSD
void EditarArquivo(fs::FS &fs, const char * local, const char * mensagem);
//Função para verificar a conexão
void Verificaconexao();
//Função para postar resultados no HTML
void postHTML();
//Função para definir a lista de sensores baseado em um arquivo do SD Card
void DefinirSensores();
//Função para registro das informações coletadas pelos sensores
void RegistraSensores(int SensorAux);
//Função para ler os sensores
void LerSensores();
//Função para ativação do Watchdog
void resetModule();

//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

//Incialização/Setup do ESP
//Incialização/Setup do ESP
//Incialização/Setup do ESP
void setup() {
  //Pequeno delay para preparar os sensores e outros componentes
  delay(500);

  //Inicializando comunicação serial para apresentar informações pela serial
  Serial.begin(115200);

  //Primeira etapa na inicialização é pegar os dados dos sensores no cartão SD
  // init SD card
  pinMode(4, OUTPUT);//Pino para verificação do cartão de memória
  //Inicialmente o pino de indicação do cartão estará desativado e em caso de erro no cartão, irá ligar o led
  SD.begin(SDCS_PIN);
  while (!SD.begin(SDCS_PIN)) { //Ficará preso nessa linha enquanto o cartão não for devidamente colocado
    Serial.println("===================================================================================================");
    Serial.println("Não foi possível iniciar o cartão SD.");
    Serial.println("===================================================================================================");
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);
  }
  
  //Função para definir os arrays dos sensores
  DefinirSensores();

  //Função para definir a quantidade  de atuadores e seus nomes
  Ativaratuadores();

  //Declarando os pinos de entradas dos sensores
  for (int x = 0; x < QuantidadeDeSensores; x++) {
    pinMode(PortDosSensores[x], INPUT);
    ValoresDosSensores[x] = 0; // Inicializando os valores dos registros das leituras dos sensores como zero
  }//Fim do for para declarar as entradas dos sensores
  //Declarando os pinos de saídas para os atuadores
  for (int x = 0; x < QuantidadeDeAtuadores; x++) {
    pinMode(PortDosAtuadores[x], OUTPUT);

  }//Fim do for para declarar as ports de saídas dos atuadores

  //Inicializando a rede Wifi, conectando na primeira rede conhecida que encontrar
  Conectar_Em_Rede_Conhecida();
  Serial.println("===================================================================================================");
  Serial.println("WiFi conectada.");

  //Demonstrando o endereço IP do ESP
  Serial.println("Endereço de IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("===================================================================================================");
  server.begin();

  //Configurações para o WatchDog e suas interrupções
  //hw_timer_t * timerBegin(uint8_t num, uint16_t divider, bool countUp)
  /*
    num: é a ordem do temporizador, o sistema tem quatro temporizadores com registros [0,1,2,3].

    divider: É um prescaler (ajuste de escala, no caso da frequencia por fator).
    Para fazer um agendador de um segundo será utilizado o divider como 80 (clock principal do ESP32 é 80MHz). Cada
    instante será T = 1/(80) = 1us

    countUp: True o contador será progressivo
  */
  timer = timerBegin(0, 80, true); //timerID 0, div 80
  //timer, callback, interrupção de borda
  timerAttachInterrupt(timer, &resetModule, true);
  //timer, tempo (us), repetição
  timerAlarmWrite(timer, 60000000, true);
  timerAlarmEnable(timer); //habilita a interrupção


  //Primeiro registro de passagem de tempo -> antes de entrar no loop principal
  temp = millis();
  temp2 = millis();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//Loop principal do projeto
//Loop principal do projeto
//Loop principal do projeto

void loop() {

  //A cada passagem de tempo será feita uma amostragem da leitura dos sensores
  if (millis() - temp > (TempoDeAmostragem / NumLei)) {
    SensorAux = SensorAux + 1;
    LerSensores();
    //Atualiza temp
    temp = millis();
  }//Fim do if de registro dos sensores


  //Confere se o ESP esta conectado a rede
  Verificaconexao();

  //Segunda etapa é SensorAuxde registrar as redes encontradas no SD card a cada "TempoDeAmostragem" segundos
  //A coleta de novas RSSI deve acontecer somente a cada "TempoDeAmostragem" segundos
  if (millis() - temp2 >= TempoDeAmostragem) {
    //A cada "TempoDeAmostragem" segundos será registrado uma amostra das leituras propostas
    RegistraRSSIRedes();
    RegistraSensores();
    SensorAux = 0; //Zera o contador de leitura
    temp2 = millis(); //Novo registro de milisegundos atual
  }//Fim da condicional que confere a passagem de tempo "TempoDeAmostragem" segundos


  //Terceira etapa verifica se o sistema irá atuar no modo automatico ou manual
  //No caso do manual, a função responderá durante o post do html
  //No caso de automatico, será chamada a função AtuacaoAutomatica
  
  ConferirModoCartao(); //Conferindo último registro de atuação em modo automático ou manual
    
  if (modo == "ON") {
    AtuacaoAutomatica();
  }


  //Post de resultados no HTML
  postHTML();

  //Função para atualizar o watchdog e conferir se o ESP32 travou muito tempo por algum motivo
  //Caso o dispositivo tenha travado, ele irá reiniciar sozinho
  ConfereWatchDog(); //60 segundos sem ser atualizado vai reiniciar o ESP32

}//Fim do Void Loop


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//Neste ponto serão colocadas as funções criadas para o projeto
//Neste ponto serão colocadas as funções criadas para o projeto
//Neste ponto serão colocadas as funções criadas para o projeto

void Conectar_Em_Rede_Conhecida() {
  //Como primeira etapa será feito um scan das redes wifi nas proximidades
  int n = WiFi.scanNetworks();
  //com o número de redes consultadas, será feito uma tentativa de conexção com as redes conhecidas
  //entre estas no alcance
  Serial.println("===================================================================================================");
  Serial.println("Scan terminado");
  if (n == 0) {
    Serial.println("Nenhuma rede encontrada");
  } else {
    //Print na quantidade de redes encontras
    Serial.print(n);
    Serial.println(" Redes encontradas ");
    //apresentando as redes encontradas
    for (int i = 0; i < n; ++i) {
      // Print na SSID e RSSI de cada rede wifi encontrada
      Serial.println("Rede Nº " + String(i) + " - SSID: " + (WiFi.SSID(i)) + " - MAC: " + WiFi.BSSIDstr(i) + " - RSSI: " + String(WiFi.RSSI(i)) + " dBm.");
      //Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
      delay(10);
    }//Fim do for que irá percorrer as redes encontradas
  }//Fim do if
  //Conectando a uma rede conhecida caso ela esteja dentre as redes encontradas no scan
  int x = 0;
  //Aqui será tentado conectar o ESP32 a uma rede conhecida uma a uma, no caso na primeira
  //que encontrar
  while (x <= NRC) {
    //Conferindo se entre as redes encontradas tem uma conhecida
    for (int i = 0; i < n; ++i) {
      if (WiFi.SSID(i) == ssid[x]) {
        //Tenta conectar a rede
        WiFi.begin(ssid[x], password[x]);
        Serial.println("Tentando se Conectar com a rede WiFi: " + String(ssid[x]));
        while (WiFi.status() != WL_CONNECTED) {
          Serial.print(".");
          delay(500);
          if (WiFi.status() == WL_CONNECT_FAILED) { //Caso demorar muito na tentatova deconexão, o esp irá
            //tentar se conectar a outra rede conhecida.
            Serial.println(" ");
            break;
          } // Fim do IF que confere se passou 5 segundos de conexão
        }//Fim do While do WiFi.status() que indica se o esp esta conectado

        //Caso a plataforma tenha conseguido se conectar o laço for deverá ser desfeito.
        if ((WiFi.status() == WL_CONNECTED)) {
          Serial.println("");
          Serial.println("Conectado a Rede " + (WiFi.SSID(i)) + " com RSSI : " + String(WiFi.RSSI(i)) + " dBm.");
          i = 999;
          x = 999;
        }//Fim do IF que confere se a rede foi conectada.

      }//Fim do If dque confere se a rede é conhecida.

    }//Fim do for que irá percorrer as redes encontradas

    x++; //Caso não encontre a rede incrementa irá tentar encontrar outra rede conhecida.
  }
  Serial.println("===================================================================================================");
}//Fim da função Conectar_Em_Rede_Conhecida

//--------------------------------------------------------------------------------------------------------------------

//Função para verificar se o Esp esta conectado, caso esteja continha direto a função, caso contrário
//tenta reconectar
void Verificaconexao() {
  if ((WiFi.status() != WL_CONNECTED)) {
    Conectar_Em_Rede_Conhecida(); //Tenta reconectar a uma rede conhecida
  } else if (WiFi.status() == WL_IDLE_STATUS) {
    Serial.println("===================================================================================================");
    Serial.println("Dispositivo perdeu a conexão ou não consegue se conectar a alguma rede");
    Serial.println("===================================================================================================");
  }//Fim do if
}//Fim da função Verificaconexao


//--------------------------------------------------------------------------------------------------------------------

//Função de Conferir informações de data, horários e timestamp
Date getDate() {
  //força atualização do horário
  timeClient.forceUpdate();

  //Recupera os dados de data e horário usando o client NTP e coloca estes em char
  char* strDate = (char*)timeClient.getFormattedDate().c_str();

  //Passa os dados da string para a struct
  Date date;
  sscanf(strDate, "%d-%d-%dT%d:%d:%dZ",
         &date.year,
         &date.month,
         &date.day,
         &date.hours,
         &date.minutes,
         &date.seconds);

  //Dia da semana de 0 a 6, sendo 0 o domingo
  date.dayOfWeek = timeClient.getDay();
  date.timestamp = timeClient.getEpochTime();

  return date;
}//Fim da função GetDate

//--------------------------------------------------------------------------------------------------------------------

//Função para coleta de RSSI das redes
void RegistraRSSIRedes() {
  int RSSIScan[NRC]; // Array de RSSI de redes conhecidas para armazenamento
  String SSIDScan[NRC];// Array para armazenamaneto das redes conhecidas encontradas
  String MACScan[NRC]; // Array para armazenamento do MAC das redes conhecidas encontradas
  //A cada inicio de operações, é colocado os valores de RSSI do array para -999 como referência
  //a sinal ausente, em seguida esses valores serão substituidos por novos valores caso seja
  //encontrado um valor para aquela rede
  for (int x = 0; x < NRC ; x++) {
    RSSIScan[x] = -999;
  }

  //Realizando o Scan das redes
  int n = WiFi.scanNetworks();
  //Conferindo se as redes encontradas pertencem ao conjunto de redes conhecidas
  for (int x = 0; x < n; x++) {
    for (int w = 0; w < NRC; w++) {
      if (WiFi.SSID(x) == ssid[w] && WiFi.BSSIDstr(x) == mac[w]) { //Caso sejam conhecidas, as redes
        SSIDScan[w] = WiFi.SSID(x);                           //serão armazenadas nas respectivas
        MACScan[w] = WiFi.BSSIDstr(x);                            //posições das arrays auxiliares
        RSSIScan[w] = WiFi.RSSI(x);
        delay(10); //Pequeno delay só para esperar novas mensagens da função
      }
    }
  }
  //Armazenando todas as redes encontradas e conhecidas no data logger
  //Pegando informações de data,horário e timestamp
  Date date = getDate(); //conferir struct da função
  data1 = String(date.day) + "-" + String(date.month) + "-" + String(date.year);
  hora = String(date.hours) + ":" + String(date.minutes) + ":" + String(date.seconds);
  timestamp = String(date.timestamp);
  DoW = dayOfWeekNames[date.dayOfWeek];
  String mens;
  for (int x = 0; x < NRC ; x++) {
    if (RSSIScan[x] != -999) { //Armazenamento apenas para as redes conhecidas com RSSI diferente de -999
      //Primeiro parâmetro = 1, salvar redes conhecidas
      mens = WiFi.macAddress() + ";" + data1 + ";" + DoW + ";" + hora + ";" + String(date.timestamp) + ";" + SSIDScan[x] + ";" + MACScan[x] + ";" + String(RSSIScan[x]);
      SaveSD(1, mens, "REDESRSSIconhecida" );
    }
  }
  //Armazenando todas as redes encontradas
  for (int x = 0; x < n ; x++) { //serão armazenadas nas respectivas
    mens = WiFi.macAddress() + ";" + data1 + ";" + DoW + ";" + hora + ";" + String(date.timestamp) + ";" + WiFi.SSID(x) + ";" + WiFi.BSSIDstr(x) + ";" + String(WiFi.RSSI(x));
    SaveSD(1, mens, "TODASREDESRSSI");
  }
}//Fim da função RegistraRSSIRedes

//--------------------------------------------------------------------------------------------------------------------


void postHTML() {
  WiFiClient client = server.available(); //client será true toda vez que um novo client pedir
  String mens; //Variável para ecsrever textos para o cartão SD
  //uma requisição da html (porta 80)
  if (client) { //Se tiver um novo client (true)
     Serial.println("New Client."); //Avisa na Serial a chegada de uma nova requisição de um client
    String currentLine = ""; // currentLine é a variável para armazenar mensagens de requisição
    //Enquanto o client estiver conectado, será lido sua requisição e armazenado na variável "c"
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c); //apresenta qual a mensagem do client
        if (c == '\n') { //a mensagem \n é um simbolo de pular linha, que significa aqui o fim da
          //mensagem do client
          if (currentLine.length() == 0) { //Então, se variável currenLine estiver igual a 0,
            //signific que pode fazer a requisição, que no caso
            //será postar o html
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            //A mensagem a seguir estará em formato html
            //De forma geral:
            //<a ... /a> permite clicar no texto (elemento âncora)
            //href=\"/ var \" retorna em href a letra ou mensagem caso seja clickado (no ex: H ou L)
            //<br> é quebra de linha
            //Conferindo no cartão qual o modo de operação, se Modo automatico esta ativado
            client.println("Status do Modo Automatico: "+ modo+".<br>");            
            client.print("Click <a href=\"/AUTO_"+modo+"\">Aqui</a> para Ativar ou Desativar o modo Automatico.<br>");
            client.println("<br>");
            client.println("Lista de atuadores e seus Status. <br>");
            String statusPort;
              for (int x = 0; x < QuantidadeDeSensores; x++) {
                if (digitalRead(PortDosAtuadores[x]) == HIGH) {
                  statusPort = "ON";
                  client.print("Atuador: " + NomeDosAtuadores[x] + " - Status: " + statusPort + "       ||||");
                  client.print("||||||||    Click para: <a href=\"/Ligar_" + NomeDosAtuadores[x] + "\">Ativar</a> ou <a href=\"/Desligar_" + NomeDosAtuadores[x] + "\">Desativar</a>.<br>");
                } else {
                  statusPort = "OFF";
                client.print("Atuador: " + NomeDosAtuadores[x] + " - Status: " + statusPort + "    ||||");
                client.print("||||||||    Click para: <a href=\"/Ligar_" + NomeDosAtuadores[x] + "\">Ativar</a> ou <a href=\"/Desligar_" + NomeDosAtuadores[x] + "\">Desativar</a>.<br>");
                };
             }//fim do for
            client.println("<br>"); //Pulando linha
            //Informando a segui as redes encontradas no scan
            int n = WiFi.scanNetworks();
            //com o número de redes consultadas, será feito uma tentativa de conexção com as redes conhecidas
            //entre estas no alcance
            client.println("Scan terminado");
            client.println("<br>");
            if (n == 0) {
              client.println("Nenhuma rede encontrada");
            } else {
              //Print na quantidade de redes encontras
              client.print("Numero de Redes encontradas: " + String(n) + "<br>");
              //apresentando as redes encontradas
              for (int i = 0; i < n; ++i) {
                // Print na SSID e RSSI de cada rede wifi encontrada
                client.println("Rede Num. " + String(i + 1) + " - SSID: " + (WiFi.SSID(i)) + " - MAC: " + WiFi.BSSIDstr(i) + " - RSSI: " + String(WiFi.RSSI(i)) + " dBm<br>");
                delay(10);
              }//Fim do for que irá percorrer as redes encontradas
            }//Fim do if
            client.println("<br>"); //Pulando alguns espaços
            if (hora == "") {
              client.println("Sem registros.<br>");
            } else {
            }//fim do if de conferir registro de data
            client.println("<br>"); //Pulando alguns espaços
            for (int x = 0; x < QuantidadeDeSensores; x++) {
              int t = analogRead(PortDosSensores[x]);
              client.println("Leitura do sendor: " + NomeDosSensores[x] + " - " + String(t) + ".<br>");
            }
            client.println("<br>"); //Pulando alguns espaços
            client.println("Ultimo registro as " + hora + " da data (" + data1 + ").<br>");
            break; //Fim do post da HTML
          } else {
            currentLine = ""; //Caso não chegue nenhum caractere, mantém currentLine sem espaços vazios
          }//Fim do else
        } else if (c != '\r') { // ‘\r’ significa um caractere de retorno, indicando o início de
          //uma nova linha.
          currentLine += c;
        }//Fim do else if
      


        
        for (int x = 0; x < QuantidadeDeSensores; x++) {
          if (currentLine.endsWith("GET /Ligar_" + NomeDosAtuadores[x]) && (digitalRead(PortDosAtuadores[x] == HIGH))) {
            digitalWrite(PortDosAtuadores[x], HIGH);
            mens = NomeDosAtuadores[x] + ";" + String(PortDosAtuadores[x]) + ";ON;" + WiFi.macAddress() + ";" + data1 + ";" + hora + ";" + String(timestamp)+";Ativacao Manual"+";0";
            SaveSD(1, mens, "RegistroDosAtuadores");
          }//fim do if de ligar o atuador
        }//Fim do for para verificar se é para ligar o atuador pela HTML
        for (int x = 0; x < QuantidadeDeSensores; x++) {
          if (currentLine.endsWith("GET /Desligar_" + NomeDosAtuadores[x]) && (TempoLigadoAtuador[x]>0)) {
            digitalWrite(PortDosAtuadores[x], LOW);
            mens = NomeDosAtuadores[x] + ";" + String(PortDosAtuadores[x]) + ";OFF;" + WiFi.macAddress() + ";" + data1 + ";" + hora + ";" + String(timestamp)+";Desativacao Manual;"+String((millis() - TempoLigadoAtuador[x])/1000);
            SaveSD(1, mens, "RegistroDosAtuadores");
          }//fim do if para desligar Atuadores pela HTML
        }//Fim do for para verificar se é para desligar o atuador pela HTML

                //Depois de apresentar as informações, irá ligar ou desligar os atuadores de acordo com o click da página
            //Conferindo se o modo mudou ou não
          if (currentLine.endsWith("GET /AUTO_"+modo) || currentLine.endsWith("GET /AUTO_"+modo+" ")|| currentLine.endsWith("GET /AUTO_ON") || currentLine.endsWith("GET /Auto_ON")|| currentLine.endsWith("GET /AUTO_ON ") || currentLine.endsWith("GET /AUTO_ON ")){
               if (modo == "ON" || modo == "ON " || modo ==  " ON" || modo == " ON "){ 
                  mens = "MododeOperacao;OFF;";
                  SaveSD(2, mens, "Modo");
                    ConferirModoCartao(); //Conferindo registro de atuação em modo automático ou manual
                    //Conferindo qual modo esta ativo, Manual ou automático?
                    Serial.println("Modo Auto:"+modo+".");
               }else{ 
                  mens = "MododeOperacao;ON;";
                  SaveSD(2, mens, "Modo");
                  ConferirModoCartao(); //Conferindo registro de atuação em modo automático ou manual
                  //Conferindo qual modo esta ativo, Manual ou automático?
                  Serial.println("Modo Auto:"+modo+".");
                  for(int x =0; x<QuantidadeDeAtuadores;x++){// colocando todas as saídas como disponíveis 
                    autoaux[x] = HIGH;
                    }
            }//Fim do IF       
         } // Fim do IF que muda o modo de operação


        
      }//Fim do if do client liberado
    }//Fim do while do client connected
    client.stop();
    Serial.println("Client Disconnected.");

  }//Fim do fim do client
 }//Fim da função postHTML

//--------------------------------------------------------------------------------------------------------------------

//Função para mapear no cartão de memória os sensores e suas ports na plataforma
void DefinirSensores() {
  //Variáveis auxiliares para manipulação de strings
  String texto1;

  //Abrindo o SD card com o arquivo desejado
  myFile = SD.open("/ListaDeSensores.txt", FILE_READ);
  if (myFile) {
    while (myFile.available()) {
      texto1 = myFile.readStringUntil('\n'); //pega todo o texto at´chegar o primeiro '\n' de leitura, que se
    }//Fim da coleta de caracteres dos arquivos
    // Fechando  o arquivo:
    myFile.close();
  }
  //Monta os arrays sobre os sensores baseado no texto lido no SD card
  //Confira no arquivo como é formado o texto, mas em geral as informações são separadas por ";"
  //Sendo sempre a primeira informação o nome do sensor e depois a Port desse sensor

  for (int x = 0; x < sizeof(NomeDosSensores) / (sizeof(NomeDosSensores[0])); x++) {
    NomeDosSensores[x] = texto1.substring(0, texto1.indexOf(';'));
    texto1 = texto1.substring(1 + texto1.indexOf(';'));
    PortDosSensores[x] = (texto1.substring(0, texto1.indexOf(';'))).toInt();
    //confere se o número de caracteres restantes do texto ainda é maior que 0
    //Se for é porque ainda tem sensores para contar, caso contrário para de contar
    //por ter passado do limite
    if (texto1.length() > 0) {
      QuantidadeDeSensores = QuantidadeDeSensores + 1;
    } else {
      x = sizeof(PortDosSensores) + 1;
    }
    //Retirando a última parte do texto antes de recomeçar o loop
    texto1 = texto1.substring(1 + texto1.indexOf(';'));
  }//Fim do for
}//Fim da função de definir sensores

//--------------------------------------------------------------------------------------------------------------------

void  Ativaratuadores() {
  //Variáveis auxiliares para manipulação de strings
  String texto1;

  //Abrindo o SD card com o arquivo desejado
  myFile = SD.open("/ListaDeAtuadores.txt", FILE_READ);
  if (myFile) {
    while (myFile.available()) {
      texto1 = myFile.readStringUntil('\n'); //pega todo o texto até chegar o primeiro '\n' de leitura, que se
    }//Fim da coleta de caracteres dos arquivos
    // Fechando  o arquivo:
    myFile.close();
  }
  for (int x = 0; x < (sizeof(PortDosAtuadores) / sizeof(PortDosAtuadores[0])); x++) {
    NomeDosAtuadores[x] = texto1.substring(0, texto1.indexOf(';')); // Nome do sensor
    texto1 = texto1.substring(1 + texto1.indexOf(';'));
    PortDosAtuadores[x] = (texto1.substring(0, texto1.indexOf(';'))).toInt(); // Número da port
    texto1 = texto1.substring(1 + texto1.indexOf(';'));
    TempDosAtuadores[x] = (texto1.substring(0, texto1.indexOf(';'))).toInt(); // timestamp de tempo ligado
    texto1 = texto1.substring(1 + texto1.indexOf(';'));
    HoraDosAtuadores[x] = (texto1.substring(0, texto1.indexOf(';'))); // Hora:Minutos de Ligar no automático
    
    //confere se o número de caracteres restantes do texto ainda é maior que 0
    //Se for é porque ainda tem atuadores para contar, caso contrário para de contar
    //por ter passado do limite
    if (texto1.length() > 0) {
      QuantidadeDeAtuadores = QuantidadeDeAtuadores + 1;
    }
    //Retirando a última parte do texto antes de recomeçar o loop
    texto1 = texto1.substring(1 + texto1.indexOf(';'));
  } //Fim do for
}//Fim da função


//--------------------------------------------------------------------------------------------------------------------


//Função para registro das informações coletadas pelos sensores
void RegistraSensores() {
  Serial.println("===================================================================================================");

  //Fazendo a média das leituras e apresentando na serial os seus resultados
  for (int x = 0; x < QuantidadeDeSensores; x++) {
    //Leitura baseada nas posições dos senores
    ValoresDosSensores[x] = (ValoresDosSensores[x] / SensorAux);
    Serial.println("Leitura Media do sensor " + NomeDosSensores[x] + " é: " + ValoresDosSensores[x]);
    String mens = NomeDosSensores[x] + ";" + String(PortDosSensores[x]) + ";" + String(ValoresDosSensores[x]) + ";" + WiFi.macAddress() + ";" + data1 + ";" + hora + ";" + timestamp + ";" + DoW;
    //SaveSD com modo(1), mensagem a ser salva(mens), e arquivo a ser salvo (path=TODOSOSSENSORES)
    SaveSD(1, mens, "TODOSOSSENSORES");
  }//Fim do for
  Serial.println("===================================================================================================");

}//Fim da Função RegistraSensores


//--------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------
//Função para leitura dos sensores
void LerSensores() {
  Serial.println("===================================================================================================");

  for (int x = 0; x < QuantidadeDeSensores; x++) {
    //Leitura baseada nas posições dos senores
    int t = analogRead(PortDosSensores[x]);
    if(t == 0){
      delay(5); //Algumas leituras tem dado 0, então será feita uma segunda tentativa de leitura  para pegar o valor da leitura
      t = analogRead(PortDosSensores[x]);
      }
    Serial.println("Sensor " + NomeDosSensores[x] + " Tem leitura: " + t);
    ValoresDosSensores[x] = ValoresDosSensores[x] + t;
    delay(5);
  }//Fim do for
Serial.println("=============================================================================================================");

}//Fim da função LerSensores

//--------------------------------------------------------------------------------------------------------------------

//Função para conferir se o Esp32 travou
void ConfereWatchDog() {
  timerWrite(timer, 0); //reseta o temporizador (alimenta o watchdog)
} //Fim da função


//--------------------------------------------------------------------------------------------------------------------


//função que o temporizador irá chamar, para reiniciar o ESP32
void IRAM_ATTR resetModule() {
  ets_printf("(watchdog) reiniciar\n"); //imprime no log
  esp_restart(); //reinicia o chip
}//Fim da função do watchdog


//--------------------------------------------------------------------------------------------------------------------
//Função para  atuação em modo automatico
void AtuacaoAutomatica() {
  Date date = getDate(); //conferir struct da função
  hora = String(date.hours) + ":" + String(date.minutes) + ":" + String(date.seconds);
  hora2 = String(date.hours) + ":" + String(date.minutes);
  String mens;
  //Ligar os atuadores de acordo com o horário estipulado no cartão de memoria, ignorando os segundos
  for (int x = 0; x < QuantidadeDeAtuadores ; x++) {
    if (HoraDosAtuadores[x] == hora2 && autoaux[x] == HIGH) {
      digitalWrite(PortDosAtuadores[x], HIGH);
      TimeStampDosAtuadores[x] = date.timestamp;
      TempoLigadoAtuador[x] = millis();// Registrando o tempo atual 
      mens = NomeDosAtuadores[x] + ";" + String(PortDosAtuadores[x]) + ";Ativado;" + WiFi.macAddress() + ";" + data1 + ";" + hora + ";" + String(date.timestamp)+";Modo Automatico ON"+";0";// O "0" depois do modo automatico indica que ligou
      SaveSD(1, mens, "TODOSOSSENSORES");
      autoaux[x] = LOW; // Variável auxiliar para não precisar ativar o modo
    }//Começa a contar após terminar o minuto do acionamento
    //Confere o Millis atual - o timestamp
    if ((date.timestamp - TimeStampDosAtuadores[x] >= TempDosAtuadores[x]) && autoaux[x] == LOW) {
      digitalWrite(PortDosAtuadores[x], LOW); // Caso tenha passado o tempo e seja necessário desligar
      mens = NomeDosAtuadores[x] + ";" + String(PortDosAtuadores[x]) + ";Desativado;" + WiFi.macAddress() + ";" + data1 + ";" + hora + ";" + String(date.timestamp)+";Modo Automatico ON"+";"+String((millis() - TempoLigadoAtuador[x])/1000); //Registro depois de Modo automatico indica o tempo que ficou ligado
      SaveSD(1, mens, "RegistroDosAtuadores");
      autoaux[x] = HIGH; //Depois do modo desativar automaticamente, volta a permitir a ativação
    }//Fim do if para desligar
  }//Fim do for
}//Fim da função AtuacaoAutomatica

//--------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------

//Função para determinar pelo cartão qual o modo de operação do sistema
String ConferirModoCartao() {
  String texto1;
  myFile = SD.open("/Modo.txt", FILE_READ);
  if (myFile) {
    while (myFile.available()) {
      texto1 = myFile.readStringUntil('\n'); //pega todo o texto at´chegar o primeiro '\n' de leitura, que se
    }//Fim da coleta de caracteres dos arquivos
    // Fechando  o arquivo:
    myFile.close();
  }
   texto1 = texto1.substring(1 + texto1.indexOf(';'));
    modo = (texto1.substring(0, texto1.indexOf(';'))); // timestamp de tempo ligado
}//Fim da função ConferirModoCartao()

//--------------------------------------------------------------------------------------------------------------------

//Função para salvar informações no SD Card.
/*A função pode salvar informações baseada na entrada i.

  Para cada valor de i será salvo uma mensagem de forma diferente no SD Card, normalmemte associado
  a um arquivo diferente e este arquivo terá sua propria forma de salvar os dados.

*/
void SaveSD(int i, String Mens, String path) {
  //Nessa variável deverá ser concatenado todas as informações a serem salvas
  String salvamensa;
  Serial.println("===================================================================================================");
  switch (i) {
    //Salvando informações apenas das redes conhecidas
    case 1:
      //Com i sendo 1, a mensagem salva será adição de informações no arquivo requisitado
      //Será utilizado para qualquer requisição que irá adicionar informações ao arquivo sem apagar nada
      Serial.println("Função 1 - Salvar Informações no SD Card adicionando novas linhas");
      myFile = SD.open("/" + path + ".txt", FILE_APPEND); //Concatenando a mensagem para abrir o cartão e adicionar uma nova linha
      //Conferindo a existência do arquivo.
      if (myFile) {
        Serial.println("Salvando informações no arquivo" + path + " com o formato: ");
        Serial.println(Mens);
        myFile.println(Mens);
        myFile.close(); //Fechando o arquivo.
      } else {
        Serial.println("Não foi possível abrir o cartão SD.");
      }
      Serial.println("===================================================================================================");
      break;
    //Salvando informações do modo operação reescrevendo o arquivo
    case 2:
      //Com i sendo 2, a mensagem salva será adição de informações de todas as redes coletadas no aquivo
      Serial.println("Função 2 - Salvar Informações no SD Card apagando as informações anteriores do arquivo");
      myFile = SD.open("/" + path + ".txt", FILE_WRITE); //Concatenando a mensagem para abrir o cartão e adicionar uma nova linha
      //Conferindo a existência do arquivo.
      if (myFile) {
        Serial.println("Salvando informações no arquivo" + path + " com o formato: ");
        Serial.println(Mens);
        myFile.println(Mens);
        myFile.close(); //Fechando o arquivo.
      } else {
        Serial.println("Não foi possível abrir o cartão SD.");
      }
      myFile.close(); //Fechando o arquivo.
      Serial.println("===================================================================================================");
      break;

    //Adicionar novas informações a serem salvas Nesse espaço obecendo a estrutura do case

    //Adicionar novas informações a serem salvas Nesse espaço obecendo a estrutura do case

    default://Se for enviado um comando de mensagem não esperado, envie mensagem de erro como padrão
      //Para a serial
      Serial.println("Comando desconhecido na função SaveSD.");
      Serial.println("===================================================================================================");
      break;
  }//Fim do Switch case
}//Fim da função SaveSD
//--------------------------------------------------------------------------------------------------------------------
