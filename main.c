/***
 * Communication Serial
 * By: Adimael Santos da Silva
 * Código disponível em: github.com/adimael
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"
#include "hardware/i2c.h"
#include "font.h"
#include "ssd1306.h"
#include "ws2812.pio.h"

// Definição de pinos e configurações
#define WS2812_PIN 7       // Pino da matriz de LEDs
#define NUM_LEDS 25        // Número de LEDs na matriz
#define LED_RGB_G 11       // Pino do LED verde
#define LED_RGB_B 12       // Pino do LED azul
#define LED_RGB_R 13       // Pino do LED vermelho
#define BOTAO_A 5          // Pino do botão A
#define BOTAO_B 6          // Pino do botão B
#define I2C_PORT i2c1      // Porta I2C
#define I2C_SDA 14         // Pino SDA do I2C
#define I2C_SCL 15         // Pino SCL do I2C
#define ENDERECO 0x3C      // Endereço do display OLED
#define FRAMES 10          // Número de frames (números de 0 a 9)

PIO pio = pio0;            // Instância do PIO
bool ok;                   // Flag para verificar sucesso na configuração do clock
uint16_t i;                // Variável de iteração
uint32_t valor_led;        // Valor do LED em formato RGB
uint sm;                   // State machine do PIO
uint offset;               // Offset do programa PIO
double r = 0.0, b = 0.0, g = 0.0; // Cores RGB

ssd1306_t ssd;             // Estrutura do display OLED

static volatile uint32_t lastEventButton = 0; // Timestamp do último evento de botão

double numeros[FRAMES][NUM_LEDS] =
{ 
    {
    0.01, 0.01, 0.01, 0.01, 0.01,
    0.01, 0.0, 0.0, 0.0, 0.01,
    0.01, 0.0, 0.0, 0.0, 0.01,
    0.01, 0.0, 0.0, 0.0, 0.01,
    0.01, 0.01, 0.01, 0.01, 0.01
    },

    {
    0.0, 0.0, 0.01, 0.0, 0.0,
    0.0, 0.01, 0.01, 0.0, 0.0,
    0.0, 0.0, 0.01, 0.0, 0.0,
    0.0, 0.0, 0.01, 0.0, 0.0,
    0.0, 0.0, 0.01, 0.0, 0.0
    },

    {
    0.01, 0.01, 0.01, 0.01, 0.01,
    0.0, 0.0, 0.0, 0.0, 0.01,
    0.01, 0.01, 0.01, 0.01, 0.01,
    0.01, 0.0, 0.0, 0.0, 0.0,
    0.01, 0.01, 0.01, 0.01, 0.01
    },


    {
    0.01, 0.01, 0.01, 0.01, 0.01,
    0.0, 0.0, 0.0, 0.0, 0.01,
    0.0, 0.01, 0.01, 0.01, 0.01,
    0.0, 0.0, 0.0, 0.0, 0.01,
    0.01, 0.01, 0.01, 0.01, 0.01
    },



    {
    0.01, 0.0, 0.0, 0.01, 0.0,
    0.01, 0.0, 0.0, 0.01, 0.0,
    0.01, 0.01, 0.01, 0.01, 0.0,
    0.0, 0.0, 0.0, 0.01, 0.0,
    0.0, 0.0, 0.0, 0.01, 0.0
    },


    {
    0.01, 0.01, 0.01, 0.01, 0.01,
    0.01, 0.0, 0.0, 0.0, 0.0,
    0.01, 0.01, 0.01, 0.01, 0.01,
    0.0, 0.0, 0.0, 0.0, 0.01,
    0.01, 0.01, 0.01, 0.01, 0.01
    },


    {
    0.01, 0.01, 0.01, 0.01, 0.01,
    0.01, 0.0, 0.0, 0.0, 0.0,
    0.01, 0.01, 0.01, 0.01, 0.01,
    0.01, 0.0, 0.0, 0.0, 0.01,
    0.01, 0.01, 0.01, 0.01, 0.01
    },


    {
    0.01, 0.01, 0.01, 0.01, 0.01,
    0.0, 0.0, 0.0, 0.0, 0.01,
    0.0, 0.0, 0.0, 0.01, 0.0,
    0.0, 0.0, 0.01, 0.0, 0.0,
    0.0, 0.01, 0.0, 0.0, 0.0
    },


    {
    0.01, 0.01, 0.01, 0.01, 0.01,
    0.01, 0.0, 0.0, 0.0, 0.01,
    0.01, 0.01, 0.01, 0.01, 0.01,
    0.01, 0.0, 0.0, 0.0, 0.01,
    0.01, 0.01, 0.01, 0.01, 0.01
    },

    {
    0.01, 0.01, 0.01, 0.01, 0.01,
    0.01, 0.0, 0.0, 0.0, 0.01,
    0.01, 0.01, 0.01, 0.01, 0.01,
    0.0, 0.0, 0.0, 0.0, 0.01,
    0.01, 0.01, 0.01, 0.01, 0.01
    }

 };

// Função para definir a intensidade das cores do LED e converter para formato RGB 32 bits
uint32_t matrix_rgb(double b, double r, double g) {
  unsigned char R = r * 255;
  unsigned char G = g * 255;
  unsigned char B = b * 255;
  return (G << 24) | (R << 16) | (B << 8);
}

// Função para "desenhar" o número na matriz de LEDs
void desenho_pio(double *desenho, uint32_t valor_led, PIO pio, uint sm, double r, double g, double b) {
  // Ordem dos LEDs para corrigir a inversão da imagem
  int ordem_leds[NUM_LEDS] = {
      24, 23, 22, 21, 20,
      15, 16, 17, 18, 19,
      14, 13, 12, 11, 10,
      5, 6, 7, 8, 9,
      4, 3, 2, 1, 0
  };

  for (int16_t i = 0; i < NUM_LEDS; i++) {
      int indice_leds = ordem_leds[i];
      valor_led = matrix_rgb(desenho[indice_leds], r, g); // Define a cor do LED
      pio_sm_put_blocking(pio, sm, valor_led); // Envia os dados para o PIO
  }
}

// Função para exibir o status dos LEDs e o texto recebido no display OLED
void atualizar_display(ssd1306_t *ssd, char *texto) {
  ssd1306_fill(ssd, false); // Limpa o display

  // Verifica o estado do LED verde
  bool estado_verde = gpio_get(LED_RGB_G);
  char *msg_verde = estado_verde ? "Verde ON" : "Verde OFF";
  ssd1306_draw_string(ssd, msg_verde, 0, 0);

  // Verifica o estado do LED azul
  bool estado_azul = gpio_get(LED_RGB_B);
  char *msg_azul = estado_azul ? "Azul ON" : "Azul OFF";
  ssd1306_draw_string(ssd, msg_azul, 0, 15);

  // Exibe o texto recebido no display
  ssd1306_draw_string(ssd, texto, 0, 30);

  ssd1306_send_data(ssd); // Atualiza o display
}

// Função de interrupção com debouncing para os botões
void gpio_irq_handler_BOTAO(uint gpio, uint32_t events) {
  // Obtém o tempo atual em microssegundos
  uint32_t current_time = to_us_since_boot(get_absolute_time());

  // Verifica se passou tempo suficiente desde o último evento
  if (current_time - lastEventButton > 200000) { // 200 ms de debouncing
      lastEventButton = current_time;

      if (gpio == BOTAO_A) {
          gpio_put(LED_RGB_G, !(gpio_get(LED_RGB_G))); // Alterna o estado do LED verde
          atualizar_display(&ssd, ""); // Atualiza o display sem texto adicional
          printf("Led verde %s\n", gpio_get(LED_RGB_G) ? "ligou" : "desligou");
      }

      if (gpio == BOTAO_B) {
          gpio_put(LED_RGB_B, !(gpio_get(LED_RGB_B))); // Alterna o estado do LED azul
          atualizar_display(&ssd, ""); // Atualiza o display sem texto adicional
          printf("Led azul %s\n", gpio_get(LED_RGB_B) ? "ligou" : "desligou");
      }
  }
}

int main() {

  // Inicialização da saída padrão
  stdio_init_all();

  // Configura a frequência de clock para 128 MHz
  ok = set_sys_clock_khz(128000, false);

  printf("Iniciando a transmissão PIO\n");
  if (ok) printf("Clock set to %ld\n", clock_get_hz(clk_sys));

  // Configuração do PIO (offset e state machine)
  offset = pio_add_program(pio, &pio_matrix_program);
  sm = pio_claim_unused_sm(pio, true);
  pio_matrix_program_init(pio, sm, offset, WS2812_PIN);

  // Inicialização dos LEDs
  gpio_init(LED_RGB_G);
  gpio_set_dir(LED_RGB_G, GPIO_OUT);
  gpio_put(LED_RGB_G, false);

  gpio_init(LED_RGB_B);
  gpio_set_dir(LED_RGB_B, GPIO_OUT);
  gpio_put(LED_RGB_B, false);

  // Inicialização dos botões
  gpio_init(BOTAO_A);
  gpio_set_dir(BOTAO_A, GPIO_IN);
  gpio_pull_up(BOTAO_A);

  gpio_init(BOTAO_B);
  gpio_set_dir(BOTAO_B, GPIO_IN);
  gpio_pull_up(BOTAO_B);

  // Inicialização do I2C
  i2c_init(I2C_PORT, 400 * 1000);
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);

  // Inicialização do display OLED
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
  ssd1306_config(&ssd);
  ssd1306_send_data(&ssd);

  // Limpa o display
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  // Configuração da interrupção com callback para os botões
  gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler_BOTAO);
  gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler_BOTAO);

  // Exibe o status inicial dos LEDs no display
  atualizar_display(&ssd, "");

  // Loop principal
  while (true) {
      if (stdio_usb_connected()) {
          char c;
          if (scanf("%c", &c) == 1) {
              printf("Recebido: %c\n", c);

              // Converte o caractere para uma string
              char texto[2] = {c, '\0'};

              // Atualiza o display com o texto recebido e o status dos LEDs
              atualizar_display(&ssd, texto);

              if (c >= '0' && c <= '9') {
                  int num = c - '0';
                  desenho_pio(numeros[num], valor_led, pio, sm, r, g, b);
              }
          }
      }
  }
}