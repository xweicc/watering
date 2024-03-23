#include "main.h"


typedef struct {
    char    *username;
    char    *password;
} basic_auth_info_t;

#define HTTPD_401      "401 UNAUTHORIZED"           /*!< HTTP Response 401 */

static char *http_auth_basic(const char *username, const char *password)
{
    int out;
    char *user_info = NULL;
    char *digest = NULL;
    size_t n = 0;
    asprintf(&user_info, "%s:%s", username, password);
    if (!user_info) {
        Printf("No enough memory for user information\n");
        return NULL;
    }
    esp_crypto_base64_encode(NULL, 0, &n, (const unsigned char *)user_info, strlen(user_info));

    /* 6: The length of the "Basic " string
     * n: Number of bytes for a base64 encode format
     * 1: Number of bytes for a reserved which be used to fill zero
    */
    digest = calloc(1, 6 + n + 1);
    if (digest) {
        strcpy(digest, "Basic ");
        esp_crypto_base64_encode((unsigned char *)digest + 6, n, (size_t *)&out, (const unsigned char *)user_info, strlen(user_info));
    }
    free(user_info);
    return digest;
}

/* An HTTP GET handler */
static esp_err_t basic_auth_get_handler(httpd_req_t *req)
{
    char *buf = NULL;
    size_t buf_len = 0;
    basic_auth_info_t *basic_auth_info = req->user_ctx;

    buf_len = httpd_req_get_hdr_value_len(req, "Authorization") + 1;
    if (buf_len > 1) {
        buf = calloc(1, buf_len);
        if (!buf) {
            Printf("No enough memory for basic authorization\n");
            return ESP_ERR_NO_MEM;
        }

        if (httpd_req_get_hdr_value_str(req, "Authorization", buf, buf_len) == ESP_OK) {
            Printf("Found header => Authorization: %s\n", buf);
        } else {
            Printf("No auth value received\n");
        }

        char *auth_credentials = http_auth_basic(basic_auth_info->username, basic_auth_info->password);
        if (!auth_credentials) {
            Printf("No enough memory for basic authorization credentials\n");
            free(buf);
            return ESP_ERR_NO_MEM;
        }

        if (strncmp(auth_credentials, buf, buf_len)) {
            Printf("Not authenticated\n");
            httpd_resp_set_status(req, HTTPD_401);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Connection", "keep-alive");
            httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Hello\"");
            httpd_resp_send(req, NULL, 0);
        } else {
            Printf("Authenticated!\n");
            char *basic_auth_resp = NULL;
            httpd_resp_set_status(req, HTTPD_200);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Connection", "keep-alive");
            asprintf(&basic_auth_resp, "{\"authenticated\": true,\"user\": \"%s\"}", basic_auth_info->username);
            if (!basic_auth_resp) {
                Printf("No enough memory for basic authorization response\n");
                free(auth_credentials);
                free(buf);
                return ESP_ERR_NO_MEM;
            }
            httpd_resp_send(req, basic_auth_resp, strlen(basic_auth_resp));
            free(basic_auth_resp);
        }
        free(auth_credentials);
        free(buf);
    } else {
        Printf("No auth header received\n");
        httpd_resp_set_status(req, HTTPD_401);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Connection", "keep-alive");
        httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Hello\"");
        httpd_resp_send(req, NULL, 0);
    }

    return ESP_OK;
}

static httpd_uri_t basic_auth = {
    .uri       = "/basic_auth",
    .method    = HTTP_GET,
    .handler   = basic_auth_get_handler,
};

static void httpd_register_basic_auth(httpd_handle_t server)
{
    basic_auth_info_t *basic_auth_info = calloc(1, sizeof(basic_auth_info_t));
    if (basic_auth_info) {
        basic_auth_info->username = "root";
        basic_auth_info->password = "admin";

        basic_auth.user_ctx = basic_auth_info;
        httpd_register_uri_handler(server, &basic_auth);
    }
}

int store_check(store_t *store)
{
    if(store->pump_flow>=1 && store->pump_flow<=100
        && store->pump_time>=1 && store->pump_time<=60
        && store->plan_mode<=3
        && store->plan_humi>=1 && store->plan_humi<=99
    ){
        return 1;
    }else{
        return 0;
    }
}


static esp_err_t data_handler(httpd_req_t *req)
{
    int len = 0;
    int buf_size=1024;
    char *buf = malloc(buf_size);
    char tmp[64];
    int n_tmp;

    if(!buf){
        Printf("malloc error\n");
        goto err;
    }

    int humi=get_humi();
    if(humi<0){
        Printf("get_humi error\n");
        goto err;
    }

    len+=Snprintf(buf+len, buf_size-len, "{\"state_humi\":%d.%d",humi/10,humi%10);
    len+=Snprintf(buf+len, buf_size-len, ",\"state_last\":%u",wvar.store.last_time);
    len+=Snprintf(buf+len, buf_size-len, ",\"pump_flow\":%d",wvar.store.pump_flow);
    len+=Snprintf(buf+len, buf_size-len, ",\"pump_time\":%d",wvar.store.pump_time);
    len+=Snprintf(buf+len, buf_size-len, ",\"plan_mode\":%d",wvar.store.plan_mode);
    len+=Snprintf(buf+len, buf_size-len, ",\"plan_humi\":%d",wvar.store.plan_humi);

    n_tmp=0;
    memset(&tmp, 0, sizeof(tmp));
    for(int i=0;i<7;i++){
        if(wvar.store.plan_week&(1<<i)){
            n_tmp+=Snprintf(tmp+n_tmp, sizeof(tmp)-n_tmp, "%d,", i);
        }
    }
    len+=Snprintf(buf+len, buf_size-len, ",\"plan_week\":\"%s\"",tmp);

    n_tmp=0;
    memset(&tmp, 0, sizeof(tmp));
    for(int i=0;i<24;i++){
        if(wvar.store.plan_time&(1<<i)){
            n_tmp+=Snprintf(tmp+n_tmp, sizeof(tmp)-n_tmp, "%d,", i);
        }
    }
    len+=Snprintf(buf+len, buf_size-len, ",\"plan_time\":\"%s\"",tmp);

    
    len+=Snprintf(buf+len, buf_size-len, ",\"plan_interval\":%d",wvar.store.plan_interval);
    len+=Snprintf(buf+len, buf_size-len, ",\"interval_unit\":%d",wvar.store.interval_unit);
    
    len+=Snprintf(buf+len, buf_size-len, ",\"free\":%lu",esp_get_free_heap_size());
    len+=Snprintf(buf+len, buf_size-len, "}");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buf, len);
    free(buf);
    return ESP_OK;
    
 err:
    if (buf) {
        free(buf);
    }
    httpd_resp_send(req, "{\"ret\":-1,\"msg\":\"error\"}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t set_handler(httpd_req_t *req)
{
    char*  buf = NULL;
    size_t buf_len;
    store_t store;
    char param[64];
    char *arr[24];
    int n_arr;

    memset(&store,0,sizeof(store));

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len < 1) {
        Printf("query_len error\n");
        goto err;
    }
    
    buf = malloc(buf_len);
    if (!buf) {
        Printf("malloc error\n");
        goto err;
    }
    
    if (httpd_req_get_url_query_str(req, buf, buf_len) != ESP_OK) {
        Printf("get query error\n");
        goto err;
    }

    if (httpd_query_key_value(buf, "pump_flow", param, sizeof(param)) != ESP_OK) {
        Printf("get pump_flow error\n");
        goto err;
    }
    store.pump_flow=atoi(param);

    if (httpd_query_key_value(buf, "pump_time", param, sizeof(param)) != ESP_OK) {
        Printf("get pump_time error\n");
        goto err;
    }
    store.pump_time=atoi(param);

    if (httpd_query_key_value(buf, "plan_mode", param, sizeof(param)) != ESP_OK) {
        Printf("get plan_mode error\n");
        goto err;
    }
    store.plan_mode=atoi(param);
    
    if (httpd_query_key_value(buf, "plan_humi", param, sizeof(param)) != ESP_OK) {
        Printf("get plan_humi error\n");
        goto err;
    }
    store.plan_humi=atoi(param);

    if (httpd_query_key_value(buf, "plan_week", param, sizeof(param)) != ESP_OK) {
        Printf("get plan_week error\n");
        goto err;
    }
    n_arr=split_string(param, ',', arr, ARRAY_SIZE(arr));
    for(int i=0;i<n_arr;i++){
        int n=atoi(arr[i]);
        if(n<7){
            Printf("week:%d\n",n);
            store.plan_week|=(1<<n);
        }
    }

    if (httpd_query_key_value(buf, "plan_time", param, sizeof(param)) != ESP_OK) {
        Printf("get plan_time error\n");
        goto err;
    }
    n_arr=split_string(param, ',', arr, ARRAY_SIZE(arr));
    for(int i=0;i<n_arr;i++){
        int n=atoi(arr[i]);
        if(n<24){
            Printf("time:%d\n",n);
            store.plan_time|=(1<<n);
        }
    }

    if (httpd_query_key_value(buf, "plan_interval", param, sizeof(param)) != ESP_OK) {
        Printf("get plan_interval error\n");
        goto err;
    }
    store.plan_interval=atoi(param);

    if (httpd_query_key_value(buf, "interval_unit", param, sizeof(param)) != ESP_OK) {
        Printf("get interval_unit error\n");
        goto err;
    }
    store.interval_unit=atoi(param);

    Printf("pump_flow:%d pump_time:%d plan_mode:%d plan_humi:%d"
        " plan_week:%d plan_time:%d plan_interval:%d interval_unit:%d\n"
        ,store.pump_flow,store.pump_time,store.plan_mode,store.plan_humi
        ,store.plan_week,store.plan_time,store.plan_interval,store.interval_unit);
    
    if(!store_check(&store)){
        Printf("store_check error\n");
        goto err;
    }

    Printf("set ok\n");
    store.last_time=wvar.store.last_time;
    wvar.store=store;
    save();

    free(buf);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ret\":0}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
    
err:
    if (buf) {
        free(buf);
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ret\":-1,\"msg\":\"error\"}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t run_handler(httpd_req_t *req)
{
    motor_run();
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ret\":0}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

http_file_data_t *find_http_file(char *name)
{
    for(int i=0;i<http_file_num;i++){
        if(!strcmp(http_files[i]->name,name)){
            return http_files[i];
        }
    }
    return NULL;
}


static esp_err_t html_handler(httpd_req_t *req)
{
    http_file_data_t *hf=find_http_file("index.html");

    httpd_resp_send(req, (char *)hf->data, hf->datalen);

    return ESP_OK;
}


static const httpd_uri_t data = {
    .uri       = "/data",
    .method    = HTTP_GET,
    .handler   = data_handler
};

static const httpd_uri_t set = {
    .uri       = "/set",
    .method    = HTTP_GET,
    .handler   = set_handler
};

static const httpd_uri_t run = {
    .uri       = "/run",
    .method    = HTTP_GET,
    .handler   = run_handler
};

static const httpd_uri_t html = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = html_handler
};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    Printf("Starting server on port: '%d'\n", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        Printf("Registering URI handlers\n");
        httpd_register_uri_handler(server, &data);
        httpd_register_uri_handler(server, &set);
        httpd_register_uri_handler(server, &run);
        httpd_register_uri_handler(server, &html);
        httpd_register_basic_auth(server);
        return server;
    }

    Printf("Error starting server!\n");
    return NULL;
}

void http_init(void)
{
    wvar.server = start_webserver();
}

