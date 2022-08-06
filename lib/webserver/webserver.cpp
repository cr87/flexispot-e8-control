#include "webserver.h"

#include <ESP8266WebServer.h>
#include "log.h"

/*****************************************************************************/

ESP8266WebServer ws_instance;
fn_height_provider ws_height_provider_fn = NULL;
fn_command_receiver ws_command_receiver_fn = NULL;
fn_time_provider ws_time_provider_fn = NULL;

/*****************************************************************************/

void handleRoot();
void handleStand();
void handleNotFound();

/*****************************************************************************/

void ws_init(const uint16 server_port_u16,
             fn_height_provider height_provider,
             fn_command_receiver command_receiver,
             fn_time_provider time_provider)
{
    /* Store function pointers */
    ws_height_provider_fn = height_provider;
    ws_command_receiver_fn = command_receiver;
    ws_time_provider_fn = time_provider;

    /* Initialize server */
    ws_instance.begin(server_port_u16);

    ws_instance.on("/", handleRoot);        // Call the 'handleRoot' function when a client requests URI "/"
    ws_instance.on("/stand", handleStand);  // Call the 'handleRoot' function when a client requests URI "/"
    ws_instance.onNotFound(handleNotFound); // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"

    ws_instance.begin(); // Actually start the server
    log_msg(LOG_LEVEL_INFO, "WebServer", "HTTP server started on port %i.", server_port_u16);
}

void ws_loop()
{
    ws_instance.handleClient(); // Listen for HTTP requests from clients
}

/*****************************************************************************/

void handleRoot()
{
    uint16 height_u16;
    const DATETIME *p_time;

    if ((NULL != ws_height_provider_fn) && (NULL != ws_time_provider_fn))
    {
        height_u16 = ws_height_provider_fn();
        p_time = ws_time_provider_fn();

        if (height_u16 > 0U)
        {
            char str[256]; /* XXX */
            sprintf(str, "Height: %i cm\nTime: %i:%i:%i\nDate: %i/%i/%i", (int)height_u16, p_time->time.hour_u8, p_time->time.minute_u8, p_time->time.second_u8, p_time->date.year_u16, p_time->date.month_u8, p_time->date.day_u8);
            ws_instance.send(200, "text/plain", str); // Send HTTP status 200 (Ok) and send some text to the browser/client
        }
        else
        {
            ws_instance.send(200, "text/plain", "No valid height reading :(");
        }
    }
    else
    {
        ws_instance.send(200, "text/plain", "No height provider registered!"); // Send HTTP status 200 (Ok) and send some text to the browser/client
    }
}

void handleNotFound()
{
    ws_instance.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

void handleStand()
{
    if (NULL != ws_command_receiver_fn)
    {
        ws_command_receiver_fn(DC_CMD_UP);
    }
}
