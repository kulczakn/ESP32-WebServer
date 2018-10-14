#ifndef	_M_CHAT_H
#define _M_CHAT_H

int chat_connection_add(struct netconn* connection);
int chat_connection_remove(int chat_id);
int chat_handle_message(WebSocket_frame_t frame);
void chat_broadcast(WebSocket_frame_t frame);

#endif