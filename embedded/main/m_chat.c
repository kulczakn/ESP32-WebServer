#include <string.h>

#include "lwip/api.h"
#include "esp_log.h"

#include "m_wifi.h"
#include "m_websocket.h"
#include "m_chat.h"

#define TAG "m_chat.c"

#define MAX_CHAT_CONNECTIONS AP_MAX_CONNECTIONS

struct netconn* chat_connections[MAX_CHAT_CONNECTIONS];

int chat_connection_add(struct netconn* conn)
{
	for(size_t i = 0; i < MAX_CHAT_CONNECTIONS; i++)
	{
		if(chat_connections[i] == NULL)
		{
			ESP_LOGI("Adding %d to chat.", i);
			chat_connections[i] = conn;
			return i;
		}
	}

	ESP_LOGW("No more room in the chat.");
	return -1;
}

int chat_connection_remove(int chat_id)
{
	ESP_LOGI(TAG, "Removing %u.", chat_id);

	if(chat_connections[chat_id])
	{
		chat_connections[chat_id] = NULL;
		return chat_id;
		ESP_LOGI(TAG, "%d removed.", chat_id);
	}

	ESP_LOGI(TAG, "%d not in chat.", chat_id);
	return -1;
}

int chat_handle_message(WebSocket_frame_t frame)
{
	ESP_LOGI(TAG, "Handling message: %.*s.", frame.payload_length, frame.payload);
	int i = -1;

	chat_broadcast(frame);

	return i;
}



void chat_broadcast(WebSocket_frame_t frame)
{
	for(size_t i = 0; i < MAX_CHAT_CONNECTIONS; i++)
	{
		if(chat_connections[i])
		{
			ESP_LOGI(TAG, "Sending %s to %d.", frame.payload, i);
			websocket_write_data(chat_connections[i], frame.payload, frame.payload_length);
		}
	}
}