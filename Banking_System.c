#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void printHeader(const char *header)
{
    int len = strlen(header);
    int padding = (50 - len) / 2;
    printf("\n");
    for (int i = 0; i < padding; i++)
        printf("=");
    printf(" %s ", header);
    for (int i = 0; i < padding; i++)
        printf("=");
    printf("\n\n");
}

void printMainMenu()
{
    printHeader("Banking System Main Menu");
    printf("1. Create Account\n");
    printf("2. View Accounts\n");
    printf("3. View Transactions\n");
    printf("4. Deposit Into Account\n");
    printf("5. Withdraw From Account\n");
    printf("6. Update Account Details\n");
    printf("7. Exit\n");
    printf("\nSelect an option: ");
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    printf("Callback function called with %d columns.\n", argc); // Debug print
    NotUsed = 0; // Suppress compiler warning
    for(int i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}


void executeSQL(sqlite3 *db, const char *sql, int (*callback)(void *, int, char **, char **), const char *debugInfo)
{
    char *errMsg = 0;
    if (sqlite3_exec(db, sql, callback, 0, &errMsg) != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s | Debug Info: %s\n", errMsg, debugInfo);
        sqlite3_free(errMsg);
    }
    else
    {
        printf("Operation done successfully | Debug Info: %s\n", debugInfo);
    }
}

static int checkAccountExistsCallback(void *data, int argc, char **argv, char **azColName)
{
    *(int *)data = 1; // Set flag to indicate that a row was found
    return 0;
}

long long int generateAccountNumber(sqlite3 *db)
{
    long long int accountNumber;
    int accountExists;
    char sql[256];

    do
    {
        accountExists = 0;
        accountNumber = rand() % 9000000000LL + 1000000000LL; // Generate a 10-digit account number

        sprintf(sql, "SELECT 1 FROM accounts WHERE id = %lld", accountNumber);
        sqlite3_exec(db, sql, checkAccountExistsCallback, &accountExists, NULL);

    } while (accountExists);

    return accountNumber;
}

void createAccount(sqlite3 *db)
{
    char first_name[100], last_name[100], dob[11], address[255], trn[15];
    double initialDeposit;
    long long int accountNumber;

    printf("Enter account holder's first name: ");
    scanf("%99s", first_name);
    getchar(); // Consume the newline character
    printf("Enter account holder's last name: ");
    scanf("%99s", last_name);
    getchar(); // Consume the newline character
    printf("Enter account holder's date of birth (YYYY-MM-DD): ");
    scanf("%10s", dob);
    getchar(); // Consume the newline character
    printf("Enter account holder's address: ");
    fgets(address, sizeof(address), stdin);
    address[strcspn(address, "\n")] = 0; // Remove newline character
    printf("Enter 9-digit TRN: ");
    scanf("%14s", trn);
    printf("Enter initial deposit: ");
    scanf("%lf", &initialDeposit);

    accountNumber = generateAccountNumber(db); // Generate a unique 10-digit account number

    char sql[1024];
    sprintf(sql, "INSERT INTO accounts (id, first_name, last_name, dob, address, trn, balance) VALUES (%lld, '%s', '%s', '%s', '%s', '%s', %f);", accountNumber, first_name, last_name, dob, address, trn, initialDeposit);
    executeSQL(db, sql, callback, "Creating account");

    printf("Account created. Number: %lld, Name: %s %s, TRN: %s, Initial Deposit: %.2f\n", accountNumber, first_name, last_name, trn, initialDeposit);
}

void viewAccounts(sqlite3 *db)
{
    const char *sql = "SELECT id, first_name, last_name, dob, address, balance FROM accounts;";
    executeSQL(db, sql, callback, "Viewing accounts");
}

void viewTransactions(sqlite3 *db)
{
    // Placeholder for viewing transactions - adjust SQL as needed for your schema
    int account_id;
    printf("Enter account ID to view transactions: ");
    scanf("%d", &account_id);
    char sql[256];
    sprintf(sql, "SELECT * FROM transactions WHERE account_id = %d;", account_id);
    executeSQL(db, sql, callback, "Viewing transactions");
}

void depositIntoAccount(sqlite3 *db)
{
    long long int accountNumber;
    double depositAmount;
    char sql[1024];
    double newBalance = 0.0;

    printf("Enter account number: ");
    scanf("%lld", &accountNumber);
    printf("Enter deposit amount: ");
    scanf("%lf", &depositAmount);

    // Update account balance
    sprintf(sql, "UPDATE accounts SET balance = balance + %f WHERE id = %lld;", depositAmount, accountNumber);
    executeSQL(db, sql, callback, "Updating Account Balance");

    // Log the transaction
    sprintf(sql, "INSERT INTO transactions (account_id, type, amount) VALUES (%lld, 'Deposit', %f);", accountNumber, depositAmount);
    executeSQL(db, sql, callback, "Depositing into account");

    // Retrieve the new balance
    sprintf(sql, "SELECT balance FROM accounts WHERE id = %lld;", accountNumber);
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK)
    {
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            newBalance = sqlite3_column_double(stmt, 0);
            printf("Deposited %.2f into account %lld successfully.\n", depositAmount, accountNumber);
            printf("New balance: %.2f\n", newBalance);
        }
        else
        {
            printf("Error retrieving new balance.\n");
        }
        sqlite3_finalize(stmt);
    }
    else
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    }
}

void withdrawFromAccount(sqlite3 *db)
{
    long long int accountNumber;
    double withdrawalAmount;
    char sql[1024];
    double currentBalance = -1.0; // Initialize with an invalid balance

    printf("Enter account number: ");
    scanf("%lld", &accountNumber);
    printf("Enter withdrawal amount: ");
    scanf("%lf", &withdrawalAmount);

    if (withdrawalAmount <= 0)
    {
        printf("Invalid withdrawal amount. Please enter a positive number.\n");
        return; // Exit if the withdrawal amount is invalid
    }

    // Prepare SQL to check current balance
    sprintf(sql, "SELECT balance FROM accounts WHERE id = %lld;", accountNumber);
    sqlite3_stmt *stmt;

    // Check current balance
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK)
    {
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            currentBalance = sqlite3_column_double(stmt, 0);
            sqlite3_finalize(stmt); // Finalize the statement to prevent resource leak

            if (currentBalance >= withdrawalAmount)
            {
                // Proceed with withdrawal
                sprintf(sql, "UPDATE accounts SET balance = balance - %f WHERE id = %lld;", withdrawalAmount, accountNumber);
                executeSQL(db, sql, NULL, "Executing withdrawal");
                sprintf(sql, "INSERT INTO transactions (account_id, type, amount) VALUES (%lld, 'Withdrawal', -%f);", accountNumber, withdrawalAmount);
                executeSQL(db, sql, callback, "Withdrawing from account");

                // Print successful withdrawal message
                printf("Withdrawal of %.2f from account %lld successful. New Balance: %.2f\n", withdrawalAmount, accountNumber, currentBalance - withdrawalAmount);
            }
            else
            {
                // Insufficient funds
                printf("Insufficient funds for withdrawal. Current balance: %.2f\n", currentBalance);
            }
        }
        else
        {
            // Account not found
            printf("Account number %lld not found.\n", accountNumber);
        }
    }
    else
    {
        fprintf(stderr, "Failed to prepare database statement.\n");
    }

    sqlite3_finalize(stmt); // Ensure the prepared statement is always finalized
}

void updateAccountDetails(sqlite3 *db)
{
    long long int accountNumber;
    char newFirstName[100], newLastName[100], newAddress[255];
    char sql[1024];

    printf("Enter account number to update: ");
    scanf("%lld", &accountNumber);
    getchar(); // Consume the newline character

    printf("Enter new first name: ");
    fgets(newFirstName, sizeof(newFirstName), stdin);
    newFirstName[strcspn(newFirstName, "\n")] = 0; // Remove newline character

    printf("Enter new last name: ");
    fgets(newLastName, sizeof(newLastName), stdin);
    newLastName[strcspn(newLastName, "\n")] = 0; // Remove newline character

    printf("Enter new address: ");
    fgets(newAddress, sizeof(newAddress), stdin);
    newAddress[strcspn(newAddress, "\n")] = 0; // Remove newline character

    // Update account details
    sprintf(sql, "UPDATE accounts SET first_name = '%s', last_name = '%s', address = '%s' WHERE id = %lld;", newFirstName, newLastName, newAddress, accountNumber);
    executeSQL(db, sql, callback, "Updating account details");

    printf("Account details updated successfully.\n");
}

int main()
{
    sqlite3 *db;
    if (sqlite3_open("bank.db", &db))
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    else
    {
        fprintf(stdout, "Opened database successfully\n");
    }

    int choice;
    do
    {
        printMainMenu();
        scanf("%d", &choice);
        getchar(); // Consume the newline character after scanf

        switch (choice)
        {
        case 1:
            printHeader("Create Account");
            createAccount(db);
            break;
        case 2:
            printHeader("View Accounts");
            viewAccounts(db);
            break;
        case 3:
            printHeader("View Transactions");
            viewTransactions(db);
            break;
        case 4:
            printHeader("Deposit Into Account");
            depositIntoAccount(db);
            break;
        case 5:
            printHeader("Withdraw From Account");
            withdrawFromAccount(db);
            break;
        case 6:
            printHeader("Update Account Details");
            updateAccountDetails(db);
            break;
        case 7:
            printHeader("Exiting Program");
            printf("Thank you for using the banking system. Goodbye!\n");
            break;
        default:
            printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 7);

    sqlite3_close(db);
    return 0;
}