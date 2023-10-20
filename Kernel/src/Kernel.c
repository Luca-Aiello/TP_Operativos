#include "../include/Kernel.h"

int main(int argc, char** argv) {
	kernel_logger = log_create("kernel.log", "[Kernel]", 1, LOG_LEVEL_INFO);
	kernel_log_obligatorio = log_create("kernel_log_obligatorio.log", "[Kernel - Log obligatorio]", 1, LOG_LEVEL_INFO);

	kernel_config = config_create(argv[1]); //Esto quiza lo descomentemos para las pruebas
	//kernel_config = config_create("kernel.config");

	if(kernel_config == NULL){
		log_error(kernel_logger, "No se encontro el path del config\n");
		config_destroy(kernel_config);
		log_destroy(kernel_logger);
		log_destroy(kernel_log_obligatorio);
		exit(2);
	}

	leer_config(kernel_config);

	pthread_t hilo_cpu_dispatch, hilo_cpu_interrupt;
	pthread_t hilo_memoria;
	pthread_t hilo_consola;
	pthread_t hilo_filesystem;
	//pthread_t hilo_experimentos_xd;

	// ---------------- LE DAMOS CORRIENTE A LOS SEMAFOROS ----------------
	iniciar_semaforos();
	iniciar_pthread();
	iniciar_listas();

	//Probando conexiones
	fd_cpu_dispatcher = crear_conexion(IP_CPU, PUERTO_CPU_DISPATCH);
	fd_cpu_interrupt = crear_conexion(IP_CPU, PUERTO_CPU_INTERRUPT);
	fd_filesystem = crear_conexion(IP_FILESYSTEM, PUERTO_FILESYSTEM);
	fd_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);


	pthread_create(&hilo_filesystem, NULL, (void*)atender_filesystem, NULL);
	pthread_detach(hilo_filesystem);

	pthread_create(&hilo_cpu_dispatch, NULL, (void*)atender_cpu_dispatch, NULL);
	pthread_detach(hilo_cpu_dispatch);
	pthread_create(&hilo_cpu_interrupt, NULL, (void*)atender_cpu_interrupt, NULL);
	pthread_detach(hilo_cpu_interrupt);

	pthread_create(&hilo_memoria, NULL, (void*)atender_memoria, NULL);
	pthread_detach(hilo_memoria);

//	pthread_create(&hilo_experimentos_xd, NULL, (void*)atender_experimentos_xd, NULL);
//	pthread_detach(hilo_experimentos_xd);

	pthread_create(&hilo_consola, NULL, (void*)leer_consola, NULL);
	pthread_join(hilo_consola, NULL);


	finalizar_kernel();

	return EXIT_SUCCESS;
}


void leer_config(t_config* config){
	IP_MEMORIA = config_get_string_value(config,"IP_MEMORIA");
	PUERTO_MEMORIA = config_get_string_value(config,"PUERTO_MEMORIA");
	IP_FILESYSTEM = config_get_string_value(config,"IP_FILESYSTEM");
	PUERTO_FILESYSTEM = config_get_string_value(config,"PUERTO_FILESYSTEM");
	IP_CPU = config_get_string_value(config,"IP_CPU");
	PUERTO_CPU_DISPATCH = config_get_string_value(config,"PUERTO_CPU_DISPATCH");
	PUERTO_CPU_INTERRUPT = config_get_string_value(config,"PUERTO_CPU_INTERRUPT");
	char* algoritmo_planificacion = config_get_string_value(config,"ALGORITMO_PLANIFICACION");
	asignar_planificador_cp(algoritmo_planificacion);
	QUANTUM = config_get_int_value(config,"QUANTUM");
	RECURSOS = config_get_array_value(config, "RECURSOS");
	INSTANCIAS_RECURSOS = config_get_array_value(config, "INSTANCIAS_RECURSOS"); //TODO: tratar de convertirlo en un array de ints
	GRADO_MULTIPROGRAMACION_INI = config_get_int_value(config,"GRADO_MULTIPROGRAMACION_INI");

}


void leer_consola(){
	char* leido;
	leido = readline("> ");
	char** ingreso = string_split(leido, " ");
	t_pcb* pcb;

	// Despues hay que actualizar la consola para que no tire error
	while(strcmp(leido,"\0") != 0){
		if(string_equals_ignore_case(ingreso[0], "INICIAR_PROCESO")){
			log_info(kernel_logger, "Ingreso en: INICIAR_PROCESO");

			pcb = iniciar_pcb(atoi(ingreso[3]));
			log_info(kernel_log_obligatorio, "Se crea el proceso %d en %s", pcb -> pid, estado_to_string(pcb -> estado));

			agregar_pcb_lista(pcb, list_new, mutex_list_new);

			if(!list_is_empty(list_new)){
				inicializar_estructura(fd_memoria, ingreso[1], atoi(ingreso[2]), pcb);
				//inicializar_ejecucion();
			}
		}else if(string_equals_ignore_case(ingreso[0], "FINALIZAR_PROCESO")){
			//
		}else if(string_equals_ignore_case(ingreso[0], "DETENER_PLANIFICACION")){
			//
		}else if(string_equals_ignore_case(ingreso[0], "INICIAR_PLANIFICACION")){
			//
		}else if(string_equals_ignore_case(ingreso[0], "MULTIPROGRAMACION")){
			//
		}else if(string_equals_ignore_case(ingreso[0], "PROCESO_ESTADO")){
			//
		}else if(string_equals_ignore_case(ingreso[0], "SALIR")){
			free(leido);
			free(ingreso);
			break;
		}
		free(leido);
		free(ingreso);
		leido = readline("> ");
		ingreso = string_split(leido, " ");
	}

	free(leido);
}

void finalizar_kernel(){
	log_destroy(kernel_logger);
	log_destroy(kernel_log_obligatorio);
	config_destroy(kernel_config);
	liberar_conexion(fd_cpu_dispatcher);
	liberar_conexion(fd_cpu_interrupt);
	liberar_conexion(fd_filesystem);
	liberar_conexion(fd_memoria);
	free(INSTANCIAS_RECURSOS);
	free(RECURSOS);
}

void asignar_planificador_cp(char* algoritmo_planificacion){
	if (strcmp(algoritmo_planificacion, "FIFO") == 0) {
			ALGORITMO_PLANIFICACION = FIFO;
		} else if (strcmp(algoritmo_planificacion, "RR") == 0) {
			ALGORITMO_PLANIFICACION = ROUNDROBIN;
		} else if (strcmp(algoritmo_planificacion, "PRIORIDADES") == 0) {
			ALGORITMO_PLANIFICACION = PRIORIDADES;
		} else {
			log_error(kernel_logger, "No se encontro el algoritmo de planificacion de corto plazo");
		}
}

void atender_memoria(){
	gestionar_handshake_como_cliente(fd_memoria, "MEMORIA", kernel_logger);
	identificarme_con_memoria(fd_memoria, KERNEL);
	log_info(kernel_logger, "HANDSHAKE CON MEMORIA [EXITOSO]");

	//int control_key = 1;
	while(1){
		int cod_op = recibir_operacion(fd_memoria);
		t_buffer* unBuffer;
		log_info(kernel_logger, "Se recibio algo de MEMORIA");

		switch (cod_op){
		case ESTRUCTURA_INICIADA_KM_OK:
			unBuffer = recibiendo_super_paquete(fd_memoria);
//			char* mensaje = recibir_string_del_buffer(unBuffer);
//			log_info(kernel_logger, mensaje);
//			free(mensaje);
//			sem_post(&sem_iniciar_estructuras_memoria);
			break;
		case LIBERAR_ESTRUCTURA_KM:
			unBuffer = recibiendo_super_paquete(fd_memoria);
			//
			break;
		case PRUEBAS:
			unBuffer = recibiendo_super_paquete(fd_memoria);
			//atender_esta_prueba(myBuffer);
			break;
		case -1:
			log_error(kernel_logger, "[DESCONEXION]: MEMORIA");
			//control_key = 0;
			exit(EXIT_FAILURE);
			break;
		default:
			log_warning(kernel_logger, "Operacion desconocida");
			free(unBuffer);
			break;
		}
	}

}

void atender_filesystem(){
	gestionar_handshake_como_cliente(fd_filesystem, "FILESYSTEM", kernel_logger);
	log_info(kernel_logger, "HANDSHAKE CON FILESYSTEM [EXITOSO]");

	//int control_key = 1;
	while(1){
		int cod_op = recibir_operacion(fd_filesystem);
		t_buffer* unBuffer;
		log_info(kernel_logger, "Se recibio algo de FILESYSTEM");

		switch (cod_op) {
		case SYSCALL_KF:
			unBuffer = recibiendo_super_paquete(fd_filesystem);
			//
			break;
		case PRUEBAS:
			unBuffer = recibiendo_super_paquete(fd_filesystem);
			//
			break;
		case -1:
			log_error(kernel_logger, "[DESCONEXION]: FILESYSTEM");
			//control_key = 0;
			exit(EXIT_FAILURE);
			break;
		default:
			log_warning(kernel_logger, "Operacion desconocida");
			free(unBuffer);
			break;
		}
	}
}

void atender_cpu_dispatch(){
	gestionar_handshake_como_cliente(fd_cpu_dispatcher, "CPU_Dispatch", kernel_logger);
	log_info(kernel_logger, "HANDSHAKE CON CPU_Dispatch [EXITOSO]");
	while(1){
		int cod_op = recibir_operacion(fd_cpu_dispatcher);
		t_buffer* unBuffer;
		log_info(kernel_logger, "Se recibio algo de FILESYSTEM");

		switch (cod_op) {
		case FINALIZAR_PROCESO_CPK: // Este caso seria para el PLANIFICADOR LARGO PLAZO
			unBuffer = recibiendo_super_paquete(fd_cpu_dispatcher);
//			t_pcb* pcb = recv_pcb(unBuffer);
//			transferir_from_actual_to_siguiente();
			break;
		case ATENDER_INSTRUCCION_CPK:
			unBuffer = recibiendo_super_paquete(fd_cpu_dispatcher);
			char* instruccion_CPU = recibir_string_del_buffer(unBuffer); // Se supone que puede ser SLEEP, WAIT, SIGNAL
			if(strcmp(instruccion_CPU, "SLEEP")){

			}else if(strcmp(instruccion_CPU, "WAIT")){

			}else if(strcmp(instruccion_CPU, "SIGNAL")){

			}else if(strcmp(instruccion_CPU, "MOV_IN")){

			}else if(strcmp(instruccion_CPU, "MOV_OUT")){

			}else if(strcmp(instruccion_CPU, "F_OPEN")){

			}else if(strcmp(instruccion_CPU, "F_CLOSE")){

			}else if(strcmp(instruccion_CPU, "F_SEEK")){

			}else if(strcmp(instruccion_CPU, "F_READ")){

			}else if(strcmp(instruccion_CPU, "F_WRITE")){

			}else if(strcmp(instruccion_CPU, "F_TRUNCATE")){

			}else if(strcmp(instruccion_CPU, "EXIT")){

			}
//			atender_motivo_block(pcb);
			break;
		case -1:
			log_error(kernel_logger, "[DESCONEXION]: FILESYSTEM");
			//control_key = 0;
			exit(EXIT_FAILURE);
			break;
		default:
			log_warning(kernel_logger, "Operacion desconocida");
			free(unBuffer);
			break;
		}
	}
}

//void atender_motivo_block(t_pcb* pcb){
////	transferir_from_actual_to_siguiente(lis)
//	proximo_a_ejecutar();
//}
//
//void atender_blockeados(){
//
//}

void atender_cpu_interrupt(){
	gestionar_handshake_como_cliente(fd_cpu_interrupt, "CPU_Interrupt", kernel_logger);
	log_info(kernel_logger, "HANDSHAKE CON CPU_Interrupt [EXITOSO]");
	while(1){
		int cod_op = recibir_operacion(fd_cpu_interrupt);
		t_buffer* unBuffer;
		log_info(kernel_logger, "Se recibio algo de FILESYSTEM");

		switch (cod_op) {
		case FORZAR_DESALOJO_KC:
			unBuffer = recibiendo_super_paquete(fd_cpu_interrupt);
			//
			break;
		case PRUEBAS:
			unBuffer = recibiendo_super_paquete(fd_cpu_interrupt);
			//
			break;
		case -1:
			log_error(kernel_logger, "[DESCONEXION]: FILESYSTEM");
			//control_key = 0;
			exit(EXIT_FAILURE);
			break;
		default:
			log_warning(kernel_logger, "Operacion desconocida");
			free(unBuffer);
			break;
		}
	}
}

void iniciar_semaforos(){
	sem_init(&sem_init_pcb, 0, 1);
	sem_init(&sem_grado_multiprogramacion, 0, GRADO_MULTIPROGRAMACION_INI);
	sem_init(&sem_list_ready, 0, 1);
	sem_init(&sem_iniciar_estructuras_memoria, 0, 0);
}

void iniciar_pthread(){
	pthread_mutex_init(&mutex_list_new, NULL);
	pthread_mutex_init(&mutex_list_ready, NULL);
	pthread_mutex_init(&mutex_list_exec, NULL);
}

void iniciar_listas(){
	list_new = list_create();
	list_ready = list_create();
	list_execute = list_create();
	list_blocked = list_create();
}

// ------ Inicializar proceso ------

t_pcb* iniciar_pcb(int prioridad){
	sem_wait(&sem_init_pcb);

	t_pcb* new_pcb = crear_pcb(process_id, prioridad);
	process_id ++;

	sem_post(&sem_init_pcb);

	return new_pcb;
}

void transferir_from_new_to_ready(){
//	sem_wait(&sem_grado_multiprogramacion);   // Queda comentado hasta que agreguemos el sem_post, porque todavia no esta hecho el finalizar proceso
	t_pcb* pcb;

	pcb = remover_proceso_lista(list_new, mutex_list_new);

	char* estado_anterior = "NEW";

	cambiar_estado_pcb(pcb, READY);

	agregar_pcb_lista(pcb, list_ready, mutex_list_ready);

	log_info(kernel_log_obligatorio, " PID: %d - Estado Anterior: %s - Estado Actual: %s", pcb -> pid, estado_anterior, estado_to_string(pcb -> estado));

//	pcb_destroy(pcb);
//	free(estado_anterior);
	// No se si son necesarios
}

void inicializar_estructura(int fd_memoria, char* path, int size, t_pcb* pcb){
	transferir_from_new_to_ready();

	char* pids_en_ready = lista_pids_en_Ready();
	log_info(kernel_log_obligatorio, "Cola Ready %s: %s",estado_to_string(pcb -> estado), pids_en_ready);

	send_enviar_path_memoria(fd_memoria, path, size, pcb -> pid);
	free(pids_en_ready);
//	sem_wait(&sem_iniciar_estructuras_memoria);
}

// ------ PCB ------
void agregar_pcb_lista(t_pcb* pcb, t_list* list_estado, pthread_mutex_t mutex_list){
	pthread_mutex_lock(&mutex_list);
	list_add(list_estado, pcb);
	pthread_mutex_unlock(&mutex_list);
}

t_pcb* remover_proceso_lista(t_list* list_estado, pthread_mutex_t mutex){
	t_pcb* pcb;
	pthread_mutex_lock(&mutex);
	pcb = list_remove(list_estado, pcb);
	pthread_mutex_unlock(&mutex);
	return pcb;
}

t_pcb* recv_pcb(t_buffer* paquete_pcb){
	t_pcb* pcb = malloc(sizeof(t_pcb));

	pcb -> pid = recibir_int_del_buffer(paquete_pcb);
	pcb -> program_counter = recibir_int_del_buffer(paquete_pcb);
	pcb -> prioridad = recibir_int_del_buffer(paquete_pcb);

	return pcb;
}

//-----------Ejecutar procesos-------------
//void proximo_a_ejecucion(){
//
//	while(1){
//		sem_wait(&sem_list_ready);
//		sem_wait(&sem_cpu_free_exec);  // Creo que deberia ir un semaforo de si el CPU esta libre para ejecutar?
//
//		t_pcb* pcb = elegir_proceso_segun_algoritmo();
//		enviar_contexto_pcb(pcb);
//	}
//}

void transferir_from_actual_to_siguiente(t_list* list_actual, pthread_mutex_t mutex_actual, t_list* list_siguiente, pthread_mutex_t mutex_siguiente, est_pcb estado_siguiente){
	t_pcb* pcb;

	pcb = remover_proceso_lista(list_actual, mutex_actual);

	char* estado_anterior = estado_to_string(pcb -> estado);

	cambiar_estado_pcb(pcb, estado_siguiente);

	agregar_pcb_lista(pcb,  list_siguiente, mutex_siguiente);

	log_info(kernel_log_obligatorio, " PID: %d - Estado Anterior: %s - Estado Actual: %s", pcb -> pid, estado_anterior, estado_to_string(pcb -> estado));

}

//t_pcb* elegir_proceso_segun_algoritmo(){
//	switch(ALGORITMO_PLANIFICACION){
//		case FIFO:
//			return 1;
//		case ROUNDROBIN:
//			// return remover_proceso_lista(list_ready, mutex_list_ready);
//		case PRIORIDADES:
//			return obtener_proceso_segun_prioridad();
//		default:
//			log_error(kernel_logger, "No se reconocio el algoritmo de planificacion");
//			exit(1);
//	}
//}
/*
t_pcb* obtener_proceso_segun_prioridad(){
	pthread_mutex_lock(&mutex_list_ready);
	list_sort(list_ready, maxima_prioridad);
	t_pcb* pcb = list_remove(list_ready, 0);

	log_info(kernel_logger, "Se eligio el proceso %d por Prioridades", pcb -> pid);
	pthread_mutex_unlock(&mutex_list_ready);

	return pcb;
}

bool maxima_prioridad(t_pcb* pcb1, t_pcb* pcb2){
	return pcb1->prioridad <= pcb2->prioridad;
}

void ejecutar(t_pcb* pcb){
	char* estado_anterior = estado_to_string(pcb -> estado);
	cambiar_estado_pcb(pcb, EXEC);

	log_info(kernel_log_obligatorio, " PID: %d - Estado Anterior: %s - Estado Actual: %s", pcb -> pid, estado_anterior, estado_to_string(pcb -> estado));

	log_info(kernel_logger, "El proceso %d se pone en ejecucion", pcb -> pid);
	//TODO: asignar un tiempo de ejecucion aca, investigar el temporal de las commons?
	agregar_pcb_lista(pcb, list_execute, mutex_list_exec);
//	enviar_contexto_de_ejecucion(pcb, fd_cpu_dispatcher);
}
*/

char* estado_to_string(est_pcb estado){
	switch(estado){
	case NEW:
		return "NEW";
	case READY:
		return "READY";
	case EXEC:
		return "EXEC";
	case BLOCKED:
		return "BLOCK";
	case EXIT:
		return "EXIT";
	default:
		return "ERROR";
	}
}

char* lista_pids_en_Ready(){
	int id_process;
	char* pids_in_string = string_new();
	string_append(&pids_in_string, "[");

	for(int i = 0; i < list_size(list_ready); i++){
		if(i == 0){
			t_pcb* pcb = list_get(list_ready, i);
			id_process = pcb -> pid;
			string_append(&pids_in_string, string_itoa(id_process));
		}else{
			string_append(&pids_in_string, ", ");
			t_pcb* pcb = list_get(list_ready, i);
			id_process = pcb -> pid;
			string_append(&pids_in_string, string_itoa(id_process));
		}

	}
	string_append(&pids_in_string, "]");

	return pids_in_string;
}
