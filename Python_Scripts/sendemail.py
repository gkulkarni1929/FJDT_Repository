import pyodbc
import smtplib
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart

# Database connection parameters
connection_string = r"Driver={ODBC Driver 17 for SQL Server};Server=192.168.120.19\SCB-APP01\SQLEXPRESS,1433;Database=scbiiotdevices;Trusted_Connection=yes;"

# Email settings
smtp_server = "smtp.office365.com"
smtp_port = 587
smtp_user = "gkulkarni@structurecraft.com"
smtp_password = "Damp-Clean-8#"  # Replace with your actual password

# List of recipients
recipients = "gkulkarni@structurecraft.com"

# Global variable to track the latest test result ID
last_test_id = None

def send_email(data):
    subject = "FJ Destructive Test Fail"
    
    # Formatting email body with all the data points line by line
    body = (
        f"created_at = {data.created_at}\n"
        f"operator_first_name = {data.operator_first_name}\n"
        f"project_id = {data.project_id}\n"
        f"panel_id = {data.panel_id}\n"
        f"shift_id = {data.shift_id}\n"
        f"sample_date = {data.sample_date}\n"
        f"sample_time = {data.sample_time}\n"
        f"specie = {data.specie}\n"
        f"grade = {data.grade}\n"
        f"dimension = {data.dimension}\n"
        f"mc_right = {data.mc_right}\n"
        f"mc_left = {data.mc_left}\n"
        f"test_result = {data.test_result}\n"
        f"max_psi_reading = {data.max_psi_reading}\n"
        f"max_load_reading = {data.max_load_reading}\n"
        f"wood_failure_mode = {data.wood_failure_mode}\n"
        f"min_ft_psi = {data.min_ft_psi}\n"
        f"fifth_ft_psi = {data.fifth_ft_psi}\n"
        f"min_uts_lbs = {data.min_uts_lbs}\n"
        f"fifth_uts_lbs = {data.fifth_uts_lbs}\n"
        f"adhesive_application = {data.adhesive_application}\n"
        f"squeeze_out = {data.squeeze_out}\n"
        f"adhesive_batch_test_result = {data.adhesive_batch_test_result}\n"
        f"finished_joint_appearance = {data.finished_joint_appearance}\n"
        f"positioning_alignment = {data.positioning_alignment}"
    )

    # Email message setup
    msg = MIMEMultipart()
    msg['From'] = smtp_user
    msg['To'] = ", ".join(recipients)
    msg['Subject'] = subject
    msg['X-Priority'] = '1'  # High importance
    msg['Importance'] = 'High'  # For some email clients

    msg.attach(MIMEText(body, 'plain'))

    # Sending the email
    try:
        server = smtplib.SMTP(smtp_server, smtp_port)
        server.starttls()  # Secure the connection
        server.login(smtp_user, smtp_password)
        server.sendmail(smtp_user, recipients, msg.as_string())
        server.quit()
        print("Email sent successfully!")
    except Exception as e:
        print(f"Failed to send email: {e}")

def check_test_result():
    global last_test_id  # Declare the flag as global to modify its state
    try:
        # Connect to the database
        conn = pyodbc.connect(connection_string)
        cursor = conn.cursor()

        # Query to get the most recent test result ordered by created_at
        query = '''
        SELECT TOP 1 test_id, created_at, operator_first_name, project_id, panel_id, shift_id, sample_date, sample_time, specie, grade, dimension,
            mc_right, mc_left, test_result, max_psi_reading, max_load_reading, wood_failure_mode, min_ft_psi,
            fifth_ft_psi, min_uts_lbs, fifth_uts_lbs, adhesive_application, squeeze_out, adhesive_batch_test_result,
            finished_joint_appearance, positioning_alignment
        FROM SampleResult
        ORDER BY created_at DESC
        '''
        cursor.execute(query)
        row = cursor.fetchone()

        if row:
            # Check if the current row's test ID is different from the last processed one
            if row.test_id != last_test_id:
                last_test_id = row.test_id  # Update to the latest test ID

                # Check if the test_result is "fail"
                if row.test_result.lower() == "fail":
                    send_email(row)
                else:
                    print("No recent fail test result.")
            else:
                print("No new test results.")
        else:
            print("No test results found.")

        cursor.close()
        conn.close()

    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    while True:  # Run the check continuously
        check_test_result()
