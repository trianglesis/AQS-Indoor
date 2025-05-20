#include <stdio.h>
#include "sqlite_driver.h"

static const char *TAG = "sqlite";

MessageBufferHandle_t xMessageBufferQuery;
static SemaphoreHandle_t sql_done;

/*
Battery
*/
#define SQL(...) #__VA_ARGS__
const char *battery_table_create = SQL(
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
const char *co2_table_create = SQL(
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
const char *bme680_table_create = SQL(
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

void select_co2_stats(int limit, int offset) {
    char db_name[32];
    snprintf(db_name, sizeof(db_name)-1, "%s/stats.db", DB_ROOT);
    sqlite3 *db;
    sqlite3_initialize();
    int rc = db_open(db_name, &db); // will print "Opened database successfully"
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "DB SELECT Cannot open database");
    }
    char table_sql[128];
    snprintf(table_sql, sizeof(table_sql) + 1, "SELECT * FROM co2_stats ORDER BY rowid DESC LIMIT %d OFFSET %d;", limit, offset);
    rc = db_exec(db, table_sql);
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "DB SELECT, failed: \n%s\n", table_sql);
    }

    // Convert results into JSON, return by Webserver API?

}

/*
Create multiple tabled in one go.
Already initialized.
Open - create tables - close
Iterate over array of tables: 0 is always the test table.

Optimizations and lower footprint hints:
- https://www.sqlite.org/withoutrowid.html
- https://www.sqlite.org/pragma.html#pragma_page_size

*/
void table_check_tsk(void *arg) {
    // Tables to create
    char tables[][16] = { 
        "test_table", 
        "battery_stats", 
        "air_temp_stats",
        "co2_stats"
    };
    
    char db_name[32];
    snprintf(db_name, sizeof(db_name)-1, "%s/stats.db", DB_ROOT);
    // Open database
    sqlite3 *db;
    int rc = db_open(db_name, &db); // will print "Opened database successfully"
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "Cannot open database: %s resp: %d", db_name, rc);
        vTaskDelete(NULL);
    } else {
        ESP_LOGI(TAG, "Opened database: %s resp: %d", db_name, rc);
    }

    // Set page size, read page size: "PRAGMA page_size;"
    // Inquiry
    rc = db_query(xMessageBufferQuery, db, "PRAGMA page_size=512; VACUUM;");
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "Set PRAGMA page_size FAILED!");
    }

    for (size_t i = 0; i < sizeof(tables) / sizeof(tables[0]); i++) {
        ESP_LOGI(TAG, "%d Check table existence:\n\tDB\t%s\n\tTable\t%s", i, db_name, tables[i]);

        char create_table_sql[256];
        if (i == 0) {
            snprintf(create_table_sql, sizeof(create_table_sql)-1, "%s", "CREATE TABLE test (id INTEGER, content);");
        } else if (i == 1) {
            snprintf(create_table_sql, sizeof(create_table_sql)-1, "%s", battery_table_create);
        } else if (i == 2) {
            snprintf(create_table_sql, sizeof(create_table_sql)-1, "%s", bme680_table_create);
        } else if (i == 3) {
            snprintf(create_table_sql, sizeof(create_table_sql)-1, "%s", co2_table_create);
        } else {
            ESP_LOGW(TAG, "No such table to create: %s", tables[i]);
        }

        // Inquiry
        char table_name_sql[96];
        snprintf(table_name_sql, sizeof(table_name_sql)-1, "select count(*) from sqlite_master where name = '%s';", tables[i]);
        rc = db_query(xMessageBufferQuery, db, table_name_sql);
        if (rc != SQLITE_OK) {
            ESP_LOGE(TAG, "SELECT count from 'sqlite_master' FAILED!\n\tTable\t%s", tables[i]);
            continue;
        }

        // Read reply
        char sqlmsg[256];
        size_t readBytes;
        readBytes = xMessageBufferReceive(xMessageBufferQuery, sqlmsg, sizeof(sqlmsg), 100);
        ESP_LOGI(TAG, "%d readBytes=%d", i, readBytes);
        if (readBytes == 0) {
            ESP_LOGE(TAG, "SELECT query is EMPTY!\n\tTable\t%s", tables[i]);
            continue;
        }
        sqlmsg[readBytes] = 0;
        ESP_LOGI(TAG, "%d sqlmsg=[%s]", i, sqlmsg);

        // Create table
        if (strcmp(sqlmsg, "count(*) = 0") == 0) {
            int rc = db_query(xMessageBufferQuery, db, create_table_sql);
            if (rc != SQLITE_OK) {
                ESP_LOGE(TAG, "%d Table cannot be created: FAIL!\n\tTable\t%s", i, tables[i]);
                continue;
            } else {
                ESP_LOGI(TAG, "%d Table created: OK!\n\tTable\t%s", i, tables[i]);
            }
        } else {
            ESP_LOGI(TAG, "%d Table already exists, OK!\n\tTable\t%s", i, tables[i]);
        }

        // Inquiry
        char select_count_sql[96];
        snprintf(select_count_sql, sizeof(select_count_sql)-1, "select count(*) from %s;", tables[i]);
        rc = db_query(xMessageBufferQuery, db, select_count_sql);
        if (rc != SQLITE_OK) {
            ESP_LOGE(TAG, "%d Select from the table FAILED!\n\tTable\t%s", i, tables[i]);
            continue;
        }

        // Read reply
        readBytes = xMessageBufferReceive(xMessageBufferQuery, sqlmsg, sizeof(sqlmsg), 100);
        ESP_LOGI(TAG, "%d readBytes=%d", i, readBytes);
        if (readBytes == 0) {
            ESP_LOGE(TAG, "%d Select from the table EMPTY response: FAILED!\n\tTable\t%s", i, tables[i]);
            vTaskDelete(NULL);
        }
        sqlmsg[readBytes] = 0;
        ESP_LOGI(TAG, "%d sqlmsg=[%s]", i, sqlmsg);

        int record_count = 0;
        if (strncmp(sqlmsg, "count(*) =", 10) == 0) {
            record_count = atoi(&sqlmsg[10]);
        } else {
            ESP_LOGE(TAG, "%d illegal reply\n\tTable\t%s", i, tables[i]);
        }

        if (record_count == 0) {
            ESP_LOGI(TAG, "%d Table is empty record_count=%d\n\tTable\t%s", i, record_count, tables[i]);
        } else if (record_count >= 3000) {
            // Add routine to delete older records after a few thousands collected
            ESP_LOGW(TAG, "%d Table is HEAVY, clean old recods! record_count=%d\n\tTable\t%s", i, record_count, tables[i]);
        } else {
            ESP_LOGI(TAG, "%d Table is not empty but ok record_count=%d\n\tTable\t%s", i, record_count, tables[i]);
        }

    } // FOR
    
    // Close and clean
    sqlite3_close(db);
    xSemaphoreGive( sql_done );  // Release in task after finishing the job
    vTaskDelete(NULL);
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
    // unlink(db_name);
    sqlite3_initialize();  // Do not init again in task!
    
    // Create Message Buffer
	xMessageBufferQuery = xMessageBufferCreate(4096);
	configASSERT( xMessageBufferQuery );
    if( xMessageBufferQuery == NULL ) {
        ESP_LOGE(TAG, "Cannot create a message buffer for SQL operations!");
    }

    sql_done = xSemaphoreCreateBinary();
    TaskHandle_t xHandle;
    xTaskCreatePinnedToCore(table_check_tsk, "check-tables", 1024*6, NULL, 5, &xHandle, tskNO_AFFINITY);

    //Wait for completion in task
    xSemaphoreTake(sql_done, portMAX_DELAY);
    // Cleanup
    sqlite3_shutdown();  // close
    vSemaphoreDelete(sql_done);

    // DEBUG and TEST:
    // Select CO2 values once
    select_co2_stats(10, 1);


    return ESP_OK;
}