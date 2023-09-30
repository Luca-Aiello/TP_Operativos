#ifndef INCLUDE_PROTOCOLO_H_
#define INCLUDE_PROTOCOLO_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>
#include"shared.h"
/*
typedef enum {
	INT,
	STRING,
	VOID
}op_tipo_dato;
*/

typedef enum{
	MEMORIA,
	FILESYSTEM,
	CPU,
	KERNEL
}modulo_code;

typedef enum{
	MENSAJE,
	PAQUETE,
	INT,
	STRING,
	VOID,
	SUPER_PAQUETE,
	ADMINISTRAR_PAGINA_MEMORIA,
	ACCESO_USUARIO_MEMORIA,
	CREAR_PAGINA,
	ELIMINAR_PAGINA,
	LECTURA_USUARIO,
	ESCRITURA_USUARIO,
	REEMPLAZO_PAGINA,
	HANDSHAKE,
	//-----
	INICIAR_ESTRUCTURA_KM,
	LIBERAR_ESTRUCTURA_KM,
	//-------
	EJECUTAR_PROCESO_KC,
	FORZAR_DESALOJO_KC,
	//--------
	SYSCALL_KF,
	//--------
	PETICION_ASIGNACION_BLOQUE_SWAP_FM,
	LIBERAR_PAGINAS_FM,
	PETICION_PAGE_FAULT_FM,
	CARGAR_INFO_DE_LECTURA_FM,
	GUARDAR_INFO_FM,
	//------
	PETICION_INFO_RELEVANTE_CM,
	PETICION_DE_INSTRUCCIONES_CM,
	PETICION_DE_EJECUCION_CM,
	CONSULTA_DE_PAGINA_CM,
	//-------
	MENSAJES_POR_CONSOLA,
	PRUEBAS,

}op_code;

//typedef enum{
//	MEMORIA,
//	FILE_SYSTEM,
//	CPU,
//	KERNEL
//}t_module;

typedef enum{
	_INT,
	_STRING,
	_VOID
}t_primitivo;

typedef struct{
	int size;
	void* stream;
} t_buffer;

typedef struct{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

/**/


/**/


/******************TODO: revisar los MENSAJES*************/
void enviar_mensaje(char* mensaje, int socket_cliente);
int recibir_operacion(int socket_cliente);
void* recibir_buffer(int* size, int socket_cliente);
void recibir_mensaje(t_log* logger, int socket_cliente);
void crear_buffer(t_paquete* paquete);

void atender_handshake_respuesta(t_buffer* myBuffer, t_log* logger);

/******************TODO: revisar los PAQUETES*************/
t_list* recibir_paquete(int);
t_paquete* crear_paquete(void);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void* serializar_paquete(t_paquete* paquete, int bytes);
void eliminar_paquete(t_paquete* paquete);

/******************TODO: revisar los EMPAQUETACION*************/


/******************TODO: revisar los SENDS*************/


/******************TODO: revisar los RECVS*************/


/******************TODO: revisar los DESTROY*************/


int* recibir_int(t_log* logger, void* coso);
t_list* recibir_paquete_int(int socket_cliente);

t_paquete* crear_super_paquete(op_code code_op);
void cargar_int_al_super_paquete(t_paquete* paquete, int numero);
void cargar_string_al_super_paquete(t_paquete* paquete, char* string);
void cargar_choclo_al_super_paquete(t_paquete* paquete, void* choclo, int size);
int recibir_int_del_buffer(t_buffer* coso);
char* recibir_string_del_buffer(t_buffer* coso);
void* recibir_choclo_del_buffer(t_buffer* coso);

t_buffer* recibiendo_super_paquete(int conexion);
void enviar_handshake(int conexion, modulo_code modulo);
int recibir_handshake(int conexion);

#endif /* INCLUDE_PROTOCOLO_H_ */
