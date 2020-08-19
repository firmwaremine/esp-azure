set(COMPONENT_SRCS "main.c"
                    "AzureClient.c"
                    "AzureCore.c"
                    "bme280_support.c"
                    "bme280.c"
                    "bmi160_support.c"
                    "bmi160.c"
                    "i2c_driver.c"
                    "LCDIIC.c"
                    "opt3001.c"
                    "SensorCore.c"
                    "tmp007.c"
                    )
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()

component_compile_definitions(SET_TRUSTED_CERT_IN_SAMPLES)
