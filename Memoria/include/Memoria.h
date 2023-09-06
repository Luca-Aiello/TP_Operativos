/*
 * Memoria.h
 *
 *  Created on: Sep 5, 2023
 *      Author: utnso
 */

#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <protocolo.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <shared.h>
#include <pthread.h>
#include <protocolo.h>
#include <socket.h>
#include <stdlib.h>




typedef enum{
	FIFO,
	LRU
}t_algoritmo;

//config
char* IP_MEMORIA;
int PUERTO_ESCUCHA;
char* IP_FILESYSTEM;
int PUERTO_FILESYSTEM;
int TAM_MEMORIA;
int TAM_PAGINA;
char* PATH_INSTRUCCIONES;
int RETARDO_RESPUESTA;
char* ALGORITMO_REEMPLAZO;
//¿no debería ser un string para leerlo en el config?

t_log* memoria_logger;
t_log* memoria_log_obligatorio;
t_config* memoria_config;
char* server_name;
int socket_server;
int fd_kernel;
int fd_filesystem;
int fd_cpu;
int fd_memoria;
void* espacio_usuario;




/*----------------TODO INIT ------------------------*/
void leer_config();
void terminar_programa();



/*----------------TODO COMUNICACION SOCKETS --------*/

static void  procesar_conexion(void *void_args);
void inicializar_memoria();
void iterator(char *value);


#endif /* MEMORIA_H_ */