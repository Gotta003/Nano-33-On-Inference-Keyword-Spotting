#/bin/bash
set -e
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'

info() {
    echo -e "${CYAN}[INFO]${RESET} $*";
}
success() {
    echo -e "${GREEN}[OK]${RESET} $*";
}
warn() {
    echo -e "${YELLOW}[WARN]${RESET} $*";
}
error() {
    echo -e "${RED}[ERROR]${RESET} $*";
}

maybe_install() {
    if command -v apt-get >/dev/null 2>&1; then
        info "Installing $1..."
        sudo apt-get install "$1"
    else
        echo "PAckage manager 'apt-get' not found."
    fi
    success "Installed $1"
}

maybe_install xxd
# 0) Check python is installed
#PYTHON3
info "Checking python3 is installed...";
command -v python3 >/dev/null 2>&1 || { error "Python3 is not installed. Please install Python3 and try again."; exit 1; }
PYTHON_VERSION=$(python3 -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')");
PYTHON_MAJOR=$(echo $PYTHON_VERSION | cut -d. -f1);
PYTHON_MINOR=$(echo $PYTHON_VERSION | cut -d. -f2);
if [[ $PYTHON_MAJOR -lt 3 || ($PYTHON_MAJOR -eq 3 && $PYTHON_MINOR -lt 12) ]]; then
    error "Python 3.12+ required (found $PYTHON_VERSION)."
fi
success "Python3 is installed (version $PYTHON_VERSION).";
#PIP3
command -v pip3 >/dev/null 2>&1 || error "pip3 not found"
success "pip3 is installed."

VENV_DIR="../.tf-env"
if [ -d "$VENV_DIR" ]; then
    warn "Virtual environment directory '$VENV_DIR' already exists. Skipping creation."
    warn "To rebuild from scratch: rm -rf $VENV_DIR && ./setup.sh"
else
    info "Creating virtual environment in './$VENV_DIR'..."
    python3 -m venv $VENV_DIR
    success "Virtual environment created"
fi
source "$VENV_DIR/bin/activate"
success "Virtual environment activated"

info "Upgrading pip, setuptools, wheel"
pip install --upgrade pip setuptools wheel -q
success "pip upgraded"

if command -v nvidia-smi >/dev/null 2>&1; then
    echo "GPU detected. Installing TensorFlow with CUDA support..."
    pip install tensorflow[and-cuda]
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$VENV_DIR/lib/python$PYTHON_VERSION/site-packages/nvidia/cudnn/lib:$VENV_DIR/lib/python$PYTHON_VERSION/site-packages/nvidia/cuda_runtime/lib
else
    echo "No GPU detected. Installing standard CPU TensorFlow..."
    pip install tensorflow
fi

if [ -f "requirements.txt" ]; then
    info "Installing packages from requirements.txt..."
    pip install -r requirements.txt -q
    success "Packages installed"
else 
    warn "No requirements.txt found"
fi

info "Checking package installations..."
python3 packages_checker.py 
success "All packages are installed correctly"