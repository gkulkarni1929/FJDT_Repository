import pyodbc

# Database connection parameters
connection_string = r"Driver={ODBC Driver 17 for SQL Server};Server=192.168.120.19\SCB-APP01\SQLEXPRESS,1433;Database=scbiiotdevices;Trusted_Connection=yes;"

def fetch_and_update_records():
    try:
        # Connect to the database
        conn = pyodbc.connect(connection_string)
        cursor = conn.cursor()

        # Query to fetch the two most recent records
        query = '''
        SELECT TOP 2 test_id, created_at, operator_first_name, project_id, panel_id, shift_id, sample_date, sample_time, specie, grade, dimension,
            test_result, max_load_reading, wood_failure_mode, fifth_uts_lbs
        FROM SampleResult
        ORDER BY created_at DESC
        '''
        cursor.execute(query)
        rows = cursor.fetchall()

        if len(rows) == 2:
            # Check if the panel IDs share the same prefix except the last character
            panel_id_1 = rows[0].panel_id
            panel_id_2 = rows[1].panel_id

            if panel_id_1[:-1] == panel_id_2[:-1] and panel_id_1[-1] != panel_id_2[-1]:
                print("2 halves:")
                for row in rows:
                    print(f"Project ID: {row.project_id}, Panel ID: {row.panel_id}, Shift ID: {row.shift_id}, Sample Date: {row.sample_date}, Sample Time: {row.sample_time}, Specie: {row.specie}, Grade: {row.grade}, Dimension: {row.dimension}, Test Result: {row.test_result}, Max Load Reading: {row.max_load_reading}, Wood Failure Mode: {row.wood_failure_mode}, Fifth UTS LBS: {row.fifth_uts_lbs}")

                # Check if test_result is empty for both records
                if not rows[0].test_result and not rows[1].test_result:
                    # Calculate averages
                    avg_max_load_reading = (rows[0].max_load_reading + rows[1].max_load_reading) / 2
                    avg_fifth_uts_lbs = (rows[0].fifth_uts_lbs + rows[1].fifth_uts_lbs) / 2

                    print(f"Average Max Load Reading: {avg_max_load_reading}")
                    print(f"Average Fifth UTS LBS: {avg_fifth_uts_lbs}")

                    # Determine the test result based on the averages
                    new_test_result = "pass" if avg_max_load_reading > avg_fifth_uts_lbs else "fail"

                    # Update the test_result for both records
                    update_query = """
                    UPDATE SampleResult
                    SET test_result = ?
                    WHERE test_id = ?
                    """
                    cursor.execute(update_query, (new_test_result, rows[0].test_id))
                    cursor.execute(update_query, (new_test_result, rows[1].test_id))
                    conn.commit()

                    print(f"Test results updated to '{new_test_result}' for test IDs {rows[0].test_id} and {rows[1].test_id}.")
                else:
                    print("Test results are already set for the two most recent records. No changes made.")

            else:
                print("The two most recent records do not have panel IDs that form 2 halves.")
        else:
            print("Not enough records to compare.")

        # Close the cursor and connection
        cursor.close()
        conn.close()

    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    while True:  # Run the check continuously
        fetch_and_update_records()
