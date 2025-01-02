from flask import Flask, request, jsonify
import pyodbc

app = Flask(__name__)

# Define your connection parameters
connection_string = r"Driver={ODBC Driver 17 for SQL Server};Server=192.168.120.19,1433\SCB-APP01\SQLEXPRESS;Database=scbiiotdevices;Trusted_Connection=yes;"

@app.route('/endpoint', methods=['POST'])
def handle_data():
    if request.method == 'POST':
        # Parse JSON data from the request
        data = request.json
        if not data:
            return jsonify({"status": "error", "message": "No JSON data provided"}), 400

        # Extract values from the incoming JSON
        try:
            operator_first_name = data.get("operator_first_name")
            project_id = data.get("project_id")
            panel_id = data.get("panel_id")
            shift_id = data.get("shift_id")
            sample_date = data.get("sample_date")
            sample_time = data.get("sample_time")
            specie = data.get("specie")
            grade = data.get("grade")
            dimension = data.get("dimension")
            mc_right = data.get("mc_right")
            mc_left = data.get("mc_left")
            test_result = data.get("test_result")
            max_psi_reading = data.get("max_psi_reading")
            max_load_reading = data.get("max_load_reading")
            wood_failure_mode = data.get("wood_failure_mode")
            min_ft_psi = data.get("min_ft_psi")
            fifth_ft_psi = data.get("fifth_ft_psi")
            min_uts_lbs = data.get("min_uts_lbs")
            fifth_uts_lbs = data.get("fifth_uts_lbs")
            adhesive_application = data.get("adhesive_application")
            squeeze_out = data.get("squeeze_out")
            adhesive_batch_test_result = data.get("adhesive_batch_test_result")
            finished_joint_appearance = data.get("finished_joint_appearance")
            positioning_alignment = data.get("positioning_alignment")

            # Establish the connection
            conn = pyodbc.connect(connection_string)
            cursor = conn.cursor()

            # Query to get the device_id for the specified device_name
            device_name = "FJ Destructive Test Device"
            device_query = "SELECT device_id FROM Device WHERE device_name = ?"
            cursor.execute(device_query, (device_name,))
            device_result = cursor.fetchone()

            if device_result:
                device_id = device_result.device_id

                # Define the SQL INSERT statement with placeholders
                insert_query = '''
                INSERT INTO SampleResult (
                    device_id, operator_first_name, project_id, panel_id, shift_id, sample_date, sample_time, specie, grade, dimension,
                    mc_right, mc_left, test_result, max_psi_reading, max_load_reading, wood_failure_mode, min_ft_psi,
                    fifth_ft_psi, min_uts_lbs, fifth_uts_lbs, adhesive_application, squeeze_out, adhesive_batch_test_result,
                    finished_joint_appearance, positioning_alignment
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                '''

                # Define the values to be inserted
                values = (
                    device_id, operator_first_name, project_id, panel_id, shift_id, sample_date, sample_time, specie, grade, dimension,
                    mc_right, mc_left, test_result, max_psi_reading, max_load_reading, wood_failure_mode, min_ft_psi,
                    fifth_ft_psi, min_uts_lbs, fifth_uts_lbs, adhesive_application, squeeze_out, adhesive_batch_test_result,
                    finished_joint_appearance, positioning_alignment
                )

                # Ensure that the number of values matches the number of placeholders (25)
                if len(values) == 25:
                    # Execute the query
                    cursor.execute(insert_query, values)

                    # Commit the transaction
                    conn.commit()

                    # Retrieve the last inserted test_id based on created_at
                    cursor.execute('''
                    SELECT TOP 1 test_id
                    FROM SampleResult
                    ORDER BY created_at DESC
                    ''')
                    last_id_result = cursor.fetchone()
                    last_test_id = last_id_result.test_id if last_id_result else None

                    # Close the connection
                    cursor.close()
                    conn.close()

                    response = {
                        "status": "success",
                        "message": "Data inserted successfully!",
                        "test_id": last_test_id
                    }

                    # Print the test_id
                    print(f"Last inserted test_id: {last_test_id}")

                else:
                    response = {
                        "status": "error",
                        "message": "Mismatch between number of values and placeholders."
                    }
            else:
                response = {
                    "status": "error",
                    "message": f"Device with name '{device_name}' not found."
                }
        except Exception as e:
            response = {
                "status": "error",
                "message": f"An error occurred: {e}"
            }

        return jsonify(response), 200
    else:
        return jsonify({"error": "Method not allowed"}), 405



if __name__ == '__main__':
    print("FJDT is running on APP01")
    app.run(host='0.0.0.0', port=5000)

 
