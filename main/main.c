#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>

const static char* TAG = "main.c";


static SemaphoreHandle_t mutex;
static esp_event_loop_handle_t event_loop;

// Handler donde se recibirán y procesarán los eventos de todos los componentes.
void event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {

    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdFALSE) {
        ESP_LOGE(TAG, "Could not acquire mutex for event base %s and id %d", event_base, event_id);
        return;
    }

    // TODO: poner el switch con los eventos.
    ESP_LOGI(TAG, "Recibido evento de base: %s e id: %d", event_base, event_id);

    xSemaphoreGive(mutex);
}

void app_main(void) {

    ESP_ERROR_CHECK(nvs_flash_init());

    mutex = xSemaphoreCreateMutex();
    esp_event_loop_args_t event_loop_args = {
        .queue_size = 10,
        .task_name = "FSM event",
        .task_stack_size = 2048, // TODO: igual hay que subir esto.
        .task_priority = uxTaskPriorityGet(NULL),
        .task_core_id = tskNO_AFFINITY
    };

    ESP_ERROR_CHECK(esp_event_loop_create(&event_loop_args, &event_loop));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(
        event_loop, 
        ESP_EVENT_ANY_BASE, 
        ESP_EVENT_ANY_ID, 
        event_handler, 
        NULL, 
        NULL)
    );


}
