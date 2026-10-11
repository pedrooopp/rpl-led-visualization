// Activa el nivel "debug" de los LOGS de RPL. Es util para ver si efectivamente se envian paquetes de control RPL
// Inicialmente la linea esta comentada para simplificar la salida en la consola
//#define LOG_CONF_LEVEL_RPL LOG_LEVEL_DBG

// Para formar el DODAG más rapido, TSCH usa solamente un canal
#define TSCH_CONF_DEFAULT_HOPPING_SEQUENCE (uint8_t[]){23} // Define el canal que se recorrera una vez el nodo se unio a la red
#define TSCH_CONF_JOIN_HOPPING_SEQUENCE (uint8_t[]){23} // Define el canal que se recorrera para unirse a la red (escaneo de canal)

// Cantidad maxima de entradas (rutas), minimiza uso de RAM. 
// Se espera usar como maximo 8 nodos, si se aumenta la cantidad hay que aumentar este numero
#define NETSTACK_MAX_ROUTE_ENTRIES 8 
