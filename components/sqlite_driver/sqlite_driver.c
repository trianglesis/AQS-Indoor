#include <stdio.h>
#include "sqlite_driver.h"

static const char *TAG = "sqlite";

MessageBufferHandle_t xMessageBuffer;
const size_t xMessageBufferSizeBytes = 4096;

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
    ESP_LOGI(__FUNCTION__, "xMessageBuffer=[%p]", xMessageBuffer);
    char *zErrMsg = 0;
    ESP_LOGI(TAG, "QUERY: %s\n", sql);
    int rc = sqlite3_exec(db, sql, callback, xMessageBuffer, &zErrMsg);
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        ESP_LOGI(TAG, "Operation done successfully\n");
    }
    return rc;
}

esp_err_t check_or_create_tables_test(void) {
    ESP_LOGI(TAG, "Check of create 'test' table!");
    // Open database
    char db_name[32];
    snprintf(db_name, sizeof(db_name)-1, "%s/stats.db", SD_MOUNT_POINT);
    sqlite3 *db;
    sqlite3_initialize();
    int rc = db_open(db_name, &db); // will print "Opened database successfully"
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "Cannot open database: %s, resp: %d", db_name, rc);
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Opened database: %s, resp: %d", db_name, rc);
    }

    // Inquiry
    rc = db_query(xMessageBuffer, db, "select count(*) from sqlite_master where name = 'test';");
    if (rc != SQLITE_OK) {
        ESP_LOGW(TAG, "Select count from 'sqlite_master' cannot be executed. %s", db_name);
        return ESP_FAIL;
    }

    // Read reply
    char sqlmsg[256];
    size_t readBytes;
    readBytes = xMessageBufferReceive(xMessageBuffer, sqlmsg, sizeof(sqlmsg), 100);
    ESP_LOGI(TAG, "readBytes=%d", readBytes);
    if (readBytes == 0) {
        ESP_LOGW(TAG, "Query response is empty. %s", db_name);
        return ESP_FAIL;
    }
    sqlmsg[readBytes] = 0;
    ESP_LOGI(TAG, "sqlmsg=[%s]", sqlmsg);

    // Create table
    if (strcmp(sqlmsg, "count(*) = 0") == 0) {
        int rc = db_query(xMessageBuffer, db, "CREATE TABLE test (id INTEGER, content);");
        if (rc != SQLITE_OK) {
            ESP_LOGI(TAG, "Table 'test' cannot be created: %s", db_name);
            return ESP_FAIL;
        } else {
            ESP_LOGW(TAG, "Table 'test' created: %s", db_name);
        }
    } else {
        ESP_LOGI(TAG, "Table 'test' already exists at: %s", db_name);
    }

    // Inquiry
    rc = db_query(xMessageBuffer, db, "select count(*) from test;");
    if (rc != SQLITE_OK) {
        ESP_LOGW(TAG, "Select count from test cannot be executed at: %s", db_name);
        return ESP_FAIL;
    }

    // Read reply
    readBytes = xMessageBufferReceive(xMessageBuffer, sqlmsg, sizeof(sqlmsg), 100);
    ESP_LOGI(TAG, "readBytes=%d", readBytes);
    if (readBytes == 0) {
        ESP_LOGW(TAG, "Query response is empty at: %s", db_name);
        return ESP_FAIL;
    }
    sqlmsg[readBytes] = 0;
    ESP_LOGI(TAG, "sqlmsg=[%s]", sqlmsg);

    // Insert record
    if (strcmp(sqlmsg, "count(*) = 0") == 0) {
        rc = db_query(xMessageBuffer, db, "INSERT INTO test VALUES (1, 'Hello, World');");
        if (rc != SQLITE_OK) {
            ESP_LOGI(TAG, "Record inserted into: %s", db_name);
        }
    } else {
        ESP_LOGW(TAG, "Record already exists at: %s", db_name);
    }

    rc = db_query(xMessageBuffer, db, "SELECT * FROM test");
    if (rc != SQLITE_OK) {
        ESP_LOGW(TAG, "Select * from test cannot be executed at: %s", db_name);
        return ESP_FAIL;
    }
    while (1) {
        readBytes = xMessageBufferReceive(xMessageBuffer, sqlmsg, sizeof(sqlmsg), 100);
        ESP_LOGI(TAG, "readBytes=%d", readBytes);
        if (readBytes == 0) break;
        sqlmsg[readBytes] = 0;
        ESP_LOGI(TAG, "sqlmsg=[%s]", sqlmsg);
    }

    sqlite3_close(db);
    ESP_LOGI(TAG, "SQL routine ended, DB is closed: %s", db_name);
    return ESP_OK;
}

/*
Battery
*/
#define SQL(...) #__VA_ARGS__
const char *battery_table_create_sql = SQL(
    CREATE TABLE "battery_stats" (
        "id"                    INTEGER,
        "adc_raw"                INTEGER,
        "voltage"                INTEGER,
        "voltage_m"                INTEGER,
        "percentage"            INTEGER,
        "max_masured_voltage"    INTEGER,
        "measure_freq"            INTEGER,
        "measure_loop_count"    INTEGER,
        PRIMARY KEY("id" AUTOINCREMENT)
    );
);


esp_err_t check_or_create_table_battery(void) {
    ESP_LOGI(TAG, "Check battery stats table!");
    // Open database
    char db_name[32];
    snprintf(db_name, sizeof(db_name)-1, "%s/stats.db", SD_MOUNT_POINT);
    sqlite3 *db;
    sqlite3_initialize();
    int rc = db_open(db_name, &db); // will print "Opened database successfully"
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "Cannot open database: %s, resp: %d", db_name, rc);
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Opened database: %s, resp: %d", db_name, rc);
    }

    // Inquiry
    rc = db_query(xMessageBuffer, db, "select count(*) from sqlite_master where name = 'battery_stats';");
    if (rc != SQLITE_OK) {
        ESP_LOGW(TAG, "Select count from 'sqlite_master' cannot be executed. %s", db_name);
        return ESP_FAIL;
    }

    // Read reply
    char sqlmsg[256];
    size_t readBytes;
    readBytes = xMessageBufferReceive(xMessageBuffer, sqlmsg, sizeof(sqlmsg), 100);
    ESP_LOGI(TAG, "readBytes=%d", readBytes);
    if (readBytes == 0) {
        ESP_LOGW(TAG, "Query response is empty. %s", db_name);
        return ESP_FAIL;
    }
    sqlmsg[readBytes] = 0;
    ESP_LOGI(TAG, "sqlmsg=[%s]", sqlmsg);

    // Create table
    if (strcmp(sqlmsg, "count(*) = 0") == 0) {
        int rc = db_query(xMessageBuffer, db, battery_table_create_sql);
        if (rc != SQLITE_OK) {
            ESP_LOGI(TAG, "Table 'battery_stats' cannot be created: %s", db_name);
            return ESP_FAIL;
        } else {
            ESP_LOGW(TAG, "Table 'battery_stats' created: %s", db_name);
        }
    } else {
        ESP_LOGI(TAG, "Table 'battery_stats' already exists at: %s", db_name);
    }

    // Inquiry
    rc = db_query(xMessageBuffer, db, "select count(*) from battery_stats;");
    if (rc != SQLITE_OK) {
        ESP_LOGW(TAG, "Select count from 'battery_stats' cannot be executed at: %s", db_name);
        return ESP_FAIL;
    }

    // Read reply
    readBytes = xMessageBufferReceive(xMessageBuffer, sqlmsg, sizeof(sqlmsg), 100);
    ESP_LOGI(TAG, "readBytes=%d", readBytes);
    if (readBytes == 0) {
        ESP_LOGW(TAG, "Query response is empty at: %s", db_name);
        return ESP_FAIL;
    }
    sqlmsg[readBytes] = 0;
    ESP_LOGI(TAG, "sqlmsg=[%s]", sqlmsg);

    sqlite3_close(db);
    ESP_LOGI(TAG, "SQL routine ended, DB is closed: %s", db_name);
    return ESP_OK;
}

esp_err_t battery_stats(
    int adc_raw, 
    int voltage, 
    int voltage_m, 
    int percentage, 
    int max_masured_voltage, 
    int measure_freq, 
    int loop_count) {
    ESP_LOGI(TAG, "Insert into 'battery_stats' table!");

    // Open database
    char db_name[32];
    snprintf(db_name, sizeof(db_name)-1, "%s/stats.db", SD_MOUNT_POINT);
    sqlite3 *db;
    sqlite3_initialize();
    int rc = db_open(db_name, &db); // will print "Opened database successfully"
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "Cannot open database: %s, resp: %d", db_name, rc);
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Opened database: %s, resp: %d", db_name, rc);
    }

    char battery_table_insert_sql[256];
    snprintf(battery_table_insert_sql, sizeof(battery_table_insert_sql), "INSERT INTO battery_stats VALUES (%d, %d, %d, %d, %d, %d, %d);", adc_raw, voltage, voltage_m, percentage, max_masured_voltage, measure_freq, loop_count);

    // Insert record
    rc = db_query(xMessageBuffer, db, battery_table_insert_sql);
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "Cannot insert into 'battery_stats' table at %s", db_name);
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Record inserted into 'battery_stats' table at %s", db_name);
    }
    sqlite3_close(db);
    ESP_LOGI(TAG, "SQL routine ended, DB is closed: %s", db_name);
    return ESP_OK;
}


esp_err_t setup_db(void) {
    sqlite_info();
    // Compose DB name and pointer
    char db_name[32];
    snprintf(db_name, sizeof(db_name)-1, "%s/stats.db", SD_MOUNT_POINT);
    sqlite3_initialize();
    ESP_LOGI(TAG, "Database setup finished!");
    
    // Create Message Buffer
    xMessageBuffer = xMessageBufferCreate( xMessageBufferSizeBytes );
    if( xMessageBuffer == NULL ) {
        ESP_LOGE(TAG, "Cannot create a message buffer for SQL operations!");
    } else {
        ESP_LOGI(TAG, "Created a message buffer for SQL operations ok.");
    }
    // Create tables
    check_or_create_tables_test();
    return ESP_OK;
}