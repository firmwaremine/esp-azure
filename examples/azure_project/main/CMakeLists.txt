set(COMPONENT_SRCS "main.c"
                "AzureClient.c"
                "AzureCore.c"
                )
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()

component_compile_definitions(SET_TRUSTED_CERT_IN_SAMPLES)
