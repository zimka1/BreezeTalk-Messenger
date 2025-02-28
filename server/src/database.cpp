#include "../include/database.h"
#include "../include/globals.h"

// Initialize the database and create tables if they don't exist
void initDatabase() {
    // Connection string to connect to the PostgreSQL database
    const char* conninfo = "dbname=postgres user=zimka password=12345 hostaddr=127.0.0.1 port=5432";

    // Establish a connection to the database
    db = PQconnectdb(conninfo);

    // Check if the connection was successful
    if (PQstatus(db) != CONNECTION_OK) {
        // Print an error message if the connection failed
        std::cerr << "Connection to database failed: " << PQerrorMessage(db) << std::endl;

        // Clean up the connection object
        PQfinish(db);

        // Exit the program with an error code
        exit(1);
    }

    // Print a success message if the connection was established
    std::cout << "Connected to PostgreSQL successfully!" << std::endl;

    // SQL query to create the 'users' table if it doesn't already exist
    const char* createUsersTable = R"(
        CREATE TABLE IF NOT EXISTS users (
            id SERIAL PRIMARY KEY,              -- Unique ID for each user (auto-incremented)
            firstname VARCHAR NOT NULL,         -- User's first name (required)
            lastname VARCHAR NOT NULL,          -- User's last name (required)
            nickname VARCHAR NOT NULL,          -- User's nickname (required)
            password VARCHAR NOT NULL           -- User's password (required)
        );
    )";

    // Execute the SQL query to create the 'users' table
    PGresult* res = PQexec(db, createUsersTable);

    // Check if the query execution was successful
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        // Print an error message if the query failed
        std::cerr << "Error creating users table: " << PQerrorMessage(db) << std::endl;

        // Clean up the result object
        PQclear(res);

        // Close the database connection
        PQfinish(db);

        // Exit the program with an error code
        exit(1);
    }

    // Clean up the result object
    PQclear(res);

    // SQL query to create the 'messages' table if it doesn't already exist
    const char* createMessagesTable = R"(
        CREATE TABLE IF NOT EXISTS messages (
            id SERIAL PRIMARY KEY,              -- Unique ID for each message (auto-incremented)
            user_from INT REFERENCES users(id),  -- ID of the user who sent the message
            user_to INT REFERENCES users(id),    -- ID of the user who received the message
            hasBeenRead BOOLEAN DEFAULT FALSE,  -- Flag to indicate if the message has been read
            message TEXT NOT NULL               -- The content of the message (required)
        );
    )";

    // Execute the SQL query to create the 'messages' table
    res = PQexec(db, createMessagesTable);

    // Check if the query execution was successful
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        // Print an error message if the query failed
        std::cerr << "Error creating messages table: " << PQerrorMessage(db) << std::endl;

        // Clean up the result object
        PQclear(res);

        // Close the database connection
        PQfinish(db);

        // Exit the program with an error code
        exit(1);
    }

    // Clean up the result object
    PQclear(res);
}

// Get the last user ID from the database
int getLastUserId(PGconn* conn) {
    // SQL query to get the maximum user ID from the 'users' table
    const char* getMaxIdSQL = "SELECT MAX(id) FROM users";

    // Execute the SQL query
    PGresult* res = PQexec(conn, getMaxIdSQL);

    // Check if the query execution was successful
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        // Print an error message if the query failed
        std::cerr << "Error executing query: " << PQerrorMessage(conn) << std::endl;

        // Clean up the result object
        PQclear(res);

        // Return -1 to indicate an error
        return -1;
    }

    // Variable to store the last user ID
    int lastId = 0;

    // Check if the query returned any rows
    if (PQntuples(res) > 0) {
        // Get the value from the first row and first column
        std::string value = PQgetvalue(res, 0, 0);

        // Check if the value is not empty
        if (!value.empty()) {
            try {
                // Convert the string value to an integer
                lastId = std::stoi(value);
            } catch (const std::invalid_argument& e) {
                // Handle the case where the value cannot be converted to an integer
                std::cerr << "Error: Invalid argument in stoi: " << e.what() << std::endl;
            } catch (const std::out_of_range& e) {
                // Handle the case where the value is out of the range of an integer
                std::cerr << "Error: Out of range in stoi: " << e.what() << std::endl;
            }
        }
    }

    // Clean up the result object
    PQclear(res);

    // Return the last user ID (or 0 if no users exist)
    return lastId;
}

// Print the contents of the 'users' table
void printUsersTable(PGconn* conn) {
    // SQL query to select all rows from the 'users' table
    const char* sql = "SELECT * FROM users";

    // Execute the SQL query
    PGresult* res = PQexec(conn, sql);

    // Check if the query execution was successful
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        // Print an error message if the query failed
        std::cerr << "Error executing query: " << PQerrorMessage(conn) << std::endl;

        // Clean up the result object
        PQclear(res);

        // Exit the function
        return;
    }

    // Get the number of rows and columns in the result
    int rows = PQntuples(res);
    int cols = PQnfields(res);

    // Loop through each row and print the values
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            // Get the column name and value
            const char* colName = PQfname(res, j);
            const char* colValue = PQgetvalue(res, i, j);

            // Print the column name and value (or "NULL" if the value is null)
            std::cout << colName << ": " << (colValue ? colValue : "NULL") << " | ";
        }
        // Move to the next line after printing all columns of a row
        std::cout << std::endl;
    }

    // Clean up the result object
    PQclear(res);
}