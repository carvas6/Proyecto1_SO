#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define LEEDATOS_SIZE 1025 //tamaño inicial del string RAM

typedef struct {
 short int ETQ;
 short int Datos[8];
} T_LINEA_CACHE;
//La función "leeDatosFichero" se encarga leer un fichero entero y lo almacena en un string
unsigned char* leeDatosFichero(FILE* fichero){
	
	unsigned char* datoSalida = (unsigned char*)malloc(LEEDATOS_SIZE); 
	int tamBuffer=LEEDATOS_SIZE;
	int indiceLectura=0;
	
	while(feof(fichero)==0){ //Mientras no sea final de fichero
		
		datoSalida[indiceLectura]=getc(fichero); //lee de caracter en caracter
		indiceLectura++;
		
		if(indiceLectura == tamBuffer){ // realojamiento de memoria si llega al máximo (+ de 1024 caracteres)
			
			tamBuffer += LEEDATOS_SIZE;
			datoSalida=(unsigned char*)realloc(datoSalida,tamBuffer);
			
		}
		
	}
	
	datoSalida[indiceLectura]='\0';
	return datoSalida;
}

unsigned char* leeDireccion(FILE* fichero){ //lee una linea del fichero y la almacena en un string
	
	unsigned char* datoSalida = (unsigned char*)malloc(5);
	int tamBuffer=5;
	int indiceLectura=0;
	unsigned char auxiliar=' ';
	
	do{
		auxiliar = getc(fichero);
		
		if(auxiliar != '\n' && feof(fichero)==0){
			datoSalida[indiceLectura]=auxiliar;
			indiceLectura++;
			
			if(indiceLectura == tamBuffer){
			
			tamBuffer++;
			datoSalida=(unsigned char*)realloc(datoSalida,tamBuffer);
			
			}
		}
	}while(auxiliar != '\n' && feof(fichero)==0);
	
	datoSalida[indiceLectura]='\0';
	return datoSalida;
}

int comprobar_etiqueta(int direccion,T_LINEA_CACHE* elementos,int contador,char* RAM,char* texto,int tiempo){ 
	int linea_actual = 0x00;
	int etiqueta_actual = 0x00;
	int palabra_actual = 0x00;
	int bloque_actual = 0x0000;
	int posicion = 0;
	
	linea_actual = (direccion & 0x0018)/pow(2,3); //aplicamos mascara de bits para quedarnos con los bits 3 y 4
	etiqueta_actual = (direccion & 0x03E0)/pow(2,5); //aplicamos mascara de bits para quedarnos con los bits del 5 al 9
	palabra_actual = direccion & 0x0007; //aplicamos mascara de bits para quedarnos con los bits 0, 1 y 2
	bloque_actual = (direccion & 0x03F8)/pow(2,3); //aplicamos mascara de bits para quedarnos con los bits del 3 al 9
	posicion = bloque_actual*8; //para localizar la primera posicion del bloque actual en el array RAM
	
	if(elementos[linea_actual].ETQ != etiqueta_actual){ //Si la etiqueta no coincide = fallo de caché
		printf("T: %d, Fallo de CACHE %d, ADDR %04X ETQ %X linea %02X palabra %02X bloque %02X\n",tiempo,contador,direccion,etiqueta_actual,linea_actual,palabra_actual,bloque_actual);
		
		elementos[linea_actual].ETQ = etiqueta_actual;
		
		for(int i=0;i<=7;++i){ //guarda los nuevos datos
			elementos[linea_actual].Datos[i] = RAM[posicion];
			++posicion;
		}
		printf("Cargando el bloque %02X en la linea %02X\n",bloque_actual,linea_actual);
		
		return 0;
	}
	else{ //si coincide = acierto de caché
		int dato = 0x00;
		dato = posicion + palabra_actual;
		printf("T: %d, Acierto de CACHE, ADDR %04X ETQ %X linea %02X palabra %02X DATO %02X\n",tiempo,direccion,etiqueta_actual,linea_actual,palabra_actual,dato);
		texto[contador] = RAM[dato];
		
		for(int i=0;i<4;++i){ // imprime el estado de la caché
			printf("ETQ:%02X\tDatos",elementos[i].ETQ);
			for(int j=7;j>=0;--j){
				printf(" %02X",elementos[i].Datos[j]);
			}
			printf("\n");
		}
		sleep(2);
		
		return 1;
	}
}

int main(int argc,char* argv[]){ 
	
	T_LINEA_CACHE* elementos = (T_LINEA_CACHE*)malloc(sizeof(T_LINEA_CACHE)*4);
							
	int tiempoglobal = -1;
	int numfallos = 0;
	int numero_accesos=0;
	int direccion_actual = 0x0000; 
	int contador = -1;
	
	int error = 1;
	
	char* texto = (char*)malloc(100);
	
	for(int i=0;i<=3;++i){ //inicializa las variables
		elementos[i].ETQ = 0xFF;
		
		for(int j=0;j<=7;++j){
			elementos[i].Datos[j] = 0;
		}
	}
	
	unsigned char RAM[1024];
	FILE* binario = fopen("RAM.bin","r+"); //abre el archivo en modo lectura (+, permite escribir al final aun que luego no se usa)
	unsigned char* RAM_provisional = (unsigned char*)malloc(LEEDATOS_SIZE);
	RAM_provisional = leeDatosFichero(binario); //se copia el fichero entero en un string auxiliar
	
	for(int i=0;i<1024;++i){ //guarda el string en el array final
		RAM[i] = RAM_provisional[i];
	}
	free(RAM_provisional); //liberamos el string auxiliar
	
	FILE* direcciones = fopen("accesos_memoria.txt","r+");
	
	while(feof(direcciones)==0 || error==0){ //mientras no sea final de fichero Ó el último acceso haya dado fallo de caché
		
		if(error==1){ //si el último acceso ha dado acierto y antes del primer acceso es obligado
			++tiempoglobal;
			++contador;
			direccion_actual = strtol(leeDireccion(direcciones),NULL,16); //lee la siguiente linea del fichero y la convierte en hexadecimal
		}
		else{		
			tiempoglobal = tiempoglobal + 10;
			++numfallos;
			}
		
		error = comprobar_etiqueta(direccion_actual,elementos,contador,RAM,texto,tiempoglobal); //comprobamos si la etiqueta actual da acierto o fallo
		++numero_accesos;
	}
	float tiempo_medio = 0.00;
	int numero_aciertos =0;
	
	numero_aciertos = numero_accesos - numfallos;
	tiempo_medio = (numero_aciertos + (numfallos*10)) / numero_accesos; 
	printf("\nNumero total de accesos = %d\nNumero total de fallos = %d\nTiempo medio de acceso = %f\n",numero_accesos,numfallos,tiempo_medio);
	
	texto[numero_aciertos]='\0'; //determina donde termina el string y donde empieza la basura
	printf("\n%s\n",texto);
	
	return 0;
}
