typedef enum {

    ESTADO_PLACEHOLDER
} estado_t;

typedef enum {

    TRANS_PLACEHOLDER
} tipo_transicion_t;

typedef struct {

    tipo_transicion_t tipo;
    void *dato;
} transicion_t;
