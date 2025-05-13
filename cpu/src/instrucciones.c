///////////////////////

/*
void atenderPeticionKernel(void) { //queda en while esperando cosas, cuando le llega una cosa (que recibira con recibirPIDyPC)
    while(true) {
        struct x { pid pc }
        recibirPIDyPC(x)
        solicitudInstruccionMemoria()
        solicitud = recibirSolicitudMemoria()

        while(!termino)
            ejecutarProximaInstruccion(x)
        
    }
}

//recibirPIDyPC(&x) //supongo que recibe un paquete de Kernel que contenga pid y pc, luego los cargo a la estructura
//solicitudInstruccionMemoria() //Envia un paquete a memoria con un codeOp "SOL_INSTRUCCION" ¿que me devuelve memoria?
//recibirSolicitudMemoria() //espera la respuesta de la memoria a la peticion

//ejecutarProximaInstruccion(pc) { //Es enorme, con cases, y puedo llegar a llamar a: traducirDireccionLogica()
    int tipo_instruccion = interpretarInstruccion();
    switch(tipo_instruccion) { //lo de traduccir sera tambien una instruccion
        case INSTR_NOOP:

        case INSTR_WHITE:
        
        case INSTR_READ:
        
        case INSTR_GOTO:
    
        case INSTR_IO:
        
        case INSTR_INIT_PROC:
        
        case INSTR_DUMP_MEMORY:
        
        case INSTR_EXIT:
    }
}

//interpretarInstruccion //devuelve el tipo de isntrucccion?
//traducirDireccionLogica

//actualizarProgramCounter

/////////////////posibles instruccciones///////
NOOP
WRITE 0 EJEMPLO_DE_ENUNCIADO
READ 0 20
GOTO 0
////////las siguientes se concideran syscalls
IO IMPRESORA 25000
INIT_PROC preceso1 256
DUMP_MEMORY
EXIT







*/