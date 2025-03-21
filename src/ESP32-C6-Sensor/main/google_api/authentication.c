#include "authentication.h"
#include "cJSON.h"
#include "mbedtls/base64.h"
#include "mbedtls/md.h"
#include "mbedtls/pk.h"
#include "mbedtls/sha256.h"
#include "mbedtls/entropy.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_mac.h"          //needed for http
#include "lwip/sockets.h"     //needed for http
#include "mbedtls/ctr_drbg.h" //needed for mbedtls_ctr_drbg_random
#include "google_api.h"
#include "keys.h"          //email and private key
#include "freertos/task.h" //task delay
#include "time.h"          //time

extern const uint8_t server_cert_pem_start[] asm("_binary_server_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_server_cert_pem_end");

static const char *TAG = "api_authentication";
time_t last_access_token_time = 0;

char *create_jwt(const char *client_email, const char *private_key)
{
    // Create JWT header
    const char *header = "{\"alg\":\"RS256\",\"typ\":\"JWT\"}";

    // Create JWT payload
    last_access_token_time = time(NULL);
    char payload[512];
    snprintf(payload, sizeof(payload),
             "{\"iss\":\"%s\",\"sub\":\"%s\",\"aud\":\"https://oauth2.googleapis.com/token\",\"iat\":%lld,\"exp\":%lld,\"scope\":\"https://www.googleapis.com/auth/spreadsheets\"}",
             client_email, client_email, last_access_token_time, last_access_token_time + 3600); // valid 3600 s

    // Base64 encode header and payload
    char header_base64[256];
    char payload_base64[512];
    size_t header_base64_len, payload_base64_len;
    mbedtls_base64_encode((unsigned char *)header_base64, sizeof(header_base64), &header_base64_len, (const unsigned char *)header, strlen(header));
    mbedtls_base64_encode((unsigned char *)payload_base64, sizeof(payload_base64), &payload_base64_len, (const unsigned char *)payload, strlen(payload));

    // Concatenate header and payload
    char jwt[1024];
    snprintf(jwt, sizeof(jwt), "%s.%s", header_base64, payload_base64);

    // Sign JWT
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);

    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);

    int ret = mbedtls_pk_parse_key(&pk, (const unsigned char *)private_key, strlen(private_key) + 1, NULL, 0, mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret != 0)
    {
        ESP_LOGE(TAG, "Failed to parse private key: -0x%04X", -ret);
        mbedtls_pk_free(&pk);
        return NULL;
    }

    unsigned char hash[32];
    mbedtls_sha256((const unsigned char *)jwt, strlen(jwt), hash, 0);

    unsigned char signature[256];
    size_t signature_len;
    mbedtls_pk_sign(&pk, MBEDTLS_MD_SHA256, hash, sizeof(hash), signature, sizeof(signature), &signature_len, mbedtls_ctr_drbg_random, &ctr_drbg);

    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    // Base64 encode signature
    char signature_base64[512];
    size_t signature_base64_len;
    mbedtls_base64_encode((unsigned char *)signature_base64, sizeof(signature_base64), &signature_base64_len, signature, signature_len);

    // Concatenate JWT and signature
    char *jwt_signed = malloc(strlen(jwt) + signature_base64_len + 2);
    snprintf(jwt_signed, strlen(jwt) + signature_base64_len + 2, "%s.%s", jwt, signature_base64);

    mbedtls_pk_free(&pk);
    ESP_LOGI(TAG, "JWT generated sucessfully %s", jwt_signed);

    return jwt_signed;
}

char *exchange_jwt_for_access_token(const char *jwt)
{
    char *access_token = NULL;

    http_response_t response = {0};

    esp_http_client_config_t config = {
        .url = "https://oauth2.googleapis.com/token",
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler,
        .cert_pem = (const char *)server_cert_pem_start, // Set the server certificate
        .buffer_size = 2 * 1024,                         // increase buffer size
        .buffer_size_tx = 2 * 1024,
        .user_data = &response,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "grant_type", "urn:ietf:params:oauth:grant-type:jwt-bearer");
    cJSON_AddStringToObject(root, "assertion", jwt);
    char *post_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "%s", response.buffer);
        if (esp_http_client_get_status_code(client) == 200)
        {

            cJSON *response_json = cJSON_Parse(response.buffer);
            if (response_json)
            {
                cJSON *access_token_json = cJSON_GetObjectItem(response_json, "access_token");
                if (access_token_json)
                {
                    access_token = strdup(access_token_json->valuestring);
                }
                cJSON_Delete(response_json);
            }
            free(response.buffer);
        }
    }
    else
    {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        access_token = strdup("error");
    }

    esp_http_client_cleanup(client);

    return access_token;
}

char *generate_access_token()
{

    char *jwt = create_jwt(CLIENT_EMAIL, PRIVATE_KEY);
    char *access_token = exchange_jwt_for_access_token(jwt);
    free(jwt);

    return access_token;
}

void checkAccessTokenValidity(char *access_token)
{
    if (time(NULL) - last_access_token_time < 3400)
    {
        return;
    }
    if (access_token != NULL)
    {
        free(access_token);
        access_token = NULL;
    }
    access_token = generate_access_token();
}
