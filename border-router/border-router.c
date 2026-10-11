/*
 * Copyright (c) 201, RISE SICS
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "dev/leds.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "sys/node-id.h"
#include "net/routing/routing.h"
#include "dev/button-hal.h"

// Se incluye "variables_compartidas.h", contiene los eventos de cada mensaje relevante
#include "../cliente/variables_compartidas.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "RPL BR"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_PORT	5678 // Define el puerto UDP

static struct simple_udp_connection udp_conn; // Define la variable llamada "udp_conn", que contiene la conexión UDP

process_event_t udp_recibido; // Proceso asociado a cuando se recibe un mensage UDP
/*---------------------------------------------------------------------------*/
PROCESS(red_leds_process, "LEDs rojos");
PROCESS(green_leds_process, "LEDs verdes");

AUTOSTART_PROCESSES(&red_leds_process, &green_leds_process); // Se inician los procesos relevantes
/*---------------------------------------------------------------------------*/
// Define el callback asociado al server
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  LOG_INFO("Mensaje recibido '%.*s' de ", datalen, (char *) data); // Muestra en los logs el mensaje recibido
  process_post(&green_leds_process, udp_recibido, NULL); // Se postea un evento para indicar que un mensaje UDP fue recibido
  LOG_INFO_6ADDR(sender_addr); // Muestra en los logs la dirección IP del reminente
  #if LLSEC802154_CONF_ENABLED
    LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
  #endif
  LOG_INFO_(".\n"); // Salto de linea
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(red_leds_process, ev, data)
{
  PROCESS_BEGIN();

  /* BORDER_ROUTER_CONF_WEBSERVER */
  #if BORDER_ROUTER_CONF_WEBSERVER
    PROCESS_NAME(webserver_nogui_process);
    process_start(&webserver_nogui_process, NULL);
  #endif
  LOG_INFO("Contiki-NG Border Router started\n");

  // Se definen las variables necesarias
  static bool unicast_indicador; // Booleano que define si un mensaje es unicast o broadcast
  static struct etimer timer_while; // Timer usado en el loop (es de 1 segundo)
  static struct etimer timer_frec; // Timer que define con que frecuencia se togglean los LEDs (tiene un valor variable para cada evento)

  radio_value_t ch_power; // Variable utilizada para cambiar la potencia del transmisor

  // Se reserva memoria para los eventos que utilizan los leds rojos
  dio_evento = process_alloc_event();
  dao_evento = process_alloc_event();
  dis_evento = process_alloc_event();

  // Inicializa la conexión UDP
  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, udp_rx_callback);

	// Inicializa la ID del nodo
	node_id_init();

  NETSTACK_RADIO.get_value(RADIO_PARAM_TXPOWER,&ch_power);
	LOG_INFO("RF_POWER: %d\n", ch_power); // Se imprime el valor de la potencia TX inicial

  NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER , (radio_value_t) -10);

  NETSTACK_RADIO.get_value(RADIO_PARAM_TXPOWER,&ch_power);
	LOG_INFO("RF_POWER: %d\n", ch_power); // Se imprime el valor de la potencia TX luego de ser cambiada. Deberia ser -10 dB

  // Loop infinito
  while(1) {

    // Espera que llegue un evento asociado a un paquete de control RPL
    PROCESS_WAIT_EVENT_UNTIL(ev == dio_evento || ev == dao_evento || ev == dis_evento); 

    if (ev == dio_evento){

      unicast_indicador = *(bool *)data; // Se castea la "data" enviada por el posteo del evento. 
      LOG_INFO("Se envio un DIO %s", (unicast_indicador ? "unicast." : "multicast.")); // Avisa en los logs que se envió un DIO, indicando si es multicast o unicast.
      LOG_INFO_("\n"); // Salto de linea

      // Se diferencian los casos cuando el DIO es un unicast o un multicast
      // Cuando es unicast, el LED verde se prende y apaga varias veces, mientras el rojo permanece prendido de forma constante.
      if (unicast_indicador) { 
        leds_on(LEDS_RED);
        etimer_set(&timer_while, 1*CLOCK_SECOND); // Se prenden el LED y se espera un segundo antes de apagarlo.

        // Se define un while que se ejecuta durante un segundo
        while (!etimer_expired(&timer_while))
        {
          leds_toggle(LEDS_GREEN);
          etimer_set(&timer_frec, 0.2*CLOCK_SECOND); // Se togglea el LED verde cada 0.2 segundos
          PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer_frec));
        }

        leds_off(LEDS_GREEN); // Se asegura que el LED verde esta apagado
        leds_off(LEDS_RED);

      } else { // Cuando es multicast solamente se prende el LED rojo, el cual permanece prendido de forma constante.
        leds_on(LEDS_RED);
        etimer_set(&timer_while, 1*CLOCK_SECOND); // Se prende el LED y se espera un segundo antes de apagarlo.
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer_while));
        leds_off(LEDS_RED);
      }

    } else if (ev == dao_evento){

      LOG_INFO("Se envio un DAO."); // Avisa en los logs que se envió un DAO
      LOG_INFO_("\n"); // Salto de linea

      etimer_set(&timer_while, 1*CLOCK_SECOND); // Se inicia un timer de un segundo

      // Se define un while que se ejecuta durante un segundo
      while (!etimer_expired(&timer_while))
      {
        leds_toggle(LEDS_RED);
        etimer_set(&timer_frec, 0.33*CLOCK_SECOND); // Se toggle el LED cada 0.33 segundos
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer_frec));
      }
      leds_off(LEDS_RED); // Se asegura que el LED este apagado

    } else if (ev == dis_evento) {

      LOG_INFO("Se envio un DIS."); // Avisa en los logs que se envió un DIS
      LOG_INFO_("\n"); // Salto de linea

      etimer_set(&timer_while, 1*CLOCK_SECOND);
      
      // Se define un while que se ejecuta durante un segundo
      while (!etimer_expired(&timer_while))
      {
        leds_toggle(LEDS_RED);
        etimer_set(&timer_frec, 0.2*CLOCK_SECOND); // Se togglea el LED cada 0.2 segundos
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer_frec));
      }
      leds_off(LEDS_RED);  // Se asegura que el LED este apagado
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(green_leds_process, ev, data)
{
  static char str[36]; // Array que guarda el mensaje UDP a enviar
  static struct etimer timer_led; // Timer usado para togglear los LEDs

  PROCESS_BEGIN();

  // Se reserva memoria para los eventos que utilizan los leds verdes
  udp_evento = process_alloc_event();
  udp_recibido = process_alloc_event();
  hop_evento = process_alloc_event();

  // Loop infinito
  while(1) {

    // Espera a que llegue un evento relevante
    PROCESS_WAIT_EVENT_UNTIL(ev == hop_evento || ev == udp_evento || ev == udp_recibido);

    if (ev == hop_evento) {

      LOG_INFO("Se realiza un salto intermedio (HOP)."); // Notifica saltos intermedios
      LOG_INFO_("\n"); // Salto de linea

      leds_on(LEDS_GREEN);
      
      // Se prende el LED y se espera medio segundo antes de apagarlo
      etimer_set(&timer_led, 0.5*CLOCK_SECOND);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer_led));
      leds_off(LEDS_GREEN);

    } else if (ev == udp_evento){

      LOG_INFO("Se envio un mensaje UDP."); // Avisa en los logs que el cliente enviara un mensaje
      LOG_INFO_("\n"); // Salto de linea

      snprintf(str, sizeof(str), "Buenas."); // Crea el string con el mensaje a enviar: "Buenas."

      // Se envía mensaje UDP a la IP que se ingresó por shell
      simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr_shell);

      leds_on(LEDS_GREEN); 
      
      etimer_set(&timer_led, 1*CLOCK_SECOND); // Se prende el LED y se espera un segundo antes de apagarlo
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer_led));
      leds_off(LEDS_GREEN);
    }
    else if (ev == udp_recibido){

      LOG_INFO("Se recibio un mensaje UDP"); // Se informa la llegada de un mensaje UDP
      LOG_INFO_("\n"); // Salto de linea

      leds_on(LEDS_GREEN);

      etimer_set(&timer_led, 1*CLOCK_SECOND); // Se prende el LED y se espera un segundo antes de apagarlo
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer_led));
      leds_off(LEDS_GREEN);
    }
  }
  PROCESS_END();
}