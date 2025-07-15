#ifndef ENUM_H
#define ENUM_H

typedef enum {
    SOYKERNEL = 0,
    SOYCPU = 1,
    SOYMEMORIA = 2,
    SOYIO = 3
} ID_MODULO;


typedef enum { //CODIGO DE OPERACION / PAQUETE
    HANDSHAKE = 1, // Parametros: ID o Nombre (CPU/IO)
    SYSCALL_EXIT = 2, // No necesita parametros
    SYSCALL_INIT_PROC = 3, // 2 Parametros: Nombre_Archivo_Pseudocodigo, Tamaño_Proceso
    SYSCALL_DUMP_MEMORY = 4, // No necesita parametros
    SYSCALL_IO = 5, // 2 Parametros: Nombre_IO, Tiempo_Milisegundos
    PETICION_IO = 6,
    PETICION_EXECUTE = 7,
    RESPUESTA_PETICION = 8,
    ASIGNACION_PROCESO_CPU = 9, // 2 Parametros: PID, PC
    SOLICITUD_MEMORIA_NUEVO_PROCESO = 10, // 3 Parametros: PID, PATH, TAMAÑO
    SOLICITUD_MEMORIA_CARGA_SWAP = 11,    // 1 Parametro: PID
    RESPUESTA_MEMORIA_PROCESO_CARGADO = 12, // No necesita parametros 
    RESPUESTA_MEMORIA_NO_HAY_MEMORIA_SUFICIENTE = 13, // No necesita parametros
    INTERRUPT_ACKNOWLEDGE = 14, // No necesita parametros. Lo envia al socket dispatch el CPU cuando se recibe un interrupt 
    PROCESO_FINALIZADO_LIBERAR_MEMORIA = 15, // 1 Parametro: PID del proceso que termino
    PROCESO_SUSPENDIDO_ENVIAR_A_SWAP = 16, // CHECKPOINT 3: Requiere el PID solamente
    SOLICITUD_MEMORIA_DUMP_MEMORY = 17,     // 1 Parametro: PID
    RESPUESTA_DUMP_COMPLETADO = 18,          // Ningun parametro 
    RESPUESTA_DUMP_ERROR = 19,                  // No deberia pasar nunca, lo dejo programado por las dudas- Ningun parametro
    RESPUESTA_MEMORIA_LIBERADA_EXITOSAMENTE = 20,
    RESPUESTA_MEMORIA_PROCESO_ENVIADO_A_SWAP = 21,
    PETICION_INTERRUPT_A_CPU = 22,     
    RESPUESTA_MEMORIA_A_CPU_PETICION_MARCO = 23, //(?)
    RESPUESTA_MEMORIA_A_CPU_PAGINA_NO_VALIDA = 24, //(?)
    PETICION_INSTRUCCION_MEMORIA,
    PETICION_ESCRIBIR_EN_MEMORIA,
    PETICION_LEER_DE_MEMORIA,
    PETICION_TRADUCCION_DIRECCION, //(?)
    RESPUESTA_INSTRUCCION_MEMORIA,
    RESPUESTA_ESCRIBIR_EN_MEMORIA,
    RESPUESTA_LEER_DE_MEMORIA,
    PETICION_MARCO_MEMORIA,
    VERIFICARCONEXION,
    PETICION_ESCRIBIR_EN_MEMORIA_LIMITADO,
    PETICION_LEER_DE_MEMORIA_LIMITADO
} CODIGO_OP;


#endif