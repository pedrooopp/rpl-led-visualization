CONTIKI_PROJECT = cliente
# CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
all: $(CONTIKI_PROJECT)

# Por defecto Contiki-NG usa RPL-Lite. Para usar RPL-Classic se debe quitar el comentario de la siguiente linea
#MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC

# Se incluye el shell de Contiki-NG en el proyecto
MODULES += os/services/shell

# Se incluye TSCH para luego poder enlentecer el sistema
MAKE_MAC = MAKE_MAC_TSCH

CONTIKI ?= ../../..
include $(CONTIKI)/Makefile.include