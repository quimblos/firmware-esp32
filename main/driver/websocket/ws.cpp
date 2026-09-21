#include "esp_log.h"

#include "../../quimblos/serial.hpp"
#include "ws.hpp"
#include "keep_alive.hpp"

using namespace qb::driver;

const char* WebSocket::TAG = "WebSocket";

httpd_handle_t WebSocket::server = NULL;
WebSocket::Config WebSocket::config = {
    .uri = "/ws",
    .max_clients = 2
};

struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
};


static esp_err_t wss_open_fd(httpd_handle_t hd, int sockfd)
{
    ESP_LOGI(WebSocket::TAG, "New client connected %d", sockfd);
    wss_keep_alive_t h = (wss_keep_alive_t) httpd_get_global_user_ctx(hd);
    return wss_keep_alive_add_client(h, sockfd);
}

static void wss_close_fd(httpd_handle_t hd, int sockfd)
{
    ESP_LOGI(WebSocket::TAG, "Client disconnected %d", sockfd);
    wss_keep_alive_t h = (wss_keep_alive_t) httpd_get_global_user_ctx(hd);
    wss_keep_alive_remove_client(h, sockfd);
    close(sockfd);
}

static void send_hello(void *arg)
{
    static const char * data = "Hello client";
    struct async_resp_arg *resp_arg = (async_resp_arg *) arg;
    httpd_handle_t hd = resp_arg->hd;
    int fd = resp_arg->fd;
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t*)data;
    ws_pkt.len = strlen(data);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    httpd_ws_send_frame_async(hd, fd, &ws_pkt);
    free(resp_arg);
}

static void send_ping(void *arg)
{
    struct async_resp_arg *resp_arg = (async_resp_arg *) arg;
    httpd_handle_t hd = resp_arg->hd;
    int fd = resp_arg->fd;
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = NULL;
    ws_pkt.len = 0;
    ws_pkt.type = HTTPD_WS_TYPE_PING;

    httpd_ws_send_frame_async(hd, fd, &ws_pkt);
    free(resp_arg);
}

static bool client_not_alive_cb(wss_keep_alive_t h, int fd)
{
    ESP_LOGE(WebSocket::TAG, "Client not alive, closing fd %d", fd);
    httpd_sess_trigger_close(wss_keep_alive_get_user_ctx(h), fd);
    return true;
}

static bool check_client_alive_cb(wss_keep_alive_t h, int fd)
{
    ESP_LOGD(WebSocket::TAG, "Checking if client (fd=%d) is alive", fd);
    struct async_resp_arg *resp_arg = (async_resp_arg *) malloc(sizeof(struct async_resp_arg));
    assert(resp_arg != NULL);
    resp_arg->hd = wss_keep_alive_get_user_ctx(h);
    resp_arg->fd = fd;

    if (httpd_queue_work(resp_arg->hd, send_ping, resp_arg) == ESP_OK) {
        return true;
    }
    return false;
}

esp_err_t WebSocket::ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        ESP_LOGI(WebSocket::TAG, "Handshake done, the new connection was opened");
        return ESP_OK;
    }
    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

    // First receive the full ws message
    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(WebSocket::TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    ESP_LOGI(WebSocket::TAG, "frame len is %d", ws_pkt.len);
    if (ws_pkt.len) {
        /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        buf = (uint8_t *)calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE(WebSocket::TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(WebSocket::TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
    }
    // If it was a PONG, update the keep-alive
    if (ws_pkt.type == HTTPD_WS_TYPE_PONG) {
        ESP_LOGD(WebSocket::TAG, "Received PONG message");
        free(buf);
        return wss_keep_alive_client_is_active((wss_keep_alive_t)httpd_get_global_user_ctx(req->handle),
                httpd_req_to_sockfd(req));

    // If it was a TEXT message, just echo it back
    } else if (ws_pkt.type == HTTPD_WS_TYPE_TEXT || ws_pkt.type == HTTPD_WS_TYPE_PING || ws_pkt.type == HTTPD_WS_TYPE_CLOSE) {
        if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) {
            ESP_LOGI(WebSocket::TAG, "Received packet with message: %s", ws_pkt.payload);
            msg_handler((const char*) ws_pkt.payload);
        } else if (ws_pkt.type == HTTPD_WS_TYPE_PING) {
            // Response PONG packet to peer
            ESP_LOGI(WebSocket::TAG, "Got a WS PING frame, Replying PONG");
            ws_pkt.type = HTTPD_WS_TYPE_PONG;
        } else if (ws_pkt.type == HTTPD_WS_TYPE_CLOSE) {
            // Response CLOSE packet with no payload to peer
            ws_pkt.len = 0;
            ws_pkt.payload = NULL;
        }
        ret = httpd_ws_send_frame(req, &ws_pkt);
        if (ret != ESP_OK) {
            ESP_LOGE(WebSocket::TAG, "httpd_ws_send_frame failed with %d", ret);
        }
        ESP_LOGI(WebSocket::TAG, "ws_handler: httpd_handle_t=%p, sockfd=%d, client_info:%d", req->handle,
                 httpd_req_to_sockfd(req), httpd_ws_get_fd_info(req->handle, httpd_req_to_sockfd(req)));
        free(buf);
        return ret;
    }
    free(buf);
    return ESP_OK;
}

httpd_handle_t WebSocket::start_wss_server(void)
{
    // Prepare keep-alive engine
    wss_keep_alive_config_t keep_alive_config = KEEP_ALIVE_CONFIG_DEFAULT();
    keep_alive_config.max_clients = WebSocket::config.max_clients;
    keep_alive_config.client_not_alive_cb = client_not_alive_cb;
    keep_alive_config.check_client_alive_cb = check_client_alive_cb;
    wss_keep_alive_t keep_alive = wss_keep_alive_start(&keep_alive_config);

    // Start the httpd server
    httpd_handle_t server = NULL;
    ESP_LOGI(WebSocket::TAG, "Starting server");

    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();
#if CONFIG_IDF_TARGET_LINUX
    /* Use non-privileged port on Linux since port 443 requires root */
    conf.port_secure = 8443;
#endif
    conf.httpd.max_open_sockets = WebSocket::config.max_clients;
    conf.httpd.global_user_ctx = keep_alive;
    conf.httpd.open_fn = wss_open_fd;
    conf.httpd.close_fn = wss_close_fd;

    extern const unsigned char servercert_start[] asm("_binary_servercert_pem_start");
    extern const unsigned char servercert_end[]   asm("_binary_servercert_pem_end");
    conf.servercert = servercert_start;
    conf.servercert_len = servercert_end - servercert_start;

    extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
    extern const unsigned char prvtkey_pem_end[]   asm("_binary_prvtkey_pem_end");
    conf.prvtkey_pem = prvtkey_pem_start;
    conf.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

    esp_err_t ret = httpd_ssl_start(&server, &conf);
    if (ESP_OK != ret) {
        ESP_LOGI(WebSocket::TAG, "Error starting server!");
        return NULL;
    }

    // Add WebSocket uri handler
    ESP_LOGI(WebSocket::TAG, "Registering URI handlers");
    static const httpd_uri_t uri = {
        .uri        = config.uri.c_str(),
        .method     = HTTP_GET,
        .handler    = WebSocket::ws_handler,
        .user_ctx   = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = true,
        .supported_subprotocol = NULL
    };
    httpd_register_uri_handler(server, &uri);
    wss_keep_alive_set_user_ctx(keep_alive, server);

    return server;
}

#if !CONFIG_IDF_TARGET_LINUX
static esp_err_t stop_wss_echo_server(httpd_handle_t server)
{
    // Stop keep alive thread
    wss_keep_alive_stop((wss_keep_alive_t)httpd_get_global_user_ctx(server));
    // Stop the httpd server
    return httpd_ssl_stop(server);
}

// /* example_connect() gives up after CONFIG_EXAMPLE_WIFI_CONN_MAX_RETRY failed attempts, so
//  * keep re-driving the connection ourselves. Without this, a device that loses its AP (or
//  * never joins it) stays offline for good, and the server never comes up.
//  */
// static void wifi_reconnect_task(void* arg)
// {
//     while (1) {
//         vTaskDelay(10000 / portTICK_PERIOD_MS);

//         wifi_ap_record_t ap_info;
//         if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
//             ESP_LOGI(TAG, "Wi-Fi not connected, retrying");
//             /* Deliberately not error-checked: a redundant call just fails harmlessly. */
//             esp_wifi_connect();
//         }
//     }
// }
#endif // !CONFIG_IDF_TARGET_LINUX


void WebSocket::connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        *server = start_wss_server();
    }
}

void WebSocket::disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        if (stop_wss_echo_server(*server) == ESP_OK) {
            *server = NULL;
        } else {
            ESP_LOGE(WebSocket::TAG, "Failed to stop https server");
        }
    }
}

void WebSocket::msg_handler(const std::string& payload) {
    uint8_t kind = qb::serial::chhex(payload[0])*16 + qb::serial::chhex(payload[1])*16;
    if (callbacks.contains(kind)) {
        ESP_LOGI(TAG, "Parsing message of kind %d", kind);
        auto wrap = engine->parse(kind, payload);
        ESP_LOGI(TAG, "%s", engine->to_json(wrap).c_str());
        callbacks.at(kind)(wrap);
        delete wrap;
    }
    else {
        ESP_LOGW(TAG, "Unknown message kind %d", kind);
    }
}

// Get all clients and send async message
void WebSocket::send_messages()
{
    bool send_messages = true;

    // Send async message to all connected clients that use websocket protocol every 10 seconds
    while (send_messages) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);

        if (!server) { // httpd might not have been created by now
            continue;
        }
        size_t clients = config.max_clients;
        int* client_fds = new int[clients]();
        if (httpd_get_client_list(server, &clients, client_fds) == ESP_OK) {
            for (size_t i=0; i < clients; ++i) {
                int sock = client_fds[i];
                if (httpd_ws_get_fd_info(server, sock) == HTTPD_WS_CLIENT_WEBSOCKET) {
                    ESP_LOGI(TAG, "Active client (fd=%d) -> sending async message", sock);
                    struct async_resp_arg *resp_arg = (async_resp_arg *) malloc(sizeof(struct async_resp_arg));
                    assert(resp_arg != NULL);
                    resp_arg->hd = server;
                    resp_arg->fd = sock;
                    if (httpd_queue_work(resp_arg->hd, send_hello, resp_arg) != ESP_OK) {
                        ESP_LOGE(TAG, "httpd_queue_work failed!");
                        send_messages = false;
                        break;
                    }
                }
            }
        } else {
            ESP_LOGE(TAG, "httpd_get_client_list failed!");
            return;
        }
    }
}