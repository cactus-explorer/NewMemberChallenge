idf_component_register(
    SRCS "main.c" "helper.c"
    INCLUDE_DIRS "."
    REQUIRES freertos driver esp_driver_gpio
) 

