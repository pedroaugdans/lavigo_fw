/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "drv_http.h"
#include "drv_wifi.h"

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"

#include "hub_error.h"

#include <esp_http_server.h>
#include "lwip/apps/netbiosns.h"
#include "driver/sdmmc_host.h"
#include "mdns.h"
#include "sdmmc_cmd.h"
#define MDNS_INSTANCE "esp home web server"

/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server.
 */


typedef struct {
    key_callback data_callback;
    char * key;
} http_new_data_t;

static esp_err_t drv_method_handler(httpd_req_t *req);
static const char *TAG = "[DRV-HTTP]";

static char index_html[] = "<!DOCTYPE HTML><html><head>  <title>Lavigo Config</title>  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">  </head><body>  <form action=\"/get\">    Username: <input type=\"text\" name=\"input1\">    <input type=\"submit\" value=\"Submit\"> </form><br>  <form action=\"/get\">    Password: <input type=\"text\" name=\"input2\">    <input type=\"submit\" value=\"Submit\">  </form><br>  <form action=\"/get\">    input3: <input type=\"text\" name=\"input3\">    <input type=\"submit\" value=\"Submit\">  </form></body></html>";

static httpd_handle_t server = NULL;
static http_new_data_t registered_kes[MAX_KEYS];
static uint8_t total_keys = 0;

uint8_t drv_http_register_key(char * key, key_callback key_cb) {
    uint8_t toreturn = total_keys;
    registered_kes[total_keys].key = key;
    registered_kes[total_keys].data_callback = key_cb;
    total_keys++;
    return toreturn;
}

/* An HTTP GET handler */
static esp_err_t drv_method_handler(httpd_req_t *req) {
    char* buf;
    size_t buf_len;
    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }



    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Test-Header-2: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Test-Header-1: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[32];
            for (uint8_t k = 0; k < total_keys; k++) {
                if (httpd_query_key_value(buf, registered_kes[k].key, param, sizeof (param)) == ESP_OK) {
                    ESP_LOGI(TAG, "Found URL query parameter => [%s]=[%s]", registered_kes[k].key, param);
                    registered_kes[k].data_callback(param);
                    break;
                }
            }
        }
        free(buf);
    }

    /* Set some custom headers */
    httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
    httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, strlen(resp_str));

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG, "Request headers lost");
    }
    return ESP_OK;
}

static const httpd_uri_t uri_get = {
    .uri = "/get",
    .method = HTTP_GET,
    .handler = drv_method_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = index_html
};

static const httpd_uri_t uri_root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = drv_method_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = index_html
};

/* This handler allows the custom error handling functionality to be
 * tested from client side. For that, when a PUT request 0 is sent to
 * URI /ctrl, the /hello and /echo URIs are unregistered and following
 * custom error handler http_404_error_handler() is registered.
 * Afterwards, when /hello or /echo is requested, this custom error
 * handler is invoked which, after sending an error message to client,
 * either closes the underlying socket (when requested URI is /echo)
 * or keeps it open (when requested URI is /hello). This allows the
 * client to infer if the custom error handler is functioning as expected
 * by observing the socket state.
 */
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err) {
    if (strcmp("/hello", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    } else if (strcmp("/echo", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/echo URI is not available");
        /* Return ESP_FAIL to close underlying socket */
        return ESP_FAIL;
    }
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

/* An HTTP PUT handler. This demonstrates realtime
 * registration and deregistration of URI handlers
 */

static httpd_handle_t start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_resp_headers = 10;
    config.max_uri_handlers = 10;
    config.max_open_sockets = 6;
    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_root);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server) {
    // Stop the httpd server
    httpd_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data) {
    ESP_LOGI(TAG, "[disConnected device handler]");
    if (server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(server);
        server = NULL;
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data) {
    ESP_LOGI(TAG, "[Connected device handler]");
    if (server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        server = start_webserver();
    }
}

static void initialise_mdns(void)__attribute__((unused));

static void initialise_mdns(void){
    mdns_init();
    mdns_hostname_set("lavigo-config");
    mdns_instance_name_set(MDNS_INSTANCE);

    mdns_txt_item_t serviceTxtData[] = {
        {"board", "esp32"},
        {"path", "/"}
    };

    ESP_ERROR_CHECK(mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData,
                                     sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}

void drv_init_http(void) {
    ESP_LOGI(TAG, "[STARTING device handler]");
    drv_ap_event_install(&server);
    drv_wifi_server_installl(Apset, connect_handler);
    drv_wifi_server_installl(apDown, disconnect_handler);
    //initialise_mdns();
    //netbiosns_init();
    //netbiosns_set_name("lavigo");
    /* Start the server for the first time */
}
