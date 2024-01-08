import subprocess
import sys

# Command and its arguments
command = "./main"
model = "-m ./models/ggml-base.en.bin"
file_path = "-f " + sys.argv[1]  # Get the first parameter as the wav file path

# Combine into a single command
full_command = f"{command} {model} {file_path}"

# Execute the command and capture the output
process = subprocess.Popen(full_command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
output, error = process.communicate()

if not bool(error.decode('utf-8').strip()): 
    # The output variable contains the command's standard output
    decoded_str = output.decode('utf-8').strip()
    processed_str = decoded_str.replace('[BLANK_AUDIO]', '').strip()
    print("Output:", processed_str)    
else: 
    # The error variable contains the command's standard error
    print("Error:", error.decode('utf-8'))
