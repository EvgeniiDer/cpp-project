#!/usr/bin/env bash
# Обёртка: активирует venv, ставит недостающие пакеты, запускает обучение.
# Запуск из WSL:
#   cd /mnt/b/workspace/www.github.com/cpp-project/trade-terminal/QuantumTrader/ml
#   bash train.sh
set -e

VENV_PATH="${VENV_PATH:-$HOME/ml_engine/venv}"

if [ ! -f "$VENV_PATH/bin/activate" ]; then
    echo "[train.sh] venv не найден по пути $VENV_PATH (переопредели через VENV_PATH=...)" >&2
    exit 1
fi

# shellcheck disable=SC1091
source "$VENV_PATH/bin/activate"

python3 -c "import sklearn" 2>/dev/null || pip install scikit-learn

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
python3 "$SCRIPT_DIR/train_catboost.py" "$@"
