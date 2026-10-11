// Se definen los eventos que se usan en más de un unico archivo como extern
extern process_event_t dio_evento;
extern process_event_t dao_evento;
extern process_event_t dis_evento;

extern process_event_t hop_evento;

extern process_event_t udp_evento;

// Se define la variable que guarda la IP del nodo destino del mensaje UDP como extern, pues se usa en más de un archivo
extern uip_ipaddr_t dest_ipaddr_shell;
