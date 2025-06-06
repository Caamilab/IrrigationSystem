#include <stdio.h>               // Biblioteca padrão para entrada e saída
#include <string.h>              // Biblioteca manipular strings
#include <stdlib.h>              // funções para realizar várias operações, incluindo alocação de memória dinâmica (malloc)

#include "pico/stdlib.h"         // Biblioteca da Raspberry Pi Pico para funções padrão (GPIO, temporização, etc.)
#include "hardware/adc.h"        // Biblioteca da Raspberry Pi Pico para manipulação do conversor ADC
#include "pico/cyw43_arch.h"     // Biblioteca para arquitetura Wi-Fi da Pico com CYW43  

#include "lwip/pbuf.h"           // Lightweight IP stack - manipulação de buffers de pacotes de rede
#include "lwip/tcp.h"            // Lightweight IP stack - fornece funções e estruturas para trabalhar com o protocolo TCP
#include "lwip/netif.h"          // Lightweight IP stack - fornece funções e estruturas para trabalhar com interfaces de rede (netif)


#define WIFI_SSID "SSID" //MUDE
#define WIFI_PASSWORD "SENHA"//MUDE

// Definição dos pinos dos LEDs
#define LED_PIN CYW43_WL_GPIO_LED_PIN   // GPIO do CI CYW43
#define LED_BLUE_PIN 12                 // GPIO12 - LED azul
#define LED_GREEN_PIN 11                // GPIO11 - LED verde
#define LED_RED_PIN 13                  // GPIO13 - LED vermelho
#define PIN_BOMBA 9                     // GPO4 - saída 
#define PIN_SENSOR  4                 // GP20 - Entrada sensor

volatile bool MODO_AUTOMATICO = 0;

// Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
void gpio_config(void);

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

// Tratamento do request do usuário
void user_request(char **request);

// Função principal
int main()
{
    //Inicializa todos os tipos de bibliotecas stdio padrão presentes que estão ligados ao binário.
    stdio_init_all();

    // Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
    gpio_config();

    //Inicializa a arquitetura do cyw43
    while (cyw43_arch_init())
    {
        printf("Falha ao inicializar Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }

    // GPIO do CI CYW43 em nível baixo
    cyw43_arch_gpio_put(LED_PIN, 0);

    // Ativa o Wi-Fi no modo Station, de modo a que possam ser feitas ligações a outros pontos de acesso Wi-Fi.
    cyw43_arch_enable_sta_mode();

    // Conectar à rede WiFI - fazer um loop até que esteja conectado
    printf("Conectando ao Wi-Fi...\n");
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000))
    {
        printf("Falha ao conectar ao Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }
    printf("Conectado ao Wi-Fi\n");

    // Caso seja a interface de rede padrão - imprimir o IP do dispositivo.
    if (netif_default)
    {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    // Configura o servidor TCP - cria novos PCBs TCP. É o primeiro passo para estabelecer uma conexão TCP.
    struct tcp_pcb *server = tcp_new();
    if (!server)
    {
        printf("Falha ao criar servidor TCP\n");
        return -1;
    }

    //vincula um PCB (Protocol Control Block) TCP a um endereço IP e porta específicos.
    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Falha ao associar servidor TCP à porta 80\n");
        return -1;
    }

    // Coloca um PCB (Protocol Control Block) TCP em modo de escuta, permitindo que ele aceite conexões de entrada.
    server = tcp_listen(server);

    // Define uma função de callback para aceitar conexões TCP de entrada. É um passo importante na configuração de servidores TCP.
    tcp_accept(server, tcp_server_accept);
    printf("Servidor ouvindo na porta 80\n");

    while (true)
    {
        /* 
        * Efetuar o processamento exigido pelo cyw43_driver ou pela stack TCP/IP.
        * Este método deve ser chamado periodicamente a partir do ciclo principal 
        * quando se utiliza um estilo de sondagem pico_cyw43_arch 
        */
       if (MODO_AUTOMATICO) {//LEITURA DO SENSOR DE UMIDADE
        bool estado_sensor = gpio_get(PIN_SENSOR);
        gpio_put(PIN_BOMBA, estado_sensor);
        sleep_ms(200);
    }
        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
        sleep_ms(100);      // Reduz o uso da CPU
    }

    //Desligar a arquitetura CYW43.
    cyw43_arch_deinit();
    return 0;
}

// -------------------------------------- Funções ---------------------------------

// Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
void gpio_config(void){

    gpio_init(LED_BLUE_PIN);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
    gpio_put(LED_BLUE_PIN, false);
    
    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_put(LED_GREEN_PIN, false);
    
    gpio_init(LED_RED_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_put(LED_RED_PIN, false);

    gpio_init(PIN_BOMBA); 
    gpio_set_dir(PIN_BOMBA, GPIO_OUT);
    gpio_put(PIN_BOMBA, 0);

    gpio_init(PIN_SENSOR);
    gpio_set_dir(PIN_SENSOR, GPIO_IN);
}

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

// Tratamento do request do usuário - digite aqui
void user_request(char **request){

 if (strstr(*request, "GET /ligar") != NULL) {
        MODO_AUTOMATICO = 0;
        gpio_put(PIN_BOMBA, 1); // ON -> Fixo em HIGH
        gpio_put(LED_GREEN_PIN, 1);
        gpio_put(LED_BLUE_PIN, 0);
        gpio_put(LED_RED_PIN, 0);
    }
    else if (strstr(*request, "GET /desligar") != NULL) {
        MODO_AUTOMATICO = 0;
        gpio_put(PIN_BOMBA, 0); // OFF -> Fixo em LOW
        gpio_put(LED_RED_PIN, 1);
        gpio_put(LED_GREEN_PIN, 0);
        gpio_put(LED_BLUE_PIN, 0);
    }
    else if (strstr(*request, "GET /automatico") != NULL) {
        MODO_AUTOMATICO = 1;
        bool estado_sensor = gpio_get(PIN_SENSOR);
        printf("Estado do sensor: %d\n", estado_sensor);
        gpio_put(PIN_BOMBA, estado_sensor); // AUTO -> Inverte estado do sensor
        gpio_put(LED_BLUE_PIN, 1);
        gpio_put(LED_RED_PIN, 0);
    gpio_put(LED_GREEN_PIN, 0);}
    }


// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    // Alocação do request na memória dinámica
    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    printf("Request: %s\n", request);

    // Tratamento de request - Controle dos LEDs
    user_request(&request);

    // Cria a resposta HTML
    char html[1024];

snprintf(html, sizeof(html),
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<!DOCTYPE html><html><head>"
    "<title>Irrigation System</title>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<style>"
    "body{background:#228B22;font-family:sans-serif;text-align:center;margin-top:50px;}"
    "h1{font-size:6vw;margin-bottom:30px;color:#fff;}"
    "form{display:inline-block;margin:10px;}"
    "button{"
        "background:#8B4513;"
        "color:#fff;"
        "font-size:5vw;"
        "padding:4vw 10vw;"
        "border:none;"
        "border-radius:8px;"
        "cursor:pointer;"
        "width:70vw;"
        "max-width:220px;"
        "min-width:120px;"
        "box-sizing:border-box;"
        "margin-bottom:20px;"
    "}"
    "@media (min-width:600px){"
        "h1{font-size:48px;}"
        "button{font-size:32px;padding:20px 60px;width:220px;}"
    "}"
    "</style></head><body>"
    "<h1>Irrigation System</h1>"
    "<form action=\"./ligar\"><button>ON</button></form>"
    "<form action=\"./desligar\"><button>OFF</button></form>"
    "<form action=\"./automatico\"><button>AUTO</button></form>"
    "</body></html>"
);


    // Escreve dados para envio (mas não os envia imediatamente).
    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);

    // Envia a mensagem
    tcp_output(tpcb);

    //libera memória alocada dinamicamente
    free(request);
    
    //libera um buffer de pacote (pbuf) que foi alocado anteriormente
    pbuf_free(p);

    return ERR_OK;
}

