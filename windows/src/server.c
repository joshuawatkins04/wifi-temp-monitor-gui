#include "mongoose.h"
#include "config.h"

static void handle_request(struct mg_connection *c, int ev, void *ev_data)
{
  if (ev == MG_EV_HTTP_MSG)
  {
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    char response[128];

    snprintf(response, sizeof(response), "Temperature: %.2f C<br>Humidity: %.2f %%", config.globalTemperature, config.globalHumidity);
    mg_http_reply(c, 200, "Content-Type: text/html\r\n", "%s", response);
  }
}

void start_server()
{
  struct mg_mgr mgr;
  mg_mgr_init(&mgr);

  struct mg_connection *c = mg_http_listen(&mgr, "http://0.0.0.0:8081", handle_request, NULL);
  if (c == NULL)
  {
    printf("Error starting server on port 8081\n");
    return;
  }
  printf("Web server started.\n");
  for (;;)
  {
    mg_mgr_poll(&mgr, 1000);
  }

  mg_mgr_free(&mgr);
}