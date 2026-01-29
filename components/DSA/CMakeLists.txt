file(GLOB
    DSA_DIR
        "falcon-512/*.c"
        "falcon-1024/*.c"
        "falcon-padded-512/*.c"
        "falcon-padded-1024/*.c"
        "ml-dsa-44/*.c"
        "ml-dsa-65/*.c"
        "ml-dsa-87/*.c"
        "sphincs-sha2-128f/*.c"
        "sphincs-sha2-128s/*.c"
        "sphincs-sha2-192f/*.c"
        "sphincs-sha2-192s/*.c"
        "sphincs-sha2-256f/*.c"
        "sphincs-sha2-256s/*.c"
        "sphincs-shake-128f/*.c"
        "sphincs-shake-128s/*.c"
        "sphincs-shake-192f/*.c"
        "sphincs-shake-192s/*.c"
        "sphincs-shake-256f/*.c"
        "sphincs-shake-256s/*.c"
)


file(GLOB
    SRCS 
        ${DSA_DIR}
        "src/*.c"
        "common/*.c"
)

idf_component_register(
    SRCS 
        ${SRCS}
    INCLUDE_DIRS 
        "."
        "src"
        "common"
)

