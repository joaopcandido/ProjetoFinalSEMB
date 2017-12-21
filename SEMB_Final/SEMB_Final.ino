#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>

#define SREG_GLOBAL_INT_ENABLE 7

uint16_t u16_count = 0, u16_cMin = 0;
char c_interrupt, c_measure;
bool b_avaliable = true;

//MACRO de Timer, executando a cada 1 segundo (configurado no Init_Config())
ISR(TIMER1_COMPA_vect)
{
  //O sistema é acionado com base no valor armazenado na variável c_measure, recebido via Bluetooth no loop principal
  switch (c_measure)
  {
    case 's': //caso o usuário tenha escolhido segundos:

      //Verificando a contagem do tempo requerido pelo usuário: 
      if (u16_count == 0) //Se zero,
      {
        PORTB &= ~(1 << PORTB5); //Desliga o motor, acoplado na porta 13 do arduíno
      }

      //Exibe nos Displays de 7 segmentos o tempo restante:
      DisplayDezena(u16_count / 10);
      DisplayUnidade(u16_count % 10);

      if (u16_count > 0) //Se a contagem ainda for maior que zero,
      {
        PORTB |= (1 << PORTB5); //o motor continua sendo acionado,
        u16_count--; //E o tempo decresce de 1 a cada segundo (já que a MACRO executa esse código a cada segundo)
      }
      break;

    case 'm': //caso o usuário tenha escolhido minutos:

      //Verificando a contagem do tempo requerido pelo usuário: 
      if (u16_count == 0) //Se zero,
      {
        PORTB &= ~(1 << PORTB5); //Desliga o motor, acoplado na porta 13 do arduíno
      }

      u16_cMin++; //Incrementa o contador de 1 minuto a cada execução da MACRO
      
      //Exibe quantos minutos ainda faltam:
      DisplayDezena(u16_count / 10);
      DisplayUnidade(u16_count % 10);

      if (u16_count > 0)//Se a contagem ainda for maior que zero,
      {
        PORTB |= (1 << PORTB5); //o motor continua sendo acionado e,
        if (u16_cMin == 60)// se o contador de 1 minuto chega em 60 (após 60 segundos),
        {
          u16_cMin = 0; //esse contador é zerado e, então,
          u16_count--; //o tempo restante é decrementado de 1.
        }
      }
      break;

    default: //caso default para quando a instrução da unidade de medida ainda não foi escolhida.
      DisplayDezena(0);
      DisplayUnidade(0);
      break;
  }
}


void Init_Config()
{
  /* -------- Configurações do Monitor Serial -------- */

  UBRR0H = 0;
  UBRR0L = 103;

  /* -------- Port Set -------- */

  DDRB |= (1 << DDB4) | (1 << DDB3) | (1 << DDB2) | (1 << DDB1); //Display Esquerda
  DDRD |= (1 << DDD7) | (1 << DDD6) | (1 << DDD5) | (1 << DDD4); //Display Direita

  DDRD |= (1 << DDD3) | (1 << DDD2); //LEDs
  DDRB |= (1 << DDB5); //Motor

  /* -------- Configurações do TIMER -------- */

  TIMSK1 &= ~((1 << ICIE1) | (1 << OCIE1B) | (1 << OCIE1A) | (1 << TOIE1));
  TIMSK1 |= (1 << OCIE1A);
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS12) | (1 << CS10); //Prescaler de 1024
  OCR1A = 15624;  //Frequência do Registrador, com base no prescaler escolhido de 1024, para que a MACRO de timer execute a cada 1 segundo

  /* -------- Configurações do BLUETOOTH -------- */

  UCSR0A &= ~(1 << U2X0);  /* _BV(U2X0) = (1 << U2X0) */
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); /* 8-bit data  */
  UCSR0B = (1 << RXEN0) | (1 << TXEN0);   /* Enable Rc_x and Tc_x */
}

char Receive_Data(void) {

  while ( !( UCSR0A & (1 << RXC0)) ) { // Função para receber dados do bluetooth

  }
  return UDR0;
}

void Transmit_Data( char data )
{
  while ( !( UCSR0A & (1 << UDRE0)) );

  UDR0 = data;
}

uint16_t ConvertC(char c_chart) { //Converte uma variável do tipo char para int
  uint16_t u16_num;
  u16_num = c_chart - '0';
  return u16_num;
}

char ConvertI(uint16_t u16_intg) { //Converte umma variável do tipo uint16_t para char
  char c_charct;
  c_charct = u16_intg + '0';
  return c_charct;
}

uint16_t CriaInt(uint16_t u16_n1, uint16_t u16_n2) { //faz a criação de um número de dois dígitos, unindo o inteiro da dezena e da unidade
  uint16_t u16_n3;
  u16_n3 = (u16_n1 * 10) + u16_n2;
  return u16_n3;
}

int main(void)
{
  Init_Config();

  while (1)
  {
    SREG |= (1 << SREG_GLOBAL_INT_ENABLE);

    char c_x;
    uint16_t i = 0;
    uint16_t u16_unid, u16_dezn, u16_number;
    char c_receive_number[2];
    char c_strDez, c_strUn;

    /*
      O Switch é uma máquina de estados que analisa a disponibilidade do controlador, ou seja, se está disponível ou não para receber novas instruções de execução.
    */

    switch (b_avaliable)
    {

      /*
        Caso o sistema esteja disponível, o LED Verde é aceso e o LED Vermelho apagado, e o controlador fica aguardando a primeira instrução do usuário.
      */

      case true:
        PORTD |= (1 << PORTD2); //Acende LED Verde
        PORTD &= ~(1 << PORTD3); //Apaga LED Vermelho
        c_measure = Receive_Data(); //Controlador pronto para receber a primeira instrução (escolha da unidade de tempo de execução: minutos ou segundos)
        b_avaliable = false; //Após recebida, o controlador entra no estado indisponível
        break;

      /*
        No estado indisponível, o LED Vermelho é aceso e o LED Verde apagado, e começa a análise dos dados recebidos.
      */

      case false:
        PORTD |= (1 << PORTD3); //Acende LED Vermelho
        PORTD &= ~(1 << PORTD2); //Apaga LED Verde

        /*
          O While abaixo é responsável por receber os dados do App e colocar tais dados em um vetor de caracteres.
        */

        while (i < 2)
        {
          c_x = Receive_Data(); // nesta parte, a variável c_x recebe o dado do tipo char do App
          c_receive_number[i] = c_x; // o vetor de char recebe, em sua primeira posição esse X
          i++; // incrementa a posição
        }

        /*
          O trecho abaixo trabalha com a conversão de char para int
        */

        c_strDez = c_receive_number[0]; // c_strDez recebe a primeira posição do vetor
        c_strUn = c_receive_number[1]; // c_strUn recebe a segunda posição do vetor

        // ConvertC é uma função criada para Converter char para uint16_t utilizando a tabela ASCII
        u16_unid = ConvertC(c_strUn);

        u16_dezn = ConvertC(c_strDez);

        // Criauint16_t é uma função que, recebe dois inteiros, referentes à dezena e à unidade do mesmo e faz a junção deles em um inteiro só.
        u16_number = CriaInt(u16_dezn, u16_unid);

        //A variável u16_count recebe, então, o número criado a partir da instrução enviada pelo usuário, referente a quanto tempo o sistema deve permanecer em atividade.
        u16_count = u16_number;

        c_interrupt = Receive_Data();
        /*
          Após o envio de uma instrução, o controlador fica aguardando um comando de interrupção,
          mesmo depois de transcorrido todo o tempo de atividade requerido pelo usuário.
          Dessa forma, o sistema só estará pronto para ser utilizado novamente após receber a ordem
          do usuário via bluetooth
        */
        u16_count = 0; //Depois de recebido o comando de interrupção, a variável u16_count é zerada.
        
        b_avaliable = true; //termina a análise e torna o bluetooth disponível novamente
        break;
    }
  }
}


/* As funções abaixo foram criadas para acender os segmentos de cada Display referentes à dezena e à unidade de um número.
  Como os decodificadores utilizados trabalham com números binários, os 'cases' do 'switch' foram construídos para enviar
  corretamente esses números para o decoder, de modo que:
  Para o Display da Unidade:
    Bit:   A3   A2   A1   A0
    Port:  D7   D6   D5   D4
  Para o Display da Dezena:
    Bit:   A3   A2   A1   A0
    Port:  B4   B3   B2   B1
  (Obs: o número 9, por exemplo, em binário, é 1001. Assim: A3 = 1; A2 = 0; A1 = 0; A0 = 1)
*/
void DisplayUnidade(uint16_t u16_cont) {
  switch (u16_cont)
  {
    case 0:
      PORTD &= ~((1 << PORTD4) | (1 << PORTD5) | (1 << PORTD6) | (1 << PORTD7));
      break;
    case 1:
      PORTD &= ~((1 << PORTD5) | (1 << PORTD6) | (1 << PORTD7));
      PORTD |= (1 << PORTD4);
      break;
    case 2:
      PORTD &= ~((1 << PORTD4) | (1 << PORTD6) | (1 << PORTD7));
      PORTD |= (1 << PORTD5);
      break;
    case 3:
      PORTD &= ~((1 << PORTD6) | (1 << PORTD7));
      PORTD |= (1 << PORTD4) | (1 << PORTD5);
      break;
    case 4:
      PORTD &= ~((1 << PORTD4) | (1 << PORTD5) | (1 << PORTD7));
      PORTD |= (1 << PORTD6);
      break;
    case 5:
      PORTD &= ~((1 << PORTD5) | (1 << PORTD7));
      PORTD |= (1 << PORTD4) | (1 << PORTD6);
      break;
    case 6:
      PORTD &= ~((1 << PORTD4) | (1 << PORTD7));
      PORTD |= (1 << PORTD5) | (1 << PORTD6);
      break;
    case 7:
      PORTD &= ~(1 << PORTD7);
      PORTD |= (1 << PORTD4) | (1 << PORTD5) | (1 << PORTD6);
      break;
    case 8:
      PORTD &= ~((1 << PORTD4) | (1 << PORTD5) | (1 << PORTD6));
      PORTD |= (1 << PORTD7);
      break;
    case 9:
      PORTD &= ~((1 << PORTD5) | (1 << PORTD6));
      PORTD |= (1 << PORTD4) | (1 << PORTD7);
      break;
    default:
      break;
  }
}

void DisplayDezena(uint16_t u16_cont) {
  switch (u16_cont)
  {
    case 0:
      PORTB &= ~((1 << PORTB1) | (1 << PORTB2) | (1 << PORTB3) | (1 << PORTB4));
      break;
    case 1:
      PORTB &= ~((1 << PORTB2) | (1 << PORTB3) | (1 << PORTB4));
      PORTB |= (1 << PORTB1);
      break;
    case 2:
      PORTB &= ~((1 << PORTB1) | (1 << PORTB3) | (1 << PORTB4));
      PORTB |= (1 << PORTB2);
      break;
    case 3:
      PORTB &= ~((1 << PORTB3) | (1 << PORTB4));
      PORTB |= (1 << PORTB1) | (1 << PORTB2);
      break;
    case 4:
      PORTB &= ~((1 << PORTB1) | (1 << PORTB2) | (1 << PORTB4));
      PORTB |= (1 << PORTB3);
      break;
    case 5:
      PORTB &= ~((1 << PORTB2) | (1 << PORTB4));
      PORTB |= (1 << PORTB1) | (1 << PORTB3);
      break;
    case 6:
      PORTB &= ~((1 << PORTB1) | (1 << PORTB4));
      PORTB |= (1 << PORTB2) | (1 << PORTB3);
      break;
    case 7:
      PORTB &= ~(1 << PORTB4);
      PORTB |= (1 << PORTB1) | (1 << PORTB2) | (1 << PORTB3);
      break;
    case 8:
      PORTB &= ~((1 << PORTB1) | (1 << PORTB2) | (1 << PORTB3));
      PORTB |= (1 << PORTB4);
      break;
    case 9:
      PORTB &= ~((1 << PORTB2) | (1 << PORTB3));
      PORTB |= (1 << PORTB1) | (1 << PORTB4);
      break;
    default:
      break;
  }
}

