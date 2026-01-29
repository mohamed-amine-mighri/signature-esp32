if(CONFIG_UART_LAYER)
    set(TRANSPORT_SRC UART_communication.c)
elseif(CONFIG_MQTT_LAYER)
    set(TRANSPORT_SRC MQTT_communication.c)
endif()

file(GLOB 
    SRCS
        ${TRANSPORT_SRC}
        main.c 
    ) 

idf_component_register(
    SRCS 
        ${SRCS}
    
    INCLUDE_DIRS 
        ".")