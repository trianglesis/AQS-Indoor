#include <stdio.h>
#include "sqlite_driver.h"

static const char *TAG = "sqlite";

MessageBufferHandle_t xMessageBufferQuery;


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


void check_or_create_tables_test(void *pvParameters) {
    ESP_LOGI(TAG, "Check of create 'test' table!");
	// Open database
	char db_name[32];
	snprintf(db_name, sizeof(db_name)-1, "%s/stats.db", SD_MOUNT_POINT);
	sqlite3 *db;
	sqlite3_initialize();
	if (db_open(db_name, &db)) vTaskDelete(NULL);

	// Inquiry
	int rc = db_query(xMessageBufferQuery, db, "select count(*) from sqlite_master where name = 'test';");
	if (rc != SQLITE_OK) vTaskDelete(NULL);

	// Read reply
	char sqlmsg[256];
	size_t readBytes;
	readBytes = xMessageBufferReceive(xMessageBufferQuery, sqlmsg, sizeof(sqlmsg), 100);
	ESP_LOGI(pcTaskGetName(NULL), "readBytes=%d", readBytes);
	if (readBytes == 0) vTaskDelete(NULL);
	sqlmsg[readBytes] = 0;
	ESP_LOGI(pcTaskGetName(NULL), "sqlmsg=[%s]", sqlmsg);

	// Create table
	if (strcmp(sqlmsg, "count(*) = 0") == 0) {
		int rc = db_query(xMessageBufferQuery, db, "CREATE TABLE test (id INTEGER, content);");
		if (rc != SQLITE_OK) vTaskDelete(NULL);
		ESP_LOGW(pcTaskGetName(NULL), "Table created");
	} else {
		ESP_LOGW(pcTaskGetName(NULL), "Table already exists");
	}

	// Inquiry
	rc = db_query(xMessageBufferQuery, db, "select count(*) from test;");
	if (rc != SQLITE_OK) vTaskDelete(NULL);

	// Read reply
	readBytes = xMessageBufferReceive(xMessageBufferQuery, sqlmsg, sizeof(sqlmsg), 100);
	ESP_LOGI(pcTaskGetName(NULL), "readBytes=%d", readBytes);
	if (readBytes == 0) vTaskDelete(NULL);
	sqlmsg[readBytes] = 0;
	ESP_LOGI(pcTaskGetName(NULL), "sqlmsg=[%s]", sqlmsg);

	// Insert record
	if (strcmp(sqlmsg, "count(*) = 0") == 0) {
		rc = db_query(xMessageBufferQuery, db, "INSERT INTO test VALUES (1, 'Hello, World');");
		if (rc != SQLITE_OK) vTaskDelete(NULL);
		ESP_LOGW(pcTaskGetName(NULL), "Record inserted");
	} else {
		ESP_LOGW(pcTaskGetName(NULL), "Record already exists");
	}

	rc = db_query(xMessageBufferQuery, db, "SELECT * FROM test");
	if (rc != SQLITE_OK) vTaskDelete(NULL);
	while (1) {
		readBytes = xMessageBufferReceive(xMessageBufferQuery, sqlmsg, sizeof(sqlmsg), 100);
		ESP_LOGI(pcTaskGetName(NULL), "readBytes=%d", readBytes);
		if (readBytes == 0) break;
		sqlmsg[readBytes] = 0;
		ESP_LOGI(pcTaskGetName(NULL), "sqlmsg=[%s]", sqlmsg);
	}

	sqlite3_close(db);
	printf("All Done\n");
	vTaskDelete(NULL);
}

/*
Battery
*/
#define SQL(...) #__VA_ARGS__
const char *battery_table_create_sql = SQL(
    CREATE TABLE "battery_stats" (
        "id"                    INTEGER,
        "adc_raw"               INTEGER,
        "voltage"               INTEGER,
        "voltage_m"             INTEGER,
        "percentage"            INTEGER,
        "max_masured_voltage"   INTEGER,
        "measure_freq"          INTEGER,
        "measure_loop_count"    INTEGER,
        PRIMARY KEY("id" AUTOINCREMENT)
    );
);


void check_or_create_table_battery(void *pvParameters) {
    ESP_LOGI(TAG, "Check battery stats table!");
    // Open database
    char db_name[32];
    snprintf(db_name, sizeof(db_name)-1, "%s/stats.db", SD_MOUNT_POINT);
    sqlite3 *db;
    sqlite3_initialize();
    int rc = db_open(db_name, &db); // will print "Opened database successfully"
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "Cannot open database: %s, resp: %d", db_name, rc);
        vTaskDelete(NULL);
    } else {
        ESP_LOGI(TAG, "Opened database: %s, resp: %d", db_name, rc);
    }

    // Inquiry
    rc = db_query(xMessageBufferQuery, db, "select count(*) from sqlite_master where name = 'battery_stats';");
    if (rc != SQLITE_OK) {
        ESP_LOGW(TAG, "Select count from 'sqlite_master' cannot be executed. %s", db_name);
        vTaskDelete(NULL);
    }

    // Read reply
    char sqlmsg[256];
    size_t readBytes;
    readBytes = xMessageBufferReceive(xMessageBufferQuery, sqlmsg, sizeof(sqlmsg), 100);
    ESP_LOGI(TAG, "readBytes=%d", readBytes);
    if (readBytes == 0) {
        ESP_LOGW(TAG, "Query response is empty. %s", db_name);
        vTaskDelete(NULL);
    }
    sqlmsg[readBytes] = 0;
    ESP_LOGI(TAG, "sqlmsg=[%s]", sqlmsg);

    // Create table
    if (strcmp(sqlmsg, "count(*) = 0") == 0) {
        int rc = db_query(xMessageBufferQuery, db, battery_table_create_sql);
        if (rc != SQLITE_OK) {
            ESP_LOGI(TAG, "Table 'battery_stats' cannot be created: %s", db_name);
            vTaskDelete(NULL);
        } else {
            ESP_LOGW(TAG, "Table 'battery_stats' created: %s", db_name);
        }
    } else {
        ESP_LOGI(TAG, "Table 'battery_stats' already exists at: %s", db_name);
    }

    // Inquiry
    rc = db_query(xMessageBufferQuery, db, "select count(*) from battery_stats;");
    if (rc != SQLITE_OK) {
        ESP_LOGW(TAG, "Select count from 'battery_stats' cannot be executed at: %s", db_name);
        vTaskDelete(NULL);
    }

    // Read reply
    readBytes = xMessageBufferReceive(xMessageBufferQuery, sqlmsg, sizeof(sqlmsg), 100);
    ESP_LOGI(TAG, "readBytes=%d", readBytes);
    if (readBytes == 0) {
        ESP_LOGW(TAG, "Query response is empty at: %s", db_name);
        vTaskDelete(NULL);
    }
    sqlmsg[readBytes] = 0;
    ESP_LOGI(TAG, "sqlmsg=[%s]", sqlmsg);

    sqlite3_close(db);
    ESP_LOGI(TAG, "SQL routine ended, DB is closed: %s", db_name);
    vTaskDelete(NULL);
}

/*
    int adc_raw, 
    int voltage, 
    int voltage_m, 
    int percentage, 
    int max_masured_voltage, 
    int measure_freq, 
    int loop_count
*/
void battery_stats(struct BattSensor battery_readings) {
    ESP_LOGI(TAG, "Insert into 'battery_stats' table!");

    // Open database
    char db_name[32];
    snprintf(db_name, sizeof(db_name)-1, "%s/stats.db", SD_MOUNT_POINT);
    sqlite3 *db;
    sqlite3_initialize();
    int rc = db_open(db_name, &db); // will print "Opened database successfully"
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "Cannot open database: %s, resp: %d", db_name, rc);
        // return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Opened database: %s, resp: %d", db_name, rc);
    }

    char battery_table_insert_sql[256];
    snprintf(battery_table_insert_sql, sizeof(battery_table_insert_sql), "INSERT INTO battery_stats VALUES (%d, %d, %d, %d, %d, %d, %d);", battery_readings.adc_raw, battery_readings.voltage, battery_readings.voltage_m, battery_readings.percentage, battery_readings.max_masured_voltage, battery_readings.measure_freq, battery_readings.loop_count);

    // Insert record
    rc = db_query(xMessageBufferQuery, db, battery_table_insert_sql);
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "Cannot insert into 'battery_stats' table at %s", db_name);
        // return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Record inserted into 'battery_stats' table at %s", db_name);
    }
    sqlite3_close(db);
    ESP_LOGI(TAG, "SQL routine ended, DB is closed: %s", db_name);
    // return ESP_OK;
}


esp_err_t setup_db(void) {
    sqlite_info();
    // Compose DB name and pointer
    char db_name[32];
    snprintf(db_name, sizeof(db_name)-1, "%s/stats.db", SD_MOUNT_POINT);
    sqlite3_initialize();
    ESP_LOGI(TAG, "Database setup finished!");
    
    // Create Message Buffer
	// Create Message Buffer
	xMessageBufferQuery = xMessageBufferCreate(4096);
	configASSERT( xMessageBufferQuery );
    if( xMessageBufferQuery == NULL ) {
        ESP_LOGE(TAG, "Cannot create a message buffer for SQL operations!");
    } else {
        ESP_LOGI(TAG, "Created a message buffer for SQL operations ok.");
    }
    

    /*
    Check or create tables if absent:
    - Test table
    - Battery stats table
    */
    xTaskCreatePinnedToCore(check_or_create_tables_test, "test-table", 1024*6, NULL, 4, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(check_or_create_table_battery, "battery-table", 1024*6, NULL, 4, NULL, tskNO_AFFINITY);
    return ESP_OK;
}