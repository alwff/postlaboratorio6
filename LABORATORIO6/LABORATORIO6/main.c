/* Universidad del Valle de Guatemala
IE2023:: Programación de Microcontroladores
LABORATORIO6.c
Autor: Alejandra Cardona
Hardware: ATMEGA328P
Creado: 22/04/2024
Última modificación: 29/04/2024

****************************************************************** 

LIBRERÍAS

******************************************************************
*/

#define F_CPU 16000000UL //Frecuencia en la que opera el sistema - 16 MHz ---- UL = unsigned long
#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include <avr/interrupt.h>

/*
******************************************************************

FUNCIONES

******************************************************************
*/

void initUART09600(void); //Configuracion UART
void writeTextUART(char* text); //Puntero
unsigned char readUART(); //Lee lo recibido

int intforport(char bufferRX); //Convertir de char a int
void writeMenu(void); //Muestra el menu
void initADC(void); //ADC

/*
******************************************************************

VARIABLES Y DEFINICIONES

******************************************************************
*/

volatile char bufferRX; //Es volatil porque va a estar dentro de la interrupcion, pues puede cambiar en todo momento
volatile char bufferRX_copia;

#define estadoADC 1
#define estadoASCII 2

int estado = 0; //Mostrar o no ADC
int enviar = 0; //Enviar o no caracteres 

//******************************************************************

int main(void){
	cli(); //Deshabilita interrupciones
	
	//Como salidas
	DDRB |= 0xFF;
	DDRD |= (1<<PORTD6)|(1<<PORTD7);
	
	initUART09600();
	initADC(); //Configura ADC
	
	sei(); //Habilita interrupciones
	
	writeMenu();
	
	while (1)
	{
		if (estado == 1){
		ADCSRA |= (1<<ADSC); //Inicia la lectura del ADC
		
		PORTB = ADCH; //Muestra en el puerto el valor del ADC
		PORTD = (0xC0 & ADCH);
		}
	}
}

/*
******************************************************************

CONFIGURACION DEL UART Y SUS FUNCIONES DE ESCRITURA Y LECTURA

******************************************************************
*/

void initUART09600(void){
	//Configurando los pines RX y TX
	DDRD &= ~(1<<DDD0); //D0 RX como entrada
	DDRD |= (1<<DDD1); //D1 TX como salida
	
	//Se define el modo de trabajo
	UCSR0A = 0;
	UCSR0A |= (1<<U2X0); // Usando el registro de control A. Se configura en Modo Fast U2X0 = 1
	
	//Se configura el registro de control B
	UCSR0B = 0;
	UCSR0B |= (1<<RXCIE0); // Se habilita la interrupcion ISR RX
	UCSR0B |= (1<<RXEN0); // Se habilita RX
	UCSR0B |= (1<<TXEN0); // Se habilita TX
	
	//Se configura el registro de control C
	//Se define el frame
	UCSR0C = 0;
	UCSR0C = (1<<UCSZ01)| (1<<UCSZ00); // Define el numero de bits de data en el frame del receiver y el transmitter
	
	//Calculos
	//UBRRn=(fosc/mhz*BAUD)-1
	//BAUD=(fosc/mhz*(UBRRn+1))
	//Error%=((BAUDcalculado/BAUD)-1)*100%
	//Usando fosc de 16 MHZ 
	//UBRRn = 207
	//BAUDRATE = 9600 Velocidad de comunicacion serial

	UBRR0 = 207;
}

//Funcion para escribir lineas completas
void writeTextUART(char* text){
	uint8_t i;
	for (i=0; text[i]!='\0'; i++){ //Inicia en 0 y para cuando sea nulo \0 -- Caracter Evaluar nueva linea POSTLAB
		while(!(UCSR0A&(1<<UDRE0))); 
		UDR0 = text[i]; //Barre todo el arreglo hasta acabar
	}
	
}

//Convierte lo recibido por el buffer en un entero para moder mostrarlo en el puerto
int intforport(char bufferRX_copia){
	return bufferRX_copia - '0'; 
}


/*
******************************************************************

INTERRUPCIONES

******************************************************************
*/
 
//USART_RX
ISR(USART_RX_vect){
	bufferRX = UDR0;
	while(!(UCSR0A&(1<<UDRE0))); //Si esto esta vacio entonces se manda
	//UDR0 = bufferRX; //Mete a UDR0 lo que esta en el buffer para enviarlo
	//PORTB=intforport(bufferRX); //Lo recibido lo muestra en el portB
	bufferRX_copia = bufferRX;
	
	unsigned int convertido = intforport(bufferRX_copia);
	
	if (enviar==0){
		switch(convertido){
			
			case estadoADC: //1
			estado = 1; //Habilita mostrar ADC
			enviar = 0; //No enviar caracteres
			writeTextUART("\n");
			writeTextUART("\n");
			writeTextUART("\n");
			writeTextUART("Valor de pot mostrado en barra de LEDs");
			writeTextUART("\n");
			writeTextUART("\n");
			writeTextUART("\n");
		
			writeMenu(); //Muestra el menu
			break;
			
			
			case estadoASCII: //2
			estado = 0; //Deshabilita mostrar ADC
			enviar = 1; //Enviar caracteres
			writeTextUART("\n ¿Qué carácter desea mostrar? \n");
			break;
			
			default:
			estado = 0; //Deshabilita mostrar ADC
			enviar = 0; //No enviar caracteres
			writeTextUART('\n');
			writeTextUART('Error');
			writeTextUART('\n');
			writeMenu();
			break;
		}
	}
	
	else{
		enviar = 0; //No enviar caracteres
		PORTB=bufferRX; //Lo recibido lo muestra en el portB
		PORTD = (0xC0 & bufferRX); //Lo recibido lo muestra en el portD
		writeMenu(); //Al terminar, muestra el menu
	}
		
}

//ADC
ISR(ADC_vect){
	ADCSRA |= (1<<ADIF);
}

/*
******************************************************************

MENU Y SUS OPCIONES 

******************************************************************
*/

//Escribe el menu
void writeMenu(void){
	writeTextUART("\n Bienvenido! \n");
	writeTextUART("\n 1. Leer Potenciometro \n");
	writeTextUART("\n 2. Enviar ASCII \n");
	writeTextUART("\n Ingrese la opción deseada (1 o 2) \n");
}

//Se configura el canal de ADC para hacer la lectura del potenciometro en A0
void initADC(void){
	// Configurando bits de ADC

	// Seleccionando una referencia (Vref)=AVCC=5V
	ADMUX = 0; // Configurando el canal 0
	ADMUX |= (1 << REFS0);
	ADMUX &= ~(1 << REFS1);

	// ADLAR - Justificación hacia la izquierda
	ADMUX |= (1 << ADLAR);

	ADCSRA = 0;
	// ADEN - Habilita o enciende ADC
	ADCSRA |= (1 << ADEN);

	// ADIE - Habilita ISR ADC
	ADCSRA |= (1 << ADIE);

	// Ya que nos encontramos en el rango permitido, es posible usar alta resolución
	// Prescaler 128 > 16MHz / 128 = 125KHz
	ADCSRA |= (1 << ADPS2)|(1 << ADPS1)|(1 << ADPS0);
	
}

