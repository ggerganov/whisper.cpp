import subprocess
batcmd="./main -m models/ggml-medium.en.bin -f samples/jfk.wav -t 8 --print_colors --output-words"
result = subprocess.check_output(batcmd, shell=True)
print(str(result, 'utf-8'))