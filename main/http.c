#include "main.h"

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
    char *query = NULL;
    size_t query_len;
    char param[64];
    int ch;

    if(!buf){
        Printf("malloc error\n");
        goto err;
    }
    
    query_len = httpd_req_get_url_query_len(req) + 1;
    if (query_len < 1) {
        Printf("query_len error\n");
        goto err;
    }
    
    query = malloc(query_len);
    if (!query) {
        Printf("malloc error\n");
        goto err;
    }

    if (httpd_req_get_url_query_str(req, query, query_len) != ESP_OK) {
        Printf("get query error\n");
        goto err;
    }

    if (httpd_query_key_value(query, "ch", param, sizeof(param)) != ESP_OK) {
        Printf("get ch error\n");
        goto err;
    }
    ch=atoi(param);
    if(ch<0 || ch>=ChannelMax) {
        Printf("ch:%d error\n",ch);
        goto err;
    }

    int humi=get_humi(ch);
    if(humi<0){
        Printf("get_humi error\n");
        goto err;
    }

    len+=Snprintf(buf+len, buf_size-len, "{\"state_humi\":%d.%d",humi/10,humi%10);
    len+=Snprintf(buf+len, buf_size-len, ",\"name\":\"%s\"",w.store[ch].name);
    len+=Snprintf(buf+len, buf_size-len, ",\"state_last\":%u",w.store[ch].last_time);
    len+=Snprintf(buf+len, buf_size-len, ",\"pump_flow\":%d",w.store[ch].pump_flow);
    len+=Snprintf(buf+len, buf_size-len, ",\"pump_time\":%d",w.store[ch].pump_time);
    len+=Snprintf(buf+len, buf_size-len, ",\"plan_mode\":%d",w.store[ch].plan_mode);
    len+=Snprintf(buf+len, buf_size-len, ",\"plan_humi\":%d",w.store[ch].plan_humi);

    n_tmp=0;
    memset(&tmp, 0, sizeof(tmp));
    for(int i=0;i<7;i++){
        if(w.store[ch].plan_week&(1<<i)){
            n_tmp+=Snprintf(tmp+n_tmp, sizeof(tmp)-n_tmp, "%d,", i);
        }
    }
    len+=Snprintf(buf+len, buf_size-len, ",\"plan_week\":\"%s\"",tmp);

    n_tmp=0;
    memset(&tmp, 0, sizeof(tmp));
    for(int i=0;i<24;i++){
        if(w.store[ch].plan_time&(1<<i)){
            n_tmp+=Snprintf(tmp+n_tmp, sizeof(tmp)-n_tmp, "%d,", i);
        }
    }
    len+=Snprintf(buf+len, buf_size-len, ",\"plan_time\":\"%s\"",tmp);

    
    len+=Snprintf(buf+len, buf_size-len, ",\"plan_interval\":%d",w.store[ch].plan_interval);
    len+=Snprintf(buf+len, buf_size-len, ",\"interval_unit\":%d",w.store[ch].interval_unit);
    
    len+=Snprintf(buf+len, buf_size-len, "}");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buf, len);
    free(buf);
    free(query);
    return ESP_OK;
    
 err:
    if (buf) {
        free(buf);
    }
    if (query) {
        free(query);
    }
    httpd_resp_send(req, "{\"ret\":-1,\"msg\":\"error\"}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t set_handler(httpd_req_t *req)
{
    char*  query = NULL;
    size_t query_len;
    store_t store;
    char param[64];
    char dest[64];
    char *arr[24];
    int n_arr;
    int ch;

    memset(&store,0,sizeof(store));

    query_len = httpd_req_get_url_query_len(req) + 1;
    if (query_len < 1) {
        Printf("query_len error\n");
        goto err;
    }
    
    query = malloc(query_len);
    if (!query) {
        Printf("malloc error\n");
        goto err;
    }
    
    if (httpd_req_get_url_query_str(req, query, query_len) != ESP_OK) {
        Printf("get query error\n");
        goto err;
    }

    if (httpd_query_key_value(query, "ch", param, sizeof(param)) != ESP_OK) {
        Printf("get ch error\n");
        goto err;
    }
    ch=atoi(param);
    if(ch<0 || ch>=ChannelMax) {
        Printf("ch:%d error\n",ch);
        goto err;
    }

    if (httpd_query_key_value(query, "name", param, sizeof(param)) != ESP_OK) {
        Printf("get name error\n");
        goto err;
    }
    urlDecode(param, dest);
    strncpy(store.name,dest,sizeof(store.name));

    if (httpd_query_key_value(query, "pump_flow", param, sizeof(param)) != ESP_OK) {
        Printf("get pump_flow error\n");
        goto err;
    }
    store.pump_flow=atoi(param);

    if (httpd_query_key_value(query, "pump_time", param, sizeof(param)) != ESP_OK) {
        Printf("get pump_time error\n");
        goto err;
    }
    store.pump_time=atoi(param);

    if (httpd_query_key_value(query, "plan_mode", param, sizeof(param)) != ESP_OK) {
        Printf("get plan_mode error\n");
        goto err;
    }
    store.plan_mode=atoi(param);
    
    if (httpd_query_key_value(query, "plan_humi", param, sizeof(param)) != ESP_OK) {
        Printf("get plan_humi error\n");
        goto err;
    }
    store.plan_humi=atoi(param);

    if (httpd_query_key_value(query, "plan_week", param, sizeof(param)) != ESP_OK) {
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

    if (httpd_query_key_value(query, "plan_time", param, sizeof(param)) != ESP_OK) {
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

    if (httpd_query_key_value(query, "plan_interval", param, sizeof(param)) != ESP_OK) {
        Printf("get plan_interval error\n");
        goto err;
    }
    store.plan_interval=atoi(param);

    if (httpd_query_key_value(query, "interval_unit", param, sizeof(param)) != ESP_OK) {
        Printf("get interval_unit error\n");
        goto err;
    }
    store.interval_unit=atoi(param);

    Printf("ch:%d pump_flow:%d pump_time:%d plan_mode:%d plan_humi:%d"
        " plan_week:%d plan_time:%d plan_interval:%d interval_unit:%d\n"
        ,ch,store.pump_flow,store.pump_time,store.plan_mode,store.plan_humi
        ,store.plan_week,store.plan_time,store.plan_interval,store.interval_unit);
    
    if(!store_check(&store)){
        Printf("store_check error\n");
        goto err;
    }

    Printf("set ok\n");
    store.last_time=w.store[ch].last_time;
    w.store[ch]=store;
    save();

    free(query);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ret\":0}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
    
err:
    if (query) {
        free(query);
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ret\":-1,\"msg\":\"error\"}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t run_handler(httpd_req_t *req)
{
    char *query = NULL;
    size_t query_len;
    char param[64];
    int ch;

    query_len = httpd_req_get_url_query_len(req) + 1;
    if (query_len < 1) {
        Printf("query_len error\n");
        goto err;
    }
    
    query = malloc(query_len);
    if (!query) {
        Printf("malloc error\n");
        goto err;
    }
    
    if (httpd_req_get_url_query_str(req, query, query_len) != ESP_OK) {
        Printf("get query error\n");
        goto err;
    }

    if (httpd_query_key_value(query, "ch", param, sizeof(param)) != ESP_OK) {
        Printf("get ch error\n");
        goto err;
    }
    ch=atoi(param);
    if(ch<0 || ch>=ChannelMax) {
        Printf("ch:%d error\n",ch);
        goto err;
    }

    motor_run(ch);
    
    free(query);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ret\":0}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
    
err:
    if (query) {
        free(query);
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ret\":-1,\"msg\":\"error\"}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t sys_data_handler(httpd_req_t *req)
{
    int len = 0;
    int buf_size=1024;
    char *buf = malloc(buf_size);

    if(!buf){
        Printf("malloc error\n");
        goto err;
    }

    len+=Snprintf(buf+len, buf_size-len, "{\"sys_time\":\"%s\"",getTimeStr());
    len+=Snprintf(buf+len, buf_size-len, ",\"run_time\":%d",getRunTime());
    len+=Snprintf(buf+len, buf_size-len, ",\"have_water\":%d",have_water());
    len+=Snprintf(buf+len, buf_size-len, ",\"net_state\":%d",w.net_state);
    len+=Snprintf(buf+len, buf_size-len, ",\"ssid\":\"%s\"",w.wlan.ssid);
    len+=Snprintf(buf+len, buf_size-len, ",\"pwd\":\"%s\"",w.wlan.pwd);
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

static esp_err_t sys_set_handler(httpd_req_t *req)
{
    char *query = NULL;
    size_t query_len;
    char param[64];
    char dest[64];

    query_len = httpd_req_get_url_query_len(req) + 1;
    if (query_len < 1) {
        Printf("query_len error\n");
        goto err;
    }
    
    query = malloc(query_len);
    if (!query) {
        Printf("malloc error\n");
        goto err;
    }
    
    if (httpd_req_get_url_query_str(req, query, query_len) != ESP_OK) {
        Printf("get query error\n");
        goto err;
    }

    int mod=0;
    if (httpd_query_key_value(query, "ssid", param, sizeof(param)) != ESP_OK) {
        Printf("get ssid error\n");
        goto err;
    }
    int len=strlen(param);
    if(!len || len>=sizeof(w.wlan.ssid)){
        Printf("ssid len:%d error\n",len);
        goto err;
    }
    urlDecode(param, dest);
    if(strcmp(w.wlan.ssid,dest)){
        strncpy(w.wlan.ssid,dest,sizeof(w.wlan.ssid));
        mod=1;
    }

    if (httpd_query_key_value(query, "pwd", param, sizeof(param)) != ESP_OK) {
        Printf("get pwd error\n");
        goto err;
    }
    len=strlen(param);
    if(len && (len<8 || len>=sizeof(w.wlan.pwd))){
        Printf("pwd len:%d error\n",len);
        goto err;
    }
    if(strcmp(w.wlan.pwd,param)){
        strncpy(w.wlan.pwd,param,sizeof(w.wlan.pwd));
        mod=1;
    }

    if(mod){
        wlanInitSta();
        nvram_set_data("wlan", &w.wlan, sizeof(w.wlan));
    }
    
    free(query);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ret\":0}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
    
err:
    if (query) {
        free(query);
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"ret\":-1,\"msg\":\"error\"}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


http_file_data_t *find_http_file(char *uri)
{
    for(int i=0;i<http_file_num;i++){
        if(!strcmp(http_files[i]->uri,uri)){
            return http_files[i]->user_ctx;
        }
    }
    return NULL;
}

int suffix(char *str, char *str2)
{
    int len=strlen(str);
    int len2=strlen(str2);

    if(len<len2){
        return 0;
    }

    if(strcmp(str+(len-len2),str2)){
        return 0;
    }
    
    return 1;
}

esp_err_t html_handler(httpd_req_t *req)
{
    http_file_data_t *file=req->user_ctx;
    if(!file){
        file=find_http_file("/index.html");
    }

    if(suffix(file->uri,".js")){
        httpd_resp_set_type(req, "application/javascript");
    }else if(suffix(file->uri,".css")){
        httpd_resp_set_type(req, "text/css");
    }else{
        httpd_resp_set_type(req, "text/html");
    }
    httpd_resp_set_hdr(req, "Content-Encoding", "deflate");
    httpd_resp_send(req, (char *)file->data, file->datalen);

    return ESP_OK;
}

static const httpd_uri_t apis[] = {
    {"/", HTTP_GET, html_handler, NULL},
    {"/data", HTTP_GET, data_handler, NULL},
    {"/set", HTTP_GET, set_handler, NULL},
    {"/run", HTTP_GET, run_handler, NULL},
    {"/sys_data", HTTP_GET, sys_data_handler, NULL},
    {"/sys_set", HTTP_GET, sys_set_handler, NULL},
    {NULL, 0, NULL, NULL},
};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.max_uri_handlers = ASIZE(apis)+http_file_num;
    // Start the httpd server
    Printf("Starting server on port: '%d'\n", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        Printf("Registering URI handlers\n");
        for(int i=0;apis[i].uri;i++){
            httpd_register_uri_handler(server, &apis[i]);
        }
        for(int i=0;i<http_file_num;i++){
            httpd_register_uri_handler(server, http_files[i]);
        }
        return server;
    }

    Printf("Error starting server!\n");
    return NULL;
}

void http_init(void)
{
    w.server = start_webserver();
}

