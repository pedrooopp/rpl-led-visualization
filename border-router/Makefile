CONTIKI_PROJECT = border-router
all: $(CONTIKI_PROJECT)
CONTIKI ?= ../../..

# Por defecto Contiki-NG usa RPL-Lite. Para usar RPL-Classic se debe quitar el comentario de la siguiente linea
#MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC

# Se incluye el shell de Contiki-NG en el proyecto
MODULES += os/services/shell

# The BR is either native or embedded, and in the latter case must support SLIP
PLATFORMS_EXCLUDE = z1

# Se incluye TSCH para luego poder enlentecer el sistema
MAKE_MAC = MAKE_MAC_TSCH

# Include RPL BR module
include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_SERVICES_DIR)/rpl-border-router
# Include webserver module
MODULES_REL += webserver
# Include optional target-specific module
include $(CONTIKI)/Makefile.identify-target
MODULES_REL += $(TARGET)

include $(CONTIKI)/Makefile.include
