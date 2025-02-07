#!/bin/bash

# Exit on error
set -e

git submodule update --init --recursive -q

cd whisper.cpp

# Build Whisper server
make -j4

# Configuration
PACKAGE_NAME="whisper-server-package"
MODEL_NAME="ggml-small.bin"


# Check if package directory exists
if [ -d "$PACKAGE_NAME" ]; then
    echo "Package directory already exists: $PACKAGE_NAME"
    exit 1
fi

MODEL_DIR="$PACKAGE_NAME/models"

# If model directory does not exist, create it
if [ ! -d "$MODEL_DIR" ]; then  
    # Create package directory
    mkdir -p "$PACKAGE_NAME"
    mkdir -p "$PACKAGE_NAME/models"
    mkdir -p "$PACKAGE_NAME/public"
fi



# Copy server binary
cp bin/whisper-server "$PACKAGE_NAME/"

# Copy model file

# Check for existing model
echo "Checking for Whisper models..."

# There will be multiple models in the directory
EXISTING_MODELS=$(find "$MODEL_DIR" -name "ggml-*.bin" -type f)

echo "Existing models: $EXISTING_MODELS"

# Whisper models
models="tiny
tiny.en
tiny-q5_1
tiny.en-q5_1
tiny-q8_0
base
base.en
base-q5_1
base.en-q5_1
base-q8_0
small
small.en
small.en-tdrz
small-q5_1
small.en-q5_1
small-q8_0
medium
medium.en
medium-q5_0
medium.en-q5_0
medium-q8_0
large-v1
large-v2
large-v2-q5_0
large-v2-q8_0
large-v3
large-v3-q5_0
large-v3-turbo
large-v3-turbo-q5_0
large-v3-turbo-q8_0"

# Ask user which model to use if the argument is not provided
if [ -z "$1" ]; then
    # Let user interactively select a model name
    echo "Available models: $models"
    read -p "Enter a model name (e.g. small): " MODEL_SHORT_NAME
else
    MODEL_SHORT_NAME=$1
fi

# Check if the model is valid
if ! echo "$models" | grep -qw "$MODEL_SHORT_NAME"; then
    echo "Invalid model: $MODEL_SHORT_NAME"
    exit 1
fi

MODEL_NAME="ggml-$MODEL_SHORT_NAME.bin"

# Check if the modelname exists in directory
if [ -f "$MODEL_DIR/$MODEL_NAME" ]; then
    echo "Model file exists: $MODEL_DIR/$MODEL_NAME"
else
    echo "Model file does not exist: $MODEL_DIR/$MODEL_NAME"
    echo "Trying to download model..."
    ./models/download-ggml-model.sh $MODEL_SHORT_NAME
    # Move model to models directory
    mv "$PACKAGE_NAME/$MODEL_NAME" "$MODEL_DIR/"
fi

# Copy web interface files
cp -r examples/server/public/* "$PACKAGE_NAME/public/"

# Create run script
cat > "$PACKAGE_NAME/run-server.sh" << 'EOL'
#!/bin/bash

# Default configuration
HOST="127.0.0.1"
PORT="8178"
MODEL="models/ggml-large-v3.bin"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --host)
            HOST="$2"
            shift 2
            ;;
        --port)
            PORT="$2"
            shift 2
            ;;
        --model)
            MODEL="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Run the server
./whisper-server \
    --model "$MODEL" \
    --host "$HOST" \
    --port "$PORT" \
    --diarize \
    --print-progress


EOL

# Make run script executable
chmod +x "$PACKAGE_NAME/run-server.sh"


# Start the whisper server in background
cd "$PACKAGE_NAME"
./run-server.sh --model "models/$MODEL_NAME" &
WHISPER_PID=$!
cd ..

sleep 2

# Start the Python backend in background
if [ -z "$VIRTUAL_ENV" ]; then
    echo "Activating venv..."
    source venv/bin/activate
fi

echo "Starting Python backend..."
python app/main.py &
PYTHON_PID=$!

# Cleanup function
cleanup() {
    echo "Cleaning up..."
    if [ -n "$WHISPER_PID" ]; then
        echo "Stopping Whisper server..."
        kill -9 $WHISPER_PID 2>/dev/null || true
        pkill -9 -f "whisper-server" 2>/dev/null || true
    fi
    if [ -n "$PYTHON_PID" ]; then
        echo "Stopping Python backend..."
        kill -9 $PYTHON_PID 2>/dev/null || true
    fi
    exit 0
}

# Set up trap for cleanup
trap cleanup EXIT INT TERM

# Keep the script running and wait for both processes
wait $WHISPER_PID $PYTHON_PID
