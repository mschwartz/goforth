#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "wifi_credentials.h"

/* FreeRTOS event group to signal when we are connected & ready to make a
 * request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

/* Constants that aren't configurable in menuconfig */
//#define WEB_SERVER "coldfire"
//#define WEB_PORT 80
//#define WEB_URL "http://coldfire/"

static const char* TAG = "example";

static const char* REQUEST =
  "GET %s"
  " HTTP/1.0\r\n"
  "Host: %s"
  "\r\n"
  "User-Agent: esp-idf/1.0 esp32\r\n"
  "\r\n";

static esp_err_t event_handler(void* ctx, system_event_t* event)
{
  switch (event->event_id) {
  case SYSTEM_EVENT_STA_START:
    esp_wifi_connect();
    break;
  case SYSTEM_EVENT_STA_GOT_IP:
    xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    /* This is a workaround as ESP32 WiFi libs don't currently
       auto-reassociate. */
    esp_wifi_connect();
    xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    break;
  default:
    break;
  }
  return ESP_OK;
}

static void initialise_wifi(void)
{
  tcpip_adapter_init();
  wifi_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  wifi_config_t wifi_config = {
    .sta =
      {
        .ssid = WIFI_SSID,     // defined in wifi_credentials.h
        .password = WIFI_PASS, // defined in wifi_credentials.h
      },
  };
  ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
}

const char* HttpGet(const char* url)
{
  printf("HttpGet(%s)\n", url);

  int len = strlen(url) + 1;
  char host[len], port[len], path[len];
  char* dst = host;

  const struct addrinfo hints = {
    .ai_family = AF_INET,
    .ai_socktype = SOCK_STREAM,
  };
  struct addrinfo* res;
  struct in_addr* addr;
  int s, r;
  char recv_buf[64];

  // parse host
  if (!strncasecmp(url, "http://", 7)) {
    url += 7;
  }
  while (*url && *url != ':' && *url != '/') {
    *dst++ = *url++;
  }
  *dst = '\0';

  // parse port, if present.  If not, assume 80.
  if (*url != ':') {
    strcpy(port, "80");
  } else if (*url == ':') {
    url++;
    dst = port;
    while (*url && *url != '/') {
      *dst++ = *url++;
    }
    *dst = '\0';
  }
  // path to resource is what's left in url
  strcpy(path, url);
  if (!strlen(path)) {
    strcpy(path, "/");
  }

  ESP_LOGI(TAG, "host:port %s:%s", host, port);
  int err = getaddrinfo(host, port, &hints, &res);

  if (err != 0 || res == NULL) {
    ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    return NULL;
  }

  /* Code to print the resolved IP.
     Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code
   */
  addr = &((struct sockaddr_in*)res->ai_addr)->sin_addr;
  ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

  s = socket(res->ai_family, res->ai_socktype, 0);
  if (s < 0) {
    ESP_LOGE(TAG, "... Failed to allocate socket.");
    freeaddrinfo(res);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    return NULL;
  }
  ESP_LOGI(TAG, "... allocated socket");

  if (connect(s, res->ai_addr, res->ai_addrlen) != 0) {
    ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
    close(s);
    freeaddrinfo(res);
    vTaskDelay(4000 / portTICK_PERIOD_MS);
    return NULL;
  }

  ESP_LOGI(TAG, "... connected");
  freeaddrinfo(res);

  {
    char request[strlen(REQUEST) - 4 + strlen(path) + strlen(host) + 1];
    sprintf(request, REQUEST, path, host);
    printf("REQUEST\n%s\n", request);
    if (write(s, request, strlen(request)) < 0) {
      ESP_LOGE(TAG, "... socket send failed");
      close(s);
      vTaskDelay(4000 / portTICK_PERIOD_MS);
      return NULL;
    }
    ESP_LOGI(TAG, "... socket send success");
  }

  struct timeval receiving_timeout;
  receiving_timeout.tv_sec = 5;
  receiving_timeout.tv_usec = 0;
  if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                 sizeof(receiving_timeout)) < 0) {
    ESP_LOGE(TAG, "... failed to set socket receiving timeout");
    close(s);
    vTaskDelay(4000 / portTICK_PERIOD_MS);
    return NULL;
  }
  ESP_LOGI(TAG, "... set socket receiving timeout success");

  /* Read HTTP response */
  {
    int len = 0;
    char* buf = NULL;
    do {
      bzero(recv_buf, sizeof(recv_buf));
      r = read(s, recv_buf, sizeof(recv_buf) - 1);
      if (!buf) {
        len = r + 1;
        buf = malloc(r + 1);
        memcpy(buf, recv_buf, r);
      } else {
        int l = len - 1;
        len += r;
        //        printf("read %d, l=%d, len=%d\n", r, l, len);
        buf = realloc(buf, len);
        memcpy(&buf[l], recv_buf, r);
        buf[len - 1] = '\0';
      }
    } while (r > 0);

    ESP_LOGI(
      TAG, "... done reading from socket. Last read return=%d errno=%d\r\n", r, errno);
    close(s);
    return buf;
  }
}

#if 0
static void http_get_task(void* pvParameters)
{
  const struct addrinfo hints = {
    .ai_family = AF_INET,
    .ai_socktype = SOCK_STREAM,
  };
  struct addrinfo* res;
  struct in_addr* addr;
  int s, r;
  char recv_buf[64];

  while (1) {
    /* Wait for the callback to set the CONNECTED_BIT in the
       event group.
    */
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "Connected to AP");

    int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

    if (err != 0 || res == NULL) {
      ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }

    /* Code to print the resolved IP.
       Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code
     */
    addr = &((struct sockaddr_in*)res->ai_addr)->sin_addr;
    ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

    s = socket(res->ai_family, res->ai_socktype, 0);
    if (s < 0) {
      ESP_LOGE(TAG, "... Failed to allocate socket.");
      freeaddrinfo(res);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }
    ESP_LOGI(TAG, "... allocated socket");

    if (connect(s, res->ai_addr, res->ai_addrlen) != 0) {
      ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
      close(s);
      freeaddrinfo(res);
      vTaskDelay(4000 / portTICK_PERIOD_MS);
      continue;
    }

    ESP_LOGI(TAG, "... connected");
    freeaddrinfo(res);

    if (write(s, REQUEST, strlen(REQUEST)) < 0) {
      ESP_LOGE(TAG, "... socket send failed");
      close(s);
      vTaskDelay(4000 / portTICK_PERIOD_MS);
      continue;
    }
    ESP_LOGI(TAG, "... socket send success");

    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 5;
    receiving_timeout.tv_usec = 0;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                   sizeof(receiving_timeout)) < 0) {
      ESP_LOGE(TAG, "... failed to set socket receiving timeout");
      close(s);
      vTaskDelay(4000 / portTICK_PERIOD_MS);
      continue;
    }
    ESP_LOGI(TAG, "... set socket receiving timeout success");

    /* Read HTTP response */
    do {
      bzero(recv_buf, sizeof(recv_buf));
      r = read(s, recv_buf, sizeof(recv_buf) - 1);
      for (int i = 0; i < r; i++) {
        putchar(recv_buf[i]);
      }
    } while (r > 0);

    ESP_LOGI(
      TAG, "... done reading from socket. Last read return=%d errno=%d\r\n", r, errno);
    close(s);
    return;
    ;
    for (int countdown = 10; countdown >= 0; countdown--) {
      ESP_LOGI(TAG, "%d... ", countdown);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "Starting again!");
  }
}
#endif

void StartWiFi()
{
  ESP_ERROR_CHECK(nvs_flash_init());
  printf("Initializing WiF\n");
  initialise_wifi();
  /* Wait for the callback to set the CONNECTED_BIT in the
     event group.
  */
  printf("Waiting for IP address\n");
  xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
  ESP_LOGI(TAG, "Connected to AP");
  const char* contents = HttpGet("http://www.google.com");
  if (!contents) {
    printf("NULL contents\n");
  } else {
    printf("---\n%s\n---\n", contents);
  }
  //  http_get_task(NULL);
  //  xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
}
