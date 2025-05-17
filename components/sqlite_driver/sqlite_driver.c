#include <stdio.h>
#include "sqlite_driver.h"

static const char *TAG = "sqlite";

MessageBufferHandle_t xMessageBufferQuery;

/*
Battery
*/
#define SQL(...) #__VA_ARGS__
const char *battery_table_create_sql = SQL(
    CREATE TABLE "battery_stats" (
        "adc_raw"               INTEGER,
        "voltage"               INTEGER,
        "voltage_m"             INTEGER,
        "percentage"            INTEGER,
        "max_masured_voltage"   INTEGER,
        "measure_freq"          INTEGER,
        "measure_loop_count"    INTEGER
    );
);

/*
CO2 Sensor
*/
#define SQL(...) #__VA_ARGS__
const char *co2_table_create_sql = SQL(
    CREATE TABLE "co2_stats" (
        "temperature"    INTEGER,
        "humidity"       INTEGER,
        "co2_ppm"        INTEGER,
        "measure_freq"   INTEGER
    );
);

/*
BME680 Sensor
    float temperature;
    float humidity;
    float pressure;
    float resistance;
    uint16_t air_q_index;
    int measure_freq;
*/
#define SQL(...) #__VA_ARGS__
const char *bme680_table_create_sql = SQL(
    CREATE TABLE "air_temp_stats" (
        "temperature"    INTEGER,
        "humidity"       INTEGER,
        "pressure"       INTEGER,
        "resistance"     INTEGER,
        "air_q_index"    INTEGER,
        "measure_freq"   INTEGER
    );
);

void sqlite_info(void) {
    printf("\n\n- Init:\t\tSQLite Driver debug info!\n");
    ESP_LOGI(TAG, "DB_ROOT: %s", DB_ROOT);
}

static int callback(void *data, int argc, char **argv, char **azColName) {
    MessageBufferHandle_t *xMessageBuffer = (MessageBufferHandle_t *)data;
    ESP_LOGD(__FUNCTION__, "data=[%p] xMessageBuffer=[%p]", data, xMessageBuffer);
    int i;
    char tx_buffer[128];
    for (i = 0; i<argc; i++){
        //printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
        int tx_length = sprintf(tx_buffer, "%s = %s", azColName[i], argv[i] ? argv[i] : "NULL");
        if (xMessageBuffer) {
            size_t sended = xMessageBufferSendFromISR((MessageBufferHandle_t)xMessageBuffer, tx_buffer, tx_length, NULL);
            ESP_LOGD(__FUNCTION__, "sended=%d tx_length=%d", sended, tx_length);
            if (sended != tx_length) {
                ESP_LOGE(TAG, "xMessageBufferSendFromISR fail tx_length=%d sended=%d", tx_length, sended);
            }
        } else {
            ESP_LOGE(TAG, "xMessageBuffer is NULL");
        }
    }
    //printf("\n");
    return 0;
}

int db_query(MessageBufferHandle_t xMessageBuffer, sqlite3 *db, const char *sql) {
	ESP_LOGD(__FUNCTION__, "xMessageBuffer=[%p]", xMessageBuffer);
	char *zErrMsg = 0;
	printf("%s\n", sql);
	int rc = sqlite3_exec(db, sql, callback, xMessageBuffer, &zErrMsg);
	if (rc != SQLITE_OK) {
		printf("SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	} else {
		printf("Operation done successfully\n");
	}
	return rc;
}

void check_or_create_table(void *pvParameters) {
    char *table_name = (char *)pvParameters;
    char *battery_table = "battery_stats";
    char *bme680_table = "air_temp_stats";
    char *co2_table = "co2_stats";

    // Open database
    char db_name[32];
    snprintf(db_name, sizeof(db_name)-1, "%s/stats.db", DB_ROOT);
    sqlite3 *db;
    sqlite3_initialize();
    int rc = db_open(db_name, &db); // will print "Opened database successfully"
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "Cannot open database: %s, resp: %d", db_name, rc);
        vTaskDelete(NULL);
    } else {
        ESP_LOGI(TAG, "Opened database: %s, resp: %d", db_name, rc);
    }
    ESP_LOGI(TAG, "Check table existence: %s", table_name);

    // Inquiry
    char table_name_sql[96];
    snprintf(table_name_sql, sizeof(table_name_sql)-1, "select count(*) from sqlite_master where name = '%s';", table_name);
    rc = db_query(xMessageBufferQuery, db, table_name_sql);
    if (rc != SQLITE_OK) {
        ESP_LOGW(TAG, "Select count from 'sqlite_master' cannot be executed. %s.%s", db_name, table_name);
        vTaskDelete(NULL);
    }

    // Read reply
    char sqlmsg[256];
    size_t readBytes;
    readBytes = xMessageBufferReceive(xMessageBufferQuery, sqlmsg, sizeof(sqlmsg), 100);
    ESP_LOGI(TAG, "readBytes=%d", readBytes);
    if (readBytes == 0) {
        ESP_LOGW(TAG, "Query response is empty. %s.%s", db_name, table_name);
        vTaskDelete(NULL);
    }
    sqlmsg[readBytes] = 0;
    ESP_LOGI(TAG, "sqlmsg=[%s]", sqlmsg);

    char create_table_sql[512];
    if (table_name == battery_table) {
        snprintf(create_table_sql, sizeof(create_table_sql)-1, "%s", battery_table_create_sql);
    } else if (table_name == bme680_table) {
        snprintf(create_table_sql, sizeof(create_table_sql)-1, "%s", bme680_table_create_sql);
    } else if (table_name == co2_table) {
        snprintf(create_table_sql, sizeof(create_table_sql)-1, "%s", co2_table_create_sql);
    } else {
        snprintf(create_table_sql, sizeof(create_table_sql)-1, "%s", "CREATE TABLE test (id INTEGER, content);");
    }

    // Create table
    if (strcmp(sqlmsg, "count(*) = 0") == 0) {
        int rc = db_query(xMessageBufferQuery, db, create_table_sql);
        if (rc != SQLITE_OK) {
            ESP_LOGI(TAG, "Table cannot be created: %s.%s", db_name, table_name);
            vTaskDelete(NULL);
        } else {
            ESP_LOGW(TAG, "Table created: %s.%s", db_name, table_name);
            vTaskDelete(NULL);  // No need to do anything else!
        }
    } else {
        ESP_LOGI(TAG, "Table already exists at: %s %s", db_name, table_name);
        vTaskDelete(NULL);  // No need to do anything else!
    }

    // Inquiry
    char select_count_sql[96];
    snprintf(select_count_sql, sizeof(select_count_sql)-1, "select count(*) from %s;", table_name);
    rc = db_query(xMessageBufferQuery, db, select_count_sql);
    if (rc != SQLITE_OK) {
        ESP_LOGW(TAG, "Select count from %s.%s cannot be executed!", db_name, table_name);
        vTaskDelete(NULL);
    }

    // Read reply
    readBytes = xMessageBufferReceive(xMessageBufferQuery, sqlmsg, sizeof(sqlmsg), 100);
    ESP_LOGI(TAG, "readBytes=%d", readBytes);
    if (readBytes == 0) {
        ESP_LOGW(TAG, "Query response is empty at: %s.%s", db_name, table_name);
        vTaskDelete(NULL);
    }
    sqlmsg[readBytes] = 0;
    ESP_LOGI(TAG, "sqlmsg=[%s]", sqlmsg);

    sqlite3_close(db);
    vTaskDelete(NULL);
}

/*
PRAGMA schema.page_size; 
PRAGMA schema.page_size = bytes;
*/
void database_setting(void) {
    sqlite3_mprintf("PRAGMA page_size;");
    sqlite3_mprintf("PRAGMA page_size = 512;");
}

void insert_task(void *pvParameters) {
    char *sql = (char *)pvParameters;
    // Open database
    char db_name[32];
    snprintf(db_name, sizeof(db_name)-1, "%s/stats.db", DB_ROOT);
    sqlite3 *db;
    sqlite3_initialize();
    // vTaskDelay(pdMS_TO_TICKS(500));
    int rc = db_open(db_name, &db); // will print "Opened database successfully"
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "Cannot open database: %s, resp: %d", db_name, rc);
        vTaskDelete(NULL);
    }
    // Insert record
    rc = db_query(xMessageBufferQuery, db, sql);
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "Cannot insert at %s\n%s\n", db_name, sql);
        vTaskDelete(NULL);
    }
    ESP_LOGI(TAG, "SQL routine ended, DB is closed: %s", db_name);
    sqlite3_close(db);
    vTaskDelete(NULL);
}

void ins_task(void *pvParameters) {
    char *sql = (char *)pvParameters;
    // Open database
    char db_name[32];
    snprintf(db_name, sizeof(db_name)-1, "%s/stats.db", DB_ROOT);
    sqlite3 *db;
    sqlite3_initialize();

    int rc = db_open(db_name, &db); // will print "Opened database successfully"
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "DB INSERT Cannot open database");
        vTaskDelete(NULL);
    }
    
	rc = db_exec(db, sql);
	if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "DB INSERT, cannot insert: \n%s\n", sql);
		vTaskDelete(NULL);
	}

    sqlite3_close(db);
    ESP_LOGI(TAG, "DB INSERT, DB is closed");
	vTaskDelete(NULL);
}

esp_err_t setup_db(void) {
    sqlite_info();
    // Compose DB name and pointer
    char db_name[32];
    snprintf(db_name, sizeof(db_name)-1, "%s/stats.db", DB_ROOT);
    // DELETE previous table for now, at each startup.
    unlink(db_name);
    sqlite3_initialize();
    // Create Message Buffer
	xMessageBufferQuery = xMessageBufferCreate(4096);
	configASSERT( xMessageBufferQuery );
    if( xMessageBufferQuery == NULL ) {
        ESP_LOGE(TAG, "Cannot create a message buffer for SQL operations!");
    }
    
    // xTaskCreate(check_or_create_table, "table-create1", 1024*6, (void *)test_table, 5, NULL);
    return ESP_OK;
}